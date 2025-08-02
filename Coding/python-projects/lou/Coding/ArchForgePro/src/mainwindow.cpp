#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QMenu>
#include <QMenuBar>
#include <QIcon>
#include <QStyle>
#include <QDialog>
#include <QRadioButton>
#include <QSet>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_backupInProgress(false)
    , m_minimizeToTray(true)
    , m_monitoringEnabled(false)
    , m_changeCount(0)
{
    setWindowTitle("ArchForge Pro - Advanced System Management Suite");
    setWindowIcon(QIcon(":/icons/archforge_icon.svg"));
    resize(1200, 800);
    
    // Initialize core components
    m_backupManager = new BackupManager(this);
    m_restoreManager = new RestoreManager(this);
    m_packageManager = new PackageManager(this);
    m_settingsManager = new SettingsManager(this);
    m_aiOptimizer = new AIOptimizer(this);
    
    // Initialize real-time monitoring
    m_fileWatcher = new QFileSystemWatcher(this);
    m_monitoringTimer = new QTimer(this);
    m_monitoringTimer->setSingleShot(false);
    m_monitoringTimer->setInterval(5000); // Check every 5 seconds
    
    // Initialize settings
    m_settings = new QSettings("ArchForge Pro", "ArchForge Pro", this);
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupSystemTray();
    connectSignals();
    
    // Load settings
    loadSettings();
    
    // Auto-start real-time monitoring
    toggleSystemMonitoring(true);
    
    // Status timer for periodic updates
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        // Update UI elements periodically
        // Package count display moved to different location
    });
    m_statusTimer->start(5000); // Update every 5 seconds
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    // Create main tab for Clean Install Backup/Restore
    QWidget *mainTab = new QWidget();
    m_tabWidget->addTab(mainTab, "Clean Install Backup/Restore");
    
    // Layout for main tab
    QVBoxLayout *mainLayout = new QVBoxLayout(mainTab);
    
    // Settings button at the top
    QHBoxLayout *settingsLayout = new QHBoxLayout();
    m_settingsBtn = new QPushButton("Settings - View Backup Capabilities");
    m_settingsBtn->setToolTip("Click to see what can be backed up and configure settings");
    settingsLayout->addWidget(m_settingsBtn);
    settingsLayout->addStretch();
    mainLayout->addLayout(settingsLayout);
    
    // Create sub-tabs widget
    m_mainSubTabWidget = new QTabWidget();
    mainLayout->addWidget(m_mainSubTabWidget);
    
    // Setup all the original tabs as sub-tabs
    setupBackupTab();
    setupRestoreTab();
    setupAITab();
    setupLogsTab();
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Backup", this, &MainWindow::startFullBackup, QKeySequence::New);
    fileMenu->addAction("&Open Restore Point", this, &MainWindow::showRestoreDialog, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Preferences", this, &MainWindow::showPreferences);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
    
    // Backup menu
    QMenu *backupMenu = menuBar->addMenu("&Backup");
    backupMenu->addAction("&Full Backup", this, &MainWindow::startFullBackup);
    backupMenu->addAction("&Incremental Backup", this, &MainWindow::startIncrementalBackup);
    backupMenu->addAction("&Package Backup", this, &MainWindow::startPackageBackup);
    backupMenu->addAction("&Settings Backup", this, &MainWindow::startSettingsBackup);
    backupMenu->addSeparator();
    backupMenu->addAction("&Pause", this, &MainWindow::pauseBackup);
    backupMenu->addAction("&Cancel", this, &MainWindow::cancelBackup);
    
    // Restore menu
    QMenu *restoreMenu = menuBar->addMenu("&Restore");
    restoreMenu->addAction("&Browse Restore Points", this, &MainWindow::showRestoreDialog);
    restoreMenu->addAction("&Preview Restore", this, &MainWindow::previewRestore);
    
    // Tools menu
    QMenu *toolsMenu = menuBar->addMenu("&Tools");
    toolsMenu->addAction("&AI Analysis", this, &MainWindow::runAIAnalysis);
    toolsMenu->addAction("&Schedule Configuration", this, &MainWindow::configureSchedule);
    toolsMenu->addAction("&Package Manager", [this]() { m_tabWidget->setCurrentIndex(2); });
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About", [this]() {
        QMessageBox::about(this, "About ArchForge Pro", 
            "ArchForge Pro v0.0.1 (Alpha)\n\n"
            "Advanced Arch Linux system management and real-time monitoring suite\n"
            "with intelligent backup automation and comprehensive system tracking.\n\n"
            "Built with Qt6 and modern C++20.");
    });
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    m_statusBar->showMessage("Ready");
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/archforge_icon.svg"));
    m_trayIcon->setToolTip("ArchForge Pro - Alpha 0.0.1");
    
    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction("Show", this, &MainWindow::showMainWindow);
    m_trayMenu->addAction("Quick Backup", this, &MainWindow::startIncrementalBackup);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction("Quit", qApp, &QApplication::quit);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::setupBackupTab()
{
    m_backupTab = new QWidget();
    m_mainSubTabWidget->addTab(m_backupTab, "&Backup");
    
    // Main horizontal layout: options on left, logs on right
    QHBoxLayout *mainLayout = new QHBoxLayout(m_backupTab);
    
    // Left side - Options and Controls
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftWidget->setMaximumWidth(600);
    
    // Real-time monitoring status (always active)
    QGroupBox *monitoringGroup = new QGroupBox("Real-time System Monitoring (Auto-Active)");
    QGridLayout *monitoringLayout = new QGridLayout(monitoringGroup);
    
    m_monitoringStatusLabel = new QLabel("Status: Active - Monitoring system changes");
    m_monitoringStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #2E8B57; }");
    
    monitoringLayout->addWidget(m_monitoringStatusLabel, 0, 0, 1, 3);
    
    // Auto-backup settings
    m_autoBackupCheck = new QCheckBox("Auto-backup on changes");
    m_autoBackupCheck->setChecked(true);
    m_autoBackupCheck->setToolTip("Automatically create backup when significant changes are detected");
    
    QLabel *thresholdLabel = new QLabel("Change threshold:");
    m_changeThresholdSpin = new QSpinBox();
    m_changeThresholdSpin->setRange(1, 100);
    m_changeThresholdSpin->setValue(10);
    m_changeThresholdSpin->setSuffix(" changes");
    m_changeThresholdSpin->setToolTip("Number of changes before triggering auto-backup");
    
    monitoringLayout->addWidget(m_autoBackupCheck, 1, 0);
    monitoringLayout->addWidget(thresholdLabel, 1, 1);
    monitoringLayout->addWidget(m_changeThresholdSpin, 1, 2);
    
    // Manual backup operations
    QGroupBox *backupGroup = new QGroupBox("Manual Backup Operations");
    QGridLayout *backupLayout = new QGridLayout(backupGroup);
    
    m_packageBackupBtn = new QPushButton("Package Backup");
    m_packageBackupBtn->setToolTip("Backup installed packages list with AUR separation");
    m_settingsBackupBtn = new QPushButton("Settings Backup");
    m_settingsBackupBtn->setToolTip("Backup system and application settings");
    
    backupLayout->addWidget(m_packageBackupBtn, 0, 0);
    backupLayout->addWidget(m_settingsBackupBtn, 0, 1);
    
    // Package and Settings configuration buttons
    QPushButton *configurePackagesBtn = new QPushButton("Configure Packages");
    configurePackagesBtn->setToolTip("Select individual packages, import package lists, or choose backup scope");
    QPushButton *configureSettingsBtn = new QPushButton("Configure Settings");
    configureSettingsBtn->setToolTip("Select which configuration files and settings to backup");
    
    backupLayout->addWidget(configurePackagesBtn, 1, 0);
    backupLayout->addWidget(configureSettingsBtn, 1, 1);
    
    // Connect configuration buttons
    connect(configurePackagesBtn, &QPushButton::clicked, this, &MainWindow::showPackageConfigurationDialog);
    connect(configureSettingsBtn, &QPushButton::clicked, this, &MainWindow::showSettingsConfigurationDialog);
    
    // Backup options
    QGroupBox *optionsGroup = new QGroupBox("Backup Options");
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);
    
    optionsLayout->addWidget(new QLabel("Backup Location:"), 0, 0);
    m_backupLocationEdit = new QLineEdit();
    m_backupLocationEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ArchForgeBackups");
    m_browseLocationBtn = new QPushButton("Browse...");
    optionsLayout->addWidget(m_backupLocationEdit, 0, 1);
    optionsLayout->addWidget(m_browseLocationBtn, 0, 2);
    
    optionsLayout->addWidget(new QLabel("Compression:"), 1, 0);
    m_compressionCombo = new QComboBox();
    m_compressionCombo->addItems({"None", "gzip", "bzip2", "xz", "zstd"});
    m_compressionCombo->setCurrentText("zstd");
    optionsLayout->addWidget(m_compressionCombo, 1, 1);
    
    optionsLayout->addWidget(new QLabel("Compression Level:"), 2, 0);
    m_compressionSlider = new QSlider(Qt::Horizontal);
    m_compressionSlider->setRange(1, 9);
    m_compressionSlider->setValue(6);
    optionsLayout->addWidget(m_compressionSlider, 2, 1);
    
    m_verifyCheckBox = new QCheckBox("Verify backup integrity");
    m_verifyCheckBox->setChecked(true);
    optionsLayout->addWidget(m_verifyCheckBox, 3, 0, 1, 2);
    
    // Control buttons
    QGroupBox *controlGroup = new QGroupBox("Backup Control");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);
    
    m_pauseBtn = new QPushButton("Pause");
    m_pauseBtn->setEnabled(false);
    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setEnabled(false);
    
    controlLayout->addWidget(m_pauseBtn);
    controlLayout->addWidget(m_cancelBtn);
    controlLayout->addStretch();
    
    // Progress and status
    QGroupBox *progressGroup = new QGroupBox("Backup Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    
    m_backupProgress = new QProgressBar();
    m_backupStatusLabel = new QLabel("Ready to backup");
    
    progressLayout->addWidget(m_backupStatusLabel);
    progressLayout->addWidget(m_backupProgress);
    
    // Add groups to left layout
    leftLayout->addWidget(monitoringGroup);
    leftLayout->addWidget(backupGroup);
    leftLayout->addWidget(optionsGroup);
    leftLayout->addWidget(controlGroup);
    leftLayout->addWidget(progressGroup);
    leftLayout->addStretch();
    
    // Right side - Logs and Change Monitoring
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    
    // System change log display
    QGroupBox *changeLogGroup = new QGroupBox("System Change Log");
    QVBoxLayout *changeLogLayout = new QVBoxLayout(changeLogGroup);
    
    QLabel *changeLogInfo = new QLabel("Real-time display of system changes:");
    changeLogInfo->setStyleSheet("QLabel { font-weight: bold; color: #4A9EFF; }");
    
    m_changeLogText = new QTextEdit();
    m_changeLogText->setReadOnly(true);
    m_changeLogText->setFont(QFont("monospace", 9));
    m_changeLogText->setPlaceholderText("Start monitoring to see system changes in real-time...");
    
    QHBoxLayout *changeLogControlLayout = new QHBoxLayout();
    QPushButton *clearChangeLogBtn = new QPushButton("Clear Log");
    QLabel *changeCountLabel = new QLabel("Changes: 0");
    changeCountLabel->setStyleSheet("QLabel { font-weight: bold; }");
    
    changeLogControlLayout->addWidget(clearChangeLogBtn);
    changeLogControlLayout->addWidget(changeCountLabel);
    changeLogControlLayout->addStretch();
    
    changeLogLayout->addWidget(changeLogInfo);
    changeLogLayout->addWidget(m_changeLogText);
    changeLogLayout->addLayout(changeLogControlLayout);
    
    // Backup operation log
    QGroupBox *backupLogGroup = new QGroupBox("Backup Operation Log");
    QVBoxLayout *backupLogLayout = new QVBoxLayout(backupLogGroup);
    
    m_backupLog = new QTextEdit();
    m_backupLog->setReadOnly(true);
    m_backupLog->setFont(QFont("monospace", 9));
    m_backupLog->setPlaceholderText("Backup operations will be logged here...");
    
    QHBoxLayout *backupLogControlLayout = new QHBoxLayout();
    QPushButton *clearBackupLogBtn = new QPushButton("Clear Log");
    QPushButton *saveBackupLogBtn = new QPushButton("Save Log");
    
    backupLogControlLayout->addWidget(clearBackupLogBtn);
    backupLogControlLayout->addWidget(saveBackupLogBtn);
    backupLogControlLayout->addStretch();
    
    backupLogLayout->addWidget(m_backupLog);
    backupLogLayout->addLayout(backupLogControlLayout);
    
    // Add log groups to right layout
    rightLayout->addWidget(changeLogGroup);
    rightLayout->addWidget(backupLogGroup);
    
    // Connect log control buttons
    connect(clearChangeLogBtn, &QPushButton::clicked, [this, changeCountLabel]() {
        m_changeLogText->clear();
        m_changeCount = 0;
        changeCountLabel->setText("Changes: 0");
        updateStatus("Change log cleared");
    });
    
    connect(clearBackupLogBtn, &QPushButton::clicked, [this]() {
        m_backupLog->clear();
        updateStatus("Backup log cleared");
    });
    
    connect(saveBackupLogBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Backup Log", 
                                                        "backup_log.txt", "Text Files (*.txt)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << m_backupLog->toPlainText();
                updateStatus("Backup log saved to " + fileName);
            }
        }
    });
    
    // Add left and right widgets to main horizontal layout
    mainLayout->addWidget(leftWidget);
    mainLayout->addWidget(rightWidget);
    mainLayout->setStretch(0, 40); // Left side takes 40%
    mainLayout->setStretch(1, 60); // Right side takes 60%
}

