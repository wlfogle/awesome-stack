#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>

// Forward declarations for main tab widgets
class CleanInstallBackupRestoreWidget;
class SoftwareManagementWidget;
class RgbFanControlWidget;
class KernelToolsWidget;
class AIAssistantWidget;
class SettingsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void aboutApp();
    void showPreferences();

private:
    void setupUI();
    void setupMenus();
    void setupToolbar();
    void setupStatusBar();
    void createTabs();

    // Main components
    QTabWidget *m_mainTabWidget;
    
    // Main tab widgets
    CleanInstallBackupRestoreWidget *m_cleanInstallWidget;
    SoftwareManagementWidget *m_softwareManagementWidget;
    RgbFanControlWidget *m_rgbFanControlWidget;
    KernelToolsWidget *m_kernelToolsWidget;
    AIAssistantWidget *m_aiAssistantWidget;
    SettingsWidget *m_settingsWidget;
    
    // Menu and toolbar actions
    QAction *m_aboutAction;
    QAction *m_preferencesAction;
    QAction *m_exitAction;
};

#endif // MAINWINDOW_H
