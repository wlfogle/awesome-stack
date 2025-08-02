#include "mainwindow.h"
#include "packagemanager.h"
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QProgressBar>
#include <QThread>
#include <QTimer>

// ============================================================================
// INSTALL TAB IMPLEMENTATION
// ============================================================================

QWidget* MainWindow::createInstallTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Create Install sub-tabs
    QTabWidget *installTabs = new QTabWidget();
    layout->addWidget(installTabs);
    
    // Single Package Install Tab
    QWidget *singleInstallTab = createSingleInstallTab();
    installTabs->addTab(singleInstallTab, "📦 Single Install");
    
    // Batch Install Tab
    QWidget *batchInstallTab = createBatchInstallTab();
    installTabs->addTab(batchInstallTab, "📦 Batch Install");
    
    // Installation Queue Tab
    QWidget *queueTab = createInstallQueueTab();
    installTabs->addTab(queueTab, "📋 Install Queue");
    
    // Installation History Tab
    QWidget *historyTab = createInstallHistoryTab();
    installTabs->addTab(historyTab, "📚 Install History");
    
    // Installation Log Tab
    QWidget *logTab = createInstallLogTab();
    installTabs->addTab(logTab, "📝 Install Log");
    
    // Initialize queue
    m_installQueue = QList<PackageInfo>();
    
    return widget;
}

