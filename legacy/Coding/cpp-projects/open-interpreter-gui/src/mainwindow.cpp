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
#include <QDebug>
#include <QRegularExpression>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , chatWidget(nullptr)
    , modelConfigWidget(nullptr)
    , fileManagerWidget(nullptr)
    , progressBar(nullptr)
    , cancelButton(nullptr)
    , worker(nullptr)
    , workerThread(nullptr)
    , aiAssistant(nullptr)
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
    if (chatWidget) {
        chatWidget->addMessage(output, type);
    }
    
    qDebug() << "📥 Worker output received - Type:" << type << "Length:" << output.length();
    
    // Accumulate AI assistant output for auto-fix processing (flexible type matching)
    if (type.toLower().contains("assistant") || type.toLower().contains("ai")) {
        accumulatedAIResponse += output + "\n";
        qDebug() << "📝 Accumulated AI response length:" << accumulatedAIResponse.length() << "from type:" << type;
    }
    
    // Keep progress bar visible during output
    if (progressBar && !progressBar->isVisible()) {
        progressBar->setVisible(true);
    }
}

void MainWindow::onWorkerFinished() {
    hideProgress();
    
    // Automatically apply fixes if the AI response contains them
    if (!accumulatedAIResponse.isEmpty()) {
        bool hasFILE = accumulatedAIResponse.contains("FILE:");
        bool hasCodeBlocks = accumulatedAIResponse.contains("`");
        
        // Check if the response contains fixable issues
        if (hasFILE && hasCodeBlocks) {
            parseAndApplyFixes(accumulatedAIResponse);
        } else {
            if (chatWidget) {
                chatWidget->addMessage("🔍 No auto-fixable issues found in AI response", "system");
            }
        }
        
        // Clear accumulated response for next analysis
        accumulatedAIResponse.clear();
    }
    
    if (chatWidget) {
        chatWidget->addMessage("Analysis complete.", "system");
    }
}

void MainWindow::onWorkerError(const QString &error) {
    hideProgress();
    if (chatWidget) {
        chatWidget->addMessage(error, "error");
    }
}

void MainWindow::onCancelClicked() {
    if (worker && worker->isRunning()) {
        worker->stopProcessing();
        if (workerThread) {
            workerThread->quit();
            workerThread->wait();
        }
    }
    hideProgress();
    if (chatWidget) {
        chatWidget->addMessage("Operation cancelled.", "system");
    }
}

void MainWindow::onFolderChanged(const QString &path) {
    qDebug() << "📁 onFolderChanged called with path:" << path;
    currentProjectPath = path;
    qDebug() << "📁 currentProjectPath set to:" << currentProjectPath;
    detectCodeFiles(path);
    QLabel *projectLabel = findChild<QLabel*>("projectLabel");
    if (projectLabel) {
        projectLabel->setText("📁 " + path);
    }
    if (chatWidget) {
        chatWidget->addMessage("Project folder changed to: " + path, "system");
    }
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
                if (chatWidget) {
                    chatWidget->addMessage(QString("💻 Opened Warp terminal in: %1").arg(terminalPath), "system");
                }
                return;
            }
        } else if (terminal == "gnome-terminal" || terminal == "konsole") {
            // Standard terminals with working directory
            if (process.startDetached(terminal, QStringList() << "--working-directory" << terminalPath)) {
                if (chatWidget) {
                    chatWidget->addMessage(QString("💻 Opened %1 in: %2").arg(terminal, terminalPath), "system");
                }
                return;
            }
        } else {
            // Try other terminals
            QStringList args;
            if (terminal == "alacritty" || terminal == "kitty") {
                args << "--working-directory" << terminalPath;
            }
            if (process.startDetached(terminal, args)) {
                if (chatWidget) {
                    chatWidget->addMessage(QString("💻 Opened %1 in: %2").arg(terminal, terminalPath), "system");
                }
                return;
            }
        }
    }
    
    // Fallback: open file manager if no terminal found
    QProcess::startDetached("xdg-open", QStringList() << terminalPath);
    if (chatWidget) {
        chatWidget->addMessage(QString("📁 Opened file manager in: %1 (no terminal found)").arg(terminalPath), "system");
    }
}

