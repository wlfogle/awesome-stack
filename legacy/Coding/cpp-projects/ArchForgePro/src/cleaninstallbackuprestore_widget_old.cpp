#include "cleaninstallbackuprestore_widget.h"
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
#include <QFileInfo>
#include <QTextStream>
#include <QIODevice>
#include <QFont>

// Async package loading function
QList<PackageInfo> loadPackagesAsync(PackageManager* packageManager) {
    if (packageManager) {
        packageManager->refreshPackageList();
        return packageManager->getInstalledPackages();
    }
    return QList<PackageInfo>();
}

CleanInstallBackupRestoreWidget::CleanInstallBackupRestoreWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainSubTabWidget(nullptr)
    , m_backupInProgress(false)
{
    setWindowTitle("ArchBackupPro - Comprehensive Backup Solution");
    resize(1200, 800);
    
    // Initialize core components
    m_backupManager = new BackupManager(this);
    m_restoreManager = new RestoreManager(this);
    m_packageManager = new PackageManager(this);
    m_settingsManager = new SettingsManager(this);
    
    // Initialize settings
    m_settings = new QSettings("ArchBackupPro", "ArchBackupPro", this);
    
    // Initialize async package watcher
    m_packageWatcher = new QFutureWatcher<QList<PackageInfo>>(this);
    connect(m_packageWatcher, &QFutureWatcher<QList<PackageInfo>>::finished, this, [this]() {
        QList<PackageInfo> packages = m_packageWatcher->result();
        emit packagesLoaded(packages);
    });
    
    // Setup UI
    setupUI();
    setupConnections();

    // Load settings
    loadWidgetSettings();
}

CleanInstallBackupRestoreWidget::~CleanInstallBackupRestoreWidget()
{
    if (m_settings) {
        m_settings->setValue("geometry", this->geometry());
        m_settings->setValue("backup_location", m_backupLocationEdit->text());
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
    
    m_mainSubTabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_mainSubTabWidget);
    
    setupBackupTab();
    setupRestoreTab();
    setupPackagesTab();
    setupSettingsTab();
    setupLogsTab();
}

void CleanInstallBackupRestoreWidget::setupBackupTab()
{
    m_backupTab = new QWidget();
    m_mainSubTabWidget->addTab(m_backupTab, "&Backup");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_backupTab);
    
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
    
    m_backupLocationEdit = new QLineEdit();
    m_backupLocationEdit->setPlaceholderText("Select backup destination...");
    m_browseLocationBtn = new QPushButton("Browse...");
    
    locationLayout->addWidget(new QLabel("Location:"));
    locationLayout->addWidget(m_backupLocationEdit);
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
    
    // Connect backup buttons to actual functions
    connect(fullBackupBtn, &QPushButton::clicked, this, [this]() {
        QString location = m_backupLocationEdit->text();
        if (location.isEmpty()) {
            QMessageBox::warning(this, "Backup Location", "Please select a backup location first.");
            return;
        }
        updateStatus("Starting full system backup...");
        m_backupManager->startFullBackup(location);
    });
    
    connect(packageOnlyBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::startPackageBackup);
    connect(settingsOnlyBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::startSettingsBackup);
    connect(m_pauseBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::pauseBackup);
    connect(m_cancelBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::cancelBackup);
    
    controlLayout->addWidget(fullBackupBtn);
    controlLayout->addWidget(packageOnlyBtn);
    controlLayout->addWidget(settingsOnlyBtn);
    controlLayout->addStretch();
    controlLayout->addWidget(m_pauseBtn);
    controlLayout->addWidget(m_cancelBtn);
    
    // Progress section
    QGroupBox *progressGroup = new QGroupBox("Progress");
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    
    m_backupProgress = new QProgressBar();
    m_backupStatusLabel = new QLabel("Ready");
    m_backupLog = new QTextEdit();
    m_backupLog->setMaximumHeight(150);
    m_backupLog->setPlaceholderText("Backup progress and status messages will appear here...");
    
    progressLayout->addWidget(m_backupProgress);
    progressLayout->addWidget(m_backupStatusLabel);
    progressLayout->addWidget(new QLabel("Log:"));
    progressLayout->addWidget(m_backupLog);
    
    // Add all groups to main layout
    mainLayout->addWidget(typeGroup);
    mainLayout->addWidget(locationGroup);
    mainLayout->addWidget(compressionGroup);
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(progressGroup);
}

