#include "mainwindow.h"
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QWidget>
#include <QLabel>
#include <QFont>
#include <QMessageBox>
#include <QTextStream>
#include <QStandardPaths>
#include <QProcess>
#include <QMovie>
#include <QProgressDialog>
#include <QFileDialog>
#include <QApplication>
#include <QDebug>
#include <QDirIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , chatWidget(nullptr)
    , modelConfigWidget(nullptr)
    , fileManagerWidget(nullptr)
    , progressBar(nullptr)
    , cancelButton(nullptr)
    , worker(nullptr)
    , workerThread(nullptr)
    , folderWatcher(new QFileSystemWatcher(this))
    , statusTimer(new QTimer(this))
{
    setupUi();
    setupConnections();
    setupStatusBar();
    
    // Check Ollama status periodically
    statusTimer->setInterval(30000); // 30 seconds
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::checkOllamaStatus);
    statusTimer->start();
    checkOllamaStatus();
}

void MainWindow::onMessageSent(const QString &message) {
    startAnalysis(message);
}

void MainWindow::onWorkerOutput(const QString &output, const QString &type) {
    chatWidget->addMessage(output, type);
    // Keep progress bar visible during output
    if (!progressBar->isVisible()) {
        progressBar->setVisible(true);
    }
}

void MainWindow::onWorkerFinished() {
    hideProgress();
    chatWidget->addMessage("Analysis complete.", "system");
}

void MainWindow::onWorkerError(const QString &error) {
    hideProgress();
    chatWidget->addMessage(error, "error");
}

void MainWindow::onCancelClicked() {
    if (worker && worker->isRunning()) {
        worker->stopProcessing();
        workerThread->quit();
        workerThread->wait();
    }
    hideProgress();
    chatWidget->addMessage("Operation cancelled.", "system");
}

void MainWindow::onFolderChanged(const QString &path) {
    currentProjectPath = path;
    detectCodeFiles(path);
    QLabel *projectLabel = findChild<QLabel*>("projectLabel");
    if (projectLabel) {
        projectLabel->setText("📁 " + path);
    }
    chatWidget->addMessage("Project folder changed to: " + path, "system");
}

void MainWindow::checkOllamaStatus() {
    // Safety check
    if (!chatWidget) return;
    
    // Check if Ollama is running
    QProcess process;
    process.start("pgrep", QStringList() << "-x" << "ollama");
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        if (modelConfigWidget && modelConfigWidget->getStatusLabel()) {
            modelConfigWidget->getStatusLabel()->setText("✅ Ollama is running");
            modelConfigWidget->getStatusLabel()->setStyleSheet("color: green;");
        }
    } else {
        if (modelConfigWidget && modelConfigWidget->getStatusLabel()) {
            modelConfigWidget->getStatusLabel()->setText("❌ Ollama is not running");
            modelConfigWidget->getStatusLabel()->setStyleSheet("color: red;");
        }
    }
}

void MainWindow::onTerminalRequested() {
    // Open terminal in current project directory
    QString terminalPath = currentProjectPath.isEmpty() ? QDir::homePath() : currentProjectPath;
    
    // Try different terminal emulators in order of preference
    QStringList terminals = {"warp-terminal", "alacritty", "kitty", "gnome-terminal", "konsole", "xterm"};
    
    for (const QString &terminal : terminals) {
        QProcess process;
        if (terminal == "warp-terminal") {
            // Warp terminal with specific project directory
            if (process.startDetached(terminal, QStringList() << "--working-directory" << terminalPath)) {
                chatWidget->addMessage(QString("💻 Opened Warp terminal in: %1").arg(terminalPath), "system");
                return;
            }
        } else if (terminal == "gnome-terminal" || terminal == "konsole") {
            // Standard terminals with working directory
            if (process.startDetached(terminal, QStringList() << "--working-directory" << terminalPath)) {
                chatWidget->addMessage(QString("💻 Opened %1 in: %2").arg(terminal, terminalPath), "system");
                return;
            }
        } else {
            // Try other terminals
            QStringList args;
            if (terminal == "alacritty" || terminal == "kitty") {
                args << "--working-directory" << terminalPath;
            }
            if (process.startDetached(terminal, args)) {
                chatWidget->addMessage(QString("💻 Opened %1 in: %2").arg(terminal, terminalPath), "system");
                return;
            }
        }
    }
    
    // Fallback: open file manager if no terminal found
    QProcess::startDetached("xdg-open", QStringList() << terminalPath);
    chatWidget->addMessage(QString("📁 Opened file manager in: %1 (no terminal found)").arg(terminalPath), "system");
}

