#ifndef CLEANINSTALLBACKUPRESTORE_WIDGET_H
#define CLEANINSTALLBACKUPRESTORE_WIDGET_H

#include <QWidget>
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
#include <QTimer>
#include <QSettings>
#include <QSlider>
#include <QLineEdit>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDialog>
#include <QRadioButton>
#include <QSet>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>
#include <QIODevice>
#include <QFont>
#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QFutureWatcher>
#include <QtConcurrent>

#include "backupmanager.h"
#include "restoremanager.h"
#include "packagemanager.h"
#include "settingsmanager.h"

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

class CleanInstallBackupRestoreWidget : public QWidget
{
    Q_OBJECT

public:
    CleanInstallBackupRestoreWidget(QWidget *parent = nullptr);
    ~CleanInstallBackupRestoreWidget();

public slots:
    void updateWidgetStatus(const QString &message);

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

    // Settings
    void showBackupCapabilities();
    void showPackageConfigurationDialog();
    void showSettingsConfigurationDialog();

    // Helper methods
    void updatePackageCount();
    void updateUIState(bool backupInProgress);
    void saveWidgetSettings();
    void loadWidgetSettings();

signals:
    void statusMessage(const QString &message);
    void packagesLoaded(const QList<PackageInfo> &packages);

private:
    void setupUI();
    void setupConnections();
    void setupBackupTab();
    void setupRestoreTab();
    void setupPackagesTab();
    void setupSettingsTab();
    void setupLogsTab();

    // Main UI components
    QTabWidget *m_mainSubTabWidget; // Sub-tabs under main Clean Install tab
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
    QTextEdit *m_archiveInfoText;

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

    // Core components
    BackupManager *m_backupManager;
    RestoreManager *m_restoreManager;
    PackageManager *m_packageManager;
    SettingsManager *m_settingsManager;

    // State
    QSettings *m_settings;
    QTimer *m_statusTimer;
    bool m_backupInProgress;
    
    // Async package loading
    QFutureWatcher<QList<PackageInfo>> *m_packageWatcher;
    QMutex m_packageMutex;
};

#endif // CLEANINSTALLBACKUPRESTORE_WIDGET_H