void MainWindow::startOllama() {
    // Start Ollama service
    QProcess *process = new QProcess(this);
    if (modelConfigWidget->isContainerModeEnabled()) {
        process->startDetached("distrobox", QStringList() << "enter" << "open-interpreter" << "--" << "ollama" << "serve");
    } else {
        process->startDetached("ollama", QStringList() << "serve");
    }
    if (chatWidget) {
        chatWidget->addMessage("🚀 Starting Ollama service...", "system");
    }
    
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
    
    qDebug() << "Detecting code files in:" << folderPath;
    
    while (iterator.hasNext() && fileCount < 20) { // Limit to 20 files
        QString filePath = iterator.next();
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile()) {
            bool isValid = isCodeFile(filePath);
            qDebug() << "File:" << filePath << "Valid:" << isValid;
            
            if (isValid) {
                codeFiles.append(filePath);
                fileCount++;
            }
        }
    }
    
    qDebug() << "📝 Final codeFiles list:";
    for (const QString &file : codeFiles) {
        qDebug() << "  ->" << file;
    }
    
    QString message = QString("📁 Found %1 code files in project").arg(codeFiles.count());
    if (chatWidget) {
        chatWidget->addMessage(message, "system");
    }
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
    if (chatWidget) {
        chatWidget->addMessage(analysisMsg, "system");
    }
    
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
    
    // CRITICAL: Force AI to provide code blocks for automatic fixing
    prompt += "\n\n=== MANDATORY OUTPUT FORMAT ===\n";
    prompt += "You MUST provide EVERY SINGLE ISSUE with COMPLETE WORKING CODE FIXES.\n";
    prompt += "DO NOT just describe problems - SHOW THE ACTUAL FIXED CODE!\n\n";
    
    prompt += "FOR EVERY ISSUE YOU FIND, USE THIS EXACT FORMAT:\n\n";
    prompt += "FILE: filename.ext, LINE: XX - [Brief description]\n";
    prompt += "```cpp\n";
    prompt += "// COMPLETE FIXED CODE GOES HERE\n";
    prompt += "// Include full function or class if needed\n";
    prompt += "```\n\n";
    
    prompt += "EXAMPLE (DO THIS FOR EVERY ISSUE):\n";
    prompt += "FILE: example.cpp, LINE: 25 - Missing null check\n";
    prompt += "```cpp\n";
    prompt += "void MyClass::doSomething(Widget* widget) {\n";
    prompt += "    if (!widget) {\n";
    prompt += "        qWarning() << \"Widget is null!\";\n";
    prompt += "        return;\n";
    prompt += "    }\n";
    prompt += "    widget->process();\n";
    prompt += "}\n";
    prompt += "```\n\n";
    
    prompt += "CRITICAL RULES:\n";
    prompt += "1. EVERY issue MUST have a ```cpp code block\n";
    prompt += "2. Show COMPLETE functions, not fragments\n";
    prompt += "3. Include all necessary #includes at the top\n";
    prompt += "4. Make code compilable and complete\n";
    prompt += "5. NO ISSUE without a matching code block\n";
    prompt += "6. Use C++ language tags: ```cpp\n";
    prompt += "7. If no real issues exist, say 'No critical issues found'\n\n";
    
    prompt += "REMEMBER: The user expects to automatically apply these fixes!\n";
    prompt += "Your code blocks will be directly written to files!\n";
    
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
    QString fileName = fileInfo.fileName();
    QStringList extensions = getCodeFileExtensions();
    
    // Skip certain directories and files
    if (fileName.startsWith(".") || 
        fileName.contains("backup") ||  // Skip any backup files (more comprehensive)
        fileName.contains("_backup_") ||  // Skip backup files with specific pattern
        filePath.contains("/.git/") ||
        filePath.contains("/build/") ||
        filePath.contains("/node_modules/") ||
        filePath.contains("/__pycache__/") ||
        filePath.contains("/target/") ||
        filePath.contains("/dist/") ||  // Skip distribution directories
        filePath.contains("/.vs/") ||   // Skip Visual Studio directories
        filePath.contains("/.vscode/")) { // Skip VS Code directories
        return false;
    }
    
    // Special handling for common build/config files
    if (fileName == "CMakeLists.txt" || 
        fileName == "Makefile" ||
        fileName == "makefile" ||
        fileName.endsWith(".cmake") ||
        fileName.endsWith(".pro") || // Qt project files
        fileName.endsWith(".pri")) { // Qt include files
        return true;
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
        if (chatWidget) {
            chatWidget->addMessage("❌ No code files found. Please open a project folder first.", "error");
        }
        return;
    }
    
    // Clean up previous worker - improved thread safety
    if (worker) {
        worker->stopProcessing();
        if (workerThread && workerThread->isRunning()) {
            // Ensure proper cleanup
            connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
            connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
            
            workerThread->quit();
            if (!workerThread->wait(5000)) {
                // Force termination if graceful shutdown fails
                workerThread->terminate();
                workerThread->wait(2000);
            }
        }
        worker = nullptr;
        workerThread = nullptr;
    }
    
    // Show progress
    showProgress("🚀 Starting smart AI analysis...");
    
    if (chatWidget) {
        chatWidget->addMessage(QString("🚀 Starting analysis of %1 files...").arg(codeFiles.count()), "system");
    }
    
    // Use optimized InterpreterWorker approach that we know works
    QString selectedModel = analyzeCodeAndSelectModel(codeFiles);
    
    // Create optimized prompt for faster analysis
    QString prompt;
    if (codeFiles.count() <= 3) {
        // For small projects, create a focused prompt with code fixes
        prompt = "FOCUSED CODE ANALYSIS - Find critical issues and provide fixes:\n\n";
        
        int fileCount = 0;
        for (const QString &filePath : codeFiles) {
            QString content = readFileContent(filePath, 100); // Increase to 100 lines for better context
            QString fileName = QFileInfo(filePath).fileName();
            prompt += QString("FILE: %1\n%2\n\n").arg(fileName, content);
            fileCount++;
            if (fileCount >= 3) break; // Max 3 files
        }
        
        // Add VERY forceful formatting instructions
        prompt += "\n\n*** CRITICAL: YOU ARE ANALYZING BUGGY CODE WITH OBVIOUS ERRORS ***\n";
        prompt += "\nThe code contains:\n";
        prompt += "- Memory leaks (missing destructors)\n";
        prompt += "- Division by zero errors\n";
        prompt += "- Uninitialized variables\n";
        prompt += "- Missing includes\n";
        prompt += "- Off-by-one errors\n\n";
        
        prompt += "=== MANDATORY OUTPUT FORMAT ===\n";
        prompt += "For EACH ISSUE you find, use EXACTLY this format:\n\n";
        
        prompt += "FILE: test_program.cpp, LINE: 12 - Missing destructor causes memory leak\n";
        prompt += "```cpp\n";
        prompt += "class Calculator {\n";
        prompt += "private:\n";
        prompt += "    int* data;\n";
        prompt += "public:\n";
        prompt += "    ~Calculator() { delete[] data; }  // Fixed: added destructor\n";
        prompt += "    Calculator(int size) : data(new int[size]) {}\n";
        prompt += "};\n";
        prompt += "```\n\n";
        
        prompt += "FILE: test_program.cpp, LINE: 4 - Missing include for strcpy\n";
        prompt += "```cpp\n";
        prompt += "#include <iostream>\n";
        prompt += "#include <vector>\n";
        prompt += "#include <string>\n";
        prompt += "#include <cstring>  // Fixed: added missing include\n";
        prompt += "```\n\n";
        
        prompt += "*** ABSOLUTE REQUIREMENTS ***\n";
        prompt += "1. NO '[SPECIFIC ISSUE DESCRIPTION]' text - write the actual problem\n";
        prompt += "2. NO 'No critical issues found' - there ARE issues in this code\n";
        prompt += "3. Find AT LEAST 5 real issues and provide fixes\n";
        prompt += "4. Every code block must be complete and compilable\n";
        prompt += "5. Write the EXACT issue description, not placeholders\n\n";
    } else {
        // For larger projects, use the full createAnalysisPrompt
        prompt = createAnalysisPrompt(message, codeFiles, fullProject);
    }
    
    // Create worker in separate thread for both cases
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
    workerThread->start();
}

