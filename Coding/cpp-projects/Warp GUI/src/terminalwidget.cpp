#include "terminalwidget.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

TerminalWidget::TerminalWidget(const QString &workingDir, QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_terminalOutput(nullptr)
    , m_commandInput(nullptr)
    , m_process(nullptr)
    , m_workingDir(workingDir.isEmpty() ? QDir::currentPath() : workingDir)
    , m_shell("fish") // Default to fish shell
    , m_historyIndex(-1)
    , m_processRunning(false)
    , m_promptTimer(nullptr)
{
    // Set up appearance
    m_terminalFont = QFont("monospace", 10);
    m_terminalFont.setStyleHint(QFont::Monospace);
    m_terminalFont.setFamily("monospace");
    
    m_backgroundColor = QColor(25, 25, 25);
    m_textColor = QColor(255, 255, 255);
    m_promptColor = QColor(42, 130, 218);
    m_errorColor = QColor(255, 100, 100);
    
    setupUI();
    setupProcess();
    initializeTerminal();
}

TerminalWidget::~TerminalWidget()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

void TerminalWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    
    // Terminal output
    m_terminalOutput = new QTextEdit();
    m_terminalOutput->setReadOnly(true);
    m_terminalOutput->setFont(m_terminalFont);
    m_terminalOutput->setStyleSheet(QString(
        "QTextEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: none;"
        "    padding: 10px;"
        "}"
        "QScrollBar:vertical {"
        "    background: rgba(255, 255, 255, 0.1);"
        "    width: 12px;"
        "    border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgba(255, 255, 255, 0.3);"
        "    border-radius: 6px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: rgba(255, 255, 255, 0.5);"
        "}"
    ).arg(m_backgroundColor.name()).arg(m_textColor.name()));
    
    // Command input
    m_commandInput = new QLineEdit();
    m_commandInput->setFont(m_terminalFont);
    m_commandInput->setStyleSheet(QString(
        "QLineEdit {"
        "    background-color: %1;"
        "    color: %2;"
        "    border: 1px solid #555;"
        "    padding: 8px;"
        "    border-radius: 4px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: %3;"
        "}"
    ).arg(m_backgroundColor.name()).arg(m_textColor.name()).arg(m_promptColor.name()));
    
    m_layout->addWidget(m_terminalOutput);
    m_layout->addWidget(m_commandInput);
    
    // Connect signals
    connect(m_commandInput, &QLineEdit::returnPressed, this, &TerminalWidget::handleInput);
    
    // Set focus to command input
    m_commandInput->setFocus();
}

void TerminalWidget::setupProcess()
{
    m_process = new QProcess(this);
    m_process->setWorkingDirectory(m_workingDir);
    
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalWidget::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &TerminalWidget::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &TerminalWidget::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &TerminalWidget::onProcessReadyRead);
}

void TerminalWidget::initializeTerminal()
{
    // Display welcome message
    appendOutput("Warp Terminal GUI - Terminal Ready\n", m_promptColor);
    appendOutput(QString("Working Directory: %1\n").arg(m_workingDir), m_textColor);
    appendOutput(QString("Shell: %1\n\n").arg(m_shell), m_textColor);
    
    // Start prompt timer
    m_promptTimer = new QTimer(this);
    connect(m_promptTimer, &QTimer::timeout, this, &TerminalWidget::updatePrompt);
    m_promptTimer->start(100);
    
    updatePrompt();
    updateTitle();
}

void TerminalWidget::executeCommand(const QString &command)
{
    if (command.isEmpty()) {
        return;
    }
    
    m_commandInput->setText(command);
    handleInput();
}

QString TerminalWidget::workingDirectory() const
{
    return m_workingDir;
}

void TerminalWidget::setWorkingDirectory(const QString &dir)
{
    if (QDir(dir).exists()) {
        m_workingDir = dir;
        if (m_process) {
            m_process->setWorkingDirectory(m_workingDir);
        }
        emit workingDirectoryChanged(m_workingDir);
        updateTitle();
    }
}

