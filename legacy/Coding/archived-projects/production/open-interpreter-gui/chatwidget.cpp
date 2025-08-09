#include "chatwidget.h"

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
    quickActionsLayout->addWidget(testBtn);
    quickActionsLayout->addWidget(optimizeBtn);
    quickActionsLayout->addWidget(buildBtn);
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
    
    // Add actions to menu
    contextMenu.addAction(copyAction);
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
