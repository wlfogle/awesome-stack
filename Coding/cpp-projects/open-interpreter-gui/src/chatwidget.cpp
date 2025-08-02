#include "chatwidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupStyles();

    // Connect context menu
    chatDisplay->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(chatDisplay, &QTextEdit::customContextMenuRequested, this, &ChatWidget::showContextMenu);
}

void ChatWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("🤖 Open Interpreter - Local AI Code Assistant");
    titleLabel->setFont(QFont("Arial", 16, QFont::Bold));
    titleLabel->setStyleSheet("color: #007acc; margin: 10px;");
    layout->addWidget(titleLabel);
    
    // Chat display
    chatDisplay = new QTextEdit(this);
    chatDisplay->setReadOnly(true);
    chatDisplay->setFont(QFont("Arial", 10));
    // Note: QTextEdit doesn't have setOpenExternalLinks or anchorClicked signals
    // We'll handle link detection through other means
    layout->addWidget(chatDisplay);
    
    // Input area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    
    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText("Ask me to write, debug, or analyze code...");
    messageInput->setFont(QFont("Arial", 10));
    
    sendButton = new QPushButton("Send", this);
    
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);
    
    // Quick analysis buttons
    QHBoxLayout *quickActionsLayout = new QHBoxLayout();
    
    QPushButton *analyzeBtn = new QPushButton("🔍 Analyze Code");
    QPushButton *debugBtn = new QPushButton("🐛 Find Bugs");
    QPushButton *optimizeBtn = new QPushButton("⚡ Optimize");
    QPushButton *documentBtn = new QPushButton("📚 Document");
    QPushButton *fixBtn = new QPushButton("🔧 Fix Issues");
    QPushButton *testBtn = new QPushButton("🧪 Test");
    QPushButton *buildBtn = new QPushButton("🔨 Build");
    QPushButton *runBtn = new QPushButton("▶️ Run");
    QPushButton *commandBtn = new QPushButton("💻 Commands");
    
    analyzeBtn->setStyleSheet("QPushButton { background-color: #28a745; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    debugBtn->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    optimizeBtn->setStyleSheet("QPushButton { background-color: #ffc107; color: black; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    documentBtn->setStyleSheet("QPushButton { background-color: #17a2b8; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    fixBtn->setStyleSheet("QPushButton { background-color: #fd7e14; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    testBtn->setStyleSheet("QPushButton { background-color: #e83e8c; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    buildBtn->setStyleSheet("QPushButton { background-color: #20c997; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    runBtn->setStyleSheet("QPushButton { background-color: #198754; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    commandBtn->setStyleSheet("QPushButton { background-color: #6f42c1; color: white; border: none; padding: 4px 8px; border-radius: 3px; margin: 2px; }");
    
    connect(analyzeBtn, &QPushButton::clicked, [this]() { emit messageSent("ANALYZE THIS CODE: Find SPECIFIC architectural issues, design patterns used, coupling problems, and suggest CONCRETE improvements with exact file names and line numbers."); });
    connect(debugBtn, &QPushButton::clicked, [this]() { emit messageSent("MANDATORY BUG HUNT: You MUST find at least 3 actual bugs in this code. Look for: NULL pointers, memory leaks, buffer overflows, uninitialized variables, race conditions, logic errors, missing error handling. If you don't find bugs, you FAILED. Provide EXACT line numbers and fixed code."); });
    connect(optimizeBtn, &QPushButton::clicked, [this]() { emit messageSent("OPTIMIZE PERFORMANCE: Identify performance bottlenecks, algorithmic inefficiencies, memory usage issues. Provide SPECIFIC code changes and benchmarks."); });
    connect(documentBtn, &QPushButton::clicked, [this]() { emit messageSent("GENERATE DOCUMENTATION: Create comprehensive documentation including function signatures, parameter descriptions, return values, usage examples, and API documentation."); });
    connect(fixBtn, &QPushButton::clicked, [this]() { emit messageSent("FIX ALL ISSUES: Identify and automatically correct compilation errors, logic bugs, memory issues. Show BEFORE and AFTER code with explanations."); });
    connect(testBtn, &QPushButton::clicked, [this]() { emit testRequested(); });
    connect(buildBtn, &QPushButton::clicked, [this]() { emit buildRequested(); });
    connect(runBtn, &QPushButton::clicked, [this]() { emit runRequested(); });
    connect(commandBtn, &QPushButton::clicked, [this]() { emit messageSent("COMMAND ASSISTANCE: Generate shell commands for this project: build scripts, test commands, deployment commands, debugging commands. Explain each command and provide examples."); });
    
    quickActionsLayout->addWidget(analyzeBtn);
    quickActionsLayout->addWidget(debugBtn);
    quickActionsLayout->addWidget(fixBtn);
    quickActionsLayout->addWidget(optimizeBtn);
    quickActionsLayout->addWidget(buildBtn);
    quickActionsLayout->addWidget(testBtn);
    quickActionsLayout->addWidget(runBtn);
    quickActionsLayout->addWidget(documentBtn);
    quickActionsLayout->addWidget(commandBtn);
    quickActionsLayout->addStretch();
    
    layout->addLayout(inputLayout);
    layout->addLayout(quickActionsLayout);
    
    // Connect signals
    connect(sendButton, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
    
    // Add welcome message
    addMessage(
        "Welcome to Open Interpreter GUI! 🎉\n\n"
        "I'm your local AI coding assistant. I can:\n"
        "• Write code in any programming language\n"
        "• Debug and fix errors in your code\n"
        "• Analyze and explain existing code\n"
        "• Execute code and show results\n"
        "• Help with algorithms and data structures\n\n"
        "Just type your request and I'll help you code!",
        "system"
    );
}

void ChatWidget::setupStyles()
{
    chatDisplay->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1a1a1a;"
        "    color: #e0e0e0;"
        "    border: 1px solid #404040;"
        "    border-radius: 8px;"
        "    padding: 10px;"
        "}"
    );
    
    messageInput->setStyleSheet(
        "QLineEdit {"
        "    padding: 8px;"
        "    border: 2px solid #007acc;"
        "    border-radius: 5px;"
        "    font-size: 10pt;"
        "}"
    );
    
    sendButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #007acc;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 5px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #005999;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #004466;"
        "}"
    );
}

