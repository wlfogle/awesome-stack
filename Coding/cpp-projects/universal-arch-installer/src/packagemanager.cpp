#include "packagemanager.h"
#include <QProcess>
#include <QDebug>

PackageManager::PackageManager(QObject *parent) : QObject(parent) {
    m_currentProcess = nullptr;
    m_initialized = false;
}

PackageManager::~PackageManager() {
    if (m_currentProcess) {
        m_currentProcess->kill();
        m_currentProcess->deleteLater();
    }
}

QList<PackageInfo> PackageManager::searchPackages(const QString &query, bool useAI) {
    Q_UNUSED(useAI)
    
    QList<PackageInfo> results;
    
    // Search using pacman first
    results.append(searchPacman(query));
    
    // Search AUR if available
    if (isMethodAvailable(InstallMethod::YAY)) {
        results.append(searchAURHelper(query, InstallMethod::YAY));
    } else if (isMethodAvailable(InstallMethod::PARU)) {
        results.append(searchAURHelper(query, InstallMethod::PARU));
    }
    
    // Search Flatpak if available
    if (isMethodAvailable(InstallMethod::FLATPAK)) {
        results.append(searchFlatpak(query));
    }
    
    emit searchCompleted(results);
    return results;
}

bool PackageManager::installPackage(const PackageInfo &package) {
    qDebug() << "Installing package:" << package.name << "using method:" << package.methodString();
    
    // If there's already a process running, don't start another one
    if (m_currentProcess && m_currentProcess->state() != QProcess::NotRunning) {
        qDebug() << "Another installation is already in progress";
        return false;
    }
    
    // Clean up any previous process
    if (m_currentProcess) {
        m_currentProcess->deleteLater();
    }
    
    // Create new process
    m_currentProcess = new QProcess(this);
    
    // Store current package info for later use in slots
    m_currentPackage = package;
    
    // Connect process signals
    connect(m_currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PackageManager::onInstallProcessFinished);
    connect(m_currentProcess, &QProcess::errorOccurred,
            this, &PackageManager::onInstallProcessError);
    connect(m_currentProcess, &QProcess::readyReadStandardOutput,
            this, &PackageManager::onInstallProcessOutput);
    connect(m_currentProcess, &QProcess::readyReadStandardError,
            this, &PackageManager::onInstallProcessOutput);
    
    // Determine installation command based on method
    QString command;
    QStringList arguments;
    
    switch (package.method) {
        case InstallMethod::PACMAN:
            command = "sudo";
            arguments << "pacman" << "-S" << "--noconfirm" << package.name;
            break;
        case InstallMethod::YAY:
            command = "yay";
            arguments << "-S" << "--noconfirm" << package.name;
            break;
        case InstallMethod::PARU:
            command = "paru";
            arguments << "-S" << "--noconfirm" << package.name;
            break;
        case InstallMethod::FLATPAK:
            command = "flatpak";
            arguments << "install" << "-y" << package.name;
            break;
        default:
            command = "sudo";
            arguments << "pacman" << "-S" << "--noconfirm" << package.name;
            break;
    }
    
    // Emit operation started signal
    emit operationStarted(QString("Installing %1...").arg(package.name));
    
    // Start the installation process asynchronously
    m_currentProcess->start(command, arguments);
    
    if (!m_currentProcess->waitForStarted(5000)) {
        qDebug() << "Failed to start installation process for:" << package.name;
        emit packageInstalled(package.name, false);
        emit operationFinished(QString("Failed to start installation of %1").arg(package.name), false);
        return false;
    }
    
    qDebug() << "Installation process started for:" << package.name;
    return true; // Process started successfully, result will come via signals
}

bool PackageManager::removePackage(const QString &packageName) {
    qDebug() << "Removing package:" << packageName;
    emit packageRemoved(packageName, true);
    return true;
}

QList<PackageInfo> PackageManager::getInstalledPackages() {
    QList<PackageInfo> installed;
    
    // Mock installed packages
    for (int i = 0; i < 3; ++i) {
        PackageInfo pkg;
        pkg.name = QString("installed-package-%1").arg(i + 1);
        pkg.version = QString("2.%1.0").arg(i);
        pkg.description = QString("Installed test package %1").arg(i + 1);
        pkg.method = InstallMethod::PACMAN;
        pkg.category = PackageCategory::SYSTEM;
        pkg.installed = true;
        installed.append(pkg);
    }
    
    return installed;
}

QString PackageManager::getSystemInfo() {
    return "Mock system information";
}

bool PackageManager::updateSystem() {
    qDebug() << "Updating system...";
    emit systemUpdated(true);
    return true;
}

// Installation process signal handlers
void PackageManager::onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "Installation process finished with exit code:" << exitCode << "status:" << exitStatus;
    
    bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
    QString message;
    
    if (success) {
        message = QString("Successfully installed %1").arg(m_currentPackage.name);
        qDebug() << message;
    } else {
        QString errorOutput = m_currentProcess->readAllStandardError();
        message = QString("Failed to install %1: %2").arg(m_currentPackage.name, errorOutput);
        qDebug() << message;
    }
    
    // Emit signals
    emit packageInstalled(m_currentPackage.name, success);
    emit operationFinished(message, success);
    
    // Clean up
    m_currentProcess->deleteLater();
    m_currentProcess = nullptr;
}

