#include "rgbcommandbatcher.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <algorithm>

RGBCommandBatcher::RGBCommandBatcher(QObject *parent)
    : QObject(parent)
    , m_devicePath("/dev/hidraw1")
    , m_batchSize(16)
    , m_maxDelay(50)
    , m_running(false)
    , m_currentDeviceIndex(0)
    , m_deviceWriteFailures(0)
    , m_maxFailures(5)
    , m_batchThread(nullptr)
    , m_batchTimer(nullptr)
{
    // Initialize fallback device paths
    m_fallbackDevicePaths = {"/dev/hidraw1", "/dev/hidraw0", "/dev/hidraw2", "/dev/hidraw3"};
    
    // Initialize default keyboard mapping
    initializeDefaultKeyboardMap();
    
    // Create timer for batch processing
    m_batchTimer = new QTimer(this);
    m_batchTimer->setSingleShot(false);
    connect(m_batchTimer, &QTimer::timeout, this, &RGBCommandBatcher::processBatchQueue);
    
    qDebug() << "RGBCommandBatcher initialized with device:" << m_devicePath;
}

RGBCommandBatcher::~RGBCommandBatcher()
{
    stop();
    qDebug() << "RGBCommandBatcher destroyed";
}

bool RGBCommandBatcher::initialize(const QString &devicePath, int batchSize, int maxDelay)
{
    QMutexLocker locker(&m_queueMutex);
    
    if (m_running) {
        qWarning() << "Cannot initialize while batcher is running";
        return false;
    }
    
    m_devicePath = devicePath;
    m_batchSize = qMax(1, batchSize);
    m_maxDelay = qMax(1, maxDelay);
    
    // Update fallback list to include the specified device first
    m_fallbackDevicePaths.removeAll(devicePath);
    m_fallbackDevicePaths.prepend(devicePath);
    
    qDebug() << "RGBCommandBatcher configured: device=" << m_devicePath 
             << "batchSize=" << m_batchSize << "maxDelay=" << m_maxDelay;
    
    return true;
}

bool RGBCommandBatcher::start()
{
    QMutexLocker locker(&m_queueMutex);
    
    if (m_running) {
        qDebug() << "RGBCommandBatcher already running";
        return true;
    }
    
    // Try to open the device file
    m_deviceFile = std::make_unique<QFile>(m_devicePath);
    if (!m_deviceFile->open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open device:" << m_devicePath << m_deviceFile->errorString();
        
        // Try fallback devices
        if (!tryDeviceFallback()) {
            emit error("No RGB devices available");
            return false;
        }
    }
    
    m_running = true;
    m_deviceWriteFailures = 0;
    m_lastBatchTime.start();
    
    // Start the batch processing timer
    m_batchTimer->start(m_maxDelay);
    
    qDebug() << "RGBCommandBatcher started successfully with device:" << m_devicePath;
    return true;
}

bool RGBCommandBatcher::stop()
{
    QMutexLocker locker(&m_queueMutex);
    
    if (!m_running) {
        return true;
    }
    
    m_running = false;
    
    // Stop the timer
    if (m_batchTimer) {
        m_batchTimer->stop();
    }
    
    // Close device file
    if (m_deviceFile && m_deviceFile->isOpen()) {
        m_deviceFile->close();
    }
    m_deviceFile.reset();
    
    // Clear any remaining commands
    m_commandQueue.clear();
    
    qDebug() << "RGBCommandBatcher stopped";
    return true;
}

bool RGBCommandBatcher::addCommand(int keyIndex, int red, int green, int blue, int priority)
{
    if (!m_running) {
        qWarning() << "Cannot add command: batcher not running";
        return false;
    }
    
    // Clamp color values to valid range
    red = qBound(0, red, 255);
    green = qBound(0, green, 255);
    blue = qBound(0, blue, 255);
    
    RGBCommand command(keyIndex, red, green, blue, priority);
    
    {
        QMutexLocker locker(&m_queueMutex);
        
        // Check if queue is getting too large
        if (m_commandQueue.size() >= m_batchSize * 20) {
            qWarning() << "Command queue overflow, dropping command";
            return false;
        }
        
        m_commandQueue.enqueue(command);
        
        // If we have enough commands, trigger immediate processing
        if (m_commandQueue.size() >= m_batchSize) {
            QMetaObject::invokeMethod(this, "processBatchNow", Qt::QueuedConnection);
        }
    }
    
    return true;
}

bool RGBCommandBatcher::addKeyColor(const QString &keyName, int red, int green, int blue, int priority)
{
    QString keyLower = keyName.toLower();
    
    if (!m_keyboardMap.contains(keyLower)) {
        qWarning() << "Unknown key name:" << keyName;
        return false;
    }
    
    int keyIndex = m_keyboardMap[keyLower];
    return addCommand(keyIndex, red, green, blue, priority);
}

