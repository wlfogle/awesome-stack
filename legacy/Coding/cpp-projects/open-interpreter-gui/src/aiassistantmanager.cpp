#include "aiassistantmanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>
#include <QApplication>
#include <QLocale>

AIAssistantManager::AIAssistantManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , realtimeTimer(new QTimer(this))
    // , speechRecognition(nullptr) // Not available in standard Qt
#ifdef HAVE_TEXTTOSPEECH
    , textToSpeech(new QTextToSpeech(this))
#endif
    , translator(new QTranslator(this))
    , realtimeEnabled(true)
    , currentLanguage("en")
    , incrementalAnalysisTimer(new QTimer(this))
    , performanceSettings(new QSettings("OpenInterpreter", "ModelPerformance", this))
{
    // Setup real-time analysis timer
    realtimeTimer->setSingleShot(true);
    realtimeTimer->setInterval(1500); // 1.5 second delay to avoid spam
    connect(realtimeTimer, &QTimer::timeout, this, &AIAssistantManager::processUserBehavior);
    
    // Setup incremental analysis timer
    incrementalAnalysisTimer->setSingleShot(true);
    incrementalAnalysisTimer->setInterval(800); // Faster response for incremental changes
    connect(incrementalAnalysisTimer, &QTimer::timeout, [this]() {
        if (!pendingAnalysisFile.isEmpty()) {
            processIncrementalAnalysis();
        }
    });
    
    // Load user behavior and performance data
    loadUserBehavior();
    loadModelPerformanceData();
    
    // Initialize text-to-speech
#ifdef HAVE_TEXTTOSPEECH
    if (textToSpeech) {
        textToSpeech->setLocale(QLocale::English);
        textToSpeech->setRate(0.5);
        textToSpeech->setVolume(0.7);
    }
#endif
    
    qDebug() << "🤖 AI Assistant Manager initialized with incremental analysis and smart model selection";
}

void AIAssistantManager::analyzeCodeRealtime(const QString &code, const QString &filePath)
{
    if (!realtimeEnabled || code.length() < 10) return;
    
    // Use thread-safe access to behavioral data
    QMutexLocker locker(&cacheMutex);
    
    trackUserAction("code_edit", QString("file:%1,length:%2").arg(filePath).arg(code.length()));
    
    // Check if we should use incremental analysis
    if (fileAnalysisCache.contains(filePath)) {
        CodeChangeInfo &info = fileAnalysisCache[filePath];
        QString newHash = calculateCodeHash(code);
        
        if (info.codeHash != newHash) {
            QString diff = generateCodeDiff(info.lastAnalyzedCode, code);
            QString fileType = QFileInfo(filePath).suffix();
            
            if (isSignificantChange(diff, fileType)) {
                // Use incremental analysis for significant changes
                pendingAnalysisFile = filePath;
                pendingAnalysisDiff = diff;
                incrementalAnalysisTimer->stop();
                incrementalAnalysisTimer->start();
                
                // Update cache with bounds checking
                info.lastAnalyzedCode = code.left(10000); // Limit cache size
                info.codeHash = newHash;
                info.lastAnalysisTime = QDateTime::currentDateTime();
                info.lineCount = qMin(10000, code.split('\n').count()); // Prevent overflow
                info.complexity = calculateComplexity(code);
                return;
            }
        }
    } else {
        // First time analyzing this file
        CodeChangeInfo info;
        info.filePath = filePath;
        info.lastAnalyzedCode = code.left(10000); // Limit cache size
        info.codeHash = calculateCodeHash(code);
        info.lastAnalysisTime = QDateTime::currentDateTime();
        info.lineCount = qMin(10000, code.split('\n').count()); // Prevent overflow
        info.complexity = calculateComplexity(code);
        fileAnalysisCache[filePath] = info;
    }
    
    // Debounce rapid changes for full analysis
    realtimeTimer->stop();
    realtimeTimer->start();
    
    QString prompt = createRealtimePrompt(code, filePath);
    QString fileType = QFileInfo(filePath).suffix();
    QString model = getOptimalModel("realtime_analysis", fileType, "speed");
    
    QJsonObject context;
    context["code"] = code;
    context["filePath"] = filePath;
    context["fileType"] = fileType;
    context["analysis_type"] = "realtime";
    context["is_incremental"] = false;
    
    callAIService(prompt, model, "realtime_analysis", context);
}

void AIAssistantManager::setRealtimeEnabled(bool enabled)
{
    realtimeEnabled = enabled;
    qDebug() << "🔄 Real-time analysis" << (enabled ? "enabled" : "disabled");
}

QList<CodeSuggestion> AIAssistantManager::getContextualSuggestions(const QString &currentCode, const QString &fileType)
{
    QList<CodeSuggestion> suggestions;
    
    // Analyze current context and user patterns
    QString context = QString("fileType:%1,codeLength:%2").arg(fileType).arg(currentCode.length());
    
    // Check contextual patterns from user behavior
    if (contextualPatterns.contains(fileType)) {
        QStringList patterns = contextualPatterns[fileType];
        for (const QString &pattern : patterns) {
            if (currentCode.contains(pattern)) {
                CodeSuggestion suggestion;
                suggestion.type = "suggestion";
                suggestion.description = QString("Consider using common pattern: %1").arg(pattern);
                suggestion.confidence = 0.8f;
                suggestions.append(suggestion);
            }
        }
    }
    
    // Generate AI-powered contextual suggestions
    QString prompt = createContextualPrompt(currentCode, fileType);
    QString model = getOptimalModel("contextual_suggestions", fileType);
    
    QJsonObject contextObj;
    contextObj["code"] = currentCode;
    contextObj["fileType"] = fileType;
    contextObj["userPatterns"] = QJsonArray::fromStringList(contextualPatterns.value(fileType));
    
    callAIService(prompt, model, "contextual_suggestions", contextObj);
    
    return suggestions;
}