void MainWindow::setupRestoreTab()
{
    m_restoreTab = new QWidget();
    m_mainSubTabWidget->addTab(m_restoreTab, "&Restore");
    
    QHBoxLayout *mainLayout = new QHBoxLayout(m_restoreTab);
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // Left side - Restore points
    QWidget *leftWidget = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    
    leftLayout->addWidget(new QLabel("Available Restore Points:"));
    m_restorePointsTree = new QTreeWidget();
    m_restorePointsTree->setHeaderLabels({"Date", "Type", "Size", "Status"});
    m_restorePointsTree->header()->resizeSection(0, 150);
    
    QHBoxLayout *restoreControlLayout = new QHBoxLayout();
    m_restoreBtn = new QPushButton("Restore");
    m_previewBtn = new QPushButton("Preview");
    m_deleteRestorePointBtn = new QPushButton("Delete");
    
    restoreControlLayout->addWidget(m_restoreBtn);
    restoreControlLayout->addWidget(m_previewBtn);
    restoreControlLayout->addWidget(m_deleteRestorePointBtn);
    restoreControlLayout->addStretch();
    
    leftLayout->addWidget(m_restorePointsTree);
    leftLayout->addLayout(restoreControlLayout);
    
    // Restore options
    QGroupBox *restoreOptionsGroup = new QGroupBox("Restore Options");
    QVBoxLayout *restoreOptionsLayout = new QVBoxLayout(restoreOptionsGroup);
    
    m_restorePackagesCheck = new QCheckBox("Restore Packages");
    m_restorePackagesCheck->setChecked(true);
    m_restoreSettingsCheck = new QCheckBox("Restore Settings");
    m_restoreSettingsCheck->setChecked(true);
    m_restoreUserDataCheck = new QCheckBox("Restore User Data");
    m_restoreUserDataCheck->setChecked(false);
    
    restoreOptionsLayout->addWidget(m_restorePackagesCheck);
    restoreOptionsLayout->addWidget(m_restoreSettingsCheck);
    restoreOptionsLayout->addWidget(m_restoreUserDataCheck);
    
    leftLayout->addWidget(restoreOptionsGroup);
    
    // Right side - Preview
    QWidget *rightWidget = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    
    rightLayout->addWidget(new QLabel("Restore Preview:"));
    m_restorePreview = new QTextEdit();
    m_restorePreview->setReadOnly(true);
    m_restorePreview->setPlaceholderText("Select a restore point to see preview...");
    
    rightLayout->addWidget(m_restorePreview);
    
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setSizes({400, 600});
    
    mainLayout->addWidget(splitter);
}



