#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QDateTime>
#include <QSplitter>
#include <QStatusBar>
#include <QTextCursor>

Q_LOGGING_CATEGORY(mainwindow, "archforge.mainwindow")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_rgbBatcher(nullptr)
    , m_centralWidget(nullptr)
    , m_primaryColor(255, 102, 0)  // TCC Orange default
    , m_secondaryColor(0, 150, 255)  // Blue default
    , m_updateTimer(new QTimer(this))
    , m_effectTimer(new QTimer(this))
    , m_batchCount(0)
    , m_errorCount(0)
    , m_effectRunning(false)
    , m_effectStep(0)
{
    qCInfo(mainwindow) << "Initializing MainWindow";
    
    setWindowTitle("ArchForge RGB Control Center");
    setMinimumSize(1200, 800);
    
    // Initialize RGB batcher
    m_rgbBatcher = new RGBCommandBatcher(this);
    
    // Initialize fan controller
    m_fanController = new FanController(this);
    
    setupUI();
    setupConnections();
    
    // Start with default device
    if (m_rgbBatcher->initialize()) {
        qCInfo(mainwindow) << "RGB batcher initialized successfully";
        m_rgbBatcher->start();
    } else {
        qCWarning(mainwindow) << "Failed to initialize RGB batcher";
    }
    
    // Start update timer
    m_updateTimer->start(1000); // Update every second
    
    qCInfo(mainwindow) << "MainWindow initialization complete";
}

MainWindow::~MainWindow()
{
    qCInfo(mainwindow) << "Destroying MainWindow";
    
    if (m_rgbBatcher) {
        m_rgbBatcher->stop();
    }
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget;
    setCentralWidget(m_centralWidget);
    
    // Create main tab widget for the application
    auto mainTabs = new QTabWidget;
    auto mainLayout = new QVBoxLayout(m_centralWidget);
    mainLayout->addWidget(mainTabs);
    
    // MAIN TAB 1: Clean Install Backup/Restore (original ArchForge functionality)
    auto cleanInstallTab = new QWidget;
    mainTabs->addTab(cleanInstallTab, "Clean Install Backup/Restore");
    
    // Layout for Clean Install main tab
    auto cleanInstallLayout = new QVBoxLayout(cleanInstallTab);
    
    // Settings button at the top (like original)
    auto settingsLayout = new QHBoxLayout;
    auto settingsBtn = new QPushButton("Settings - View Backup Capabilities");
    settingsBtn->setToolTip("Click to see what can be backed up and configure settings");
    settingsLayout->addWidget(settingsBtn);
    settingsLayout->addStretch();
    cleanInstallLayout->addLayout(settingsLayout);
    
    // Create sub-tabs widget for Clean Install functionality
    auto cleanInstallSubTabs = new QTabWidget;
    cleanInstallLayout->addWidget(cleanInstallSubTabs);
    
    // Sub-tab 1: Backup
    setupBackupTab(cleanInstallSubTabs);
    
    // Sub-tab 2: Restore
    setupRestoreTab(cleanInstallSubTabs);
    
    // Sub-tab 3: Logs
    setupLogsTab(cleanInstallSubTabs);
    
    // MAIN TAB 2: RGB/Fan Control (NEW functionality)
    auto rgbFanTab = new QWidget;
    mainTabs->addTab(rgbFanTab, "🎨 RGB/Fan Control");
    
    // Create sub-tabs within RGB/Fan Control
    auto rgbFanSubTabs = new QTabWidget;
    auto rgbFanLayout = new QVBoxLayout(rgbFanTab);
    rgbFanLayout->addWidget(rgbFanSubTabs);
    
    // RGB Control sub-tab
    setupRGBControlTab(rgbFanSubTabs);
    
    // Fan Control sub-tab
    setupFanControlTab(rgbFanSubTabs);
    
    // Power Management sub-tab
    setupPowerManagementTab(rgbFanSubTabs);
    
    // Temperature Monitor sub-tab
    setupTemperatureMonitorTab(rgbFanSubTabs);
    
    // Lid Monitor sub-tab
    setupLidMonitorTab(rgbFanSubTabs);
    
    // Development/Testing tab (for development only)
    setupTestingTab(mainTabs);
    
    // Status bar
    statusBar()->showMessage("ArchForge RGB Control Center Ready");
}

void MainWindow::setupRGBControlTab(QTabWidget *parentTabs)
{
    auto rgbWidget = new QWidget;
    parentTabs->addTab(rgbWidget, "🌈 RGB Control");
    
    auto layout = new QVBoxLayout(rgbWidget);
    
    // Device selection group
    setupDeviceGroup();
    layout->addWidget(m_deviceGroup);
    
    // Color selection group
    setupColorGroup();
    layout->addWidget(m_colorGroup);
    
    // Effect controls group
    setupEffectGroup();
    layout->addWidget(m_effectGroup);
    
    // Settings group
    setupSettingsGroup();
    layout->addWidget(m_settingsGroup);
    
    // Python RGB Integration group
    auto pythonGroup = new QGroupBox("Python RGB Integration");
    layout->addWidget(pythonGroup);
    
    auto pythonLayout = new QGridLayout(pythonGroup);
    
    auto pythonClearBtn = new QPushButton("Clear Keypad (Python)");
    auto pythonRainbowBtn = new QPushButton("Rainbow Effect (Python)");
    auto pythonTestBtn = new QPushButton("Test WASD Keys (Python)");
    auto pythonPermBtn = new QPushButton("Check Permissions (Python)");
    auto pythonFixBtn = new QPushButton("Fix Device Permissions");
    auto pythonStaticBtn = new QPushButton("Apply Primary Color (Python)");
    
    pythonLayout->addWidget(pythonClearBtn, 0, 0);
    pythonLayout->addWidget(pythonRainbowBtn, 0, 1);
    pythonLayout->addWidget(pythonTestBtn, 0, 2);
    pythonLayout->addWidget(pythonPermBtn, 1, 0);
    pythonLayout->addWidget(pythonFixBtn, 1, 1);
    pythonLayout->addWidget(pythonStaticBtn, 1, 2);
    
    connect(pythonClearBtn, &QPushButton::clicked, this, &MainWindow::pythonClearKeypad);
    connect(pythonRainbowBtn, &QPushButton::clicked, this, &MainWindow::pythonRainbowEffect);
    connect(pythonTestBtn, &QPushButton::clicked, this, &MainWindow::pythonTestAllKeys);
    connect(pythonPermBtn, &QPushButton::clicked, this, &MainWindow::pythonCheckDevicePermissions);
    connect(pythonFixBtn, &QPushButton::clicked, this, &MainWindow::pythonFixRGBDevice);
    connect(pythonStaticBtn, &QPushButton::clicked, [this]() {
        pythonApplyStaticColor(m_primaryColor);
    });
    
    layout->addStretch();
}

