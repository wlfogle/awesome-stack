#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

class TabWidget;
class SettingsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newTab();
    void closeCurrentTab();
    void openSettings();
    void toggleSidebar();
    void runCommand();
    void updateStatusBar();
    void onTabChanged(int index);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void createActions();
    void connectSignals();

    // UI Components
    TabWidget *m_tabWidget;
    QSplitter *m_mainSplitter;
    QWidget *m_sidebar;
    QTreeWidget *m_fileTree;
    QTextEdit *m_aiAssistant;
    QLineEdit *m_commandInput;
    QPushButton *m_runButton;
    
    // Menu and toolbar actions
    QAction *m_newTabAction;
    QAction *m_closeTabAction;
    QAction *m_settingsAction;
    QAction *m_toggleSidebarAction;
    QAction *m_exitAction;
    
    // Status bar components
    QLabel *m_statusLabel;
    QLabel *m_shellLabel;
    QLabel *m_directoryLabel;
    QProgressBar *m_progressBar;
    
    // Settings
    SettingsDialog *m_settingsDialog;
    
    // Timer for status updates
    QTimer *m_statusTimer;
};

#endif // MAINWINDOW_H
