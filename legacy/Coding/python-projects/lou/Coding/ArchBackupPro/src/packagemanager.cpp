#include "packagemanager.h"
#include <QProcess>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QRegularExpression>

PackageManager::PackageManager(QObject *parent)
    : QObject(parent)
    , m_pacmanProcess(nullptr)
    , m_refreshInProgress(false)
{
}

PackageManager::~PackageManager()
{
    if (m_pacmanProcess && m_pacmanProcess->state() != QProcess::NotRunning) {
        m_pacmanProcess->terminate();
        m_pacmanProcess->waitForFinished(3000);
    }
}

void PackageManager::refreshPackageList()
{
    if (m_refreshInProgress) return;
    
    m_refreshInProgress = true;
    emit operationProgress("Refreshing package list...", 0);
    
    // Get list of explicitly installed packages
    QString output = runPacmanCommand({"-Qe"});
    parsePackageList(output);
    
    m_lastRefreshTime = QDateTime::currentDateTime();
    m_refreshInProgress = false;
    
    emit operationProgress("Package list refreshed", 100);
    emit packageListRefreshed();
}

QList<PackageInfo> PackageManager::getInstalledPackages() const
{
    return m_installedPackages;
}

QList<PackageInfo> PackageManager::getExplicitPackages() const
{
    QList<PackageInfo> explicit_packages;
    for (const auto &pkg : m_installedPackages) {
        if (pkg.isExplicit) {
            explicit_packages.append(pkg);
        }
    }
    return explicit_packages;
}

QList<PackageInfo> PackageManager::getAURPackages() const
{
    QList<PackageInfo> aur_packages;
    for (const auto &pkg : m_installedPackages) {
        if (pkg.isAUR) {
            aur_packages.append(pkg);
        }
    }
    return aur_packages;
}

int PackageManager::getInstalledPackageCount() const
{
    return m_installedPackages.size();
}

PackageInfo PackageManager::getPackageInfo(const QString &packageName) const
{
    return m_packageCache.value(packageName, PackageInfo());
}

void PackageManager::backupPackageList(const QString &location)
{
    QDir().mkpath(location);
    
    // First refresh the package list to ensure we have current data
    emit operationProgress("Refreshing package list...", 10);
    refreshPackageList();
    
    // Export files
    QString explicitFile = location + "/installed_packages.txt";
    QString aurFile = location + "/aur_packages.txt";
    QString allPackagesFile = location + "/all_packages.txt";
    QString dependenciesFile = location + "/package_dependencies.txt";
    QString restoreScript = location + "/restore_packages.sh";
    
    emit operationProgress("Exporting package lists...", 30);
    
    // Export explicitly installed packages
    QFile explicitFileHandle(explicitFile);
    if (explicitFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&explicitFileHandle);
        QList<PackageInfo> explicitPackages = getExplicitPackages();
        for (const auto &pkg : explicitPackages) {
            out << pkg.name << " " << pkg.version << "\n";
        }
        explicitFileHandle.close();
    }
    
    emit operationProgress("Exporting AUR packages...", 50);
    
    // Export AUR packages separately
    QFile aurFileHandle(aurFile);
    if (aurFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&aurFileHandle);
        QList<PackageInfo> aurPackages = getAURPackages();
        for (const auto &pkg : aurPackages) {
            out << pkg.name << " " << pkg.version << "\n";
        }
        aurFileHandle.close();
    }
    
    emit operationProgress("Exporting all packages with dependencies...", 70);
    
    // Export all installed packages (including dependencies)
    QString allPackagesOutput = runPacmanCommand({"-Q"});
    QFile allPackagesFileHandle(allPackagesFile);
    if (allPackagesFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        allPackagesFileHandle.write(allPackagesOutput.toUtf8());
        allPackagesFileHandle.close();
    }
    
    // Export package dependencies information
    QFile dependenciesFileHandle(dependenciesFile);
    if (dependenciesFileHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&dependenciesFileHandle);
        out << "# Package Dependencies Information\n";
        out << "# Format: package_name -> dependency1 dependency2 ...\n\n";
        
        QList<PackageInfo> explicitPackages = getExplicitPackages();
        for (const auto &pkg : explicitPackages) {
            QStringList deps = getPackageDependencies(pkg.name);
            if (!deps.isEmpty()) {
                out << pkg.name << " -> " << deps.join(" ") << "\n";
            }
        }
        dependenciesFileHandle.close();
    }
    
    emit operationProgress("Creating restore script...", 90);
    
    // Create restoration script
    QFile restoreScriptHandle(restoreScript);
    if (restoreScriptHandle.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&restoreScriptHandle);
        out << "#!/bin/bash\n";
        out << "# ArchForge Pro Package Restoration Script\n";
        out << "# Generated on: " << QDateTime::currentDateTime().toString() << "\n\n";
        
        out << "echo \"ArchForge Pro - Package Restoration\"\n";
        out << "echo \"======================================\"\n\n";
        
        out << "# Update package database\n";
        out << "echo \"Updating package database...\"\n";
        out << "sudo pacman -Sy\n\n";
        
        out << "# Install explicitly installed packages (official repos)\n";
        out << "echo \"Installing official repository packages...\"\n";
        QList<PackageInfo> explicitPackages = getExplicitPackages();
        QStringList officialPackages;
        for (const auto &pkg : explicitPackages) {
            if (!pkg.isAUR) {
                officialPackages << pkg.name;
            }
        }
        if (!officialPackages.isEmpty()) {
            out << "sudo pacman -S --needed --noconfirm " << officialPackages.join(" ") << "\n\n";
        }
        
        out << "# Install AUR packages (requires AUR helper like yay or paru)\n";
        out << "echo \"Installing AUR packages...\"\n";
        QList<PackageInfo> aurPackages = getAURPackages();
        for (const auto &pkg : aurPackages) {
            out << "# " << pkg.name << " (AUR)\n";
            out << "if command -v yay &> /dev/null; then\n";
            out << "    yay -S --needed --noconfirm " << pkg.name << "\n";
            out << "elif command -v paru &> /dev/null; then\n";
            out << "    paru -S --needed --noconfirm " << pkg.name << "\n";
            out << "else\n";
            out << "    echo \"Warning: No AUR helper found. Please install " << pkg.name << " manually.\"\n";
            out << "fi\n\n";
        }
        
        out << "echo \"Package restoration completed!\"\n";
        out << "echo \"Please verify that all packages are installed correctly.\"\n";
        
        restoreScriptHandle.close();
        
        // Make script executable
        QFile::setPermissions(restoreScript, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                           QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                           QFileDevice::ReadOther | QFileDevice::ExeOther);
    }
    
    int explicitCount = getExplicitPackages().size();
    int aurCount = getAURPackages().size();
    int totalCount = m_installedPackages.size();
    
    emit operationProgress(QString("Package backup completed - %1 explicit (%2 AUR), %3 total packages with dependencies")
                          .arg(explicitCount).arg(aurCount).arg(totalCount), 100);
}

