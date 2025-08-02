#include "cleaninstallbackuprestore_widget.h"
#include "backupmanager.h"
#include "restoremanager.h"
#include "packagemanager.h"
#include "settingsmanager.h"
#include "rgbfancontrol.h"
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
#include <QIcon>
#include <QStyle>
#include <QDialog>
#include <QRadioButton>
#include <QSet>
#include <QProcess>
#include <QCoreApplication>

CleanInstallBackupRestoreWidget::CleanInstallBackupRestoreWidget(QWidget *parent)
    : QWidget(parent)
    , m_backupInProgress(false)
{
    // Initialize ArchBackupPro core components
    m_backupManager = new BackupManager(this);
    m_restoreManager = new RestoreManager(this);
    m_packageManager = new PackageManager(this);
    m_settingsManager = new SettingsManager(this);
    
    // Initialize settings
    m_settings = new QSettings("ArchForgePro", "ArchBackupPro", this);
    
    // Setup UI with full ArchBackupPro functionality
    setupUI();
    setupConnections();
    
    // Status timer for periodic updates
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        // Update UI elements periodically
        updatePackageCount();
    });
    m_statusTimer->start(5000); // Update every 5 seconds
}

CleanInstallBackupRestoreWidget::~CleanInstallBackupRestoreWidget()
{
    if (m_settings) {
        // Save current settings
        m_settings->setValue("geometry", this->geometry());
        m_settings->setValue("compression_level", m_compressionSlider->value());
        m_settings->setValue("verify_backups", m_verifyCheckBox->isChecked());
        m_settings->setValue("backup_location", m_archBackupLocationEdit->text());
    }
}

void CleanInstallBackupRestoreWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Settings button at the top
    QHBoxLayout *settingsLayout = new QHBoxLayout();
    m_settingsBtn = new QPushButton("⚙️ Settings - View Backup Capabilities");
    m_settingsBtn->setToolTip("Click to see what can be backed up and configure settings");
    settingsLayout->addWidget(m_settingsBtn);
    settingsLayout->addStretch();
    mainLayout->addLayout(settingsLayout);
    
    // Create sub-tabs widget for full ArchBackupPro functionality
    m_mainSubTabWidget = new QTabWidget();
    mainLayout->addWidget(m_mainSubTabWidget);
    
    // Setup all ArchBackupPro tabs
    setupArchBackupTab();
    setupArchRestoreTab();
    setupPackagesTab();
    setupSettingsTab();
    setupArchLogsTab();
    
    // Setup RGB/Fan Control tab (from ArchBackupPro)
    m_rgbFanControl = new RGBFanControl(this);
    m_mainSubTabWidget->addTab(m_rgbFanControl, "🌈 RGB/Fan Control");
}