void ChatWidget::addMessage(const QString &message, const QString &sender)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString color, prefix;
    
    if (sender == "user") {
        color = "#007acc";
        prefix = "👤 You";
    } else if (sender == "assistant") {
        color = "#28a745";
        prefix = "🤖 AI Assistant";
    } else if (sender == "system") {
        color = "#6c757d";
        prefix = "⚙️ System";
    } else { // error
        color = "#dc3545";
        prefix = "❌ Error";
    }
    
    // Process message for file links if it's from assistant
    QString processedMessage = message;
    if (sender == "assistant") {
        processedMessage = makeFileLinksClickable(message);
    }
    
    QString formattedMessage = QString(
        "<div style=\"margin: 10px 0; padding: 10px; border-left: 4px solid %1; "
        "background-color: #2a2a2a; border-radius: 5px;\">"
        "<b style=\"color: %1;\">%2</b> "
        "<span style=\"color: #888888; font-size: 9pt;\">[%3]</span><br>"
        "<div style=\"margin-top: 5px; white-space: pre-wrap; color: #e0e0e0;\">%4</div>"
        "</div>"
    ).arg(color, prefix, timestamp, sender == "assistant" ? processedMessage : processedMessage.toHtmlEscaped());
    
    chatDisplay->insertHtml(formattedMessage);
    chatDisplay->moveCursor(QTextCursor::End);
}

