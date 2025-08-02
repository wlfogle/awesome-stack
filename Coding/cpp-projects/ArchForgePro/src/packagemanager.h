#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTimer>
#include <QHash>
#include <QDateTime>

struct PackageInfo {
    QString name;
    QString version;
    QString description;
    QString repository;
    qint64 size;
    bool isExplicit;
    bool isAUR;
    QDateTime installDate;
};

class PackageManager : public QObject
{
    Q_OBJECT

public:
    explicit PackageManager(QObject *parent = nullptr);
    ~PackageManager();

    // Package information
    void refreshPackageList();
    QList<PackageInfo> getInstalledPackages() const;
    QList<PackageInfo> getExplicitPackages() const;
    QList<PackageInfo> getAURPackages() const;
    int getInstalledPackageCount() const;
    PackageInfo getPackageInfo(const QString &packageName) const;
    
    // Package operations
    void backupPackageList(const QString &location);
    void exportPackageList(const QString &fileName);
    void importPackageList(const QString &fileName);
    bool installPackage(const QString &packageName);
    bool removePackage(const QString &packageName);
    
    // Package searching and filtering
    QList<PackageInfo> searchPackages(const QString &query) const;
    QList<PackageInfo> filterPackagesByRepository(const QString &repository) const;
    QList<PackageInfo> getOrphanedPackages() const;
    QList<PackageInfo> getOutdatedPackages() const;
    
    // System update operations
    void checkForUpdates();
    QStringList getAvailableUpdates() const;
    void updateSystem();
    
    // Package groups and dependencies
    QStringList getPackageGroups() const;
    QList<PackageInfo> getPackagesInGroup(const QString &group) const;
    QStringList getPackageDependencies(const QString &packageName) const;
    QStringList getPackageOptionalDependencies(const QString &packageName) const;
    
    // Statistics
    qint64 getTotalInstalledSize() const;
    QHash<QString, int> getRepositoryStats() const;
    QDateTime getLastRefreshTime() const { return m_lastRefreshTime; }

signals:
    void packageListRefreshed();
    void packageInstalled(const QString &packageName);
    void packageRemoved(const QString &packageName);
    void updateCheckCompleted(int availableUpdates);
    void operationProgress(const QString &operation, int percentage);
    void errorOccurred(const QString &error);

private slots:
    void onPacmanProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onPacmanProcessError(QProcess::ProcessError error);

private:
    void parsePackageList(const QString &output);
    void parsePackageInfo(const QString &output);
    void parseUpdateList(const QString &output);
    PackageInfo parsePackageEntry(const QString &entry) const;
    QString runPacmanCommand(const QStringList &arguments) const;
    bool isAURPackage(const QString &packageName) const;
    qint64 parseSize(const QString &sizeString) const;
    
    QList<PackageInfo> m_installedPackages;
    QHash<QString, PackageInfo> m_packageCache;
    QStringList m_availableUpdates;
    QStringList m_packageGroups;
    QProcess *m_pacmanProcess;
    QDateTime m_lastRefreshTime;
    QString m_currentOperation;
    bool m_refreshInProgress;
};

#endif // PACKAGEMANAGER_H
