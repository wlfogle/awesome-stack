#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QThread>
#include <QMutex>
#include <QStringList>
#include <QFileSystemWatcher>
#include <QCryptographicHash>

class BackupJob;

class BackupManager : public QObject
{
    Q_OBJECT

public:
    enum BackupType {
        FullBackup,
        IncrementalBackup,
        PackageBackup,
        SettingsBackup
    };

    enum BackupStatus {
        Idle,
        Running,
        Paused,
        Completed,
        Failed,
        Cancelled
    };

    enum CompressionType {
        None,
        Gzip,
        Bzip2,
        Xz,
        Zstd
    };

    explicit BackupManager(QObject *parent = nullptr);
    ~BackupManager();

    // Main backup operations
    void startFullBackup(const QString &location, const QString &compression = "zstd", bool verify = true);
    void startIncrementalBackup(const QString &location);
    void startPackageBackup(const QString &location);
    void startSettingsBackup(const QString &location);
    
    // Backup control
    void pauseBackup();
    void resumeBackup();
    void cancelBackup();
    
    // Status and information
    BackupStatus getStatus() const { return m_status; }
    int getProgress() const { return m_progress; }
    QString getCurrentOperation() const { return m_currentOperation; }
    QDateTime getLastBackupTime() const { return m_lastBackupTime; }
    QString getLastBackupLocation() const { return m_lastBackupLocation; }
    
    // Configuration
    void setCompressionLevel(int level) { m_compressionLevel = level; }
    void setExcludePaths(const QStringList &paths) { m_excludePaths = paths; }
    void setVerifyBackups(bool verify) { m_verifyBackups = verify; }
    void setMaxBackupSize(qint64 size) { m_maxBackupSize = size; }
    
    // Backup management
    QStringList getAvailableBackups(const QString &location) const;
    bool deleteBackup(const QString &backupPath);
    qint64 getBackupSize(const QString &backupPath) const;
    bool verifyBackup(const QString &backupPath);
    
    // Incremental backup support
    void createSnapshotDatabase(const QString &location);
    QStringList getChangedFiles(const QString &location) const;

signals:
    void progressChanged(int percentage);
    void statusChanged(const QString &status);
    void backupCompleted(bool success);
    void backupStarted(BackupType type);
    void operationChanged(const QString &operation);
    void errorOccurred(const QString &error);

private slots:
    void onBackupProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onBackupProcessError(QProcess::ProcessError error);
    void updateProgress();
    void onFileChanged(const QString &path);

private:
    void setupBackupJob(BackupType type, const QString &location, const QString &compression = "zstd");
    void createBackupScript(const QString &scriptPath, BackupType type, const QString &location, const QString &compression);
    QString generateBackupName(BackupType type) const;
    QString getCompressionExtension(const QString &compression) const;
    QString getCompressionCommand(const QString &compression) const;
    void updateBackupDatabase(const QString &location, const QString &backupPath);
    void cleanupOldBackups(const QString &location);
    bool checkDiskSpace(const QString &location, qint64 estimatedSize);
    void calculateBackupProgress();
    QStringList getSystemPaths() const;
    QStringList getPackagePaths() const;
    QStringList getSettingsPaths() const;
    QString createFileHash(const QString &filePath) const;
    void saveFileDatabase(const QString &location, const QStringList &files);
    QStringList loadFileDatabase(const QString &location) const;
    
    BackupStatus m_status;
    BackupType m_currentBackupType;
    QProcess *m_backupProcess;
    QTimer *m_progressTimer;
    QString m_currentOperation;
    QString m_backupLocation;
    QString m_currentBackupPath;
    int m_progress;
    int m_compressionLevel;
    bool m_verifyBackups;
    qint64 m_maxBackupSize;
    QStringList m_excludePaths;
    QDateTime m_lastBackupTime;
    QString m_lastBackupLocation;
    QMutex m_mutex;
    
    // File system monitoring
    QFileSystemWatcher *m_fileWatcher;
    QStringList m_monitoredPaths;
    
    // Backup statistics
    qint64 m_totalBytes;
    qint64 m_processedBytes;
    int m_totalFiles;
    int m_processedFiles;
    
    // Database for incremental backups
    QString m_databasePath;
    QHash<QString, QString> m_fileHashes; // file path -> hash
    QHash<QString, QDateTime> m_fileModTimes; // file path -> modification time
};

#endif // BACKUPMANAGER_H
