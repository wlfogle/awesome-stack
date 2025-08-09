#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QSplitter>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QTimer>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QSlider>
#include <QLineEdit>
#include <QTableWidget>
#include <QFileDialog>
#include <QCloseEvent>

#include "backupmanager.h"
#include "restoremanager.h"
#include "packagemanager.h"
#include "settingsmanager.h"
#include "rgbfancontrol.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Backup operations
    void startPackageBackup();
    void startSettingsBackup();
    void pauseBackup();
    void cancelBackup();
    
    // Restore operations
    void showRestoreDialog();
    void startRestore();
    void previewRestore();
    
    // Package management
    void refreshPackageList();
    void exportPackageList();
    void importPackageList();
    void selectAllPackages();
    void deselectAllPackages();
    
    // Settings management
    void refreshSettingsList();
    void selectAllSettings();
    void deselectAllSettings();
    void exportSettings();
    void importSettings();
    
    
    // UI updates
    void updateProgress(int percentage);
    void updateStatus(const QString &message);
    void onBackupComplete(bool success);
    void onRestoreComplete(bool success);
    void showLogDetails();
    void clearLogs();
    
    // System tray
    void showMainWindow();
    void minimizeToTray();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    
    // Settings
    void saveSettings();
    void loadSettings();
    void showPreferences();
    void showBackupCapabilities();
    void showPackageConfigurationDialog();
    void showSettingsConfigurationDialog();
    void checkAndInstallMonitoringDaemon();
    bool isMonitoringDaemonInstalled();
    bool isMonitoringDaemonRunning();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupBackupTab();
    void setupRestoreTab();
    void setupLogsTab();
    void connectSignals();
    void updateUIState(bool backupInProgress);
    
    // Main UI components
    QTabWidget *m_tabWidget;
    QTabWidget *m_mainSubTabWidget; // Sub-tabs under main Clean Install tab
    QStatusBar *m_statusBar;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QPushButton *m_settingsBtn; // Settings button to show backup capabilities
    
    // Backup tab
    QWidget *m_backupTab;
    QPushButton *m_packageBackupBtn;
    QPushButton *m_settingsBackupBtn;
    QPushButton *m_pauseBtn;
    QPushButton *m_cancelBtn;
    QProgressBar *m_backupProgress;
    QLabel *m_backupStatusLabel;
    QTextEdit *m_backupLog;
    QComboBox *m_compressionCombo;
    QSlider *m_compressionSlider;
    QCheckBox *m_verifyCheckBox;
    QLineEdit *m_backupLocationEdit;
    QPushButton *m_browseLocationBtn;
    
    // Restore tab
    QWidget *m_restoreTab;
    QTreeWidget *m_restorePointsTree;
    QPushButton *m_restoreBtn;
    QPushButton *m_previewBtn;
    QPushButton *m_deleteRestorePointBtn;
    QTextEdit *m_restorePreview;
    QCheckBox *m_restorePackagesCheck;
    QCheckBox *m_restoreSettingsCheck;
    QCheckBox *m_restoreUserDataCheck;
    QLineEdit *m_archivePathEdit;
    QTextEdit *m_archiveInfoText;
    QProgressBar *m_restoreProgress;
    QLabel *m_restoreStatusLabel;
    QTextEdit *m_restoreLog;
    
    // Packages tab
    QWidget *m_packagesTab;
    QTreeWidget *m_packagesTree;
    QPushButton *m_refreshPackagesBtn;
    QPushButton *m_selectAllPackagesBtn;
    QPushButton *m_deselectAllPackagesBtn;
    QPushButton *m_exportPackagesBtn;
    QPushButton *m_importPackagesBtn;
    QLineEdit *m_packageSearchEdit;
    QLabel *m_packageCountLabel;
    
    // Settings tab
    QWidget *m_settingsTab;
    QTreeWidget *m_settingsTree;
    QPushButton *m_refreshSettingsBtn;
    QPushButton *m_selectAllSettingsBtn;
    QPushButton *m_deselectAllSettingsBtn;
    QPushButton *m_exportSettingsBtn;
    QPushButton *m_importSettingsBtn;
    QLineEdit *m_settingsSearchEdit;
    
    
    
    // Logs tab
    QWidget *m_logsTab;
    QTextEdit *m_logsText;
    QPushButton *m_clearLogsBtn;
    QPushButton *m_exportLogsBtn;
    QComboBox *m_logLevelCombo;
    
    // RGB/Fan Control tab
    RGBFanControl *m_rgbFanControl;
    
    // Core components
    BackupManager *m_backupManager;
    RestoreManager *m_restoreManager;
    PackageManager *m_packageManager;
    SettingsManager *m_settingsManager;
    
    // State
    QSettings *m_settings;
    QTimer *m_statusTimer;
    bool m_backupInProgress;
    bool m_minimizeToTray;
};

#endif // MAINWINDOW_H