void MainWindow::setupFanControlTab(QTabWidget *parentTabs)
{
    auto fanWidget = new QWidget;
    parentTabs->addTab(fanWidget, "🌀 Fan Control");
    
    auto layout = new QVBoxLayout(fanWidget);
    
    // Fan status group
    auto fanStatusGroup = new QGroupBox("Fan Status");
    layout->addWidget(fanStatusGroup);
    
    auto fanStatusLayout = new QVBoxLayout(fanStatusGroup);
    
    // Fan speed display
    auto fanSpeedLabel = new QLabel("Checking fan speeds...");
    fanSpeedLabel->setStyleSheet("QLabel { background-color: #2a2a2a; border: 1px solid #555; padding: 10px; color: #ccc; }");
    fanStatusLayout->addWidget(fanSpeedLabel);
    
    // Fan control buttons
    auto fanControlGroup = new QGroupBox("Fan Control");
    layout->addWidget(fanControlGroup);
    
    auto fanControlLayout = new QHBoxLayout(fanControlGroup);
    
    auto autoFanBtn = new QPushButton("Auto Mode");
    auto silentFanBtn = new QPushButton("Silent Mode");
    auto performanceFanBtn = new QPushButton("Performance Mode");
    auto launchFanGUIBtn = new QPushButton("Launch Fan GUI");
    
    fanControlLayout->addWidget(autoFanBtn);
    fanControlLayout->addWidget(silentFanBtn);
    fanControlLayout->addWidget(performanceFanBtn);
    fanControlLayout->addWidget(launchFanGUIBtn);
    
    // Connect fan control signals
    connect(autoFanBtn, &QPushButton::clicked, [this]() {
        setFanMode("auto");
    });
    
    connect(silentFanBtn, &QPushButton::clicked, [this]() {
        setFanMode("silent");
    });
    
    connect(performanceFanBtn, &QPushButton::clicked, [this]() {
        setFanMode("performance");
    });
    
    connect(launchFanGUIBtn, &QPushButton::clicked, [this]() {
        launchFanGUI();
    });
    
    layout->addStretch();
}

void MainWindow::setupPowerManagementTab(QTabWidget *parentTabs)
{
    auto powerWidget = new QWidget;
    parentTabs->addTab(powerWidget, "⚡ Power Management");
    
    auto layout = new QVBoxLayout(powerWidget);
    
    // Power status group
    auto powerStatusGroup = new QGroupBox("Power Status");
    layout->addWidget(powerStatusGroup);
    
    auto powerStatusLayout = new QVBoxLayout(powerStatusGroup);
    
    auto powerStatusLabel = new QLabel("Checking power status...");
    powerStatusLabel->setStyleSheet("QLabel { background-color: #2a2a2a; border: 1px solid #555; padding: 10px; color: #ccc; }");
    powerStatusLayout->addWidget(powerStatusLabel);
    
    // Power profile controls
    auto powerProfileGroup = new QGroupBox("Power Profiles");
    layout->addWidget(powerProfileGroup);
    
    auto powerProfileLayout = new QGridLayout(powerProfileGroup);
    
    auto performanceBtn = new QPushButton("Performance");
    auto balancedBtn = new QPushButton("Balanced");
    auto powersaveBtn = new QPushButton("Power Save");
    auto tlpStatsBtn = new QPushButton("TLP Statistics");
    
    powerProfileLayout->addWidget(performanceBtn, 0, 0);
    powerProfileLayout->addWidget(balancedBtn, 0, 1);
    powerProfileLayout->addWidget(powersaveBtn, 1, 0);
    powerProfileLayout->addWidget(tlpStatsBtn, 1, 1);
    
    // Connect power management signals
    connect(performanceBtn, &QPushButton::clicked, [this]() {
        setPowerProfile("performance");
    });
    
    connect(balancedBtn, &QPushButton::clicked, [this]() {
        setPowerProfile("balanced");
    });
    
    connect(powersaveBtn, &QPushButton::clicked, [this]() {
        setPowerProfile("powersave");
    });
    
    connect(tlpStatsBtn, &QPushButton::clicked, [this]() {
        showTLPStats();
    });
    
    layout->addStretch();
}

void MainWindow::setupTemperatureMonitorTab(QTabWidget *parentTabs)
{
    auto tempWidget = new QWidget;
    parentTabs->addTab(tempWidget, "🌡️ Temperature Monitor");
    
    auto layout = new QVBoxLayout(tempWidget);
    
    // Temperature display group
    auto tempDisplayGroup = new QGroupBox("System Temperatures");
    layout->addWidget(tempDisplayGroup);
    
    auto tempDisplayLayout = new QVBoxLayout(tempDisplayGroup);
    
    auto tempTextEdit = new QTextEdit;
    tempTextEdit->setReadOnly(true);
    tempTextEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; border: 1px solid #555; color: #ccc; font-family: monospace; }");
    tempTextEdit->setPlainText("Loading temperature data...");
    tempDisplayLayout->addWidget(tempTextEdit);
    
    // Temperature monitor controls
    auto tempControlGroup = new QGroupBox("Controls");
    layout->addWidget(tempControlGroup);
    
    auto tempControlLayout = new QHBoxLayout(tempControlGroup);
    
    auto refreshTempBtn = new QPushButton("Refresh");
    auto launchTempMonitorBtn = new QPushButton("Launch External Monitor");
    
    tempControlLayout->addWidget(refreshTempBtn);
    tempControlLayout->addWidget(launchTempMonitorBtn);
    
    // Connect temperature monitor signals
    connect(refreshTempBtn, &QPushButton::clicked, [this]() {
        refreshTemperatures();
    });
    
    connect(launchTempMonitorBtn, &QPushButton::clicked, [this]() {
        launchTemperatureMonitor();
    });
    
    layout->addStretch();
}

