#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QProcess>
#include <QTimer>
#include <QScrollBar>
#include <QFont>
#include <QTextCursor>
#include <QKeyEvent>

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(const QString &workingDir = QString(), QWidget *parent = nullptr);
    ~TerminalWidget();

    void executeCommand(const QString &command);
    QString workingDirectory() const;
    void setWorkingDirectory(const QString &dir);
    
    void clear();
    void copy();
    void paste();
    void selectAll();
    
    void setFont(const QFont &font);
    void setColorScheme(const QString &scheme);

signals:
    void titleChanged(const QString &title);
    void workingDirectoryChanged(const QString &dir);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessReadyRead();
    void updatePrompt();
    void handleInput();

private:
    void setupUI();
    void setupProcess();
    void initializeTerminal();
    void appendOutput(const QString &text, const QColor &color = QColor());
    void scrollToBottom();
    void updateTitle();
    QString getCurrentPrompt() const;
    QString formatOutput(const QString &text) const;
    
    // UI Components
    QVBoxLayout *m_layout;
    QTextEdit *m_terminalOutput;
    QLineEdit *m_commandInput;
    
    // Process management
    QProcess *m_process;
    QString m_workingDir;
    QString m_shell;
    QStringList m_commandHistory;
    int m_historyIndex;
    
    // Terminal state
    QString m_currentCommand;
    bool m_processRunning;
    QString m_lastPrompt;
    
    // Appearance
    QFont m_terminalFont;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_promptColor;
    QColor m_errorColor;
    
    // Timer for prompt updates
    QTimer *m_promptTimer;
};

#endif // TERMINALWIDGET_H