void CleanInstallBackupRestoreWidget::setupArchBackupTab()
{
    m_archBackupTab = new QWidget();
    m_mainSubTabWidget->addTab(m_archBackupTab, "💾 Backup");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_archBackupTab);
    
    // Backup type selection
    QGroupBox *typeGroup = new QGroupBox("Backup Operations");
    QGridLayout *typeLayout = new QGridLayout(typeGroup);
    
    m_packageBackupBtn = new QPushButton("📦 Package Backup Options");
    m_packageBackupBtn->setToolTip("Configure package backup settings and selection");
    m_settingsBackupBtn = new QPushButton("⚙️ Settings Backup Options");
    m_settingsBackupBtn->setToolTip("Configure settings backup categories and files");
    
    typeLayout->addWidget(m_packageBackupBtn, 0, 0);
    typeLayout->addWidget(m_settingsBackupBtn, 0, 1);
    
    // Backup location section
    QGroupBox *locationGroup = new QGroupBox("Backup Location");
    QHBoxLayout *locationLayout = new QHBoxLayout(locationGroup);
    
    m_archBackupLocationEdit = new QLineEdit();
    m_archBackupLocationEdit->setPlaceholderText("Select backup destination...");
    m_browseLocationBtn = new QPushButton("Browse...");
    
    locationLayout->addWidget(new QLabel("Location:"));
    locationLayout->addWidget(m_archBackupLocationEdit);
    locationLayout->addWidget(m_browseLocationBtn);
    
    // Compression settings
    QGroupBox *compressionGroup = new QGroupBox("Compression Settings");
    QGridLayout *compressionLayout = new QGridLayout(compressionGroup);
    
    m_compressionCombo = new QComboBox();
    m_compressionCombo->addItems({"zstd (Recommended)", "gzip", "bzip2", "xz", "none"});
    
    m_compressionSlider = new QSlider(Qt::Horizontal);
    m_compressionSlider->setRange(1, 9);
    m_compressionSlider->setValue(3);
    
    m_verifyCheckBox = new QCheckBox("Verify backup integrity");
    m_verifyCheckBox->setChecked(true);
    
    compressionLayout->addWidget(new QLabel("Method:"), 0, 0);
    compressionLayout->addWidget(m_compressionCombo, 0, 1);
    compressionLayout->addWidget(new QLabel("Level:"), 1, 0);
    compressionLayout->addWidget(m_compressionSlider, 1, 1);
    compressionLayout->addWidget(m_verifyCheckBox, 2, 0, 1, 2);
    
    // Control buttons
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    QPushButton *fullBackupBtn = new QPushButton("🗃️ Full Backup");
    QPushButton *packageOnlyBtn = new QPushButton("📦 Package Backup");
    QPushButton *settingsOnlyBtn = new QPushButton("⚙️ Settings Backup");
    m_pauseBtn = new QPushButton("⏸️ Pause");
    m_cancelBtn = new QPushButton("❌ Cancel");
    
    m_pauseBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    
    controlLayout->addWidget(fullBackupBtn);
    controlLayout->addWidget(packageOnlyBtn);
    controlLayout->addWidget(settingsOnlyBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(m_pauseBtn);
    controlLayout->addWidget(m_cancelBtn);
    
    // Progress section
    QGroupBox *progressGroup = new QGroupBox("Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    
    m_archBackupProgress = new QProgressBar();
    m_archBackupStatusLabel = new QLabel("Ready");
    m_archBackupLog = new QTextEdit();
    m_archBackupLog->setMaximumHeight(150);
    m_archBackupLog->setPlaceholderText("Backup progress and status messages will appear here...");
    
    progressLayout->addWidget(m_archBackupProgress);
    progressLayout->addWidget(m_archBackupStatusLabel);
    progressLayout->addWidget(new QLabel("Log:"));
    progressLayout->addWidget(m_archBackupLog);
    
    // Add all groups to main layout
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(locationGroup);
    mainLayout->addWidget(compressionGroup);
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(progressGroup);
}

void CleanInstallBackupRestoreWidget::setupArchRestoreTab()
{
    m_archRestoreTab = new QWidget();
    m_mainSubTabWidget->addTab(m_archRestoreTab, "🔄 Restore");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_archRestoreTab);
    
    // Restore source selection
    QGroupBox *sourceGroup = new QGroupBox("Restore Source");
    QVBoxLayout *sourceLayout = new QVBoxLayout(sourceGroup);
    
    QHBoxLayout *archiveLayout = new QHBoxLayout();
    m_archivePathEdit = new QLineEdit();
    m_archivePathEdit->setPlaceholderText("Select backup archive...");
    QPushButton *browseArchiveBtn = new QPushButton("Browse Archive...");
    
    archiveLayout->addWidget(new QLabel("Archive:"));
    archiveLayout->addWidget(m_archivePathEdit);
    archiveLayout->addWidget(browseArchiveBtn);
    
    // Available restore points
    m_restorePointsTree = new QTreeWidget();
    m_restorePointsTree->setHeaderLabels({"Backup Name", "Date", "Type", "Size"});
    m_restorePointsTree->setAlternatingRowColors(true);
    
    sourceLayout->addLayout(archiveLayout);
    sourceLayout->addWidget(new QLabel("Available Restore Points:"));
    sourceLayout->addWidget(m_restorePointsTree);
    
    // Restore options
    QGroupBox *optionsGroup = new QGroupBox("Restore Options");
    QGridLayout *optionsLayout = new QGridLayout(optionsGroup);
    
    m_restorePackagesCheck = new QCheckBox("Restore Packages");
    m_restoreSettingsCheck = new QCheckBox("Restore Settings");
    m_restoreUserDataCheck = new QCheckBox("Restore User Data");
    
    m_restorePackagesCheck->setChecked(true);
    m_restoreSettingsCheck->setChecked(true);
    
    optionsLayout->addWidget(m_restorePackagesCheck, 0, 0);
    optionsLayout->addWidget(m_restoreSettingsCheck, 0, 1);
    optionsLayout->addWidget(m_restoreUserDataCheck, 1, 0, 1, 2);
    
    // Preview and control
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    m_archPreviewBtn = new QPushButton("👁️ Preview Restore");
    m_archRestoreBtn = new QPushButton("🔄 Start Restore");
    m_deleteRestorePointBtn = new QPushButton("🗑️ Delete Restore Point");
    
    m_archPreviewBtn->setEnabled(false);
    m_archRestoreBtn->setEnabled(false);
    m_deleteRestorePointBtn->setEnabled(false);
    
    controlLayout->addWidget(m_archPreviewBtn);
    controlLayout->addWidget(m_archRestoreBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(m_deleteRestorePointBtn);
    
    // Preview and progress
    QSplitter *splitter = new QSplitter(Qt::Vertical);
    
    QGroupBox *previewGroup = new QGroupBox("Restore Preview");
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    m_restorePreview = new QTextEdit();
    m_restorePreview->setPlaceholderText("Select a restore point to preview contents...");
    previewLayout->addWidget(m_restorePreview);
    
    QGroupBox *progressGroup = new QGroupBox("Restore Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    m_archRestoreProgress = new QProgressBar();
    m_archRestoreStatusLabel = new QLabel("Ready");
    m_archRestoreLog = new QTextEdit();
    m_archRestoreLog->setMaximumHeight(100);
    
    progressLayout->addWidget(m_archRestoreProgress);
    progressLayout->addWidget(m_archRestoreStatusLabel);
    progressLayout->addWidget(m_archRestoreLog);
    
    splitter->addWidget(previewGroup);
    splitter->addWidget(progressGroup);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    // Add to main layout
    mainLayout->addWidget(sourceGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(splitter);
}

void CleanInstallBackupRestoreWidget::setupPackagesTab()
{
    m_packagesTab = new QWidget();
    m_mainSubTabWidget->addTab(m_packagesTab, "📦 Packages");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_packagesTab);
    
    // Search and controls
    QHBoxLayout *searchLayout = new QHBoxLayout();
    
    m_packageSearchEdit = new QLineEdit();
    m_packageSearchEdit->setPlaceholderText("Search packages...");
    
    m_refreshPackagesBtn = new QPushButton("🔄 Refresh");
    m_selectAllPackagesBtn = new QPushButton("☑️ Select All");
    m_deselectAllPackagesBtn = new QPushButton("☐ Deselect All");
    
    searchLayout->addWidget(new QLabel("Search:"));
    searchLayout->addWidget(m_packageSearchEdit);
    searchLayout->addWidget(m_refreshPackagesBtn);
    searchLayout->addWidget(m_selectAllPackagesBtn);
    searchLayout->addWidget(m_deselectAllPackagesBtn);
    
    // Package list
    m_packagesTree = new QTreeWidget();
    m_packagesTree->setHeaderLabels({"Package", "Version", "Size", "Description"});
    m_packagesTree->setAlternatingRowColors(true);
    m_packagesTree->setSelectionMode(QAbstractItemView::MultiSelection);
    
    // Export/Import controls
    QHBoxLayout *fileLayout = new QHBoxLayout();
    
    m_exportPackagesBtn = new QPushButton("📤 Export List");
    m_importPackagesBtn = new QPushButton("📥 Import List");
    m_packageCountLabel = new QLabel("Packages: 0");
    
    fileLayout->addWidget(m_exportPackagesBtn);
    fileLayout->addWidget(m_importPackagesBtn);
    fileLayout->addStretch();
    fileLayout->addWidget(m_packageCountLabel);
    
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_packagesTree);
    mainLayout->addLayout(fileLayout);
}

void CleanInstallBackupRestoreWidget::setupSettingsTab()
{
    m_settingsTab = new QWidget();
    m_mainSubTabWidget->addTab(m_settingsTab, "⚙️ Settings");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_settingsTab);
    
    // Search and controls
    QHBoxLayout *searchLayout = new QHBoxLayout();
    
    m_settingsSearchEdit = new QLineEdit();
    m_settingsSearchEdit->setPlaceholderText("Search settings...");
    
    m_refreshSettingsBtn = new QPushButton("🔄 Refresh");
    m_selectAllSettingsBtn = new QPushButton("☑️ Select All");
    m_deselectAllSettingsBtn = new QPushButton("☐ Deselect All");
    
    searchLayout->addWidget(new QLabel("Search:"));
    searchLayout->addWidget(m_settingsSearchEdit);
    searchLayout->addWidget(m_refreshSettingsBtn);
    searchLayout->addWidget(m_selectAllSettingsBtn);
    searchLayout->addWidget(m_deselectAllSettingsBtn);
    
    // Settings tree
    m_settingsTree = new QTreeWidget();
    m_settingsTree->setHeaderLabels({"Setting Category", "Path", "Size"});
    m_settingsTree->setAlternatingRowColors(true);
    m_settingsTree->setSelectionMode(QAbstractItemView::MultiSelection);
    
    // Export/Import controls
    QHBoxLayout *fileLayout = new QHBoxLayout();
    
    m_exportSettingsBtn = new QPushButton("📤 Export Settings");
    m_importSettingsBtn = new QPushButton("📥 Import Settings");
    
    fileLayout->addWidget(m_exportSettingsBtn);
    fileLayout->addWidget(m_importSettingsBtn);
    fileLayout->addStretch();
    
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_settingsTree);
    mainLayout->addLayout(fileLayout);
}