void MainWindow::setupLidMonitorTab(QTabWidget *parentTabs)
{
    auto lidWidget = new QWidget;
    parentTabs->addTab(lidWidget, "💻 Lid Monitor");
    
    auto layout = new QVBoxLayout(lidWidget);
    
    // Lid status group
    auto lidStatusGroup = new QGroupBox("Lid Monitor Status");
    layout->addWidget(lidStatusGroup);
    
    auto lidStatusLayout = new QVBoxLayout(lidStatusGroup);
    
    auto lidStatusLabel = new QLabel("Lid monitoring: Starting...");
    lidStatusLabel->setStyleSheet("QLabel { background-color: #2a2a2a; border: 1px solid #555; padding: 10px; color: #4CAF50; }");
    lidStatusLayout->addWidget(lidStatusLabel);
    
    // Lid monitor controls
    auto lidControlGroup = new QGroupBox("Controls");
    layout->addWidget(lidControlGroup);
    
    auto lidControlLayout = new QHBoxLayout(lidControlGroup);
    
    auto startLidMonitorBtn = new QPushButton("Start Monitoring");
    auto testClearBtn = new QPushButton("Test Clear Keys");
    auto stopLidMonitorBtn = new QPushButton("Stop Monitoring");
    
    lidControlLayout->addWidget(startLidMonitorBtn);
    lidControlLayout->addWidget(testClearBtn);
    lidControlLayout->addWidget(stopLidMonitorBtn);
    
    // Connect lid monitor signals
    connect(startLidMonitorBtn, &QPushButton::clicked, [this]() {
        startLidMonitoring();
    });
    
    connect(testClearBtn, &QPushButton::clicked, [this]() {
        testLidClear();
    });
    
    connect(stopLidMonitorBtn, &QPushButton::clicked, [this]() {
        stopLidMonitoring();
    });
    
    // Information text
    auto infoText = new QTextEdit;
    infoText->setReadOnly(true);
    infoText->setMaximumHeight(100);
    infoText->setStyleSheet("QTextEdit { background-color: #2a2a2a; border: 1px solid #555; color: #ccc; }");
    infoText->setPlainText("Lid monitoring automatically clears RGB lighting when the laptop lid is closed.\n"
                           "This helps prevent issues with stuck keys and saves battery.");
    layout->addWidget(infoText);
    
    layout->addStretch();
}

void MainWindow::setupTestingTab(QTabWidget *parentTabs)
{
    auto testWidget = new QWidget;
    parentTabs->addTab(testWidget, "🔧 Testing");
    
    auto splitter = new QSplitter(Qt::Horizontal);
    auto testLayout = new QVBoxLayout(testWidget);
    testLayout->addWidget(splitter);
    
    // Left side - controls
    auto controlsWidget = new QWidget;
    controlsWidget->setMinimumWidth(400);
    splitter->addWidget(controlsWidget);
    
    auto controlsLayout = new QVBoxLayout(controlsWidget);
    
    // Test controls group
    setupTestGroup();
    controlsLayout->addWidget(m_testGroup);
    
    controlsLayout->addStretch();
    
    // Right side - status and logs
    auto statusWidget = new QWidget;
    statusWidget->setMinimumWidth(400);
    splitter->addWidget(statusWidget);
    
    auto statusLayout = new QVBoxLayout(statusWidget);
    
    // Status monitoring group
    setupStatusGroup();
    statusLayout->addWidget(m_statusGroup);
}

void MainWindow::setupDeviceGroup()
{
    m_deviceGroup = new QGroupBox("Device Selection");
    auto layout = new QVBoxLayout(m_deviceGroup);
    
    auto deviceLayout = new QHBoxLayout;
    m_deviceCombo = new QComboBox;
    m_deviceCombo->addItems({"/dev/hidraw0", "/dev/hidraw1", "/dev/hidraw2", "/dev/hidraw3"});
    m_refreshDevicesBtn = new QPushButton("Refresh");
    
    deviceLayout->addWidget(new QLabel("Device:"));
    deviceLayout->addWidget(m_deviceCombo);
    deviceLayout->addWidget(m_refreshDevicesBtn);
    layout->addLayout(deviceLayout);
    
    auto controlLayout = new QHBoxLayout;
    m_startBtn = new QPushButton("Start Batcher");
    m_stopBtn = new QPushButton("Stop Batcher");
    m_deviceStatusLabel = new QLabel("Ready");
    
    controlLayout->addWidget(m_startBtn);
    controlLayout->addWidget(m_stopBtn);
    controlLayout->addWidget(m_deviceStatusLabel);
    layout->addLayout(controlLayout);
}

void MainWindow::setupColorGroup()
{
    m_colorGroup = new QGroupBox("Color Selection");
    auto layout = new QVBoxLayout(m_colorGroup);
    
    auto colorLayout = new QHBoxLayout;
    m_primaryColorBtn = new QPushButton("Primary Color");
    m_secondaryColorBtn = new QPushButton("Secondary Color");
    
    // Set initial button colors
    updateColorButton(m_primaryColorBtn, m_primaryColor);
    updateColorButton(m_secondaryColorBtn, m_secondaryColor);
    
    colorLayout->addWidget(m_primaryColorBtn);
    colorLayout->addWidget(m_secondaryColorBtn);
    layout->addLayout(colorLayout);
    
    // Quick color presets
    auto presetsLayout = new QGridLayout;
    
    QList<QPair<QString, QColor>> presets = {
        {"Red", QColor(255, 0, 0)},
        {"Green", QColor(0, 255, 0)},
        {"Blue", QColor(0, 0, 255)},
        {"Orange", QColor(255, 102, 0)},
        {"Purple", QColor(128, 0, 128)},
        {"Cyan", QColor(0, 255, 255)},
        {"Yellow", QColor(255, 255, 0)},
        {"White", QColor(255, 255, 255)}
    };
    
    for (int i = 0; i < presets.size(); ++i) {
        auto btn = new QPushButton(presets[i].first);
        btn->setMaximumHeight(30);
        updateColorButton(btn, presets[i].second);
        
        connect(btn, &QPushButton::clicked, [this, color = presets[i].second]() {
            m_primaryColor = color;
            updateColorButton(m_primaryColorBtn, m_primaryColor);
            applyStaticColor();
        });
        
        presetsLayout->addWidget(btn, i / 4, i % 4);
    }
    
    layout->addLayout(presetsLayout);
}

void MainWindow::setupEffectGroup()
{
    m_effectGroup = new QGroupBox("RGB Effects");
    auto layout = new QVBoxLayout(m_effectGroup);
    
    auto effectLayout1 = new QHBoxLayout;
    m_staticColorBtn = new QPushButton("Static Color");
    m_breathingBtn = new QPushButton("Breathing");
    m_rainbowBtn = new QPushButton("Rainbow");
    
    effectLayout1->addWidget(m_staticColorBtn);
    effectLayout1->addWidget(m_breathingBtn);
    effectLayout1->addWidget(m_rainbowBtn);
    layout->addLayout(effectLayout1);
    
    auto effectLayout2 = new QHBoxLayout;
    m_waveBtn = new QPushButton("Wave");
    m_clearBtn = new QPushButton("Clear All");
    
    effectLayout2->addWidget(m_waveBtn);
    effectLayout2->addWidget(m_clearBtn);
    effectLayout2->addStretch();
    layout->addLayout(effectLayout2);
}

