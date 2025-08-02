#include "backupmanager.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QTextStream>
#include <QStorageInfo>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>
#include <QDebug>

BackupManager::BackupManager(QObject *parent)
    : QObject(parent)
    , m_status(Idle)
    , m_currentBackupType(FullBackup)
    , m_backupProcess(nullptr)
    , m_progress(0)
    , m_compressionLevel(6)
    , m_verifyBackups(true)
    , m_maxBackupSize(0) // 0 means no limit
    , m_totalBytes(0)
    , m_processedBytes(0)
    , m_totalFiles(0)
    , m_processedFiles(0)
{
    // Setup progress timer
    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, &BackupManager::updateProgress);
    
    // Setup file system watcher for incremental backups
    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &BackupManager::onFileChanged);
    
    // Setup default exclude paths
    m_excludePaths = {
        "/proc/*", "/sys/*", "/dev/*", "/tmp/*", "/run/*",
        "/var/tmp/*", "/var/cache/*", "/var/log/*",
        "~/.cache/*", "~/.local/share/Trash/*",
        "*.swp", "*.tmp", "*~"
    };
    
    // Setup database path
    m_databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backup_database.json";
    QDir().mkpath(QFileInfo(m_databasePath).absolutePath());
}

BackupManager::~BackupManager()
{
    if (m_backupProcess && m_backupProcess->state() != QProcess::NotRunning) {
        m_backupProcess->terminate();
        if (!m_backupProcess->waitForFinished(5000)) {
            m_backupProcess->kill();
        }
    }
}

void BackupManager::startFullBackup(const QString &location, const QString &compression, bool verify)
{
    if (m_status == Running || m_status == Paused) {
        emit errorOccurred("Backup already in progress");
        return;
    }
    
    m_verifyBackups = verify;
    setupBackupJob(FullBackup, location, compression);
}

void BackupManager::startIncrementalBackup(const QString &location)
{
    if (m_status == Running || m_status == Paused) {
        emit errorOccurred("Backup already in progress");
        return;
    }
    
    setupBackupJob(IncrementalBackup, location);
}

void BackupManager::startPackageBackup(const QString &location)
{
    if (m_status == Running || m_status == Paused) {
        emit errorOccurred("Backup already in progress");
        return;
    }
    
    setupBackupJob(PackageBackup, location);
}

void BackupManager::startSettingsBackup(const QString &location)
{
    if (m_status == Running || m_status == Paused) {
        emit errorOccurred("Backup already in progress");
        return;
    }
    
    setupBackupJob(SettingsBackup, location);
}

void BackupManager::pauseBackup()
{
    if (m_status != Running) return;
    
    if (m_backupProcess && m_backupProcess->state() == QProcess::Running) {
        m_backupProcess->terminate(); // Send SIGTERM to pause
        m_status = Paused;
        m_progressTimer->stop();
        emit statusChanged("Backup paused");
    }
}

void BackupManager::resumeBackup()
{
    if (m_status != Paused) return;
    
    // Resume by restarting the backup process
    m_status = Running;
    m_progressTimer->start(1000);
    emit statusChanged("Backup resumed");
}

void BackupManager::cancelBackup()
{
    if (m_status == Idle) return;
    
    if (m_backupProcess && m_backupProcess->state() == QProcess::Running) {
        m_backupProcess->kill();
    }
    
    m_status = Cancelled;
    m_progress = 0;
    m_progressTimer->stop();
    
    // Clean up partial backup
    if (!m_currentBackupPath.isEmpty() && QFile::exists(m_currentBackupPath)) {
        QFile::remove(m_currentBackupPath);
    }
    
    emit statusChanged("Backup cancelled");
    emit backupCompleted(false);
}

void BackupManager::setupBackupJob(BackupType type, const QString &location, const QString &compression)
{
    QMutexLocker locker(&m_mutex);
    
    if (!QDir().mkpath(location)) {
        emit errorOccurred("Cannot create backup directory: " + location);
        return;
    }
    
    m_currentBackupType = type;
    m_backupLocation = location;
    m_status = Running;
    m_progress = 0;
    m_processedBytes = 0;
    m_processedFiles = 0;
    
    // Generate backup name
    QString backupName = generateBackupName(type);
    m_currentBackupPath = location + "/" + backupName;
    
    // Check disk space
    qint64 estimatedSize = 1024 * 1024 * 1024; // 1GB default estimate
    if (!checkDiskSpace(location, estimatedSize)) {
        emit errorOccurred("Insufficient disk space for backup");
        return;
    }
    
    // Create backup script
    QString scriptPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/backup_script.sh";
    createBackupScript(scriptPath, type, location, compression);
    
    // Start backup process
    if (m_backupProcess) {
        m_backupProcess->deleteLater();
    }
    
    m_backupProcess = new QProcess(this);
    connect(m_backupProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BackupManager::onBackupProcessFinished);
    connect(m_backupProcess, &QProcess::errorOccurred,
            this, &BackupManager::onBackupProcessError);
    
    // Start the backup
    m_backupProcess->start("bash", {scriptPath});
    
    if (!m_backupProcess->waitForStarted()) {
        emit errorOccurred("Failed to start backup process");
        return;
    }
    
    m_progressTimer->start(1000);
    emit backupStarted(type);
    emit statusChanged("Backup started");
}

