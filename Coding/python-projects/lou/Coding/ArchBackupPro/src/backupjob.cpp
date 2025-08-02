#include "backupjob.h"
#include <QTimer>

BackupJob::BackupJob(JobType type, const QString &source, const QString &destination, QObject *parent)
    : QObject(parent)
    , m_type(type)
    , m_source(source)
    , m_destination(destination)
    , m_progress(0)
    , m_running(false)
    , m_paused(false)
    , m_thread(nullptr)
{
}

BackupJob::~BackupJob()
{
    if (m_thread && m_thread->isRunning()) {
        m_thread->quit();
        m_thread->wait();
    }
}

void BackupJob::start()
{
    if (m_running) return;
    
    m_running = true;
    m_startTime = QDateTime::currentDateTime();
    emit started();
    
    // Start work in separate thread
    m_thread = new QThread();
    moveToThread(m_thread);
    connect(m_thread, &QThread::started, this, &BackupJob::doWork);
    connect(this, &BackupJob::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    m_thread->start();
}

void BackupJob::stop()
{
    m_running = false;
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void BackupJob::pause()
{
    m_paused = true;
    emit statusChanged("Paused");
}

void BackupJob::resume()
{
    m_paused = false;
    emit statusChanged("Resumed");
}

void BackupJob::doWork()
{
    // Simulate backup work
    for (int i = 0; i <= 100 && m_running; i += 5) {
        if (m_paused) {
            // Wait while paused
            while (m_paused && m_running) {
                QThread::msleep(100);
            }
        }
        
        if (!m_running) break;
        
        m_progress = i;
        emit progressChanged(i);
        
        // Simulate work
        QThread::msleep(200);
    }
    
    bool success = m_running; // Success if not stopped
    emit finished(success);
}