void CleanInstallBackupRestoreWidget::setupArchLogsTab()
{
    m_archLogsTab = new QWidget();
    m_mainSubTabWidget->addTab(m_archLogsTab, "📋 Logs");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_archLogsTab);
    
    // Log controls
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"All Logs", "Info", "Warning", "Error", "Debug"});
    
    m_clearArchLogsBtn = new QPushButton("🧹 Clear Logs");
    m_exportArchLogsBtn = new QPushButton("📤 Export Logs");
    QPushButton *refreshLogsBtn = new QPushButton("🔄 Refresh");
    
    controlLayout->addWidget(new QLabel("Filter:"));
    controlLayout->addWidget(m_logLevelCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(refreshLogsBtn);
    controlLayout->addWidget(m_clearArchLogsBtn);
    controlLayout->addWidget(m_exportArchLogsBtn);
    
    // Logs display
    m_logsText = new QTextEdit();
    m_logsText->setReadOnly(true);
    m_logsText->setPlaceholderText("ArchBackupPro logs will appear here...");
    
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_logsText);
}

void CleanInstallBackupRestoreWidget::setupConnections()
{
    // Settings button
    connect(m_settingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showBackupCapabilities);
    
    // Backup connections
    connect(m_packageBackupBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showPackageConfigurationDialog);
    connect(m_settingsBackupBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showSettingsConfigurationDialog);
    connect(m_browseLocationBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Location");
        if (!dir.isEmpty()) {
            m_archBackupLocationEdit->setText(dir);
        }
    });
    
    // Package management connections
    connect(m_refreshPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::refreshPackageList);
    connect(m_selectAllPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::selectAllPackages);
    connect(m_deselectAllPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::deselectAllPackages);
    connect(m_exportPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::exportPackageList);
    connect(m_importPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::importPackageList);
    
    // Settings management connections
    connect(m_refreshSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::refreshSettingsList);
    connect(m_selectAllSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::selectAllSettings);
    connect(m_deselectAllSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::deselectAllSettings);
    connect(m_exportSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::exportSettings);
    connect(m_importSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::importSettings);
    
    // Logs connections
    connect(m_clearArchLogsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::clearLogs);
    connect(m_exportArchLogsBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this, "Export Logs", "archbackuppro_logs.txt", "Text Files (*.txt)");
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << m_logsText->toPlainText();
                updateStatus("Logs exported successfully");
            }
        }
    });
    
    // BackupManager connections
    connect(m_backupManager, &BackupManager::progressChanged, this, &CleanInstallBackupRestoreWidget::updateProgress);
    connect(m_backupManager, &BackupManager::statusChanged, this, &CleanInstallBackupRestoreWidget::updateStatus);
    connect(m_backupManager, &BackupManager::backupCompleted, this, &CleanInstallBackupRestoreWidget::onBackupComplete);
    
    // RestoreManager connections
    connect(m_restoreManager, &RestoreManager::restoreCompleted, this, &CleanInstallBackupRestoreWidget::onRestoreComplete);
    
    // RGB/Fan Control status messages
    if (m_rgbFanControl) {
        connect(m_rgbFanControl, &RGBFanControl::statusMessage, this, &CleanInstallBackupRestoreWidget::updateStatus);
    }
}

