#include "mainwindow.h"
#include "tabwidget.h"
#include "settingsdialog.h"
#include <QApplication>
#include <QDir>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QKeySequence>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_sidebar(nullptr)
    , m_fileTree(nullptr)
    , m_aiAssistant(nullptr)
    , m_commandInput(nullptr)
    , m_runButton(nullptr)
    , m_settingsDialog(nullptr)
    , m_statusTimer(nullptr)
{
    setWindowTitle("Warp Terminal GUI");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    createActions();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupCentralWidget();
    connectSignals();

    // Start status timer
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_statusTimer->start(1000); // Update every second

    // Create first tab
    newTab();
}

MainWindow::~MainWindow()
{
}

void MainWindow::createActions()
{
    m_newTabAction = new QAction("&New Tab", this);
    m_newTabAction->setShortcut(QKeySequence::AddTab);
    m_newTabAction->setStatusTip("Create a new terminal tab");

    m_closeTabAction = new QAction("&Close Tab", this);
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setStatusTip("Close current terminal tab");

    m_settingsAction = new QAction("&Settings", this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    m_settingsAction->setStatusTip("Open settings dialog");

    m_toggleSidebarAction = new QAction("Toggle &Sidebar", this);
    m_toggleSidebarAction->setShortcut(QKeySequence("Ctrl+B"));
    m_toggleSidebarAction->setStatusTip("Show/hide the sidebar");
    m_toggleSidebarAction->setCheckable(true);
    m_toggleSidebarAction->setChecked(true);

    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_exitAction->setStatusTip("Exit the application");
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(m_newTabAction);
    fileMenu->addAction(m_closeTabAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_settingsAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(m_toggleSidebarAction);

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About Warp Terminal GUI",
                          "Warp Terminal GUI v1.0.0\n\n"
                          "A modern terminal interface built with Qt6\n"
                          "Features AI assistance and file management");
    });
}

void MainWindow::setupToolBar()
{
    QToolBar *mainToolBar = addToolBar("Main");
    mainToolBar->addAction(m_newTabAction);
    mainToolBar->addAction(m_closeTabAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_settingsAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(m_toggleSidebarAction);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel("Ready");
    m_shellLabel = new QLabel("Shell: fish");
    m_directoryLabel = new QLabel("Dir: " + QDir::currentPath());
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setMaximumWidth(150);

    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->addPermanentWidget(m_shellLabel);
    statusBar()->addPermanentWidget(m_directoryLabel);
}

void MainWindow::setupCentralWidget()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    // Setup sidebar
    m_sidebar = new QWidget();
    m_sidebar->setMaximumWidth(300);
    m_sidebar->setMinimumWidth(200);

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    
    // File tree
    QLabel *filesLabel = new QLabel("Files");
    filesLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    sidebarLayout->addWidget(filesLabel);
    
    m_fileTree = new QTreeWidget();
    m_fileTree->setHeaderLabel("Project Files");
    m_fileTree->setMaximumHeight(300);
    
    // Populate file tree with current directory
    QFileSystemModel *model = new QFileSystemModel();
    model->setRootPath(QDir::currentPath());
    // For simplicity, we'll add some dummy items
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(m_fileTree);
    rootItem->setText(0, "Current Directory");
    QTreeWidgetItem *srcItem = new QTreeWidgetItem(rootItem);
    srcItem->setText(0, "src/");
    QTreeWidgetItem *configItem = new QTreeWidgetItem(rootItem);
    configItem->setText(0, "config/");
    m_fileTree->expandAll();
    
    sidebarLayout->addWidget(m_fileTree);

    // AI Assistant
    QLabel *aiLabel = new QLabel("AI Assistant");
    aiLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    sidebarLayout->addWidget(aiLabel);
    
    m_aiAssistant = new QTextEdit();
    m_aiAssistant->setPlaceholderText("Ask AI for help with commands...");
    m_aiAssistant->setMaximumHeight(200);
    sidebarLayout->addWidget(m_aiAssistant);

    // Command input
    QHBoxLayout *commandLayout = new QHBoxLayout();
    m_commandInput = new QLineEdit();
    m_commandInput->setPlaceholderText("Enter command...");
    m_runButton = new QPushButton("Run");
    commandLayout->addWidget(m_commandInput);
    commandLayout->addWidget(m_runButton);
    sidebarLayout->addLayout(commandLayout);

    sidebarLayout->addStretch();

    // Setup tab widget
    m_tabWidget = new TabWidget();
    
    // Add to splitter
    m_mainSplitter->addWidget(m_sidebar);
    m_mainSplitter->addWidget(m_tabWidget);
    m_mainSplitter->setSizes({250, 1150});
}

void MainWindow::connectSignals()
{
    connect(m_newTabAction, &QAction::triggered, this, &MainWindow::newTab);
    connect(m_closeTabAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    connect(m_toggleSidebarAction, &QAction::triggered, this, &MainWindow::toggleSidebar);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::runCommand);
    connect(m_commandInput, &QLineEdit::returnPressed, this, &MainWindow::runCommand);
    
    connect(m_tabWidget, &TabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

void MainWindow::newTab()
{
    m_tabWidget->addNewTab();
}

void MainWindow::closeCurrentTab()
{
    m_tabWidget->closeCurrentTab();
}

void MainWindow::openSettings()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new SettingsDialog(this);
    }
    m_settingsDialog->exec();
}

void MainWindow::toggleSidebar()
{
    m_sidebar->setVisible(m_toggleSidebarAction->isChecked());
}

void MainWindow::runCommand()
{
    QString command = m_commandInput->text().trimmed();
    if (command.isEmpty()) {
        return;
    }

    // Add command to AI assistant for context
    m_aiAssistant->append("Running: " + command);
    
    // Clear input
    m_commandInput->clear();
    
    // Send command to current terminal tab
    m_tabWidget->executeCommand(command);
    
    m_statusLabel->setText("Executing: " + command);
}

void MainWindow::updateStatusBar()
{
    // Update current directory
    QString currentDir = QDir::currentPath();
    m_directoryLabel->setText("Dir: " + QFileInfo(currentDir).baseName());
    
    // Update status based on current tab
    if (m_tabWidget && m_tabWidget->count() > 0) {
        m_statusLabel->setText("Ready - " + QString::number(m_tabWidget->count()) + " tab(s)");
    }
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index)
    updateStatusBar();
}
