#ifndef RESTOREMANAGER_H
#define RESTOREMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QDateTime>

class RestoreManager : public QObject
{
    Q_OBJECT

public:
    explicit RestoreManager(QObject *parent = nullptr);
    ~RestoreManager();

    void startRestore(const QString &backupPath, const QString &destination = "/");
    void previewRestore(const QString &backupPath);
    QStringList getRestorePoints(const QString &location) const;
    bool verifyRestorePoint(const QString &backupPath) const;

signals:
    void restoreCompleted(bool success);
    void restoreProgress(int percentage);
    void restorePreviewReady(const QString &preview);

private:
    QProcess *m_restoreProcess;
};

#endif // RESTOREMANAGER_H