QStringList AIAssistantManager::predictDirectoryPaths(const QString &currentPath, const QString &projectType)
{
    QStringList predictions;
    
    // Analyze user's frequent paths
    QMap<QString, int> pathFrequency;
    for (const UserBehaviorData &data : behaviorHistory) {
        if (data.action == "folder_open" || data.action == "file_open") {
            QString dir = QFileInfo(data.context).absolutePath();
            pathFrequency[dir]++;
        }
    }
    
    // Sort by frequency and relevance
    QMapIterator<QString, int> it(pathFrequency);
    QList<QPair<QString, int>> sortedPaths;
    while (it.hasNext()) {
        it.next();
        if (it.key().contains(projectType, Qt::CaseInsensitive) || 
            it.key().startsWith(currentPath)) {
            sortedPaths.append({it.key(), it.value()});
        }
    }
    
    std::sort(sortedPaths.begin(), sortedPaths.end(), 
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                  return a.second > b.second;
              });
    
    // Add top predictions
    for (int i = 0; i < qMin(10, sortedPaths.size()); ++i) {
        predictions.append(sortedPaths[i].first);
    }
    
    // Add common project structure predictions
    QStringList commonDirs = {"src", "include", "lib", "test", "docs", "build", "bin", "assets"};
    for (const QString &dir : commonDirs) {
        QString predicted = currentPath + "/" + dir;
        if (QDir(predicted).exists() && !predictions.contains(predicted)) {
            predictions.append(predicted);
        }
    }
    
    emit predictedPathsReady(predictions);
    return predictions;
}

void AIAssistantManager::startVoiceRecognition()
{
    // Note: QSpeechRecognition might not be available in all Qt versions
    // This is a conceptual implementation
    qDebug() << "🎤 Starting voice recognition...";
    
    // Speech recognition not available in standard Qt
    // if (!speechRecognition) {
    //     speechRecognition = new QSpeechRecognition(this);
    //     connect(speechRecognition, &QSpeechRecognition::resultReady, 
    //             this, &AIAssistantManager::onVoiceCommandReceived);
    // }
    
    // For now, simulate voice commands
    trackUserAction("voice_start", "voice_recognition_activated");
    
    // speechRecognition->start();
    speakText("Voice recognition activated. What would you like me to do?");
}

void AIAssistantManager::stopVoiceRecognition()
{
    qDebug() << "🎤 Stopping voice recognition...";
    
    // Speech recognition not available in standard Qt
    // if (speechRecognition) {
    //     speechRecognition->stop();
    // }
    
    trackUserAction("voice_stop", "voice_recognition_deactivated");
    speakText("Voice recognition deactivated.");
}

void AIAssistantManager::speakText(const QString &text)
{
#ifdef HAVE_TEXTTOSPEECH
    if (textToSpeech && textToSpeech->state() == QTextToSpeech::Ready) {
        QString translatedText = translateText(text, currentLanguage);
        textToSpeech->say(translatedText);
        qDebug() << "🔊 Speaking:" << translatedText;
    }
#else
    qDebug() << "🔊 TTS not available, would speak:" << text;
#endif
}

void AIAssistantManager::trackUserAction(const QString &action, const QString &context)
{
    UserBehaviorData data;
    data.action = action;
    data.context = context;
    data.timestamp = QDateTime::currentDateTime();
    data.projectType = "cpp"; // Can be determined from current project
    data.fileType = QFileInfo(context).suffix();
    data.duration = 0; // Can be calculated for timed actions
    
    behaviorHistory.append(data);
    
    // Update contextual patterns
    if (action == "code_snippet_used") {
        QString fileType = data.fileType;
        if (!contextualPatterns.contains(fileType)) {
            contextualPatterns[fileType] = QStringList();
        }
        if (!contextualPatterns[fileType].contains(context)) {
            contextualPatterns[fileType].append(context);
        }
    }
    
    // Limit history size
    if (behaviorHistory.size() > 10000) {
        behaviorHistory.removeFirst();
    }
    
    // Auto-save behavior data every 100 actions
    if (behaviorHistory.size() % 100 == 0) {
        saveUserBehavior();
    }
}