void MainWindow::startOllama() {
    // Start Ollama service
    QProcess *process = new QProcess(this);
    if (modelConfigWidget->isContainerModeEnabled()) {
        process->startDetached("distrobox", QStringList() << "enter" << "open-interpreter" << "--" << "ollama" << "serve");
    } else {
        process->startDetached("ollama", QStringList() << "serve");
    }
    chatWidget->addMessage("🚀 Starting Ollama service...", "system");
    
    // Check status after a delay
    QTimer::singleShot(3000, this, &MainWindow::checkOllamaStatus);
}

void MainWindow::setupUi()
{
    setWindowTitle("🤖 Open Interpreter - AI Code Analysis Assistant");
    setMinimumSize(1400, 900);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QSplitter *splitter = new QSplitter(Qt::Horizontal, centralWidget);
    mainLayout->addWidget(splitter);

    // Left panel with enhanced chat
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    
    // Title with project indicator
    QLabel *titleLabel = new QLabel("🤖 AI Code Analysis Assistant");
    titleLabel->setFont(QFont("Arial", 16, QFont::Bold));
    titleLabel->setStyleSheet("color: #007acc; margin: 10px; padding: 5px;");
    leftLayout->addWidget(titleLabel);
    
    // Project path indicator
    QLabel *projectLabel = new QLabel("📁 No project folder selected");
    projectLabel->setObjectName("projectLabel");
    projectLabel->setStyleSheet("color: #888; margin: 5px; font-size: 10pt;");
    leftLayout->addWidget(projectLabel);
    
    chatWidget = new ChatWidget();
    leftLayout->addWidget(chatWidget);
    
    splitter->addWidget(leftPanel);

    // Right panel with tabs
    modelConfigWidget = new ModelConfigWidget;
    fileManagerWidget = new FileManagerWidget;
    
    QTabWidget *rightPanel = new QTabWidget(splitter);
    rightPanel->addTab(modelConfigWidget, "⚙️ AI Settings");
    rightPanel->addTab(fileManagerWidget, "📁 Project Files");
    splitter->addWidget(rightPanel);
    
    splitter->setSizes({900, 500});
}

void MainWindow::setupConnections()
{
    // Chat connections
    connect(chatWidget, &ChatWidget::messageSent, this, &MainWindow::onMessageSent);
    connect(chatWidget, &ChatWidget::buildRequested, this, &MainWindow::onBuildRequested);
    connect(chatWidget, &ChatWidget::testRequested, this, &MainWindow::onTestRequested);
    connect(chatWidget, &ChatWidget::runRequested, this, &MainWindow::onRunRequested);
    
    // Model config connections
    connect(modelConfigWidget, &ModelConfigWidget::checkStatusRequested, this, &MainWindow::checkOllamaStatus);
    connect(modelConfigWidget, &ModelConfigWidget::startOllamaRequested, this, &MainWindow::startOllama);
    
    // File manager connections
    connect(fileManagerWidget, &FileManagerWidget::folderOpened, this, &MainWindow::onFolderChanged);
    
    // Folder watcher
    connect(folderWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onFolderChanged);
}

void MainWindow::setupStatusBar()
{
    // Progress bar (initially hidden)
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setRange(0, 0); // Indeterminate progress
    
    // Cancel button (initially hidden)
    cancelButton = new QPushButton("❌ Cancel");
    cancelButton->setVisible(false);
    cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: none;"
        "    padding: 5px 10px;"
        "    border-radius: 3px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c82333;"
        "}"
    );
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::onCancelClicked);
    
    QStatusBar *statusBar = this->statusBar();
    statusBar->addWidget(progressBar);
    statusBar->addPermanentWidget(cancelButton);
    statusBar->showMessage("Ready - Select a project folder to begin analysis");
}

