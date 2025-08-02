#ifndef KERNELTOOLS_WIDGET_H
#define KERNELTOOLS_WIDGET_H

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
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>

class KernelToolsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KernelToolsWidget(QWidget *parent = nullptr);
    ~KernelToolsWidget();

private slots:
    // Download slots
    void refreshKernelList();
    void downloadKernel();
    void selectKernelVersion();
    void showDownloadProgress();
    
    // Configure slots
    void loadKernelConfig();
    void saveKernelConfig();
    void resetKernelConfig();
    void enableKernelOption();
    void disableKernelOption();
    
    // Compile slots
    void startCompilation();
    void stopCompilation();
    void setCompileOptions();
    void showCompileProgress();
    
    // Install slots
    void installKernel();
    void uninstallKernel();
    void setDefaultKernel();
    void updateBootloader();

private:
    void setupUI();
    void setupConnections();
    
    // Tab creation methods
    QWidget* createDownloadTab();
    QWidget* createConfigureTab();
    QWidget* createCompileTab();
    QWidget* createInstallTab();
    
    // Main components
    QTabWidget *m_tabWidget;
    
    // Download tab components
    QComboBox *m_kernelVersionCombo;
    QListWidget *m_availableKernelsList;
    QPushButton *m_refreshKernelsButton;
    QPushButton *m_downloadKernelButton;
    QProgressBar *m_downloadProgressBar;
    QTextEdit *m_downloadStatusText;
    QLabel *m_downloadSizeLabel;
    
    // Configure tab components
    QTextEdit *m_kernelConfigText;
    QTableWidget *m_configOptionsTable;
    QPushButton *m_loadConfigButton;
    QPushButton *m_saveConfigButton;
    QPushButton *m_resetConfigButton;
    QLineEdit *m_configSearchEdit;
    QCheckBox *m_enableModulesCheck;
    QCheckBox *m_enableDebuggingCheck;
    
    // Compile tab components
    QSpinBox *m_compileJobsSpin;
    QComboBox *m_compilerCombo;
    QPushButton *m_startCompileButton;
    QPushButton *m_stopCompileButton;
    QProgressBar *m_compileProgressBar;
    QTextEdit *m_compileLogText;
    QLabel *m_compileStatusLabel;
    QCheckBox *m_cleanBuildCheck;
    
    // Install tab components
    QListWidget *m_installedKernelsList;
    QListWidget *m_compiledKernelsList;
    QPushButton *m_installKernelButton;
    QPushButton *m_uninstallKernelButton;
    QPushButton *m_setDefaultButton;
    QPushButton *m_updateBootloaderButton;
    QLabel *m_currentKernelLabel;
    QTextEdit *m_installLogText;
    
    // Status tracking
    bool m_isDownloading;
    bool m_isCompiling;
    QString m_currentKernelVersion;
};

#endif // KERNELTOOLS_WIDGET_H