QList<QString> AIAssistantManager::getPersonalizedInsights()
{
    QList<QString> insights;
    
    analyzeUserPatterns();
    
    // Analyze coding patterns
    QMap<QString, int> actionCounts;
    QMap<QString, int> fileTypeCounts;
    QMap<int, int> hourlyActivity;
    
    for (const UserBehaviorData &data : behaviorHistory) {
        actionCounts[data.action]++;
        fileTypeCounts[data.fileType]++;
        hourlyActivity[data.timestamp.time().hour()]++;
    }
    
    // Generate insights
    if (actionCounts.contains("analyze_code") && actionCounts["analyze_code"] > 50) {
        insights.append("💡 You frequently analyze code. Consider setting up real-time analysis for instant feedback.");
    }
    
    // Find most productive hours
    int maxActivity = 0;
    int peakHour = 0;
    for (auto it = hourlyActivity.begin(); it != hourlyActivity.end(); ++it) {
        if (it.value() > maxActivity) {
            maxActivity = it.value();
            peakHour = it.key();
        }
    }
    
    if (maxActivity > 0) {
        insights.append(QString("⏰ Your peak productivity is around %1:00. Consider scheduling complex tasks during this time.").arg(peakHour));
    }
    
    // File type preferences
    QString mostUsedType = "unknown";
    if (!fileTypeCounts.isEmpty()) {
        int maxCount = 0;
        for (auto it = fileTypeCounts.begin(); it != fileTypeCounts.end(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                mostUsedType = it.key();
            }
        }
    }
    
    if (!mostUsedType.isEmpty() && mostUsedType != "unknown") {
        insights.append(QString("📁 You work primarily with %1 files. I can optimize suggestions for this language.").arg(mostUsedType));
    }
    
    // Efficiency insights
    int debugSessions = actionCounts.value("debug_code", 0);
    int buildFailures = actionCounts.value("build_failed", 0);
    
    if (buildFailures > debugSessions * 2) {
        insights.append("🔧 High build failure rate detected. Consider enabling real-time error checking.");
    }
    
    return insights;
}

void AIAssistantManager::analyzePerformance(const QString &code, const QString &language)
{
    QString prompt = createPerformancePrompt(code, language);
    QString model = getOptimalModel("performance_analysis", language);
    
    QJsonObject context;
    context["code"] = code;
    context["language"] = language;
    context["analysis_type"] = "performance";
    
    callAIService(prompt, model, "performance_analysis", context);
    trackUserAction("performance_analysis", QString("language:%1,codeLength:%2").arg(language).arg(code.length()));
}

QString AIAssistantManager::generateTests(const QString &code, const QString &language)
{
    QString prompt = createTestPrompt(code, language);
    QString model = getOptimalModel("test_generation", language);
    
    QJsonObject context;
    context["code"] = code;
    context["language"] = language;
    context["analysis_type"] = "test_generation";
    
    callAIService(prompt, model, "test_generation", context);
    trackUserAction("test_generation", QString("language:%1,codeLength:%2").arg(language).arg(code.length()));
    
    return QString(); // Will be returned via signal
}

void AIAssistantManager::setLanguage(const QString &languageCode)
{
    currentLanguage = languageCode;
    
    // Load appropriate translation
    QString translationFile = QString(":/translations/app_%1.qm").arg(languageCode);
    if (translator->load(translationFile)) {
        QApplication::installTranslator(translator);
        qDebug() << "🌐 Language changed to" << languageCode;
    }
    
    // Update text-to-speech locale
#ifdef HAVE_TEXTTOSPEECH
    if (textToSpeech) {
        QLocale locale(languageCode);
        textToSpeech->setLocale(locale);
    }
#endif
    
    trackUserAction("language_change", languageCode);
}

QString AIAssistantManager::translateText(const QString &text, const QString &targetLanguage)
{
    if (targetLanguage == "en" || targetLanguage.isEmpty()) {
        return text; // No translation needed
    }
    
    // Simple translation using AI service
    QString prompt = QString("Translate the following text to %1: \"%2\"").arg(targetLanguage, text);
    
    // For now, return original text - real implementation would call translation API
    return text;
}

void AIAssistantManager::integrateWithGitHub(const QString &token)
{
    githubToken = token;
    
    // Test GitHub connection
    QJsonObject testData;
    testData["test"] = "connection";
    callGitHubAPI("user", testData);
    
    qDebug() << "🐙 GitHub integration configured";
    trackUserAction("github_integration", "token_configured");
}

void AIAssistantManager::integrateWithJira(const QString &apiKey, const QString &domain)
{
    jiraApiKey = apiKey;
    jiraDomain = domain;
    
    // Test Jira connection
    QJsonObject testData;
    testData["test"] = "connection";
    callJiraAPI("myself", testData);
    
    qDebug() << "🎫 Jira integration configured for" << domain;
    trackUserAction("jira_integration", QString("domain:%1").arg(domain));
}

void AIAssistantManager::onCodeChanged(const QString &code, const QString &filePath)
{
    if (realtimeEnabled) {
        analyzeCodeRealtime(code, filePath);
    }
}

void AIAssistantManager::onVoiceCommandReceived(const QString &command)
{
    processVoiceCommand(command);
}

void AIAssistantManager::processUserBehavior()
{
    // Process recent user behavior for real-time insights
    if (behaviorHistory.size() < 5) return;
    
    // Analyze recent patterns
    QList<UserBehaviorData> recentData = behaviorHistory.mid(behaviorHistory.size() - 10);
    QMap<QString, int> recentActions;
    
    for (const UserBehaviorData &data : recentData) {
        recentActions[data.action]++;
    }
    
    // Detect patterns and provide insights
    if (recentActions.value("build_failed", 0) >= 3) {
        emit userInsightReady("🔥 Multiple build failures detected. Consider running code analysis first.");
    }
    
    if (recentActions.value("file_open", 0) >= 5) {
        emit userInsightReady("📁 Opening many files. Use Ctrl+P for quick file search or enable file predictions.");
    }
}

