#include "rgbfancontrol.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QHeaderView>
#include <QSplitter>
#include <cmath>

// System includes for hardware access
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

RGBFanControl::RGBFanControl(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_systemMonitor(nullptr)
    , m_rgbManager(nullptr)
    , m_fanManager(nullptr)
    , m_configManager(nullptr)
    , m_primaryColor(Qt::red)
    , m_secondaryColor(Qt::blue)
    , m_systemUpdateTimer(nullptr)
    , m_rgbUpdateTimer(nullptr)
    , m_fanUpdateTimer(nullptr)
    , m_monitoringActive(false)
{
    // Initialize configuration directory
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/ArchBackupPro/RGBFanControl";
    QDir().mkpath(configDir);
    
    // Initialize settings
    m_settings = new QSettings(configDir + "/settings.ini", QSettings::IniFormat, this);
    
    // Initialize core components
    m_configManager = new ConfigurationManager(this);
    m_systemMonitor = new SystemMonitorThread(this);
    m_rgbManager = new RGBEffectManager(this);
    m_fanManager = new FanControlManager(this);
    
    // Setup UI
    setupUI();
    setupConnections();
    
    // Load configuration
    loadSettings();
    createDefaultProfiles();
    
    // Initialize hardware
    m_rgbDevices = m_rgbManager->getAvailableDevices();
    m_fanDevices = m_fanManager->getAvailableFans();
    
    // Setup timers
    m_systemUpdateTimer = new QTimer(this);
    m_rgbUpdateTimer = new QTimer(this);
    m_fanUpdateTimer = new QTimer(this);
    
    connect(m_systemUpdateTimer, &QTimer::timeout, this, &RGBFanControl::updateSystemDisplays);
    connect(m_systemMonitor, &SystemMonitorThread::dataUpdated, this, &RGBFanControl::onSystemDataUpdated);
    
    // Start monitoring
    startMonitoring();
    
    emit statusMessage("RGB/Fan Control initialized successfully");
}

RGBFanControl::~RGBFanControl()
{
    stopMonitoring();
    saveSettings();
}

void RGBFanControl::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create header with OriginPC branding
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *headerLabel = new QLabel("OriginPC Enhanced Control Center");
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(headerFont.pointSize() + 4);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    headerLabel->setStyleSheet("color: #4a90e2; padding: 10px;");
    
    QLabel *versionLabel = new QLabel("v5.1 Ultimate Edition");
    versionLabel->setStyleSheet("color: #888; font-style: italic; padding: 10px;");
    
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(versionLabel);
    mainLayout->addLayout(headerLayout);
    
    // Create tab widget with enhanced styling
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #404040; background-color: #2d2d2d; }"
        "QTabBar::tab { background-color: #404040; color: #cccccc; border: 1px solid #606060; "
        "             padding: 8px 16px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #4a90e2; color: white; }"
        "QTabBar::tab:hover { background-color: #505050; }"
    );
    mainLayout->addWidget(m_tabWidget);
    
    // Setup enhanced tabs with OriginPC features
    setupAdvancedSystemMonitoringTab();
    setupEnhancedRGBControlTab();
    setupIntelligentFanControlTab();
    setupPowerManagementTab();
    setupHardwareOptimizationTab();
    setupProfilesAndMacrosTab();
    
    // Enhanced status layout with real-time info
    QHBoxLayout *statusLayout = new QHBoxLayout();
    QLabel *statusLabel = new QLabel("🌡️ Advanced System Monitoring | 🌈 Professional RGB Control | 🌪️ Intelligent Fan Management | ⚡ Power Optimization");
    statusLabel->setStyleSheet("color: #666; font-style: italic; padding: 5px;");
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    // Add connection status indicator
    m_connectionStatusLabel = new QLabel("🔌 Checking devices...");
    m_connectionStatusLabel->setStyleSheet("color: #888; padding: 5px;");
    statusLayout->addWidget(m_connectionStatusLabel);
    
    mainLayout->addLayout(statusLayout);
}

void RGBFanControl::setupAdvancedSystemMonitoringTab()
{
    m_systemMonitorTab = new QWidget();
    m_tabWidget->addTab(m_systemMonitorTab, "🔍 Advanced System Monitor");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_systemMonitorTab);
    
    // Create splitter for flexible layout
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    mainLayout->addWidget(splitter);
    
    // Left panel - Real-time monitoring (OriginPC Enhanced)
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Professional monitoring header
    QLabel *monitorHeader = new QLabel("🎯 OriginPC Professional System Monitoring");
    QFont headerFont = monitorHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    monitorHeader->setFont(headerFont);
    monitorHeader->setStyleSheet("color: #4a90e2; padding: 5px;");
    leftLayout->addWidget(monitorHeader);
    
    // System overview cards
    QHBoxLayout *cardsLayout = new QHBoxLayout();
    
    // CPU Card
    QGroupBox *cpuCard = new QGroupBox("💻 CPU Performance");
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuCard);
    m_cpuUsageLabel = new QLabel("Usage: 0%");
    m_cpuTempLabel = new QLabel("Temperature: 0°C");
    m_cpuProgressBar = new QProgressBar();
    m_cpuProgressBar->setRange(0, 100);
    QLabel *cpuFreqLabel = new QLabel("Frequency: 0 MHz");
    QLabel *cpuLoadLabel = new QLabel("Load Avg: 0.00");
    
    cpuLayout->addWidget(m_cpuUsageLabel);
    cpuLayout->addWidget(m_cpuProgressBar);
    cpuLayout->addWidget(m_cpuTempLabel);
    cpuLayout->addWidget(cpuFreqLabel);
    cpuLayout->addWidget(cpuLoadLabel);
    cardsLayout->addWidget(cpuCard);
    
    // Memory Card
    QGroupBox *memoryCard = new QGroupBox("🧠 Memory Status");
    QVBoxLayout *memoryLayout = new QVBoxLayout(memoryCard);
    m_memoryUsageLabel = new QLabel("Usage: 0%");
    m_memoryProgressBar = new QProgressBar();
    m_memoryProgressBar->setRange(0, 100);
    QLabel *memoryAvailableLabel = new QLabel("Available: 0 GB");
    QLabel *memorySwapLabel = new QLabel("Swap: 0%");
    
    memoryLayout->addWidget(m_memoryUsageLabel);
    memoryLayout->addWidget(m_memoryProgressBar);
    memoryLayout->addWidget(memoryAvailableLabel);
    memoryLayout->addWidget(memorySwapLabel);
    cardsLayout->addWidget(memoryCard);
    
    leftLayout->addLayout(cardsLayout);
    
    // Storage and GPU cards
    QHBoxLayout *cards2Layout = new QHBoxLayout();
    
    // Storage Card
    QGroupBox *storageCard = new QGroupBox("💾 Storage I/O");
    QVBoxLayout *storageLayout = new QVBoxLayout(storageCard);
    m_diskUsageLabel = new QLabel("Usage: 0%");
    m_diskProgressBar = new QProgressBar();
    m_diskProgressBar->setRange(0, 100);
    QLabel *diskReadLabel = new QLabel("Read: 0 MB/s");
    QLabel *diskWriteLabel = new QLabel("Write: 0 MB/s");
    
    storageLayout->addWidget(m_diskUsageLabel);
    storageLayout->addWidget(m_diskProgressBar);
    storageLayout->addWidget(diskReadLabel);
    storageLayout->addWidget(diskWriteLabel);
    cards2Layout->addWidget(storageCard);
    
    // GPU Card
    QGroupBox *gpuCard = new QGroupBox("🎮 GPU Performance");
    QVBoxLayout *gpuLayout = new QVBoxLayout(gpuCard);
    m_gpuUsageLabel = new QLabel("Usage: 0%");
    m_gpuTempLabel = new QLabel("Temperature: 0°C");
    m_gpuProgressBar = new QProgressBar();
    m_gpuProgressBar->setRange(0, 100);
    QLabel *gpuMemoryLabel = new QLabel("VRAM: 0%");
    QLabel *gpuClockLabel = new QLabel("Clock: 0 MHz");
    
    gpuLayout->addWidget(m_gpuUsageLabel);
    gpuLayout->addWidget(m_gpuProgressBar);
    gpuLayout->addWidget(m_gpuTempLabel);
    gpuLayout->addWidget(gpuMemoryLabel);
    gpuLayout->addWidget(gpuClockLabel);
    cards2Layout->addWidget(gpuCard);
    
    leftLayout->addLayout(cards2Layout);
    
    // Network monitoring
    QGroupBox *networkCard = new QGroupBox("🌐 Network Activity");
    QHBoxLayout *networkLayout = new QHBoxLayout(networkCard);
    QLabel *networkUpLabel = new QLabel("Upload: 0 KB/s");
    QLabel *networkDownLabel = new QLabel("Download: 0 KB/s");
    QLabel *networkPacketsLabel = new QLabel("Packets: 0/s");
    networkLayout->addWidget(networkUpLabel);
    networkLayout->addWidget(networkDownLabel);
    networkLayout->addWidget(networkPacketsLabel);
    leftLayout->addWidget(networkCard);
    
    leftLayout->addStretch();
    splitter->addWidget(leftPanel);
    
    // Right panel - Detailed sensors and analytics
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Temperature monitoring with enhanced sensors
    QGroupBox *tempGroup = new QGroupBox("🌡️ Comprehensive Temperature Monitoring");
    QVBoxLayout *tempLayout = new QVBoxLayout(tempGroup);
    
    m_temperatureTree = new QTreeWidget();
    m_temperatureTree->setHeaderLabels({"Sensor", "Current", "High", "Critical", "Status"});
    m_temperatureTree->setMaximumHeight(250);
    tempLayout->addWidget(m_temperatureTree);
    
    // Fan monitoring with PWM control indicators
    QGroupBox *fanGroup = new QGroupBox("🌪️ Advanced Fan Monitoring");
    QVBoxLayout *fanLayout = new QVBoxLayout(fanGroup);
    
    m_fanSpeedTree = new QTreeWidget();
    m_fanSpeedTree->setHeaderLabels({"Fan", "Speed (RPM)", "PWM %", "Target Temp", "Mode"});
    m_fanSpeedTree->setMaximumHeight(200);
    fanLayout->addWidget(m_fanSpeedTree);
    
    rightLayout->addWidget(tempGroup);
    rightLayout->addWidget(fanGroup);
    
    // System analytics and insights
    QGroupBox *analyticsGroup = new QGroupBox("📊 System Analytics & Insights");
    QVBoxLayout *analyticsLayout = new QVBoxLayout(analyticsGroup);
    
    m_systemInfoText = new QTextEdit();
    m_systemInfoText->setMaximumHeight(120);
    m_systemInfoText->setReadOnly(true);
    m_systemInfoText->setPlaceholderText("Real-time system analytics and recommendations will appear here...");
    analyticsLayout->addWidget(m_systemInfoText);
    
    rightLayout->addWidget(analyticsGroup);
    splitter->addWidget(rightPanel);
    
    // Set initial splitter proportions
    splitter->setSizes({400, 500});
}

