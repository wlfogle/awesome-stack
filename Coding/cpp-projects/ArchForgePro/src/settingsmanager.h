#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QHash>

struct SettingFile {
    QString path;
    QString name;
    qint64 size;
    QDateTime modified;
    bool isSystemConfig;
    bool isUserConfig;
};

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager();

    void refreshSettingsList();
    QList<SettingFile> getSettingFiles() const;
    QList<SettingFile> getSystemSettings() const;
    QList<SettingFile> getUserSettings() const;
    
    void backupSettings(const QString &location);
    void exportSettings(const QString &fileName);
    void importSettings(const QString &fileName);
    
    QStringList getConfigDirectories() const;
    QList<SettingFile> searchSettings(const QString &query) const;

signals:
    void settingsListRefreshed();
    void operationProgress(const QString &operation, int percentage);
    void errorOccurred(const QString &error);

private:
    void scanDirectory(const QString &path, bool isSystem);
    SettingFile createSettingFile(const QString &filePath, bool isSystem) const;
    
    // Comprehensive scanning methods
    void scanSystemConfigs();
    void scanUserConfigs();
    void scanPacmanComponents();
    void scanSystemdComponents();
    void scanNetworkConfigs();
    void scanBootConfigs();
    void scanDesktopConfigs();
    void scanVirtualMachines();
    void scanBtrfsSnapshots();
    void scanAdditionalComponents();
    
    // Helper methods
    void addIfExists(const QString &path, bool isSystem);
    void addDirectoryInfo(const QString &path, const QString &description, bool isSystem);
    
    QList<SettingFile> m_settingFiles;
    QDateTime m_lastRefreshTime;
};

#endif // SETTINGSMANAGER_H