void MainWindow::detectCodeFiles(const QString &folderPath)
{
    codeFiles.clear();
    QDir directory(folderPath);
    QStringList extensions = getCodeFileExtensions();
    
    // Add file watcher for this directory
    if (!folderWatcher->directories().contains(folderPath)) {
        folderWatcher->addPath(folderPath);
    }
    
    // Recursively find code files
    QDirIterator iterator(folderPath, QDirIterator::Subdirectories);
    int fileCount = 0;
    
    while (iterator.hasNext() && fileCount < 20) { // Limit to 20 files
        QString filePath = iterator.next();
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile() && isCodeFile(filePath)) {
            codeFiles.append(filePath);
            fileCount++;
        }
    }
    
    QString message = QString("📁 Found %1 code files in project").arg(codeFiles.count());
    chatWidget->addMessage(message, "system");
    statusBar()->showMessage(message);
}

QString MainWindow::analyzeCodeAndSelectModel(const QStringList &files)
{
    // Analyze file types and complexity to select best model
    QMap<QString, int> languageCount;
    int totalLines = 0;
    bool hasComplexCode = false;
    
    foreach (const QString &filePath, files) {
        QFileInfo fileInfo(filePath);
        QString extension = fileInfo.suffix().toLower();
        languageCount[extension]++;
        
        // Count lines to estimate complexity
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString content = stream.readAll();
            totalLines += content.split('\n').count();
            
            // Check for complex patterns
            if (content.contains("class") || content.contains("template") || 
                content.contains("namespace") || content.contains("async") ||
                content.contains("import") || content.contains("#include")) {
                hasComplexCode = true;
            }
        }
    }
    
    // Select model based on analysis
    QString selectedModel;
    if (languageCount.contains("cpp") || languageCount.contains("h") || languageCount.contains("hpp") || languageCount.contains("cc") || languageCount.contains("cxx")) {
        selectedModel = hasComplexCode ? "ollama/magicoder:7b" : "ollama/codellama:7b"; // MagiCoder for complex C++
    } else if (languageCount.contains("py")) {
        selectedModel = hasComplexCode ? "ollama/qwen2.5-coder:7b" : "ollama/deepseek-coder:6.7b";
    } else if (languageCount.contains("js") || languageCount.contains("ts") || languageCount.contains("jsx") || languageCount.contains("tsx")) {
        selectedModel = "ollama/codegemma:7b"; // Good for JavaScript/TypeScript
    } else if (languageCount.contains("rs")) {
        selectedModel = "ollama/starcoder2:7b"; // Good for Rust
    } else if (languageCount.contains("go")) {
        selectedModel = "ollama/llama3.1:8b"; // Good for Go
    } else if (totalLines > 1000 || hasComplexCode) {
        selectedModel = "ollama/magicoder:7b"; // Complex projects
    } else {
        selectedModel = "ollama/codellama:7b"; // Default
    }
    
    // Update model selection in UI
    modelConfigWidget->setCurrentModel(selectedModel);
    
    QString analysisMsg = QString("🤖 Selected %1 for %2 lines across %3 languages")
                         .arg(selectedModel.split("/").last())
                         .arg(totalLines)
                         .arg(languageCount.keys().count());
    chatWidget->addMessage(analysisMsg, "system");
    
    return selectedModel;
}

