#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QDateTime>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QTextCursor>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QSettings>
#include <QStandardPaths>

class ChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    void addMessage(const QString &message, const QString &sender = "user");
    QPushButton* getSendButton() const { return sendButton; }
    QLineEdit* getMessageInput() const { return messageInput; }

private slots:
    void onSendClicked();
    void showContextMenu(const QPoint &pos);
    void onLinkClicked(const QUrl &url);

signals:
    void messageSent(const QString& message);
    void testRequested();
    void buildRequested();
    void runRequested();
    void fileNavigationRequested(const QString& filePath, int lineNumber);
    void aiCommandRequested(const QString& command, const QString& selectedText);
    void applyFixRequested(const QString& selectedText);

private:
    QTextEdit *chatDisplay;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QLineEdit *quickCommandInput;
    QPushButton *quickCommandButton;
    void setupUi();
    void setupStyles();
    QString makeFileLinksClickable(const QString &message);
    void parseAndApplyFixes(const QString &aiResponse);
    QString findFileInProject(const QString &fileName);
    QString inferFilenameFromCode(const QString &codeSnippet);
    QString getLastUsedDirectory() const;
    void setLastUsedDirectory(const QString &directory);
};

#endif // CHATWIDGET_H
