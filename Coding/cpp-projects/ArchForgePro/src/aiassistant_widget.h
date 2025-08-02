#ifndef AIASSISTANT_WIDGET_H
#define AIASSISTANT_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class AIManager;

class AIAssistantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AIAssistantWidget(QWidget *parent = nullptr);
    ~AIAssistantWidget();

public slots:
    // AI features
    void processAIQuery();
    void generateAIRecommendations();
    void analyzePackage();
    void sendQuickPrompt(const QString &prompt);
    void clearAIChat();

private:
    void setupUI();
    void setupConnections();
    
    // Tab creation methods
    QWidget* createAIChatTab();
    QWidget* createAIRecommendationsTab();
    QWidget* createAIAnalysisTab();
    QWidget* createAISettingsTab();
    
    // Core components
    AIManager *m_aiManager;
    
    // Main UI components
    QTabWidget *m_aiAssistantTabWidget;
    
    // AI assistant widgets
    QTextEdit *m_aiChatDisplay;
    QLineEdit *m_aiInput;
    QPushButton *m_aiSendButton;
};

#endif // AIASSISTANT_WIDGET_H