void AIAssistantManager::onRealtimeAnalysisReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QByteArray data = reply->readAll();
    
    // Parse Ollama response format
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject ollamaResponse = doc.object();
    
    QString aiText;
    if (ollamaResponse.contains("response")) {
        aiText = ollamaResponse["response"].toString();
    } else {
        aiText = QString::fromUtf8(data); // Fallback to raw text
    }
    
    // Try to parse as JSON first (if AI followed our JSON request)
    QJsonDocument jsonDoc = QJsonDocument::fromJson(aiText.toUtf8());
    if (!jsonDoc.isNull() && jsonDoc.isObject()) {
        QJsonObject response = jsonDoc.object();
        if (response.contains("suggestions")) {
            QJsonArray suggestions = response["suggestions"].toArray();
            for (const QJsonValue &suggestionValue : suggestions) {
                QJsonObject suggestionObj = suggestionValue.toObject();
                
                CodeSuggestion suggestion;
                suggestion.type = suggestionObj["type"].toString();
                suggestion.description = suggestionObj["description"].toString();
                suggestion.fixedCode = suggestionObj["fixedCode"].toString();
                suggestion.lineNumber = suggestionObj["lineNumber"].toInt();
                suggestion.confidence = suggestionObj["confidence"].toDouble();
                
                emit realtimeSuggestionReady(suggestion);
            }
            reply->deleteLater();
            return;
        }
    }
    
    // If not JSON, parse as plain text response
    if (!aiText.isEmpty()) {
        qDebug() << "🔍 AI Response received:" << aiText.left(100) << "...";
        
        CodeSuggestion suggestion;
        suggestion.type = "analysis";
        suggestion.description = aiText;
        suggestion.fixedCode = "";
        suggestion.lineNumber = 0;
        suggestion.confidence = 0.8;
        suggestion.filePath = reply->property("context").toJsonObject()["filePath"].toString();
        
        qDebug() << "📤 Emitting realtimeSuggestionReady signal";
        emit realtimeSuggestionReady(suggestion);
        qDebug() << "✅ Signal emitted successfully";
    } else {
        qDebug() << "❌ No AI text received in response";
    }
    
    reply->deleteLater();
}

void AIAssistantManager::onPerformanceAnalysisReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QByteArray data = reply->readAll();
    QString insight = QString::fromUtf8(data);
    
    emit performanceInsightReady(insight);
    reply->deleteLater();
}

void AIAssistantManager::onTestGenerationReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    QByteArray data = reply->readAll();
    QString testCode = QString::fromUtf8(data);
    
    emit testCodeGenerated(testCode);
    reply->deleteLater();
}

void AIAssistantManager::onVoiceRecognitionFinished()
{
    // Handle voice recognition completion
    qDebug() << "🎤 Voice recognition finished";
}

QString AIAssistantManager::getOptimalModel(const QString &task, const QString &language, 
                                           const QString &priority)
{
    // Define available models with their capabilities
    QMap<QString, QMap<QString, float>> modelScores;
    
    // Model performance for different tasks (0.0 to 1.0)
    modelScores["ollama/magicoder:7b"]["cpp"] = 0.9;
    modelScores["ollama/magicoder:7b"]["speed"] = 0.8;
    modelScores["ollama/magicoder:7b"]["accuracy"] = 0.85;
    
    modelScores["ollama/deepseek-coder:6.7b"]["python"] = 0.95;
    modelScores["ollama/deepseek-coder:6.7b"]["speed"] = 0.7;
    modelScores["ollama/deepseek-coder:6.7b"]["accuracy"] = 0.9;
    
    modelScores["ollama/codegemma:7b"]["javascript"] = 0.85;
    modelScores["ollama/codegemma:7b"]["speed"] = 0.9;
    modelScores["ollama/codegemma:7b"]["accuracy"] = 0.8;
    
    modelScores["ollama/qwen2.5-coder:7b"]["optimization"] = 0.95;
    modelScores["ollama/qwen2.5-coder:7b"]["speed"] = 0.6;
    modelScores["ollama/qwen2.5-coder:7b"]["accuracy"] = 0.95;
    
    modelScores["ollama/codellama:7b"]["general"] = 0.8;
    modelScores["ollama/codellama:7b"]["speed"] = 0.85;
    modelScores["ollama/codellama:7b"]["accuracy"] = 0.8;
    
    // Get best model based on task, language, and priority
    QString bestModel = "ollama/codellama:7b"; // Default
    float bestScore = 0.0;
    
    for (auto modelIt = modelScores.begin(); modelIt != modelScores.end(); ++modelIt) {
        QString model = modelIt.key();
        QMap<QString, float> scores = modelIt.value();
        
        float score = getModelScore(model, task, language);
        
        // Apply priority weighting
        if (priority == "speed") {
            score = score * 0.7 + scores.value("speed", 0.5) * 0.3;
        } else if (priority == "accuracy") {
            score = score * 0.7 + scores.value("accuracy", 0.5) * 0.3;
        } else if (priority == "balanced") {
            score = score * 0.6 + scores.value("speed", 0.5) * 0.2 + scores.value("accuracy", 0.5) * 0.2;
        }
        
        // Factor in recent performance and availability
        if (modelResponseTimes.contains(model) && !modelResponseTimes[model].isEmpty()) {
            float avgResponseTime = 0.0;
            for (float time : modelResponseTimes[model]) {
                avgResponseTime += time;
            }
            avgResponseTime /= modelResponseTimes[model].size();
            
            // Penalize slow models for speed priority
            if (priority == "speed" && avgResponseTime > 5.0) {
                score *= 0.8;
            }
        }
        
        // Check model load and availability
        float currentLoad = modelCurrentLoad.value(model, 0.0);
        if (currentLoad > 0.8) {
            score *= 0.7; // Penalize heavily loaded models
        }
        
        // Factor in recent usage to distribute load
        QDateTime lastUsed = modelLastUsed.value(model, QDateTime());
        if (lastUsed.isValid()) {
            qint64 minutesSinceLastUse = lastUsed.secsTo(QDateTime::currentDateTime()) / 60;
            if (minutesSinceLastUse < 5) {
                score *= 0.9; // Slight penalty for recently used models
            }
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestModel = model;
        }
    }
    
    // Update usage tracking
    modelLastUsed[bestModel] = QDateTime::currentDateTime();
    
    qDebug() << "🧠 Selected model:" << bestModel << "for task:" << task << "score:" << bestScore;
    return bestModel;
}