void CleanInstallBackupRestoreWidget::setupRestoreTab()
{
    m_restoreTab = new QWidget();
    m_mainSubTabWidget->addTab(m_restoreTab, "🔄 Restore");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_restoreTab);
    
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
    
    // Connect browse archive button
    connect(browseArchiveBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Select Backup Archive", "", "Archive Files (*.tar.gz *.tar.bz2 *.tar.xz *.zip);;All Files (*)");
        if (!fileName.isEmpty()) {
            m_archivePathEdit->setText(fileName);
            // Enable preview and restore buttons
            m_previewBtn->setEnabled(true);
            m_restoreBtn->setEnabled(true);
            m_deleteRestorePointBtn->setEnabled(true);
            updateStatus("Archive selected: " + QFileInfo(fileName).fileName());
        }
    });
    
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
    
    m_previewBtn = new QPushButton("👁️ Preview Restore");
    m_restoreBtn = new QPushButton("🔄 Start Restore");
    m_deleteRestorePointBtn = new QPushButton("🗑️ Delete Restore Point");
    
    m_previewBtn->setEnabled(false);
    m_restoreBtn->setEnabled(false);
    m_deleteRestorePointBtn->setEnabled(false);
    
    // Connect restore buttons
    connect(m_previewBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::previewRestore);
    connect(m_restoreBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::startRestore);
    connect(m_deleteRestorePointBtn, &QPushButton::clicked, this, [this]() {
        QString archivePath = m_archivePathEdit->text();
        if (!archivePath.isEmpty()) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Delete Archive", 
                "Are you sure you want to delete this backup archive?", 
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                if (QFile::remove(archivePath)) {
                    updateStatus("Archive deleted successfully");
                    m_archivePathEdit->clear();
                    m_previewBtn->setEnabled(false);
                    m_restoreBtn->setEnabled(false);
                    m_deleteRestorePointBtn->setEnabled(false);
                } else {
                    QMessageBox::warning(this, "Delete Failed", "Could not delete the archive file.");
                }
            }
        }
    });
    
    controlLayout->addWidget(m_previewBtn);
    controlLayout->addWidget(m_restoreBtn);
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
    m_restoreProgress = new QProgressBar();
    m_restoreStatusLabel = new QLabel("Ready");
    m_restoreLog = new QTextEdit();
    m_restoreLog->setMaximumHeight(100);
    
    progressLayout->addWidget(m_restoreProgress);
    progressLayout->addWidget(m_restoreStatusLabel);
    progressLayout->addWidget(m_restoreLog);
    
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

void CleanInstallBackupRestoreWidget::setupLogsTab()
{
    m_logsTab = new QWidget();
    m_mainSubTabWidget->addTab(m_logsTab, "📋 Logs");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_logsTab);
    
    // Log controls
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    m_logLevelCombo = new QComboBox();
    m_logLevelCombo->addItems({"All Logs", "Info", "Warning", "Error", "Debug"});
    
    m_clearLogsBtn = new QPushButton("🧹 Clear Logs");
    m_exportLogsBtn = new QPushButton("📤 Export Logs");
    QPushButton *refreshLogsBtn = new QPushButton("🔄 Refresh");
    
    controlLayout->addWidget(new QLabel("Filter:"));
    controlLayout->addWidget(m_logLevelCombo);
    controlLayout->addStretch();
    controlLayout->addWidget(refreshLogsBtn);
    controlLayout->addWidget(m_clearLogsBtn);
    controlLayout->addWidget(m_exportLogsBtn);
    
    // Logs display
    m_logsText = new QTextEdit();
    m_logsText->setReadOnly(true);
    m_logsText->setPlaceholderText("ArchBackupPro logs will appear here...");
    
    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_logsText);
}