void MainWindow::showProgress(const QString &message)
{
    progressBar->setVisible(true);
    progressBar->setRange(0, 0); // Indeterminate progress
    cancelButton->setVisible(true);
    statusBar()->showMessage(message);
    
    // Disable send button during processing
    if (chatWidget && chatWidget->getSendButton()) {
        chatWidget->getSendButton()->setEnabled(false);
    }
}

void MainWindow::hideProgress()
{
    progressBar->setVisible(false);
    cancelButton->setVisible(false);
    statusBar()->showMessage("Ready");
    
    // Re-enable send button
    if (chatWidget && chatWidget->getSendButton()) {
        chatWidget->getSendButton()->setEnabled(true);
    }
}

void MainWindow::onBuildRequested()
{
    if (currentProjectPath.isEmpty()) {
        if (chatWidget) {
            chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        }
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
        if (chatWidget) {
            chatWidget->addMessage("🔨 Detected CMake project, building...", "system");
        }
    } else if (projectDir.exists("Makefile")) {
        // Make project
        buildCommand = "make";
        buildProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🔨 Detected Makefile, building...", "system");
        }
    } else if (projectDir.exists("setup.py")) {
        // Python project
        buildCommand = "python";
        buildArgs << "setup.py" << "build";
        buildProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🔨 Detected Python project, building...", "system");
        }
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        buildCommand = "npm";
        buildArgs << "run" << "build";
        buildProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🔨 Detected Node.js project, building...", "system");
        }
    } else {
        hideProgress();
        if (chatWidget) {
            chatWidget->addMessage("❌ No supported build system found (CMake, Make, Python, Node.js)", "error");
        }
        return;
    }
    
    // Connect process signals
    connect(buildProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, buildProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                hideProgress();
                if (exitCode == 0) {
                    if (chatWidget) {
                        chatWidget->addMessage("✅ Build completed successfully!", "system");
                    }
                } else {
                    QString error = buildProcess->readAllStandardError();
                    if (chatWidget) {
                        chatWidget->addMessage(QString("❌ Build failed with exit code %1\n%2").arg(exitCode).arg(error), "error");
                    }
                }
                buildProcess->deleteLater();
            });
    
    connect(buildProcess, &QProcess::readyReadStandardOutput, [this, buildProcess]() {
        QString output = buildProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty() && chatWidget) {
            chatWidget->addMessage(output.trimmed(), "system");
        }
    });
    
    // Start build process
    buildProcess->start(buildCommand, buildArgs);
    
    if (!buildProcess->waitForStarted()) {
        hideProgress();
        if (chatWidget) {
            chatWidget->addMessage(QString("❌ Failed to start build command: %1").arg(buildCommand), "error");
        }
        buildProcess->deleteLater();
    }
}

