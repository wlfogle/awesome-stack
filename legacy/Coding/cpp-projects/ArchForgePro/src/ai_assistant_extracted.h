// AI Assistant functionality extracted from Universal Arch Installer
// Save this code for later integration into ArchForgePro

#ifndef AI_ASSISTANT_EXTRACTED_H
#define AI_ASSISTANT_EXTRACTED_H

#include <QtWidgets>
#include <QTabWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>

class AIAssistantExtracted {
private:
    // AI Assistant tab widgets
    QTabWidget* m_aiAssistantTabWidget;
    
    // AI Chat tab
    QTextEdit* m_aiChatDisplay;
    QLineEdit* m_aiInput;
    QPushButton* m_aiSendButton;
    
    // AI Recommendations tab
    QTableWidget* m_aiRecommendationsTable;
    QComboBox* m_recCategoryCombo;
    
    // AI Analysis tab
    QLineEdit* m_analysisPackageInput;
    QTextEdit* m_analysisResults;
    
    // AI Settings tab
    QCheckBox* m_aiEnabledCheck;
    QCheckBox* m_verboseAICheck;
    QCheckBox* m_aiCacheCheck;
    QSpinBox* m_aiTimeoutSpinBox;

public:
    // AI methods
    void processAIQuery();
    void generateAIRecommendations();
    void analyzePackage();
    void sendQuickPrompt(const QString &prompt);
    void clearAIChat();
    
    // AI Assistant tab creation
    QWidget* createAIAssistantTab();
    QWidget* createAIChatTab();
    QWidget* createAIRecommendationsTab();
    QWidget* createAIAnalysisTab();
    QWidget* createAISettingsTab();
};

#endif // AI_ASSISTANT_EXTRACTED_H