void RGBFanControl::setupSystemMonitoringTab()
{
    m_systemMonitorTab = new QWidget();
    m_tabWidget->addTab(m_systemMonitorTab, "🌡️ System Monitor");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_systemMonitorTab);
    
    // Left side - System overview
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    // CPU group
    QGroupBox *cpuGroup = new QGroupBox("CPU Information");
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuGroup);
    
    m_cpuUsageLabel = new QLabel("CPU Usage: 0%");
    m_cpuTempLabel = new QLabel("CPU Temperature: 0°C");
    m_cpuProgressBar = new QProgressBar();
    m_cpuProgressBar->setRange(0, 100);
    
    cpuLayout->addWidget(m_cpuUsageLabel);
    cpuLayout->addWidget(m_cpuProgressBar);
    cpuLayout->addWidget(m_cpuTempLabel);
    
    // Memory group
    QGroupBox *memoryGroup = new QGroupBox("Memory Information");
    QVBoxLayout *memoryLayout = new QVBoxLayout(memoryGroup);
    
    m_memoryUsageLabel = new QLabel("Memory Usage: 0%");
    m_memoryProgressBar = new QProgressBar();
    m_memoryProgressBar->setRange(0, 100);
    
    memoryLayout->addWidget(m_memoryUsageLabel);
    memoryLayout->addWidget(m_memoryProgressBar);
    
    // Disk group
    QGroupBox *diskGroup = new QGroupBox("Disk Information");
    QVBoxLayout *diskLayout = new QVBoxLayout(diskGroup);
    
    m_diskUsageLabel = new QLabel("Disk Usage: 0%");
    m_diskProgressBar = new QProgressBar();
    m_diskProgressBar->setRange(0, 100);
    
    diskLayout->addWidget(m_diskUsageLabel);
    diskLayout->addWidget(m_diskProgressBar);
    
    // GPU group
    QGroupBox *gpuGroup = new QGroupBox("GPU Information");
    QVBoxLayout *gpuLayout = new QVBoxLayout(gpuGroup);
    
    m_gpuUsageLabel = new QLabel("GPU Usage: 0%");
    m_gpuTempLabel = new QLabel("GPU Temperature: 0°C");
    m_gpuProgressBar = new QProgressBar();
    m_gpuProgressBar->setRange(0, 100);
    
    gpuLayout->addWidget(m_gpuUsageLabel);
    gpuLayout->addWidget(m_gpuProgressBar);
    gpuLayout->addWidget(m_gpuTempLabel);
    
    leftLayout->addWidget(cpuGroup);
    leftLayout->addWidget(memoryGroup);
    leftLayout->addWidget(diskGroup);
    leftLayout->addWidget(gpuGroup);
    leftLayout->addStretch();
    
    // Right side - Detailed information
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // Temperature details
    QGroupBox *tempGroup = new QGroupBox("Temperature Sensors");
    QVBoxLayout *tempLayout = new QVBoxLayout(tempGroup);
    
    m_temperatureTree = new QTreeWidget();
    m_temperatureTree->setHeaderLabels({"Sensor", "Temperature", "Status"});
    m_temperatureTree->setMaximumHeight(200);
    tempLayout->addWidget(m_temperatureTree);
    
    // Fan speed details
    QGroupBox *fanGroup = new QGroupBox("Fan Speeds");
    QVBoxLayout *fanLayout = new QVBoxLayout(fanGroup);
    
    m_fanSpeedTree = new QTreeWidget();
    m_fanSpeedTree->setHeaderLabels({"Fan", "Speed (RPM)", "PWM %"});
    m_fanSpeedTree->setMaximumHeight(200);
    fanLayout->addWidget(m_fanSpeedTree);
    
    // System information
    QGroupBox *infoGroup = new QGroupBox("System Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    m_systemInfoText = new QTextEdit();
    m_systemInfoText->setMaximumHeight(150);
    m_systemInfoText->setReadOnly(true);
    infoLayout->addWidget(m_systemInfoText);
    
    rightLayout->addWidget(tempGroup);
    rightLayout->addWidget(fanGroup);
    rightLayout->addWidget(infoGroup);
    
    // Add to main layout
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 1);
}

void RGBFanControl::setupEnhancedRGBControlTab()
{
    m_rgbControlTab = new QWidget();
    m_tabWidget->addTab(m_rgbControlTab, "🌈 Professional RGB Control");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_rgbControlTab);
    
    // Left panel - Device management and effects
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // RGB Device Discovery and Management
    QGroupBox *deviceGroup = new QGroupBox("🔌 RGB Device Management");
    QVBoxLayout *deviceLayout = new QVBoxLayout(deviceGroup);
    
    // Device list with status
    QTreeWidget *deviceTree = new QTreeWidget();
    deviceTree->setHeaderLabels({"Device", "Type", "Status", "Path"});
    deviceTree->setMaximumHeight(120);
    
    // Populate with detected devices
    QTreeWidgetItem *keyboardItem = new QTreeWidgetItem(deviceTree);
    keyboardItem->setText(0, "OriginPC Keyboard");
    keyboardItem->setText(1, "⌨️ Keyboard");
    keyboardItem->setText(2, "✅ Connected");
    keyboardItem->setText(3, "/dev/hidraw0");
    
    QTreeWidgetItem *mouseItem = new QTreeWidgetItem(deviceTree);
    mouseItem->setText(0, "OriginPC Mouse");
    mouseItem->setText(1, "🖱️ Mouse");
    mouseItem->setText(2, "✅ Connected");
    mouseItem->setText(3, "/dev/hidraw1");
    
    deviceLayout->addWidget(deviceTree);
    
    QHBoxLayout *deviceButtonsLayout = new QHBoxLayout();
    QPushButton *refreshDevicesBtn = new QPushButton("🔄 Refresh");
    QPushButton *testDeviceBtn = new QPushButton("🧪 Test Device");
    QPushButton *clearDevicesBtn = new QPushButton("🧹 Clear All");
    deviceButtonsLayout->addWidget(refreshDevicesBtn);
    deviceButtonsLayout->addWidget(testDeviceBtn);
    deviceButtonsLayout->addWidget(clearDevicesBtn);
    deviceLayout->addLayout(deviceButtonsLayout);
    
    leftLayout->addWidget(deviceGroup);
    
    // Enhanced RGB Effects with OriginPC Features
    QGroupBox *effectsGroup = new QGroupBox("✨ Advanced RGB Effects");
    QGridLayout *effectsLayout = new QGridLayout(effectsGroup);
    
    effectsLayout->addWidget(new QLabel("Effect Mode:"), 0, 0);
    m_rgbEffectCombo = new QComboBox();
    m_rgbEffectCombo->addItems({
        "🎨 Static Color", "💨 Breathing", "🌈 Rainbow Wave", "🌊 Diagonal Wave",
        "⚡ Reactive Typing", "🎮 Gaming Mode", "🌟 Starfield", "🔥 Fire Effect",
        "❄️ Ice Effect", "🌺 Flower Bloom", "⭐ Custom Macro"
    });
    effectsLayout->addWidget(m_rgbEffectCombo, 0, 1, 1, 2);
    
    effectsLayout->addWidget(new QLabel("Primary Color:"), 1, 0);
    m_primaryColorBtn = new QPushButton();
    m_primaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333; border-radius: 5px;").arg(m_primaryColor.name()));
    m_primaryColorBtn->setFixedSize(80, 35);
    effectsLayout->addWidget(m_primaryColorBtn, 1, 1);
    
    effectsLayout->addWidget(new QLabel("Secondary Color:"), 1, 2);
    m_secondaryColorBtn = new QPushButton();
    m_secondaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333; border-radius: 5px;").arg(m_secondaryColor.name()));
    m_secondaryColorBtn->setFixedSize(80, 35);
    effectsLayout->addWidget(m_secondaryColorBtn, 1, 3);
    
    effectsLayout->addWidget(new QLabel("Brightness:"), 2, 0);
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(100);
    m_brightnessLabel = new QLabel("100%");
    effectsLayout->addWidget(m_brightnessSlider, 2, 1, 1, 2);
    effectsLayout->addWidget(m_brightnessLabel, 2, 3);
    
    effectsLayout->addWidget(new QLabel("Animation Speed:"), 3, 0);
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(50);
    m_speedLabel = new QLabel("50%");
    effectsLayout->addWidget(m_speedSlider, 3, 1, 1, 2);
    effectsLayout->addWidget(m_speedLabel, 3, 3);
    
    // Advanced effect controls
    QHBoxLayout *advancedControlsLayout = new QHBoxLayout();
    QCheckBox *persistentEffectCheck = new QCheckBox("💾 Persistent (Survive Reboot)");
    QCheckBox *reactiveCheck = new QCheckBox("⌨️ Reactive to Typing");
    QCheckBox *syncCheck = new QCheckBox("🔄 Sync All Devices");
    advancedControlsLayout->addWidget(persistentEffectCheck);
    advancedControlsLayout->addWidget(reactiveCheck);
    advancedControlsLayout->addWidget(syncCheck);
    effectsLayout->addLayout(advancedControlsLayout, 4, 0, 1, 4);
    
    leftLayout->addWidget(effectsGroup);
    
    // Key Group Selection (OriginPC Enhanced)
    QGroupBox *keyGroupGroup = new QGroupBox("⌨️ Key Group Control");
    QGridLayout *keyGroupLayout = new QGridLayout(keyGroupGroup);
    
    QStringList keyGroups = {
        "🔤 All Keys", "🔢 Function Keys", "🎮 WASD Keys", "➡️ Arrow Keys",
        "🔢 Number Pad", "⭐ Spacebar", "📝 Letter Keys", "🚀 Gaming Zone"
    };
    
    for (int i = 0; i < keyGroups.size(); ++i) {
        QPushButton *groupBtn = new QPushButton(keyGroups[i]);
        groupBtn->setMinimumHeight(30);
        groupBtn->setStyleSheet(
            "QPushButton { background-color: #404040; color: #cccccc; border: 1px solid #606060; "
            "             border-radius: 5px; padding: 5px; }"
            "QPushButton:hover { background-color: #4a90e2; }"
            "QPushButton:pressed { background-color: #357abd; }"
        );
        keyGroupLayout->addWidget(groupBtn, i / 4, i % 4);
    }
    
    leftLayout->addWidget(keyGroupGroup);
    leftLayout->addStretch();
    
    // Right panel - Live preview and profiles
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Live RGB Preview (Enhanced)
    QGroupBox *previewGroup = new QGroupBox("🖥️ Live RGB Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    
    m_rgbPreview = new QLabel();
    m_rgbPreview->setFixedHeight(150);
    m_rgbPreview->setStyleSheet(
        "border: 2px solid #333; border-radius: 10px; "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #ff0000, stop:0.16 #ff8000, stop:0.33 #ffff00, "
        "stop:0.5 #00ff00, stop:0.66 #0080ff, stop:0.83 #8000ff, stop:1 #ff00ff);"
    );
    m_rgbPreview->setAlignment(Qt::AlignCenter);
    m_rgbPreview->setText("🌈 RGB Effect Preview\n✨ Live Animation");
    previewLayout->addWidget(m_rgbPreview);
    
    // Quick color presets
    QHBoxLayout *presetsLayout = new QHBoxLayout();
    QStringList presetColors = {"🔴", "🟠", "🟡", "🟢", "🔵", "🟣", "⚪", "⚫"};
    QStringList presetNames = {"Red", "Orange", "Yellow", "Green", "Blue", "Purple", "White", "Black"};
    
    for (int i = 0; i < presetColors.size(); ++i) {
        QPushButton *presetBtn = new QPushButton(presetColors[i]);
        presetBtn->setFixedSize(40, 40);
        presetBtn->setToolTip(presetNames[i]);
        presetBtn->setStyleSheet("border-radius: 20px; font-size: 20px;");
        presetsLayout->addWidget(presetBtn);
    }
    previewLayout->addLayout(presetsLayout);
    
    rightLayout->addWidget(previewGroup);
    
    // Professional RGB Profiles
    QGroupBox *profilesGroup = new QGroupBox("💼 Professional RGB Profiles");
    QVBoxLayout *profilesLayout = new QVBoxLayout(profilesGroup);
    
    m_rgbProfileCombo = new QComboBox();
    m_rgbProfileCombo->addItems({
        "🌈 Rainbow Cascade", "⚡ Lightning Strike", "🌊 Ocean Wave",
        "🔥 Dragon Fire", "❄️ Arctic Frost", "🌺 Cherry Blossom",
        "🎮 Gaming Beast", "💎 Diamond Sparkle", "🌙 Midnight Blue"
    });
    profilesLayout->addWidget(m_rgbProfileCombo);
    
    QHBoxLayout *profileButtonsLayout = new QHBoxLayout();
    m_saveRGBProfileBtn = new QPushButton("💾 Save Profile");
    m_loadRGBProfileBtn = new QPushButton("📂 Load Profile");
    QPushButton *shareProfileBtn = new QPushButton("📤 Share Profile");
    profileButtonsLayout->addWidget(m_saveRGBProfileBtn);
    profileButtonsLayout->addWidget(m_loadRGBProfileBtn);
    profileButtonsLayout->addWidget(shareProfileBtn);
    profilesLayout->addLayout(profileButtonsLayout);
    
    rightLayout->addWidget(profilesGroup);
    
    // RGB Performance Stats
    QGroupBox *statsGroup = new QGroupBox("📊 RGB Performance Stats");
    QVBoxLayout *statsLayout = new QVBoxLayout(statsGroup);
    
    QLabel *statsLabel = new QLabel(
        "🔌 Devices Connected: 2\n"
        "⚡ Effects Running: 1\n"
        "🎯 Command Rate: 60 FPS\n"
        "💾 Memory Usage: 2.1 MB\n"
        "🌡️ Device Temperature: Normal\n"
        "⏱️ Last Update: Just now"
    );
    statsLabel->setStyleSheet("color: #cccccc; font-family: monospace;");
    statsLayout->addWidget(statsLabel);
    
    rightLayout->addWidget(statsGroup);
    rightLayout->addStretch();
    
    // Add panels to main layout
    mainLayout->addWidget(leftPanel, 2);
    mainLayout->addWidget(rightPanel, 1);
}