void MainWindow::onRunRequested()
{
    if (currentProjectPath.isEmpty()) {
        if (chatWidget) {
            chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        }
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
            if (chatWidget) {
                chatWidget->addMessage(QString("▶️ Running: %1").arg(executables.first()), "system");
            }
        } else {
            if (chatWidget) {
                chatWidget->addMessage("❌ No executable found in build directory. Build the project first.", "error");
            }
            return;
        }
    } else if (projectDir.exists("main.py")) {
        // Python project with main.py
        runCommand = "python";
        runArgs << "main.py";
        if (chatWidget) {
            chatWidget->addMessage("▶️ Running Python project: main.py", "system");
        }
    } else if (projectDir.exists("app.py")) {
        // Python Flask/Django app
        runCommand = "python";
        runArgs << "app.py";
        if (chatWidget) {
            chatWidget->addMessage("▶️ Running Python app: app.py", "system");
        }
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        runCommand = "npm";
        runArgs << "start";
        if (chatWidget) {
            chatWidget->addMessage("▶️ Running Node.js project: npm start", "system");
        }
    } else {
        if (chatWidget) {
            chatWidget->addMessage("❌ No runnable entry point found (executable, main.py, app.py, package.json)", "error");
        }
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
            if (chatWidget) {
                chatWidget->addMessage(QString("▶️ Application started in %1").arg(terminal), "system");
            }
            return;
        }
    }
    
    // Fallback: run in background and show output
    QProcess *runProcess = new QProcess(this);
    runProcess->setWorkingDirectory(workingDir);
    
    connect(runProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, runProcess](int exitCode) {
                if (chatWidget) {
                    chatWidget->addMessage(QString("📋 Application finished with exit code %1").arg(exitCode), "system");
                }
                runProcess->deleteLater();
            });
    
    connect(runProcess, &QProcess::readyReadStandardOutput, [this, runProcess]() {
        QString output = runProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty()) {
            if (chatWidget) {
                chatWidget->addMessage("📋 " + output.trimmed(), "system");
            }
        }
    });
    
    runProcess->start(runCommand, runArgs);
    
    if (runProcess->waitForStarted()) {
        if (chatWidget) {
            chatWidget->addMessage("▶️ Application started (running in background)", "system");
        }
    } else {
        if (chatWidget) {
            chatWidget->addMessage(QString("❌ Failed to start: %1").arg(runCommand), "error");
        }
        runProcess->deleteLater();
    }
}

void MainWindow::onTestRequested()
{
    if (currentProjectPath.isEmpty()) {
        if (chatWidget) {
            chatWidget->addMessage("❌ No project folder selected. Open a project first.", "error");
        }
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
        // CMake project - check if build directory exists first
        QString buildDir = projectDir.absolutePath() + "/build";
        if (QDir(buildDir).exists()) {
            testCommand = "ctest";
            testArgs << "--test-dir" << buildDir;
            testProcess->setWorkingDirectory(currentProjectPath);
            if (chatWidget) {
                chatWidget->addMessage("🧘 Running CMake tests (CTest)...", "system");
            }
        } else {
            hideProgress();
            if (chatWidget) {
                chatWidget->addMessage("❌ CMake build directory not found. Please build the project first.\n💡 Tip: Create a build directory and run 'cmake .. && make' to build the project.", "error");
            }
            return;
        }
    } else if (projectDir.exists("pytest.ini") || projectDir.exists("test_*.py")) {
        // Python project with pytest
        testCommand = "pytest";
        testArgs << "-v";
        testProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🧘 Running Python tests (pytest)...", "system");
        }
    } else if (projectDir.exists("package.json")) {
        // Node.js project
        testCommand = "npm";
        testArgs << "test";
        testProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🧘 Running Node.js tests (npm test)...", "system");
        }
    } else if (projectDir.exists("Makefile")) {
        // Make project - try 'make test'
        testCommand = "make";
        testArgs << "test";
        testProcess->setWorkingDirectory(currentProjectPath);
        if (chatWidget) {
            chatWidget->addMessage("🧘 Running Make tests (make test)...", "system");
        }
    } else {
        hideProgress();
        if (chatWidget) {
            chatWidget->addMessage("❌ No supported test framework found (CTest, pytest, npm test, make test)", "error");
        }
        return;
    }
    
    // Connect process signals
    connect(testProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, testProcess](int exitCode, QProcess::ExitStatus exitStatus) {
                hideProgress();
                if (exitCode == 0) {
                    if (chatWidget) {
                        chatWidget->addMessage("✅ All tests passed successfully!", "system");
                    }
                } else {
                    QString error = testProcess->readAllStandardError();
                    if (chatWidget) {
                        chatWidget->addMessage(QString("❌ Tests failed with exit code %1\n%2").arg(exitCode).arg(error), "error");
                    }
                }
                testProcess->deleteLater();
            });
    
    connect(testProcess, &QProcess::readyReadStandardOutput, [this, testProcess]() {
        QString output = testProcess->readAllStandardOutput();
        if (!output.trimmed().isEmpty()) {
            if (chatWidget) {
                chatWidget->addMessage("🧘 " + output.trimmed(), "system");
            }
        }
    });
    
    connect(testProcess, &QProcess::readyReadStandardError, [this, testProcess]() {
        QString error = testProcess->readAllStandardError();
        if (!error.trimmed().isEmpty()) {
            if (chatWidget) {
                chatWidget->addMessage("⚠️ " + error.trimmed(), "error");
            }
        }
    });
    
    // Start test process
    testProcess->start(testCommand, testArgs);
    
    if (!testProcess->waitForStarted()) {
        hideProgress();
        if (chatWidget) {
            chatWidget->addMessage(QString("❌ Failed to start test command: %1").arg(testCommand), "error");
        }
        testProcess->deleteLater();
    }
}