void ChatWidget::showContextMenu(const QPoint &pos) {
    QString selectedText = chatDisplay->textCursor().selectedText().trimmed();
    if (selectedText.isEmpty()) {
        return; // No text selected, don't show menu
    }

    QMenu contextMenu(tr("AI Assistant"), this);

    // Fix/Debug Actions
    QAction *fixAction = new QAction("🔧 Fix This Code", this);
    connect(fixAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Fix this code: " + selectedText, "user");
        emit messageSent("FIX THIS CODE: Analyze the following code and provide corrected version with explanations for any bugs or issues found:\n\n" + selectedText);
    });
    
    QAction *explainAction = new QAction("💡 Explain This", this);
    connect(explainAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Explain: " + selectedText, "user");
        emit messageSent("EXPLAIN CODE: Provide detailed explanation of what this code does, how it works, and any potential issues:\n\n" + selectedText);
    });
    
    QAction *optimizeAction = new QAction("⚡ Optimize This", this);
    connect(optimizeAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Optimize: " + selectedText, "user");
        emit messageSent("OPTIMIZE CODE: Improve performance, reduce complexity, and enhance readability of this code:\n\n" + selectedText);
    });
    
    QAction *testAction = new QAction("🧪 Generate Tests", this);
    connect(testAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Generate tests for: " + selectedText, "user");
        emit messageSent("GENERATE TESTS: Create comprehensive unit tests for this code including edge cases and error conditions:\n\n" + selectedText);
    });
    
    QAction *refactorAction = new QAction("🔄 Refactor", this);
    connect(refactorAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Refactor: " + selectedText, "user");
        emit messageSent("REFACTOR CODE: Improve code structure, apply design patterns, reduce coupling, and enhance maintainability:\n\n" + selectedText);
    });
    
    QAction *documentAction = new QAction("📚 Document", this);
    connect(documentAction, &QAction::triggered, [this, selectedText]() {
        addMessage("Document: " + selectedText, "user");
        emit messageSent("GENERATE DOCUMENTATION: Create comprehensive documentation including function signatures, parameters, return values, usage examples:\n\n" + selectedText);
    });
    
    // Copy Text Action
    QAction *copyAction = new QAction("📋 Copy Text", this);
    connect(copyAction, &QAction::triggered, [selectedText]() {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(selectedText);
    });
    
    // Apply Fixes Action (only show if the text contains fixes)
    QAction *applyFixAction = nullptr;
    if (selectedText.contains("FILE:") && selectedText.contains("LINE:") && selectedText.contains("```")) {
        applyFixAction = new QAction("🔧 Apply Fixes Automatically", this);
        connect(applyFixAction, &QAction::triggered, [this, selectedText]() {
            parseAndApplyFixes(selectedText);
        });
    }
    
    // Add actions to menu
    contextMenu.addAction(copyAction);
    if (applyFixAction) {
        contextMenu.addSeparator();
        contextMenu.addAction(applyFixAction);
    }
    contextMenu.addSeparator();
    contextMenu.addAction(fixAction);
    contextMenu.addAction(explainAction);
    contextMenu.addSeparator();
    contextMenu.addAction(optimizeAction);
    contextMenu.addAction(refactorAction);
    contextMenu.addSeparator();
    contextMenu.addAction(testAction);
    contextMenu.addAction(documentAction);

    contextMenu.exec(chatDisplay->mapToGlobal(pos));
}

void ChatWidget::onSendClicked()
{
    QString message = messageInput->text().trimmed();
    if (message.isEmpty()) {
        return;
    }

    messageInput->clear();
    addMessage(message, "user");
    emit messageSent(message);
}

void ChatWidget::onLinkClicked(const QUrl &url)
{
    QString urlString = url.toString();
    if (urlString.startsWith("file://")) {
        // Extract file path and line number
        QString path = urlString.mid(7); // Remove "file://" prefix
        int lineNumber = 1;
        
        // Check if there's a line number parameter
        if (path.contains("?line=")) {
            QStringList parts = path.split("?line=");
            if (parts.size() == 2) {
                path = parts[0];
                lineNumber = parts[1].toInt();
            }
        }
        
        emit fileNavigationRequested(path, lineNumber);
    }
}

QString ChatWidget::makeFileLinksClickable(const QString &message)
{
    QString result = message.toHtmlEscaped();
    
    // Regular expression to match file references like "filename.cpp:line" or "./path/file.h:123"
    QRegularExpression fileRegex(R"((\./)?[\w/\-\.]+\.(cpp|h|hpp|c|cc|cxx|py|js|ts|java|cs|php|rb|go|rs|swift|kt|scala|dart):(\d+))");
    QRegularExpressionMatchIterator iterator = fileRegex.globalMatch(result);
    
    // Process matches in reverse order to maintain correct positions
    QList<QRegularExpressionMatch> matches;
    while (iterator.hasNext()) {
        matches.prepend(iterator.next());
    }
    
    for (const QRegularExpressionMatch &match : matches) {
        QString fullMatch = match.captured(0);
        QString filePath = fullMatch.split(":").first();
        QString lineNumber = fullMatch.split(":").last();
        
        // Create clickable link
        QString link = QString("<a href=\"file://%1?line=%2\" style=\"color: #007acc; text-decoration: underline;\">%3</a>")
                      .arg(filePath, lineNumber, fullMatch);
        
        result.replace(match.capturedStart(), match.capturedLength(), link);
    }
    
    return result;
}