void PackageManager::onInstallProcessError(QProcess::ProcessError error) {
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start installation process";
            break;
        case QProcess::Crashed:
            errorMessage = "Installation process crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "Installation process timed out";
            break;
        case QProcess::WriteError:
            errorMessage = "Write error during installation";
            break;
        case QProcess::ReadError:
            errorMessage = "Read error during installation";
            break;
        default:
            errorMessage = "Unknown installation error";
            break;
    }
    
    qDebug() << "Installation process error:" << errorMessage;
    emit packageInstalled(m_currentPackage.name, false);
    emit operationFinished(QString("Installation of %1 failed: %2").arg(m_currentPackage.name, errorMessage), false);
}

void PackageManager::onInstallProcessOutput() {
    if (m_currentProcess) {
        QByteArray data = m_currentProcess->readAllStandardOutput();
        QString output = QString::fromUtf8(data);
        
        qDebug() << "Installation output:" << output;
        emit operationOutput(output);
        
        // Simple progress estimation based on output keywords
        if (output.contains("downloading", Qt::CaseInsensitive) || 
            output.contains("retrieving", Qt::CaseInsensitive)) {
            emit operationProgress("Installing", 25);
        } else if (output.contains("installing", Qt::CaseInsensitive) || 
                   output.contains("unpacking", Qt::CaseInsensitive)) {
            emit operationProgress("Installing", 50);
        } else if (output.contains("configuring", Qt::CaseInsensitive) || 
                   output.contains("setting up", Qt::CaseInsensitive)) {
            emit operationProgress("Installing", 75);
        }
    }
}

// Legacy slot implementations (kept for compatibility)
void PackageManager::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    qDebug() << "Process finished";
}

void PackageManager::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error)
    qDebug() << "Process error";
}

void PackageManager::onProcessOutput() {
    qDebug() << "Process output";
}

void PackageManager::onNetworkReply() {
    qDebug() << "Network reply";
}

// Real package search implementations
QList<PackageInfo> PackageManager::searchPacman(const QString &query) {
    QList<PackageInfo> results;
    
    QProcess process;
    process.start("pacman", QStringList() << "-Ss" << query);
    process.waitForFinished(10000); // 10 second timeout
    
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        results = parsePacmanOutput(output, InstallMethod::PACMAN);
    }
    
    return results;
}

QList<PackageInfo> PackageManager::searchAURHelper(const QString &query, InstallMethod method) {
    QList<PackageInfo> results;
    
    QString command;
    switch (method) {
        case InstallMethod::YAY:
            command = "yay";
            break;
        case InstallMethod::PARU:
            command = "paru";
            break;
        case InstallMethod::PIKAUR:
            command = "pikaur";
            break;
        default:
            return results;
    }
    
    QProcess process;
    process.start(command, QStringList() << "-Ss" << query);
    process.waitForFinished(15000); // 15 second timeout for AUR
    
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        results = parsePacmanOutput(output, method); // AUR helpers use pacman-like output
    }
    
    return results;
}

QList<PackageInfo> PackageManager::searchFlatpak(const QString &query) {
    QList<PackageInfo> results;
    
    QProcess process;
    process.start("flatpak", QStringList() << "search" << query);
    process.waitForFinished(10000);
    
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        results = parseFlatpakOutput(output);
    }
    
    return results;
}

QList<PackageInfo> PackageManager::parsePacmanOutput(const QString &output, InstallMethod method) {
    QList<PackageInfo> packages;
    QStringList lines = output.split('\n');
    
    for (int i = 0; i < lines.size(); i++) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;
        
        // Pacman output format: repo/package version
        //                      description
        if (line.contains('/')) {
            PackageInfo pkg;
            pkg.method = method;
            
            // Parse package line
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                QString nameRepo = parts[0];
                QStringList nameRepoParts = nameRepo.split('/');
                if (nameRepoParts.size() == 2) {
                    pkg.source = nameRepoParts[0];
                    pkg.name = nameRepoParts[1];
                }
                pkg.version = parts[1];
                
                // Get description from next line
                if (i + 1 < lines.size()) {
                    QString descLine = lines[i + 1].trimmed();
                    if (!descLine.contains('/') && !descLine.isEmpty()) {
                        pkg.description = descLine;
                    }
                }
                
                // Set category based on repository
                if (pkg.source == "core" || pkg.source == "extra") {
                    pkg.category = PackageCategory::SYSTEM;
                } else if (pkg.source == "community") {
                    pkg.category = PackageCategory::UTILITIES;
                } else {
                    pkg.category = PackageCategory::OTHER;
                }
                
                packages.append(pkg);
            }
        }
    }
    
    return packages;
}

QList<PackageInfo> PackageManager::parseFlatpakOutput(const QString &output) {
    QList<PackageInfo> packages;
    QStringList lines = output.split('\n');
    
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QStringList parts = line.split('\t', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            PackageInfo pkg;
            pkg.method = InstallMethod::FLATPAK;
            pkg.name = parts[0];
            pkg.description = parts[1];
            pkg.source = parts[2];
            pkg.category = PackageCategory::OTHER;
            
            packages.append(pkg);
        }
    }
    
    return packages;
}

bool PackageManager::isMethodAvailable(InstallMethod method) {
    QString command;
    switch (method) {
        case InstallMethod::PACMAN:
            command = "pacman";
            break;
        case InstallMethod::YAY:
            command = "yay";
            break;
        case InstallMethod::PARU:
            command = "paru";
            break;
        case InstallMethod::PIKAUR:
            command = "pikaur";
            break;
        case InstallMethod::FLATPAK:
            command = "flatpak";
            break;
        case InstallMethod::SNAP:
            command = "snap";
            break;
        default:
            return false;
    }
    
    QProcess process;
    process.start("which", QStringList() << command);
    process.waitForFinished(1000);
    
    return process.exitCode() == 0;
}