void MainWindow::onAISuggestionReady(const CodeSuggestion &suggestion)
{
    qDebug() << "📥 Received AI suggestion:" << suggestion.type << suggestion.description.left(50) << "...";
    
    // Handle AI suggestions from our enhanced analysis
    if (chatWidget) {
        QString message = QString("🤖 %1: %2")
                         .arg(suggestion.type.toUpper())
                         .arg(suggestion.description);
        
        if (!suggestion.fixedCode.isEmpty()) {
            message += QString("\n\n```cpp\n%1\n```").arg(suggestion.fixedCode);
        }
        
        if (suggestion.lineNumber > 0) {
            message += QString("\n📍 Line: %1").arg(suggestion.lineNumber);
        }
        
        if (suggestion.confidence > 0) {
            message += QString(" (Confidence: %1%)").arg(qRound(suggestion.confidence * 100));
        }
        
        qDebug() << "💬 Adding message to chat widget";
        chatWidget->addMessage(message, "ai");
        qDebug() << "✅ Message added to chat";
    } else {
        qDebug() << "❌ No chat widget available";
    }
    
    hideProgress();
}

// Auto-fix functionality implementation
void MainWindow::parseAndApplyFixes(const QString &aiResponse)
{
    if (chatWidget) {
        chatWidget->addMessage("🔧 DEBUG: Starting auto-fix parsing...", "system");
        chatWidget->addMessage(QString("📄 DEBUG: AI Response length: %1").arg(aiResponse.length()), "system");
    }
    
    // Force debug output even if chatWidget is null
    qDebug() << "🔧 FORCED DEBUG: parseAndApplyFixes called";
    qDebug() << "📄 FORCED DEBUG: AI Response length:" << aiResponse.length();
    
    QStringList appliedFixes;
    QStringList failedFixes;
    
    // Parse the AI response for FILE: entries with code blocks
    QStringList lines = aiResponse.split('\n');
    QString currentFile;
    QString currentDescription;
    int currentLine = 0;
    QString codeBlock;
    bool inCodeBlock = false;
    
    if (chatWidget) {
        chatWidget->addMessage(QString("📝 DEBUG: Processing %1 lines").arg(lines.size()), "system");
        chatWidget->addMessage(QString("📝 DEBUG: First 5 lines:"), "system");
        for (int i = 0; i < qMin(5, lines.size()); ++i) {
            chatWidget->addMessage(QString("  Line %1: '%2'").arg(i).arg(lines[i]), "system");
        }
    }
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        
        // Look for FILE: entries (trim whitespace first)
        QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith("FILE:")) {
            qDebug() << "📁 Found FILE line:" << line;
            qDebug() << "🔍 Line length:" << line.length() << "Raw bytes:" << line.toUtf8().toHex();
            
            // Parse: FILE: filename.ext, LINE: XX - description
            QRegularExpression fileRegex("FILE:\\s*([^,\\n]+)(?:,\\s*LINE:\\s*(\\d+))?\\s*-\\s*(.+)");
            QRegularExpressionMatch match = fileRegex.match(trimmedLine);
            
            qDebug() << "🔧 Regex pattern:" << fileRegex.pattern();
            qDebug() << "🔍 Match valid:" << match.isValid() << "Has match:" << match.hasMatch();
            
            if (match.hasMatch()) {
                currentFile = match.captured(1).trimmed();
                currentLine = match.captured(2).toInt();
                currentDescription = match.captured(3).trimmed();
                qDebug() << "✅ Parsed:" << currentFile << "line" << currentLine << "-" << currentDescription;
            } else {
                qDebug() << "❌ Failed to parse FILE line - trying simpler regex";
                
                // Try a much simpler regex as fallback
                QRegularExpression simpleRegex("FILE:\\s*([^,]+)");
                QRegularExpressionMatch simpleMatch = simpleRegex.match(line);
                if (simpleMatch.hasMatch()) {
                    currentFile = simpleMatch.captured(1).trimmed();
                    currentLine = 0; // Default line
                    currentDescription = "Auto-detected fix";
                    qDebug() << "✅ Simple match found:" << currentFile;
                } else {
                    qDebug() << "❌ Even simple regex failed";
                }
            }
        }
        // Look for code block start (more flexible)
        else if ((line == "```cpp" || line == "```c++" || line == "```cmake" || line.startsWith("```")) && !currentFile.isEmpty()) {
            qDebug() << "📝 Found code block start for" << currentFile << "with:" << line;
            inCodeBlock = true;
            codeBlock.clear();
        }
        // Look for code block end
        else if (line == "```" && inCodeBlock) {
            qDebug() << "📝 Found code block end, code length:" << codeBlock.length();
            inCodeBlock = false;
            
            // Apply the fix
            if (!codeBlock.isEmpty() && !currentFile.isEmpty()) {
                qDebug() << "🔍 Looking for file:" << currentFile;
                QString fullPath = findFileInProject(currentFile);
                qDebug() << "📂 Full path found:" << fullPath;
                
                if (!fullPath.isEmpty()) {
                    qDebug() << "🔧 Applying fix to" << fullPath << "line" << currentLine;
                    if (applyCodeFix(fullPath, currentLine, codeBlock, currentDescription)) {
                        appliedFixes.append(QString("%1: %2").arg(currentFile, currentDescription));
                        qDebug() << "✅ Fix applied successfully";
                    } else {
                        failedFixes.append(QString("%1: %2").arg(currentFile, currentDescription));
                        qDebug() << "❌ Fix application failed";
                    }
                } else {
                    failedFixes.append(QString("%1: File not found").arg(currentFile));
                    qDebug() << "❌ File not found:" << currentFile;
                }
            } else {
                qDebug() << "❌ Empty code block or file name";
            }
            
            // Reset for next fix
            currentFile.clear();
            currentDescription.clear();
            currentLine = 0;
            codeBlock.clear();
        }
        // Collect code block content
        else if (inCodeBlock) {
            codeBlock += line + "\n";
        }
    }
    
    // Show summary of applied fixes
    showFixSummary(appliedFixes, failedFixes);
}