// ArchBackupPro Slot Implementations
void CleanInstallBackupRestoreWidget::startPackageBackup()
{
    QString location = m_archBackupLocationEdit->text();
    if (location.isEmpty()) {
        QMessageBox::warning(this, "Backup Location", "Please select a backup location first.");
        return;
    }
    
    updateStatus("Starting package backup...");
    m_backupInProgress = true;
    m_pauseBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);
    
    m_backupManager->startPackageBackup(location);
}

void CleanInstallBackupRestoreWidget::startSettingsBackup()
{
    QString location = m_archBackupLocationEdit->text();
    if (location.isEmpty()) {
        QMessageBox::warning(this, "Backup Location", "Please select a backup location first.");
        return;
    }
    
    updateStatus("Starting settings backup...");
    m_backupInProgress = true;
    m_pauseBtn->setEnabled(true);
    m_cancelBtn->setEnabled(true);
    
    m_backupManager->startSettingsBackup(location);
}

void CleanInstallBackupRestoreWidget::pauseBackup()
{
    m_backupManager->pauseBackup();
    updateStatus("Backup paused");
}

void CleanInstallBackupRestoreWidget::cancelBackup()
{
    m_backupManager->cancelBackup();
    updateStatus("Backup cancelled");
    m_backupInProgress = false;
    m_pauseBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
}

