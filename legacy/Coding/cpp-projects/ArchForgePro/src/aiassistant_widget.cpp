#include "aiassistant_widget.h"
#include <QMessageBox>

AIAssistantWidget::AIAssistantWidget(QWidget *parent)
    : QWidget(parent)
    , m_aiManager(nullptr)
    , m_aiAssistantTabWidget(nullptr)
{
    setupUI();
    setupConnections();
}

AIAssistantWidget::~AIAssistantWidget()
{
    // Qt handles cleanup automatically
}

void AIAssistantWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_aiAssistantTabWidget = new QTabWidget(this);
    m_aiAssistantTabWidget->addTab(createAIChatTab(), "Chat");
    m_aiAssistantTabWidget->addTab(createAIRecommendationsTab(), "Recommendations");
    m_aiAssistantTabWidget->addTab(createAIAnalysisTab(), "Analysis");
    m_aiAssistantTabWidget->addTab(createAISettingsTab(), "Settings");
    
    mainLayout->addWidget(m_aiAssistantTabWidget);
}

void AIAssistantWidget::setupConnections()
{
    connect(m_aiSendButton, &QPushButton::clicked, this, &AIAssistantWidget::processAIQuery);
}

QWidget* AIAssistantWidget::createAIChatTab()
{
    QWidget *chatTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(chatTab);
    
    // Chat display
    m_aiChatDisplay = new QTextEdit();
    m_aiChatDisplay->setReadOnly(true);
    m_aiChatDisplay->setPlaceholderText("AI Assistant chat will appear here...");
    layout->addWidget(m_aiChatDisplay);
    
    // Input section
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_aiInput = new QLineEdit();
    m_aiInput->setPlaceholderText("Type your question here...");
    m_aiSendButton = new QPushButton("Send");
    
    inputLayout->addWidget(m_aiInput);
    inputLayout->addWidget(m_aiSendButton);
    layout->addLayout(inputLayout);
    
    return chatTab;
}

QWidget* AIAssistantWidget::createAIRecommendationsTab()
{
    QWidget *recommendationsTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(recommendationsTab);
    
    layout->addWidget(new QLabel("AI Recommendations will be displayed here"));
    
    return recommendationsTab;
}

QWidget* AIAssistantWidget::createAIAnalysisTab()
{
    QWidget *analysisTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(analysisTab);
    
    layout->addWidget(new QLabel("AI Analysis will be displayed here"));
    
    return analysisTab;
}

QWidget* AIAssistantWidget::createAISettingsTab()
{
    QWidget *settingsTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(settingsTab);
    
    layout->addWidget(new QLabel("AI Settings will be displayed here"));
    
    return settingsTab;
}

// AI functionality slots
void AIAssistantWidget::processAIQuery()
{
    QString query = m_aiInput->text();
    if (!query.isEmpty()) {
        m_aiChatDisplay->append("You: " + query);
        m_aiInput->clear();
        // TODO: Implement AI processing
        m_aiChatDisplay->append("AI: I'm not yet connected to an AI service.");
    }
}

void AIAssistantWidget::generateAIRecommendations()
{
    // TODO: Implement AI recommendations
    QMessageBox::information(this, "AI Recommendations", "AI recommendations not yet implemented.");
}

void AIAssistantWidget::analyzePackage()
{
    // TODO: Implement package analysis
    QMessageBox::information(this, "AI Analysis", "AI package analysis not yet implemented.");
}

void AIAssistantWidget::sendQuickPrompt(const QString &prompt)
{
    m_aiInput->setText(prompt);
    processAIQuery();
}

void AIAssistantWidget::clearAIChat()
{
    m_aiChatDisplay->clear();
}
