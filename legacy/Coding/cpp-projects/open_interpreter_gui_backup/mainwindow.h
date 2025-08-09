#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QFont>
#include <QDateTime>

class ChatWidget;
class ModelConfigWidget;
class FileManagerWidget;
class InterpreterWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendMessage();
    void handleOutput(const QString &output, const QString &type);
    void handleFinished();
    void checkOllamaStatus();
    void startOllama();
    void checkModels();
    void openFile();
    void openFolder();
    void newChat();
    void saveChat();
    void showAbout();

private:
    void setupUI();
    void createMenuBar();
    void setupConnections();
    void addMessageToChat(const QString &message, const QString &sender);
    void populateFileTree(const QString &folderPath);
    
    // UI Components
    ChatWidget *m_chatWidget;
    ModelConfigWidget *m_modelConfigWidget;
    FileManagerWidget *m_fileManagerWidget;
    QStatusBar *m_statusBar;
    QProgressBar *m_progressBar;
    
    // Worker thread
    InterpreterWorker *m_worker;
    QTimer *m_statusTimer;
};

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    
    QTextEdit *getChatDisplay() const { return m_chatDisplay; }
    QLineEdit *getMessageInput() const { return m_messageInput; }
    QPushButton *getSendButton() const { return m_sendButton; }
    
    void addMessage(const QString &message, const QString &sender);

private:
    QTextEdit *m_chatDisplay;
    QLineEdit *m_messageInput;
    QPushButton *m_sendButton;
};

class ModelConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModelConfigWidget(QWidget *parent = nullptr);
    
    QString getCurrentModel() const;
    bool isContainerMode() const;
    bool isAutoRun() const;
    
    QLabel *getStatusLabel() const { return m_statusLabel; }
    QPushButton *getCheckStatusButton() const { return m_checkStatusButton; }
    QPushButton *getStartOllamaButton() const { return m_startOllamaButton; }

private:
    QComboBox *m_modelCombo;
    QCheckBox *m_autoRunCheckBox;
    QCheckBox *m_containerModeCheckBox;
    QLabel *m_statusLabel;
    QPushButton *m_checkStatusButton;
    QPushButton *m_startOllamaButton;
};

class FileManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerWidget(QWidget *parent = nullptr);
    
    QPushButton *getOpenFileButton() const { return m_openFileButton; }
    QPushButton *getOpenFolderButton() const { return m_openFolderButton; }
    QTreeWidget *getFileTree() const { return m_fileTree; }
    QTextEdit *getFileViewer() const { return m_fileViewer; }

private:
    QPushButton *m_openFileButton;
    QPushButton *m_openFolderButton;
    QTreeWidget *m_fileTree;
    QTextEdit *m_fileViewer;
};

class InterpreterWorker : public QObject
{
    Q_OBJECT

public:
    explicit InterpreterWorker(const QString &message, const QString &model, bool containerMode, QObject *parent = nullptr);
    
public slots:
    void run();
    void stop();

signals:
    void outputReceived(const QString &output, const QString &type);
    void finished();

private:
    QString m_message;
    QString m_model;
    bool m_containerMode;
    QProcess *m_process;
};

#endif // MAINWINDOW_H