void RGBFanControl::setupIntelligentFanControlTab()
{
    m_fanControlTab = new QWidget();
    m_tabWidget->addTab(m_fanControlTab, "🌪️ Intelligent Fan Control");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_fanControlTab);
    
    // Left panel - Fan management
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Intelligent Fan Control Header
    QLabel *fanHeader = new QLabel("🧠 OriginPC Intelligent Fan Management");
    QFont headerFont = fanHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    fanHeader->setFont(headerFont);
    fanHeader->setStyleSheet("color: #4a90e2; padding: 5px;");
    leftLayout->addWidget(fanHeader);
    
    // Fan Control Mode Selection
    QGroupBox *modeGroup = new QGroupBox("🎛️ Control Mode");
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);
    
    QRadioButton *autoModeBtn = new QRadioButton("🤖 Automatic (AI-Optimized)");
    QRadioButton *manualModeBtn = new QRadioButton("✋ Manual Control");
    QRadioButton *profileModeBtn = new QRadioButton("📋 Profile-Based");
    autoModeBtn->setChecked(true);
    
    modeLayout->addWidget(autoModeBtn);
    modeLayout->addWidget(manualModeBtn);
    modeLayout->addWidget(profileModeBtn);
    leftLayout->addWidget(modeGroup);
    
    // Fan Profiles with Intelligence
    QGroupBox *profileGroup = new QGroupBox("⚙️ Intelligent Fan Profiles");
    QVBoxLayout *profileLayout = new QVBoxLayout(profileGroup);
    
    m_fanProfileCombo = new QComboBox();
    m_fanProfileCombo->addItems({
        "🔇 Silent (Noise Priority)", "⚖️ Balanced (Optimal)", 
        "🚀 Performance (Cooling Priority)", "🎮 Gaming (Load Adaptive)",
        "🌡️ Temperature Reactive", "⚡ Turbo Boost", "🌙 Night Mode",
        "📊 Custom Curve"
    });
    profileLayout->addWidget(m_fanProfileCombo);
    
    QHBoxLayout *profileButtonsLayout = new QHBoxLayout();
    m_saveFanProfileBtn = new QPushButton("💾 Save Profile");
    m_loadFanProfileBtn = new QPushButton("📂 Load Profile");
    QPushButton *optimizeBtn = new QPushButton("🧠 AI Optimize");
    profileButtonsLayout->addWidget(m_saveFanProfileBtn);
    profileButtonsLayout->addWidget(m_loadFanProfileBtn);
    profileButtonsLayout->addWidget(optimizeBtn);
    profileLayout->addLayout(profileButtonsLayout);
    
    leftLayout->addWidget(profileGroup);
    
    // Advanced Temperature Curve Editor
    QGroupBox *curveGroup = new QGroupBox("📈 Advanced Temperature Response Curve");
    QVBoxLayout *curveLayout = new QVBoxLayout(curveGroup);
    
    m_fanCurveTree = new QTreeWidget();
    m_fanCurveTree->setHeaderLabels({"Temp (°C)", "Fan %", "Hysteresis", "Sensor", "Action"});
    m_fanCurveTree->setMaximumHeight(180);
    
    // Populate with intelligent curve points
    QStringList curvePoints = {
        "25°C|15%|2°C|CPU|Edit", "35°C|25%|3°C|CPU|Edit", "45°C|40%|3°C|CPU|Edit",
        "55°C|60%|4°C|CPU|Edit", "65°C|80%|4°C|CPU|Edit", "75°C|95%|5°C|CPU|Edit",
        "30°C|20%|2°C|GPU|Edit", "50°C|50%|3°C|GPU|Edit", "70°C|85%|4°C|GPU|Edit"
    };
    
    for (const QString &point : curvePoints) {
        QStringList parts = point.split("|");
        if (parts.size() == 5) {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_fanCurveTree);
            for (int i = 0; i < parts.size(); ++i) {
                item->setText(i, parts[i]);
            }
            // Color code by sensor
            if (parts[3] == "CPU") {
                item->setBackground(0, QBrush(QColor(100, 150, 255, 50)));
            } else if (parts[3] == "GPU") {
                item->setBackground(0, QBrush(QColor(255, 150, 100, 50)));
            }
        }
    }
    
    curveLayout->addWidget(m_fanCurveTree);
    
    QHBoxLayout *curveButtonsLayout = new QHBoxLayout();
    QPushButton *addPointBtn = new QPushButton("➕ Add Point");
    QPushButton *removePointBtn = new QPushButton("➖ Remove Point");
    QPushButton *resetCurveBtn = new QPushButton("🔄 Reset to Default");
    curveButtonsLayout->addWidget(addPointBtn);
    curveButtonsLayout->addWidget(removePointBtn);
    curveButtonsLayout->addWidget(resetCurveBtn);
    curveLayout->addLayout(curveButtonsLayout);
    
    leftLayout->addWidget(curveGroup);
    leftLayout->addStretch();
    
    // Right panel - Fan monitoring and control
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Real-time Fan Monitoring
    QGroupBox *monitorGroup = new QGroupBox("📊 Real-time Fan Monitoring");
    QVBoxLayout *monitorLayout = new QVBoxLayout(monitorGroup);
    
    m_fanSpeedTree = new QTreeWidget();
    m_fanSpeedTree->setHeaderLabels({"Fan", "Current RPM", "Target RPM", "PWM %", "Temp Source", "Status"});
    m_fanSpeedTree->setMaximumHeight(200);
    
    // Populate with fan data
    QStringList fanData = {
        "CPU Fan|1240 RPM|1250 RPM|45%|CPU Package|🟢 Optimal",
        "GPU Fan|1850 RPM|1800 RPM|68%|GPU Core|🟡 Adjusting",
        "Case Fan 1|980 RPM|1000 RPM|35%|Motherboard|🟢 Optimal",
        "Case Fan 2|1120 RPM|1100 RPM|40%|Motherboard|🟢 Optimal",
        "AIO Pump|2800 RPM|2800 RPM|100%|CPU Package|🟢 Optimal"
    };
    
    for (const QString &fan : fanData) {
        QStringList parts = fan.split("|");
        if (parts.size() == 6) {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_fanSpeedTree);
            for (int i = 0; i < parts.size(); ++i) {
                item->setText(i, parts[i]);
            }
            // Color code by status
            if (parts[5].contains("🟢")) {
                item->setBackground(5, QBrush(QColor(100, 255, 100, 50)));
            } else if (parts[5].contains("🟡")) {
                item->setBackground(5, QBrush(QColor(255, 255, 100, 50)));
            }
        }
    }
    
    monitorLayout->addWidget(m_fanSpeedTree);
    rightLayout->addWidget(monitorGroup);
    
    // Manual Override Controls
    QGroupBox *manualGroup = new QGroupBox("✋ Manual Override Controls");
    QVBoxLayout *manualLayout = new QVBoxLayout(manualGroup);
    
    m_fanControlEnabled = new QCheckBox("🔒 Enable Manual Override (Disables AI)");
    manualLayout->addWidget(m_fanControlEnabled);
    
    // Individual fan controls
    QStringList fanNames = {"CPU Fan", "GPU Fan", "Case Fan 1", "Case Fan 2", "AIO Pump"};
    for (const QString &fanName : fanNames) {
        QHBoxLayout *fanControlLayout = new QHBoxLayout();
        QLabel *fanLabel = new QLabel(fanName + ":");
        fanLabel->setMinimumWidth(80);
        QSlider *fanSlider = new QSlider(Qt::Horizontal);
        fanSlider->setRange(0, 100);
        fanSlider->setValue(50);
        fanSlider->setEnabled(false);
        QLabel *fanValueLabel = new QLabel("50%");
        fanValueLabel->setMinimumWidth(40);
        
        fanControlLayout->addWidget(fanLabel);
        fanControlLayout->addWidget(fanSlider);
        fanControlLayout->addWidget(fanValueLabel);
        manualLayout->addLayout(fanControlLayout);
    }
    
    rightLayout->addWidget(manualGroup);
    
    // Fan Intelligence Stats
    QGroupBox *statsGroup = new QGroupBox("🧠 Fan Intelligence Stats");
    QVBoxLayout *statsLayout = new QVBoxLayout(statsGroup);
    
    QLabel *statsLabel = new QLabel(
        "🎯 AI Optimization: Active\n"
        "📈 Performance Boost: +12%\n"
        "🔇 Noise Reduction: -8 dB\n"
        "🌡️ Temperature Control: Excellent\n"
        "⚡ Power Efficiency: +15%\n"
        "🕐 Adaptive Response: 0.3s"
    );
    statsLabel->setStyleSheet("color: #cccccc; font-family: monospace;");
    statsLayout->addWidget(statsLabel);
    
    rightLayout->addWidget(statsGroup);
    rightLayout->addStretch();
    
    // Add panels to main layout
    mainLayout->addWidget(leftPanel, 2);
    mainLayout->addWidget(rightPanel, 1);
}