void AIAssistantManager::callAIService(const QString &prompt, const QString &model, 
                                       const QString &task, const QJsonObject &context)
{
    QNetworkRequest request(QUrl("http://localhost:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject payload;
    QString modelName = model.split("/").last(); // Remove "ollama/" prefix
    payload["model"] = modelName; // Keep the full model name with tag (e.g., "magicoder:7b")
    payload["prompt"] = prompt;
    payload["stream"] = false;
    
    QJsonObject options;
    options["temperature"] = 0.1;
    options["top_p"] = 0.9;
    payload["options"] = options;
    
    // Track request start time for performance measurement
    QDateTime startTime = QDateTime::currentDateTime();
    
    QNetworkReply *reply = networkManager->post(request, QJsonDocument(payload).toJson());
    reply->setProperty("task", task);
    reply->setProperty("context", context);
    reply->setProperty("model", model);
    reply->setProperty("startTime", startTime);
    
    // Single connection that handles both performance tracking and response processing
    if (task == "realtime_analysis" || task == "incremental_analysis") {
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            QDateTime endTime = QDateTime::currentDateTime();
            QDateTime startTime = reply->property("startTime").toDateTime();
            QString model = reply->property("model").toString();
            QString task = reply->property("task").toString();
            
            float responseTime = startTime.msecsTo(endTime);
            bool success = (reply->error() == QNetworkReply::NoError);
            
            // Update model performance tracking
            updateModelPerformance(model, task, responseTime, success);
            
            // Call specific handler
            onRealtimeAnalysisReply();
        });
    } else if (task == "performance_analysis") {
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            QDateTime endTime = QDateTime::currentDateTime();
            QDateTime startTime = reply->property("startTime").toDateTime();
            QString model = reply->property("model").toString();
            QString task = reply->property("task").toString();
            
            float responseTime = startTime.msecsTo(endTime);
            bool success = (reply->error() == QNetworkReply::NoError);
            
            updateModelPerformance(model, task, responseTime, success);
            onPerformanceAnalysisReply();
        });
    } else if (task == "test_generation") {
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            QDateTime endTime = QDateTime::currentDateTime();
            QDateTime startTime = reply->property("startTime").toDateTime();
            QString model = reply->property("model").toString();
            QString task = reply->property("task").toString();
            
            float responseTime = startTime.msecsTo(endTime);
            bool success = (reply->error() == QNetworkReply::NoError);
            
            updateModelPerformance(model, task, responseTime, success);
            onTestGenerationReply();
        });
    }
    
    qDebug() << "🚀 AI request sent:" << model << "task:" << task << "prompt length:" << prompt.length();
}

QString AIAssistantManager::createRealtimePrompt(const QString &code, const QString &filePath)
{
    return QString(
        "REALTIME CODE ANALYSIS:\n"
        "File: %1\n"
        "Analyze this code for immediate issues and provide quick suggestions.\n"
        "Focus on: syntax errors, potential bugs, optimization opportunities.\n"
        "Respond with JSON format: {\"suggestions\": [{\"type\": \"error|warning|suggestion\", "
        "\"description\": \"...\", \"lineNumber\": 0, \"fixedCode\": \"...\", \"confidence\": 0.95}]}\n\n"
        "CODE:\n%2"
    ).arg(filePath, code);
}

QString AIAssistantManager::createContextualPrompt(const QString &code, const QString &fileType)
{
    return QString(
        "CONTEXTUAL SUGGESTIONS for %1:\n"
        "Based on the current code context, suggest improvements, common patterns, "
        "and best practices specific to %1 development.\n"
        "Consider: design patterns, performance, readability, maintainability.\n\n"
        "CODE:\n%2"
    ).arg(fileType, code);
}

QString AIAssistantManager::createPerformancePrompt(const QString &code, const QString &language)
{
    return QString(
        "PERFORMANCE ANALYSIS for %1:\n"
        "Analyze this code for performance bottlenecks and optimization opportunities.\n"
        "Focus on: algorithmic complexity, memory usage, I/O operations, parallel processing.\n"
        "Provide specific optimization recommendations with code examples.\n\n"
        "CODE:\n%2"
    ).arg(language, code);
}