void TerminalWidget::clear()
{
    m_terminalOutput->clear();
    updatePrompt();
}

void TerminalWidget::copy()
{
    if (m_terminalOutput->textCursor().hasSelection()) {
        QApplication::clipboard()->setText(m_terminalOutput->textCursor().selectedText());
    }
}

void TerminalWidget::paste()
{
    QString text = QApplication::clipboard()->text();
    if (!text.isEmpty()) {
        m_commandInput->insert(text);
    }
}

void TerminalWidget::selectAll()
{
    m_terminalOutput->selectAll();
}

void TerminalWidget::setFont(const QFont &font)
{
    m_terminalFont = font;
    m_terminalOutput->setFont(font);
    m_commandInput->setFont(font);
}

void TerminalWidget::setColorScheme(const QString &scheme)
{
    // Implement different color schemes
    if (scheme == "dark") {
        m_backgroundColor = QColor(25, 25, 25);
        m_textColor = QColor(255, 255, 255);
    } else if (scheme == "light") {
        m_backgroundColor = QColor(255, 255, 255);
        m_textColor = QColor(0, 0, 0);
    }
    // Update UI with new colors
    setupUI();
}

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    // Handle terminal-specific key combinations
    if (event->modifiers() & Qt::ControlModifier) {
        switch (event->key()) {
        case Qt::Key_C:
            if (m_processRunning) {
                // Send interrupt signal
                if (m_process && m_process->state() == QProcess::Running) {
                    m_process->terminate();
                }
                return;
            }
            break;
        case Qt::Key_V:
            paste();
            return;
        case Qt::Key_A:
            selectAll();
            return;
        case Qt::Key_L:
            clear();
            return;
        }
    }
    
    // Handle history navigation
    if (event->key() == Qt::Key_Up) {
        if (m_historyIndex > 0) {
            m_historyIndex--;
            m_commandInput->setText(m_commandHistory[m_historyIndex]);
        }
        return;
    } else if (event->key() == Qt::Key_Down) {
        if (m_historyIndex < m_commandHistory.size() - 1) {
            m_historyIndex++;
            m_commandInput->setText(m_commandHistory[m_historyIndex]);
        } else {
            m_historyIndex = m_commandHistory.size();
            m_commandInput->clear();
        }
        return;
    }
    
    QWidget::keyPressEvent(event);
}

void TerminalWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    m_commandInput->setFocus();
}

void TerminalWidget::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_processRunning = false;
    
    QString statusText;
    if (exitStatus == QProcess::CrashExit) {
        statusText = QString("Process crashed (exit code: %1)").arg(exitCode);
        appendOutput(statusText + "\n", m_errorColor);
    } else if (exitCode != 0) {
        statusText = QString("Process finished with exit code: %1").arg(exitCode);
        appendOutput(statusText + "\n", m_errorColor);
    }
    
    updatePrompt();
}

void TerminalWidget::onProcessError(QProcess::ProcessError error)
{
    m_processRunning = false;
    
    QString errorText;
    switch (error) {
    case QProcess::FailedToStart:
        errorText = "Failed to start process";
        break;
    case QProcess::Crashed:
        errorText = "Process crashed";
        break;
    case QProcess::Timedout:
        errorText = "Process timed out";
        break;
    case QProcess::ReadError:
        errorText = "Read error";
        break;
    case QProcess::WriteError:
        errorText = "Write error";
        break;
    default:
        errorText = "Unknown error";
        break;
    }
    
    appendOutput("Error: " + errorText + "\n", m_errorColor);
    updatePrompt();
}