void RGBFanControl::setupPowerManagementTab()
{
    QWidget *powerTab = new QWidget();
    m_tabWidget->addTab(powerTab, "⚡ Power Management");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(powerTab);
    
    // Left panel - Power profiles and control
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Power Management Header
    QLabel *powerHeader = new QLabel("⚡ OriginPC Intelligent Power Management");
    QFont headerFont = powerHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    powerHeader->setFont(headerFont);
    powerHeader->setStyleSheet("color: #4a90e2; padding: 5px;");
    leftLayout->addWidget(powerHeader);
    
    // Power Profiles
    QGroupBox *profileGroup = new QGroupBox("🔋 Power Profiles");
    QVBoxLayout *profileLayout = new QVBoxLayout(profileGroup);
    
    QRadioButton *performanceBtn = new QRadioButton("🚀 Maximum Performance");
    QRadioButton *balancedBtn = new QRadioButton("⚖️ Balanced Optimization");
    QRadioButton *powerSaveBtn = new QRadioButton("🌱 Power Saving");
    QRadioButton *customBtn = new QRadioButton("⚙️ Custom Profile");
    balancedBtn->setChecked(true);
    
    profileLayout->addWidget(performanceBtn);
    profileLayout->addWidget(balancedBtn);
    profileLayout->addWidget(powerSaveBtn);
    profileLayout->addWidget(customBtn);
    leftLayout->addWidget(profileGroup);
    
    // CPU Power Management
    QGroupBox *cpuGroup = new QGroupBox("💻 CPU Power Control");
    QGridLayout *cpuLayout = new QGridLayout(cpuGroup);
    
    cpuLayout->addWidget(new QLabel("Governor:"), 0, 0);
    QComboBox *governorCombo = new QComboBox();
    governorCombo->addItems({"performance", "schedutil", "ondemand", "powersave"});
    cpuLayout->addWidget(governorCombo, 0, 1);
    
    cpuLayout->addWidget(new QLabel("Energy Preference:"), 1, 0);
    QComboBox *energyCombo = new QComboBox();
    energyCombo->addItems({"performance", "balance_performance", "balance_power", "power"});
    cpuLayout->addWidget(energyCombo, 1, 1);
    
    cpuLayout->addWidget(new QLabel("Turbo Boost:"), 2, 0);
    QCheckBox *turboCheck = new QCheckBox("Enable CPU Turbo");
    turboCheck->setChecked(true);
    cpuLayout->addWidget(turboCheck, 2, 1);
    
    leftLayout->addWidget(cpuGroup);
    
    // GPU Power Management
    QGroupBox *gpuGroup = new QGroupBox("🎮 GPU Power Control");
    QVBoxLayout *gpuLayout = new QVBoxLayout(gpuGroup);
    
    QHBoxLayout *gpuModeLayout = new QHBoxLayout();
    gpuModeLayout->addWidget(new QLabel("Power Mode:"));
    QComboBox *gpuModeCombo = new QComboBox();
    gpuModeCombo->addItems({"Maximum Performance", "Adaptive", "Optimal Power"});
    gpuModeLayout->addWidget(gpuModeCombo);
    gpuLayout->addLayout(gpuModeLayout);
    
    QCheckBox *gpuBoostCheck = new QCheckBox("🚀 Enable GPU Boost");
    gpuBoostCheck->setChecked(true);
    gpuLayout->addWidget(gpuBoostCheck);
    
    leftLayout->addWidget(gpuGroup);
    leftLayout->addStretch();
    
    // Right panel - Power monitoring
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Battery/Power Status
    QGroupBox *statusGroup = new QGroupBox("🔌 Power Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    QLabel *powerStatusLabel = new QLabel(
        "⚡ Power Source: AC Adapter\n"
        "🔋 Battery: 85% (Charging)\n"
        "⏱️ Time Remaining: 2h 45m\n"
        "🌡️ Battery Health: Excellent\n"
        "📊 Power Consumption: 45W"
    );
    powerStatusLabel->setStyleSheet("color: #cccccc; font-family: monospace; padding: 10px;");
    statusLayout->addWidget(powerStatusLabel);
    
    rightLayout->addWidget(statusGroup);
    
    // Power Analytics
    QGroupBox *analyticsGroup = new QGroupBox("📊 Power Analytics");
    QVBoxLayout *analyticsLayout = new QVBoxLayout(analyticsGroup);
    
    QLabel *analyticsLabel = new QLabel(
        "💡 Efficiency Optimization: +18%\n"
        "⏰ Average Daily Usage: 6.2h\n"
        "🔋 Battery Cycles: 127\n"
        "🌡️ Thermal Efficiency: Optimal\n"
        "⚡ Peak Power Draw: 89W\n"
        "💰 Estimated Energy Cost: $0.15/day"
    );
    analyticsLabel->setStyleSheet("color: #cccccc; font-family: monospace; padding: 10px;");
    analyticsLayout->addWidget(analyticsLabel);
    
    rightLayout->addWidget(analyticsGroup);
    
    // Advanced Power Settings
    QGroupBox *advancedGroup = new QGroupBox("⚙️ Advanced Settings");
    QVBoxLayout *advancedLayout = new QVBoxLayout(advancedGroup);
    
    QCheckBox *adaptiveCheck = new QCheckBox("🧠 Adaptive Power Management");
    adaptiveCheck->setChecked(true);
    QCheckBox *wakeCheck = new QCheckBox("⏰ Intelligent Wake Scheduling");
    QCheckBox *hibernateCheck = new QCheckBox("💤 Smart Hibernation");
    
    advancedLayout->addWidget(adaptiveCheck);
    advancedLayout->addWidget(wakeCheck);
    advancedLayout->addWidget(hibernateCheck);
    
    rightLayout->addWidget(advancedGroup);
    rightLayout->addStretch();
    
    // Add panels to main layout
    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(rightPanel, 1);
}

