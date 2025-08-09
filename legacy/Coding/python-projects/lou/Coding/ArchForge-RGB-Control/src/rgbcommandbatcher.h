#ifndef RGBCOMMANDBATCHER_H
#define RGBCOMMANDBATCHER_H

#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QDebug>
#include <QStringList>
#include <QMap>
#include <memory>

/**
 * @brief RGB command structure for batched processing
 */
struct RGBCommand {
    int keyIndex;           ///< Index of the key to control
    int red;               ///< Red color value (0-255)
    int green;             ///< Green color value (0-255)
    int blue;              ///< Blue color value (0-255)
    int priority;          ///< Command priority (higher = processed first)
    qint64 timestamp;      ///< Timestamp when command was created
    
    RGBCommand() = default;
    RGBCommand(int key, int r, int g, int b, int prio = 0)
        : keyIndex(key), red(r), green(g), blue(b), priority(prio)
        , timestamp(QDateTime::currentMSecsSinceEpoch()) {}
};

/**
 * @brief Batches RGB commands for improved performance
 * 
 * Instead of sending each RGB command individually, which introduces
 * latency and overhead, this class batches commands and sends them
 * in optimized groups for better performance and smoother effects.
 */
class RGBCommandBatcher : public QObject
{
    Q_OBJECT

public:
    explicit RGBCommandBatcher(QObject *parent = nullptr);
    ~RGBCommandBatcher();

    /**
     * @brief Initialize the batcher with device settings
     * @param devicePath Path to the RGB device (e.g., "/dev/hidraw0")
     * @param batchSize Maximum number of commands to batch before sending
     * @param maxDelay Maximum delay in milliseconds before sending a partial batch
     * @return true if initialization was successful
     */
    bool initialize(const QString &devicePath = "/dev/hidraw1", 
                   int batchSize = 16, 
                   int maxDelay = 50);

    /**
     * @brief Start the batch processing thread
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the batch processing thread
     * @return true if stopped successfully
     */
    bool stop();

    /**
     * @brief Add an RGB command to the batch queue
     * @param keyIndex Index of the key to control
     * @param red Red color value (0-255)
     * @param green Green color value (0-255)
     * @param blue Blue color value (0-255)
     * @param priority Command priority (higher = processed first)
     * @return true if command was added to queue
     */
    bool addCommand(int keyIndex, int red, int green, int blue, int priority = 0);

    /**
     * @brief Add a key color command using key name instead of index
     * @param keyName Name of the key (e.g., 'a', 'enter')
     * @param red Red color value (0-255)
     * @param green Green color value (0-255)
     * @param blue Blue color value (0-255)
     * @param priority Command priority
     * @return true if command was added
     */
    bool addKeyColor(const QString &keyName, int red, int green, int blue, int priority = 0);

    /**
     * @brief Add color commands for a group of keys
     * @param keyGroup List of key names
     * @param red Red color value (0-255)
     * @param green Green color value (0-255)
     * @param blue Blue color value (0-255)
     * @param priority Command priority
     * @return Number of commands successfully added
     */
    int addGroupColors(const QStringList &keyGroup, int red, int green, int blue, int priority = 0);

    /**
     * @brief Clear all pending commands in the queue
     * @return true if queue was cleared
     */
    bool clearQueue();

    /**
     * @brief Get current queue size
     * @return Number of pending commands
     */
    int queueSize() const;

    /**
     * @brief Check if the batcher is currently running
     * @return true if running
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Get device failure count
     * @return Number of consecutive device write failures
     */
    int deviceFailureCount() const { return m_deviceWriteFailures; }

    /**
     * @brief Set the keyboard mapping for key name to index conversion
     * @param keyboardMap Map of key names to indices
     */
    void setKeyboardMap(const QMap<QString, int> &keyboardMap);

signals:
    /**
     * @brief Emitted when a batch of commands is sent successfully
     * @param batchSize Number of commands in the batch
     */
    void batchSent(int batchSize);

    /**
     * @brief Emitted when there's an error sending commands
     * @param error Error description
     */
    void error(const QString &error);

    /**
     * @brief Emitted when device fallback occurs
     * @param newDevicePath Path to the new device being used
     */
    void deviceChanged(const QString &newDevicePath);

private slots:
    /**
     * @brief Process the command queue in batches
     */
    void processBatchQueue();

    /**
     * @brief Force immediate processing of the current batch
     */
    void processBatchNow();

private:
    /**
     * @brief Send a batch of RGB commands to the device
     * @param batch List of commands to send
     * @return true if batch was sent successfully
     */
    bool sendBatch(const QList<RGBCommand> &batch);

    /**
     * @brief Write RGB command data to device
     * @param data Command data to write
     * @return true if write was successful
     */
    bool writeToDevice(const QByteArray &data);

    /**
     * @brief Try to switch to next available device
     * @return true if fallback was successful
     */
    bool tryDeviceFallback();

    /**
     * @brief Create RGB command data for device
     * @param command RGB command to convert
     * @return Raw command data for device
     */
    QByteArray createCommandData(const RGBCommand &command) const;

    /**
     * @brief Initialize default keyboard mapping
     */
    void initializeDefaultKeyboardMap();

private:
    // Configuration
    QString m_devicePath;                   ///< Current device path
    int m_batchSize;                       ///< Maximum commands per batch
    int m_maxDelay;                        ///< Maximum delay in milliseconds
    QStringList m_fallbackDevicePaths;    ///< Fallback device paths
    
    // State
    bool m_running;                        ///< Whether the batcher is running
    int m_currentDeviceIndex;              ///< Current device index in fallback list
    int m_deviceWriteFailures;             ///< Consecutive device write failures
    int m_maxFailures;                     ///< Maximum failures before fallback
    
    // Threading and synchronization
    QThread *m_batchThread;                ///< Background processing thread
    mutable QMutex m_queueMutex;          ///< Mutex for queue protection
    QWaitCondition m_queueCondition;      ///< Condition for queue processing
    QTimer *m_batchTimer;                  ///< Timer for batch processing
    QElapsedTimer m_lastBatchTime;         ///< Time tracking for batches
    
    // Data structures
    QQueue<RGBCommand> m_commandQueue;     ///< Queue of pending commands
    QMap<QString, int> m_keyboardMap;      ///< Mapping of key names to indices
    
    // Device file handle
    std::unique_ptr<QFile> m_deviceFile;   ///< Device file for writing commands
};

#endif // RGBCOMMANDBATCHER_H