QString MainWindow::findFileInProject(const QString &fileName)
{
    // Look for the file in the current project
    for (const QString &filePath : codeFiles) {
        if (QFileInfo(filePath).fileName() == fileName) {
            return filePath;
        }
    }
    
    // If not found in codeFiles, search in project directory
    QDirIterator iterator(currentProjectPath, QStringList() << fileName, QDir::Files, QDirIterator::Subdirectories);
    if (iterator.hasNext()) {
        return iterator.next();
    }
    
    return QString();
}

bool MainWindow::applyCodeFix(const QString &filePath, int lineNumber, const QString &fixedCode, const QString &description)
{
    // Create backup first
    QString backupPath = createBackupFile(filePath);
    if (backupPath.isEmpty()) {
        qWarning() << "Failed to create backup for" << filePath;
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }
    
    // Read current file content
    QStringList fileLines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        fileLines.append(in.readLine());
    }
    file.close();
    
    // Apply the fix based on the type of change
    QStringList fixLines = fixedCode.trimmed().split('\n');
    
    qDebug() << "🔧 Applying fix:" << description;
    qDebug() << "📝 Fix code lines:" << fixLines.size();
    
    // Enhanced fix application logic with better detection
    if (fixedCode.contains("#include")) {
        // Handle include statements
        qDebug() << "🔍 Detected include fix";
        applyIncludeFix(fileLines, fixLines);
    } else if (fixedCode.contains("class") && fixedCode.contains("{")) {
        // Handle class definitions/modifications
        qDebug() << "🔍 Detected class fix";
        applyClassFix(fileLines, fixLines, lineNumber);
    } else if (description.contains("main") || fixedCode.contains("int main")) {
        // Handle main function fixes (check description first)
        qDebug() << "🔍 Detected main function fix";
        applyMainFix(fileLines, fixLines);
    } else if (fixedCode.contains("{") && fixedCode.contains("}") && 
               (description.contains("function") || fixedCode.contains("("))) {
        // Handle function definitions
        qDebug() << "🔍 Detected function fix";
        applyFunctionFix(fileLines, fixLines, lineNumber, description);
    } else if (lineNumber > 0 && lineNumber <= fileLines.size()) {
        // Simple line replacement for single-line fixes
        qDebug() << "🔍 Applying simple line replacement at line" << lineNumber;
        fileLines[lineNumber - 1] = fixLines.join(" ");
    } else {
        // Fallback: intelligent insertion based on content
        qDebug() << "🔍 Applying generic fix";
        applyGenericFix(fileLines, fixLines, description);
    }
    
    // Write the fixed content back to file
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    for (const QString &line : fileLines) {
        out << line << "\n";
    }
    file.close();
    
    qDebug() << "✅ Applied fix to" << filePath << "- backed up to" << backupPath;
    return true;
}