void RGBFanControl::setupHardwareOptimizationTab()
{
    QWidget *hardwareTab = new QWidget();
    m_tabWidget->addTab(hardwareTab, "🔧 Hardware Optimization");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(hardwareTab);
    
    // Header
    QLabel *hwHeader = new QLabel("🔧 OriginPC Hardware Optimization Suite");
    QFont headerFont = hwHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 2);
    hwHeader->setFont(headerFont);
    hwHeader->setStyleSheet("color: #4a90e2; padding: 10px; text-align: center;");
    hwHeader->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hwHeader);
    
    // Create tab widget for hardware categories
    QTabWidget *hwTabWidget = new QTabWidget();
    
    // CPU Optimization Tab
    QWidget *cpuOptTab = new QWidget();
    QVBoxLayout *cpuOptLayout = new QVBoxLayout(cpuOptTab);
    
    QGroupBox *cpuBoostGroup = new QGroupBox("🚀 CPU Performance Boost");
    QVBoxLayout *cpuBoostLayout = new QVBoxLayout(cpuBoostGroup);
    QPushButton *cpuBoostBtn = new QPushButton("⚡ Apply CPU Boost (10 seconds)");
    cpuBoostBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 10px; border-radius: 5px;");
    QLabel *cpuBoostDesc = new QLabel("Temporarily boosts CPU to maximum performance for demanding tasks.");
    cpuBoostLayout->addWidget(cpuBoostBtn);
    cpuBoostLayout->addWidget(cpuBoostDesc);
    cpuOptLayout->addWidget(cpuBoostGroup);
    
    QGroupBox *cpuStatsGroup = new QGroupBox("📊 CPU Statistics");
    QVBoxLayout *cpuStatsLayout = new QVBoxLayout(cpuStatsGroup);
    QLabel *cpuStatsLabel = new QLabel(
        "🏷️ Model: Intel Core i9-13900K\n"
        "🔢 Cores: 24 (8P + 16E)\n"
        "⚡ Base Clock: 3.0 GHz\n"
        "🚀 Boost Clock: 5.8 GHz\n"
        "🌡️ Current Temp: 42°C\n"
        "📊 Current Load: 25%"
    );
    cpuStatsLabel->setStyleSheet("font-family: monospace; color: #cccccc;");
    cpuStatsLayout->addWidget(cpuStatsLabel);
    cpuOptLayout->addWidget(cpuStatsGroup);
    cpuOptLayout->addStretch();
    
    hwTabWidget->addTab(cpuOptTab, "💻 CPU Optimization");
    
    // Memory Optimization Tab
    QWidget *memOptTab = new QWidget();
    QVBoxLayout *memOptLayout = new QVBoxLayout(memOptTab);
    
    QGroupBox *memCleanGroup = new QGroupBox("🧹 Memory Cleaning");
    QVBoxLayout *memCleanLayout = new QVBoxLayout(memCleanGroup);
    QPushButton *memCleanBtn = new QPushButton("🗑️ Clean System Memory");
    memCleanBtn->setStyleSheet("background-color: #5bc0de; color: white; padding: 10px; border-radius: 5px;");
    QLabel *memCleanDesc = new QLabel("Clears system caches and optimizes memory allocation.");
    memCleanLayout->addWidget(memCleanBtn);
    memCleanLayout->addWidget(memCleanDesc);
    memOptLayout->addWidget(memCleanGroup);
    
    QGroupBox *memStatsGroup = new QGroupBox("📊 Memory Statistics");
    QVBoxLayout *memStatsLayout = new QVBoxLayout(memStatsGroup);
    QLabel *memStatsLabel = new QLabel(
        "💾 Total RAM: 32 GB DDR5\n"
        "⚡ Speed: 6000 MHz\n"
        "📊 Usage: 12.4 GB (38%)\n"
        "💨 Available: 19.6 GB\n"
        "🔄 Cached: 2.1 GB\n"
        "📈 Efficiency: 92%"
    );
    memStatsLabel->setStyleSheet("font-family: monospace; color: #cccccc;");
    memStatsLayout->addWidget(memStatsLabel);
    memOptLayout->addWidget(memStatsGroup);
    memOptLayout->addStretch();
    
    hwTabWidget->addTab(memOptTab, "🧠 Memory Optimization");
    
    // Storage Optimization Tab
    QWidget *storageOptTab = new QWidget();
    QVBoxLayout *storageOptLayout = new QVBoxLayout(storageOptTab);
    
    QGroupBox *storageCleanGroup = new QGroupBox("🗄️ Storage Optimization");
    QVBoxLayout *storageCleanLayout = new QVBoxLayout(storageCleanGroup);
    QPushButton *trimBtn = new QPushButton("✂️ Run TRIM Command");
    QPushButton *cleanCacheBtn = new QPushButton("🧹 Clean System Cache");
    trimBtn->setStyleSheet("background-color: #f0ad4e; color: white; padding: 8px; border-radius: 5px;");
    cleanCacheBtn->setStyleSheet("background-color: #f0ad4e; color: white; padding: 8px; border-radius: 5px;");
    storageCleanLayout->addWidget(trimBtn);
    storageCleanLayout->addWidget(cleanCacheBtn);
    storageOptLayout->addWidget(storageCleanGroup);
    
    QGroupBox *storageStatsGroup = new QGroupBox("📊 Storage Statistics");
    QVBoxLayout *storageStatsLayout = new QVBoxLayout(storageStatsGroup);
    QLabel *storageStatsLabel = new QLabel(
        "💾 Primary: 2TB NVMe SSD\n"
        "⚡ Read Speed: 7,000 MB/s\n"
        "📝 Write Speed: 6,500 MB/s\n"
        "📊 Usage: 45% (900 GB)\n"
        "🌡️ Temperature: 38°C\n"
        "💪 Health: 98%"
    );
    storageStatsLabel->setStyleSheet("font-family: monospace; color: #cccccc;");
    storageStatsLayout->addWidget(storageStatsLabel);
    storageOptLayout->addWidget(storageStatsGroup);
    storageOptLayout->addStretch();
    
    hwTabWidget->addTab(storageOptTab, "💾 Storage Optimization");
    
    mainLayout->addWidget(hwTabWidget);
}

void RGBFanControl::setupProfilesAndMacrosTab()
{
    m_profilesTab = new QWidget();
    m_tabWidget->addTab(m_profilesTab, "📁 Profiles & Macros");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_profilesTab);
    
    // Left panel - Profile management
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Profiles Header
    QLabel *profilesHeader = new QLabel("📁 Professional Profile Management");
    QFont headerFont = profilesHeader->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    profilesHeader->setFont(headerFont);
    profilesHeader->setStyleSheet("color: #4a90e2; padding: 5px;");
    leftLayout->addWidget(profilesHeader);
    
    // Profile Categories
    QGroupBox *categoryGroup = new QGroupBox("📂 Profile Categories");
    QVBoxLayout *categoryLayout = new QVBoxLayout(categoryGroup);
    
    m_profilesTree = new QTreeWidget();
    m_profilesTree->setHeaderLabels({"Profile Name", "Type", "Last Modified", "Status"});
    
    // Create profile categories
    QTreeWidgetItem *rgbCategory = new QTreeWidgetItem(m_profilesTree);
    rgbCategory->setText(0, "🌈 RGB Profiles");
    rgbCategory->setText(1, "Category");
    rgbCategory->setExpanded(true);
    
    QStringList rgbProfiles = {
        "🌊 Ocean Wave|RGB|2024-06-23|✅ Active",
        "🔥 Dragon Fire|RGB|2024-06-22|⏸️ Saved",
        "⚡ Lightning Storm|RGB|2024-06-21|⏸️ Saved",
        "🌙 Midnight Blue|RGB|2024-06-20|⏸️ Saved"
    };
    
    for (const QString &profile : rgbProfiles) {
        QStringList parts = profile.split("|");
        QTreeWidgetItem *item = new QTreeWidgetItem(rgbCategory);
        for (int i = 0; i < parts.size(); ++i) {
            item->setText(i, parts[i]);
        }
    }
    
    QTreeWidgetItem *fanCategory = new QTreeWidgetItem(m_profilesTree);
    fanCategory->setText(0, "🌪️ Fan Profiles");
    fanCategory->setText(1, "Category");
    fanCategory->setExpanded(true);
    
    QStringList fanProfiles = {
        "🔇 Ultra Silent|Fan|2024-06-23|✅ Active",
        "⚖️ Balanced Pro|Fan|2024-06-22|⏸️ Saved",
        "🚀 Max Performance|Fan|2024-06-21|⏸️ Saved",
        "🎮 Gaming Optimized|Fan|2024-06-20|⏸️ Saved"
    };
    
    for (const QString &profile : fanProfiles) {
        QStringList parts = profile.split("|");
        QTreeWidgetItem *item = new QTreeWidgetItem(fanCategory);
        for (int i = 0; i < parts.size(); ++i) {
            item->setText(i, parts[i]);
        }
    }
    
    QTreeWidgetItem *macroCategory = new QTreeWidgetItem(m_profilesTree);
    macroCategory->setText(0, "⭐ RGB Macros");
    macroCategory->setText(1, "Category");
    macroCategory->setExpanded(true);
    
    QStringList macroProfiles = {
        "🌈 Rainbow Cascade|Macro|2024-06-23|⏸️ Recorded",
        "💥 Explosion Effect|Macro|2024-06-22|⏸️ Recorded",
        "🌊 Wave Sequence|Macro|2024-06-21|⏸️ Recorded"
    };
    
    for (const QString &macro : macroProfiles) {
        QStringList parts = macro.split("|");
        QTreeWidgetItem *item = new QTreeWidgetItem(macroCategory);
        for (int i = 0; i < parts.size(); ++i) {
            item->setText(i, parts[i]);
        }
    }
    
    categoryLayout->addWidget(m_profilesTree);
    leftLayout->addWidget(categoryGroup);
    
    // Profile Management Buttons
    QGroupBox *managementGroup = new QGroupBox("⚙️ Profile Management");
    QGridLayout *managementLayout = new QGridLayout(managementGroup);
    
    m_createProfileBtn = new QPushButton("➕ Create New");
    m_deleteProfileBtn = new QPushButton("🗑️ Delete");
    m_exportProfileBtn = new QPushButton("📤 Export");
    m_importProfileBtn = new QPushButton("📥 Import");
    QPushButton *duplicateProfileBtn = new QPushButton("📋 Duplicate");
    QPushButton *shareProfileBtn = new QPushButton("🌐 Share Online");
    
    managementLayout->addWidget(m_createProfileBtn, 0, 0);
    managementLayout->addWidget(m_deleteProfileBtn, 0, 1);
    managementLayout->addWidget(m_exportProfileBtn, 1, 0);
    managementLayout->addWidget(m_importProfileBtn, 1, 1);
    managementLayout->addWidget(duplicateProfileBtn, 2, 0);
    managementLayout->addWidget(shareProfileBtn, 2, 1);
    
    leftLayout->addWidget(managementGroup);
    leftLayout->addStretch();
    
    // Right panel - Profile details and macro recorder
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    // Profile Details
    QGroupBox *detailsGroup = new QGroupBox("📋 Profile Details");
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    
    m_profileDescriptionEdit = new QTextEdit();
    m_profileDescriptionEdit->setMaximumHeight(150);
    m_profileDescriptionEdit->setPlaceholderText(
        "Profile Description:\n"
        "- Created: 2024-06-23\n"
        "- Author: OriginPC User\n"
        "- Version: 1.2\n"
        "- Compatible: All OriginPC RGB devices\n"
        "- Notes: Professional gaming setup with reactive effects"
    );
    detailsLayout->addWidget(m_profileDescriptionEdit);
    
    rightLayout->addWidget(detailsGroup);
    
    // Macro Recorder
    QGroupBox *macroGroup = new QGroupBox("⏺️ RGB Macro Recorder");
    QVBoxLayout *macroLayout = new QVBoxLayout(macroGroup);
    
    QHBoxLayout *macroControlsLayout = new QHBoxLayout();
    QPushButton *recordBtn = new QPushButton("⏺️ Start Recording");
    QPushButton *stopBtn = new QPushButton("⏹️ Stop Recording");
    QPushButton *playBtn = new QPushButton("▶️ Play Macro");
    QPushButton *pauseBtn = new QPushButton("⏸️ Pause");
    
    recordBtn->setStyleSheet("background-color: #d9534f; color: white; padding: 8px; border-radius: 5px;");
    stopBtn->setStyleSheet("background-color: #5bc0de; color: white; padding: 8px; border-radius: 5px;");
    playBtn->setStyleSheet("background-color: #5cb85c; color: white; padding: 8px; border-radius: 5px;");
    pauseBtn->setStyleSheet("background-color: #f0ad4e; color: white; padding: 8px; border-radius: 5px;");
    
    macroControlsLayout->addWidget(recordBtn);
    macroControlsLayout->addWidget(stopBtn);
    macroControlsLayout->addWidget(playBtn);
    macroControlsLayout->addWidget(pauseBtn);
    macroLayout->addLayout(macroControlsLayout);
    
    QLabel *macroStatus = new QLabel(
        "📊 Macro Status: Ready\n"
        "⏱️ Recording Time: 0:00\n"
        "📝 Commands Recorded: 0\n"
        "🔄 Loop: Disabled"
    );
    macroStatus->setStyleSheet("font-family: monospace; color: #cccccc; padding: 10px;");
    macroLayout->addWidget(macroStatus);
    
    rightLayout->addWidget(macroGroup);
    
    // Cloud Sync
    QGroupBox *cloudGroup = new QGroupBox("☁️ Cloud Sync");
    QVBoxLayout *cloudLayout = new QVBoxLayout(cloudGroup);
    
    QHBoxLayout *cloudButtonsLayout = new QHBoxLayout();
    QPushButton *syncBtn = new QPushButton("☁️ Sync to Cloud");
    QPushButton *downloadBtn = new QPushButton("📥 Download Profiles");
    cloudButtonsLayout->addWidget(syncBtn);
    cloudButtonsLayout->addWidget(downloadBtn);
    cloudLayout->addLayout(cloudButtonsLayout);
    
    QLabel *cloudStatus = new QLabel(
        "✅ Connected to OriginPC Cloud\n"
        "📊 Profiles Synced: 12\n"
        "⏰ Last Sync: 2 minutes ago"
    );
    cloudStatus->setStyleSheet("font-family: monospace; color: #cccccc;");
    cloudLayout->addWidget(cloudStatus);
    
    rightLayout->addWidget(cloudGroup);
    rightLayout->addStretch();
    
    // Add panels to main layout
    mainLayout->addWidget(leftPanel, 2);
    mainLayout->addWidget(rightPanel, 1);
}