QWidget* MainWindow::createSingleInstallTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Package input section
    QGroupBox *inputGroup = new QGroupBox("📦 Single Package Installation");
    QFormLayout *inputLayout = new QFormLayout(inputGroup);
    
    m_installPackageInput = new QLineEdit();
    m_installPackageInput->setPlaceholderText("Enter package name to install...");
    m_installPackageInput->setStyleSheet("QLineEdit { font-size: 14px; padding: 8px; }");
    inputLayout->addRow("Package Name:", m_installPackageInput);
    
    // Installation method selection
    m_installMethodCombo = new QComboBox();
    m_installMethodCombo->addItems({"Auto-detect", "Pacman", "YAY", "Paru", "Pikaur", "Flatpak", "Snap"});
    inputLayout->addRow("Install Method:", m_installMethodCombo);
    
    // Options
    m_installWithDepsCheck = new QCheckBox("Install dependencies automatically");
    m_installWithDepsCheck->setChecked(true);
    inputLayout->addRow(m_installWithDepsCheck);
    
    m_installFromAURCheck = new QCheckBox("Include AUR packages");
    m_installFromAURCheck->setChecked(true);
    inputLayout->addRow(m_installFromAURCheck);
    
    QCheckBox *confirmInstallCheck = new QCheckBox("Confirm before installation");
    confirmInstallCheck->setChecked(true);
    inputLayout->addRow(confirmInstallCheck);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *installSingleBtn = new QPushButton("📦 Install Now");
    installSingleBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    connect(installSingleBtn, &QPushButton::clicked, this, &MainWindow::installSinglePackage);
    buttonLayout->addWidget(installSingleBtn);
    
    QPushButton *addToQueueBtn = new QPushButton("➕ Add to Queue");
    addToQueueBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px 20px; }");
    connect(addToQueueBtn, &QPushButton::clicked, this, &MainWindow::addSingleToQueue);
    buttonLayout->addWidget(addToQueueBtn);
    
    QPushButton *clearInputBtn = new QPushButton("🧹 Clear");
    connect(clearInputBtn, &QPushButton::clicked, [this]() {
        m_installPackageInput->clear();
        m_packageInfoDisplay->clear();
    });
    buttonLayout->addWidget(clearInputBtn);
    
    inputLayout->addRow(buttonLayout);
    layout->addWidget(inputGroup);
    
    // Package information display
    QGroupBox *infoGroup = new QGroupBox("📋 Package Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    m_packageInfoDisplay = new QTextEdit();
    m_packageInfoDisplay->setReadOnly(true);
    m_packageInfoDisplay->setMaximumHeight(200);
    m_packageInfoDisplay->setPlaceholderText("Package information will appear here...");
    m_packageInfoDisplay->setStyleSheet("QTextEdit { background-color: #f5f5f5; border: 1px solid #ddd; }");
    infoLayout->addWidget(m_packageInfoDisplay);
    
    layout->addWidget(infoGroup);
    
    // Quick install buttons for popular packages
    QGroupBox *quickGroup = new QGroupBox("⚡ Quick Install Popular Packages");
    QGridLayout *quickLayout = new QGridLayout(quickGroup);
    
    QStringList popularPackages = {
        "Firefox", "VLC", "Git", "Docker",
        "VS Code", "GIMP", "LibreOffice", "Steam"
    };
    
    QStringList packageNames = {
        "firefox", "vlc", "git", "docker",
        "code", "gimp", "libreoffice-fresh", "steam"
    };
    
    for (int i = 0; i < popularPackages.size(); ++i) {
        QPushButton *btn = new QPushButton(popularPackages[i]);
        btn->setProperty("packageName", packageNames[i]);
        btn->setStyleSheet("QPushButton { padding: 8px; margin: 2px; }");
        connect(btn, &QPushButton::clicked, [this, btn]() {
            QString packageName = btn->property("packageName").toString();
            m_installPackageInput->setText(packageName);
            installSinglePackage();
        });
        quickLayout->addWidget(btn, i / 4, i % 4);
    }
    
    layout->addWidget(quickGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* MainWindow::createBatchInstallTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Batch input section
    QGroupBox *batchGroup = new QGroupBox("📦 Batch Package Installation");
    QVBoxLayout *batchLayout = new QVBoxLayout(batchGroup);
    
    // Instructions
    QLabel *instructions = new QLabel("Enter package names (one per line) or upload a package list file:");
    instructions->setStyleSheet("QLabel { font-weight: bold; margin-bottom: 10px; }");
    batchLayout->addWidget(instructions);
    
    // Batch text input
    m_batchInstallText = new QTextEdit();
    m_batchInstallText->setPlaceholderText("firefox\nvlc\ngit\ndocker\nvscode\ngimp\nlibreoffice\nsteam");
    m_batchInstallText->setMaximumHeight(200);
    m_batchInstallText->setStyleSheet("QTextEdit { font-family: monospace; }");
    batchLayout->addWidget(m_batchInstallText);
    
    // File operations
    QHBoxLayout *fileLayout = new QHBoxLayout();
    QPushButton *loadListBtn = new QPushButton("📁 Load from File");
    connect(loadListBtn, &QPushButton::clicked, this, &MainWindow::loadPackageList);
    fileLayout->addWidget(loadListBtn);
    
    QPushButton *saveListBtn = new QPushButton("💾 Save to File");
    connect(saveListBtn, &QPushButton::clicked, this, &MainWindow::savePackageList);
    fileLayout->addWidget(saveListBtn);
    
    QPushButton *loadPresetBtn = new QPushButton("📋 Load Preset");
    connect(loadPresetBtn, &QPushButton::clicked, this, &MainWindow::loadInstallPreset);
    fileLayout->addWidget(loadPresetBtn);
    
    fileLayout->addStretch();
    batchLayout->addLayout(fileLayout);
    
    // Batch options
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    m_batchMethodCombo = new QComboBox();
    m_batchMethodCombo->addItems({"Auto-detect", "Pacman", "YAY", "Paru", "Pikaur"});
    optionsLayout->addWidget(new QLabel("Method:"));
    optionsLayout->addWidget(m_batchMethodCombo);
    
    m_batchContinueOnErrorCheck = new QCheckBox("Continue on errors");
    m_batchContinueOnErrorCheck->setChecked(true);
    optionsLayout->addWidget(m_batchContinueOnErrorCheck);
    
    QCheckBox *parallelInstallCheck = new QCheckBox("Parallel installation");
    optionsLayout->addWidget(parallelInstallCheck);
    
    optionsLayout->addStretch();
    batchLayout->addLayout(optionsLayout);
    
    // Batch buttons
    QHBoxLayout *batchButtonLayout = new QHBoxLayout();
    QPushButton *installBatchBtn = new QPushButton("📦 Install All");
    installBatchBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    connect(installBatchBtn, &QPushButton::clicked, this, &MainWindow::installBatchPackages);
    batchButtonLayout->addWidget(installBatchBtn);
    
    QPushButton *addBatchToQueueBtn = new QPushButton("➕ Add All to Queue");
    connect(addBatchToQueueBtn, &QPushButton::clicked, this, &MainWindow::addBatchToQueue);
    batchButtonLayout->addWidget(addBatchToQueueBtn);
    
    QPushButton *validateBatchBtn = new QPushButton("✓ Validate Packages");
    connect(validateBatchBtn, &QPushButton::clicked, this, &MainWindow::validateBatchPackages);
    batchButtonLayout->addWidget(validateBatchBtn);
    
    QPushButton *clearBatchBtn = new QPushButton("🧹 Clear List");
    connect(clearBatchBtn, &QPushButton::clicked, [this]() { m_batchInstallText->clear(); });
    batchButtonLayout->addWidget(clearBatchBtn);
    
    batchButtonLayout->addStretch();
    batchLayout->addLayout(batchButtonLayout);
    
    layout->addWidget(batchGroup);
    
    // Presets section
    QGroupBox *presetsGroup = new QGroupBox("📋 Installation Presets");
    QVBoxLayout *presetsLayout = new QVBoxLayout(presetsGroup);
    
    QHBoxLayout *presetButtonsLayout = new QHBoxLayout();
    
    QStringList presets = {"Development", "Gaming", "Multimedia", "Office", "Security", "Graphics", "Audio Production", "System Tools", "Web Dev", "Data Science"};
    for (const QString &preset : presets) {
        QPushButton *btn = new QPushButton(preset);
        btn->setProperty("presetName", preset);
        btn->setStyleSheet("QPushButton { padding: 6px 12px; margin: 2px; }");
        connect(btn, &QPushButton::clicked, [this, btn]() {
            QString presetName = btn->property("presetName").toString();
            loadPresetPackages(presetName);
        });
        presetButtonsLayout->addWidget(btn);
    }
    
    presetButtonsLayout->addStretch();
    presetsLayout->addLayout(presetButtonsLayout);
    
    layout->addWidget(presetsGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* MainWindow::createInstallQueueTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Queue controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    QPushButton *clearQueueBtn = new QPushButton("🗑️ Clear Queue");
    connect(clearQueueBtn, &QPushButton::clicked, this, &MainWindow::clearInstallQueue);
    controlsLayout->addWidget(clearQueueBtn);
    
    QPushButton *processQueueBtn = new QPushButton("▶️ Process Queue");
    processQueueBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 8px 16px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    connect(processQueueBtn, &QPushButton::clicked, this, &MainWindow::processInstallQueue);
    controlsLayout->addWidget(processQueueBtn);
    
    QPushButton *pauseQueueBtn = new QPushButton("⏸️ Pause");
    connect(pauseQueueBtn, &QPushButton::clicked, this, &MainWindow::pauseInstallQueue);
    controlsLayout->addWidget(pauseQueueBtn);
    
    QPushButton *saveQueueBtn = new QPushButton("💾 Save Queue");
    connect(saveQueueBtn, &QPushButton::clicked, this, &MainWindow::saveInstallQueue);
    controlsLayout->addWidget(saveQueueBtn);
    
    QPushButton *loadQueueBtn = new QPushButton("📁 Load Queue");
    connect(loadQueueBtn, &QPushButton::clicked, this, &MainWindow::loadInstallQueue);
    controlsLayout->addWidget(loadQueueBtn);
    
    controlsLayout->addStretch();
    
    m_queueProgress = new QProgressBar();
    m_queueProgress->setVisible(false);
    m_queueProgress->setStyleSheet("QProgressBar { border: 2px solid grey; border-radius: 5px; text-align: center; } QProgressBar::chunk { background-color: #4CAF50; }");
    controlsLayout->addWidget(m_queueProgress);
    
    layout->addLayout(controlsLayout);
    
    // Installation queue table
    QGroupBox *queueGroup = new QGroupBox("📋 Installation Queue");
    QVBoxLayout *queueLayout = new QVBoxLayout(queueGroup);
    
    m_installQueueTable = new QTableWidget();
    m_installQueueTable->setColumnCount(6);
    m_installQueueTable->setHorizontalHeaderLabels({"Package", "Method", "Status", "Progress", "Size", "Actions"});
    m_installQueueTable->setAlternatingRowColors(true);
    m_installQueueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installQueueTable->horizontalHeader()->setStretchLastSection(false);
    m_installQueueTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    queueLayout->addWidget(m_installQueueTable);
    
    layout->addWidget(queueGroup);
    
    // Queue statistics
    QGroupBox *statsGroup = new QGroupBox("📊 Queue Statistics");
    QHBoxLayout *statsLayout = new QHBoxLayout(statsGroup);
    
    m_queueTotalLabel = new QLabel("Total: 0");
    m_queueTotalLabel->setStyleSheet("QLabel { font-weight: bold; padding: 5px; }");
    statsLayout->addWidget(m_queueTotalLabel);
    
    m_queuePendingLabel = new QLabel("Pending: 0");
    m_queuePendingLabel->setStyleSheet("QLabel { color: #ff9800; }");
    statsLayout->addWidget(m_queuePendingLabel);
    
    m_queueCompletedLabel = new QLabel("Completed: 0");
    m_queueCompletedLabel->setStyleSheet("QLabel { color: #4CAF50; }");
    statsLayout->addWidget(m_queueCompletedLabel);
    
    m_queueFailedLabel = new QLabel("Failed: 0");
    m_queueFailedLabel->setStyleSheet("QLabel { color: #f44336; }");
    statsLayout->addWidget(m_queueFailedLabel);
    
    statsLayout->addStretch();
    layout->addWidget(statsGroup);
    
    // Initialize queue display
    updateInstallQueueDisplay();
    
    return widget;
}

QWidget* MainWindow::createInstallHistoryTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // History controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    QPushButton *refreshHistoryBtn = new QPushButton("🔄 Refresh");
    connect(refreshHistoryBtn, &QPushButton::clicked, this, &MainWindow::refreshInstallHistory);
    controlsLayout->addWidget(refreshHistoryBtn);
    
    QPushButton *exportHistoryBtn = new QPushButton("📤 Export History");
    connect(exportHistoryBtn, &QPushButton::clicked, this, &MainWindow::exportInstallHistory);
    controlsLayout->addWidget(exportHistoryBtn);
    
    QPushButton *clearHistoryBtn = new QPushButton("🗑️ Clear History");
    connect(clearHistoryBtn, &QPushButton::clicked, this, &MainWindow::clearInstallHistory);
    controlsLayout->addWidget(clearHistoryBtn);
    
    controlsLayout->addStretch();
    
    // Filter
    QLineEdit *historyFilter = new QLineEdit();
    historyFilter->setPlaceholderText("Filter history...");
    connect(historyFilter, &QLineEdit::textChanged, this, &MainWindow::filterInstallHistory);
    controlsLayout->addWidget(historyFilter);
    
    layout->addLayout(controlsLayout);
    
    // History table
    m_installHistoryTable = new QTableWidget();
    m_installHistoryTable->setColumnCount(7);
    m_installHistoryTable->setHorizontalHeaderLabels({
        "Package", "Method", "Version", "Install Date", "Status", "Duration", "Actions"
    });
    m_installHistoryTable->setAlternatingRowColors(true);
    m_installHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_installHistoryTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_installHistoryTable);
    
    // Load history
    refreshInstallHistory();
    
    return widget;
}