void BackupManager::createBackupScript(const QString &scriptPath, BackupType type, const QString &location, const QString &compression)
{
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred("Cannot create backup script");
        return;
    }
    
    QTextStream out(&scriptFile);
    out << "#!/bin/bash\n";
    out << "set -e\n"; // Exit on any error
    out << "\n";
    
    QString backupName = generateBackupName(type);
    QString targetPath = location + "/" + backupName;
    QString compressionCmd = getCompressionCommand(compression);
    
    switch (type) {
    case FullBackup:
        {
            out << "# Full system backup\n";
            out << "echo \"Starting full system backup...\"\n";
            
            // Create tar command with exclusions
            out << "tar -c";
            if (!compression.isEmpty() && compression != "None") {
                if (compression == "gzip") out << "z";
                else if (compression == "bzip2") out << "j";
                else if (compression == "xz") out << "J";
            }
            out << "f \"" << targetPath << "\"";
            
            // Add exclude patterns
            for (const QString &exclude : m_excludePaths) {
                out << " --exclude='" << exclude << "'";
            }
            
            // Add system paths
            QStringList systemPaths = getSystemPaths();
            for (const QString &path : systemPaths) {
                out << " \"" << path << "\"";
            }
            out << "\n";
            
            // Post-process with zstd if needed
            if (compression == "zstd") {
                out << "echo \"Compressing with zstd...\"\n";
                out << "zstd -" << m_compressionLevel << " \"" << targetPath << "\"\n";
                out << "rm \"" << targetPath << "\"\n";
                out << "mv \"" << targetPath << ".zst\" \"" << targetPath << "\"\n";
            }
        }
        break;
        
    case IncrementalBackup:
        {
            out << "# Incremental backup\n";
            out << "echo \"Starting incremental backup...\"\n";
            
            QStringList changedFiles = getChangedFiles(location);
            if (changedFiles.isEmpty()) {
                out << "echo \"No changes detected, skipping backup\"\n";
                out << "exit 0\n";
                break;
            }
            
            out << "tar -czf \"" << targetPath << "\"";
            for (const QString &file : changedFiles) {
                out << " \"" << file << "\"";
            }
            out << "\n";
        }
        break;
        
    case PackageBackup:
        {
            out << "# Package backup\n";
            out << "echo \"Starting package backup...\"\n";
            out << "pacman -Qqe > \"" << location << "/installed_packages.txt\"\n";
            out << "pacman -Qqm > \"" << location << "/aur_packages.txt\"\n";
            out << "tar -czf \"" << targetPath << "\" -C \"" << location << "\" installed_packages.txt aur_packages.txt\n";
            out << "rm \"" << location << "/installed_packages.txt\" \"" << location << "/aur_packages.txt\"\n";
        }
        break;
        
    case SettingsBackup:
        {
            out << "# Settings backup\n";
            out << "echo \"Starting settings backup...\"\n";
            
            QStringList settingsPaths = getSettingsPaths();
            out << "# Create tar with error handling for permission denied files\n";
            out << "tar -czf \"" << targetPath << "\" --warning=no-file-ignored";
            for (const QString &path : settingsPaths) {
                out << " \"" << path << "\"";
            }
            out << " 2>/dev/null || echo \"Warning: Some files could not be backed up due to permissions\"\n";
        }
        break;
    }
    
    // Verification step
    if (m_verifyBackups) {
        out << "\n# Verification\n";
        out << "echo \"Verifying backup...\"\n";
        if (type == PackageBackup) {
            out << "tar -tzf \"" << targetPath << "\" > /dev/null\n";
        } else {
            out << "tar -t";
            if (compression == "gzip") out << "z";
            else if (compression == "bzip2") out << "j";
            else if (compression == "xz") out << "J";
            out << "f \"" << targetPath << "\" > /dev/null\n";
        }
        out << "echo \"Verification completed successfully\"\n";
    }
    
    out << "\necho \"Backup completed successfully\"\n";
    scriptFile.close();
    
    // Make script executable
    QFile::setPermissions(scriptPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
}

