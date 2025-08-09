#ifndef BACKUPJOB_H
#define BACKUPJOB_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QDateTime>

class BackupJob : public QObject
{
    Q_OBJECT

public:
    enum JobType {
        FullBackupJob,
        IncrementalBackupJob,
        PackageBackupJob,
        SettingsBackupJob
    };

    explicit BackupJob(JobType type, const QString &source, const QString &destination, QObject *parent = nullptr);
    ~BackupJob();

    void start();
    void stop();
    void pause();
    void resume();

    JobType getType() const { return m_type; }
    QString getSource() const { return m_source; }
    QString getDestination() const { return m_destination; }
    QDateTime getStartTime() const { return m_startTime; }
    int getProgress() const { return m_progress; }

signals:
    void started();
    void finished(bool success);
    void progressChanged(int percentage);
    void statusChanged(const QString &status);
    void errorOccurred(const QString &error);

private slots:
    void doWork();

private:
    JobType m_type;
    QString m_source;
    QString m_destination;
    QDateTime m_startTime;
    int m_progress;
    bool m_running;
    bool m_paused;
    QThread *m_thread;
};

#endif // BACKUPJOB_H