int RGBCommandBatcher::addGroupColors(const QStringList &keyGroup, int red, int green, int blue, int priority)
{
    int successCount = 0;
    
    for (const QString &keyName : keyGroup) {
        if (addKeyColor(keyName, red, green, blue, priority)) {
            successCount++;
        }
    }
    
    return successCount;
}

bool RGBCommandBatcher::clearQueue()
{
    QMutexLocker locker(&m_queueMutex);
    m_commandQueue.clear();
    qDebug() << "Command queue cleared";
    return true;
}

int RGBCommandBatcher::queueSize() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_commandQueue.size();
}

void RGBCommandBatcher::setKeyboardMap(const QMap<QString, int> &keyboardMap)
{
    QMutexLocker locker(&m_queueMutex);
    m_keyboardMap = keyboardMap;
    qDebug() << "Keyboard mapping updated with" << keyboardMap.size() << "keys";
}

void RGBCommandBatcher::processBatchQueue()
{
    if (!m_running) {
        return;
    }
    
    QList<RGBCommand> batch;
    
    {
        QMutexLocker locker(&m_queueMutex);
        
        // Check if we should process a batch
        bool shouldProcess = false;
        
        if (m_commandQueue.size() >= m_batchSize) {
            shouldProcess = true;
        } else if (!m_commandQueue.isEmpty() && m_lastBatchTime.elapsed() >= m_maxDelay) {
            shouldProcess = true;
        }
        
        if (!shouldProcess) {
            return;
        }
        
        // Extract commands for this batch
        int commandsToTake = qMin(m_batchSize, m_commandQueue.size());
        
        for (int i = 0; i < commandsToTake; ++i) {
            batch.append(m_commandQueue.dequeue());
        }
    }
    
    if (!batch.isEmpty()) {
        // Sort batch by priority (high to low) then by timestamp (old to new)
        std::sort(batch.begin(), batch.end(), [](const RGBCommand &a, const RGBCommand &b) {
            if (a.priority != b.priority) {
                return a.priority > b.priority; // Higher priority first
            }
            return a.timestamp < b.timestamp; // Older commands first
        });
        
        // Send the batch
        if (sendBatch(batch)) {
            m_lastBatchTime.restart();
            emit batchSent(batch.size());
        }
    }
}

void RGBCommandBatcher::processBatchNow()
{
    processBatchQueue();
}

bool RGBCommandBatcher::sendBatch(const QList<RGBCommand> &batch)
{
    if (batch.isEmpty()) {
        return true;
    }
    
    if (!m_deviceFile || !m_deviceFile->isOpen()) {
        qWarning() << "Device file not available";
        m_deviceWriteFailures++;
        
        if (m_deviceWriteFailures >= m_maxFailures) {
            if (!tryDeviceFallback()) {
                emit error("All RGB devices failed");
                return false;
            }
        }
        return false;
    }
    
    bool allSuccessful = true;
    
    for (const RGBCommand &command : batch) {
        QByteArray commandData = createCommandData(command);
        
        if (!writeToDevice(commandData)) {
            allSuccessful = false;
            m_deviceWriteFailures++;
            
            if (m_deviceWriteFailures >= m_maxFailures) {
                qWarning() << "Too many device failures, trying fallback";
                if (!tryDeviceFallback()) {
                    emit error("All RGB devices failed");
                    return false;
                }
                break;
            }
        } else {
            m_deviceWriteFailures = 0; // Reset failure count on success
        }
    }
    
    return allSuccessful;
}

bool RGBCommandBatcher::writeToDevice(const QByteArray &data)
{
    if (!m_deviceFile || !m_deviceFile->isOpen()) {
        return false;
    }
    
    qint64 bytesWritten = m_deviceFile->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "Failed to write complete command to device";
        return false;
    }
    
    // Ensure data is written immediately
    m_deviceFile->flush();
    
    return true;
}

bool RGBCommandBatcher::tryDeviceFallback()
{
    // Close current device if open
    if (m_deviceFile && m_deviceFile->isOpen()) {
        m_deviceFile->close();
    }
    
    // Try next device in fallback list
    for (int i = 0; i < m_fallbackDevicePaths.size(); ++i) {
        QString devicePath = m_fallbackDevicePaths[i];
        
        if (devicePath == m_devicePath) {
            continue; // Skip current device
        }
        
        m_deviceFile = std::make_unique<QFile>(devicePath);
        if (m_deviceFile->open(QIODevice::WriteOnly)) {
            m_devicePath = devicePath;
            m_deviceWriteFailures = 0;
            
            qDebug() << "Successfully switched to fallback device:" << devicePath;
            emit deviceChanged(devicePath);
            return true;
        }
    }
    
    qWarning() << "All fallback devices failed";
    m_deviceFile.reset();
    return false;
}