void CleanInstallBackupRestoreWidget::showRestoreDialog()
{
    // Implementation for restore dialog
    QMessageBox::information(this, "Restore", "Restore dialog functionality to be implemented");
}

void CleanInstallBackupRestoreWidget::startRestore()
{
    updateStatus("Starting restore operation...");
    // Implementation for restore operation
}

void CleanInstallBackupRestoreWidget::previewRestore()
{
    // Implementation for restore preview
    m_restorePreview->append("Restore preview functionality to be implemented");
}

void CleanInstallBackupRestoreWidget::refreshPackageList()
{
    updateStatus("Refreshing package list...");
    
    // Clear existing items
    m_packagesTree->clear();
    
    // Get packages from PackageManager
    QStringList packages = m_packageManager->getInstalledPackages();
    
    for (const QString &pkg : packages) {
        QStringList parts = pkg.split(" ");
        if (parts.size() >= 2) {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_packagesTree);
            item->setText(0, parts[0]); // Package name
            item->setText(1, parts[1]); // Version
            item->setText(2, "Unknown"); // Size (would need to be calculated)
            item->setText(3, "Package description"); // Description
            item->setCheckState(0, Qt::Unchecked);
        }
    }
    
    updatePackageCount();
    updateStatus("Package list refreshed");
}

void CleanInstallBackupRestoreWidget::updatePackageCount()
{
    int totalCount = m_packagesTree->topLevelItemCount();
    int selectedCount = 0;
    
    for (int i = 0; i < totalCount; ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        if (item && item->checkState(0) == Qt::Checked) {
            selectedCount++;
        }
    }
    
    m_packageCountLabel->setText(QString("Packages: %1 total, %2 selected").arg(totalCount).arg(selectedCount));
}

void CleanInstallBackupRestoreWidget::exportPackageList()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Package List", "packages.txt", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        m_packageManager->exportPackageList(fileName);
        updateStatus("Package list exported successfully");
    }
}

void CleanInstallBackupRestoreWidget::importPackageList()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Package List", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        m_packageManager->importPackageList(fileName);
        refreshPackageList();
        updateStatus("Package list imported successfully");
    }
}

void CleanInstallBackupRestoreWidget::selectAllPackages()
{
    for (int i = 0; i < m_packagesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        if (item) {
            item->setCheckState(0, Qt::Checked);
        }
    }
    updatePackageCount();
}