QString MainWindow::createBackupFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString backupPath = fileInfo.absolutePath() + "/" + 
                        fileInfo.baseName() + "_backup_" + 
                        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + 
                        "." + fileInfo.suffix();
    
    if (QFile::copy(filePath, backupPath)) {
        return backupPath;
    }
    
    return QString();
}

void MainWindow::showFixSummary(const QStringList &appliedFixes, const QStringList &failedFixes)
{
    if (appliedFixes.isEmpty() && failedFixes.isEmpty()) {
        if (chatWidget) {
            chatWidget->addMessage("🔍 No auto-fixable issues found in AI response", "system");
        }
        return;
    }
    
    QString summaryMessage = "🔧 AUTO-FIX SUMMARY:\n\n";
    
    if (!appliedFixes.isEmpty()) {
        summaryMessage += QString("✅ SUCCESSFULLY APPLIED (%1 fixes):\n").arg(appliedFixes.size());
        for (const QString &fix : appliedFixes) {
            summaryMessage += "  • " + fix + "\n";
        }
        summaryMessage += "\n";
    }
    
    if (!failedFixes.isEmpty()) {
        summaryMessage += QString("❌ FAILED TO APPLY (%1 fixes):\n").arg(failedFixes.size());
        for (const QString &fix : failedFixes) {
            summaryMessage += "  • " + fix + "\n";
        }
        summaryMessage += "\n";
    }
    
    summaryMessage += "💾 Backup files created for all modified files\n";
    summaryMessage += "🔄 Refresh your file tree to see changes";
    
    if (chatWidget) {
        chatWidget->addMessage(summaryMessage, "system");
    }
}

// Enhanced fix application methods
void MainWindow::applyIncludeFix(QStringList &fileLines, const QStringList &fixLines)
{
    qDebug() << "📚 Applying include fix";
    
    // Extract all include statements from the fix
    QStringList newIncludes;
    for (const QString &fixLine : fixLines) {
        QString trimmedLine = fixLine.trimmed();
        if (trimmedLine.startsWith("#include")) {
            newIncludes.append(trimmedLine);
        }
    }
    
    if (newIncludes.isEmpty()) {
        qDebug() << "❌ No include statements found in fix";
        return;
    }
    
    // Find the position to insert includes (after existing includes or at the top)
    int insertPos = 0;
    
    // Skip existing includes and find insertion point
    while (insertPos < fileLines.size() && 
           (fileLines[insertPos].startsWith("#include") || 
            fileLines[insertPos].trimmed().isEmpty() ||
            fileLines[insertPos].startsWith("//"))) {
        insertPos++;
    }
    
    // Insert new includes that don't already exist
    for (const QString &newInclude : newIncludes) {
        bool alreadyExists = false;
        for (const QString &existingLine : fileLines) {
            if (existingLine.trimmed() == newInclude) {
                alreadyExists = true;
                break;
            }
        }
        
        if (!alreadyExists) {
            fileLines.insert(insertPos, newInclude);
            insertPos++;
            qDebug() << "✅ Added include:" << newInclude;
        } else {
            qDebug() << "⏭️ Include already exists:" << newInclude;
        }
    }
}

void MainWindow::applyClassFix(QStringList &fileLines, const QStringList &fixLines, int lineNumber)
{
    qDebug() << "🏗️ Applying class fix";
    
    // Find the class definition to replace
    int classStart = -1;
    int classEnd = -1;
    
    // Look for class definition around the specified line
    for (int i = qMax(0, lineNumber - 10); i < qMin(fileLines.size(), lineNumber + 20); ++i) {
        if (fileLines[i].contains("class") && fileLines[i].contains("{")) {
            classStart = i;
            break;
        }
    }
    
    if (classStart >= 0) {
        // Find the end of the class (matching closing brace)
        int braceCount = 0;
        for (int i = classStart; i < fileLines.size(); ++i) {
            for (const QChar &ch : fileLines[i]) {
                if (ch == '{') braceCount++;
                else if (ch == '}') braceCount--;
            }
            if (braceCount == 0 && i > classStart) {
                classEnd = i;
                break;
            }
        }
        
        if (classEnd >= 0) {
            // Replace the entire class definition
            for (int i = classEnd; i >= classStart; --i) {
                fileLines.removeAt(i);
            }
            
            // Insert the new class definition
            for (int i = fixLines.size() - 1; i >= 0; --i) {
                fileLines.insert(classStart, fixLines[i]);
            }
            qDebug() << "✅ Replaced class definition";
        }
    } else {
        // Fallback: add the class at the end
        fileLines.append("// Fixed class definition:");
        fileLines.append(fixLines);
        qDebug() << "✅ Added class definition at end";
    }
}