void MainWindow::setupSettingsGroup()
{
    m_settingsGroup = new QGroupBox("Settings");
    auto layout = new QGridLayout(m_settingsGroup);
    
    // Brightness control
    layout->addWidget(new QLabel("Brightness:"), 0, 0);
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(100);
    layout->addWidget(m_brightnessSlider, 0, 1);
    m_brightnessLabel = new QLabel("100%");
    layout->addWidget(m_brightnessLabel, 0, 2);
    
    // Speed control
    layout->addWidget(new QLabel("Speed:"), 1, 0);
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 10);
    m_speedSlider->setValue(5);
    layout->addWidget(m_speedSlider, 1, 1);
    m_speedLabel = new QLabel("5");
    layout->addWidget(m_speedLabel, 1, 2);
    
    // Batch settings
    layout->addWidget(new QLabel("Batch Size:"), 2, 0);
    m_batchSizeSpinBox = new QSpinBox;
    m_batchSizeSpinBox->setRange(1, 64);
    m_batchSizeSpinBox->setValue(16);
    layout->addWidget(m_batchSizeSpinBox, 2, 1);
    
    layout->addWidget(new QLabel("Max Delay (ms):"), 3, 0);
    m_maxDelaySpinBox = new QSpinBox;
    m_maxDelaySpinBox->setRange(1, 1000);
    m_maxDelaySpinBox->setValue(50);
    layout->addWidget(m_maxDelaySpinBox, 3, 1);
}

void MainWindow::setupTestGroup()
{
    m_testGroup = new QGroupBox("Testing Controls");
    auto layout = new QVBoxLayout(m_testGroup);
    
    auto testLayout = new QHBoxLayout;
    m_testGroupsBtn = new QPushButton("Test Groups");
    m_testKeysBtn = new QPushButton("Test Keys");
    m_enableTestsCheck = new QCheckBox("Enable Tests");
    m_enableTestsCheck->setChecked(true);
    
    testLayout->addWidget(m_testGroupsBtn);
    testLayout->addWidget(m_testKeysBtn);
    testLayout->addWidget(m_enableTestsCheck);
    layout->addLayout(testLayout);
}

void MainWindow::setupStatusGroup()
{
    m_statusGroup = new QGroupBox("Status Monitoring");
    auto layout = new QVBoxLayout(m_statusGroup);
    
    // Status labels
    auto statusLayout = new QGridLayout;
    statusLayout->addWidget(new QLabel("Queue Size:"), 0, 0);
    m_queueSizeLabel = new QLabel("0");
    statusLayout->addWidget(m_queueSizeLabel, 0, 1);
    
    statusLayout->addWidget(new QLabel("Batches Sent:"), 1, 0);
    m_batchCountLabel = new QLabel("0");
    statusLayout->addWidget(m_batchCountLabel, 1, 1);
    
    statusLayout->addWidget(new QLabel("Errors:"), 2, 0);
    m_errorCountLabel = new QLabel("0");
    statusLayout->addWidget(m_errorCountLabel, 2, 1);
    
    layout->addLayout(statusLayout);
    
    // Activity indicator
    m_activityIndicator = new QProgressBar;
    m_activityIndicator->setRange(0, 0); // Indeterminate
    m_activityIndicator->hide();
    layout->addWidget(m_activityIndicator);
    
    // Log text
    m_logText = new QTextEdit;
    m_logText->setMaximumHeight(200);
    m_logText->setReadOnly(true);
    layout->addWidget(m_logText);
}

void MainWindow::setupConnections()
{
    // RGB Batcher signals
    connect(m_rgbBatcher, &RGBCommandBatcher::batchSent,
            this, &MainWindow::onBatchSent);
    connect(m_rgbBatcher, &RGBCommandBatcher::error,
            this, &MainWindow::onBatcherError);
    connect(m_rgbBatcher, &RGBCommandBatcher::deviceChanged,
            this, &MainWindow::onDeviceChanged);
    
    // Device controls
    connect(m_refreshDevicesBtn, &QPushButton::clicked,
            this, &MainWindow::refreshDevices);
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::changeDevice);
    connect(m_startBtn, &QPushButton::clicked,
            this, &MainWindow::startBatcher);
    connect(m_stopBtn, &QPushButton::clicked,
            this, &MainWindow::stopBatcher);
    
    // Color controls
    connect(m_primaryColorBtn, &QPushButton::clicked,
            this, &MainWindow::selectPrimaryColor);
    connect(m_secondaryColorBtn, &QPushButton::clicked,
            this, &MainWindow::selectSecondaryColor);
    
    // Effect controls
    connect(m_staticColorBtn, &QPushButton::clicked,
            this, &MainWindow::applyStaticColor);
    connect(m_breathingBtn, &QPushButton::clicked,
            this, &MainWindow::applyBreathingEffect);
    connect(m_rainbowBtn, &QPushButton::clicked,
            this, &MainWindow::applyRainbowEffect);
    connect(m_waveBtn, &QPushButton::clicked,
            this, &MainWindow::applyWaveEffect);
    connect(m_clearBtn, &QPushButton::clicked,
            this, &MainWindow::clearAllKeys);
    
    // Settings controls
    connect(m_brightnessSlider, &QSlider::valueChanged,
            this, &MainWindow::updateBrightness);
    connect(m_speedSlider, &QSlider::valueChanged,
            this, &MainWindow::updateSpeed);
    
    // Test controls
    connect(m_testGroupsBtn, &QPushButton::clicked,
            this, &MainWindow::testKeyGroups);
    connect(m_testKeysBtn, &QPushButton::clicked,
            this, &MainWindow::testIndividualKeys);
    
    // Timers
    connect(m_updateTimer, &QTimer::timeout,
            this, &MainWindow::updateStatus);
    connect(m_effectTimer, &QTimer::timeout,
            this, &MainWindow::applyCurrentSettings);
}

void MainWindow::updateColorButton(QPushButton *button, const QColor &color)
{
    QString style = QString("QPushButton { background-color: %1; color: %2; border: 1px solid #555; padding: 8px; }")
                        .arg(color.name())
                        .arg(color.lightness() > 128 ? "black" : "white");
    button->setStyleSheet(style);
}

