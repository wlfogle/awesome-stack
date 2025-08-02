#ifndef RGBFANCONTROL_H
#define RGBFANCONTROL_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QSpinBox>
#include <QColorDialog>
#include <QTextEdit>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>
#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

// Forward declarations
class SystemMonitorThread;
class RGBEffectManager;
class FanControlManager;
class ConfigurationManager;

/**
 * @brief Advanced system monitoring data structure (OriginPC Enhanced)
 */
struct SystemData {
    double cpuPercent;
    double cpuTemp;
    double memoryPercent;
    double memoryUsed;
    double memoryTotal;
    double diskPercent;
    double diskUsed;
    double diskTotal;
    double gpuLoad;
    double gpuTemp;
    double gpuMemory;
    QList<QPair<QString, double>> cpuTemps;
    QList<QPair<QString, double>> fanSpeeds;
    QList<QPair<QString, double>> voltages;
    qint64 timestamp;
    
    // OriginPC Enhanced monitoring data
    QList<QPair<QString, double>> nvmeTemps;
    QList<QPair<QString, double>> memoryTemps;
    QList<QPair<QString, double>> motherboardTemps;
    QList<QPair<QString, double>> networkStats;
    double powerConsumption;
    QString powerProfile;
    bool batteryPresent;
    double batteryPercent;
    bool acConnected;
    QString thermalState;
    QMap<QString, double> sensorData;
    QStringList runningProcesses;
    double systemLoadAvg1;
    double systemLoadAvg5;
    double systemLoadAvg15;
};

/**
 * @brief RGB Device Information
 */
struct RGBDevice {
    QString id;
    QString name;
    QString path;
    QString type; // keyboard, mouse, headset, etc.
    bool connected;
    bool accessible;
    QMap<QString, QVariant> capabilities;
    qint64 lastSeen;
};

/**
 * @brief Fan Device Information
 */
struct FanDevice {
    QString id;
    QString name;
    QString path;
    QString chipName;
    int currentRPM;
    int currentPWM;
    int maxRPM;
    bool controllable;
    QString tempSensor;
    qint64 lastUpdate;
};

/**
 * @brief Power Management Data
 */
struct PowerData {
    QString profile; // performance, balanced, powersave
    bool onBattery;
    double batteryPercent;
    double powerConsumption;
    QString cpuGovernor;
    QString energyPerformance;
    bool boostEnabled;
    int idleTimeout;
    int sleepTimeout;
    qint64 lastActivity;
};

/**
 * @brief RGB Effect configuration
 */
struct RGBEffect {
    QString name;
    QString type; // static, breathing, wave, rainbow, etc.
    QColor primaryColor;
    QColor secondaryColor;
    int speed;
    int brightness;
    bool enabled;
    QJsonObject customData;
};

/**
 * @brief Fan control profile
 */
struct FanProfile {
    QString name;
    QMap<int, int> tempToPwmCurve; // temperature -> PWM percentage
    bool enabled;
    int hysteresis;
    QString targetSensor;
};

/**
 * @brief Main RGB/Fan Control widget
 */
class RGBFanControl : public QWidget
{
    Q_OBJECT

public:
    explicit RGBFanControl(QWidget *parent = nullptr);
    ~RGBFanControl();

    // Public interface
    void startMonitoring();
    void stopMonitoring();
    void applyRGBEffect(const RGBEffect &effect);
    void applyFanProfile(const FanProfile &profile);

public slots:
    void onSystemDataUpdated(const SystemData &data);
    void onRGBEffectChanged();
    void onFanProfileChanged();
    void refreshSystemInfo();

signals:
    void statusMessage(const QString &message);
    void systemDataReady(const SystemData &data);

private slots:
    void setupUI();
    void setupSystemMonitoringTab();
    void setupRGBControlTab();
    void setupFanControlTab();
    void setupProfilesTab();
    