void CleanInstallBackupRestoreWidget::setupConnections()
{
    // Settings button - FIXED
    connect(m_settingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showBackupCapabilities);
    
    // Backup location browse - FIXED
    connect(m_browseLocationBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Backup Location", 
                                                        m_backupLocationEdit->text());
        if (!dir.isEmpty()) {
            m_backupLocationEdit->setText(dir);
            updateStatus("Backup location set: " + dir);
        }
    });
    
    // Package backup options - FIXED
    connect(m_packageBackupBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showPackageConfigurationDialog);
    
    // Settings backup options - FIXED  
    connect(m_settingsBackupBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::showSettingsConfigurationDialog);
    
    // Package management connections - FIXED
    connect(m_refreshPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::refreshPackageList);
    connect(m_selectAllPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::selectAllPackages);
    connect(m_deselectAllPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::deselectAllPackages);
    connect(m_exportPackagesBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::exportPackageList);
    
    // Package search - FIXED to filter without reloading
    connect(m_packageSearchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        // Simple filtering - no reloading
        int visibleCount = 0;
        int totalCount = m_packagesTree->topLevelItemCount();
        
        // If no packages loaded, show message but don't auto-load
        if (totalCount == 0) {
            if (!text.isEmpty()) {
                updateStatus("No packages loaded. Click 'Refresh' to load packages first.");
            }
            return;
        }
        
        // Filter existing packages
        for (int i = 0; i < totalCount; ++i) {
            QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
            if (item) {
                bool matches = text.isEmpty();
                
                if (!matches) {
                    // Search in package name, version, size, and description
                    matches = item->text(0).contains(text, Qt::CaseInsensitive) ||
                             item->text(1).contains(text, Qt::CaseInsensitive) ||
                             item->text(2).contains(text, Qt::CaseInsensitive) ||
                             item->text(3).contains(text, Qt::CaseInsensitive);
                }
                
                item->setHidden(!matches);
                
                if (matches) {
                    visibleCount++;
                }
            }
        }
        
        updatePackageCount();
        
        if (text.isEmpty()) {
            updateStatus(QString("Showing all %1 packages").arg(totalCount));
        } else {
            updateStatus(QString("Search '%1': %2 of %3 packages match").arg(text).arg(visibleCount).arg(totalCount));
        }
    });
    
    // Package import - FIXED with proper implementation
    connect(m_importPackagesBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Import Package List", "", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty()) {
            updateStatus("Package file import will be implemented in next version");
            QMessageBox::information(this, "Import Packages", "Package file import functionality will be implemented in the next version.");
        }
    });
    
    // Settings management connections - FIXED
    connect(m_refreshSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::refreshSettingsList);
    connect(m_selectAllSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::selectAllSettings);
    connect(m_deselectAllSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::deselectAllSettings);
    connect(m_exportSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::exportSettings);
    connect(m_importSettingsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::importSettings);
    
    // Settings search - FIXED to filter without reloading
    connect(m_settingsSearchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < m_settingsTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *topItem = m_settingsTree->topLevelItem(i);
            bool hasVisibleChild = false;
            
            for (int j = 0; j < topItem->childCount(); ++j) {
                QTreeWidgetItem *child = topItem->child(j);
                bool matches = child->text(0).contains(text, Qt::CaseInsensitive) ||
                              child->text(1).contains(text, Qt::CaseInsensitive);
                child->setHidden(!matches && !text.isEmpty());
                if (matches || text.isEmpty()) {
                    hasVisibleChild = true;
                }
            }
            
            // Hide parent if no children match
            topItem->setHidden(!hasVisibleChild && !text.isEmpty());
        }
    });
    
    // Logs connections - FIXED
    connect(m_clearLogsBtn, &QPushButton::clicked, this, &CleanInstallBackupRestoreWidget::clearLogs);
    connect(m_exportLogsBtn, &QPushButton::clicked, this, [this]() {
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
    
    // Core component connections - FIXED
    if (m_backupManager) {
        connect(m_backupManager, &BackupManager::progressChanged, this, &CleanInstallBackupRestoreWidget::updateProgress);
        connect(m_backupManager, &BackupManager::statusChanged, this, &CleanInstallBackupRestoreWidget::updateStatus);
        connect(m_backupManager, &BackupManager::backupCompleted, this, &CleanInstallBackupRestoreWidget::onBackupComplete);
    }
    
    if (m_restoreManager) {
        connect(m_restoreManager, &RestoreManager::restoreCompleted, this, &CleanInstallBackupRestoreWidget::onRestoreComplete);
    }
}

// ArchBackupPro Slot Implementations
void CleanInstallBackupRestoreWidget::startPackageBackup()
{
    QString location = m_backupLocationEdit->text();
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
    QString location = m_backupLocationEdit->text();
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
    QString archivePath = m_archivePathEdit->text();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "No Archive Selected", "Please select a backup archive first.");
        return;
    }
    
    if (!QFile::exists(archivePath)) {
        QMessageBox::warning(this, "Archive Not Found", "The selected archive file does not exist.");
        return;
    }
    
    // Confirm restore operation
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Restore", 
        "Are you sure you want to restore from this backup?\n\nThis operation may overwrite existing files.", 
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    updateStatus("Starting restore operation...");
    m_restoreProgress->setValue(0);
    
    // Start the restore using RestoreManager
    if (m_restoreManager) {
        // Note: RestoreManager has simple signature, options are handled internally
        m_restoreManager->startRestore(archivePath);
        updateStatus(QString("Restore started - packages: %1, settings: %2, user data: %3")
                    .arg(m_restorePackagesCheck->isChecked() ? "Yes" : "No")
                    .arg(m_restoreSettingsCheck->isChecked() ? "Yes" : "No")
                    .arg(m_restoreUserDataCheck->isChecked() ? "Yes" : "No"));
    } else {
        updateStatus("Error: Restore manager not available");
    }
}