void ChatWidget::parseAndApplyFixes(const QString &aiResponse)
{
    // Parse AI response to extract file fixes with enhanced robustness
    QStringList lines = aiResponse.split('\n');
    QList<QString> filesToFix;
    QMap<QString, QStringList> fixesPerFile;
    QMap<QString, int> lineNumbersPerFile; // Store line numbers for fixes
    QString currentFile;
    QString currentCodeBlock;
    bool inCodeBlock = false;
    int currentLineNumber = -1;
    QString codeBlockLanguage;
    
    // Enhanced patterns for file detection with more flexibility
    QList<QRegularExpression> filePatterns = {
        QRegularExpression(R"((?:FILE|File|file)\s*[:=]\s*([^,\n\r]+))", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((?:In file|File name|Filename)\s*[:=]?\s*([^,\n\r]+))", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((?:Path|File path)\s*[:=]\s*([^,\n\r]+))", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"(`([^`]+\.[a-zA-Z]+)`)"), // Files in backticks
        QRegularExpression(R"(([a-zA-Z_][\w/\-\.]*\.(cpp|h|hpp|c|cc|cxx|py|js|ts|java|cs|php|rb|go|rs|swift|kt|scala|dart|m|mm|html|css|json|xml|yaml|yml)))") // Standalone filenames
    };
    
    // Enhanced patterns for line number detection
    QList<QRegularExpression> linePatterns = {
        QRegularExpression(R"((?:LINE|Line|line)\s*[:=]?\s*(\d+))", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"((?:At line|Line number)\s*[:=]?\s*(\d+))", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression(R"(:(\d+):)"), // :123: format
        QRegularExpression(R"(\[(\d+)\])") // [123] format
    };
    
    // Enhanced code block detection patterns
    QList<QRegularExpression> codeBlockPatterns = {
        QRegularExpression(R"(^\s*```(\w*)\s*$)"), // Standard markdown with optional language
        QRegularExpression(R"(^\s*~~~(\w*)\s*$)"), // Alternative markdown
        QRegularExpression(R"(^\s*<code>\s*$)"), // HTML style
        QRegularExpression(R"(^\s*\[code\]\s*$)") // BBCode style
    };
    
    // Debug output for testing
    addMessage(QString("[DEBUG] Starting to parse AI response (%1 lines)").arg(lines.size()), "system");
    
    for (int i = 0; i < lines.size(); ++i) {
        const QString &line = lines[i];
        QString trimmedLine = line.trimmed();
        
        // Try to detect file references with multiple patterns
        bool fileFound = false;
        for (const QRegularExpression &pattern : filePatterns) {
            QRegularExpressionMatch match = pattern.match(line);
            if (match.hasMatch()) {
                QString extractedFile = match.captured(1).trimmed();
                // Clean up the filename - remove quotes, extra spaces, etc.
                extractedFile = extractedFile.remove('"').remove('\'').remove('`').trimmed();
                
                if (!extractedFile.isEmpty() && extractedFile.contains('.')) {
                    currentFile = extractedFile;
                    if (!filesToFix.contains(currentFile)) {
                        filesToFix.append(currentFile);
                        fixesPerFile[currentFile] = QStringList();
                        lineNumbersPerFile[currentFile] = -1;
                        addMessage(QString("[DEBUG] Found file: %1").arg(currentFile), "system");
                    }
                    fileFound = true;
                    break;
                }
            }
        }
        
        // Try to detect line numbers if we have a current file
        if (!currentFile.isEmpty() && !fileFound) {
            for (const QRegularExpression &pattern : linePatterns) {
                QRegularExpressionMatch match = pattern.match(line);
                if (match.hasMatch()) {
                    currentLineNumber = match.captured(1).toInt();
                    lineNumbersPerFile[currentFile] = currentLineNumber;
                    addMessage(QString("[DEBUG] Found line number: %1 for file %2").arg(currentLineNumber).arg(currentFile), "system");
                    break;
                }
            }
        }
        
        // Enhanced code block detection
        bool isCodeBlockStart = false;
        for (const QRegularExpression &pattern : codeBlockPatterns) {
            QRegularExpressionMatch match = pattern.match(trimmedLine);
            if (match.hasMatch()) {
                if (inCodeBlock && !currentCodeBlock.isEmpty() && !currentFile.isEmpty()) {
                    // End of code block - save it
                    QString cleanedCode = currentCodeBlock.trimmed();
                    if (!cleanedCode.isEmpty()) {
                        fixesPerFile[currentFile].append(cleanedCode);
                        addMessage(QString("[DEBUG] Saved code block for %1 (%2 chars)").arg(currentFile).arg(cleanedCode.length()), "system");
                    }
                    currentCodeBlock.clear();
                }
                
                inCodeBlock = !inCodeBlock;
                codeBlockLanguage = match.capturedLength() > 1 ? match.captured(1) : "";
                isCodeBlockStart = true;
                addMessage(QString("[DEBUG] Code block %1 (language: %2)").arg(inCodeBlock ? "started" : "ended").arg(codeBlockLanguage.isEmpty() ? "auto" : codeBlockLanguage), "system");
                break;
            }
        }
        
        // Collect code content
        if (inCodeBlock && !isCodeBlockStart) {
            currentCodeBlock += line + "\n";
        }
        
        // Alternative approach: look for immediate file:line patterns in text
        if (!inCodeBlock && currentFile.isEmpty()) {
            QRegularExpression fileLinePattern(R"(([a-zA-Z_][\w/\-\.]*\.(cpp|h|hpp|c|cc|cxx|py|js|ts|java|cs|php|rb|go|rs|swift|kt|scala|dart|m|mm|html|css|json|xml|yaml|yml)):(\d+))");
            QRegularExpressionMatch match = fileLinePattern.match(line);
            if (match.hasMatch()) {
                QString fileName = match.captured(1);
                int lineNum = match.captured(3).toInt();
                
                if (!filesToFix.contains(fileName)) {
                    filesToFix.append(fileName);
                    fixesPerFile[fileName] = QStringList();
                    lineNumbersPerFile[fileName] = lineNum;
                    addMessage(QString("[DEBUG] Found file:line pattern: %1:%2").arg(fileName).arg(lineNum), "system");
                }
                currentFile = fileName;
            }
        }
    }
    
    // Handle final code block if we ended in one
    if (inCodeBlock && !currentCodeBlock.isEmpty() && !currentFile.isEmpty()) {
        QString cleanedCode = currentCodeBlock.trimmed();
        if (!cleanedCode.isEmpty()) {
            fixesPerFile[currentFile].append(cleanedCode);
            addMessage(QString("[DEBUG] Saved final code block for %1 (%2 chars)").arg(currentFile).arg(cleanedCode.length()), "system");
        }
    }
    
    // If we found no files using the patterns, try fallback heuristics
    if (filesToFix.isEmpty()) {
        addMessage("[DEBUG] No files found with patterns, trying fallback heuristics", "system");
        
        // Look for code blocks without explicit file references
        QRegularExpression codeBlockRegex(R"(```(?:\w+)?\s*\n([\s\S]*?)\n```)");
        QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(aiResponse);
        
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            QString codeContent = match.captured(1).trimmed();
            
            if (!codeContent.isEmpty()) {
                // Try to infer filename from code content
                QString inferredFile = inferFilenameFromCode(codeContent);
                if (!inferredFile.isEmpty()) {
                    if (!filesToFix.contains(inferredFile)) {
                        filesToFix.append(inferredFile);
                        fixesPerFile[inferredFile] = QStringList();
                        lineNumbersPerFile[inferredFile] = -1;
                        addMessage(QString("[DEBUG] Inferred file from code: %1").arg(inferredFile), "system");
                    }
                    fixesPerFile[inferredFile].append(codeContent);
                }
            }
        }
    }
    
    if (filesToFix.isEmpty()) {
        QMessageBox::information(this, "No Fixes Found", 
            "No fixable code was found in the selected text.\n\n"
            "Tried to detect:\n"
            "• FILE: filename references\n"
            "• Code blocks with ```\n"
            "• Inline filename patterns\n"
            "• Code content analysis\n\n"
            "Make sure your selection contains clear file references and code blocks.");
        return;
    }
    
    // Show confirmation dialog
    QString filesText;
    for (const QString &file : filesToFix) {
        filesText += "• " + file + "\n";
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Apply Automatic Fixes",
        QString("Do you want to apply fixes to the following files?\n\n%1\n"
                "⚠️ Warning: This will modify your source files.\n"
                "Make sure you have backups or version control!").arg(filesText),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Apply fixes
    int successCount = 0;
    int errorCount = 0;
    QString resultMessage;
    
    for (const QString &fileName : filesToFix) {
        QStringList fixes = fixesPerFile[fileName];
        if (fixes.isEmpty()) continue;
        
        // Find the file in current directory or ask user
        QString filePath = findFileInProject(fileName);
        if (filePath.isEmpty()) {
            QString msg = QString("Could not locate file: %1\n").arg(fileName);
            resultMessage += msg;
            errorCount++;
            continue;
        }
        
        // Apply the first (and usually main) fix
        QString fixedCode = fixes.first();
        
        // Backup original file
        QString backupPath = filePath + ".backup." + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QFile::copy(filePath, backupPath);
        
        // Write fixed code
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << fixedCode;
            file.close();
            
            QString msg = QString("✅ Applied fixes to: %1\n   Backup: %2\n").arg(fileName, QFileInfo(backupPath).fileName());
            resultMessage += msg;
            successCount++;
        } else {
            QString msg = QString("❌ Failed to write to: %1\n").arg(fileName);
            resultMessage += msg;
            errorCount++;
        }
    }
    
    // Show results
    QString title = QString("Applied %1 fixes, %2 errors").arg(successCount).arg(errorCount);
    addMessage(resultMessage, "system");
    
    if (successCount > 0) {
        QMessageBox::information(this, title, 
            QString("Successfully applied fixes to %1 file(s).\n\n%2\n\n"
                    "💡 Tip: Backup files were created automatically.").arg(successCount).arg(resultMessage));
    } else {
        QMessageBox::warning(this, "Fix Application Failed", 
            QString("Could not apply any fixes.\n\n%1").arg(resultMessage));
    }
}