    // System monitoring
    void updateSystemDisplays();
    void updateTemperatureDisplays();
    void updateFanDisplays();
    
    // RGB control
    void selectPrimaryColor();
    void selectSecondaryColor();
    void changeRGBEffect();
    void changeBrightness(int value);
    void changeSpeed(int value);
    void saveRGBProfile();
    void loadRGBProfile();
    
    // Fan control
    void enableFanControl(bool enabled);
    void updateFanCurve();
    void saveFanProfile();
    void loadFanProfile();
    void setManualFanSpeed(int speed);
    
    // Profiles
    void createNewProfile();
    void deleteProfile();
    void exportProfile();
    void importProfile();

private:
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void createDefaultProfiles();
    
    // UI helper methods
    QGroupBox* createTemperatureGroup();
    QGroupBox* createFanSpeedGroup();
    QGroupBox* createSystemInfoGroup();
    QGroupBox* createRGBEffectsGroup();
    QGroupBox* createFanControlGroup();
    QGroupBox* createProfileManagementGroup();
    
    // System monitoring helpers
    SystemData collectSystemData();
    QList<QPair<QString, double>> getCPUTemperatures();
    QList<QPair<QString, double>> getFanSpeeds();
    QList<QPair<QString, double>> getVoltages();
    
    // RGB control helpers
    bool sendRGBCommand(const QByteArray &command);
    QList<QString> detectRGBDevices();
    void applyStaticColor(const QColor &color);
    void applyBreathingEffect(const QColor &color1, const QColor &color2, int speed);
    void applyRainbowEffect(int speed);
    
    // Fan control helpers
    bool setFanSpeed(const QString &fanDevice, int pwmValue);
    QList<QString> detectFanDevices();
    int calculateFanSpeed(double temperature, const FanProfile &profile);
    
    // Configuration helpers
    void loadRGBProfiles();
    void saveRGBProfiles();
    void loadFanProfiles();
    void saveFanProfiles();
    void updateRGBPreview(const RGBEffect &effect);
    void updateProfilesTree();
    int calculateFanSpeed(double temperature);

private:
    // Main UI components
    QTabWidget *m_tabWidget;
    
    // System monitoring tab
    QWidget *m_systemMonitorTab;
    QLabel *m_cpuUsageLabel;
    QLabel *m_cpuTempLabel;
    QLabel *m_memoryUsageLabel;
    QLabel *m_diskUsageLabel;
    QLabel *m_gpuUsageLabel;
    QLabel *m_gpuTempLabel;
    QProgressBar *m_cpuProgressBar;
    QProgressBar *m_memoryProgressBar;
    QProgressBar *m_diskProgressBar;
    QProgressBar *m_gpuProgressBar;
    QTreeWidget *m_temperatureTree;
    QTreeWidget *m_fanSpeedTree;
    QTextEdit *m_systemInfoText;
    
    // RGB control tab
    QWidget *m_rgbControlTab;
    QComboBox *m_rgbEffectCombo;
    QPushButton *m_primaryColorBtn;
    QPushButton *m_secondaryColorBtn;
    QSlider *m_brightnessSlider;
    QSlider *m_speedSlider;
    QLabel *m_brightnessLabel;
    QLabel *m_speedLabel;
    QComboBox *m_rgbProfileCombo;
    QPushButton *m_saveRGBProfileBtn;
    QPushButton *m_loadRGBProfileBtn;
    QLabel *m_rgbPreview;
    
    // Fan control tab
    QWidget *m_fanControlTab;
    QCheckBox *m_fanControlEnabled;
    QComboBox *m_fanProfileCombo;
    QTreeWidget *m_fanCurveTree;
    QSlider *m_manualFanSlider;
    QLabel *m_manualFanLabel;
    QPushButton *m_saveFanProfileBtn;
    QPushButton *m_loadFanProfileBtn;
    QLabel *m_fanStatusLabel;
    