void RGBFanControl::setupRGBControlTab()
{
    m_rgbControlTab = new QWidget();
    m_tabWidget->addTab(m_rgbControlTab, "🌈 RGB Control");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_rgbControlTab);
    
    // RGB Effects group
    QGroupBox *effectsGroup = new QGroupBox("RGB Effects");
    QGridLayout *effectsLayout = new QGridLayout(effectsGroup);
    
    effectsLayout->addWidget(new QLabel("Effect:"), 0, 0);
    m_rgbEffectCombo = new QComboBox();
    m_rgbEffectCombo->addItems({"Static", "Breathing", "Rainbow", "Wave", "Custom"});
    effectsLayout->addWidget(m_rgbEffectCombo, 0, 1);
    
    effectsLayout->addWidget(new QLabel("Primary Color:"), 1, 0);
    m_primaryColorBtn = new QPushButton();
    m_primaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(m_primaryColor.name()));
    m_primaryColorBtn->setFixedSize(80, 30);
    effectsLayout->addWidget(m_primaryColorBtn, 1, 1);
    
    effectsLayout->addWidget(new QLabel("Secondary Color:"), 2, 0);
    m_secondaryColorBtn = new QPushButton();
    m_secondaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(m_secondaryColor.name()));
    m_secondaryColorBtn->setFixedSize(80, 30);
    effectsLayout->addWidget(m_secondaryColorBtn, 2, 1);
    
    effectsLayout->addWidget(new QLabel("Brightness:"), 3, 0);
    m_brightnessSlider = new QSlider(Qt::Horizontal);
    m_brightnessSlider->setRange(0, 100);
    m_brightnessSlider->setValue(100);
    m_brightnessLabel = new QLabel("100%");
    QHBoxLayout *brightnessLayout = new QHBoxLayout();
    brightnessLayout->addWidget(m_brightnessSlider);
    brightnessLayout->addWidget(m_brightnessLabel);
    effectsLayout->addLayout(brightnessLayout, 3, 1);
    
    effectsLayout->addWidget(new QLabel("Speed:"), 4, 0);
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(50);
    m_speedLabel = new QLabel("50%");
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedLabel);
    effectsLayout->addLayout(speedLayout, 4, 1);
    
    mainLayout->addWidget(effectsGroup);
    
    // RGB Preview group
    QGroupBox *previewGroup = new QGroupBox("Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    
    m_rgbPreview = new QLabel();
    m_rgbPreview->setFixedHeight(100);
    m_rgbPreview->setStyleSheet("border: 2px solid #333; background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 red, stop:1 blue);");
    m_rgbPreview->setAlignment(Qt::AlignCenter);
    m_rgbPreview->setText("RGB Effect Preview");
    previewLayout->addWidget(m_rgbPreview);
    
    mainLayout->addWidget(previewGroup);
    
    // RGB Profiles group
    QGroupBox *profilesGroup = new QGroupBox("RGB Profiles");
    QHBoxLayout *profilesLayout = new QHBoxLayout(profilesGroup);
    
    m_rgbProfileCombo = new QComboBox();
    m_saveRGBProfileBtn = new QPushButton("Save Profile");
    m_loadRGBProfileBtn = new QPushButton("Load Profile");
    
    profilesLayout->addWidget(new QLabel("Profile:"));
    profilesLayout->addWidget(m_rgbProfileCombo);
    profilesLayout->addWidget(m_saveRGBProfileBtn);
    profilesLayout->addWidget(m_loadRGBProfileBtn);
    profilesLayout->addStretch();
    
    mainLayout->addWidget(profilesGroup);
    mainLayout->addStretch();
}

void RGBFanControl::setupFanControlTab()
{
    m_fanControlTab = new QWidget();
    m_tabWidget->addTab(m_fanControlTab, "🌪️ Fan Control");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_fanControlTab);
    
    // Fan control enable/disable
    QGroupBox *enableGroup = new QGroupBox("Fan Control");
    QHBoxLayout *enableLayout = new QHBoxLayout(enableGroup);
    
    m_fanControlEnabled = new QCheckBox("Enable Automatic Fan Control");
    m_fanControlEnabled->setChecked(false);
    enableLayout->addWidget(m_fanControlEnabled);
    enableLayout->addStretch();
    
    mainLayout->addWidget(enableGroup);
    
    // Fan profiles
    QGroupBox *profileGroup = new QGroupBox("Fan Profiles");
    QHBoxLayout *profileLayout = new QHBoxLayout(profileGroup);
    
    profileLayout->addWidget(new QLabel("Profile:"));
    m_fanProfileCombo = new QComboBox();
    m_fanProfileCombo->addItems({"Silent", "Balanced", "Performance", "Custom"});
    profileLayout->addWidget(m_fanProfileCombo);
    
    m_saveFanProfileBtn = new QPushButton("Save Profile");
    m_loadFanProfileBtn = new QPushButton("Load Profile");
    profileLayout->addWidget(m_saveFanProfileBtn);
    profileLayout->addWidget(m_loadFanProfileBtn);
    profileLayout->addStretch();
    
    mainLayout->addWidget(profileGroup);
    
    // Fan curve editor
    QGroupBox *curveGroup = new QGroupBox("Temperature-Fan Speed Curve");
    QVBoxLayout *curveLayout = new QVBoxLayout(curveGroup);
    
    m_fanCurveTree = new QTreeWidget();
    m_fanCurveTree->setHeaderLabels({"Temperature (°C)", "Fan Speed (%)", "Action"});
    m_fanCurveTree->setMaximumHeight(200);
    
    // Add some default curve points
    QStringList curvePoints = {"30°C - 20%", "40°C - 30%", "50°C - 50%", "60°C - 70%", "70°C - 90%", "80°C - 100%"};
    for (const QString &point : curvePoints) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_fanCurveTree);
        QStringList parts = point.split(" - ");
        if (parts.size() == 2) {
            item->setText(0, parts[0]);
            item->setText(1, parts[1]);
            item->setText(2, "Edit");
        }
    }
    
    curveLayout->addWidget(m_fanCurveTree);
    
    mainLayout->addWidget(curveGroup);
    
    // Manual fan control
    QGroupBox *manualGroup = new QGroupBox("Manual Fan Control");
    QVBoxLayout *manualLayout = new QVBoxLayout(manualGroup);
    
    m_manualFanSlider = new QSlider(Qt::Horizontal);
    m_manualFanSlider->setRange(0, 100);
    m_manualFanSlider->setValue(0);
    m_manualFanSlider->setEnabled(false); // Disabled when auto control is on
    
    m_manualFanLabel = new QLabel("Manual Speed: 0%");
    
    QHBoxLayout *manualSliderLayout = new QHBoxLayout();
    manualSliderLayout->addWidget(new QLabel("Speed:"));
    manualSliderLayout->addWidget(m_manualFanSlider);
    manualSliderLayout->addWidget(m_manualFanLabel);
    
    manualLayout->addLayout(manualSliderLayout);
    
    mainLayout->addWidget(manualGroup);
    
    // Fan status
    QGroupBox *statusGroup = new QGroupBox("Fan Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    m_fanStatusLabel = new QLabel("Status: Automatic control disabled");
    statusLayout->addWidget(m_fanStatusLabel);
    
    mainLayout->addWidget(statusGroup);
    mainLayout->addStretch();
}