QString MainWindow::createAnalysisPrompt(const QString &message, const QStringList &files, bool fullProject)
{
    QString prompt = "You are an expert C++/Qt code analyst. Analyze this code THOROUGHLY and provide SPECIFIC findings. ";
    prompt += "DO NOT give generic advice - find ACTUAL issues in the code provided.\n\n";
    prompt += "USER REQUEST: " + message + "\n\n";
    
    if (!fullProject && files.count() > 5) {
        prompt += "ANALYSIS MODE: Individual file analysis (limited scope)\n\n";
    } else {
        prompt += "ANALYSIS MODE: Full project analysis\n\n";
    }
    
    int promptSize = 0;
    const int MAX_PROMPT_SIZE = 8000; // Reduced to 8KB to prevent 500 errors
    int filesIncluded = 0;
    
    foreach (const QString &filePath, files) {
        if (filesIncluded >= (fullProject ? 10 : 3)) break; // Reduced file limits
        
        QString fileContent = readFileContent(filePath, fullProject ? 200 : 100); // Reduced line limits
        QString relativePath = QDir(currentProjectPath).relativeFilePath(filePath);
        QString fileSection = QString("\n=== FILE: %1 ===\n%2\n\n")
                             .arg(relativePath.isEmpty() ? QFileInfo(filePath).fileName() : relativePath)
                             .arg(fileContent);
        
        if (promptSize + fileSection.length() > MAX_PROMPT_SIZE) {
            prompt += "\n[Additional files truncated to stay within size limits]\n";
            break;
        }
        
        prompt += fileSection;
        promptSize += fileSection.length();
        filesIncluded++;
    }
    
    // Add structured analysis request with Warp AI functionality
    prompt += "\nYou MUST provide SPECIFIC findings, not generic advice. DO NOT say 'no obvious bugs' - find ACTUAL issues:\n\n";
    prompt += "CRITICAL: For EVERY bug, issue, or problem you find, you MUST specify BOTH the exact file name AND line number in this format:\n";
    prompt += "FILE: filename.ext, LINE: XX - [description of issue]\n\n";
    prompt += "1. 🔍 CODE ANALYSIS: List SPECIFIC design flaws, architectural issues, and violations. ALWAYS include file names and line numbers.\n";
    prompt += "2. 🐛 BUG DETECTION: Find ACTUAL bugs - null pointers, memory leaks, race conditions, logic errors. MANDATORY: State FILE: filename.ext, LINE: XX for each bug.\n";
    prompt += "3. ⚡ OPTIMIZATION: Identify SPECIFIC performance bottlenecks in FILE: filename.ext, LINE: XX format, show BEFORE/AFTER code examples.\n";
    prompt += "4. 🛠️ REFACTORING: Suggest CONCRETE code improvements with FILE: filename.ext, LINE: XX references and examples.\n";
    prompt += "5. 📚 DOCUMENTATION: Generate ACTUAL documentation - function signatures, parameters, examples with file references.\n";
    prompt += "6. 🧪 TESTING: Write SPECIFIC test cases and test code examples.\n";
    prompt += "7. 💻 COMMANDS: Suggest build/test/debug commands for this project type.\n";
    prompt += "\nREMEMBER: NEVER report a line number without the corresponding file name. Use format: FILE: filename.ext, LINE: XX\n";
    prompt += "\nProvide ACTIONABLE, SPECIFIC recommendations with code examples and exact file/line references.";
    
    return prompt;
}

QStringList MainWindow::getCodeFileExtensions()
{
    return QStringList() << "cpp" << "h" << "hpp" << "c" << "cc" << "cxx"
                        << "py" << "pyx" << "pyi"
                        << "js" << "jsx" << "ts" << "tsx"
                        << "java" << "kt" << "scala"
                        << "rs" << "go" << "rb" << "php"
                        << "cs" << "vb" << "fs"
                        << "swift" << "m" << "mm"
                        << "sql" << "r" << "matlab" << "m"
                        << "sh" << "bash" << "zsh" << "fish"
                        << "xml" << "json" << "yaml" << "yml"
                        << "md" << "rst" << "txt";
}

bool MainWindow::isCodeFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    QStringList extensions = getCodeFileExtensions();
    
    // Skip certain directories and files
    QString fileName = fileInfo.fileName();
    if (fileName.startsWith(".") || 
        filePath.contains("/.git/") ||
        filePath.contains("/build/") ||
        filePath.contains("/node_modules/") ||
        filePath.contains("/__pycache__/") ||
        filePath.contains("/target/")) {
        return false;
    }
    
    return extensions.contains(extension);
}