    // Profiles tab
    QWidget *m_profilesTab;
    QTreeWidget *m_profilesTree;
    QPushButton *m_createProfileBtn;
    QPushButton *m_deleteProfileBtn;
    QPushButton *m_exportProfileBtn;
    QPushButton *m_importProfileBtn;
    QTextEdit *m_profileDescriptionEdit;
    
    // Core components
    SystemMonitorThread *m_systemMonitor;
    RGBEffectManager *m_rgbManager;
    FanControlManager *m_fanManager;
    ConfigurationManager *m_configManager;
    
    // Current state
    SystemData m_lastSystemData;
    RGBEffect m_currentRGBEffect;
    FanProfile m_currentFanProfile;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    
    // Configuration
    QSettings *m_settings;
    QMap<QString, RGBEffect> m_rgbProfiles;
    QMap<QString, FanProfile> m_fanProfiles;
    QStringList m_rgbDevices;
    QStringList m_fanDevices;
    
    // Timers
    QTimer *m_systemUpdateTimer;
    QTimer *m_rgbUpdateTimer;
    QTimer *m_fanUpdateTimer;
    
    // Thread safety
    QMutex m_dataMutex;
    bool m_monitoringActive;
};

/**
 * @brief System monitoring thread for real-time data collection
 */
class SystemMonitorThread : public QThread
{
    Q_OBJECT

public:
    explicit SystemMonitorThread(QObject *parent = nullptr);
    ~SystemMonitorThread();
    
    void setUpdateInterval(int msec);
    void stopMonitoring();

signals:
    void dataUpdated(const SystemData &data);

protected:
    void run() override;

private:
    SystemData collectData();
    
    int m_updateInterval;
    bool m_running;
    QMutex m_mutex;
};

/**
 * @brief RGB effect management
 */
class RGBEffectManager : public QObject
{
    Q_OBJECT

public:
    explicit RGBEffectManager(QObject *parent = nullptr);
    
    bool initializeDevices();
    bool applyEffect(const RGBEffect &effect);
    QList<QString> getAvailableDevices();
    QList<QString> getAvailableEffects();

public slots:
    void updateEffect();

private:
    bool sendCommand(const QString &device, const QByteArray &command);
    void generateRainbowEffect();
    void generateBreathingEffect();
    void generateWaveEffect();
    QByteArray createStaticColorCommand(const QColor &color, int brightness);
    
    QStringList m_devices;
    QTimer *m_effectTimer;
    RGBEffect m_currentEffect;
    int m_effectStep;
};

/**
 * @brief Fan control management
 */
class FanControlManager : public QObject
{
    Q_OBJECT

public:
    explicit FanControlManager(QObject *parent = nullptr);
    
    bool initializeFans();
    bool applyProfile(const FanProfile &profile);
    QList<QString> getAvailableFans();
    bool setFanSpeed(const QString &fan, int pwmValue);

public slots:
    void updateFanSpeeds(const SystemData &data);

private:
    bool writeToFanDevice(const QString &device, int value);
    int readFromFanDevice(const QString &device);
    
    QStringList m_fanDevices;
    FanProfile m_currentProfile;
    QMap<QString, int> m_lastFanSpeeds;
};

/**
 * @brief Configuration management for profiles and settings
 */
class ConfigurationManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigurationManager(QObject *parent = nullptr);
    
    // RGB profile management
    QMap<QString, RGBEffect> loadRGBProfiles();
    bool saveRGBProfiles(const QMap<QString, RGBEffect> &profiles);
    
    // Fan profile management
    QMap<QString, FanProfile> loadFanProfiles();
    bool saveFanProfiles(const QMap<QString, FanProfile> &profiles);
    
    // General settings
    QVariant getSetting(const QString &key, const QVariant &defaultValue = QVariant());
    void setSetting(const QString &key, const QVariant &value);

private:
    void createDefaultProfiles();
    
    QString m_configDir;
    QSettings *m_settings;
};

#endif // RGBFANCONTROL_H