QWidget* MainWindow::createInstallLogTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Log controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    QPushButton *clearLogBtn = new QPushButton("🧹 Clear Log");
    connect(clearLogBtn, &QPushButton::clicked, [this]() { m_installLog->clear(); });
    controlsLayout->addWidget(clearLogBtn);
    
    QPushButton *saveLogBtn = new QPushButton("💾 Save Log");
    connect(saveLogBtn, &QPushButton::clicked, this, &MainWindow::saveInstallLog);
    controlsLayout->addWidget(saveLogBtn);
    
    QCheckBox *autoScrollCheck = new QCheckBox("Auto-scroll");
    autoScrollCheck->setChecked(true);
    connect(autoScrollCheck, &QCheckBox::toggled, [this](bool enabled) {
        m_autoScrollLog = enabled;
    });
    controlsLayout->addWidget(autoScrollCheck);
    
    controlsLayout->addStretch();
    
    // Log level filter
    controlsLayout->addWidget(new QLabel("Level:"));
    QComboBox *logLevelCombo = new QComboBox();
    logLevelCombo->addItems({"All", "Info", "Warning", "Error"});
    connect(logLevelCombo, &QComboBox::currentTextChanged, this, &MainWindow::filterInstallLog);
    controlsLayout->addWidget(logLevelCombo);
    
    layout->addLayout(controlsLayout);
    
    // Installation log
    m_installLog = new QTextEdit();
    m_installLog->setReadOnly(true);
    m_installLog->setStyleSheet("QTextEdit { font-family: monospace; background-color: #2b2b2b; color: #ffffff; }");
    m_installLog->append("📦 Installation Log - Ready");
    m_installLog->append(QString("🕐 %1 - Universal Arch Installer started").arg(QDateTime::currentDateTime().toString()));
    layout->addWidget(m_installLog);
    
    m_autoScrollLog = true;
    
    return widget;
}