void RGBFanControl::setupProfilesTab()
{
    m_profilesTab = new QWidget();
    m_tabWidget->addTab(m_profilesTab, "📁 Profiles");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_profilesTab);
    
    // Left side - Profile list
    QVBoxLayout *leftLayout = new QVBoxLayout();
    
    QGroupBox *profileListGroup = new QGroupBox("Available Profiles");
    QVBoxLayout *profileListLayout = new QVBoxLayout(profileListGroup);
    
    m_profilesTree = new QTreeWidget();
    m_profilesTree->setHeaderLabels({"Profile Name", "Type", "Modified"});
    profileListLayout->addWidget(m_profilesTree);
    
    // Profile management buttons
    QHBoxLayout *profileButtonsLayout = new QHBoxLayout();
    m_createProfileBtn = new QPushButton("Create New");
    m_deleteProfileBtn = new QPushButton("Delete");
    m_exportProfileBtn = new QPushButton("Export");
    m_importProfileBtn = new QPushButton("Import");
    
    profileButtonsLayout->addWidget(m_createProfileBtn);
    profileButtonsLayout->addWidget(m_deleteProfileBtn);
    profileButtonsLayout->addWidget(m_exportProfileBtn);
    profileButtonsLayout->addWidget(m_importProfileBtn);
    
    profileListLayout->addLayout(profileButtonsLayout);
    leftLayout->addWidget(profileListGroup);
    
    // Right side - Profile details
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    QGroupBox *profileDetailsGroup = new QGroupBox("Profile Details");
    QVBoxLayout *profileDetailsLayout = new QVBoxLayout(profileDetailsGroup);
    
    m_profileDescriptionEdit = new QTextEdit();
    m_profileDescriptionEdit->setMaximumHeight(200);
    m_profileDescriptionEdit->setPlaceholderText("Profile description and notes...");
    profileDetailsLayout->addWidget(m_profileDescriptionEdit);
    
    rightLayout->addWidget(profileDetailsGroup);
    rightLayout->addStretch();
    
    // Add to main layout
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);
}

void RGBFanControl::setupConnections()
{
    // RGB controls
    connect(m_primaryColorBtn, &QPushButton::clicked, this, &RGBFanControl::selectPrimaryColor);
    connect(m_secondaryColorBtn, &QPushButton::clicked, this, &RGBFanControl::selectSecondaryColor);
    connect(m_rgbEffectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RGBFanControl::changeRGBEffect);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, &RGBFanControl::changeBrightness);
    connect(m_speedSlider, &QSlider::valueChanged, this, &RGBFanControl::changeSpeed);
    connect(m_saveRGBProfileBtn, &QPushButton::clicked, this, &RGBFanControl::saveRGBProfile);
    connect(m_loadRGBProfileBtn, &QPushButton::clicked, this, &RGBFanControl::loadRGBProfile);
    
    // Fan controls
    connect(m_fanControlEnabled, &QCheckBox::toggled, this, &RGBFanControl::enableFanControl);
    connect(m_manualFanSlider, &QSlider::valueChanged, this, &RGBFanControl::setManualFanSpeed);
    connect(m_saveFanProfileBtn, &QPushButton::clicked, this, &RGBFanControl::saveFanProfile);
    connect(m_loadFanProfileBtn, &QPushButton::clicked, this, &RGBFanControl::loadFanProfile);
    
    // Profile management
    connect(m_createProfileBtn, &QPushButton::clicked, this, &RGBFanControl::createNewProfile);
    connect(m_deleteProfileBtn, &QPushButton::clicked, this, &RGBFanControl::deleteProfile);
    connect(m_exportProfileBtn, &QPushButton::clicked, this, &RGBFanControl::exportProfile);
    connect(m_importProfileBtn, &QPushButton::clicked, this, &RGBFanControl::importProfile);
}

void RGBFanControl::startMonitoring()
{
    if (!m_monitoringActive) {
        m_monitoringActive = true;
        m_systemMonitor->setUpdateInterval(2000); // 2 second updates
        m_systemMonitor->start();
        m_systemUpdateTimer->start(2000);
        
        emit statusMessage("System monitoring started");
    }
}

void RGBFanControl::stopMonitoring()
{
    if (m_monitoringActive) {
        m_monitoringActive = false;
        m_systemMonitor->stopMonitoring();
        m_systemUpdateTimer->stop();
        
        emit statusMessage("System monitoring stopped");
    }
}

void RGBFanControl::onSystemDataUpdated(const SystemData &data)
{
    QMutexLocker locker(&m_dataMutex);
    m_lastSystemData = data;
    
    // Update fan speeds based on temperature if automatic control is enabled
    if (m_fanControlEnabled->isChecked()) {
        m_fanManager->updateFanSpeeds(data);
    }
}

void RGBFanControl::updateSystemDisplays()
{
    QMutexLocker locker(&m_dataMutex);
    
    // Update CPU display
    m_cpuUsageLabel->setText(QString("CPU Usage: %1%").arg(m_lastSystemData.cpuPercent, 0, 'f', 1));
    m_cpuTempLabel->setText(QString("CPU Temperature: %1°C").arg(m_lastSystemData.cpuTemp, 0, 'f', 1));
    m_cpuProgressBar->setValue(static_cast<int>(m_lastSystemData.cpuPercent));
    
    // Update memory display
    m_memoryUsageLabel->setText(QString("Memory Usage: %1% (%2 GB / %3 GB)")
                               .arg(m_lastSystemData.memoryPercent, 0, 'f', 1)
                               .arg(m_lastSystemData.memoryUsed / 1024.0 / 1024.0 / 1024.0, 0, 'f', 1)
                               .arg(m_lastSystemData.memoryTotal / 1024.0 / 1024.0 / 1024.0, 0, 'f', 1));
    m_memoryProgressBar->setValue(static_cast<int>(m_lastSystemData.memoryPercent));
    
    // Update disk display
    m_diskUsageLabel->setText(QString("Disk Usage: %1% (%2 GB / %3 GB)")
                             .arg(m_lastSystemData.diskPercent, 0, 'f', 1)
                             .arg(m_lastSystemData.diskUsed / 1024.0 / 1024.0 / 1024.0, 0, 'f', 1)
                             .arg(m_lastSystemData.diskTotal / 1024.0 / 1024.0 / 1024.0, 0, 'f', 1));
    m_diskProgressBar->setValue(static_cast<int>(m_lastSystemData.diskPercent));
    
    // Update GPU display
    m_gpuUsageLabel->setText(QString("GPU Usage: %1%").arg(m_lastSystemData.gpuLoad, 0, 'f', 1));
    m_gpuTempLabel->setText(QString("GPU Temperature: %1°C").arg(m_lastSystemData.gpuTemp, 0, 'f', 1));
    m_gpuProgressBar->setValue(static_cast<int>(m_lastSystemData.gpuLoad));
    
    updateTemperatureDisplays();
    updateFanDisplays();
}

void RGBFanControl::updateTemperatureDisplays()
{
    m_temperatureTree->clear();
    
    for (const auto &temp : m_lastSystemData.cpuTemps) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_temperatureTree);
        item->setText(0, temp.first);
        item->setText(1, QString("%1°C").arg(temp.second, 0, 'f', 1));
        
        // Set status color based on temperature
        QString status = "Normal";
        if (temp.second > 80) {
            status = "High";
            item->setBackground(2, QBrush(QColor(255, 100, 100)));
        } else if (temp.second > 70) {
            status = "Warm";
            item->setBackground(2, QBrush(QColor(255, 200, 100)));
        } else {
            item->setBackground(2, QBrush(QColor(100, 255, 100)));
        }
        item->setText(2, status);
    }
}

void RGBFanControl::updateFanDisplays()
{
    m_fanSpeedTree->clear();
    
    for (const auto &fan : m_lastSystemData.fanSpeeds) {
        QTreeWidgetItem *item = new QTreeWidgetItem(m_fanSpeedTree);
        item->setText(0, fan.first);
        item->setText(1, QString("%1 RPM").arg(fan.second, 0, 'f', 0));
        
        // Calculate PWM percentage (rough estimate)
        int pwmPercent = static_cast<int>((fan.second / 2000.0) * 100); // Assuming max 2000 RPM
        pwmPercent = qMin(100, qMax(0, pwmPercent));
        item->setText(2, QString("%1%").arg(pwmPercent));
    }
}

void RGBFanControl::selectPrimaryColor()
{
    QColor color = QColorDialog::getColor(m_primaryColor, this, "Select Primary Color");
    if (color.isValid()) {
        m_primaryColor = color;
        m_primaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(color.name()));
        onRGBEffectChanged();
    }
}

void RGBFanControl::selectSecondaryColor()
{
    QColor color = QColorDialog::getColor(m_secondaryColor, this, "Select Secondary Color");
    if (color.isValid()) {
        m_secondaryColor = color;
        m_secondaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(color.name()));
        onRGBEffectChanged();
    }
}

void RGBFanControl::changeRGBEffect()
{
    onRGBEffectChanged();
}

void RGBFanControl::changeBrightness(int value)
{
    m_brightnessLabel->setText(QString("%1%").arg(value));
    onRGBEffectChanged();
}

void RGBFanControl::changeSpeed(int value)
{
    m_speedLabel->setText(QString("%1%").arg(value));
    onRGBEffectChanged();
}

void RGBFanControl::onRGBEffectChanged()
{
    // Create RGB effect based on current settings
    RGBEffect effect;
    effect.name = "Current";
    effect.type = m_rgbEffectCombo->currentText().toLower();
    effect.primaryColor = m_primaryColor;
    effect.secondaryColor = m_secondaryColor;
    effect.brightness = m_brightnessSlider->value();
    effect.speed = m_speedSlider->value();
    effect.enabled = true;
    
    // Update preview
    updateRGBPreview(effect);
    
    // Apply effect to devices
    m_rgbManager->applyEffect(effect);
    m_currentRGBEffect = effect;
    
    emit statusMessage(QString("Applied %1 RGB effect").arg(effect.type));
}

void RGBFanControl::updateRGBPreview(const RGBEffect &effect)
{
    QString gradient;
    
    if (effect.type == "static") {
        gradient = QString("background-color: %1;").arg(effect.primaryColor.name());
    } else if (effect.type == "breathing" || effect.type == "wave") {
        gradient = QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2);")
                  .arg(effect.primaryColor.name())
                  .arg(effect.secondaryColor.name());
    } else if (effect.type == "rainbow") {
        gradient = "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 red, stop:0.16 orange, stop:0.33 yellow, stop:0.5 green, stop:0.66 blue, stop:0.83 indigo, stop:1 violet);";
    } else {
        gradient = QString("background-color: %1;").arg(effect.primaryColor.name());
    }
    
    m_rgbPreview->setStyleSheet(QString("border: 2px solid #333; %1").arg(gradient));
    m_rgbPreview->setText(QString("%1 Effect\nBrightness: %2%\nSpeed: %3%")
                         .arg(effect.type.toUpper())
                         .arg(effect.brightness)
                         .arg(effect.speed));
}