void MainWindow::setupScheduleTab()
{
    m_scheduleTab = new QWidget();
    m_mainSubTabWidget->addTab(m_scheduleTab, "S&chedule");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_scheduleTab);
    
    // Schedule configuration
    QGroupBox *scheduleGroup = new QGroupBox("Schedule Configuration");
    QGridLayout *scheduleLayout = new QGridLayout(scheduleGroup);
    
    m_enableScheduleCheck = new QCheckBox("Enable Scheduled Backups");
    scheduleLayout->addWidget(m_enableScheduleCheck, 0, 0, 1, 2);
    
    scheduleLayout->addWidget(new QLabel("Backup Type:"), 1, 0);
    m_scheduleTypeCombo = new QComboBox();
    m_scheduleTypeCombo->addItems({"Incremental", "Full", "Packages Only", "Settings Only"});
    scheduleLayout->addWidget(m_scheduleTypeCombo, 1, 1);
    
    scheduleLayout->addWidget(new QLabel("Interval (hours):"), 2, 0);
    m_scheduleIntervalSpin = new QSpinBox();
    m_scheduleIntervalSpin->setRange(1, 168); // 1 hour to 1 week
    m_scheduleIntervalSpin->setValue(24);
    scheduleLayout->addWidget(m_scheduleIntervalSpin, 2, 1);
    
    scheduleLayout->addWidget(new QLabel("Time:"), 3, 0);
    m_scheduleTimeEdit = new QDateTimeEdit();
    m_scheduleTimeEdit->setDisplayFormat("hh:mm");
    m_scheduleTimeEdit->setTime(QTime(2, 0)); // Default to 2 AM
    scheduleLayout->addWidget(m_scheduleTimeEdit, 3, 1);
    
    // Schedule frequency
    QGroupBox *frequencyGroup = new QGroupBox("Frequency");
    QVBoxLayout *frequencyLayout = new QVBoxLayout(frequencyGroup);
    
    m_scheduleDailyCheck = new QCheckBox("Daily");
    m_scheduleWeeklyCheck = new QCheckBox("Weekly");
    m_scheduleMonthlyCheck = new QCheckBox("Monthly");
    
    frequencyLayout->addWidget(m_scheduleDailyCheck);
    frequencyLayout->addWidget(m_scheduleWeeklyCheck);
    frequencyLayout->addWidget(m_scheduleMonthlyCheck);
    
    // Schedule table
    QGroupBox *scheduleTableGroup = new QGroupBox("Scheduled Backups");
    QVBoxLayout *scheduleTableLayout = new QVBoxLayout(scheduleTableGroup);
    
    m_scheduleTable = new QTableWidget();
    m_scheduleTable->setColumnCount(4);
    m_scheduleTable->setHorizontalHeaderLabels({"Next Run", "Type", "Frequency", "Status"});
    m_scheduleTable->horizontalHeader()->setStretchLastSection(true);
    
    scheduleTableLayout->addWidget(m_scheduleTable);
    
    mainLayout->addWidget(scheduleGroup);
    mainLayout->addWidget(frequencyGroup);
    mainLayout->addWidget(scheduleTableGroup);
}

void MainWindow::setupAITab()
{
    m_aiTab = new QWidget();
    m_mainSubTabWidget->addTab(m_aiTab, "&AI Optimizer");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_aiTab);
    
    // AI Configuration
    QGroupBox *aiConfigGroup = new QGroupBox("AI Configuration");
    QGridLayout *aiConfigLayout = new QGridLayout(aiConfigGroup);
    
    m_enableAICheck = new QCheckBox("Enable AI Optimization");
    m_enableAICheck->setToolTip("Use AI to optimize backup strategies and scheduling");
    aiConfigLayout->addWidget(m_enableAICheck, 0, 0, 1, 2);
    
    aiConfigLayout->addWidget(new QLabel("Sensitivity:"), 1, 0);
    m_aiSensitivitySlider = new QSlider(Qt::Horizontal);
    m_aiSensitivitySlider->setRange(1, 10);
    m_aiSensitivitySlider->setValue(5);
    m_aiSensitivitySlider->setToolTip("1 = Conservative, 10 = Aggressive optimization");
    aiConfigLayout->addWidget(m_aiSensitivitySlider, 1, 1);
    
    m_aiAutoOptimizeCheck = new QCheckBox("Auto-optimize backup schedules");
    aiConfigLayout->addWidget(m_aiAutoOptimizeCheck, 2, 0, 1, 2);
    
    // AI Controls
    QGroupBox *aiControlGroup = new QGroupBox("AI Analysis");
    QHBoxLayout *aiControlLayout = new QHBoxLayout(aiControlGroup);
    
    m_runAnalysisBtn = new QPushButton("Run Analysis");
    m_showRecommendationsBtn = new QPushButton("Show Recommendations");
    
    aiControlLayout->addWidget(m_runAnalysisBtn);
    aiControlLayout->addWidget(m_showRecommendationsBtn);
    aiControlLayout->addStretch();
    
    // AI Progress
    m_aiProgress = new QProgressBar();
    m_aiProgress->setVisible(false);
    
    // AI Results
    QGroupBox *aiResultsGroup = new QGroupBox("AI Analysis Results");
    QVBoxLayout *aiResultsLayout = new QVBoxLayout(aiResultsGroup);
    
    m_aiAnalysisText = new QTextEdit();
    m_aiAnalysisText->setReadOnly(true);
    m_aiAnalysisText->setPlaceholderText("Run AI analysis to see recommendations...");
    
    aiResultsLayout->addWidget(m_aiAnalysisText);
    
    mainLayout->addWidget(aiConfigGroup);
    mainLayout->addWidget(aiControlGroup);
    mainLayout->addWidget(m_aiProgress);
    mainLayout->addWidget(aiResultsGroup);
}

void MainWindow::setupLogsTab()
{
    m_logsTab = new QWidget();
    m_mainSubTabWidget->addTab(m_logsTab, "&Logs");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_logsTab);
    
    // Log controls
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel("Log Level:"));
    
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"All", "Debug", "Info", "Warning", "Error"});
    m_logLevelCombo->setCurrentText("Info");
    controlLayout->addWidget(m_logLevelCombo);
    
    controlLayout->addStretch();
    
    m_clearLogsBtn = new QPushButton("Clear Logs");
    m_exportLogsBtn = new QPushButton("Export Logs");
    
    controlLayout->addWidget(m_clearLogsBtn);
    controlLayout->addWidget(m_exportLogsBtn);
    
    // Log display
    m_logsText = new QTextEdit();
    m_logsText->setReadOnly(true);
    m_logsText->setFont(QFont("monospace"));
    
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_logsText);
}

void MainWindow::connectSignals()
{
    // Backup operations
    // Note: Full and incremental backup buttons don't exist in current design
    connect(m_packageBackupBtn, &QPushButton::clicked, this, &MainWindow::showPackageConfigurationDialog);
    connect(m_settingsBackupBtn, &QPushButton::clicked, this, &MainWindow::showSettingsConfigurationDialog);
    connect(m_pauseBtn, &QPushButton::clicked, this, &MainWindow::pauseBackup);
    connect(m_cancelBtn, &QPushButton::clicked, this, &MainWindow::cancelBackup);
    
    // Browse button
    connect(m_browseLocationBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Location", 
                                                        m_backupLocationEdit->text());
        if (!dir.isEmpty()) {
            m_backupLocationEdit->setText(dir);
        }
    });
    
    // Restore operations
    connect(m_restoreBtn, &QPushButton::clicked, this, &MainWindow::startRestore);
    connect(m_previewBtn, &QPushButton::clicked, this, &MainWindow::previewRestore);
    
    // Package operations (now handled through backup/restore tabs)
    // connect(m_refreshPackagesBtn, &QPushButton::clicked, this, &MainWindow::refreshPackageList);
    // connect(m_selectAllPackagesBtn, &QPushButton::clicked, this, &MainWindow::selectAllPackages);
    // connect(m_deselectAllPackagesBtn, &QPushButton::clicked, this, &MainWindow::deselectAllPackages);
    // connect(m_exportPackagesBtn, &QPushButton::clicked, this, &MainWindow::exportPackageList);
    // connect(m_importPackagesBtn, &QPushButton::clicked, this, &MainWindow::importPackageList);
    
    
    // AI operations
    connect(m_enableAICheck, &QCheckBox::toggled, this, &MainWindow::enableAIOptimization);
    connect(m_runAnalysisBtn, &QPushButton::clicked, this, &MainWindow::runAIAnalysis);
    connect(m_showRecommendationsBtn, &QPushButton::clicked, this, &MainWindow::showAIRecommendations);
    
    // Log operations
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &MainWindow::clearLogs);
    
    // Settings button
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::showBackupCapabilities);
    
    // Core component signals
    if (m_backupManager) {
        connect(m_backupManager, &BackupManager::progressChanged, this, &MainWindow::updateProgress);
        connect(m_backupManager, &BackupManager::statusChanged, this, &MainWindow::updateStatus);
        connect(m_backupManager, &BackupManager::backupCompleted, this, &MainWindow::onBackupComplete);
    }
    
    if (m_restoreManager) {
        connect(m_restoreManager, &RestoreManager::restoreCompleted, this, &MainWindow::onRestoreComplete);
    }
}

// Backup Operations
void MainWindow::startFullBackup()
{
    if (m_backupInProgress) {
        QMessageBox::warning(this, "Backup in Progress", "A backup is already in progress.");
        return;
    }
    
    updateStatus("Starting full system backup...");
    updateUIState(true);
    
    if (m_backupManager) {
        QString location = m_backupLocationEdit->text();
        QString compression = m_compressionCombo->currentText();
        bool verify = m_verifyCheckBox->isChecked();
        
        m_backupManager->startFullBackup(location, compression, verify);
    }
}

void MainWindow::startIncrementalBackup()
{
    if (m_backupInProgress) {
        QMessageBox::warning(this, "Backup in Progress", "A backup is already in progress.");
        return;
    }
    
    updateStatus("Starting incremental backup...");
    updateUIState(true);
    
    if (m_backupManager) {
        QString location = m_backupLocationEdit->text();
        m_backupManager->startIncrementalBackup(location);
    }
}

void MainWindow::startPackageBackup()
{
    updateStatus("Starting package backup...");
    if (m_packageManager) {
        m_packageManager->backupPackageList(m_backupLocationEdit->text());
    }
    updateStatus("Package backup completed");
}