void TerminalWidget::onProcessReadyRead()
{
    if (!m_process) return;
    
    QByteArray data = m_process->readAllStandardOutput();
    if (!data.isEmpty()) {
        QString output = QString::fromUtf8(data);
        appendOutput(formatOutput(output), m_textColor);
    }
    
    data = m_process->readAllStandardError();
    if (!data.isEmpty()) {
        QString output = QString::fromUtf8(data);
        appendOutput(formatOutput(output), m_errorColor);
    }
}

void TerminalWidget::updatePrompt()
{
    if (!m_processRunning) {
        QString prompt = getCurrentPrompt();
        if (prompt != m_lastPrompt) {
            m_lastPrompt = prompt;
            m_commandInput->setPlaceholderText(prompt);
        }
    }
}

void TerminalWidget::handleInput()
{
    QString command = m_commandInput->text().trimmed();
    if (command.isEmpty()) {
        return;
    }
    
    // Add to history
    if (m_commandHistory.isEmpty() || m_commandHistory.last() != command) {
        m_commandHistory.append(command);
        if (m_commandHistory.size() > 1000) {
            m_commandHistory.removeFirst();
        }
    }
    m_historyIndex = m_commandHistory.size();
    
    // Display command in output
    appendOutput(getCurrentPrompt() + command + "\n", m_promptColor);
    
    // Clear input
    m_commandInput->clear();
    
    // Handle built-in commands
    if (command == "clear") {
        clear();
        return;
    } else if (command.startsWith("cd ")) {
        QString newDir = command.mid(3).trimmed();
        if (newDir.isEmpty()) {
            newDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        } else if (newDir.startsWith("~")) {
            newDir.replace(0, 1, QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        } else if (!QDir::isAbsolutePath(newDir)) {
            newDir = QDir(m_workingDir).absoluteFilePath(newDir);
        }
        
        if (QDir(newDir).exists()) {
            setWorkingDirectory(newDir);
            appendOutput("", m_textColor);
        } else {
            appendOutput("cd: no such file or directory: " + newDir + "\n", m_errorColor);
        }
        updatePrompt();
        return;
    }
    
    // Execute external command
    m_processRunning = true;
    m_currentCommand = command;
    
    // Use the shell to execute the command
    QStringList arguments;
    arguments << "-c" << command;
    
    m_process->start(m_shell, arguments);
    
    if (!m_process->waitForStarted()) {
        m_processRunning = false;
        appendOutput("Failed to start command: " + command + "\n", m_errorColor);
        updatePrompt();
    }
}

void TerminalWidget::appendOutput(const QString &text, const QColor &color)
{
    if (text.isEmpty()) return;
    
    QTextCursor cursor = m_terminalOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    
    QTextCharFormat format;
    if (color.isValid()) {
        format.setForeground(color);
    } else {
        format.setForeground(m_textColor);
    }
    
    cursor.setCharFormat(format);
    cursor.insertText(text);
    
    scrollToBottom();
}

void TerminalWidget::scrollToBottom()
{
    QScrollBar *scrollBar = m_terminalOutput->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void TerminalWidget::updateTitle()
{
    QString title = QFileInfo(m_workingDir).baseName();
    if (title.isEmpty()) {
        title = "Terminal";
    }
    emit titleChanged(title);
}

QString TerminalWidget::getCurrentPrompt() const
{
    QString user = qgetenv("USER");
    if (user.isEmpty()) {
        user = "user";
    }
    
    QString hostname = qgetenv("HOSTNAME");
    if (hostname.isEmpty()) {
        hostname = "localhost";
    }
    
    QString shortPath = QFileInfo(m_workingDir).baseName();
    if (shortPath.isEmpty()) {
        shortPath = "/";
    }
    
    return QString("[%1@%2 %3] $ ").arg(user, hostname, shortPath);
}

QString TerminalWidget::formatOutput(const QString &text) const
{
    // Remove ANSI escape sequences for now
    QString result = text;
    QRegularExpression ansiRegex("\x1b\\[[0-9;]*[a-zA-Z]");
    result.remove(ansiRegex);
    return result;
}
