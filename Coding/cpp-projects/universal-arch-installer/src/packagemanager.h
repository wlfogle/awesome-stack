#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "packageinfo.h"

class DatabaseManager;
class PerformanceMonitor;

class PackageManager : public QObject
{
    Q_OBJECT

public:
    explicit PackageManager(QObject *parent = nullptr);
    ~PackageManager();

    // Package search
    QList<PackageInfo> searchPackages(const QString &query, bool useAI = false);
    QList<PackageInfo> searchPackagesByMethod(const QString &query, InstallMethod method);
    QList<PackageInfo> getInstalledPackages();
    QList<PackageInfo> getAvailableUpdates();

    // Package installation/removal
    bool installPackage(const PackageInfo &package);
    bool removePackage(const QString &packageName);
    bool updatePackage(const QString &packageName);
    bool updateSystem();

    // System maintenance
    bool updateMirrors();
    bool cleanCache();
    bool cleanAURCache();
    bool optimizeMirrors();
    bool removeOrphanedPackages();

    // Method availability
    QStringList getAvailableMethods();
    bool isMethodAvailable(InstallMethod method);
    QString getMethodCommand(InstallMethod method);

    // System information
    QString getSystemInfo();
    QString getCacheSize();
    QString getDiskUsage();
    QStringList getSystemStatus();

    // Package information
    PackageInfo getPackageInfo(const QString &packageName, InstallMethod method = InstallMethod::PACMAN);
    QStringList getPackageDependencies(const QString &packageName);
    QStringList getPackageFiles(const QString &packageName);
    QString getPackageDescription(const QString &packageName);

    // Repository management
    bool addRepository(const QString &name, const QString &url);
    bool removeRepository(const QString &name);
    QStringList getRepositories();
    bool refreshRepositories();

    // Package building
    bool createPKGBUILD(const QString &packageName, const QString &version, 
                       const QString &description, const QString &sourcePath,
                       const QStringList &dependencies);
    bool buildPackage(const QString &pkgbuildPath);
    bool testPackage(const QString &packagePath);

    // Network operations
    bool downloadFile(const QString &url, const QString &destination);
    QByteArray fetchUrl(const QString &url);

signals:
    void operationStarted(const QString &operation);
    void operationFinished(const QString &operation, bool success);
    void operationProgress(const QString &operation, int percentage);
    void operationOutput(const QString &output);
    void operationError(const QString &error);
    
    void packageInstalled(const QString &package, bool success);
    void packageRemoved(const QString &package, bool success);
    void packageUpdated(const QString &package, bool success);
    void systemUpdated(bool success);
    
    void searchCompleted(const QList<PackageInfo> &results);
    void searchError(const QString &error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();
    void onNetworkReply();
    
    // Installation-specific slots
    void onInstallProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onInstallProcessError(QProcess::ProcessError error);
    void onInstallProcessOutput();

private:
    // Core search methods
    QList<PackageInfo> searchPacman(const QString &query);
    QList<PackageInfo> searchAURHelper(const QString &query, InstallMethod method);
    QList<PackageInfo> searchFlatpak(const QString &query);
    QList<PackageInfo> searchSnap(const QString &query);
    QList<PackageInfo> searchPip(const QString &query);
    QList<PackageInfo> searchConda(const QString &query);
    
    // Output parsing
    QList<PackageInfo> parsePacmanOutput(const QString &output, InstallMethod method);
    QList<PackageInfo> parseFlatpakOutput(const QString &output);
    QList<PackageInfo> parseSnapOutput(const QString &output);
    QList<PackageInfo> parsePipOutput(const QString &output);
    QList<PackageInfo> parseCondaOutput(const QString &output);

    // Package categorization
    PackageCategory categorizePackage(const QString &name, const QString &description);

    // Utilities
    bool runCommand(const QString &command, const QStringList &arguments, 
                   QString *output = nullptr, int timeoutMs = 30000);
    bool runCommandAsync(const QString &command, const QStringList &arguments);
    QString escapeShellArgument(const QString &arg);
    QStringList parsePackageList(const QString &output);

    // AI-powered features
    QList<PackageInfo> applyAIRanking(const QString &query, QList<PackageInfo> packages);
    double calculateRelevanceScore(const QString &query, const PackageInfo &package);

    // Performance monitoring
    void recordOperation(const QString &operation, const QString &package, 
                        double duration, bool success);

    // Members
    DatabaseManager *m_database;
    PerformanceMonitor *m_performanceMonitor;
    QNetworkAccessManager *m_networkManager;
    
    QProcess *m_currentProcess;
    QTimer *m_processTimeoutTimer;
    QString m_currentOperation;
    PackageInfo m_currentPackage;
    QDateTime m_operationStartTime;
    
    QStringList m_availableMethods;
    bool m_initialized;
    
    // Cache
    QList<PackageInfo> m_installedPackagesCache;
    QDateTime m_cacheLastUpdated;
    static const int CACHE_TIMEOUT_MS = 300000; // 5 minutes
};

#endif // PACKAGEMANAGER_H