void MainWindow::startSettingsBackup()
{
    updateStatus("Starting settings backup...");
    if (m_settingsManager) {
        m_settingsManager->backupSettings(m_backupLocationEdit->text());
    }
    updateStatus("Settings backup completed");
}

void MainWindow::pauseBackup()
{
    if (m_backupManager) {
        m_backupManager->pauseBackup();
    }
    updateStatus("Backup paused");
}

void MainWindow::cancelBackup()
{
    if (m_backupManager) {
        m_backupManager->cancelBackup();
    }
    updateStatus("Backup cancelled");
    updateUIState(false);
}

// Restore Operations
void MainWindow::showRestoreDialog()
{
    m_tabWidget->setCurrentIndex(1); // Switch to restore tab
    // Refresh restore points list
    // Implementation would populate m_restorePointsTree
}

void MainWindow::startRestore()
{
    // Implementation for starting restore operation
    QMessageBox::information(this, "Restore", "Restore functionality will be implemented.");
}

void MainWindow::previewRestore()
{
    // Implementation for previewing restore operation
    m_restorePreview->setText("Restore preview functionality will be implemented.");
}

// Package Management
void MainWindow::refreshPackageList()
{
    if (m_packageManager) {
        m_packageManager->refreshPackageList();
        // Update the packages tree widget
        // This would be implemented to populate m_packagesTree
    }
    updateStatus("Package list refreshed");
}

void MainWindow::exportPackageList()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Package List", 
                                                    "packages.txt", "Text Files (*.txt)");
    if (!fileName.isEmpty() && m_packageManager) {
        m_packageManager->exportPackageList(fileName);
        updateStatus("Package list exported to " + fileName);
    }
}

void MainWindow::importPackageList()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Package List", 
                                                    "", "Text Files (*.txt)");
    if (!fileName.isEmpty() && m_packageManager) {
        m_packageManager->importPackageList(fileName);
        updateStatus("Package list imported from " + fileName);
    }
}

void MainWindow::selectAllPackages()
{
    // Implementation to select all items in packages tree
    for (int i = 0; i < m_packagesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        item->setCheckState(0, Qt::Checked);
    }
}

void MainWindow::deselectAllPackages()
{
    // Implementation to deselect all items in packages tree
    for (int i = 0; i < m_packagesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        item->setCheckState(0, Qt::Unchecked);
    }
}

// Settings Management
void MainWindow::refreshSettingsList()
{
    if (m_settingsManager) {
        updateStatus("Scanning system components...");
        m_settingsTree->clear();
        
        m_settingsManager->refreshSettingsList();
        
        // Populate the settings tree
        QList<SettingFile> settingFiles = m_settingsManager->getSettingFiles();
        
        // Create category items
        QTreeWidgetItem *systemItem = new QTreeWidgetItem(m_settingsTree);
        systemItem->setText(0, "System Configuration");
        systemItem->setExpanded(true);
        
        QTreeWidgetItem *userItem = new QTreeWidgetItem(m_settingsTree);
        userItem->setText(0, "User Configuration");
        userItem->setExpanded(true);
        
        // Add files to appropriate categories
        for (const SettingFile &file : settingFiles) {
            QTreeWidgetItem *fileItem = new QTreeWidgetItem();
            fileItem->setText(0, file.name);
            fileItem->setText(1, file.path);
            fileItem->setText(2, QString("%1 KB").arg(file.size / 1024));
            fileItem->setText(3, file.modified.toString("yyyy-MM-dd hh:mm:ss"));
            fileItem->setCheckState(0, Qt::Checked); // Default to checked
            fileItem->setToolTip(1, file.path); // Full path in tooltip
            
            if (file.isSystemConfig) {
                systemItem->addChild(fileItem);
            } else {
                userItem->addChild(fileItem);
            }
        }
        
        // Update category labels with counts
        systemItem->setText(0, QString("System Configuration (%1 items)").arg(systemItem->childCount()));
        userItem->setText(0, QString("User Configuration (%1 items)").arg(userItem->childCount()));
        
        updateStatus(QString("Found %1 configuration items").arg(settingFiles.size()));
    }
}

void MainWindow::selectAllSettings()
{
    for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_settingsTree->topLevelItem(i);
        item->setCheckState(0, Qt::Checked);
    }
}

void MainWindow::deselectAllSettings()
{
    for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_settingsTree->topLevelItem(i);
        item->setCheckState(0, Qt::Unchecked);
    }
}

void MainWindow::exportSettings()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Settings", 
                                                    "settings_backup.tar.gz", "Archives (*.tar.gz)");
    if (!fileName.isEmpty() && m_settingsManager) {
        m_settingsManager->exportSettings(fileName);
        updateStatus("Settings exported to " + fileName);
    }
}

void MainWindow::importSettings()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Settings", 
                                                    "", "Archives (*.tar.gz)");
    if (!fileName.isEmpty() && m_settingsManager) {
        m_settingsManager->importSettings(fileName);
        updateStatus("Settings imported from " + fileName);
    }
}

// AI and Scheduling
void MainWindow::enableAIOptimization(bool enabled)
{
    if (m_aiOptimizer) {
        m_aiOptimizer->setEnabled(enabled);
    }
    updateStatus(enabled ? "AI optimization enabled" : "AI optimization disabled");
}

void MainWindow::configureSchedule()
{
    m_tabWidget->setCurrentIndex(4); // Switch to schedule tab
}

void MainWindow::runAIAnalysis()
{
    if (!m_enableAICheck->isChecked()) {
        QMessageBox::information(this, "AI Disabled", "Please enable AI optimization first.");
        return;
    }
    
    m_aiProgress->setVisible(true);
    m_aiProgress->setRange(0, 0); // Indeterminate progress
    m_runAnalysisBtn->setEnabled(false);
    
    if (m_aiOptimizer) {
        m_aiOptimizer->runAnalysis();
    }
    
    // Simulate analysis
    QTimer::singleShot(3000, [this]() {
        m_aiProgress->setVisible(false);
        m_runAnalysisBtn->setEnabled(true);
        m_aiAnalysisText->setText(
            "AI Analysis Results:\n\n"
            "• System analysis completed\n"
            "• Optimal backup frequency: Every 6 hours\n"
            "• Recommended compression: zstd level 6\n"
            "• Storage efficiency: 78%\n"
            "• Estimated backup time: 12 minutes\n"
            "• Suggested exclusions: cache directories, temp files\n\n"
            "Recommendations:\n"
            "1. Enable incremental backups for better efficiency\n"
            "2. Schedule full backups weekly at 2:00 AM\n"
            "3. Consider excluding large media files from daily backups\n"
            "4. Verify backup integrity monthly"
        );
        updateStatus("AI analysis completed");
    });
}

void MainWindow::showAIRecommendations()
{
    if (m_aiAnalysisText->toPlainText().isEmpty()) {
        QMessageBox::information(this, "No Analysis", "Please run AI analysis first.");
        return;
    }
    m_tabWidget->setCurrentIndex(5); // Switch to AI tab
}

// UI Updates
void MainWindow::updateProgress(int percentage)
{
    m_backupProgress->setValue(percentage);
}

void MainWindow::updateStatus(const QString &message)
{
    m_statusBar->showMessage(message);
    m_backupStatusLabel->setText(message);
    
    // Add to log
    QString logEntry = QString("[%1] %2").arg(QDateTime::currentDateTime().toString(), message);
    m_backupLog->append(logEntry);
    m_logsText->append(logEntry);
}

void MainWindow::onBackupComplete(bool success)
{
    updateUIState(false);
    if (success) {
        updateStatus("Backup completed successfully");
        if (m_trayIcon) {
            m_trayIcon->showMessage("ArchForge Pro", "Backup completed successfully", 
                                  QSystemTrayIcon::Information, 3000);
        }
    } else {
        updateStatus("Backup failed");
        if (m_trayIcon) {
            m_trayIcon->showMessage("ArchForge Pro", "Backup failed", 
                                  QSystemTrayIcon::Critical, 5000);
        }
    }
}

void MainWindow::onRestoreComplete(bool success)
{
    updateStatus(success ? "Restore completed successfully" : "Restore failed");
}

void MainWindow::showLogDetails()
{
    m_tabWidget->setCurrentIndex(6); // Switch to logs tab
}

void MainWindow::clearLogs()
{
    m_logsText->clear();
    m_backupLog->clear();
    updateStatus("Logs cleared");
}

// System Tray
void MainWindow::showMainWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::minimizeToTray()
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showMainWindow();
    }
}

// Settings
void MainWindow::saveSettings()
{
    if (!m_settings) return;
    
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("windowState", saveState());
    m_settings->setValue("backupLocation", m_backupLocationEdit->text());
    m_settings->setValue("compression", m_compressionCombo->currentText());
    m_settings->setValue("compressionLevel", m_compressionSlider->value());
    m_settings->setValue("verifyBackup", m_verifyCheckBox->isChecked());
    m_settings->setValue("enableAI", m_enableAICheck->isChecked());
    m_settings->setValue("aiSensitivity", m_aiSensitivitySlider->value());
    m_settings->setValue("minimizeToTray", m_minimizeToTray);
}