void CleanInstallBackupRestoreWidget::deselectAllPackages()
{
    for (int i = 0; i < m_packagesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        if (item) {
            item->setCheckState(0, Qt::Unchecked);
        }
    }
    updatePackageCount();
}

void CleanInstallBackupRestoreWidget::refreshSettingsList()
{
    updateStatus("Refreshing settings list...");
    
    m_settingsTree->clear();
    
    // Get settings categories from SettingsManager
    QStringList categories = m_settingsManager->getSettingsCategories();
    
    for (const QString &category : categories) {
        QTreeWidgetItem *categoryItem = new QTreeWidgetItem(m_settingsTree);
        categoryItem->setText(0, category);
        categoryItem->setCheckState(0, Qt::Unchecked);
        
        QStringList files = m_settingsManager->getSettingsFiles(category);
        for (const QString &file : files) {
            QTreeWidgetItem *fileItem = new QTreeWidgetItem(categoryItem);
            fileItem->setText(0, QFileInfo(file).fileName());
            fileItem->setText(1, file);
            fileItem->setText(2, "Unknown"); // Size
            fileItem->setCheckState(0, Qt::Unchecked);
        }
    }
    
    m_settingsTree->expandAll();
    updateStatus("Settings list refreshed");
}

void CleanInstallBackupRestoreWidget::selectAllSettings()
{
    for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_settingsTree->topLevelItem(i);
        if (item) {
            item->setCheckState(0, Qt::Checked);
        }
    }
}

void CleanInstallBackupRestoreWidget::deselectAllSettings()
{
    for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_settingsTree->topLevelItem(i);
        if (item) {
            item->setCheckState(0, Qt::Unchecked);
        }
    }
}

void CleanInstallBackupRestoreWidget::exportSettings()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Settings", "settings.tar.gz", "Archive Files (*.tar.gz)");
    if (!fileName.isEmpty()) {
        m_settingsManager->exportSettings(fileName);
        updateStatus("Settings exported successfully");
    }
}

void CleanInstallBackupRestoreWidget::importSettings()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Import Settings", "", "Archive Files (*.tar.gz)");
    if (!fileName.isEmpty()) {
        m_settingsManager->importSettings(fileName);
        refreshSettingsList();
        updateStatus("Settings imported successfully");
    }
}

void CleanInstallBackupRestoreWidget::updateProgress(int percentage)
{
    m_archBackupProgress->setValue(percentage);
    m_archRestoreProgress->setValue(percentage);
}

void CleanInstallBackupRestoreWidget::updateStatus(const QString &message)
{
    m_archBackupStatusLabel->setText(message);
    m_archRestoreStatusLabel->setText(message);
    
    // Add to log with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp).arg(message);
    
    m_archBackupLog->append(logEntry);
    m_archRestoreLog->append(logEntry);
    m_logsText->append(logEntry);
}

void CleanInstallBackupRestoreWidget::onBackupComplete(bool success)
{
    m_backupInProgress = false;
    m_pauseBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    
    if (success) {
        updateStatus("✅ Backup completed successfully!");
        QMessageBox::information(this, "Backup Complete", "Backup operation completed successfully!");
    } else {
        updateStatus("❌ Backup failed!");
        QMessageBox::warning(this, "Backup Failed", "Backup operation failed. Check logs for details.");
    }
}

void CleanInstallBackupRestoreWidget::onRestoreComplete(bool success)
{
    if (success) {
        updateStatus("✅ Restore completed successfully!");
        QMessageBox::information(this, "Restore Complete", "Restore operation completed successfully!");
    } else {
        updateStatus("❌ Restore failed!");
        QMessageBox::warning(this, "Restore Failed", "Restore operation failed. Check logs for details.");
    }
}

void CleanInstallBackupRestoreWidget::showLogDetails()
{
    // Show detailed log dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Detailed Logs");
    dialog.resize(800, 600);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QTextEdit *detailLog = new QTextEdit();
    detailLog->setReadOnly(true);
    detailLog->setText(m_logsText->toPlainText());
    
    layout->addWidget(detailLog);
    
    QPushButton *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    dialog.exec();
}