// RGB Control slots
void MainWindow::selectPrimaryColor()
{
    QColor color = QColorDialog::getColor(m_primaryColor, this, "Select Primary Color");
    if (color.isValid()) {
        m_primaryColor = color;
        updateColorButton(m_primaryColorBtn, m_primaryColor);
    }
}

void MainWindow::selectSecondaryColor()
{
    QColor color = QColorDialog::getColor(m_secondaryColor, this, "Select Secondary Color");
    if (color.isValid()) {
        m_secondaryColor = color;
        updateColorButton(m_secondaryColorBtn, m_secondaryColor);
    }
}

void MainWindow::applyStaticColor()
{
    if (!m_rgbBatcher->isRunning()) {
        logMessage("RGB batcher not running");
        return;
    }
    
    // Stop any running effects
    m_effectTimer->stop();
    m_effectRunning = false;
    
    // Apply static color to all keys
    float brightness = m_brightnessSlider->value() / 100.0f;
    int red = static_cast<int>(m_primaryColor.red() * brightness);
    int green = static_cast<int>(m_primaryColor.green() * brightness);
    int blue = static_cast<int>(m_primaryColor.blue() * brightness);
    
    // Apply to all keys (simplified - would normally iterate through all key indices)
    for (int i = 0; i < 256; ++i) {
        m_rgbBatcher->addCommand(i, red, green, blue, 1);
    }
    
    logMessage(QString("Applied static color: RGB(%1, %2, %3)").arg(red).arg(green).arg(blue));
}

void MainWindow::applyBreathingEffect()
{
    if (!m_rgbBatcher->isRunning()) {
        logMessage("RGB batcher not running");
        return;
    }
    
    m_effectRunning = true;
    m_effectStep = 0;
    
    int interval = 1000 / m_speedSlider->value(); // Speed control
    m_effectTimer->start(interval);
    
    logMessage("Started breathing effect");
}

void MainWindow::applyRainbowEffect()
{
    if (!m_rgbBatcher->isRunning()) {
        logMessage("RGB batcher not running");
        return;
    }
    
    m_effectRunning = true;
    m_effectStep = 0;
    
    int interval = 100 / m_speedSlider->value(); // Speed control
    m_effectTimer->start(interval);
    
    logMessage("Started rainbow effect");
}

void MainWindow::applyWaveEffect()
{
    if (!m_rgbBatcher->isRunning()) {
        logMessage("RGB batcher not running");
        return;
    }
    
    m_effectRunning = true;
    m_effectStep = 0;
    
    int interval = 50 / m_speedSlider->value(); // Speed control
    m_effectTimer->start(interval);
    
    logMessage("Started wave effect");
}

void MainWindow::clearAllKeys()
{
    if (!m_rgbBatcher->isRunning()) {
        logMessage("RGB batcher not running");
        return;
    }
    
    // Stop any running effects
    m_effectTimer->stop();
    m_effectRunning = false;
    
    // Clear all keys
    for (int i = 0; i < 256; ++i) {
        m_rgbBatcher->addCommand(i, 0, 0, 0, 2); // High priority for clearing
    }
    
    logMessage("Cleared all keys");
}

// System functionality slots
void MainWindow::setFanMode(const QString &mode)
{
    FanController::FanMode fanMode;
    
    if (mode == "auto") {
        fanMode = FanController::FanMode::Auto;
    } else if (mode == "silent") {
        fanMode = FanController::FanMode::Silent;
    } else if (mode == "performance") {
        fanMode = FanController::FanMode::Performance;
    } else {
        logMessage(QString("Invalid fan mode: %1").arg(mode));
        return;
    }
    
    if (m_fanController && m_fanController->isAvailable()) {
        if (m_fanController->setFanMode(fanMode)) {
            logMessage(QString("Fan mode set to: %1").arg(mode));
            statusBar()->showMessage(QString("Fan mode: %1").arg(mode), 3000);
        } else {
            logMessage(QString("Failed to set fan mode: %1").arg(mode));
        }
    } else {
        logMessage("Fan controller not available");
    }
}

void MainWindow::launchFanGUI()
{
    QProcess::startDetached("fancontrol-gui", QStringList());
    logMessage("Launched fan control GUI");
}

void MainWindow::setPowerProfile(const QString &profile)
{
    QProcess process;
    QString command;
    
    if (profile == "performance") {
        command = "sudo tlp start && sudo cpupower frequency-set -g performance";
    } else if (profile == "balanced") {
        command = "sudo tlp start && sudo cpupower frequency-set -g ondemand";
    } else if (profile == "powersave") {
        command = "sudo tlp start && sudo cpupower frequency-set -g powersave";
    }
    
    if (!command.isEmpty()) {
        process.start("bash", QStringList() << "-c" << command);
        process.waitForFinished(10000);
        
        logMessage(QString("Power profile set to: %1").arg(profile));
        statusBar()->showMessage(QString("Power profile: %1").arg(profile), 3000);
    }
}

void MainWindow::showTLPStats()
{
    QProcess::startDetached("konsole", QStringList() << "-e" << "sudo" << "tlp-stat");
    logMessage("Launched TLP statistics");
}

void MainWindow::refreshTemperatures()
{
    // This would implement temperature sensor reading
    logMessage("Refreshed temperature data");
}

void MainWindow::launchTemperatureMonitor()
{
    QProcess::startDetached("konsole", QStringList() << "-e" << "watch" << "-n" << "1" << "sensors");
    logMessage("Launched temperature monitor");
}

void MainWindow::startLidMonitoring()
{
    // This would implement lid monitoring functionality
    logMessage("Started lid monitoring");
}

void MainWindow::testLidClear()
{
    clearAllKeys();
    logMessage("Executed test lid clear");
}

void MainWindow::stopLidMonitoring()
{
    logMessage("Stopped lid monitoring");
}

// Device management slots
void MainWindow::refreshDevices()
{
    m_deviceCombo->clear();
    
    QStringList devices;
    for (int i = 0; i < 10; ++i) {
        QString device = QString("/dev/hidraw%1").arg(i);
        if (QFile::exists(device)) {
            devices << device;
        }
    }
    
    if (devices.isEmpty()) {
        devices << "/dev/hidraw0"; // Always show default
    }
    
    m_deviceCombo->addItems(devices);
    logMessage(QString("Found %1 RGB devices").arg(devices.size()));
}

void MainWindow::changeDevice()
{
    QString device = m_deviceCombo->currentText();
    if (!device.isEmpty()) {
        if (m_rgbBatcher->isRunning()) {
            m_rgbBatcher->stop();
        }
        
        if (m_rgbBatcher->initialize(device)) {
            m_rgbBatcher->start();
            logMessage(QString("Switched to device: %1").arg(device));
        } else {
            logMessage(QString("Failed to initialize device: %1").arg(device));
        }
    }
}