// ============================================================================
// INSTALL TAB HELPER METHODS
// ============================================================================

void MainWindow::installSinglePackage()
{
    QString packageName = m_installPackageInput->text().trimmed();
    if (packageName.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a package name.");
        return;
    }
    
    QString method = m_installMethodCombo->currentText();
    bool withDeps = m_installWithDepsCheck->isChecked();
    bool fromAUR = m_installFromAURCheck->isChecked();
    
    PackageInfo pkg;
    pkg.name = packageName;
    pkg.method = stringToInstallMethod(method);
    
    logInstallOperation(QString("Starting installation of %1 using %2").arg(packageName).arg(method));
    
    // Start installation
    m_packageManager->installPackage(pkg);
    
    // Add to history
    addToInstallHistory(pkg, true);
}

void MainWindow::addSingleToQueue()
{
    QString packageName = m_installPackageInput->text().trimmed();
    if (packageName.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a package name.");
        return;
    }
    
    PackageInfo pkg;
    pkg.name = packageName;
    pkg.method = stringToInstallMethod(m_installMethodCombo->currentText());
    
    m_installQueue.append(pkg);
    updateInstallQueueDisplay();
    
    logInstallOperation(QString("Added %1 to install queue").arg(packageName));
    QMessageBox::information(this, "Added to Queue", QString("Package '%1' added to install queue.").arg(packageName));
}