void PackageManager::exportPackageList(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred("Cannot open file for writing: " + fileName);
        return;
    }
    
    QTextStream out(&file);
    for (const auto &pkg : getExplicitPackages()) {
        out << pkg.name << "\n";
    }
    
    emit operationProgress("Package list exported", 100);
}

void PackageManager::importPackageList(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Cannot open file for reading: " + fileName);
        return;
    }
    
    QTextStream in(&file);
    QStringList packages;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            packages << line;
        }
    }
    
    emit operationProgress("Package list imported", 100);
    
    // Note: Actual installation would require user confirmation
    // This is just for demonstration
}

bool PackageManager::installPackage(const QString &packageName)
{
    QString output = runPacmanCommand({"-S", "--noconfirm", packageName});
    bool success = !output.contains("error") && !output.contains("failed");
    
    if (success) {
        emit packageInstalled(packageName);
        refreshPackageList(); // Refresh to include new package
    }
    
    return success;
}

bool PackageManager::removePackage(const QString &packageName)
{
    QString output = runPacmanCommand({"-R", "--noconfirm", packageName});
    bool success = !output.contains("error") && !output.contains("failed");
    
    if (success) {
        emit packageRemoved(packageName);
        refreshPackageList(); // Refresh to remove package
    }
    
    return success;
}

QList<PackageInfo> PackageManager::searchPackages(const QString &query) const
{
    QList<PackageInfo> results;
    for (const auto &pkg : m_installedPackages) {
        if (pkg.name.contains(query, Qt::CaseInsensitive) ||
            pkg.description.contains(query, Qt::CaseInsensitive)) {
            results.append(pkg);
        }
    }
    return results;
}

QList<PackageInfo> PackageManager::filterPackagesByRepository(const QString &repository) const
{
    QList<PackageInfo> results;
    for (const auto &pkg : m_installedPackages) {
        if (pkg.repository == repository) {
            results.append(pkg);
        }
    }
    return results;
}

QList<PackageInfo> PackageManager::getOrphanedPackages() const
{
    // Simulate orphaned packages detection
    QString output = runPacmanCommand({"-Qdt"});
    QList<PackageInfo> orphans;
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            PackageInfo pkg;
            pkg.name = parts[0];
            pkg.version = parts[1];
            orphans.append(pkg);
        }
    }
    
    return orphans;
}

QList<PackageInfo> PackageManager::getOutdatedPackages() const
{
    // This would typically check against remote repositories
    return QList<PackageInfo>();
}

void PackageManager::checkForUpdates()
{
    QString output = runPacmanCommand({"-Qu"});
    parseUpdateList(output);
    emit updateCheckCompleted(m_availableUpdates.size());
}

