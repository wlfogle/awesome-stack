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
#include <QProcess>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_backupInProgress(false)
    , m_minimizeToTray(true)
{
    setWindowTitle("ArchBackupPro - Comprehensive Backup Solution");
    setWindowIcon(QIcon(":/icons/archforge_icon.svg"));
    resize(1200, 800);
    
    // Initialize core components
    m_backupManager = new BackupManager(this);
    m_restoreManager = new RestoreManager(this);
    m_packageManager = new PackageManager(this);
    m_settingsManager = new SettingsManager(this);
    
    // Initialize settings
    m_settings = new QSettings("ArchBackupPro", "ArchBackupPro", this);
    
    // Setup UI
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupSystemTray();
    connectSignals();
    
    // Load settings
    loadSettings();
    
    // Status timer for periodic updates
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        // Update UI elements periodically
        // Package count display moved to different location
    });
    m_statusTimer->start(5000); // Update every 5 seconds
    
    // Check and install monitoring daemon if needed
    checkAndInstallMonitoringDaemon();
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
    setupLogsTab();
    
    // Setup RGB/Fan Control tab
    m_rgbFanControl = new RGBFanControl(this);
    m_mainSubTabWidget->addTab(m_rgbFanControl, "🌈 RGB/Fan Control");
    
    // Connect RGB/Fan Control status messages
    connect(m_rgbFanControl, &RGBFanControl::statusMessage, this, &MainWindow::updateStatus);
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Package Backup", this, &MainWindow::startPackageBackup, QKeySequence::New);
    fileMenu->addAction("&Open Restore Point", this, &MainWindow::showRestoreDialog, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Preferences", this, &MainWindow::showPreferences);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);
    
    // Backup menu
    QMenu *backupMenu = menuBar->addMenu("&Backup");
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
    toolsMenu->addAction("&Real-time Monitoring", [this]() {
        updateStatus("Real-time monitoring runs automatically via systemd daemon");
        QMessageBox::information(this, "Real-time Monitoring", 
            "Real-time monitoring is handled by the archbackuppro-monitor systemd service.\n\n"
            "Service status: Use 'systemctl status archbackuppro-monitor' to check status\n"
            "View logs: Use 'journalctl -u archbackuppro-monitor' to view monitoring logs");
    });
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    helpMenu->addAction("&About", [this]() {
        QMessageBox::about(this, "About ArchBackupPro", 
            "ArchBackupPro v0.0.1 (Alpha)\n\n"
            "Comprehensive backup and restore solution for Arch Linux\n"
            "with AI-powered optimization and smart scheduling.\n\n"
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
    m_trayMenu->addAction("Package Backup", this, &MainWindow::startPackageBackup);
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
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_backupTab);
    
    // Backup type selection
    QGroupBox *typeGroup = new QGroupBox("Backup Operations");
    QGridLayout *typeLayout = new QGridLayout(typeGroup);
    
    m_packageBackupBtn = new QPushButton("Package Backup Options");
    m_packageBackupBtn->setToolTip("Configure package backup settings and selection");
    m_settingsBackupBtn = new QPushButton("Settings Backup Options");
    m_settingsBackupBtn->setToolTip("Configure settings backup categories and files");
    
    typeLayout->addWidget(m_packageBackupBtn, 0, 0);
    typeLayout->addWidget(m_settingsBackupBtn, 0, 1);
    
    // Backup options
    QGroupBox *optionsGroup = new QGroupBox("Backup Options");
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);
    
    optionsLayout->addWidget(new QLabel("Backup Location:"), 0, 0);
    m_backupLocationEdit = new QLineEdit();
    m_backupLocationEdit->setText(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ArchBackups");
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
    m_backupLog = new QTextEdit();
    m_backupLog->setMaximumHeight(150);
    m_backupLog->setReadOnly(true);
    
    progressLayout->addWidget(m_backupStatusLabel);
    progressLayout->addWidget(m_backupProgress);
    progressLayout->addWidget(new QLabel("Backup Log:"));
    progressLayout->addWidget(m_backupLog);
    
    // Package and Settings configuration buttons
    QGroupBox *configGroup = new QGroupBox("Package & Settings Configuration");
    QHBoxLayout *configLayout = new QHBoxLayout(configGroup);
    
    QPushButton *configurePackagesBtn = new QPushButton("Configure Packages");
    configurePackagesBtn->setToolTip("Select individual packages, import package lists, or choose backup scope");
    QPushButton *configureSettingsBtn = new QPushButton("Configure Settings");
    configureSettingsBtn->setToolTip("Select which configuration files and settings to backup");
    
    configLayout->addWidget(configurePackagesBtn);
    configLayout->addWidget(configureSettingsBtn);
    configLayout->addStretch();
    
    // Execute backup buttons
    QGroupBox *executeGroup = new QGroupBox("Execute Backup");
    QHBoxLayout *executeLayout = new QHBoxLayout(executeGroup);
    
    QPushButton *executePackageBackupBtn = new QPushButton("🚀 Start Package Backup");
    executePackageBackupBtn->setToolTip("Execute package backup with current settings");
    QPushButton *executeSettingsBackupBtn = new QPushButton("🚀 Start Settings Backup");
    executeSettingsBackupBtn->setToolTip("Execute settings backup with current configuration");
    
    executeLayout->addWidget(executePackageBackupBtn);
    executeLayout->addWidget(executeSettingsBackupBtn);
    executeLayout->addStretch();
    
    // Connect configuration buttons
    connect(configurePackagesBtn, &QPushButton::clicked, [this]() {
        showPackageConfigurationDialog();
    });
    
    connect(configureSettingsBtn, &QPushButton::clicked, [this]() {
        showSettingsConfigurationDialog();
    });
    
    // Connect execute buttons
    connect(executePackageBackupBtn, &QPushButton::clicked, [this]() {
        startPackageBackup();
    });
    
    connect(executeSettingsBackupBtn, &QPushButton::clicked, [this]() {
        startSettingsBackup();
    });
    
    // Add all groups to main layout
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addWidget(configGroup);
    mainLayout->addWidget(executeGroup);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(progressGroup);
}

void MainWindow::setupRestoreTab()
{
    m_restoreTab = new QWidget();
    m_mainSubTabWidget->addTab(m_restoreTab, "&Restore");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_restoreTab);
    
    // Simple header
    QLabel *headerLabel = new QLabel("📦 Package & Settings Restoration");
    QFont headerFont = headerLabel->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 2);
    headerLabel->setFont(headerFont);
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);
    
    QLabel *descLabel = new QLabel("Browse for backup archives and restore packages or settings to your system");
    descLabel->setStyleSheet("color: #666; font-style: italic;");
    descLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(descLabel);
    
    mainLayout->addSpacing(20);
    
    // Archive selection
    QGroupBox *archiveGroup = new QGroupBox("Select Backup Archive");
    QVBoxLayout *archiveLayout = new QVBoxLayout(archiveGroup);
    
    QHBoxLayout *fileLayout = new QHBoxLayout();
    m_archivePathEdit = new QLineEdit();
    m_archivePathEdit->setPlaceholderText("Choose a backup archive file (.tar.gz)");
    QPushButton *browseArchiveBtn = new QPushButton("Browse...");
    
    fileLayout->addWidget(new QLabel("Archive File:"));
    fileLayout->addWidget(m_archivePathEdit);
    fileLayout->addWidget(browseArchiveBtn);
    
    archiveLayout->addLayout(fileLayout);
    
    // Quick archive info display
    m_archiveInfoText = new QTextEdit();
    m_archiveInfoText->setMaximumHeight(100);
    m_archiveInfoText->setReadOnly(true);
    m_archiveInfoText->setPlaceholderText("Select an archive to see its contents...");
    archiveLayout->addWidget(new QLabel("Archive Contents:"));
    archiveLayout->addWidget(m_archiveInfoText);
    
    mainLayout->addWidget(archiveGroup);
    
    // Restoration options
    QGroupBox *restoreOptionsGroup = new QGroupBox("What to Restore");
    QVBoxLayout *restoreOptionsLayout = new QVBoxLayout(restoreOptionsGroup);
    
    m_restorePackagesCheck = new QCheckBox("📦 Restore Packages");
    m_restorePackagesCheck->setChecked(true);
    m_restorePackagesCheck->setToolTip("Install packages from backup using pacman and AUR helper");
    
    m_restoreSettingsCheck = new QCheckBox("⚙️ Restore Configuration Files");
    m_restoreSettingsCheck->setChecked(true);
    m_restoreSettingsCheck->setToolTip("Restore configuration files to their original locations");
    
    restoreOptionsLayout->addWidget(m_restorePackagesCheck);
    restoreOptionsLayout->addWidget(m_restoreSettingsCheck);
    
    mainLayout->addWidget(restoreOptionsGroup);
    
    // Action buttons
    QGroupBox *actionsGroup = new QGroupBox("Restore Actions");
    QHBoxLayout *actionsLayout = new QHBoxLayout(actionsGroup);
    
    m_restoreBtn = new QPushButton("🔄 Start Restoration");
    m_restoreBtn->setToolTip("Begin restoration process with selected options");
    m_previewBtn = new QPushButton("👁 Preview Restoration");
    m_previewBtn->setToolTip("Show what will be restored without making changes");
    
    actionsLayout->addWidget(m_restoreBtn);
    actionsLayout->addWidget(m_previewBtn);
    actionsLayout->addStretch();
    
    mainLayout->addWidget(actionsGroup);
    
    // Progress and logs
    QGroupBox *progressGroup = new QGroupBox("Restoration Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    
    m_restoreProgress = new QProgressBar();
    m_restoreStatusLabel = new QLabel("Ready to restore");
    m_restoreLog = new QTextEdit();
    m_restoreLog->setMaximumHeight(150);
    m_restoreLog->setReadOnly(true);
    
    progressLayout->addWidget(m_restoreStatusLabel);
    progressLayout->addWidget(m_restoreProgress);
    progressLayout->addWidget(new QLabel("Restoration Log:"));
    progressLayout->addWidget(m_restoreLog);
    
    mainLayout->addWidget(progressGroup);
    
    // Connect browse button functionality
    connect(browseArchiveBtn, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, 
            "Select Backup Archive", 
            m_backupLocationEdit->text(),
            "Archive Files (*.tar.gz *.tgz *.tar.bz2 *.tar.xz);;All Files (*)");
        
        if (!fileName.isEmpty()) {
            m_archivePathEdit->setText(fileName);
            
            // Try to read archive contents
            QProcess listProcess;
            listProcess.start("tar", QStringList() << "-tzf" << fileName);
            listProcess.waitForFinished(5000);
            
            if (listProcess.exitCode() == 0) {
                QString contents = listProcess.readAllStandardOutput();
                QStringList files = contents.split('\n', Qt::SkipEmptyParts);
                
                QString info = QString("Archive: %1\n").arg(QFileInfo(fileName).fileName());
                info += QString("Files: %1\n\n").arg(files.size());
                
                // Check for known backup files
                bool hasPackages = false;
                bool hasSettings = false;
                
                for (const QString &file : files) {
                    if (file.contains("installed_packages.txt") || file.contains("aur_packages.txt")) {
                        hasPackages = true;
                    }
                    if (file.contains("settings") || file.contains(".config") || file.contains("etc/")) {
                        hasSettings = true;
                    }
                }
                
                info += "Contents detected:\n";
                if (hasPackages) info += "✓ Package lists found\n";
                if (hasSettings) info += "✓ Configuration files found\n";
                if (!hasPackages && !hasSettings) info += "⚠ No recognized backup files found\n";
                
                m_archiveInfoText->setText(info);
                updateStatus("Archive loaded: " + QFileInfo(fileName).fileName());
            } else {
                m_archiveInfoText->setText("Error: Could not read archive contents");
                updateStatus("Failed to read archive");
            }
        }
    });
    
    // Connect restore button
    connect(m_restoreBtn, &QPushButton::clicked, [this]() {
        QString archivePath = m_archivePathEdit->text();
        if (archivePath.isEmpty()) {
            QMessageBox::warning(this, "No Archive Selected", "Please select a backup archive first.");
            return;
        }
        
        if (!QFile::exists(archivePath)) {
            QMessageBox::warning(this, "Archive Not Found", "The selected archive file does not exist.");
            return;
        }
        
        bool restorePackages = m_restorePackagesCheck->isChecked();
        bool restoreSettings = m_restoreSettingsCheck->isChecked();
        
        if (!restorePackages && !restoreSettings) {
            QMessageBox::warning(this, "Nothing Selected", "Please select what to restore (packages and/or settings).");
            return;
        }
        
        // Confirm restoration
        QString confirmMsg = "Are you sure you want to restore from:\n" + archivePath + "\n\n";
        if (restorePackages) confirmMsg += "• Packages will be installed\n";
        if (restoreSettings) confirmMsg += "• Configuration files will be restored\n";
        confirmMsg += "\nThis may overwrite existing files and install packages.";
        
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Restoration", confirmMsg);
        if (reply != QMessageBox::Yes) {
            return;
        }
        
        // Start restoration process
        m_restoreProgress->setValue(0);
        m_restoreStatusLabel->setText("Starting restoration...");
        m_restoreLog->clear();
        m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] Starting restoration from: " + archivePath);
        
        // Extract archive to temporary location
        QString tempDir = "/tmp/archbackuppro_restore_" + QString::number(QDateTime::currentSecsSinceEpoch());
        QDir().mkpath(tempDir);
        
        m_restoreProgress->setValue(10);
        m_restoreStatusLabel->setText("Extracting archive...");
        m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] Extracting to: " + tempDir);
        
        QProcess extractProcess;
        extractProcess.start("tar", QStringList() << "-xzf" << archivePath << "-C" << tempDir);
        extractProcess.waitForFinished(30000);
        
        if (extractProcess.exitCode() != 0) {
            m_restoreStatusLabel->setText("Failed to extract archive");
            m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] ERROR: Failed to extract archive");
            QMessageBox::critical(this, "Extraction Failed", "Could not extract the backup archive.");
            return;
        }
        
        m_restoreProgress->setValue(30);
        
        // Restore packages if requested
        if (restorePackages) {
            m_restoreStatusLabel->setText("Restoring packages...");
            m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] Restoring packages...");
            
            // Look for package files
            QStringList packageFiles;
            packageFiles << tempDir + "/installed_packages.txt";
            packageFiles << tempDir + "/aur_packages.txt";
            
            for (const QString &pkgFile : packageFiles) {
                if (QFile::exists(pkgFile)) {
                    QFile file(pkgFile);
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QTextStream in(&file);
                        QStringList packages;
                        while (!in.atEnd()) {
                            QString line = in.readLine().trimmed();
                            if (!line.isEmpty() && !line.startsWith("#")) {
                                // Extract package name (remove version info)
                                QString pkgName = line.split(' ').first();
                                packages << pkgName;
                            }
                        }
                        
                        if (!packages.isEmpty()) {
                            QString installer = pkgFile.contains("aur") ? "yay" : "pacman";
                            QStringList installCmd;
                            
                            if (installer == "pacman") {
                                installCmd << "sudo" << "pacman" << "-S" << "--needed" << "--noconfirm" << packages;
                            } else {
                                installCmd << "yay" << "-S" << "--needed" << "--noconfirm" << packages;
                            }
                            
                            m_restoreLog->append(QString("[%1] Installing %2 packages with %3...")
                                             .arg(QDateTime::currentDateTime().toString())
                                             .arg(packages.size())
                                             .arg(installer));
                            
                            QProcess installProcess;
                            installProcess.start(installCmd.first(), installCmd.mid(1));
                            installProcess.waitForFinished(300000); // 5 minute timeout
                            
                            if (installProcess.exitCode() == 0) {
                                m_restoreLog->append(QString("[%1] ✓ %2 packages installed successfully")
                                                 .arg(QDateTime::currentDateTime().toString())
                                                 .arg(installer));
                            } else {
                                m_restoreLog->append(QString("[%1] ⚠ Some %2 packages may have failed to install")
                                                 .arg(QDateTime::currentDateTime().toString())
                                                 .arg(installer));
                            }
                        }
                    }
                }
            }
            
            m_restoreProgress->setValue(60);
        }
        
        // Restore settings if requested
        if (restoreSettings) {
            m_restoreStatusLabel->setText("Restoring configuration files...");
            m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] Restoring configuration files...");
            
            // Look for settings archives or directories
            QStringList settingsFiles;
            settingsFiles << tempDir + "/settings_backup.tar.gz";
            settingsFiles << tempDir + "/user_settings.tar.gz";
            
            for (const QString &settingsFile : settingsFiles) {
                if (QFile::exists(settingsFile)) {
                    QProcess restoreProcess;
                    restoreProcess.start("tar", QStringList() << "-xzf" << settingsFile << "-C" << "/");
                    restoreProcess.waitForFinished(60000);
                    
                    if (restoreProcess.exitCode() == 0) {
                        m_restoreLog->append(QString("[%1] ✓ Restored: %2")
                                         .arg(QDateTime::currentDateTime().toString())
                                         .arg(QFileInfo(settingsFile).fileName()));
                    } else {
                        m_restoreLog->append(QString("[%1] ⚠ Failed to restore: %2")
                                         .arg(QDateTime::currentDateTime().toString())
                                         .arg(QFileInfo(settingsFile).fileName()));
                    }
                }
            }
            
            m_restoreProgress->setValue(90);
        }
        
        // Cleanup
        QProcess cleanupProcess;
        cleanupProcess.start("rm", QStringList() << "-rf" << tempDir);
        cleanupProcess.waitForFinished(5000);
        
        m_restoreProgress->setValue(100);
        m_restoreStatusLabel->setText("Restoration completed");
        m_restoreLog->append("[" + QDateTime::currentDateTime().toString() + "] ✓ Restoration process completed");
        
        QMessageBox::information(this, "Restoration Complete", 
            "Restoration process has been completed.\n\nPlease check the log for any warnings or errors.\n\nYou may need to reboot for some changes to take effect.");
        
        updateStatus("Restoration completed successfully");
    });
    
    // Connect preview button  
    connect(m_previewBtn, &QPushButton::clicked, [this]() {
        QString archivePath = m_archivePathEdit->text();
        if (archivePath.isEmpty()) {
            QMessageBox::warning(this, "No Archive Selected", "Please select a backup archive first.");
            return;
        }
        
        QProcess listProcess;
        listProcess.start("tar", QStringList() << "-tzf" << archivePath);
        listProcess.waitForFinished(10000);
        
        if (listProcess.exitCode() == 0) {
            QString contents = listProcess.readAllStandardOutput();
            
            QDialog previewDialog(this);
            previewDialog.setWindowTitle("Archive Preview - " + QFileInfo(archivePath).fileName());
            previewDialog.resize(600, 500);
            
            QVBoxLayout *layout = new QVBoxLayout(&previewDialog);
            QTextEdit *textEdit = new QTextEdit();
            textEdit->setPlainText(contents);
            textEdit->setReadOnly(true);
            textEdit->setFont(QFont("monospace"));
            
            QPushButton *closeBtn = new QPushButton("Close");
            connect(closeBtn, &QPushButton::clicked, &previewDialog, &QDialog::accept);
            
            layout->addWidget(new QLabel("Files in archive:"));
            layout->addWidget(textEdit);
            layout->addWidget(closeBtn);
            
            previewDialog.exec();
        } else {
            QMessageBox::warning(this, "Preview Failed", "Could not read archive contents.");
        }
    });
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
    
    // Restore operations - connections are handled in setupRestoreTab now
    
    // Package operations (now handled through backup/restore tabs)
    // connect(m_refreshPackagesBtn, &QPushButton::clicked, this, &MainWindow::refreshPackageList);
    // connect(m_selectAllPackagesBtn, &QPushButton::clicked, this, &MainWindow::selectAllPackages);
    // connect(m_deselectAllPackagesBtn, &QPushButton::clicked, this, &MainWindow::deselectAllPackages);
    // connect(m_exportPackagesBtn, &QPushButton::clicked, this, &MainWindow::exportPackageList);
    // connect(m_importPackagesBtn, &QPushButton::clicked, this, &MainWindow::importPackageList);
    
    
    // AI operations - removed
    
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
            m_trayIcon->showMessage("ArchBackupPro", "Backup completed successfully", 
                                  QSystemTrayIcon::Information, 3000);
        }
    } else {
        updateStatus("Backup failed");
        if (m_trayIcon) {
            m_trayIcon->showMessage("ArchBackupPro", "Backup failed", 
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
    
    // Enable/disable backup buttons
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

void MainWindow::checkAndInstallMonitoringDaemon()
{
    // Check if monitoring daemon is installed and running
    if (!isMonitoringDaemonInstalled()) {
        updateStatus("Real-time monitoring daemon not found, installing...");
        
        // Show installation dialog
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Install Monitoring Daemon", 
            "ArchBackupPro requires a real-time monitoring daemon for optimal functionality.\n\n"
            "This daemon will:\n"
            "• Monitor package changes\n"
            "• Track configuration file modifications\n"
            "• Monitor system resources\n"
            "• Suggest backup schedules\n\n"
            "Install monitoring daemon now?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            // Install the daemon using the install script
            QProcess installProcess;
            QString scriptPath = QCoreApplication::applicationDirPath() + "/../share/archbackuppro/install-monitor.sh";
            
            // If install script not found, try project directory
            if (!QFile::exists(scriptPath)) {
                scriptPath = QCoreApplication::applicationDirPath() + "/../../install-monitor.sh";
            }
            
            if (QFile::exists(scriptPath)) {
                updateStatus("Installing monitoring daemon (requires root privileges)...");
                
                // Run installation with pkexec for GUI sudo
                QStringList arguments;
                arguments << scriptPath;
                
                installProcess.start("pkexec", arguments);
                installProcess.waitForFinished(30000); // 30 second timeout
                
                if (installProcess.exitCode() == 0) {
                    updateStatus("Monitoring daemon installed successfully");
                    QMessageBox::information(this, "Installation Complete", 
                        "Real-time monitoring daemon has been installed and started.\n\n"
                        "The daemon will now monitor your system and provide\n"
                        "intelligent backup recommendations.");
                } else {
                    updateStatus("Failed to install monitoring daemon");
                    QMessageBox::warning(this, "Installation Failed", 
                        "Failed to install monitoring daemon. You can install it manually by running:\n\n"
                        "sudo " + scriptPath);
                }
            } else {
                updateStatus("Installation script not found");
                QMessageBox::warning(this, "Installation Error", 
                    "Monitoring daemon installation script not found.\n\n"
                    "Please ensure ArchBackupPro is properly installed.");
            }
        } else {
            updateStatus("Monitoring daemon installation skipped");
        }
    } else if (!isMonitoringDaemonRunning()) {
        updateStatus("Starting monitoring daemon...");
        
        // Try to start the daemon
        QProcess startProcess;
        startProcess.start("systemctl", QStringList() << "start" << "archbackuppro-monitor");
        startProcess.waitForFinished(5000);
        
        if (startProcess.exitCode() == 0) {
            updateStatus("Monitoring daemon started successfully");
        } else {
            updateStatus("Failed to start monitoring daemon");
        }
    } else {
        updateStatus("Real-time monitoring daemon is running");
    }
}

bool MainWindow::isMonitoringDaemonInstalled()
{
    // Check if the systemd service file exists
    return QFile::exists("/etc/systemd/system/archbackuppro-monitor.service") &&
           QFile::exists("/usr/local/bin/archbackuppro-monitor");
}

bool MainWindow::isMonitoringDaemonRunning()
{
    QProcess process;
    process.start("systemctl", QStringList() << "is-active" << "archbackuppro-monitor");
    process.waitForFinished(3000);
    
    QString output = process.readAllStandardOutput().trimmed();
    return output == "active";
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