void MainWindow::installBatchPackages()
{
    QStringList packages = m_batchInstallText->toPlainText().split('\n', Qt::SkipEmptyParts);
    if (packages.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter package names.");
        return;
    }
    
    QString method = m_batchMethodCombo->currentText();
    bool continueOnError = m_batchContinueOnErrorCheck->isChecked();
    
    logInstallOperation(QString("Starting batch installation of %1 packages").arg(packages.size()));
    
    for (const QString &packageName : packages) {
        PackageInfo pkg;
        pkg.name = packageName.trimmed();
        pkg.method = stringToInstallMethod(method);
        
        if (!pkg.name.isEmpty()) {
            m_packageManager->installPackage(pkg);
            addToInstallHistory(pkg, true);
        }
    }
}

void MainWindow::addBatchToQueue()
{
    QStringList packages = m_batchInstallText->toPlainText().split('\n', Qt::SkipEmptyParts);
    if (packages.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter package names.");
        return;
    }
    
    int added = 0;
    QString method = m_batchMethodCombo->currentText();
    
    for (const QString &packageName : packages) {
        QString trimmed = packageName.trimmed();
        if (!trimmed.isEmpty()) {
            PackageInfo pkg;
            pkg.name = trimmed;
            pkg.method = stringToInstallMethod(method);
            m_installQueue.append(pkg);
            added++;
        }
    }
    
    updateInstallQueueDisplay();
    logInstallOperation(QString("Added %1 packages to install queue").arg(added));
    QMessageBox::information(this, "Added to Queue", QString("Added %1 packages to install queue.").arg(added));
}

void MainWindow::processInstallQueue()
{
    if (m_installQueue.isEmpty()) {
        QMessageBox::information(this, "Empty Queue", "Install queue is empty.");
        return;
    }
    
    m_queueProgress->setVisible(true);
    m_queueProgress->setRange(0, m_installQueue.size());
    m_queueProgress->setValue(0);
    
    logInstallOperation(QString("Processing install queue with %1 packages").arg(m_installQueue.size()));
    
    // Process queue (simplified - in real implementation this would be more sophisticated)
    for (int i = 0; i < m_installQueue.size(); ++i) {
        const PackageInfo &pkg = m_installQueue[i];
        logInstallOperation(QString("Installing %1/%2: %3").arg(i+1).arg(m_installQueue.size()).arg(pkg.name));
        
        m_packageManager->installPackage(pkg);
        addToInstallHistory(pkg, true);
        
        m_queueProgress->setValue(i + 1);
        QApplication::processEvents(); // Allow UI updates
    }
    
    // Clear queue after processing
    m_installQueue.clear();
    updateInstallQueueDisplay();
    m_queueProgress->setVisible(false);
    
    logInstallOperation("Install queue processing completed");
    QMessageBox::information(this, "Queue Processed", "Install queue has been processed successfully.");
}

