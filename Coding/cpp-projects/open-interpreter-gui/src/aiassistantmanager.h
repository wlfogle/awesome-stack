#ifndef AIASSISTANTMANAGER_H
#define AIASSISTANTMANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QTextDocument>
#include <QTextCursor>
#ifdef HAVE_TEXTTOSPEECH
#include <QTextToSpeech>
#endif
#include <QTranslator>
#include <QMutex>
#include <QMutexLocker>

struct CodeSuggestion {
    QString type;           // "error", "warning", "optimization", "suggestion"
    QString description;
    QString fixedCode;
    int lineNumber;
    QString filePath;
    float confidence;
};

struct UserBehaviorData {
    QString action;
    QString context;
    QDateTime timestamp;
    QString projectType;
    QString fileType;
    int duration;
};

struct ModelPerformance {
    QString modelName;
    QString taskType;
    QString language;
    float avgResponseTime;
    float successRate;
    int usageCount;
    QDateTime lastUsed;
};

struct CodeChangeInfo {
    QString filePath;
    QString lastAnalyzedCode;
    QDateTime lastAnalysisTime;
    QString codeHash;
    int lineCount;
    float complexity;
};

class AIAssistantManager : public QObject
{
    Q_OBJECT

public:
    explicit AIAssistantManager(QObject *parent = nullptr);
    
    // Real-time analysis
    void analyzeCodeRealtime(const QString &code, const QString &filePath);
    void setRealtimeEnabled(bool enabled);
    
    // Contextual suggestions
    QList<CodeSuggestion> getContextualSuggestions(const QString &currentCode, 
                                                   const QString &fileType);
    
    // Predictive paths
    QStringList predictDirectoryPaths(const QString &currentPath, 
                                      const QString &projectType);
    
    // Voice commands
    void startVoiceRecognition();
    void stopVoiceRecognition();
    void speakText(const QString &text);
    
    // User behavior analysis
    void trackUserAction(const QString &action, const QString &context);
    QList<QString> getPersonalizedInsights();
    
    // Performance insights
    void analyzePerformance(const QString &code, const QString &language);
    
    // Test generation
    QString generateTests(const QString &code, const QString &language);
    
    // Multilingual support
    void setLanguage(const QString &languageCode);
    QString translateText(const QString &text, const QString &targetLanguage);
    
    // External API integration
    void integrateWithGitHub(const QString &token);
    void integrateWithJira(const QString &apiKey, const QString &domain);

public slots:
    void onCodeChanged(const QString &code, const QString &filePath);
    void onVoiceCommandReceived(const QString &command);
    void processUserBehavior();

signals:
    void realtimeSuggestionReady(const CodeSuggestion &suggestion);
    void contextualSuggestionsReady(const QList<CodeSuggestion> &suggestions);
    void predictedPathsReady(const QStringList &paths);
    void voiceCommandProcessed(const QString &command, const QString &result);
    void performanceInsightReady(const QString &insight);
    void testCodeGenerated(const QString &testCode);
    void userInsightReady(const QString &insight);
    void externalApiResponse(const QString &service, const QJsonObject &data);

private slots:
    void onRealtimeAnalysisReply();
    void onPerformanceAnalysisReply();
    void onTestGenerationReply();
    void onVoiceRecognitionFinished();

private:
    QNetworkAccessManager *networkManager;
    QTimer *realtimeTimer;
    // QSpeechRecognition *speechRecognition; // Not available in standard Qt
#ifdef HAVE_TEXTTOSPEECH
    QTextToSpeech *textToSpeech;
#endif
    QTranslator *translator;
    
    bool realtimeEnabled;
    QString currentLanguage;
    QString githubToken;
    QString jiraApiKey;
    QString jiraDomain;
    
    QList<UserBehaviorData> behaviorHistory;
    QStringList frequentPaths;
    QMap<QString, QStringList> contextualPatterns;
    
    // Incremental analysis
    QMap<QString, CodeChangeInfo> fileAnalysisCache;
    QTimer *incrementalAnalysisTimer;
    QString pendingAnalysisFile;
    QString pendingAnalysisDiff;
    
    // Smart model selection
    QList<ModelPerformance> modelPerformanceHistory;
    QSettings *performanceSettings;
    QMap<QString, QDateTime> modelLastUsed;
    QMap<QString, float> modelCurrentLoad;
    
    // Performance tracking
    QMap<QString, QList<float>> modelResponseTimes;
    QMap<QString, QList<bool>> modelSuccessRates;
    
    // Thread safety
    QMutex cacheMutex;
    
    // AI model endpoints
    QString getOptimalModel(const QString &task, const QString &language, 
                           const QString &priority = "balanced");
    void callAIService(const QString &prompt, const QString &model, 
                       const QString &task, const QJsonObject &context = QJsonObject());
    
    // Incremental analysis helpers
    QString calculateCodeHash(const QString &code);
    QString generateCodeDiff(const QString &oldCode, const QString &newCode);
    bool isSignificantChange(const QString &diff, const QString &fileType);
    float calculateComplexity(const QString &code);
    void processIncrementalAnalysis();
    
    // Model performance tracking
    void updateModelPerformance(const QString &model, const QString &task, 
                               float responseTime, bool success);
    float getModelScore(const QString &model, const QString &task, const QString &language);
    void loadModelPerformanceData();
    void saveModelPerformanceData();
    
    // Analysis helpers
    QString createRealtimePrompt(const QString &code, const QString &filePath);
    QString createContextualPrompt(const QString &code, const QString &fileType);
    QString createPerformancePrompt(const QString &code, const QString &language);
    QString createTestPrompt(const QString &code, const QString &language);
    
    // Behavior analysis
    void analyzeUserPatterns();
    void updatePredictivePaths(const QString &path);
    void saveUserBehavior();
    void loadUserBehavior();
    
    // Voice processing
    void processVoiceCommand(const QString &command);
    
    // External API helpers
    void callGitHubAPI(const QString &endpoint, const QJsonObject &data);
    void callJiraAPI(const QString &endpoint, const QJsonObject &data);
};

#endif // AIASSISTANTMANAGER_H