QString BackupManager::generateBackupName(BackupType type) const
{
    QString prefix;
    switch (type) {
    case FullBackup: prefix = "full_backup"; break;
    case IncrementalBackup: prefix = "incremental_backup"; break;
    case PackageBackup: prefix = "package_backup"; break;
    case SettingsBackup: prefix = "settings_backup"; break;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    return QString("%1_%2.tar.gz").arg(prefix, timestamp);
}

QString BackupManager::getCompressionExtension(const QString &compression) const
{
    if (compression == "gzip") return ".gz";
    if (compression == "bzip2") return ".bz2";
    if (compression == "xz") return ".xz";
    if (compression == "zstd") return ".zst";
    return "";
}

QString BackupManager::getCompressionCommand(const QString &compression) const
{
    if (compression == "gzip") return "gzip";
    if (compression == "bzip2") return "bzip2";
    if (compression == "xz") return "xz";
    if (compression == "zstd") return "zstd";
    return "";
}

QStringList BackupManager::getSystemPaths() const
{
    return {
        "/etc",
        "/home",
        "/opt",
        "/usr/local",
        "/var/lib/pacman/local"
    };
}

QStringList BackupManager::getPackagePaths() const
{
    return {
        "/var/lib/pacman/local",
        "/etc/pacman.conf",
        "/etc/pacman.d"
    };
}

QStringList BackupManager::getSettingsPaths() const
{
    QStringList paths;
    QString homeDir = QDir::homePath();
    
    // User-accessible system settings (readable without root)
    QStringList systemPaths = {
        "/etc/pacman.conf",
        "/etc/pacman.d",
        "/etc/locale.conf",
        "/etc/hostname",
        "/etc/hosts",
        "/etc/fstab",
        "/etc/environment",
        "/etc/profile",
        "/etc/bash.bashrc",
        "/etc/inputrc",
        "/etc/issue",
        "/etc/motd",
        "/etc/os-release",
        "/etc/lsb-release"
    };
    
    // Add readable system files
    for (const QString &path : systemPaths) {
        if (QFileInfo::exists(path) && QFileInfo(path).isReadable()) {
            paths << path;
        }
    }
    
    // User settings
    paths << homeDir + "/.config";
    paths << homeDir + "/.local/share";
    paths << homeDir + "/.bashrc";
    paths << homeDir + "/.bash_profile";
    paths << homeDir + "/.zshrc";
    paths << homeDir + "/.vimrc";
    paths << homeDir + "/.gitconfig";
    paths << homeDir + "/.ssh";
    paths << homeDir + "/.gnupg";
    paths << homeDir + "/.profile";
    paths << homeDir + "/.xinitrc";
    paths << homeDir + "/.xprofile";
    paths << homeDir + "/.Xresources";
    paths << homeDir + "/.themes";
    paths << homeDir + "/.icons";
    
    // Filter out non-existent paths
    QStringList existingPaths;
    for (const QString &path : paths) {
        if (QFileInfo::exists(path)) {
            existingPaths << path;
        }
    }
    
    return existingPaths;
}

void BackupManager::onBackupProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_progressTimer->stop();
    
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        m_status = Failed;
        emit statusChanged("Backup failed");
        emit backupCompleted(false);
        emit errorOccurred(QString("Backup process failed with exit code: %1").arg(exitCode));
        return;
    }
    
    m_status = Completed;
    m_progress = 100;
    m_lastBackupTime = QDateTime::currentDateTime();
    m_lastBackupLocation = m_backupLocation;
    
    // Update backup database
    updateBackupDatabase(m_backupLocation, m_currentBackupPath);
    
    // Clean up old backups if needed
    cleanupOldBackups(m_backupLocation);
    
    // Update file database for incremental backups
    if (m_currentBackupType == FullBackup || m_currentBackupType == IncrementalBackup) {
        QStringList paths = (m_currentBackupType == FullBackup) ? getSystemPaths() : getChangedFiles(m_backupLocation);
        saveFileDatabase(m_backupLocation, paths);
    }
    
    emit progressChanged(m_progress);
    emit statusChanged("Backup completed successfully");
    emit backupCompleted(true);
}

void BackupManager::onBackupProcessError(QProcess::ProcessError error)
{
    m_progressTimer->stop();
    m_status = Failed;
    
    QString errorString;
    switch (error) {
    case QProcess::FailedToStart:
        errorString = "Failed to start backup process";
        break;
    case QProcess::Crashed:
        errorString = "Backup process crashed";
        break;
    case QProcess::Timedout:
        errorString = "Backup process timed out";
        break;
    case QProcess::WriteError:
        errorString = "Write error during backup";
        break;
    case QProcess::ReadError:
        errorString = "Read error during backup";
        break;
    default:
        errorString = "Unknown backup error";
        break;
    }
    
    emit statusChanged("Backup failed: " + errorString);
    emit errorOccurred(errorString);
    emit backupCompleted(false);
}