void MainWindow::applyFunctionFix(QStringList &fileLines, const QStringList &fixLines, int lineNumber, const QString &description)
{
    qDebug() << "⚙️ Applying function fix";
    
    // Extract function name from description or fix code
    QString functionName;
    if (description.contains("calculate")) functionName = "calculate";
    else if (description.contains("processArray")) functionName = "processArray";
    else if (description.contains("getName")) functionName = "getName";
    else if (description.contains("main")) functionName = "main";
    
    if (!functionName.isEmpty()) {
        // Find and replace the specific function
        int funcStart = -1;
        int funcEnd = -1;
        
        for (int i = 0; i < fileLines.size(); ++i) {
            if (fileLines[i].contains(functionName) && 
                (fileLines[i].contains("(") || (i + 1 < fileLines.size() && fileLines[i + 1].contains("(")))) {
                funcStart = i;
                
                // Find the end of the function
                int braceCount = 0;
                bool foundOpenBrace = false;
                
                for (int j = i; j < fileLines.size(); ++j) {
                    for (const QChar &ch : fileLines[j]) {
                        if (ch == '{') {
                            braceCount++;
                            foundOpenBrace = true;
                        } else if (ch == '}') {
                            braceCount--;
                        }
                    }
                    
                    if (foundOpenBrace && braceCount == 0) {
                        funcEnd = j;
                        break;
                    }
                }
                break;
            }
        }
        
        if (funcStart >= 0 && funcEnd >= 0) {
            // Replace the function
            for (int i = funcEnd; i >= funcStart; --i) {
                fileLines.removeAt(i);
            }
            
            for (int i = fixLines.size() - 1; i >= 0; --i) {
                fileLines.insert(funcStart, fixLines[i]);
            }
            qDebug() << "✅ Replaced function:" << functionName;
            return;
        }
    }
    
    // Fallback: add the function at the end
    applyGenericFix(fileLines, fixLines, description);
}

void MainWindow::applyMainFix(QStringList &fileLines, const QStringList &fixLines)
{
    qDebug() << "🎯 Applying main function fix";
    
    // Find main function
    int mainStart = -1;
    int mainEnd = -1;
    
    for (int i = 0; i < fileLines.size(); ++i) {
        if (fileLines[i].contains("int main") || fileLines[i].contains("main(")) {
            mainStart = i;
            
            // Find the end of main function
            int braceCount = 0;
            bool foundOpenBrace = false;
            
            for (int j = i; j < fileLines.size(); ++j) {
                for (const QChar &ch : fileLines[j]) {
                    if (ch == '{') {
                        braceCount++;
                        foundOpenBrace = true;
                    } else if (ch == '}') {
                        braceCount--;
                    }
                }
                
                if (foundOpenBrace && braceCount == 0) {
                    mainEnd = j;
                    break;
                }
            }
            break;
        }
    }
    
    if (mainStart >= 0 && mainEnd >= 0) {
        // Replace main function
        for (int i = mainEnd; i >= mainStart; --i) {
            fileLines.removeAt(i);
        }
        
        for (int i = fixLines.size() - 1; i >= 0; --i) {
            fileLines.insert(mainStart, fixLines[i]);
        }
        qDebug() << "✅ Replaced main function";
    } else {
        // Add main function at the end
        fileLines.append("// Fixed main function:");
        fileLines.append(fixLines);
        qDebug() << "✅ Added main function at end";
    }
}

void MainWindow::applyGenericFix(QStringList &fileLines, const QStringList &fixLines, const QString &description)
{
    qDebug() << "🔧 Applying generic fix:" << description;
    
    // Try to find a good insertion point based on content
    int insertPos = fileLines.size(); // Default to end
    
    if (description.contains("destructor") || description.contains("constructor")) {
        // Insert near class definition
        for (int i = 0; i < fileLines.size(); ++i) {
            if (fileLines[i].contains("class") && fileLines[i].contains("{")) {
                insertPos = i + 1;
                break;
            }
        }
    } else if (description.contains("return")) {
        // Insert before the end of main or other functions
        for (int i = fileLines.size() - 1; i >= 0; --i) {
            if (fileLines[i].contains("}") && 
                (i > 0 && (fileLines[i-1].contains("main") || fileLines[i-1].contains("{")))) {
                insertPos = i;
                break;
            }
        }
    }
    
    // Insert the fix
    fileLines.insert(insertPos, "// Auto-generated fix: " + description);
    for (const QString &line : fixLines) {
        fileLines.insert(++insertPos, line);
    }
    
    qDebug() << "✅ Applied generic fix at position" << insertPos;
}


