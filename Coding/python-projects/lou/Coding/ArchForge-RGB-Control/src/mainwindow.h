#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QColorDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QProcess>
#include <QThread>
#include <QSettings>
#include <QLineEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include "rgbcommandbatcher.h"
#include "fancontroller.h"
#include "spatialeffects.h"
#include "powermanager.h"

/**
 * @brief Main window for testing RGB command batcher functionality
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Color selection
    void selectPrimaryColor();
    void selectSecondaryColor();
    
    // RGB effects
    void applyStaticColor();
    void applyBreathingEffect();
    void applyRainbowEffect();
    void applyWaveEffect();
    void clearAllKeys();
    
    // Device management
    void refreshDevices();
    void changeDevice();
    void startBatcher();
    void stopBatcher();
    
    // Batcher feedback
    void onBatchSent(int batchSize);
    void onBatcherError(const QString &error);
    void onDeviceChanged(const QString &newDevice);
    
    // Updates
    void updateStatus();
    void updateBrightness(int value);
    void updateSpeed(int value);
    
    // System control functions
    void setFanMode(const QString &mode);
    void launchFanGUI();
    void setPowerProfile(const QString &profile);
    void showTLPStats();
    void refreshTemperatures();
    void launchTemperatureMonitor();
    void startLidMonitoring();
    void testLidClear();
    void stopLidMonitoring();
    
    // Python RGB integration functions
    void pythonSetKeyColor(const QString &keyName, int red, int green, int blue);
    void pythonClearKeypad();
    void pythonRainbowEffect();
    void pythonBreathingEffect();
    void pythonWaveEffect();
    void pythonCheckDevicePermissions();
    void pythonFixRGBDevice();
    void pythonTestAllKeys();
    void pythonApplyStaticColor(const QColor &color);

private:
    void setupUI();
    void setupConnections();
    void updateButtonStates();
    void logMessage(const QString &message);
    void updateColorButton(QPushButton *button, const QColor &color);
    
    // Tab setup functions
    void setupRGBControlTab(QTabWidget *parentTabs);
    void setupFanControlTab(QTabWidget *parentTabs);
    void setupPowerManagementTab(QTabWidget *parentTabs);
    void setupTemperatureMonitorTab(QTabWidget *parentTabs);
    void setupLidMonitorTab(QTabWidget *parentTabs);
    void setupTestingTab(QTabWidget *parentTabs);
    
    // Clean Install Backup/Restore tab functions
    void setupBackupTab(QTabWidget *parentTabs);
    void setupRestoreTab(QTabWidget *parentTabs);
    void setupLogsTab(QTabWidget *parentTabs);
    
    // Group setup functions
    void setupDeviceGroup();
    void setupColorGroup();
    void setupEffectGroup();
    void setupSettingsGroup();
    void setupTestGroup();
    void setupStatusGroup();
    
    // Apply effects with current settings
    void applyCurrentSettings();
    
    // Test patterns
    void testKeyGroups();
    void testIndividualKeys();

private:
    // Core RGB functionality
    RGBCommandBatcher *m_rgbBatcher;
    FanController *m_fanController;
    
    // UI components
    QWidget *m_centralWidget;
    
    // Device controls
    QGroupBox *m_deviceGroup;
    QComboBox *m_deviceCombo;
    QPushButton *m_refreshDevicesBtn;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QLabel *m_deviceStatusLabel;
    
    // Color controls
    QGroupBox *m_colorGroup;
    QPushButton *m_primaryColorBtn;
    QPushButton *m_secondaryColorBtn;
    QColor m_primaryColor;
    QColor m_secondaryColor;
    
    // Effect controls
    QGroupBox *m_effectGroup;
    QPushButton *m_staticColorBtn;
    QPushButton *m_breathingBtn;
    QPushButton *m_rainbowBtn;
    QPushButton *m_waveBtn;
    QPushButton *m_clearBtn;
    
    // Settings controls
    QGroupBox *m_settingsGroup;
    QSlider *m_brightnessSlider;
    QSlider *m_speedSlider;
    QLabel *m_brightnessLabel;
    QLabel *m_speedLabel;
    QSpinBox *m_batchSizeSpinBox;
    QSpinBox *m_maxDelaySpinBox;
    
    // Test controls
    QGroupBox *m_testGroup;
    QPushButton *m_testGroupsBtn;
    QPushButton *m_testKeysBtn;
    QCheckBox *m_enableTestsCheck;
    
    // Status and monitoring
    QGroupBox *m_statusGroup;
    QLabel *m_queueSizeLabel;
    QLabel *m_batchCountLabel;
    QLabel *m_errorCountLabel;
    QProgressBar *m_activityIndicator;
    QTextEdit *m_logText;
    
    // Timers and state
    QTimer *m_updateTimer;
    QTimer *m_effectTimer;
    int m_batchCount;
    int m_errorCount;
    bool m_effectRunning;
    int m_effectStep;
};

#endif // MAINWINDOW_H