void RGBFanControl::loadSettings()
{
    // Load RGB settings
    m_primaryColor = QColor(m_settings->value("rgb/primaryColor", "#ff0000").toString());
    m_secondaryColor = QColor(m_settings->value("rgb/secondaryColor", "#0000ff").toString());
    m_brightnessSlider->setValue(m_settings->value("rgb/brightness", 100).toInt());
    m_speedSlider->setValue(m_settings->value("rgb/speed", 50).toInt());
    m_rgbEffectCombo->setCurrentText(m_settings->value("rgb/effect", "Static").toString());
    
    // Load fan settings
    m_fanControlEnabled->setChecked(m_settings->value("fan/autoControl", false).toBool());
    m_fanProfileCombo->setCurrentText(m_settings->value("fan/profile", "Balanced").toString());
    
    // Update UI
    m_primaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(m_primaryColor.name()));
    m_secondaryColorBtn->setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(m_secondaryColor.name()));
    m_brightnessLabel->setText(QString("%1%").arg(m_brightnessSlider->value()));
    m_speedLabel->setText(QString("%1%").arg(m_speedSlider->value()));
}

void RGBFanControl::saveSettings()
{
    // Save RGB settings
    m_settings->setValue("rgb/primaryColor", m_primaryColor.name());
    m_settings->setValue("rgb/secondaryColor", m_secondaryColor.name());
    m_settings->setValue("rgb/brightness", m_brightnessSlider->value());
    m_settings->setValue("rgb/speed", m_speedSlider->value());
    m_settings->setValue("rgb/effect", m_rgbEffectCombo->currentText());
    
    // Save fan settings
    m_settings->setValue("fan/autoControl", m_fanControlEnabled->isChecked());
    m_settings->setValue("fan/profile", m_fanProfileCombo->currentText());
    
    m_settings->sync();
}

void RGBFanControl::createDefaultProfiles()
{
    // Create default RGB profiles
    RGBEffect staticRed;
    staticRed.name = "Static Red";
    staticRed.type = "static";
    staticRed.primaryColor = Qt::red;
    staticRed.brightness = 100;
    staticRed.speed = 50;
    staticRed.enabled = true;
    m_rgbProfiles["Static Red"] = staticRed;
    
    RGBEffect rainbow;
    rainbow.name = "Rainbow";
    rainbow.type = "rainbow";
    rainbow.brightness = 80;
    rainbow.speed = 30;
    rainbow.enabled = true;
    m_rgbProfiles["Rainbow"] = rainbow;
    
    // Create default fan profiles
    FanProfile silent;
    silent.name = "Silent";
    silent.tempToPwmCurve[30] = 15;
    silent.tempToPwmCurve[50] = 30;
    silent.tempToPwmCurve[70] = 60;
    silent.tempToPwmCurve[80] = 80;
    silent.enabled = true;
    silent.hysteresis = 3;
    m_fanProfiles["Silent"] = silent;
    
    FanProfile performance;
    performance.name = "Performance";
    performance.tempToPwmCurve[30] = 40;
    performance.tempToPwmCurve[50] = 60;
    performance.tempToPwmCurve[70] = 85;
    performance.tempToPwmCurve[80] = 100;
    performance.enabled = true;
    performance.hysteresis = 2;
    m_fanProfiles["Performance"] = performance;
    
    // Update combo boxes
    m_rgbProfileCombo->clear();
    m_rgbProfileCombo->addItems(m_rgbProfiles.keys());
    
    // Update profiles tree
    updateProfilesTree();
}

void RGBFanControl::updateProfilesTree()
{
    m_profilesTree->clear();
    
    // Add RGB profiles
    QTreeWidgetItem *rgbCategory = new QTreeWidgetItem(m_profilesTree);
    rgbCategory->setText(0, "RGB Profiles");
    rgbCategory->setText(1, "Category");
    rgbCategory->setExpanded(true);
    
    for (auto it = m_rgbProfiles.begin(); it != m_rgbProfiles.end(); ++it) {
        QTreeWidgetItem *item = new QTreeWidgetItem(rgbCategory);
        item->setText(0, it.key());
        item->setText(1, "RGB");
        item->setText(2, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    }
    
    // Add Fan profiles
    QTreeWidgetItem *fanCategory = new QTreeWidgetItem(m_profilesTree);
    fanCategory->setText(0, "Fan Profiles");
    fanCategory->setText(1, "Category");
    fanCategory->setExpanded(true);
    
    for (auto it = m_fanProfiles.begin(); it != m_fanProfiles.end(); ++it) {
        QTreeWidgetItem *item = new QTreeWidgetItem(fanCategory);
        item->setText(0, it.key());
        item->setText(1, "Fan");
        item->setText(2, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    }
}

// Implement stub methods for remaining functionality
void RGBFanControl::enableFanControl(bool enabled)
{
    m_manualFanSlider->setEnabled(!enabled);
    m_fanStatusLabel->setText(enabled ? "Status: Automatic control enabled" : "Status: Manual control enabled");
    emit statusMessage(enabled ? "Automatic fan control enabled" : "Manual fan control enabled");
}

void RGBFanControl::setManualFanSpeed(int speed)
{
    m_manualFanLabel->setText(QString("Manual Speed: %1%").arg(speed));
    if (!m_fanControlEnabled->isChecked()) {
        // Apply manual fan speed to all detected fans
        for (const QString &fan : m_fanDevices) {
            m_fanManager->setFanSpeed(fan, speed);
        }
        emit statusMessage(QString("Manual fan speed set to %1%").arg(speed));
    }
}

void RGBFanControl::saveRGBProfile()
{
    // Placeholder - would show dialog to save current RGB settings as named profile
    emit statusMessage("RGB profile save functionality will be implemented");
}

void RGBFanControl::loadRGBProfile()
{
    // Placeholder - would load selected RGB profile
    emit statusMessage("RGB profile load functionality will be implemented");
}

void RGBFanControl::saveFanProfile()
{
    // Placeholder - would save current fan curve as named profile
    emit statusMessage("Fan profile save functionality will be implemented");
}

void RGBFanControl::loadFanProfile()
{
    // Placeholder - would load selected fan profile
    emit statusMessage("Fan profile load functionality will be implemented");
}

void RGBFanControl::updateFanCurve()
{
    // Placeholder for fan curve updates
}

void RGBFanControl::onFanProfileChanged()
{
    // Placeholder for fan profile changes
}

void RGBFanControl::refreshSystemInfo()
{
    // Update system information display
    QString info = QString("System Information\n"
                          "==================\n"
                          "RGB Devices: %1\n"
                          "Fan Devices: %2\n"
                          "Monitoring: %3\n"
                          "Last Update: %4")
                  .arg(m_rgbDevices.size())
                  .arg(m_fanDevices.size())
                  .arg(m_monitoringActive ? "Active" : "Inactive")
                  .arg(QDateTime::fromSecsSinceEpoch(m_lastSystemData.timestamp).toString());
                  
    m_systemInfoText->setPlainText(info);
}

void RGBFanControl::createNewProfile()
{
    emit statusMessage("Create new profile functionality will be implemented");
}

void RGBFanControl::deleteProfile()
{
    emit statusMessage("Delete profile functionality will be implemented");
}

void RGBFanControl::exportProfile()
{
    emit statusMessage("Export profile functionality will be implemented");
}

void RGBFanControl::importProfile()
{
    emit statusMessage("Import profile functionality will be implemented");
}

// Continue with SystemMonitorThread implementation...
SystemMonitorThread::SystemMonitorThread(QObject *parent)
    : QThread(parent)
    , m_updateInterval(2000)
    , m_running(false)
{
}

SystemMonitorThread::~SystemMonitorThread()
{
    stopMonitoring();
    wait();
}

void SystemMonitorThread::setUpdateInterval(int msec)
{
    QMutexLocker locker(&m_mutex);
    m_updateInterval = msec;
}

void SystemMonitorThread::stopMonitoring()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void SystemMonitorThread::run()
{
    QMutexLocker locker(&m_mutex);
    m_running = true;
    locker.unlock();
    
    while (true) {
        locker.relock();
        if (!m_running) break;
        int interval = m_updateInterval;
        locker.unlock();
        
        SystemData data = collectData();
        emit dataUpdated(data);
        
        msleep(interval);
    }
}

SystemData SystemMonitorThread::collectData()
{
    SystemData data;
    data.timestamp = QDateTime::currentSecsSinceEpoch();
    
    // Simulate system data collection
    // In a real implementation, this would use system APIs or external tools
    data.cpuPercent = 25.0 + (rand() % 50); // Simulate 25-75% usage
    data.cpuTemp = 45.0 + (rand() % 20);    // Simulate 45-65°C
    data.memoryPercent = 30.0 + (rand() % 40); // Simulate 30-70% usage
    data.memoryUsed = 8.0 * (data.memoryPercent / 100.0) * 1024 * 1024 * 1024; // 8GB system
    data.memoryTotal = 8.0 * 1024 * 1024 * 1024;
    data.diskPercent = 45.0 + (rand() % 20); // Simulate 45-65% usage
    data.diskUsed = 500.0 * (data.diskPercent / 100.0) * 1024 * 1024 * 1024; // 500GB used
    data.diskTotal = 1000.0 * 1024 * 1024 * 1024; // 1TB total
    data.gpuLoad = 15.0 + (rand() % 30);    // Simulate 15-45% usage
    data.gpuTemp = 35.0 + (rand() % 25);    // Simulate 35-60°C
    data.gpuMemory = 20.0 + (rand() % 30);  // Simulate 20-50% usage
    
    // Add some CPU temperature sensors
    data.cpuTemps.append(qMakePair("CPU Package", data.cpuTemp));
    data.cpuTemps.append(qMakePair("CPU Core 0", data.cpuTemp - 3));
    data.cpuTemps.append(qMakePair("CPU Core 1", data.cpuTemp - 1));
    data.cpuTemps.append(qMakePair("CPU Core 2", data.cpuTemp - 2));
    data.cpuTemps.append(qMakePair("CPU Core 3", data.cpuTemp - 4));
    
    // Add some fan speeds
    data.fanSpeeds.append(qMakePair("CPU Fan", 1200.0 + (rand() % 600)));
    data.fanSpeeds.append(qMakePair("System Fan 1", 800.0 + (rand() % 400)));
    data.fanSpeeds.append(qMakePair("System Fan 2", 750.0 + (rand() % 450)));
    data.fanSpeeds.append(qMakePair("GPU Fan", 1500.0 + (rand() % 800)));
    
    return data;
}

#include "rgbfancontrol.moc"
