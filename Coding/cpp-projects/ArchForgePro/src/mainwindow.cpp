#include "mainwindow.h"
#include "cleaninstallbackuprestore_widget.h"
#include "softwaremanagement_widget.h"
#include "rgbfancontrol_widget.h"
#include "kerneltools_widget.h"
#include "aiassistant_widget.h"
#include "settings_widget.h"

#include <QApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_mainTabWidget(nullptr)
    , m_cleanInstallWidget(nullptr)
    , m_softwareManagementWidget(nullptr)
    , m_rgbFanControlWidget(nullptr)
    , m_kernelToolsWidget(nullptr)
    , m_aiAssistantWidget(nullptr)
    , m_settingsWidget(nullptr)
{
    setupUI();
    setupMenus();
    setupToolbar();
    setupStatusBar();
    createTabs();
    
    setWindowTitle("ArchForgePro");
    setMinimumSize(1200, 800);
    resize(1400, 1000);
}

MainWindow::~MainWindow()
{
    // Qt will handle cleanup of child widgets automatically
}

void MainWindow::setupUI()
{
    m_mainTabWidget = new QTabWidget(this);
    setCentralWidget(m_mainTabWidget);
}

void MainWindow::setupMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    m_preferencesAction = new QAction("&Preferences", this);
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    connect(m_preferencesAction, &QAction::triggered, this, &MainWindow::showPreferences);
    fileMenu->addAction(m_preferencesAction);
    
    fileMenu->addSeparator();
    
    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(m_exitAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    m_aboutAction = new QAction("&About ArchForgePro", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::aboutApp);
    helpMenu->addAction(m_aboutAction);
}

void MainWindow::setupToolbar()
{
    QToolBar *mainToolBar = addToolBar("Main");
    mainToolBar->addAction(m_preferencesAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_aboutAction);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("ArchForgePro Ready", 2000);
}

void MainWindow::createTabs()
{
    // Main tab 1: Clean Install Backup/Restore
    m_cleanInstallWidget = new CleanInstallBackupRestoreWidget(this);
    m_mainTabWidget->addTab(m_cleanInstallWidget, "Clean Install Backup/Restore");
    
    // Main tab 2: Software Management
    m_softwareManagementWidget = new SoftwareManagementWidget(this);
    m_mainTabWidget->addTab(m_softwareManagementWidget, "Software Management");
    
    // Main tab 3: RGB/Fan Control
    m_rgbFanControlWidget = new RgbFanControlWidget(this);
    m_mainTabWidget->addTab(m_rgbFanControlWidget, "RGB/Fan Control");
    
    // Main tab 4: Kernel Tools
    m_kernelToolsWidget = new KernelToolsWidget(this);
    m_mainTabWidget->addTab(m_kernelToolsWidget, "Kernel Tools");
    
    // Main tab 5: AI Assistant
    m_aiAssistantWidget = new AIAssistantWidget(this);
    m_mainTabWidget->addTab(m_aiAssistantWidget, "AI Assistant");
    
    // Main tab 6: Settings
    m_settingsWidget = new SettingsWidget(this);
    m_mainTabWidget->addTab(m_settingsWidget, "Settings");
}

void MainWindow::aboutApp()
{
    QMessageBox::about(this, "About ArchForgePro",
                      "ArchForgePro v1.0.0\n\n"
                      "A comprehensive Arch Linux management tool\n"
                      "with AI assistance, package management,\n"
                      "system maintenance, and more.\n\n"
                      "Built with Qt6 and C++");
}

void MainWindow::showPreferences()
{
    // Switch to the Settings tab
    m_mainTabWidget->setCurrentWidget(m_settingsWidget);
}
