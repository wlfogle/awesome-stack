#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QProgressBar>
#include <QListWidget>
#include <QTimer>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QCloseEvent>

#include "packageinfo.h"

// Forward declarations
class PackageManager;
class SearchThread;
class InstallThread;
class CylonThread;
class WineManager;
class SettingsManager;
class DatabaseManager;
class PerformanceMonitor;
class BackupManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Search functionality
    void performSearch();
    void performAdvancedSearch();
    void clearSearchFilters();
    void filterSearchResults(const QString &filter);
    void sortSearchResults(const QString &sortBy);
    void displaySearchResults(const QList<PackageInfo> &packages);
    void searchForPopularPackage(const QString &package);
    void onSearchCompleted(const QList<PackageInfo> &results);
    void onSearchError(const QString &error);

    // Installation functionality
    void installSinglePackage();
    void installBatchPackages();
    void installSelectedResults();
    void addToInstallQueue();
    void addSingleToQueue();
    void addBatchToQueue();
    void processInstallQueue();
    void clearInstallQueue();
    void pauseInstallQueue();
    void onInstallCompleted(const QString &package, bool success);
    void onInstallProgress(const QString &package, int progress);
    
    // PackageManager signal handlers
    void onOperationStarted(const QString &operation);
    void onOperationProgress(const QString &operation, int percentage);
    void onOperationOutput(const QString &output);
    void onOperationFinished(const QString &operation, bool success);
    void onPackageInstalled(const QString &package, bool success);
    
    // Install tab specific methods
    void searchBeforeInstall();
    void validateBatchPackages();
    void loadInstallPreset();
    void saveInstallQueue();
    void refreshInstallHistory();
    void exportInstallHistory();
    void clearInstallHistory();
    void filterInstallHistory(const QString &filter);
    void filterInstallLog(const QString &level);
    void updateInstallQueueDisplay();
    void logInstallOperation(const QString &message);
    void addToInstallHistory(const PackageInfo &package, bool success);
    void loadPresetPackages(const QString &presetName);
    InstallMethod stringToInstallMethod(const QString &str);
    void loadInstallQueue();
    void addPackagesToInstall(const QList<PackageInfo> &packages);

    // System maintenance
    void runQuickMaintenance(const QString &operation);
    void checkForUpdates();
    void installSystemUpdates();
    void cleanPackageCache();
    void cleanAURCache();
    void cleanAllCaches();
    void viewCacheContents();
    void optimizeMirrorList();
    void cleanupOrphanedPackages();
    void trimSystemLogs();
    void optimizePackageDatabase();

    // Cylon integration
    void showCylonTerminal();
    void startCylonProcess();
    void stopCylonProcess();
    void handleCylonInput();
    void onCylonOutput(const QString &output);
    void onCylonFinished();

    // Wine/Windows management
    void checkWineStatus();
    void installWine();
    void openWineConfig();
    void scanWinePrefixes();
    void createWinePrefix();
    void installWindowsProgram();
    void searchWindowsPrograms();
    void refreshInstalledPrograms();
    void uninstallSelectedPrograms();
    void runSelectedProgram();


    // Package management
    void refreshInstalledPackages();
    void removePackage(const QString &package);
    void exportInstalledPackages();
    void importPackageList();

    // Search history
    void loadSearchHistory();
    void exportSearchHistory();
    void clearSearchHistory();
    void repeatSearch(const QString &query);

    // Build and distribution
    void createPKGBUILD();
    void buildPackage();
    void testPackage();
    void createRepository();
    void addPackageToRepository();
    void signPackages();
    void browseSourceDirectory();
    void browseRepositoryPath();

    // Settings and configuration
    void saveSettings();
    void resetSettings();
    void changeTheme(const QString &theme);
    void exportConfiguration();
    void importConfiguration();
    void updateSystemInfo();

    // Performance monitoring
    void updatePerformanceMetrics();
    void updateSystemStatus();

    // File operations
    void loadPackageList();
    void savePackageList();
    void saveBuildLog();
    void saveMaintenanceLog();
    void saveInstallLog();
    void saveWindowsLog();

    // UI helpers
    void applyBauhTheme();
    void updateInstallQueueStats();
    void onPackageSelectionChanged();
    void onProgramSelectionChanged();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    void setupTimers();
    
    // Tab creation methods
    QWidget* createSearchTab();
    QWidget* createQuickSearchTab();
    QWidget* createAdvancedSearchTab();
    QWidget* createSearchResultsTab();
    QWidget* createSearchHistoryTab();
    
    // Search tab helper methods
    void setupSearchConnections();
    void populatePopularPackages();
    void setupAdvancedFilters();
    void setupSearchResultsTable();
    void setupSearchHistoryTable();
    void updateSearchStatus(const QString &status);
    void addSearchToHistory(const QString &query, int results);
    void loadSavedSearches();
    void saveCurrentSearch();
    void manageSavedSearches();
    void showPackageInfo(const PackageInfo &package);
    
    QWidget* createInstallTab();
    QWidget* createSingleInstallTab();
    QWidget* createBatchInstallTab();
    QWidget* createInstallQueueTab();
    QWidget* createInstallHistoryTab();
    QWidget* createInstallLogTab();
    
    
    QWidget* createBuildDistributeTab();
    QWidget* createPackageBuilderTab();
    QWidget* createPackageDistributionTab();
    QWidget* createBuildLogTab();
    
    QWidget* createWindowsTab();
    QWidget* createWineManagementTab();
    QWidget* createProgramInstallerTab();
    QWidget* createInstalledProgramsTab();
    QWidget* createWinePrefixesTab();
    QWidget* createWindowsLogsTab();
    
    QWidget* createMaintenanceTab();
    QWidget* createQuickMaintenanceTab();
    QWidget* createSystemUpdatesTab();
    QWidget* createPackageCacheTab();
    QWidget* createSystemOptimizationTab();
    QWidget* createMaintenanceLogsTab();
    
    QWidget* createInstalledTab();
    QWidget* createSettingsTab();
    QWidget* createGeneralSettingsTab();
    QWidget* createPackageManagerPreferencesTab();
    QWidget* createAppearanceTab();
    QWidget* createSystemInformationTab();

    // Core components
    PackageManager *m_packageManager;
    SearchThread *m_searchThread;
    InstallThread *m_installThread;
    CylonThread *m_cylonThread;
    WineManager *m_wineManager;
    SettingsManager *m_settingsManager;
    DatabaseManager *m_databaseManager;
    PerformanceMonitor *m_performanceMonitor;
    BackupManager *m_backupManager;

    // Main UI components
    QTabWidget *m_tabWidget;
    QStatusBar *m_statusBar;
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    
    // Main level tab widgets
    QTabWidget *m_softwareManagementTabWidget;
    QTabWidget *m_rgbFanControlTabWidget;
    QTabWidget *m_kernelToolsTabWidget;
    QTabWidget *m_cleanInstallTabWidget;
    
    // Software Management nested tab widgets
    QTabWidget *m_searchPackagesTabWidget;
    QTabWidget *m_packageInstallTabWidget;

    // Search tab widgets
    QLineEdit *m_searchInput;
    QPushButton *m_searchButton;
    QCheckBox *m_includeAURCheck;
    QCheckBox *m_includeFlatpakCheck;
    QTableWidget *m_resultsTable;
    QLabel *m_searchStatus;
    QComboBox *m_resultsSortCombo;
    QLineEdit *m_resultsFilterInput;
    QTableWidget *m_historyTable;

    // Advanced search widgets
    QLineEdit *m_advPackageName;
    QLineEdit *m_advDescription;
    QComboBox *m_advCategoryCombo;
    QComboBox *m_advMethodCombo;
    QSpinBox *m_minSizeSpinBox;
    QSpinBox *m_maxSizeSpinBox;
    QListWidget *m_savedSearchesList;

    // Install tab widgets
    QLineEdit *m_installPackageInput;
    QComboBox *m_installMethodCombo;
    QCheckBox *m_installWithDepsCheck;
    QCheckBox *m_installFromAURCheck;
    QTextEdit *m_packageInfoDisplay;
    QTextEdit *m_batchInstallText;
    QComboBox *m_batchMethodCombo;
    QCheckBox *m_batchContinueOnErrorCheck;
    QTableWidget *m_installQueueTable;
    QProgressBar *m_queueProgress;
    QTableWidget *m_installHistoryTable;
    QTextEdit *m_installLog;
    QLabel *m_queueTotalLabel;
    QLabel *m_queuePendingLabel;
    QLabel *m_queueCompletedLabel;
    QLabel *m_queueFailedLabel;


    // Build/Distribution widgets
    QLineEdit *m_buildPackageName;
    QLineEdit *m_buildVersion;
    QLineEdit *m_buildDescription;
    QLineEdit *m_buildSourcePath;
    QComboBox *m_buildTypeCombo;
    QTextEdit *m_buildDependencies;
    QLineEdit *m_repoName;
    QLineEdit *m_repoDescription;
    QLineEdit *m_repoPath;
    QTableWidget *m_repoPackagesTable;
    QTextEdit *m_buildLog;

    // Windows/Wine widgets
    QLabel *m_wineStatusLabel;
    QLabel *m_currentPrefixLabel;
    QLineEdit *m_programNameInput;
    QComboBox *m_winePrefixCombo;
    QLineEdit *m_exePathInput;
    QLineEdit *m_downloadUrlInput;
    QCheckBox *m_installDepsCheck;
    QCheckBox *m_createShortcutCheck;
    QTableWidget *m_installedProgramsTable;
    QTableWidget *m_prefixesTable;
    QLineEdit *m_prefixSearchInput;
    QTextEdit *m_windowsLog;

    // Maintenance widgets
    QLabel *m_systemStatusLabel;
    QComboBox *m_updateTypeCombo;
    QCheckBox *m_downloadOnlyCheck;
    QCheckBox *m_ignoreDepthCheck;
    QTableWidget *m_updatesTable;
    QLabel *m_cacheSizeLabel;
    QLabel *m_cacheLocationLabel;
    QCheckBox *m_autoMaintenanceCheck;
    QSpinBox *m_maintenanceIntervalSpinBox;
    QTextEdit *m_maintenanceLog;

    // Cylon terminal widgets
    QTextEdit *m_cylonOutput;
    QLineEdit *m_cylonInput;
    QPushButton *m_startCylonButton;
    QPushButton *m_stopCylonButton;

    // Installed packages widgets
    QTableWidget *m_installedTable;
    QLabel *m_installedStatusLabel;

    // Settings widgets
    QCheckBox *m_autoUpdateCheck;
    QCheckBox *m_performanceMonitoringCheck;
    QCheckBox *m_confirmInstallsCheck;
    QComboBox *m_mirrorCountryCombo;
    QSpinBox *m_parallelDownloadsSpinBox;
    QComboBox *m_preferredAURHelperCombo;
    QCheckBox *m_enableMultilibCheck;
    QCheckBox *m_cleanCacheAutoCheck;
    QComboBox *m_themeCombo;
    QSpinBox *m_fontSizeSpinBox;
    QLabel *m_systemInfoLabel;

    // Performance monitoring widgets
    QLabel *m_cpuLabel;
    QLabel *m_memoryLabel;
    QLabel *m_diskLabel;

    // Timers
    QTimer *m_performanceTimer;
    QTimer *m_statusUpdateTimer;

    // Install queue
    QList<PackageInfo> m_installQueue;
    bool m_queuePaused;
    int m_currentQueueIndex;
    
    // State tracking
    QList<PackageInfo> m_searchResults;
    QList<PackageInfo> m_installedPackages;
    bool m_cylonRunning;
    
    // UI state
    QString m_currentTheme;
    QStringList m_availableMethods;
    bool m_autoScrollLog;
};

#endif // MAINWINDOW_H