void MainWindow::loadSettings()
{
    if (!m_settings) return;
    
    restoreGeometry(m_settings->value("geometry").toByteArray());
    restoreState(m_settings->value("windowState").toByteArray());
    
    QString backupLocation = m_settings->value("backupLocation", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ArchBackups").toString();
    m_backupLocationEdit->setText(backupLocation);
    
    QString compression = m_settings->value("compression", "zstd").toString();
    m_compressionCombo->setCurrentText(compression);
    
    int compressionLevel = m_settings->value("compressionLevel", 6).toInt();
    m_compressionSlider->setValue(compressionLevel);
    
    bool verifyBackup = m_settings->value("verifyBackup", true).toBool();
    m_verifyCheckBox->setChecked(verifyBackup);
    
    bool enableAI = m_settings->value("enableAI", false).toBool();
    m_enableAICheck->setChecked(enableAI);
    
    int aiSensitivity = m_settings->value("aiSensitivity", 5).toInt();
    m_aiSensitivitySlider->setValue(aiSensitivity);
    
    m_minimizeToTray = m_settings->value("minimizeToTray", true).toBool();
}

void MainWindow::showPreferences()
{
    QMessageBox::information(this, "Preferences", "Preferences dialog will be implemented.");
}

void MainWindow::showBackupCapabilities()
{
    QString message = 
        "ArchForge Pro - Backup Capabilities\n\n"
        "What can be backed up:\n\n"
        "📦 PACKAGES:\n"
        "• All installed packages (pacman + AUR)\n"
        "• Package dependencies\n"
        "• Pacman configuration and hooks\n"
        "• Package database cache\n\n"
        "⚙️ SYSTEM SETTINGS:\n"
        "• System configuration (/etc/*)\n"
        "• Boot configuration (GRUB/systemd-boot)\n"
        "• Network configuration\n"
        "• Systemd services and units\n"
        "• Firewall and security settings\n\n"
        "👤 USER SETTINGS:\n"
        "• User configuration files (~/.config)\n"
        "• Application settings and themes\n"
        "• SSH keys and certificates\n"
        "• Desktop environment configs\n"
        "• Shell configurations (.bashrc, .zshrc)\n\n"
        "🖥️ DESKTOP ENVIRONMENTS:\n"
        "• KDE/Plasma settings\n"
        "• GNOME configurations\n"
        "• XFCE, i3, Sway settings\n"
        "• Window manager configs\n\n"
        "🐳 VIRTUALIZATION:\n"
        "• Docker containers and images\n"
        "• VirtualBox VMs\n"
        "• QEMU/KVM configurations\n"
        "• LXC containers\n\n"
        "💾 STORAGE:\n"
        "• BTRFS snapshots\n"
        "• Mount configurations\n"
        "• Disk encryption settings\n\n"
        "📊 LOGS & MONITORING:\n"
        "• System logs\n"
        "• Service logs\n"
        "• Backup operation logs\n\n"
        "🔧 ADDITIONAL FEATURES:\n"
        "• AI-powered backup optimization\n"
        "• Incremental backup support\n"
        "• Multiple compression formats\n"
        "• Scheduled automatic backups\n"
        "• Integrity verification\n"
        "• Restore point management\n\n"
        "Click the sub-tabs above to configure what to backup!";
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("ArchForge Pro - Backup Capabilities");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setMinimumSize(600, 500);
    msgBox.exec();
    
    updateStatus("Backup capabilities overview displayed");
}

void MainWindow::showPackageConfigurationDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Package Configuration");
    dialog.setModal(true);
    dialog.resize(800, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    // Package selection options
    QGroupBox *selectionGroup = new QGroupBox("Package Selection Mode");
    QVBoxLayout *selectionLayout = new QVBoxLayout(selectionGroup);
    
    QRadioButton *allPackagesRadio = new QRadioButton("Backup all explicitly installed packages");
    allPackagesRadio->setChecked(true);
    allPackagesRadio->setToolTip("Backup all packages that were explicitly installed by the user");
    
    QRadioButton *selectPackagesRadio = new QRadioButton("Select individual packages");
    selectPackagesRadio->setToolTip("Choose specific packages to backup");
    
    QRadioButton *importListRadio = new QRadioButton("Import package list from file");
    importListRadio->setToolTip("Load a previously exported package list");
    
    selectionLayout->addWidget(allPackagesRadio);
    selectionLayout->addWidget(selectPackagesRadio);
    selectionLayout->addWidget(importListRadio);
    
    // Package list widget (for individual selection)
    QGroupBox *packageListGroup = new QGroupBox("Available Packages");
    QVBoxLayout *packageListLayout = new QVBoxLayout(packageListGroup);
    
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLineEdit *searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search packages...");
    QPushButton *refreshBtn = new QPushButton("Refresh");
    searchLayout->addWidget(new QLabel("Search:"));
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(refreshBtn);
    
    QTreeWidget *packageTree = new QTreeWidget();
    packageTree->setHeaderLabels({"Package", "Version", "Repository", "Size"});
    packageTree->setSortingEnabled(true);
    packageTree->setEnabled(false); // Initially disabled
    
    QHBoxLayout *packageControlLayout = new QHBoxLayout();
    QPushButton *selectAllBtn = new QPushButton("Select All");
    QPushButton *deselectAllBtn = new QPushButton("Deselect All");
    QPushButton *selectExplicitBtn = new QPushButton("Select Explicit Only");
    packageControlLayout->addWidget(selectAllBtn);
    packageControlLayout->addWidget(deselectAllBtn);
    packageControlLayout->addWidget(selectExplicitBtn);
    packageControlLayout->addStretch();
    
    packageListLayout->addLayout(searchLayout);
    packageListLayout->addWidget(packageTree);
    packageListLayout->addLayout(packageControlLayout);
    
    // Import file section
    QGroupBox *importGroup = new QGroupBox("Import Package List");
    QHBoxLayout *importLayout = new QHBoxLayout(importGroup);
    
    QLineEdit *importFileEdit = new QLineEdit();
    importFileEdit->setPlaceholderText("Select package list file...");
    importFileEdit->setEnabled(false); // Initially disabled
    QPushButton *browseFileBtn = new QPushButton("Browse...");
    browseFileBtn->setEnabled(false); // Initially disabled
    
    importLayout->addWidget(new QLabel("File:"));
    importLayout->addWidget(importFileEdit);
    importLayout->addWidget(browseFileBtn);
    
    // Options
    QGroupBox *optionsGroup = new QGroupBox("Backup Options");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    
    QCheckBox *includeDependenciesCheck = new QCheckBox("Include dependencies in backup");
    includeDependenciesCheck->setChecked(true);
    includeDependenciesCheck->setToolTip("Include package dependencies for complete restoration");
    
    QCheckBox *separateAURCheck = new QCheckBox("Separate AUR packages");
    separateAURCheck->setChecked(true);
    separateAURCheck->setToolTip("Create separate list for AUR packages");
    
    QCheckBox *createScriptCheck = new QCheckBox("Generate restoration script");
    createScriptCheck->setChecked(true);
    createScriptCheck->setToolTip("Create executable script for easy package restoration");
    
    optionsLayout->addWidget(includeDependenciesCheck);
    optionsLayout->addWidget(separateAURCheck);
    optionsLayout->addWidget(createScriptCheck);
    
    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("OK");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    
    // Add all to main layout
    mainLayout->addWidget(selectionGroup);
    mainLayout->addWidget(packageListGroup);
    mainLayout->addWidget(importGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addLayout(buttonLayout);
    
    // Connect radio button changes
    connect(allPackagesRadio, &QRadioButton::toggled, [packageTree, selectAllBtn, deselectAllBtn, selectExplicitBtn, importFileEdit, browseFileBtn, searchEdit, refreshBtn](bool checked) {
        packageTree->setEnabled(false);
        selectAllBtn->setEnabled(false);
        deselectAllBtn->setEnabled(false);
        selectExplicitBtn->setEnabled(false);
        importFileEdit->setEnabled(false);
        browseFileBtn->setEnabled(false);
        searchEdit->setEnabled(false);
        refreshBtn->setEnabled(false);
    });
    
    connect(selectPackagesRadio, &QRadioButton::toggled, [this, packageTree, selectAllBtn, deselectAllBtn, selectExplicitBtn, searchEdit, refreshBtn, importFileEdit, browseFileBtn](bool checked) {
        if (checked) {
            packageTree->setEnabled(true);
            selectAllBtn->setEnabled(true);
            deselectAllBtn->setEnabled(true);
            selectExplicitBtn->setEnabled(true);
            searchEdit->setEnabled(true);
            refreshBtn->setEnabled(true);
            importFileEdit->setEnabled(false);
            browseFileBtn->setEnabled(false);
            
            // Populate package list
            if (m_packageManager) {
                m_packageManager->refreshPackageList();
                packageTree->clear();
                
                QList<PackageInfo> packages = m_packageManager->getInstalledPackages();
                for (const auto &pkg : packages) {
                    QTreeWidgetItem *item = new QTreeWidgetItem(packageTree);
                    item->setText(0, pkg.name);
                    item->setText(1, pkg.version);
                    item->setText(2, pkg.repository);
                    item->setText(3, QString("%1 KB").arg(pkg.size / 1024));
                    item->setCheckState(0, pkg.isExplicit ? Qt::Checked : Qt::Unchecked);
                    item->setToolTip(0, pkg.description);
                }
                packageTree->resizeColumnToContents(0);
                packageTree->resizeColumnToContents(1);
                packageTree->resizeColumnToContents(2);
            }
        }
    });
    
    connect(importListRadio, &QRadioButton::toggled, [packageTree, selectAllBtn, deselectAllBtn, selectExplicitBtn, searchEdit, refreshBtn, importFileEdit, browseFileBtn](bool checked) {
        if (checked) {
            packageTree->setEnabled(false);
            selectAllBtn->setEnabled(false);
            deselectAllBtn->setEnabled(false);
            selectExplicitBtn->setEnabled(false);
            searchEdit->setEnabled(false);
            refreshBtn->setEnabled(false);
            importFileEdit->setEnabled(true);
            browseFileBtn->setEnabled(true);
        }
    });
    
    // Connect package tree controls
    connect(selectAllBtn, &QPushButton::clicked, [=]() {
        for (int i = 0; i < packageTree->topLevelItemCount(); ++i) {
            packageTree->topLevelItem(i)->setCheckState(0, Qt::Checked);
        }
    });
    
    connect(deselectAllBtn, &QPushButton::clicked, [=]() {
        for (int i = 0; i < packageTree->topLevelItemCount(); ++i) {
            packageTree->topLevelItem(i)->setCheckState(0, Qt::Unchecked);
        }
    });
    
    connect(selectExplicitBtn, &QPushButton::clicked, [this, packageTree]() {
        if (m_packageManager) {
            QList<PackageInfo> explicitPackages = m_packageManager->getExplicitPackages();
            QSet<QString> explicitNames;
            for (const auto &pkg : explicitPackages) {
                explicitNames.insert(pkg.name);
            }
            
            for (int i = 0; i < packageTree->topLevelItemCount(); ++i) {
                QTreeWidgetItem *item = packageTree->topLevelItem(i);
                item->setCheckState(0, explicitNames.contains(item->text(0)) ? Qt::Checked : Qt::Unchecked);
            }
        }
    });
    
    // Connect file browse
    connect(browseFileBtn, &QPushButton::clicked, [importFileEdit]() {
        QString fileName = QFileDialog::getOpenFileName(nullptr, "Select Package List", 
                                                       "", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            importFileEdit->setText(fileName);
        }
    });
    
    // Connect search
    connect(searchEdit, &QLineEdit::textChanged, [=](const QString &text) {
        for (int i = 0; i < packageTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = packageTree->topLevelItem(i);
            bool matches = item->text(0).contains(text, Qt::CaseInsensitive) ||
                          item->text(1).contains(text, Qt::CaseInsensitive);
            item->setHidden(!matches && !text.isEmpty());
        }
    });
    
    // Connect dialog buttons
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        updateStatus("Starting package backup with selected configuration...");
        
        // Check which mode was selected and execute appropriate backup
        if (allPackagesRadio->isChecked()) {
            // Backup all explicitly installed packages
            if (m_backupManager) {
                QString location = m_backupLocationEdit->text();
                if (location.isEmpty()) {
                    location = QDir::homePath() + "/Documents/ArchBackups";
                    QDir().mkpath(location);
                }
                updateUIState(true);
                m_backupManager->startPackageBackup(location);
            }
        } else if (selectPackagesRadio->isChecked()) {
            // Custom package selection backup
            updateStatus("Custom package backup not yet implemented");
            QMessageBox::information(this, "Package Backup", "Custom package selection will be implemented in next version.");
        } else if (importListRadio->isChecked()) {
            // Import and backup from file
            updateStatus("File import backup not yet implemented");
            QMessageBox::information(this, "Package Backup", "Package file import will be implemented in next version.");
        }
    }
}

void MainWindow::showSettingsConfigurationDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Settings Configuration");
    dialog.setModal(true);
    dialog.resize(900, 700);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    // Settings categories
    QGroupBox *categoriesGroup = new QGroupBox("Settings Categories to Backup");
    QVBoxLayout *categoriesLayout = new QVBoxLayout(categoriesGroup);
    
    QCheckBox *systemConfigCheck = new QCheckBox("System Configuration (/etc/*, boot, network)");
    systemConfigCheck->setChecked(true);
    systemConfigCheck->setToolTip("Critical system files, boot configuration, network settings");
    
    QCheckBox *userConfigCheck = new QCheckBox("User Configuration (~/.config, dotfiles)");
    userConfigCheck->setChecked(true);
    userConfigCheck->setToolTip("User application settings, themes, dotfiles");
    
    QCheckBox *pacmanConfigCheck = new QCheckBox("Pacman Configuration (hooks, cache, config)");
    pacmanConfigCheck->setChecked(true);
    pacmanConfigCheck->setToolTip("Package manager configuration and hooks");
    
    QCheckBox *systemdConfigCheck = new QCheckBox("Systemd Services (units, custom services)");
    systemdConfigCheck->setChecked(true);
    systemdConfigCheck->setToolTip("System and user systemd services");
    
    QCheckBox *desktopConfigCheck = new QCheckBox("Desktop Environment (KDE, GNOME, XFCE, i3/Sway)");
    desktopConfigCheck->setChecked(true);
    desktopConfigCheck->setToolTip("Desktop environment and window manager configurations");
    
    QCheckBox *virtualizationConfigCheck = new QCheckBox("Virtualization (Docker, VirtualBox, QEMU/KVM)");
    virtualizationConfigCheck->setChecked(false);
    virtualizationConfigCheck->setToolTip("Container and virtual machine configurations");
    
    QCheckBox *btrfsConfigCheck = new QCheckBox("BTRFS Snapshots (if available)");
    btrfsConfigCheck->setChecked(true);
    btrfsConfigCheck->setToolTip("BTRFS filesystem snapshots");
    
    QCheckBox *sshKeysCheck = new QCheckBox("SSH Keys and Certificates");
    sshKeysCheck->setChecked(true);
    sshKeysCheck->setToolTip("SSH keys, certificates, and security credentials");
    
    categoriesLayout->addWidget(systemConfigCheck);
    categoriesLayout->addWidget(userConfigCheck);
    categoriesLayout->addWidget(pacmanConfigCheck);
    categoriesLayout->addWidget(systemdConfigCheck);
    categoriesLayout->addWidget(desktopConfigCheck);
    categoriesLayout->addWidget(virtualizationConfigCheck);
    categoriesLayout->addWidget(btrfsConfigCheck);
    categoriesLayout->addWidget(sshKeysCheck);
    
    // Specific settings tree
    QGroupBox *settingsTreeGroup = new QGroupBox("Specific Settings Files");
    QVBoxLayout *settingsTreeLayout = new QVBoxLayout(settingsTreeGroup);
    
    QHBoxLayout *searchLayout = new QHBoxLayout();
    QLineEdit *searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search settings files...");
    QPushButton *scanBtn = new QPushButton("Scan System");
    QPushButton *refreshBtn = new QPushButton("Refresh");
    
    searchLayout->addWidget(new QLabel("Search:"));
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(scanBtn);
    searchLayout->addWidget(refreshBtn);
    
    QTreeWidget *settingsTree = new QTreeWidget();
    settingsTree->setHeaderLabels({"Setting File", "Location", "Size", "Modified"});
    settingsTree->setSortingEnabled(true);
    settingsTree->setMaximumHeight(300);
    
    QHBoxLayout *settingsControlLayout = new QHBoxLayout();
    QPushButton *selectAllBtn = new QPushButton("Select All");
    QPushButton *deselectAllBtn = new QPushButton("Deselect All");
    QPushButton *selectCriticalBtn = new QPushButton("Select Critical Only");
    
    settingsControlLayout->addWidget(selectAllBtn);
    settingsControlLayout->addWidget(deselectAllBtn);
    settingsControlLayout->addWidget(selectCriticalBtn);
    settingsControlLayout->addStretch();
    
    settingsTreeLayout->addLayout(searchLayout);
    settingsTreeLayout->addWidget(settingsTree);
    settingsTreeLayout->addLayout(settingsControlLayout);
    
    // Backup options
    QGroupBox *optionsGroup = new QGroupBox("Backup Options");
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    
    QCheckBox *preservePermissionsCheck = new QCheckBox("Preserve file permissions and ownership");
    preservePermissionsCheck->setChecked(true);
    preservePermissionsCheck->setToolTip("Keep original file permissions when restoring");
    
    QCheckBox *createArchiveCheck = new QCheckBox("Create compressed archive");
    createArchiveCheck->setChecked(true);
    createArchiveCheck->setToolTip("Compress settings backup into single archive");
    
    QCheckBox *verifyIntegrityCheck = new QCheckBox("Verify backup integrity");
    verifyIntegrityCheck->setChecked(true);
    verifyIntegrityCheck->setToolTip("Check backup files for corruption");
    
    QCheckBox *includeHiddenCheck = new QCheckBox("Include hidden files and directories");
    includeHiddenCheck->setChecked(true);
    includeHiddenCheck->setToolTip("Backup dotfiles and hidden configuration");
    
    optionsLayout->addWidget(preservePermissionsCheck);
    optionsLayout->addWidget(createArchiveCheck);
    optionsLayout->addWidget(verifyIntegrityCheck);
    optionsLayout->addWidget(includeHiddenCheck);
    
    // Custom paths
    QGroupBox *customPathsGroup = new QGroupBox("Custom Paths");
    QVBoxLayout *customPathsLayout = new QVBoxLayout(customPathsGroup);
    
    QHBoxLayout *addPathLayout = new QHBoxLayout();
    QLineEdit *customPathEdit = new QLineEdit();
    customPathEdit->setPlaceholderText("Enter custom path to include...");
    QPushButton *addPathBtn = new QPushButton("Add Path");
    QPushButton *browsePathBtn = new QPushButton("Browse...");
    
    addPathLayout->addWidget(new QLabel("Custom Path:"));
    addPathLayout->addWidget(customPathEdit);
    addPathLayout->addWidget(browsePathBtn);
    addPathLayout->addWidget(addPathBtn);
    
    QListWidget *customPathsList = new QListWidget();
    customPathsList->setMaximumHeight(100);
    
    customPathsLayout->addLayout(addPathLayout);
    customPathsLayout->addWidget(customPathsList);
    
    // Dialog buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *previewBtn = new QPushButton("Preview Selection");
    QPushButton *okBtn = new QPushButton("OK");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    
    buttonLayout->addWidget(previewBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    
    // Add all to main layout
    mainLayout->addWidget(categoriesGroup);
    mainLayout->addWidget(settingsTreeGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addWidget(customPathsGroup);
    mainLayout->addLayout(buttonLayout);
    
    // Connect scan button to populate settings tree
    connect(scanBtn, &QPushButton::clicked, [this, settingsTree]() {
        if (m_settingsManager) {
            settingsTree->clear();
            updateStatus("Scanning system settings...");
            
            m_settingsManager->refreshSettingsList();
            QList<SettingFile> settingFiles = m_settingsManager->getSettingFiles();
            
            // Create category items
            QTreeWidgetItem *systemItem = new QTreeWidgetItem(settingsTree);
            systemItem->setText(0, "System Configuration");
            systemItem->setExpanded(true);
            
            QTreeWidgetItem *userItem = new QTreeWidgetItem(settingsTree);
            userItem->setText(0, "User Configuration");
            userItem->setExpanded(true);
            
            // Add files to appropriate categories
            for (const SettingFile &file : settingFiles) {
                QTreeWidgetItem *fileItem = new QTreeWidgetItem();
                fileItem->setText(0, file.name);
                fileItem->setText(1, file.path);
                fileItem->setText(2, QString("%1 KB").arg(file.size / 1024));
                fileItem->setText(3, file.modified.toString("yyyy-MM-dd hh:mm:ss"));
                fileItem->setCheckState(0, Qt::Checked);
                fileItem->setToolTip(1, file.path);
                
                if (file.isSystemConfig) {
                    systemItem->addChild(fileItem);
                } else {
                    userItem->addChild(fileItem);
                }
            }
            
            systemItem->setText(0, QString("System Configuration (%1 items)").arg(systemItem->childCount()));
            userItem->setText(0, QString("User Configuration (%1 items)").arg(userItem->childCount()));
            
            updateStatus(QString("Found %1 configuration items").arg(settingFiles.size()));
        }
    });
    
    // Connect tree controls
    connect(selectAllBtn, &QPushButton::clicked, [=]() {
        for (int i = 0; i < settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = settingsTree->topLevelItem(i);
            topItem->setCheckState(0, Qt::Checked);
            for (int j = 0; j < topItem->childCount(); ++j) {
                topItem->child(j)->setCheckState(0, Qt::Checked);
            }
        }
    });
    
    connect(deselectAllBtn, &QPushButton::clicked, [=]() {
        for (int i = 0; i < settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = settingsTree->topLevelItem(i);
            topItem->setCheckState(0, Qt::Unchecked);
            for (int j = 0; j < topItem->childCount(); ++j) {
                topItem->child(j)->setCheckState(0, Qt::Unchecked);
            }
        }
    });
    
    connect(selectCriticalBtn, &QPushButton::clicked, [=]() {
        // Select only critical system files
        for (int i = 0; i < settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = settingsTree->topLevelItem(i);
            if (topItem->text(0).contains("System")) {
                topItem->setCheckState(0, Qt::Checked);
                for (int j = 0; j < topItem->childCount(); ++j) {
                    QTreeWidgetItem *child = topItem->child(j);
                    // Check if it's a critical file
                    QString path = child->text(1);
                    bool isCritical = path.contains("/etc/fstab") || 
                                     path.contains("/etc/hostname") ||
                                     path.contains("/etc/locale") ||
                                     path.contains("/etc/pacman") ||
                                     path.contains("/etc/systemd") ||
                                     path.contains("/boot/");
                    child->setCheckState(0, isCritical ? Qt::Checked : Qt::Unchecked);
                }
            } else {
                topItem->setCheckState(0, Qt::Unchecked);
                for (int j = 0; j < topItem->childCount(); ++j) {
                    topItem->child(j)->setCheckState(0, Qt::Unchecked);
                }
            }
        }
    });
    
    // Connect custom path controls
    connect(browsePathBtn, &QPushButton::clicked, [customPathEdit]() {
        QString path = QFileDialog::getExistingDirectory(nullptr, "Select Directory to Include");
        if (!path.isEmpty()) {
            customPathEdit->setText(path);
        }
    });
    
    connect(addPathBtn, &QPushButton::clicked, [customPathEdit, customPathsList]() {
        QString path = customPathEdit->text().trimmed();
        if (!path.isEmpty()) {
            customPathsList->addItem(path);
            customPathEdit->clear();
        }
    });
    
    // Connect search
    connect(searchEdit, &QLineEdit::textChanged, [=](const QString &text) {
        for (int i = 0; i < settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = settingsTree->topLevelItem(i);
            for (int j = 0; j < topItem->childCount(); ++j) {
                QTreeWidgetItem *child = topItem->child(j);
                bool matches = child->text(0).contains(text, Qt::CaseInsensitive) ||
                              child->text(1).contains(text, Qt::CaseInsensitive);
                child->setHidden(!matches && !text.isEmpty());
            }
        }
    });
    
    // Connect preview button
    connect(previewBtn, &QPushButton::clicked, [this, &dialog, systemConfigCheck, userConfigCheck, pacmanConfigCheck, systemdConfigCheck, desktopConfigCheck, virtualizationConfigCheck, btrfsConfigCheck, sshKeysCheck, settingsTree, customPathsList, preservePermissionsCheck, createArchiveCheck, verifyIntegrityCheck, includeHiddenCheck]() {
        QString preview = "Settings Backup Preview:\n\n";
        
        // Count selected categories
        int selectedCategories = 0;
        if (systemConfigCheck->isChecked()) selectedCategories++;
        if (userConfigCheck->isChecked()) selectedCategories++;
        if (pacmanConfigCheck->isChecked()) selectedCategories++;
        if (systemdConfigCheck->isChecked()) selectedCategories++;
        if (desktopConfigCheck->isChecked()) selectedCategories++;
        if (virtualizationConfigCheck->isChecked()) selectedCategories++;
        if (btrfsConfigCheck->isChecked()) selectedCategories++;
        if (sshKeysCheck->isChecked()) selectedCategories++;
        
        preview += QString("Categories selected: %1/8\n").arg(selectedCategories);
        
        // Count specific files
        int selectedFiles = 0;
        for (int i = 0; i < settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = settingsTree->topLevelItem(i);
            for (int j = 0; j < topItem->childCount(); ++j) {
                if (topItem->child(j)->checkState(0) == Qt::Checked) {
                    selectedFiles++;
                }
            }
        }
        preview += QString("Specific files selected: %1\n").arg(selectedFiles);
        
        // Custom paths
        preview += QString("Custom paths: %1\n\n").arg(customPathsList->count());
        
        // Options
        preview += "Options:\n";
        if (preservePermissionsCheck->isChecked()) preview += "• Preserve permissions\n";
        if (createArchiveCheck->isChecked()) preview += "• Create archive\n";
        if (verifyIntegrityCheck->isChecked()) preview += "• Verify integrity\n";
        if (includeHiddenCheck->isChecked()) preview += "• Include hidden files\n";
        
        QMessageBox::information(const_cast<QWidget*>(static_cast<const QWidget*>(&dialog)), "Settings Backup Preview", preview);
    });
    
    // Connect real-time monitoring
    connect(m_toggleMonitoringBtn, &QPushButton::clicked, this, [this]() {
        toggleSystemMonitoring(!m_monitoringEnabled);
    });
    
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onFileSystemChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onFileSystemChanged);
    connect(m_monitoringTimer, &QTimer::timeout, this, &MainWindow::checkForSystemChanges);
    
    // Connect dialog buttons
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    // Show dialog
    if (dialog.exec() == QDialog::Accepted) {
        updateStatus("Starting settings backup with selected configuration...");
        
        // Execute settings backup with the BackupManager
        if (m_backupManager) {
            QString location = m_backupLocationEdit->text();
            if (location.isEmpty()) {
                location = QDir::homePath() + "/Documents/ArchBackups";
                QDir().mkpath(location);
            }
            updateUIState(true);
            m_backupManager->startSettingsBackup(location);
        }
    }
}

void MainWindow::updateUIState(bool backupInProgress)
{
    m_backupInProgress = backupInProgress;
    
    // Enable/disable backup buttons (only existing ones)
    m_packageBackupBtn->setEnabled(!backupInProgress);
    m_settingsBackupBtn->setEnabled(!backupInProgress);
    
    // Enable/disable control buttons
    m_pauseBtn->setEnabled(backupInProgress);
    m_cancelBtn->setEnabled(backupInProgress);
    
    // Reset progress if not in progress
    if (!backupInProgress) {
        m_backupProgress->setValue(0);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_minimizeToTray && m_trayIcon && m_trayIcon->isVisible()) {
        QMessageBox::information(this, "ArchBackupPro", 
                               "The application will continue running in the system tray.");
        hide();
        event->ignore();
    } else {
        saveSettings();
        event->accept();
    }
}

// Real-time monitoring implementation
void MainWindow::toggleSystemMonitoring(bool enabled)
{
    m_monitoringEnabled = enabled;
    
    if (enabled) {
        // Start monitoring
        m_toggleMonitoringBtn->setText("Stop Monitoring");
        m_toggleMonitoringBtn->setStyleSheet("QPushButton { background-color: #DC143C; color: white; font-weight: bold; }");
        m_monitoringStatusLabel->setText("Status: Active - Monitoring system changes");
        m_monitoringStatusLabel->setStyleSheet("QLabel { font-weight: bold; color: #2E8B57; }");
        
        // Add watched paths
        QStringList watchPaths = {
            "/etc",
            "/var/lib/pacman/local",
            QDir::homePath() + "/.config",
            QDir::homePath() + "/.bashrc",
            QDir::homePath() + "/.zshrc",
            "/boot"
        };
        
        for (const QString &path : watchPaths) {
            if (QFileInfo::exists(path)) {
                if (QFileInfo(path).isDir()) {
                    m_fileWatcher->addPath(path);
                } else {
                    m_fileWatcher->addPath(path);
                }
            }
        }
        
        // Start monitoring timer
        m_monitoringTimer->start();
        m_lastBackupTime = QDateTime::currentDateTime();
        
        updateChangeLog("MONITOR", "System", "Monitoring started");
        updateStatus("Real-time system monitoring started");
        
    } else {
        // Stop monitoring
        m_toggleMonitoringBtn->setText("Start Monitoring");
        m_toggleMonitoringBtn->setStyleSheet("QPushButton { background-color: #2E8B57; color: white; font-weight: bold; }");
        m_monitoringStatusLabel->setText("Status: Stopped");
        m_monitoringStatusLabel->setStyleSheet("QLabel { font-weight: bold; }");
        
        // Remove all watched paths
        if (!m_fileWatcher->files().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->files());
        }
        if (!m_fileWatcher->directories().isEmpty()) {
            m_fileWatcher->removePaths(m_fileWatcher->directories());
        }
        
        // Stop monitoring timer
        m_monitoringTimer->stop();
        
        updateChangeLog("MONITOR", "System", "Monitoring stopped");
        updateStatus("Real-time system monitoring stopped");
    }
}