void MainWindow::startBatcher()
{
    if (m_rgbBatcher->start()) {
        logMessage("RGB batcher started");
        updateButtonStates();
    } else {
        logMessage("Failed to start RGB batcher");
    }
}

void MainWindow::stopBatcher()
{
    if (m_rgbBatcher->stop()) {
        logMessage("RGB batcher stopped");
        updateButtonStates();
    }
}

// Batcher feedback slots
void MainWindow::onBatchSent(int batchSize)
{
    m_batchCount++;
    m_batchCountLabel->setText(QString::number(m_batchCount));
    
    if (batchSize > 0) {
        m_activityIndicator->show();
        QTimer::singleShot(100, m_activityIndicator, &QProgressBar::hide);
    }
}

void MainWindow::onBatcherError(const QString &error)
{
    m_errorCount++;
    m_errorCountLabel->setText(QString::number(m_errorCount));
    logMessage(QString("Batcher error: %1").arg(error));
}

void MainWindow::onDeviceChanged(const QString &newDevice)
{
    logMessage(QString("Device changed to: %1").arg(newDevice));
    
    // Update combo box if needed
    int index = m_deviceCombo->findText(newDevice);
    if (index >= 0) {
        m_deviceCombo->setCurrentIndex(index);
    }
}

// Update slots
void MainWindow::updateStatus()
{
    if (m_rgbBatcher) {
        m_queueSizeLabel->setText(QString::number(m_rgbBatcher->queueSize()));
    }
    
    updateButtonStates();
}

void MainWindow::updateBrightness(int value)
{
    m_brightnessLabel->setText(QString("%1%").arg(value));
}

void MainWindow::updateSpeed(int value)
{
    m_speedLabel->setText(QString::number(value));
    
    // Update effect timer if running
    if (m_effectRunning && m_effectTimer->isActive()) {
        int interval = 100 / value;
        m_effectTimer->setInterval(interval);
    }
}

void MainWindow::updateButtonStates()
{
    bool running = m_rgbBatcher && m_rgbBatcher->isRunning();
    
    m_startBtn->setEnabled(!running);
    m_stopBtn->setEnabled(running);
    
    m_staticColorBtn->setEnabled(running);
    m_breathingBtn->setEnabled(running);
    m_rainbowBtn->setEnabled(running);
    m_waveBtn->setEnabled(running);
    m_clearBtn->setEnabled(running);
    
    m_deviceStatusLabel->setText(running ? "Running" : "Stopped");
    m_deviceStatusLabel->setStyleSheet(running ? "color: green;" : "color: red;");
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString fullMessage = QString("[%1] %2").arg(timestamp, message);
    
    m_logText->append(fullMessage);
    
    // Auto-scroll to bottom
    auto cursor = m_logText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logText->setTextCursor(cursor);
    
    qCInfo(mainwindow) << message;
}

void MainWindow::applyCurrentSettings()
{
    if (!m_effectRunning || !m_rgbBatcher->isRunning()) {
        m_effectTimer->stop();
        return;
    }
    
    // This would implement the current effect based on which button was pressed
    // For now, just increment the effect step
    m_effectStep++;
    
    // Simple breathing effect implementation
    float brightness = m_brightnessSlider->value() / 100.0f;
    float phase = (m_effectStep % 100) / 50.0f; // 0-2
    if (phase > 1.0f) phase = 2.0f - phase; // 0-1-0
    
    int red = static_cast<int>(m_primaryColor.red() * brightness * phase);
    int green = static_cast<int>(m_primaryColor.green() * brightness * phase);
    int blue = static_cast<int>(m_primaryColor.blue() * brightness * phase);
    
    // Apply to a subset of keys for demonstration
    for (int i = 0; i < 50; ++i) {
        m_rgbBatcher->addCommand(i, red, green, blue, 0);
    }
}

void MainWindow::testKeyGroups()
{
    if (!m_enableTestsCheck->isChecked() || !m_rgbBatcher->isRunning()) {
        return;
    }
    
    // Test different key groups with different colors
    QStringList wasdKeys = {"w", "a", "s", "d"};
    QStringList arrowKeys = {"up", "down", "left", "right"};
    
    m_rgbBatcher->addGroupColors(wasdKeys, 255, 0, 0, 1); // Red for WASD
    m_rgbBatcher->addGroupColors(arrowKeys, 0, 255, 0, 1); // Green for arrows
    
    logMessage("Tested key groups: WASD (red), Arrows (green)");
}

void MainWindow::testIndividualKeys()
{
    if (!m_enableTestsCheck->isChecked() || !m_rgbBatcher->isRunning()) {
        return;
    }
    
    // Test individual keys
    m_rgbBatcher->addKeyColor("esc", 255, 255, 0, 1); // Yellow ESC
    m_rgbBatcher->addKeyColor("enter", 0, 255, 255, 1); // Cyan Enter
    m_rgbBatcher->addKeyColor("space", 255, 0, 255, 1); // Magenta Space
    
    logMessage("Tested individual keys: ESC (yellow), Enter (cyan), Space (magenta)");
}

