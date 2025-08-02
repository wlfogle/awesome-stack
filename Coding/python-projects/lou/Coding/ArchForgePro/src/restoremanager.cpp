#include "restoremanager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

RestoreManager::RestoreManager(QObject *parent)
    : QObject(parent)
    , m_restoreProcess(nullptr)
{
}

RestoreManager::~RestoreManager()
{
    if (m_restoreProcess && m_restoreProcess->state() != QProcess::NotRunning) {
        m_restoreProcess->terminate();
        m_restoreProcess->waitForFinished(3000);
    }
}

void RestoreManager::startRestore(const QString &backupPath, const QString &destination)
{
    if (!QFileInfo::exists(backupPath)) {
        emit restoreCompleted(false);
        return;
    }
    
    if (m_restoreProcess) {
        m_restoreProcess->deleteLater();
    }
    
    m_restoreProcess = new QProcess(this);
    connect(m_restoreProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                Q_UNUSED(exitStatus)
                emit restoreCompleted(exitCode == 0);
            });
    
    QStringList arguments = {"-xzf", backupPath, "-C", destination};
    m_restoreProcess->start("tar", arguments);
    
    if (!m_restoreProcess->waitForStarted()) {
        emit restoreCompleted(false);
    }
}

void RestoreManager::previewRestore(const QString &backupPath)
{
    QProcess process;
    process.start("tar", {"-tzf", backupPath});
    process.waitForFinished();
    
    if (process.exitCode() == 0) {
        QString preview = process.readAllStandardOutput();
        emit restorePreviewReady(preview);
    } else {
        emit restorePreviewReady("Error reading backup file");
    }
}

QStringList RestoreManager::getRestorePoints(const QString &location) const
{
    QDir dir(location);
    QStringList filters;
    filters << "*.tar.gz" << "*.tar.bz2" << "*.tar.xz" << "*.tar.zst";
    
    QStringList files = dir.entryList(filters, QDir::Files, QDir::Time);
    QStringList fullPaths;
    
    for (const QString &file : files) {
        fullPaths << dir.absoluteFilePath(file);
    }
    
    return fullPaths;
}

bool RestoreManager::verifyRestorePoint(const QString &backupPath) const
{
    QProcess process;
    process.start("tar", {"-tzf", backupPath});
    process.waitForFinished();
    return process.exitCode() == 0;
}