void CleanInstallBackupRestoreWidget::previewRestore()
{
    QString archivePath = m_archivePathEdit->text();
    if (archivePath.isEmpty()) {
        QMessageBox::warning(this, "No Archive Selected", "Please select a backup archive first.");
        return;
    }
    
    if (!QFile::exists(archivePath)) {
        QMessageBox::warning(this, "Archive Not Found", "The selected archive file does not exist.");
        return;
    }
    
    updateStatus("Generating restore preview...");
    m_restorePreview->clear();
    
    // Get archive info and contents
    QFileInfo archiveInfo(archivePath);
    m_restorePreview->append("📁 Archive Information:");
    m_restorePreview->append(QString("Name: %1").arg(archiveInfo.fileName()));
    m_restorePreview->append(QString("Size: %1 MB").arg(archiveInfo.size() / (1024 * 1024)));
    m_restorePreview->append(QString("Modified: %1").arg(archiveInfo.lastModified().toString()));
    m_restorePreview->append("");
    
    // List archive contents using tar
    m_restorePreview->append("📋 Archive Contents:");
    QProcess tarProcess;
    tarProcess.start("tar", {"-tzf", archivePath});
    tarProcess.waitForFinished();
    
    if (tarProcess.exitCode() == 0) {
        QString contents = tarProcess.readAllStandardOutput();
        QStringList files = contents.split("\n", Qt::SkipEmptyParts);
        
        int maxFiles = qMin(50, files.size()); // Show first 50 files
        for (int i = 0; i < maxFiles; ++i) {
            m_restorePreview->append(QString("  • %1").arg(files[i]));
        }
        
        if (files.size() > 50) {
            m_restorePreview->append(QString("  ... and %1 more files").arg(files.size() - 50));
        }
        
        m_restorePreview->append("");
        m_restorePreview->append(QString("Total files: %1").arg(files.size()));
    } else {
        m_restorePreview->append("❌ Error reading archive contents");
    }
    
    updateStatus("Restore preview generated");
}