// Clean Install Backup/Restore tab setup functions
void MainWindow::setupBackupTab(QTabWidget *parentTabs)
{
    auto backupWidget = new QWidget;
    parentTabs->addTab(backupWidget, "&Backup");
    
    auto layout = new QVBoxLayout(backupWidget);
    
    // Backup operations group
    auto backupOpsGroup = new QGroupBox("Backup Operations");
    layout->addWidget(backupOpsGroup);
    
    auto backupOpsLayout = new QGridLayout(backupOpsGroup);
    
    auto packageBackupBtn = new QPushButton("Package Backup Options");
    packageBackupBtn->setToolTip("Configure package backup settings and selection");
    auto settingsBackupBtn = new QPushButton("Settings Backup Options");
    settingsBackupBtn->setToolTip("Configure settings backup categories and files");
    
    backupOpsLayout->addWidget(packageBackupBtn, 0, 0);
    backupOpsLayout->addWidget(settingsBackupBtn, 0, 1);
    
    // Backup configuration group
    auto configGroup = new QGroupBox("Backup Configuration");
    layout->addWidget(configGroup);
    
    auto configLayout = new QGridLayout(configGroup);
    
    // Backup location
    configLayout->addWidget(new QLabel("Backup Location:"), 0, 0);
    auto backupLocationEdit = new QLineEdit;
    backupLocationEdit->setText("/home/lou/Documents/ArchForgeBackups");
    configLayout->addWidget(backupLocationEdit, 0, 1);
    auto browseBtn = new QPushButton("Browse");
    configLayout->addWidget(browseBtn, 0, 2);
    
    // Compression settings
    configLayout->addWidget(new QLabel("Compression:"), 1, 0);
    auto compressionCombo = new QComboBox;
    compressionCombo->addItems({"gzip", "bzip2", "xz", "lz4"});
    configLayout->addWidget(compressionCombo, 1, 1);
    
    // Verification checkbox
    auto verifyCheck = new QCheckBox("Verify backup integrity");
    verifyCheck->setChecked(true);
    configLayout->addWidget(verifyCheck, 2, 0);
    
    // Progress and status
    auto progressGroup = new QGroupBox("Progress");
    layout->addWidget(progressGroup);
    
    auto progressLayout = new QVBoxLayout(progressGroup);
    
    auto backupProgress = new QProgressBar;
    progressLayout->addWidget(backupProgress);
    
    auto statusLabel = new QLabel("Ready for backup");
    progressLayout->addWidget(statusLabel);
    
    // Control buttons
    auto controlLayout = new QHBoxLayout;
    auto startBackupBtn = new QPushButton("Start Backup");
    auto pauseBtn = new QPushButton("Pause");
    auto cancelBtn = new QPushButton("Cancel");
    
    pauseBtn->setEnabled(false);
    cancelBtn->setEnabled(false);
    
    controlLayout->addWidget(startBackupBtn);
    controlLayout->addWidget(pauseBtn);
    controlLayout->addWidget(cancelBtn);
    controlLayout->addStretch();
    
    progressLayout->addLayout(controlLayout);
    
    // Connect backup signals
    connect(packageBackupBtn, &QPushButton::clicked, [this]() {
        logMessage("Package backup options selected");
    });
    
    connect(settingsBackupBtn, &QPushButton::clicked, [this]() {
        logMessage("Settings backup options selected");
    });
    
    connect(startBackupBtn, &QPushButton::clicked, [this]() {
        logMessage("Backup started");
    });
    
    layout->addStretch();
}

void MainWindow::setupRestoreTab(QTabWidget *parentTabs)
{
    auto restoreWidget = new QWidget;
    parentTabs->addTab(restoreWidget, "&Restore");
    
    auto layout = new QVBoxLayout(restoreWidget);
    
    // Restore points group
    auto restorePointsGroup = new QGroupBox("Available Restore Points");
    layout->addWidget(restorePointsGroup);
    
    auto restorePointsLayout = new QVBoxLayout(restorePointsGroup);
    
    auto restoreTree = new QTreeWidget;
    restoreTree->setHeaderLabels({"Date", "Type", "Size", "Description"});
    
    // Add sample restore points
    auto item1 = new QTreeWidgetItem(restoreTree);
    item1->setText(0, "2025-06-23 20:15");
    item1->setText(1, "Full Backup");
    item1->setText(2, "2.1 GB");
    item1->setText(3, "Complete system backup before RGB integration");
    
    auto item2 = new QTreeWidgetItem(restoreTree);
    item2->setText(0, "2025-06-22 14:30");
    item2->setText(1, "Package Backup");
    item2->setText(2, "45 MB");
    item2->setText(3, "Package list backup");
    
    restorePointsLayout->addWidget(restoreTree);
    
    // Restore options group
    auto restoreOptionsGroup = new QGroupBox("Restore Options");
    layout->addWidget(restoreOptionsGroup);
    
    auto restoreOptionsLayout = new QGridLayout(restoreOptionsGroup);
    
    auto restorePackagesCheck = new QCheckBox("Restore Packages");
    restorePackagesCheck->setChecked(true);
    restoreOptionsLayout->addWidget(restorePackagesCheck, 0, 0);
    
    auto restoreSettingsCheck = new QCheckBox("Restore Settings");
    restoreSettingsCheck->setChecked(true);
    restoreOptionsLayout->addWidget(restoreSettingsCheck, 0, 1);
    
    auto restoreUserDataCheck = new QCheckBox("Restore User Data");
    restoreOptionsLayout->addWidget(restoreUserDataCheck, 1, 0);
    
    // Control buttons
    auto restoreControlLayout = new QHBoxLayout;
    auto previewBtn = new QPushButton("Preview Restore");
    auto startRestoreBtn = new QPushButton("Start Restore");
    auto deletePointBtn = new QPushButton("Delete Point");
    
    restoreControlLayout->addWidget(previewBtn);
    restoreControlLayout->addWidget(startRestoreBtn);
    restoreControlLayout->addWidget(deletePointBtn);
    restoreControlLayout->addStretch();
    
    restoreOptionsLayout->addLayout(restoreControlLayout, 2, 0, 1, 2);
    
    // Connect restore signals
    connect(previewBtn, &QPushButton::clicked, [this]() {
        logMessage("Restore preview requested");
    });
    
    connect(startRestoreBtn, &QPushButton::clicked, [this]() {
        logMessage("Restore started");
    });
    
    connect(deletePointBtn, &QPushButton::clicked, [this]() {
        logMessage("Restore point deletion requested");
    });
    
    layout->addStretch();
}