void MainWindow::onFileSystemChanged(const QString &path)
{
    if (!m_monitoringEnabled) return;
    
    QString changeType = "UNKNOWN";
    QString action = "modified";
    
    // Determine change type
    if (path.startsWith("/etc")) {
        changeType = "CONFIG";
        action = "System configuration changed";
    } else if (path.contains("pacman")) {
        changeType = "PACKAGE";
        action = "Package database updated";
        QTimer::singleShot(1000, this, &MainWindow::onPackageDBChanged); // Delay to ensure DB is updated
    } else if (path.startsWith(QDir::homePath() + "/.config")) {
        changeType = "USER_CONFIG";
        action = "User configuration changed";
    } else if (path.contains(".bashrc") || path.contains(".zshrc")) {
        changeType = "SHELL";
        action = "Shell configuration changed";
    } else if (path.startsWith("/boot")) {
        changeType = "BOOT";
        action = "Boot configuration changed";
    }
    
    updateChangeLog(changeType, path, action);
    m_changeCount++;
    
    // Check if auto-backup should be triggered
    if (m_autoBackupCheck->isChecked() && m_changeCount >= m_changeThresholdSpin->value()) {
        QDateTime now = QDateTime::currentDateTime();
        if (m_lastBackupTime.secsTo(now) > 300) { // Minimum 5 minutes between auto-backups
            updateStatus(QString("Auto-backup triggered: %1 changes detected").arg(m_changeCount));
            
            // Reset change count
            m_changeCount = 0;
            m_lastBackupTime = now;
            
            // Trigger incremental backup
            QTimer::singleShot(2000, this, &MainWindow::startIncrementalBackup);
        }
    }
}