QString AIAssistantManager::createTestPrompt(const QString &code, const QString &language)
{
    return QString(
        "GENERATE COMPREHENSIVE TESTS for %1:\n"
        "Create unit tests that cover:\n"
        "- Normal operation cases\n"
        "- Edge cases and boundary conditions\n"
        "- Error conditions and exception handling\n"
        "- Performance edge cases\n"
        "Use appropriate testing framework for %1.\n\n"
        "CODE TO TEST:\n%2"
    ).arg(language, code);
}

void AIAssistantManager::analyzeUserPatterns()
{
    // Advanced pattern analysis
    QMap<QString, QStringList> patterns;
    
    for (const UserBehaviorData &data : behaviorHistory) {
        if (data.action == "code_edit") {
            QString fileType = data.fileType;
            if (!patterns.contains(fileType)) {
                patterns[fileType] = QStringList();
            }
            
            // Extract common code patterns (simplified)
            QStringList commonPatterns = {"class", "function", "if", "for", "while", "try", "catch"};
            for (const QString &pattern : commonPatterns) {
                if (data.context.contains(pattern)) {
                    patterns[fileType].append(pattern);
                }
            }
        }
    }
    
    contextualPatterns = patterns;
}

void AIAssistantManager::updatePredictivePaths(const QString &path)
{
    if (!frequentPaths.contains(path)) {
        frequentPaths.append(path);
    }
    
    // Keep only recent paths (limit to 100)
    if (frequentPaths.size() > 100) {
        frequentPaths.removeFirst();
    }
}

void AIAssistantManager::saveUserBehavior()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    
    QString filePath = dataDir + "/user_behavior.json";
    QFile file(filePath);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray behaviorArray;
        for (const UserBehaviorData &data : behaviorHistory) {
            QJsonObject obj;
            obj["action"] = data.action;
            obj["context"] = data.context;
            obj["timestamp"] = data.timestamp.toString(Qt::ISODate);
            obj["projectType"] = data.projectType;
            obj["fileType"] = data.fileType;
            obj["duration"] = data.duration;
            behaviorArray.append(obj);
        }
        
        QJsonObject root;
        root["behavior"] = behaviorArray;
        root["frequentPaths"] = QJsonArray::fromStringList(frequentPaths);
        root["language"] = currentLanguage;
        
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void AIAssistantManager::loadUserBehavior()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataDir + "/user_behavior.json";
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        
        QJsonArray behaviorArray = root["behavior"].toArray();
        behaviorHistory.clear();
        
        for (const QJsonValue &value : behaviorArray) {
            QJsonObject obj = value.toObject();
            UserBehaviorData data;
            data.action = obj["action"].toString();
            data.context = obj["context"].toString();
            data.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
            data.projectType = obj["projectType"].toString();
            data.fileType = obj["fileType"].toString();
            data.duration = obj["duration"].toInt();
            behaviorHistory.append(data);
        }
        
        QJsonArray pathsArray = root["frequentPaths"].toArray();
        frequentPaths.clear();
        for (const QJsonValue &value : pathsArray) {
            frequentPaths.append(value.toString());
        }
        
        currentLanguage = root["language"].toString("en");
        file.close();
    }
}

void AIAssistantManager::processVoiceCommand(const QString &command)
{
    qDebug() << "🎤 Processing voice command:" << command;
    
    QString result;
    QString lowerCommand = command.toLower();
    
    if (lowerCommand.contains("analyze") || lowerCommand.contains("check")) {
        result = "Starting code analysis...";
        emit voiceCommandProcessed(command, result);
        // Trigger analysis
    } else if (lowerCommand.contains("build") || lowerCommand.contains("compile")) {
        result = "Starting build process...";
        emit voiceCommandProcessed(command, result);
        // Trigger build
    } else if (lowerCommand.contains("test")) {
        result = "Running tests...";
        emit voiceCommandProcessed(command, result);
        // Trigger tests
    } else if (lowerCommand.contains("open file") || lowerCommand.contains("show file")) {
        result = "Opening file dialog...";
        emit voiceCommandProcessed(command, result);
        // Trigger file open
    } else if (lowerCommand.contains("help") || lowerCommand.contains("what can you do")) {
        result = "I can help with code analysis, building, testing, and file management. "
                "Try saying: 'analyze code', 'build project', 'run tests', or 'open file'.";
        emit voiceCommandProcessed(command, result);
    } else {
        result = "I didn't understand that command. Try 'help' for available commands.";
        emit voiceCommandProcessed(command, result);
    }
    
    speakText(result);
    trackUserAction("voice_command", command);
}

void AIAssistantManager::callGitHubAPI(const QString &endpoint, const QJsonObject &data)
{
    Q_UNUSED(data);
    if (githubToken.isEmpty()) return;
    
    QNetworkRequest request(QUrl(QString("https://api.github.com/%1").arg(endpoint)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("token %1").arg(githubToken).toUtf8());
    request.setRawHeader("User-Agent", "OpenInterpreterGUI/1.0");
    
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit externalApiResponse("github", doc.object());
        reply->deleteLater();
    });
}