void CleanInstallBackupRestoreWidget::refreshPackageList()
{
    updateStatus("Refreshing package list...");
    
    // Clear existing items
    m_packagesTree->clear();
    
    // Disable UI while loading
    m_refreshPackagesBtn->setEnabled(false);
    m_refreshPackagesBtn->setText("🔄 Loading...");
    
    // Start async package loading
    QFuture<QList<PackageInfo>> future = QtConcurrent::run(loadPackagesAsync, m_packageManager);
    m_packageWatcher->setFuture(future);
    
    // Connect to handle results
    connect(this, &CleanInstallBackupRestoreWidget::packagesLoaded, this, [this](const QList<PackageInfo> &packages) {
        // Re-enable UI
        m_refreshPackagesBtn->setEnabled(true);
        m_refreshPackagesBtn->setText("🔄 Refresh");
        
        // Populate tree
        for (const PackageInfo &pkg : packages) {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_packagesTree);
            item->setText(0, pkg.name); // Package name
            item->setText(1, pkg.version); // Version
            item->setText(2, QString::number(pkg.size / 1024) + " KB"); // Size
            item->setText(3, pkg.description); // Description
            item->setCheckState(0, Qt::Unchecked);
        }
        
        updatePackageCount();
        updateStatus(QString("Package list refreshed - %1 packages loaded").arg(packages.size()));
    }, Qt::SingleShotConnection);
}

void CleanInstallBackupRestoreWidget::updatePackageCount()
{
    int totalCount = m_packagesTree->topLevelItemCount();
    int visibleCount = 0;
    int selectedCount = 0;
    
    for (int i = 0; i < totalCount; ++i) {
        QTreeWidgetItem *item = m_packagesTree->topLevelItem(i);
        if (item) {
            if (!item->isHidden()) {
                visibleCount++;
            }
            if (item->checkState(0) == Qt::Checked) {
                selectedCount++;
            }
        }
    }
    
    QString searchText = m_packageSearchEdit->text();
    if (searchText.isEmpty()) {
        m_packageCountLabel->setText(QString("Packages: %1 total, %2 selected").arg(totalCount).arg(selectedCount));
    } else {
        m_packageCountLabel->setText(QString("Packages: %1 visible of %2 total, %3 selected").arg(visibleCount).arg(totalCount).arg(selectedCount));
    }
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
    
    // Get settings from SettingsManager
    m_settingsManager->refreshSettingsList();
    QList<SettingFile> settingFiles = m_settingsManager->getSettingFiles();
    
    // Create category items
    QTreeWidgetItem *systemItem = new QTreeWidgetItem(m_settingsTree);
    systemItem->setText(0, "System Configuration");
    systemItem->setCheckState(0, Qt::Unchecked);
    
    QTreeWidgetItem *userItem = new QTreeWidgetItem(m_settingsTree);
    userItem->setText(0, "User Configuration");
    userItem->setCheckState(0, Qt::Unchecked);
    
    // Add files to appropriate categories
    for (const SettingFile &file : settingFiles) {
        QTreeWidgetItem *fileItem = new QTreeWidgetItem();
        fileItem->setText(0, file.name);
        fileItem->setText(1, file.path);
        fileItem->setText(2, QString::number(file.size / 1024) + " KB");
        fileItem->setCheckState(0, Qt::Unchecked);
        
        if (file.isSystemConfig) {
            systemItem->addChild(fileItem);
        } else {
            userItem->addChild(fileItem);
        }
    }
    
    m_settingsTree->expandAll();
    updateStatus(QString("Settings list refreshed - %1 files found").arg(settingFiles.size()));
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
    m_backupProgress->setValue(percentage);
    m_restoreProgress->setValue(percentage);
}