void CleanInstallBackupRestoreWidget::clearLogs()
{
    m_archBackupLog->clear();
    m_archRestoreLog->clear();
    m_logsText->clear();
    updateStatus("Logs cleared");
}

void CleanInstallBackupRestoreWidget::showBackupCapabilities()
{
    QDialog dialog(this);
    dialog.setWindowTitle("ArchBackupPro - Backup Capabilities");
    dialog.resize(700, 500);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QTextEdit *capabilitiesText = new QTextEdit();
    capabilitiesText->setReadOnly(true);
    capabilitiesText->setHtml(
        "<h2>📦 ArchBackupPro Backup Capabilities</h2>"
        "<h3>🗃️ Full System Backup</h3>"
        "<ul>"
        "<li>Complete system state snapshot</li>"
        "<li>All installed packages and their configurations</li>"
        "<li>User data and home directories</li>"
        "<li>System configurations and services</li>"
        "</ul>"
        
        "<h3>📦 Package Management</h3>"
        "<ul>"
        "<li>Backup installed package lists</li>"
        "<li>Export/import package selections</li>"
        "<li>Incremental package tracking</li>"
        "<li>AUR package support</li>"
        "</ul>"
        
        "<h3>⚙️ Settings & Configurations</h3>"
        "<ul>"
        "<li>System-wide configurations (/etc)</li>"
        "<li>User configurations (~/.config)</li>"
        "<li>Application settings</li>"
        "<li>Desktop environment settings</li>"
        "</ul>"
        
        "<h3>🔄 Restore Options</h3>"
        "<ul>"
        "<li>Selective restore capabilities</li>"
        "<li>Preview before restore</li>"
        "<li>Incremental restore support</li>"
        "<li>Rollback functionality</li>"
        "</ul>"
        
        "<h3>💡 Advanced Features</h3>"
        "<ul>"
        "<li>Compression options (zstd, gzip, bzip2, xz)</li>"
        "<li>Backup verification and integrity checks</li>"
        "<li>Scheduled automatic backups</li>"
        "<li>RGB/Fan control integration</li>"
        "<li>Real-time monitoring daemon</li>"
        "<li>AI-powered backup optimization</li>"
        "</ul>"
    );
    
    layout->addWidget(capabilitiesText);
    
    QPushButton *closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeBtn);
    
    dialog.exec();
}

void CleanInstallBackupRestoreWidget::showPackageConfigurationDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Package Backup Configuration");
    dialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *info = new QLabel("Configure which packages to include in backup:");
    layout->addWidget(info);
    
    QCheckBox *explicitCheck = new QCheckBox("Explicitly installed packages only");
    QCheckBox *aurCheck = new QCheckBox("Include AUR packages");
    QCheckBox *depsCheck = new QCheckBox("Include dependencies");
    QCheckBox *orphansCheck = new QCheckBox("Include orphaned packages");
    
    explicitCheck->setChecked(true);
    aurCheck->setChecked(true);
    
    layout->addWidget(explicitCheck);
    layout->addWidget(aurCheck);
    layout->addWidget(depsCheck);
    layout->addWidget(orphansCheck);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("OK");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);
    
    dialog.exec();
}

void CleanInstallBackupRestoreWidget::showSettingsConfigurationDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Settings Backup Configuration");
    dialog.resize(600, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *info = new QLabel("Configure which settings to include in backup:");
    layout->addWidget(info);
    
    QCheckBox *systemCheck = new QCheckBox("System configurations (/etc)");
    QCheckBox *userCheck = new QCheckBox("User configurations (~/.config)");
    QCheckBox *dotfilesCheck = new QCheckBox("Dotfiles in home directory");
    QCheckBox *desktopCheck = new QCheckBox("Desktop environment settings");
    
    systemCheck->setChecked(true);
    userCheck->setChecked(true);
    dotfilesCheck->setChecked(true);
    
    layout->addWidget(systemCheck);
    layout->addWidget(userCheck);
    layout->addWidget(dotfilesCheck);
    layout->addWidget(desktopCheck);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okBtn = new QPushButton("OK");
    QPushButton *cancelBtn = new QPushButton("Cancel");
    
    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    
    buttonLayout->addWidget(okBtn);
    buttonLayout->addWidget(cancelBtn);
    layout->addLayout(buttonLayout);
    
    dialog.exec();
}