QByteArray RGBCommandBatcher::createCommandData(const RGBCommand &command) const
{
    // Create OriginPC-style RGB command
    // Format: [CC][01][key_index][red][green][blue][00][00][00][00][00][00][00][00][00][00]
    QByteArray data(16, 0x00);
    
    data[0] = 0xCC;  // Command header
    data[1] = 0x01;  // RGB command type
    data[2] = static_cast<char>(command.keyIndex & 0xFF);
    data[3] = static_cast<char>(command.red & 0xFF);
    data[4] = static_cast<char>(command.green & 0xFF);
    data[5] = static_cast<char>(command.blue & 0xFF);
    
    return data;
}

void RGBCommandBatcher::initializeDefaultKeyboardMap()
{
    // Initialize with basic QWERTY layout mapping
    // This is a simplified mapping - a full implementation would have all keys
    
    // Numbers row
    m_keyboardMap["1"] = 0x02; m_keyboardMap["2"] = 0x03; m_keyboardMap["3"] = 0x04;
    m_keyboardMap["4"] = 0x05; m_keyboardMap["5"] = 0x06; m_keyboardMap["6"] = 0x07;
    m_keyboardMap["7"] = 0x08; m_keyboardMap["8"] = 0x09; m_keyboardMap["9"] = 0x0A;
    m_keyboardMap["0"] = 0x0B;
    
    // Top letter row
    m_keyboardMap["q"] = 0x10; m_keyboardMap["w"] = 0x11; m_keyboardMap["e"] = 0x12;
    m_keyboardMap["r"] = 0x13; m_keyboardMap["t"] = 0x14; m_keyboardMap["y"] = 0x15;
    m_keyboardMap["u"] = 0x16; m_keyboardMap["i"] = 0x17; m_keyboardMap["o"] = 0x18;
    m_keyboardMap["p"] = 0x19;
    
    // Middle letter row
    m_keyboardMap["a"] = 0x1E; m_keyboardMap["s"] = 0x1F; m_keyboardMap["d"] = 0x20;
    m_keyboardMap["f"] = 0x21; m_keyboardMap["g"] = 0x22; m_keyboardMap["h"] = 0x23;
    m_keyboardMap["j"] = 0x24; m_keyboardMap["k"] = 0x25; m_keyboardMap["l"] = 0x26;
    
    // Bottom letter row
    m_keyboardMap["z"] = 0x2C; m_keyboardMap["x"] = 0x2D; m_keyboardMap["c"] = 0x2E;
    m_keyboardMap["v"] = 0x2F; m_keyboardMap["b"] = 0x30; m_keyboardMap["n"] = 0x31;
    m_keyboardMap["m"] = 0x32;
    
    // Special keys
    m_keyboardMap["space"] = 0x39;
    m_keyboardMap["enter"] = 0x1C;
    m_keyboardMap["escape"] = 0x01;
    m_keyboardMap["tab"] = 0x0F;
    m_keyboardMap["capslock"] = 0x3A;
    m_keyboardMap["shift"] = 0x2A;
    m_keyboardMap["ctrl"] = 0x1D;
    m_keyboardMap["alt"] = 0x38;
    
    // Function keys
    m_keyboardMap["f1"] = 0x3B; m_keyboardMap["f2"] = 0x3C; m_keyboardMap["f3"] = 0x3D;
    m_keyboardMap["f4"] = 0x3E; m_keyboardMap["f5"] = 0x3F; m_keyboardMap["f6"] = 0x40;
    m_keyboardMap["f7"] = 0x41; m_keyboardMap["f8"] = 0x42; m_keyboardMap["f9"] = 0x43;
    m_keyboardMap["f10"] = 0x44; m_keyboardMap["f11"] = 0x57; m_keyboardMap["f12"] = 0x58;
    
    // Arrow keys
    m_keyboardMap["up"] = 0xC8; m_keyboardMap["down"] = 0xD0;
    m_keyboardMap["left"] = 0xCB; m_keyboardMap["right"] = 0xCD;
    
    // Numeric keypad
    m_keyboardMap["kp_0"] = 0x52; m_keyboardMap["kp_1"] = 0x4F; m_keyboardMap["kp_2"] = 0x50;
    m_keyboardMap["kp_3"] = 0x51; m_keyboardMap["kp_4"] = 0x4B; m_keyboardMap["kp_5"] = 0x4C;
    m_keyboardMap["kp_6"] = 0x4D; m_keyboardMap["kp_7"] = 0x47; m_keyboardMap["kp_8"] = 0x48;
    m_keyboardMap["kp_9"] = 0x49; m_keyboardMap["kp_plus"] = 0x53; m_keyboardMap["kp_minus"] = 0x4A;
    m_keyboardMap["kp_multiply"] = 0x37; m_keyboardMap["kp_divide"] = 0xB5;
    m_keyboardMap["kp_enter"] = 0x9C; m_keyboardMap["kp_decimal"] = 0x4E;
    
    qDebug() << "Initialized keyboard mapping with" << m_keyboardMap.size() << "keys";
}
