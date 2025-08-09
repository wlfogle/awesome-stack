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
    
    QString currentProjectPath;
    QStringList codeFiles;
    
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
};

#endif // MAINWINDOW_H