QString ChatWidget::inferFilenameFromCode(const QString &codeSnippet)
{
    // Analyze code content to infer the most likely filename
    QString code = codeSnippet.trimmed();
    
    if (code.isEmpty()) {
        return QString();
    }
    
    // Look for explicit filename comments or headers
    QRegularExpression fileCommentPattern(R"((?://|#|/\*)\s*(?:File:|Filename:|file:|filename:)\s*([a-zA-Z_][\w/\-\.]*\.[a-zA-Z]+))");
    QRegularExpressionMatch commentMatch = fileCommentPattern.match(code);
    if (commentMatch.hasMatch()) {
        return commentMatch.captured(1);
    }
    
    // Check for #include statements to infer C/C++ header files
    QRegularExpression includePattern(R"(#include\s*["<]([a-zA-Z_][\w/\-\.]*\.h(?:pp)?)[">");
    QRegularExpressionMatch includeMatch = includePattern.match(code);
    if (includeMatch.hasMatch()) {
        QString headerFile = includeMatch.captured(1);
        // If it's a local include (with quotes), it's likely the target file
        if (code.contains('"' + headerFile + '"')) {
            return headerFile;
        }
    }
    
    // Look for class definitions to infer filenames
    QRegularExpression classPattern(R"((?:class|struct|interface)\s+([A-Z][a-zA-Z0-9_]*)))");
    QRegularExpressionMatch classMatch = classPattern.match(code);
    if (classMatch.hasMatch()) {
        QString className = classMatch.captured(1);
        
        // Determine file extension based on code content
        QString extension;
        if (code.contains("#include") || code.contains("namespace") || 
            code.contains("public:") || code.contains("private:")) {
            extension = code.contains("template") ? ".hpp" : ".h";
        } else if (code.contains("function") || code.contains("const ") || 
                  code.contains("class ") || code.contains("import ")) {
            if (code.contains("import ") && code.contains("from ")) {
                extension = ".py";
            } else if (code.contains("function") && code.contains("{")) {
                extension = ".js";
            } else {
                extension = ".cpp";
            }
        } else {
            extension = ".cpp"; // Default for C++
        }
        
        return className.toLower() + extension;
    }
    
    // Look for function definitions to infer filenames
    QRegularExpression functionPattern(R"((?:void|int|bool|char|float|double|QString|std::\w+|auto)\s+([a-zA-Z_][\w]*))\s*\()");
    QRegularExpressionMatch functionMatch = functionPattern.match(code);
    if (functionMatch.hasMatch()) {
        QString functionName = functionMatch.captured(1);
        return functionName + ".cpp";
    }
    
    // Check for Python-specific patterns
    if (code.contains("def ") || code.contains("import ") || code.contains("from ")) {
        QRegularExpression defPattern(R"(def\s+([a-zA-Z_][\w]*))\s*\()");
        QRegularExpressionMatch defMatch = defPattern.match(code);
        if (defMatch.hasMatch()) {
            return defMatch.captured(1) + ".py";
        }
        return "main.py";
    }
    
    // Check for JavaScript/TypeScript patterns
    if (code.contains("function") || code.contains("const ") || code.contains("let ") || 
        code.contains("var ") || code.contains("=>")) {
        QRegularExpression jsPattern(R"((?:function|const|let|var)\s+([a-zA-Z_][\w]*)))");
        QRegularExpressionMatch jsMatch = jsPattern.match(code);
        if (jsMatch.hasMatch()) {
            QString extension = code.contains("interface") || code.contains("type ") ? ".ts" : ".js";
            return jsMatch.captured(1) + extension;
        }
        return "main.js";
    }
    
    // Check for HTML/CSS patterns
    if (code.contains("<html>") || code.contains("<!DOCTYPE") || code.contains("<body>")) {
        return "index.html";
    }
    if (code.contains("{") && code.contains("}") && (code.contains("color:") || code.contains("margin:"))) {
        return "styles.css";
    }
    
    // Check for configuration files
    if (code.contains("{") && code.contains("}") && (code.contains("\"name\"") || code.contains("\"version\""))) {
        return "package.json";
    }
    
    // Fallback: return a generic filename based on apparent language
    if (code.contains("#include") || code.contains("namespace") || code.contains("std::")) {
        return "main.cpp";
    } else if (code.contains("public class") || code.contains("import java")) {
        return "Main.java";
    } else if (code.contains("using") || code.contains("namespace")) {
        return "Program.cs";
    }
    
    // Final fallback
    return "code.txt";
}