QStringList PackageManager::getAvailableUpdates() const
{
    return m_availableUpdates;
}

void PackageManager::updateSystem()
{
    // Note: This would require user privileges
    QString output = runPacmanCommand({"-Syu", "--noconfirm"});
    emit operationProgress("System update completed", 100);
}

QStringList PackageManager::getPackageGroups() const
{
    if (m_packageGroups.isEmpty()) {
        QString output = runPacmanCommand({"-Qg"});
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        QSet<QString> groups;
        
        for (const QString &line : lines) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                groups.insert(parts[0]);
            }
        }
        
        const_cast<PackageManager*>(this)->m_packageGroups = groups.values();
    }
    
    return m_packageGroups;
}

QList<PackageInfo> PackageManager::getPackagesInGroup(const QString &group) const
{
    QString output = runPacmanCommand({"-Qg", group});
    QList<PackageInfo> packages;
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            QString packageName = parts[1];
            if (m_packageCache.contains(packageName)) {
                packages.append(m_packageCache[packageName]);
            }
        }
    }
    
    return packages;
}

QStringList PackageManager::getPackageDependencies(const QString &packageName) const
{
    QString output = runPacmanCommand({"-Qi", packageName});
    QStringList deps;
    
    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("Depends On")) {
            QRegularExpression re("Depends On\\s*:\\s*(.+)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString depString = match.captured(1);
                deps = depString.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            }
            break;
        }
    }
    
    return deps;
}

QStringList PackageManager::getPackageOptionalDependencies(const QString &packageName) const
{
    QString output = runPacmanCommand({"-Qi", packageName});
    QStringList optDeps;
    
    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.startsWith("Optional Deps")) {
            QRegularExpression re("Optional Deps\\s*:\\s*(.+)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString depString = match.captured(1);
                optDeps = depString.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            }
            break;
        }
    }
    
    return optDeps;
}

qint64 PackageManager::getTotalInstalledSize() const
{
    qint64 totalSize = 0;
    for (const auto &pkg : m_installedPackages) {
        totalSize += pkg.size;
    }
    return totalSize;
}

QHash<QString, int> PackageManager::getRepositoryStats() const
{
    QHash<QString, int> stats;
    for (const auto &pkg : m_installedPackages) {
        stats[pkg.repository]++;
    }
    return stats;
}

void PackageManager::onPacmanProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    // Handle process completion
}

void PackageManager::onPacmanProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    emit errorOccurred("Pacman process error occurred");
}

void PackageManager::parsePackageList(const QString &output)
{
    m_installedPackages.clear();
    m_packageCache.clear();
    
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        PackageInfo pkg = parsePackageEntry(line);
        if (!pkg.name.isEmpty()) {
            m_installedPackages.append(pkg);
            m_packageCache[pkg.name] = pkg;
        }
    }
}

void PackageManager::parsePackageInfo(const QString &output)
{
    Q_UNUSED(output)
    // Parse detailed package information
}

void PackageManager::parseUpdateList(const QString &output)
{
    m_availableUpdates.clear();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            m_availableUpdates.append(parts[0]);
        }
    }
}

PackageInfo PackageManager::parsePackageEntry(const QString &entry) const
{
    PackageInfo pkg;
    QStringList parts = entry.split(' ', Qt::SkipEmptyParts);
    
    if (parts.size() >= 2) {
        pkg.name = parts[0];
        pkg.version = parts[1];
        pkg.isExplicit = true;
        pkg.isAUR = isAURPackage(pkg.name);
        pkg.repository = pkg.isAUR ? "AUR" : "official";
        pkg.size = 0; // Would need additional query for size
        pkg.installDate = QDateTime::currentDateTime(); // Placeholder
    }
    
    return pkg;
}

QString PackageManager::runPacmanCommand(const QStringList &arguments) const
{
    QProcess process;
    process.start("pacman", arguments);
    process.waitForFinished(30000); // 30 second timeout
    
    if (process.exitCode() == 0) {
        return process.readAllStandardOutput();
    } else {
        return process.readAllStandardError();
    }
}

bool PackageManager::isAURPackage(const QString &packageName) const
{
    // Simple heuristic - check if package is in foreign packages list
    QString output = runPacmanCommand({"-Qm"});
    return output.contains(packageName);
}

qint64 PackageManager::parseSize(const QString &sizeString) const
{
    // Parse size strings like "1.2 MiB", "500 KiB", etc.
    QRegularExpression re("([0-9.]+)\\s*(KiB|MiB|GiB|B)");
    QRegularExpressionMatch match = re.match(sizeString);
    
    if (match.hasMatch()) {
        double value = match.captured(1).toDouble();
        QString unit = match.captured(2);
        
        if (unit == "KiB") return value * 1024;
        else if (unit == "MiB") return value * 1024 * 1024;
        else if (unit == "GiB") return value * 1024 * 1024 * 1024;
        else return value;
    }
    
    return 0;
}