QString MainWindow::readFileContent(const QString &filePath, int maxLines)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("// Error: Could not read file %1").arg(filePath);
    }
    
    QTextStream stream(&file);
    QString content;
    int lineCount = 0;
    
    while (!stream.atEnd() && lineCount < maxLines) {
        QString line = stream.readLine();
        content += line + "\n";
        lineCount++;
    }
    
    if (lineCount >= maxLines && !stream.atEnd()) {
        content += "\n// [File truncated - showing first " + QString::number(maxLines) + " lines]\n";
    }
    
    return content;
}

void MainWindow::startAnalysis(const QString &message, bool fullProject)
{
    if (codeFiles.isEmpty()) {
        chatWidget->addMessage("❌ No code files found. Please open a project folder first.", "error");
        return;
    }
    
    // Clean up previous worker
    if (worker) {
        worker->stopProcessing();
        if (workerThread) {
            workerThread->quit();
            workerThread->wait(3000);
        }
        worker = nullptr;
        workerThread = nullptr;
    }
    
    // Analyze code and select optimal model
    QString selectedModel = analyzeCodeAndSelectModel(codeFiles);
    
    // Create analysis prompt
    QString prompt = createAnalysisPrompt(message, codeFiles, fullProject);
    
    // Calculate estimated analysis time based on content size
    int totalLines = 0;
    foreach (const QString &filePath, codeFiles) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            totalLines += file.readAll().split('\n').count();
        }
    }
    
    int estimatedTime = qMin(60, qMax(10, totalLines / 50)); // 10-60 seconds estimate
    
    // Show progress and start analysis
    showProgress(QString("🔍 Analyzing %1 files (%2 lines) with %3... Est. %4s")
                .arg(codeFiles.count())
                .arg(totalLines)
                .arg(selectedModel.split("/").last())
                .arg(estimatedTime));
    
    // Create worker in separate thread
    worker = new InterpreterWorker(prompt, selectedModel, modelConfigWidget->isContainerModeEnabled());
    workerThread = new QThread();
    worker->moveToThread(workerThread);
    
    // Connect signals
    connect(workerThread, &QThread::started, worker, &InterpreterWorker::startProcessing);
    connect(worker, &InterpreterWorker::outputReceived, this, &MainWindow::onWorkerOutput);
    connect(worker, &InterpreterWorker::processingFinished, this, &MainWindow::onWorkerFinished);
    connect(worker, &InterpreterWorker::errorOccurred, this, &MainWindow::onWorkerError);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    
    // Start processing
    qDebug() << "[DEBUG] Starting worker thread...";
    workerThread->start();
    
    qDebug() << "[DEBUG] Worker thread started, prompt size:" << prompt.length();
    chatWidget->addMessage(QString("🚀 Starting analysis of %1 files...").arg(codeFiles.count()), "system");
}

void MainWindow::showProgress(const QString &message)
{
    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Indeterminate progress
    cancelButton->setVisible(true);
    statusBar()->showMessage(message);
    
    // Disable send button during processing
    chatWidget->getSendButton()->setEnabled(false);
}

void MainWindow::hideProgress()
{
    progressBar->setVisible(false);
    cancelButton->setVisible(false);
    statusBar()->showMessage("Ready");
    
    // Re-enable send button
    chatWidget->getSendButton()->setEnabled(true);
}

