#ifndef SOFTWAREMANAGEMENT_WIDGET_H
#define SOFTWAREMANAGEMENT_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QListWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QTreeWidget>

class SoftwareManagementWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SoftwareManagementWidget(QWidget *parent = nullptr);
    ~SoftwareManagementWidget();

private slots:
    // Search Package slots
    void performQuickSearch();
    void performAdvancedSearch();
    void clearSearchResults();
    void showSearchHistory();
    
    // Package Install slots
    void installSinglePackage();
    void addToBatchInstall();
    void processBatchInstall();
    void showInstallQueue();
    void clearInstallQueue();
    void showInstallHistory();
    void clearInstallHistory();
    
    // Build & Distribute slots
    void buildPackage();
    void distributePackage();
    void showBuildLog();
    void clearBuildLog();
    
    // Windows Programs slots
    void manageWine();
    void installWindowsProgram();
    void showInstalledPrograms();
    void manageWinePrefixes();
    void showWineLogs();
    
    // Maintenance slots
    void performQuickMaintenance();
    void checkSystemUpdates();
    void cleanPackageCache();
    void optimizeSystem();
    void showMaintenanceLogs();
    
    // Installed Packages slots
    void refreshInstalledPackages();
    void uninstallPackage();
    void showPackageInfo();
    
    // Settings slots
    void saveSettings();
    void resetSettings();

private:
    void setupUI();
    void setupConnections();
    
    // Main sub-tab creation methods
    QWidget* createSearchPackagesTab();
    QWidget* createPackageInstallTab();
    QWidget* createBuildDistributeTab();
    QWidget* createWindowsProgramsTab();
    QWidget* createMaintenanceTab();
    QWidget* createInstalledPackagesTab();
    QWidget* createSettingsTab();
    
    // Search sub-tab creation methods
    QWidget* createQuickSearchTab();
    QWidget* createAdvancedSearchTab();
    QWidget* createSearchResultsTab();
    QWidget* createSearchHistoryTab();
    
    // Install sub-tab creation methods
    QWidget* createSingleInstallTab();
    QWidget* createBatchInstallTab();
    QWidget* createInstallQueueTab();
    QWidget* createInstallHistoryTab();
    QWidget* createInstallLogTab();
    
    // Build sub-tab creation methods
    QWidget* createPackageBuilderTab();
    QWidget* createDistributionTab();
    QWidget* createBuildLogTab();
    
    // Windows sub-tab creation methods
    QWidget* createWineManagementTab();
    QWidget* createProgramInstallerTab();
    QWidget* createInstalledProgramsTab();
    QWidget* createWinePrefixesTab();
    QWidget* createWineLogsTab();
    
    // Maintenance sub-tab creation methods
    QWidget* createQuickMaintenanceTab();
    QWidget* createSystemUpdatesTab();
    QWidget* createPackageCacheTab();
    QWidget* createSystemOptimizationTab();
    QWidget* createMaintenanceLogsTab();
    
    // Main components
    QTabWidget *m_mainTabWidget;
    
    // Search Package components
    QTabWidget *m_searchTabWidget;
    QLineEdit *m_quickSearchEdit;
    QPushButton *m_quickSearchButton;
    QTableWidget *m_searchResultsTable;
    QListWidget *m_searchHistoryList;
    
    // Advanced search components
    QLineEdit *m_advancedNameEdit;
    QLineEdit *m_advancedDescEdit;
    QComboBox *m_repositoryCombo;
    QComboBox *m_categoryCombo;
    QCheckBox *m_exactMatchCheck;
    
    // Package Install components
    QTabWidget *m_installTabWidget;
    QLineEdit *m_singlePackageEdit;
    QPushButton *m_installSingleButton;
    QListWidget *m_batchInstallList;
    QListWidget *m_installQueueList;
    QTableWidget *m_installHistoryTable;
    QTextEdit *m_installLogText;
    QProgressBar *m_installProgressBar;
    
    // Build & Distribute components
    QTabWidget *m_buildTabWidget;
    QLineEdit *m_packageNameEdit;
    QTextEdit *m_packageDescEdit;
    QPushButton *m_buildButton;
    QPushButton *m_distributeButton;
    QTextEdit *m_buildLogText;
    
    // Windows Programs components
    QTabWidget *m_windowsTabWidget;
    QTableWidget *m_wineVersionsTable;
    QListWidget *m_winePrefixesList;
    QTableWidget *m_installedProgramsTable;
    QTextEdit *m_wineLogsText;
    
    // Maintenance components
    QTabWidget *m_maintenanceTabWidget;
    QPushButton *m_quickMaintenanceButton;
    QPushButton *m_updateSystemButton;
    QPushButton *m_cleanCacheButton;
    QPushButton *m_optimizeButton;
    QTextEdit *m_maintenanceLogsText;
    QProgressBar *m_maintenanceProgressBar;
    
    // Installed Packages components
    QTreeWidget *m_installedPackagesTree;
    QLineEdit *m_packageFilterEdit;
    QPushButton *m_uninstallButton;
    QPushButton *m_packageInfoButton;
    
    // Settings components
    QCheckBox *m_autoUpdateCheck;
    QCheckBox *m_parallelDownloadsCheck;
    QSpinBox *m_maxDownloadsSpin;
    QComboBox *m_mirrorCombo;
    QPushButton *m_saveSettingsButton;
    QPushButton *m_resetSettingsButton;
};

#endif // SOFTWAREMANAGEMENT_WIDGET_H