QString ChatWidget::findFileInProject(const QString &fileName)
{
    // Try current directory and common project paths
    QStringList searchPaths = {
        ".",
        "./src",
        "./include",
        "../",
        "../src",
        "../include"
    };
    
    for (const QString &path : searchPaths) {
        QString fullPath = QDir(path).absoluteFilePath(fileName);
        if (QFile::exists(fullPath)) {
            return fullPath;
        }
        
        // Also try without path prefix if fileName includes path
        QString baseFileName = QFileInfo(fileName).fileName();
        QString fullPathBase = QDir(path).absoluteFilePath(baseFileName);
        if (QFile::exists(fullPathBase)) {
            return fullPathBase;
        }
    }
    
    // If not found, ask user to locate the file
    QString startDir = getLastUsedDirectory();
    QString selectedFile = QFileDialog::getOpenFileName(this,
        QString("Locate file: %1").arg(fileName),
        startDir,
        "All Files (*.*);; C++ Files (*.cpp *.h *.hpp);; Python Files (*.py)");
    
    if (!selectedFile.isEmpty()) {
        setLastUsedDirectory(QFileInfo(selectedFile).absolutePath());
    }
    
    return selectedFile;
}

QString ChatWidget::getLastUsedDirectory() const
{
    QSettings settings;
    QString lastDir = settings.value("filemanager/lastUsedDirectory", 
                                   QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    
    // Verify the directory still exists
    if (QDir(lastDir).exists()) {
        return lastDir;
    } else {
        // Fall back to home directory if saved directory no longer exists
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
}

void ChatWidget::setLastUsedDirectory(const QString &directory)
{
    if (!directory.isEmpty() && QDir(directory).exists()) {
        QSettings settings;
        settings.setValue("filemanager/lastUsedDirectory", directory);
        settings.sync(); // Ensure the setting is written immediately
    }
}