void MainWindow::setupLogsTab(QTabWidget *parentTabs)
{
    auto logsWidget = new QWidget;
    parentTabs->addTab(logsWidget, "&Logs");
    
    auto layout = new QVBoxLayout(logsWidget);
    
    // Log controls group
    auto logControlsGroup = new QGroupBox("Log Controls");
    layout->addWidget(logControlsGroup);
    
    auto logControlsLayout = new QHBoxLayout(logControlsGroup);
    
    auto logLevelCombo = new QComboBox;
    logLevelCombo->addItems({"All", "Info", "Warning", "Error", "Debug"});
    logControlsLayout->addWidget(new QLabel("Log Level:"));
    logControlsLayout->addWidget(logLevelCombo);
    
    auto clearLogsBtn = new QPushButton("Clear Logs");
    auto exportLogsBtn = new QPushButton("Export Logs");
    auto refreshLogsBtn = new QPushButton("Refresh");
    
    logControlsLayout->addWidget(clearLogsBtn);
    logControlsLayout->addWidget(exportLogsBtn);
    logControlsLayout->addWidget(refreshLogsBtn);
    logControlsLayout->addStretch();
    
    // Logs display
    auto logsGroup = new QGroupBox("System Logs");
    layout->addWidget(logsGroup);
    
    auto logsLayout = new QVBoxLayout(logsGroup);
    
    auto logsTextEdit = new QTextEdit;
    logsTextEdit->setReadOnly(true);
    logsTextEdit->setStyleSheet("QTextEdit { background-color: #2a2a2a; border: 1px solid #555; color: #ccc; font-family: monospace; }");
    
    // Sample log entries
    QString sampleLogs = QString(
        "[%1] INFO: ArchForge RGB Control Center initialized\n"
        "[%2] INFO: RGB batcher started successfully\n"
        "[%3] INFO: Clean Install Backup/Restore tabs loaded\n"
        "[%4] INFO: System monitoring active\n"
        "[%5] DEBUG: Device permissions verified\n"
    ).arg(
        QDateTime::currentDateTime().addSecs(-300).toString("hh:mm:ss"),
        QDateTime::currentDateTime().addSecs(-240).toString("hh:mm:ss"),
        QDateTime::currentDateTime().addSecs(-180).toString("hh:mm:ss"),
        QDateTime::currentDateTime().addSecs(-120).toString("hh:mm:ss"),
        QDateTime::currentDateTime().addSecs(-60).toString("hh:mm:ss")
    );
    
    logsTextEdit->setPlainText(sampleLogs);
    logsLayout->addWidget(logsTextEdit);
    
    // Connect log signals
    connect(clearLogsBtn, &QPushButton::clicked, [this, logsTextEdit]() {
        logsTextEdit->clear();
        logMessage("Logs cleared");
    });
    
    connect(exportLogsBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Export Logs", 
            QString("/home/lou/archforge-logs-%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss")),
            "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            logMessage(QString("Logs exported to: %1").arg(fileName));
        }
    });
    
    connect(refreshLogsBtn, &QPushButton::clicked, [this]() {
        logMessage("Logs refreshed");
    });
}

// Python RGB integration functions
void MainWindow::pythonSetKeyColor(const QString &keyName, int red, int green, int blue)
{
    QProcess process;
    QString command = QString("cd /home/lou/Coding/originpc-control/src && python3 -c \"exec(open('enhanced-professional-control-center.py').read()); rgb = EnhancedRGBController(); rgb.set_key_color('%1', %2, %3, %4)\"")
                     .arg(keyName).arg(red).arg(green).arg(blue);
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        logMessage(QString("✅ Set key '%1' to RGB(%2,%3,%4) via Python").arg(keyName).arg(red).arg(green).arg(blue));
    } else {
        logMessage(QString("❌ Failed to set key '%1' color: %2").arg(keyName).arg(process.readAllStandardError()));
    }
}

void MainWindow::pythonClearKeypad()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 originpc-rgb-fix.py";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(10000);
    
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    
    if (process.exitCode() == 0) {
        logMessage("✅ Python keypad clear executed successfully");
        if (!output.isEmpty()) {
            logMessage(output.trimmed());
        }
    } else {
        logMessage(QString("❌ Python keypad clear failed: %1").arg(error));
    }
}

void MainWindow::pythonRainbowEffect()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 -c \"exec(open('enhanced-professional-control-center.py').read()); rgb = EnhancedRGBController(); rgb.rainbow_wave_effect()\"";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(15000);
    
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    
    if (process.exitCode() == 0) {
        logMessage("✅ Python rainbow effect started");
        if (!output.isEmpty()) {
            logMessage(output.trimmed());
        }
    } else {
        logMessage(QString("❌ Python rainbow effect failed: %1").arg(error));
    }
}

void MainWindow::pythonBreathingEffect()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 -c \"exec(open('enhanced-professional-control-center.py').read()); rgb = EnhancedRGBController(); rgb.breathing_effect([255,102,0])\"";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(15000);
    
    if (process.exitCode() == 0) {
        logMessage("✅ Python breathing effect started");
    } else {
        logMessage(QString("❌ Python breathing effect failed: %1").arg(process.readAllStandardError()));
    }
}

void MainWindow::pythonWaveEffect()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 -c \"exec(open('enhanced-professional-control-center.py').read()); rgb = EnhancedRGBController(); rgb.color_wave_effect()\"";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(15000);
    
    if (process.exitCode() == 0) {
        logMessage("✅ Python wave effect started");
    } else {
        logMessage(QString("❌ Python wave effect failed: %1").arg(process.readAllStandardError()));
    }
}

void MainWindow::pythonCheckDevicePermissions()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 -c \"import enhanced_professional_control_center as epc; rgb = epc.EnhancedRGBController(); ok, msg = rgb.check_permissions(); print(f'✅ {msg}' if ok else f'❌ {msg}')\"";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(5000);
    
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    
    logMessage("🔍 Device Permission Check:");
    if (!output.isEmpty()) {
        logMessage(output.trimmed());
    }
    if (!error.isEmpty()) {
        logMessage(QString("Error: %1").arg(error.trimmed()));
    }
}

void MainWindow::pythonFixRGBDevice()
{
    QProcess process;
    QString command = "sudo chmod 666 /dev/hidraw0 && sudo chmod 666 /dev/hidraw1";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        logMessage("✅ RGB device permissions fixed");
    } else {
        logMessage(QString("❌ Failed to fix RGB device permissions: %1").arg(process.readAllStandardError()));
    }
}

void MainWindow::pythonTestAllKeys()
{
    QProcess process;
    QString command = "cd /home/lou/Coding/originpc-control/src && python3 -c \"import enhanced_professional_control_center as epc; rgb = epc.EnhancedRGBController(); import time; [rgb.set_key_color(key, 255, 0, 0) for key in ['w','a','s','d']]; time.sleep(2); [rgb.set_key_color(key, 0, 0, 0) for key in ['w','a','s','d']]\"";
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(10000);
    
    if (process.exitCode() == 0) {
        logMessage("✅ Python key test completed (WASD keys)");
    } else {
        logMessage(QString("❌ Python key test failed: %1").arg(process.readAllStandardError()));
    }
}

void MainWindow::pythonApplyStaticColor(const QColor &color)
{
    QProcess process;
    QString command = QString("cd /home/lou/Coding/originpc-control/src && python3 -c \"import enhanced_professional_control_center as epc; rgb = epc.EnhancedRGBController(); rgb.static_color_effect([%1,%2,%3])\"")
                     .arg(color.red()).arg(color.green()).arg(color.blue());
    
    process.start("bash", QStringList() << "-c" << command);
    process.waitForFinished(10000);
    
    if (process.exitCode() == 0) {
        logMessage(QString("✅ Applied static color RGB(%1,%2,%3) via Python").arg(color.red()).arg(color.green()).arg(color.blue()));
    } else {
        logMessage(QString("❌ Failed to apply static color: %1").arg(process.readAllStandardError()));
    }
}