void MainWindow::onBuildRequested()
{
    if (currentProjectPath.isEmpty()) {
        chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        return;
    }
    
    // Show progress
    showProgress("🔨 Building project...");
    
    // Detect build system and build
    QDir projectDir(currentProjectPath);
    QProcess *buildProcess = new QProcess(this);
    
    QString buildCommand;
    QStringList buildArgs;
    
    // Check for different build systems
    if (projectDir.exists("CMakeLists.txt")) {
        // CMake project
        buildCommand = "cmake";
        QString buildDir = projectDir.absolutePath() + "/build";
        QDir().mkpath(buildDir);
        buildArgs << "--build" << buildDir;
        chatWidget->addMessage("🔨 Detected CMake project, building...", "system");
    } else if (projectDir.exists("Makefile")) {
        // Make project
        buildCommand = "make";
        buildProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🔨 Detected Makefile, building...", "system");
    } else if (projectDir.exists("setup.py")) {
        // Python project
        buildCommand = "python";
        buildArgs << "setup.py" << "build";
        buildProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🔨 Detected Python project, building...", "system");
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        buildCommand = "npm";
        buildArgs << "run" << "build";
        buildProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🔨 Detected Node.js project, building...", "system");
    } else {
        hideProgress();
        chatWidget->addMessage("❌ No supported build system found (CMake, Make, Python, Node.js)", "error");
        return;
    }
    
    // Connect process signals
    connect(buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, buildProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                hideProgress();
                if (exitCode == 0) {
                    chatWidget->addMessage("✅ Build completed successfully!", "system");
                } else {
                    QString error = buildProcess->readAllStandardError();
                    chatWidget->addMessage(QString("❌ Build failed with exit code %1\n%2").arg(exitCode).arg(error), "error");
                }
                buildProcess->deleteLater();
            });
    
    connect(buildProcess, &QProcess::readyReadStandardOutput, [this, buildProcess]() {
        QString output = buildProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty()) {
            chatWidget->addMessage(output.trimmed(), "system");
        }
    });
    
    // Start build process
    buildProcess->start(buildCommand, buildArgs);
    
    if (!buildProcess->waitForStarted()) {
        hideProgress();
        chatWidget->addMessage(QString("❌ Failed to start build command: %1").arg(buildCommand), "error");
        buildProcess->deleteLater();
    }
}

void MainWindow::onRunRequested()
{
    if (currentProjectPath.isEmpty()) {
        chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        return;
    }
    
    QDir projectDir(currentProjectPath);
    QString runCommand;
    QStringList runArgs;
    QString workingDir = currentProjectPath;
    
    // Detect and run based on project type
    if (projectDir.exists("CMakeLists.txt")) {
        // CMake project - look for executable
        QString buildDir = projectDir.absolutePath() + "/build";
        QDir buildDirObj(buildDir);
        QStringList executables = buildDirObj.entryList(QStringList() << "*", QDir::Files | QDir::Executable);
        
        if (!executables.isEmpty()) {
            runCommand = buildDir + "/" + executables.first();
            chatWidget->addMessage(QString("▶️ Running: %1").arg(executables.first()), "system");
        } else {
            chatWidget->addMessage("❌ No executable found in build directory. Build the project first.", "error");
            return;
        }
    } else if (projectDir.exists("main.py")) {
        // Python project with main.py
        runCommand = "python";
        runArgs << "main.py";
        chatWidget->addMessage("▶️ Running Python project: main.py", "system");
    } else if (projectDir.exists("app.py")) {
        // Python Flask/Django app
        runCommand = "python";
        runArgs << "app.py";
        chatWidget->addMessage("▶️ Running Python app: app.py", "system");
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        runCommand = "npm";
        runArgs << "start";
        chatWidget->addMessage("▶️ Running Node.js project: npm start", "system");
    } else {
        chatWidget->addMessage("❌ No runnable entry point found (executable, main.py, app.py, package.json)", "error");
        return;
    }
    
    // Start the application in a new terminal
    QStringList terminalCommands = {"warp-terminal", "gnome-terminal", "konsole", "xterm"};
    
    for (const QString &terminal : terminalCommands) {
        QProcess process;
        QStringList terminalArgs;
        
        if (terminal == "warp-terminal") {
            terminalArgs << "--working-directory" << workingDir << "--" << runCommand;
            terminalArgs.append(runArgs);
        } else if (terminal == "gnome-terminal") {
            terminalArgs << "--working-directory" << workingDir << "--" << runCommand;
            terminalArgs.append(runArgs);
        } else if (terminal == "konsole") {
            terminalArgs << "--workdir" << workingDir << "-e" << runCommand;
            terminalArgs.append(runArgs);
        } else {
            terminalArgs << "-e" << runCommand;
            terminalArgs.append(runArgs);
        }
        
        if (process.startDetached(terminal, terminalArgs)) {
            chatWidget->addMessage(QString("▶️ Application started in %1").arg(terminal), "system");
            return;
        }
    }
    
    // Fallback: run in background and show output
    QProcess *runProcess = new QProcess(this);
    runProcess->setWorkingDirectory(workingDir);
    
    connect(runProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, runProcess](int exitCode) {
                chatWidget->addMessage(QString("📋 Application finished with exit code %1").arg(exitCode), "system");
                runProcess->deleteLater();
            });
    
    connect(runProcess, &QProcess::readyReadStandardOutput, [this, runProcess]() {
        QString output = runProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty()) {
            chatWidget->addMessage("📋 " + output.trimmed(), "system");
        }
    });
    
    runProcess->start(runCommand, runArgs);
    
    if (runProcess->waitForStarted()) {
        chatWidget->addMessage("▶️ Application started (running in background)", "system");
    } else {
        chatWidget->addMessage(QString("❌ Failed to start: %1").arg(runCommand), "error");
        runProcess->deleteLater();
    }
}

