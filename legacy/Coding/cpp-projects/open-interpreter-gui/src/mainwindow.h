#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QStatusBar>
#include <QPushButton>
#include <QTimer>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QThread>
#include "chatwidget.h"
#include "modelconfigwidget.h"
#include "filemanagerwidget.h"
#include "interpreterworker.h"
#include "aiassistantmanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onMessageSent(const QString &message);
    void onWorkerOutput(const QString &output, const QString &type);
    void onWorkerFinished();
    void onWorkerError(const QString &error);
    void onCancelClicked();
    void onFolderChanged(const QString &path);
    void onTerminalRequested();
    void onBuildRequested();
    void onTestRequested();
    void onRunRequested();
    void checkOllamaStatus();
    void startOllama();
    void onAISuggestionReady(const CodeSuggestion &suggestion);

private:
    ChatWidget *chatWidget;
    ModelConfigWidget *modelConfigWidget;
    FileManagerWidget *fileManagerWidget;
    QProgressBar *progressBar;
    QPushButton *cancelButton;
    QFileSystemWatcher *folderWatcher;
    QTimer *statusTimer;
    InterpreterWorker *worker;
    QThread *workerThread;
    AIAssistantManager *aiAssistant;
    
    QString currentProjectPath;
    QStringList codeFiles;
    QString accumulatedAIResponse;  // Accumulate AI responses for auto-fix processing
    
    void setupUi();
    void setupConnections();
    void setupStatusBar();
    void detectCodeFiles(const QString &folderPath);
    QString analyzeCodeAndSelectModel(const QStringList &files);
    QString createAnalysisPrompt(const QString &message, const QStringList &files, bool fullProject = false);
    QStringList getCodeFileExtensions();
    bool isCodeFile(const QString &filePath);
    QString readFileContent(const QString &filePath, int maxLines = 500);
    void startAnalysis(const QString &message, bool fullProject = false);
    void showProgress(const QString &message);
    void hideProgress();
    
    // Auto-fix functionality
    void parseAndApplyFixes(const QString &aiResponse);
    QString findFileInProject(const QString &fileName);
    bool applyCodeFix(const QString &filePath, int lineNumber, const QString &fixedCode, const QString &description);
    QString createBackupFile(const QString &filePath);
    void showFixSummary(const QStringList &appliedFixes, const QStringList &failedFixes);
    
    // Enhanced fix application methods
    void applyIncludeFix(QStringList &fileLines, const QStringList &fixLines);
    void applyClassFix(QStringList &fileLines, const QStringList &fixLines, int lineNumber);
    void applyFunctionFix(QStringList &fileLines, const QStringList &fixLines, int lineNumber, const QString &description);
    void applyMainFix(QStringList &fileLines, const QStringList &fixLines);
    void applyGenericFix(QStringList &fileLines, const QStringList &fixLines, const QString &description);
};

#endif // MAINWINDOW_H

