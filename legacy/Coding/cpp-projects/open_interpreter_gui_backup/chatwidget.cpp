#include "mainwindow.h"

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent),
      m_chatDisplay(new QTextEdit(this)),
      m_messageInput(new QLineEdit(this)),
      m_sendButton(new QPushButton("Send", this))
{
    QVBoxLayout *layout = new QVBoxLayout;

    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setFont(QFont("Arial", 10));
    m_chatDisplay->setStyleSheet("QTextEdit { background-color: #1a1a1a; color: #e0e0e0; }");
    layout->addWidget(m_chatDisplay);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    m_messageInput->setPlaceholderText("Ask me to write, debug, or analyze code...");
    m_messageInput->setFont(QFont("Arial", 10));
    m_messageInput->setStyleSheet("QLineEdit { padding: 8px; border: 2px solid #007acc; }");
    inputLayout->addWidget(m_messageInput);

    m_sendButton->setStyleSheet("QPushButton { background-color: #007acc; color: white; }");
    inputLayout->addWidget(m_sendButton);

    layout->addLayout(inputLayout);

    setLayout(layout);
}

void ChatWidget::addMessage(const QString &message, const QString &sender)
{
    QString color = (sender == "user") ? "#007acc" : "#28a745";
    QString prefix = (sender == "user") ? "👤 You" : "🤖 AI Assistant";
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss]");

    QString formattedMessage = QStringLiteral(
        "<div style='margin:10px 0;padding:10px;border-left:4px solid %1;'>"
        "<b style='color:%1;'>%2</b> <span style='color:#6c757d;'>%3</span><br>"
        "<div style='margin-top:5px;'>%4</div>"
        "</div>"
    ).arg(color, prefix, timestamp, message);

    m_chatDisplay->append(formattedMessage);
}

// Additional widget implementation will follow here. This is the ChatWidget example implementation.