void MainWindow::onTestRequested()
{
    if (currentProjectPath.isEmpty()) {
        chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        return;
    }
    
    // Show progress
    showProgress("🧪 Running tests...");
    
    QDir projectDir(currentProjectPath);
    QProcess *testProcess = new QProcess(this);
    QString testCommand;
    QStringList testArgs;
    
    // Detect test framework and run appropriate tests
    if (projectDir.exists("CMakeLists.txt")) {
        // CMake project - try CTest
        testCommand = "ctest";
        testArgs << "--test-dir" << projectDir.absolutePath() + "/build";
        testProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🧪 Running CMake tests (CTest)...", "system");
    } else if (projectDir.exists("pytest.ini") || projectDir.exists("test_*.py")) {
        // Python project with pytest
        testCommand = "pytest";
        testArgs << "-v";
        testProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🧪 Running Python tests (pytest)...", "system");
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        testCommand = "npm";
        testArgs << "test";
        testProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🧪 Running Node.js tests (npm test)...", "system");
    } else if (projectDir.exists("Makefile")) {
        // Make project - try 'make test'
        testCommand = "make";
        testArgs << "test";
        testProcess->setWorkingDirectory(currentProjectPath);
        chatWidget->addMessage("🧪 Running Make tests (make test)...", "system");
    } else {
        hideProgress();
        chatWidget->addMessage("❌ No supported test framework found (CTest, pytest, npm test, make test)", "error");
        return;
    }
    
    // Connect process signals
    connect(testProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, testProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                hideProgress();
                if (exitCode == 0) {
                    chatWidget->addMessage("✅ All tests passed successfully!", "system");
                } else {
                    QString error = testProcess->readAllStandardError();
                    chatWidget->addMessage(QString("❌ Tests failed with exit code %1\n%2").arg(exitCode).arg(error), "error");
                }
                testProcess->deleteLater();
            });
    
    connect(testProcess, &QProcess::readyReadStandardOutput, [this, testProcess]() {
        QString output = testProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty()) {
            chatWidget->addMessage("🧪 " + output.trimmed(), "system");
        }
    });
    
    connect(testProcess, &QProcess::readyReadStandardError, [this, testProcess]() {
        QString error = testProcess->readAllStandardError();
        if (!error.trimmed().isEmpty()) {
            chatWidget->addMessage("⚠️ " + error.trimmed(), "error");
        }
    });
    
    // Start test process
    testProcess->start(testCommand, testArgs);
    
    if (!testProcess->waitForStarted()) {
        hideProgress();
        chatWidget->addMessage(QString("❌ Failed to start test command: %1").arg(testCommand), "error");
        testProcess->deleteLater();
    }
}