void CleanInstallBackupRestoreWidget::updateStatus(const QString &message)
{
    m_backupStatusLabel->setText(message);
    m_restoreStatusLabel->setText(message);
    
    // Add to log with timestamp
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp).arg(message);
    
    m_backupLog->append(logEntry);
    m_restoreLog->append(logEntry);
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
    m_backupLog->clear();
    m_restoreLog->clear();
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
            packageTree->setEnabled(false); // Initially disabled during loading
            selectAllBtn->setEnabled(false);
            deselectAllBtn->setEnabled(false);
            selectExplicitBtn->setEnabled(false);
            searchEdit->setEnabled(false);
            refreshBtn->setEnabled(false);
            importFileEdit->setEnabled(false);
            browseFileBtn->setEnabled(false);
            
            // Show loading message
            QTreeWidgetItem *loadingItem = new QTreeWidgetItem(packageTree);
            loadingItem->setText(0, "Loading packages...");
            loadingItem->setText(1, "Please wait");
            
            // Start async package loading
            if (m_packageManager) {
                QFuture<QList<PackageInfo>> future = QtConcurrent::run(loadPackagesAsync, m_packageManager);
                
                // Use a separate watcher for the dialog
                QFutureWatcher<QList<PackageInfo>> *dialogWatcher = new QFutureWatcher<QList<PackageInfo>>();
                connect(dialogWatcher, &QFutureWatcher<QList<PackageInfo>>::finished, [this, packageTree, selectAllBtn, deselectAllBtn, selectExplicitBtn, searchEdit, refreshBtn, dialogWatcher]() {
                    QList<PackageInfo> packages = dialogWatcher->result();
                    
                    // Clear loading message
                    packageTree->clear();
                    
                    // Populate package list
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
                    
                    // Re-enable controls
                    packageTree->setEnabled(true);
                    selectAllBtn->setEnabled(true);
                    deselectAllBtn->setEnabled(true);
                    selectExplicitBtn->setEnabled(true);
                    searchEdit->setEnabled(true);
                    refreshBtn->setEnabled(true);
                    
                    // Clean up watcher
                    dialogWatcher->deleteLater();
                });
                
                dialogWatcher->setFuture(future);
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

void CleanInstallBackupRestoreWidget::showSettingsConfigurationDialog()
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

void CleanInstallBackupRestoreWidget::updateUIState(bool backupInProgress)
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
        m_restoreProgress->setValue(0);
    }
}

void CleanInstallBackupRestoreWidget::saveWidgetSettings()
{
    if (m_settings) {
        m_settings->setValue("widget_geometry", this->geometry());
        m_settings->setValue("compression_level", m_compressionSlider->value());
        m_settings->setValue("verify_backups", m_verifyCheckBox->isChecked());
        m_settings->setValue("backup_location", m_backupLocationEdit->text());
        m_settings->setValue("compression_method", m_compressionCombo->currentText());
    }
}

void CleanInstallBackupRestoreWidget::loadWidgetSettings()
{
    if (m_settings) {
        // Load backup location
        QString location = m_settings->value("backup_location", 
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ArchBackups").toString();
        m_backupLocationEdit->setText(location);
        
        // Load compression settings
        int compressionLevel = m_settings->value("compression_level", 3).toInt();
        m_compressionSlider->setValue(compressionLevel);
        
        QString compressionMethod = m_settings->value("compression_method", "zstd (Recommended)").toString();
        m_compressionCombo->setCurrentText(compressionMethod);
        
        // Load verification setting
        bool verifyBackups = m_settings->value("verify_backups", true).toBool();
        m_verifyCheckBox->setChecked(verifyBackups);
    }
}

void CleanInstallBackupRestoreWidget::updateWidgetStatus(const QString &message)
{
    updateStatus(message);
}