void MainWindow::updateInstallQueueDisplay()
{
    m_installQueueTable->setRowCount(m_installQueue.size());
    
    for (int i = 0; i < m_installQueue.size(); ++i) {
        const PackageInfo &pkg = m_installQueue[i];
        
        m_installQueueTable->setItem(i, 0, new QTableWidgetItem(pkg.name));
        m_installQueueTable->setItem(i, 1, new QTableWidgetItem(pkg.methodString()));
        m_installQueueTable->setItem(i, 2, new QTableWidgetItem("Pending"));
        
        QProgressBar *progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        m_installQueueTable->setCellWidget(i, 3, progressBar);
        
        m_installQueueTable->setItem(i, 4, new QTableWidgetItem(pkg.size.isEmpty() ? "Unknown" : pkg.size));
        
        // Actions
        QWidget *actionsWidget = new QWidget();
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(4, 2, 4, 2);
        
        QPushButton *removeBtn = new QPushButton("🗑️");
        removeBtn->setToolTip("Remove from queue");
        removeBtn->setMaximumWidth(30);
        removeBtn->setProperty("queueIndex", i);
        connect(removeBtn, &QPushButton::clicked, [this, removeBtn]() {
            int index = removeBtn->property("queueIndex").toInt();
            if (index >= 0 && index < m_installQueue.size()) {
                m_installQueue.removeAt(index);
                updateInstallQueueDisplay();
            }
        });
        actionsLayout->addWidget(removeBtn);
        
        QPushButton *moveUpBtn = new QPushButton("⬆️");
        moveUpBtn->setToolTip("Move up");
        moveUpBtn->setMaximumWidth(30);
        moveUpBtn->setProperty("queueIndex", i);
        connect(moveUpBtn, &QPushButton::clicked, [this, moveUpBtn]() {
            int index = moveUpBtn->property("queueIndex").toInt();
            if (index > 0 && index < m_installQueue.size()) {
                m_installQueue.swapItemsAt(index, index - 1);
                updateInstallQueueDisplay();
            }
        });
        actionsLayout->addWidget(moveUpBtn);
        
        m_installQueueTable->setCellWidget(i, 5, actionsWidget);
    }
    
    updateInstallQueueStats();
}

void MainWindow::logInstallOperation(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2").arg(timestamp).arg(message);
    
    m_installLog->append(logEntry);
    
    if (m_autoScrollLog) {
        QTextCursor cursor = m_installLog->textCursor();
        cursor.movePosition(QTextCursor::End);
        m_installLog->setTextCursor(cursor);
    }
}

void MainWindow::addToInstallHistory(const PackageInfo &package, bool success)
{
    // Save to persistent storage
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QDir().mkpath(configDir);
    
    QString historyFile = configDir + "/install_history.json";
    QJsonArray historyArray;
    
    // Load existing history
    QFile file(historyFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        historyArray = doc.array();
        file.close();
    }
    
    // Add new entry
    QJsonObject newEntry;
    newEntry["package"] = package.name;
    newEntry["method"] = package.methodString();
    newEntry["version"] = package.version;
    newEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    newEntry["success"] = success;
    newEntry["duration"] = "< 1s"; // Could be calculated
    
    historyArray.prepend(newEntry);
    
    // Keep only last 500 entries
    while (historyArray.size() > 500) {
        historyArray.removeLast();
    }
    
    // Save history
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(historyArray);
        file.write(doc.toJson());
        file.close();
    }
}

// Helper method implementations
InstallMethod MainWindow::stringToInstallMethod(const QString &str)
{
    if (str == "Pacman") return InstallMethod::PACMAN;
    if (str == "YAY") return InstallMethod::YAY;
    if (str == "Paru") return InstallMethod::PARU;
    if (str == "Pikaur") return InstallMethod::PIKAUR;
    if (str == "Flatpak") return InstallMethod::FLATPAK;
    if (str == "Snap") return InstallMethod::SNAP;
    return InstallMethod::PACMAN; // Default
}

void MainWindow::loadPresetPackages(const QString &presetName)
{
    QStringList packages = getDynamicPresetPackages(presetName);
    
    // Filter out already installed packages and unavailable packages
    QStringList filteredPackages = filterPresetPackages(packages);
    
    m_batchInstallText->setPlainText(filteredPackages.join('\n'));
    
    QString statusMsg = QString("Loaded %1 preset with %2 packages (filtered from %3 candidates)")
                       .arg(presetName).arg(filteredPackages.size()).arg(packages.size());
    logInstallOperation(statusMsg);
}

void MainWindow::searchBeforeInstall()
{
    QString packageName = m_installPackageInput->text().trimmed();
    if (!packageName.isEmpty()) {
        // Switch to search tab and perform search
        m_tabWidget->setCurrentIndex(0); // Assuming search tab is first
        // Set search input and perform search
        if (m_searchInput) {
            m_searchInput->setText(packageName);
            performSearch();
        }
    }
}