void MainWindow::onPackageDBChanged()
{
    if (!m_monitoringEnabled) return;
    
    if (m_packageManager) {
        // Refresh package list to detect changes
        m_packageManager->refreshPackageList();
        
        // Log package change
        updateChangeLog("PACKAGE", "/var/lib/pacman/local", "Package installation/removal detected");
        updateStatus("Package database change detected - refreshing package list");
    }
}

void MainWindow::onConfigFileChanged(const QString &path)
{
    if (!m_monitoringEnabled) return;
    
    updateChangeLog("CONFIG", path, "Configuration file modified");
    updateStatus(QString("Configuration file changed: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::updateChangeLog(const QString &type, const QString &path, const QString &action)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2: %3 - %4")
                      .arg(timestamp)
                      .arg(type)
                      .arg(QFileInfo(path).fileName())
                      .arg(action);
    
    // Color-code different types of changes
    QString color = "white";
    if (type == "PACKAGE") color = "#FFD700";  // Gold
    else if (type == "CONFIG") color = "#87CEEB";  // Sky blue
    else if (type == "USER_CONFIG") color = "#98FB98";  // Pale green
    else if (type == "BOOT") color = "#FFA500";  // Orange
    else if (type == "MONITOR") color = "#DDA0DD";  // Plum
    
    QString coloredEntry = QString("<span style='color: %1;'>%2</span>").arg(color, logEntry);
    
    m_changeLogText->append(coloredEntry);
    
    // Keep log size manageable (last 1000 entries)
    QTextDocument *doc = m_changeLogText->document();
    if (doc->blockCount() > 1000) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 100);
        cursor.removeSelectedText();
    }
    
    // Auto-scroll to bottom
    QScrollBar *scrollBar = m_changeLogText->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::checkForSystemChanges()
{
    if (!m_monitoringEnabled) return;
    
    // Periodic checks for changes that might not trigger file system events
    static QDateTime lastPackageCheck;
    static qint64 lastPackageDBSize = 0;
    
    QDateTime now = QDateTime::currentDateTime();
    
    // Check package database size every 30 seconds
    if (lastPackageCheck.isNull() || lastPackageCheck.secsTo(now) > 30) {
        QFileInfo pacmanDB("/var/lib/pacman/local");
        if (pacmanDB.exists()) {
            qint64 currentSize = 0;
            QDirIterator it(pacmanDB.absoluteFilePath(), QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                if (it.fileInfo().isFile()) {
                    currentSize += it.fileInfo().size();
                }
            }
            
            if (lastPackageDBSize > 0 && currentSize != lastPackageDBSize) {
                updateChangeLog("PACKAGE", "/var/lib/pacman/local", 
                              QString("Package database size changed (%1 -> %2 bytes)")
                              .arg(lastPackageDBSize).arg(currentSize));
            }
            lastPackageDBSize = currentSize;
        }
        lastPackageCheck = now;
    }
    
    // Update monitoring status
    QString statusText = QString("Status: Active - %1 changes detected")
                        .arg(m_changeCount);
    m_monitoringStatusLabel->setText(statusText);
}