void AIAssistantManager::callJiraAPI(const QString &endpoint, const QJsonObject &data)
{
    Q_UNUSED(data);
    if (jiraApiKey.isEmpty() || jiraDomain.isEmpty()) return;
    
    QNetworkRequest request(QUrl(QString("https://%1.atlassian.net/rest/api/3/%2").arg(jiraDomain, endpoint)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QString credentials = QString("%1:%2").arg("email", jiraApiKey); // Simplified
    request.setRawHeader("Authorization", QString("Basic %1").arg(credentials.toUtf8().toBase64()).toUtf8());
    
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit externalApiResponse("jira", doc.object());
        reply->deleteLater();
    });
}

// Incremental Analysis Helper Methods
QString AIAssistantManager::calculateCodeHash(const QString &code)
{
    return QString::number(qHash(code.simplified()), 16);
}

QString AIAssistantManager::generateCodeDiff(const QString &oldCode, const QString &newCode)
{
    QStringList oldLines = oldCode.split('\n');
    QStringList newLines = newCode.split('\n');
    
    QStringList diff;
    int maxLines = qMax(oldLines.size(), newLines.size());
    
    for (int i = 0; i < maxLines; ++i) {
        QString oldLine = (i < oldLines.size()) ? oldLines[i] : "";
        QString newLine = (i < newLines.size()) ? newLines[i] : "";
        
        if (oldLine != newLine) {
            if (!oldLine.isEmpty()) {
                diff.append(QString("- %1").arg(oldLine));
            }
            if (!newLine.isEmpty()) {
                diff.append(QString("+ %1").arg(newLine));
            }
        }
    }
    
    return diff.join('\n');
}