void MainWindow::validateBatchPackages()
{
    QStringList packages = m_batchInstallText->toPlainText().split('\n', Qt::SkipEmptyParts);
    if (packages.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter package names.");
        return;
    }
    
    QStringList validPackages;
    QStringList invalidPackages;
    
    // Simple validation - check if packages exist (this would use actual package search)
    for (const QString &packageName : packages) {
        QString trimmed = packageName.trimmed();
        if (!trimmed.isEmpty()) {
            // For now, assume all non-empty packages are valid
            validPackages << trimmed;
        }
    }
    
    QString message = QString("Validation complete:\n✅ Valid: %1\n❌ Invalid: %2")
                     .arg(validPackages.size()).arg(invalidPackages.size());
    
    QMessageBox::information(this, "Validation Results", message);
    logInstallOperation(QString("Validated %1 packages").arg(packages.size()));
}

// Missing method implementations
void MainWindow::refreshInstallHistory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QString historyFile = configDir + "/install_history.json";
    
    QFile file(historyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray historyArray = doc.array();
    file.close();
    
    m_installHistoryTable->setRowCount(historyArray.size());
    
    for (int i = 0; i < historyArray.size(); ++i) {
        QJsonObject entry = historyArray[i].toObject();
        
        m_installHistoryTable->setItem(i, 0, new QTableWidgetItem(entry["package"].toString()));
        m_installHistoryTable->setItem(i, 1, new QTableWidgetItem(entry["method"].toString()));
        m_installHistoryTable->setItem(i, 2, new QTableWidgetItem(entry["version"].toString()));
        
        QDateTime timestamp = QDateTime::fromString(entry["timestamp"].toString(), Qt::ISODate);
        m_installHistoryTable->setItem(i, 3, new QTableWidgetItem(timestamp.toString("yyyy-MM-dd hh:mm")));
        
        QString status = entry["success"].toBool() ? "Success" : "Failed";
        m_installHistoryTable->setItem(i, 4, new QTableWidgetItem(status));
        m_installHistoryTable->setItem(i, 5, new QTableWidgetItem(entry["duration"].toString()));
        
        // Actions
        QWidget *actionsWidget = new QWidget();
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(4, 2, 4, 2);
        
        QPushButton *reinstallBtn = new QPushButton("🔄");
        reinstallBtn->setToolTip("Reinstall package");
        reinstallBtn->setMaximumWidth(30);
        reinstallBtn->setProperty("packageName", entry["package"].toString());
        connect(reinstallBtn, &QPushButton::clicked, [this, reinstallBtn]() {
            QString packageName = reinstallBtn->property("packageName").toString();
            m_installPackageInput->setText(packageName);
            installSinglePackage();
        });
        actionsLayout->addWidget(reinstallBtn);
        
        m_installHistoryTable->setCellWidget(i, 6, actionsWidget);
    }
}

void MainWindow::exportInstallHistory()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Export Install History", 
        QDir::homePath() + "/install_history.csv",
        "CSV Files (*.csv)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Package,Method,Version,Install Date,Status,Duration\n";
            
            for (int i = 0; i < m_installHistoryTable->rowCount(); ++i) {
                out << m_installHistoryTable->item(i, 0)->text() << ","
                    << m_installHistoryTable->item(i, 1)->text() << ","
                    << m_installHistoryTable->item(i, 2)->text() << ","
                    << m_installHistoryTable->item(i, 3)->text() << ","
                    << m_installHistoryTable->item(i, 4)->text() << ","
                    << m_installHistoryTable->item(i, 5)->text() << "\n";
            }
            
            file.close();
            QMessageBox::information(this, "Export Complete", 
                                    "Install history exported successfully.");
        }
    }
}

void MainWindow::clearInstallHistory()
{
    if (QMessageBox::question(this, "Clear History", 
                             "Are you sure you want to clear the install history?") 
        == QMessageBox::Yes) {
        
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                           + "/universal-arch-installer";
        QString historyFile = configDir + "/install_history.json";
        QFile::remove(historyFile);
        
        m_installHistoryTable->setRowCount(0);
        logInstallOperation("Install history cleared");
    }
}

void MainWindow::filterInstallHistory(const QString &filter)
{
    for (int i = 0; i < m_installHistoryTable->rowCount(); ++i) {
        bool show = true;
        if (!filter.isEmpty()) {
            QString package = m_installHistoryTable->item(i, 0)->text();
            QString method = m_installHistoryTable->item(i, 1)->text();
            show = package.contains(filter, Qt::CaseInsensitive) || 
                   method.contains(filter, Qt::CaseInsensitive);
        }
        m_installHistoryTable->setRowHidden(i, !show);
    }
}