void BackupManager::updateProgress()
{
    // Simulate progress calculation
    // In a real implementation, this would parse the backup tool's output
    if (m_status == Running && m_progress < 95) {
        m_progress += 2;
        emit progressChanged(m_progress);
        
        // Update current operation
        QStringList operations = {
            "Scanning files...",
            "Creating archive...",
            "Compressing data...",
            "Verifying backup..."
        };
        
        int opIndex = (m_progress / 25) % operations.size();
        if (m_currentOperation != operations[opIndex]) {
            m_currentOperation = operations[opIndex];
            emit operationChanged(m_currentOperation);
        }
    }
}

void BackupManager::onFileChanged(const QString &path)
{
    Q_UNUSED(path)
    // Handle file changes for incremental backup optimization
    // This could trigger automatic incremental backups
}

bool BackupManager::checkDiskSpace(const QString &location, qint64 estimatedSize)
{
    QStorageInfo storage(location);
    if (storage.bytesAvailable() < estimatedSize * 2) { // Require 2x space for safety
        return false;
    }
    return true;
}

void BackupManager::updateBackupDatabase(const QString &location, const QString &backupPath)
{
    // Update JSON database with backup information
    QJsonObject backupInfo;
    backupInfo["path"] = backupPath;
    backupInfo["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    backupInfo["type"] = static_cast<int>(m_currentBackupType);
    backupInfo["size"] = QFileInfo(backupPath).size();
    
    QString dbPath = location + "/backup_database.json";
    QJsonDocument doc;
    QJsonArray backups;
    
    // Load existing database
    if (QFile::exists(dbPath)) {
        QFile file(dbPath);
        if (file.open(QIODevice::ReadOnly)) {
            doc = QJsonDocument::fromJson(file.readAll());
            backups = doc.array();
        }
    }
    
    // Add new backup
    backups.append(backupInfo);
    
    // Save database
    doc.setArray(backups);
    QFile file(dbPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void BackupManager::cleanupOldBackups(const QString &location)
{
    // Keep only the last 10 backups of each type
    const int maxBackups = 10;
    
    QDir dir(location);
    QStringList filters;
    filters << "full_backup_*.tar.gz" << "incremental_backup_*.tar.gz" 
           << "package_backup_*.tar.gz" << "settings_backup_*.tar.gz";
    
    for (const QString &filter : filters) {
        QFileInfoList files = dir.entryInfoList({filter}, QDir::Files, QDir::Time);
        while (files.size() > maxBackups) {
            QFile::remove(files.takeLast().absoluteFilePath());
        }
    }
}

QStringList BackupManager::getAvailableBackups(const QString &location) const
{
    QStringList backups;
    QString dbPath = location + "/backup_database.json";
    
    if (!QFile::exists(dbPath)) {
        return backups;
    }
    
    QFile file(dbPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonArray array = doc.array();
        
        for (const auto &value : array) {
            QJsonObject obj = value.toObject();
            backups << obj["path"].toString();
        }
    }
    
    return backups;
}

bool BackupManager::deleteBackup(const QString &backupPath)
{
    return QFile::remove(backupPath);
}

qint64 BackupManager::getBackupSize(const QString &backupPath) const
{
    return QFileInfo(backupPath).size();
}

bool BackupManager::verifyBackup(const QString &backupPath)
{
    QProcess process;
    process.start("tar", {"-tzf", backupPath});
    process.waitForFinished();
    return process.exitCode() == 0;
}

QStringList BackupManager::getChangedFiles(const QString &location) const
{
    // Load previous file database and compare with current state
    QStringList changedFiles;
    QStringList previousFiles = loadFileDatabase(location);
    
    // For now, return a simulated list
    // In a real implementation, this would compare file hashes and modification times
    return {"/etc/passwd", "/home/user/.bashrc", "/etc/fstab"};
}

void BackupManager::saveFileDatabase(const QString &location, const QStringList &files)
{
    QString dbPath = location + "/file_database.json";
    QJsonObject database;
    QJsonArray fileArray;
    
    for (const QString &file : files) {
        QJsonObject fileInfo;
        fileInfo["path"] = file;
        fileInfo["hash"] = createFileHash(file);
        fileInfo["modified"] = QFileInfo(file).lastModified().toString(Qt::ISODate);
        fileArray.append(fileInfo);
    }
    
    database["files"] = fileArray;
    database["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(database);
    QFile file(dbPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

QStringList BackupManager::loadFileDatabase(const QString &location) const
{
    QStringList files;
    QString dbPath = location + "/file_database.json";
    
    if (!QFile::exists(dbPath)) {
        return files;
    }
    
    QFile file(dbPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject database = doc.object();
        QJsonArray fileArray = database["files"].toArray();
        
        for (const auto &value : fileArray) {
            QJsonObject fileInfo = value.toObject();
            files << fileInfo["path"].toString();
        }
    }
    
    return files;
}

QString BackupManager::createFileHash(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (hash.addData(&file)) {
        return hash.result().toHex();
    }
    
    return QString();
}