bool AIAssistantManager::isSignificantChange(const QString &diff, const QString &fileType)
{
    // Define what constitutes a significant change for incremental analysis
    QStringList lines = diff.split('\n');
    int changedLines = lines.size();
    
    // Check for significant keywords that indicate structural changes
    QStringList significantKeywords;
    if (fileType == "cpp" || fileType == "h" || fileType == "hpp") {
        significantKeywords = {"class", "struct", "function", "#include", "namespace", "template"};
    } else if (fileType == "py") {
        significantKeywords = {"def", "class", "import", "from", "if __name__"};
    } else if (fileType == "js" || fileType == "ts") {
        significantKeywords = {"function", "class", "const", "let", "var", "import", "export"};
    }
    
    // Check if any significant keywords are present in the diff
    for (const QString &keyword : significantKeywords) {
        if (diff.contains(keyword, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    // Consider it significant if more than 3 lines changed
    return changedLines > 3;
}

float AIAssistantManager::calculateComplexity(const QString &code)
{
    // Simple complexity calculation based on control flow statements
    float complexity = 1.0; // Base complexity
    
    QStringList complexityKeywords = {"if", "else", "for", "while", "switch", "case", "catch", "&&", "||"};
    
    for (const QString &keyword : complexityKeywords) {
        complexity += code.count(keyword, Qt::CaseInsensitive) * 0.5;
    }
    
    // Factor in nested braces
    int braceDepth = 0;
    int maxDepth = 0;
    for (const QChar &ch : code) {
        if (ch == '{') {
            braceDepth++;
            maxDepth = qMax(maxDepth, braceDepth);
        } else if (ch == '}') {
            braceDepth--;
        }
    }
    
    complexity += maxDepth * 0.3;
    return complexity;
}

void AIAssistantManager::processIncrementalAnalysis()
{
    if (pendingAnalysisFile.isEmpty() || pendingAnalysisDiff.isEmpty()) return;
    
    QString prompt = QString(
        "INCREMENTAL CODE ANALYSIS:\n"
        "File: %1\n"
        "Analyze only the following code changes for quick feedback.\n"
        "Focus on: syntax issues in changed lines, immediate logical errors, type mismatches.\n"
        "Provide fast, targeted suggestions only for the modified parts.\n"
        "Respond with JSON: {\"suggestions\": [{\"type\": \"error|warning|info\", "
        "\"description\": \"...\", \"lineNumber\": 0, \"confidence\": 0.95}]}\n\n"
        "CHANGES:\n%2"
    ).arg(pendingAnalysisFile, pendingAnalysisDiff);
    
    QString fileType = QFileInfo(pendingAnalysisFile).suffix();
    QString model = getOptimalModel("incremental_analysis", fileType, "speed");
    
    QJsonObject context;
    context["filePath"] = pendingAnalysisFile;
    context["diff"] = pendingAnalysisDiff;
    context["fileType"] = fileType;
    context["analysis_type"] = "incremental";
    context["is_incremental"] = true;
    
    callAIService(prompt, model, "realtime_analysis", context);
    
    // Clear pending analysis
    pendingAnalysisFile.clear();
    pendingAnalysisDiff.clear();
}

// Model Performance Tracking Methods
void AIAssistantManager::updateModelPerformance(const QString &model, const QString &task, 
                                               float responseTime, bool success)
{
    // Update response times
    if (!modelResponseTimes.contains(model)) {
        modelResponseTimes[model] = QList<float>();
    }
    modelResponseTimes[model].append(responseTime);
    
    // Keep only recent measurements (last 50)
    if (modelResponseTimes[model].size() > 50) {
        modelResponseTimes[model].removeFirst();
    }
    
    // Update success rates
    if (!modelSuccessRates.contains(model)) {
        modelSuccessRates[model] = QList<bool>();
    }
    modelSuccessRates[model].append(success);
    
    // Keep only recent measurements (last 50)
    if (modelSuccessRates[model].size() > 50) {
        modelSuccessRates[model].removeFirst();
    }
    
    // Update performance history for persistent storage
    bool found = false;
    for (ModelPerformance &perf : modelPerformanceHistory) {
        if (perf.modelName == model && perf.taskType == task) {
            // Update existing entry
            perf.avgResponseTime = (perf.avgResponseTime * perf.usageCount + responseTime) / (perf.usageCount + 1);
            perf.successRate = (perf.successRate * perf.usageCount + (success ? 1.0 : 0.0)) / (perf.usageCount + 1);
            perf.usageCount++;
            perf.lastUsed = QDateTime::currentDateTime();
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Create new performance entry
        ModelPerformance perf;
        perf.modelName = model;
        perf.taskType = task;
        perf.avgResponseTime = responseTime;
        perf.successRate = success ? 1.0 : 0.0;
        perf.usageCount = 1;
        perf.lastUsed = QDateTime::currentDateTime();
        modelPerformanceHistory.append(perf);
    }
    
    // Periodically save performance data
    if (modelPerformanceHistory.size() % 10 == 0) {
        saveModelPerformanceData();
    }
    
    qDebug() << "📊 Updated performance for" << model << "task:" << task 
             << "time:" << responseTime << "ms success:" << success;
}

float AIAssistantManager::getModelScore(const QString &model, const QString &task, const QString &language)
{
    float score = 0.5; // Base score
    
    // Find historical performance for this model and task
    for (const ModelPerformance &perf : modelPerformanceHistory) {
        if (perf.modelName == model && (perf.taskType == task || perf.taskType == "general")) {
            // Weight based on usage count (more data = more reliable)
            float reliability = qMin(1.0f, perf.usageCount / 10.0f);
            
            // Calculate performance score (0.0 to 1.0)
            float performanceScore = 0.0;
            performanceScore += perf.successRate * 0.6; // Success rate is most important
            performanceScore += (1.0 - qMin(1.0f, perf.avgResponseTime / 10.0f)) * 0.4; // Speed factor
            
            score = (score * (1.0 - reliability)) + (performanceScore * reliability);
            break;
        }
    }
    
    // Language-specific adjustments
    if (language == "cpp" || language == "h" || language == "hpp") {
        if (model.contains("magicoder") || model.contains("codellama")) {
            score += 0.1; // Boost for C++ specialized models
        }
    } else if (language == "py" || language == "python") {
        if (model.contains("deepseek") || model.contains("codellama")) {
            score += 0.1; // Boost for Python specialized models
        }
    } else if (language == "js" || language == "ts" || language == "javascript") {
        if (model.contains("codegemma") || model.contains("codellama")) {
            score += 0.1; // Boost for JavaScript specialized models
        }
    }
    
    // Task-specific adjustments
    if (task == "incremental_analysis" || task == "realtime_analysis") {
        if (model.contains("codegemma") || model.contains("magicoder")) {
            score += 0.05; // Boost for fast analysis models
        }
    } else if (task == "performance_analysis" || task == "optimization") {
        if (model.contains("qwen") || model.contains("deepseek")) {
            score += 0.1; // Boost for optimization-focused models
        }
    }
    
    return qMin(1.0f, qMax(0.0f, score)); // Clamp to valid range
}

void AIAssistantManager::loadModelPerformanceData()
{
    modelPerformanceHistory.clear();
    
    performanceSettings->beginGroup("ModelPerformance");
    int size = performanceSettings->beginReadArray("models");
    
    for (int i = 0; i < size; ++i) {
        performanceSettings->setArrayIndex(i);
        
        ModelPerformance perf;
        perf.modelName = performanceSettings->value("modelName").toString();
        perf.taskType = performanceSettings->value("taskType").toString();
        perf.language = performanceSettings->value("language").toString();
        perf.avgResponseTime = performanceSettings->value("avgResponseTime").toFloat();
        perf.successRate = performanceSettings->value("successRate").toFloat();
        perf.usageCount = performanceSettings->value("usageCount").toInt();
        perf.lastUsed = performanceSettings->value("lastUsed").toDateTime();
        
        if (!perf.modelName.isEmpty()) {
            modelPerformanceHistory.append(perf);
        }
    }
    
    performanceSettings->endArray();
    performanceSettings->endGroup();
    
    qDebug() << "📊 Loaded" << modelPerformanceHistory.size() << "model performance records";
}

void AIAssistantManager::saveModelPerformanceData()
{
    performanceSettings->beginGroup("ModelPerformance");
    performanceSettings->beginWriteArray("models");
    
    for (int i = 0; i < modelPerformanceHistory.size(); ++i) {
        const ModelPerformance &perf = modelPerformanceHistory[i];
        performanceSettings->setArrayIndex(i);
        
        performanceSettings->setValue("modelName", perf.modelName);
        performanceSettings->setValue("taskType", perf.taskType);
        performanceSettings->setValue("language", perf.language);
        performanceSettings->setValue("avgResponseTime", perf.avgResponseTime);
        performanceSettings->setValue("successRate", perf.successRate);
        performanceSettings->setValue("usageCount", perf.usageCount);
        performanceSettings->setValue("lastUsed", perf.lastUsed);
    }
    
    performanceSettings->endArray();
    performanceSettings->endGroup();
    performanceSettings->sync();
    
    qDebug() << "💾 Saved" << modelPerformanceHistory.size() << "model performance records";
}