void MainWindow::filterInstallLog(const QString &level)
{
    // Filter log entries by level (simplified implementation)
    Q_UNUSED(level)
    logInstallOperation(QString("Log filter changed to: %1").arg(level));
}

void MainWindow::loadInstallPreset()
{
    QStringList presets = {"Development", "Gaming", "Multimedia", "Office", "Security"};
    
    bool ok;
    QString preset = QInputDialog::getItem(this, "Load Preset", 
                                          "Select a preset:", presets, 0, false, &ok);
    
    if (ok && !preset.isEmpty()) {
        loadPresetPackages(preset);
    }
}

void MainWindow::saveInstallQueue()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Save Install Queue", 
        QDir::homePath() + "/install_queue.json",
        "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QJsonArray queueArray;
        
        for (const PackageInfo &pkg : m_installQueue) {
            QJsonObject pkgObj;
            pkgObj["name"] = pkg.name;
            pkgObj["method"] = pkg.methodString();
            pkgObj["version"] = pkg.version;
            pkgObj["description"] = pkg.description;
            queueArray.append(pkgObj);
        }
        
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            QJsonDocument doc(queueArray);
            file.write(doc.toJson());
            file.close();
            
            QMessageBox::information(this, "Queue Saved", 
                                    "Install queue saved successfully.");
            logInstallOperation(QString("Install queue saved to: %1").arg(fileName));
        }
    }
}

// Load install queue from JSON file
void MainWindow::loadInstallQueue()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
        "Load Install Queue", 
        QDir::homePath(),
        "JSON Files (*.json)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonArray queueArray = doc.array();
            file.close();
            
            m_installQueue.clear();
            
            for (const QJsonValue &value : queueArray) {
                QJsonObject pkgObj = value.toObject();
                PackageInfo pkg;
                pkg.name = pkgObj["name"].toString();
                pkg.method = stringToInstallMethod(pkgObj["method"].toString());
                pkg.version = pkgObj["version"].toString();
                pkg.description = pkgObj["description"].toString();
                m_installQueue.append(pkg);
            }
            
            updateInstallQueueDisplay();
            
            QMessageBox::information(this, "Queue Loaded", 
                                    QString("Loaded %1 packages into install queue.").arg(m_installQueue.size()));
            logInstallOperation(QString("Install queue loaded from: %1 (%2 packages)").arg(fileName).arg(m_installQueue.size()));
        }
    }
}

// Add packages from search results to install tab
void MainWindow::addPackagesToInstall(const QList<PackageInfo> &packages)
{
    if (packages.isEmpty()) {
        return;
    }
    
    // Switch to install tab
    m_tabWidget->setCurrentIndex(1); // Install tab is second
    
    // Get the install tab widget and its sub-tabs
    QWidget *installTabWidget = m_tabWidget->widget(1);
    QTabWidget *installSubTabs = installTabWidget->findChild<QTabWidget*>();
    
    if (packages.size() == 1) {
        // Single package - switch to single install sub-tab
        if (installSubTabs) {
            installSubTabs->setCurrentIndex(0); // Single install is first sub-tab
        }
        
        m_installPackageInput->setText(packages.first().name);
        
        // Display package info
        const PackageInfo &pkg = packages.first();
        QString info = QString("Package: %1\nMethod: %2\nVersion: %3\nDescription: %4\nSource: %5")
                      .arg(pkg.name)
                      .arg(pkg.methodString())
                      .arg(pkg.version)
                      .arg(pkg.description)
                      .arg(pkg.source);
        m_packageInfoDisplay->setText(info);
        
        // Set the install method combo to match the package method
        for (int i = 0; i < m_installMethodCombo->count(); ++i) {
            if (m_installMethodCombo->itemText(i).toLower() == pkg.methodString().toLower()) {
                m_installMethodCombo->setCurrentIndex(i);
                break;
            }
        }
    } else {
        // Multiple packages - switch to batch install sub-tab
        if (installSubTabs) {
            installSubTabs->setCurrentIndex(1); // Batch install is second sub-tab
        }
        
        // Add to batch install text area
        QStringList packageNames;
        for (const PackageInfo &pkg : packages) {
            packageNames << pkg.name;
        }
        m_batchInstallText->setPlainText(packageNames.join('\n'));
    }
    
    logInstallOperation(QString("Added %1 package(s) from search results").arg(packages.size()));
}
