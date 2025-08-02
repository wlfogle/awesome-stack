#include "interpreterworker.h"
#include <QStandardPaths>
#include <QCoreApplication>

InterpreterWorker::InterpreterWorker(const QString &message, 
                                    const QString &model, 
                                    bool containerMode,
                                    QObject *parent)
    : QObject(parent)
    , message(message)
    , model(model)
    , containerMode(containerMode)
    , process(nullptr)
    , timeoutTimer(new QTimer(this))
    , networkManager(new QNetworkAccessManager(this))
{
    // Set up timeout timer
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(300000); // 5 minute timeout
    connect(timeoutTimer, &QTimer::timeout, this, &InterpreterWorker::onTimeout);
}

void InterpreterWorker::startProcessing()
{
    emit outputReceived("🚀 Starting Open Interpreter...", "system");
    qDebug() << "[DEBUG] Starting analysis for model:" << model;
    qDebug() << "[DEBUG] Message length:" << message.length();
    qDebug() << "[DEBUG] Container mode:" << containerMode;
    
    // Log first 200 chars of message for debugging
    QString messagePreview = message.left(200) + (message.length() > 200 ? "..." : "");
    qDebug() << "[DEBUG] Message preview:" << messagePreview;
    
    setupProcess();
    timeoutTimer->start();
}

void InterpreterWorker::stopProcessing()
{
    timeoutTimer->stop();
    if (process && process->state() != QProcess::NotRunning) {
        process->terminate();
        if (!process->waitForFinished(3000)) {
            process->kill();
        }
    }
}

bool InterpreterWorker::isRunning() const
{
    return process && (process->state() == QProcess::Running);
}

void InterpreterWorker::setupProcess()
{
    qDebug() << "[DEBUG] Setting up process for containerized open-interpreter";
    
    // First check if we can reach Ollama API directly
    QNetworkRequest testRequest(QUrl("http://localhost:11434/api/tags"));
    QNetworkReply *testReply = networkManager->get(testRequest);
    
    // Set a short timeout for the test
    QTimer *testTimer = new QTimer(this);
    testTimer->setSingleShot(true);
    testTimer->setInterval(5000); // 5 second timeout
    
    connect(testTimer, &QTimer::timeout, [=]() {
        testReply->abort();
        emit outputReceived("⚠️ Ollama not responding on localhost:11434, trying to start open-interpreter container...", "system");
        startOpenInterpreterContainer();
    });
    
    connect(testReply, &QNetworkReply::finished, [=]() {
        testTimer->stop();
        if (testReply->error() == QNetworkReply::NoError) {
            emit outputReceived("✅ Connected to Ollama API", "system");
            fallbackToDirectAPI();
        } else {
            emit outputReceived("🚀 Starting open-interpreter container...", "system");
            startOpenInterpreterContainer();
        }
        testReply->deleteLater();
    });
    
    testTimer->start();
}

void InterpreterWorker::fallbackToDirectAPI()
{
    emit outputReceived("Falling back to direct Ollama API...", "system");
    
    QString enhancedPrompt = QString(
        "CRITICAL CODE REVIEW: %1\n\n"
        "You are a SENIOR CODE REVIEWER. I am paying you to find ACTUAL PROBLEMS in this code.\n\n"
        "REQUIREMENTS:\n"
        "1. FIND REAL BUGS - null pointers, memory leaks, race conditions, buffer overflows\n"
        "2. PROVIDE EXACT LINE NUMBERS for every issue you find\n"
        "3. NO GENERIC ADVICE - only specific problems with specific solutions\n"
        "4. If you say 'no bugs found' you FAILED the review\n"
        "5. Look for: missing error handling, resource leaks, logic errors, security issues\n"
        "6. Provide FIXED CODE examples for every issue\n\n"
        "ANALYZE THIS CODE AGGRESSIVELY:\n\n"
    ).arg(message);
    
    QJsonObject payload;
    payload["model"] = model.split("/").last();
    payload["prompt"] = enhancedPrompt;
    payload["stream"] = true; // Enable streaming for real-time feedback
    
    QJsonObject options;
    options["temperature"] = 0.1;
    options["top_p"] = 0.9;
    payload["options"] = options;
    
    QNetworkRequest request(QUrl("http://localhost:11434/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonDocument doc(payload);
    QNetworkReply *reply = networkManager->post(request, doc.toJson());
    connect(reply, &QNetworkReply::finished, this, &InterpreterWorker::onNetworkReplyFinished);
}

void InterpreterWorker::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    timeoutTimer->stop();
    if (exitStatus == QProcess::CrashExit) {
        emit errorOccurred("Process crashed");
        fallbackToDirectAPI();
    } else {
        emit processingFinished();
    }
}

void InterpreterWorker::onProcessError(QProcess::ProcessError error)
{
    timeoutTimer->stop();
    QString errorString;
    switch (error) {
    case QProcess::FailedToStart:
        errorString = "Failed to start process";
        break;
    case QProcess::Crashed:
        errorString = "Process crashed";
        break;
    case QProcess::Timedout:
        errorString = "Process timed out";
        break;
    default:
        errorString = "Unknown error";
        break;
    }
    emit errorOccurred(errorString);
    fallbackToDirectAPI();
}

void InterpreterWorker::onProcessOutput()
{
    if (process) {
        QByteArray data = process->readAllStandardOutput();
        QString output = QString::fromUtf8(data);
        if (!output.trimmed().isEmpty()) {
            emit outputReceived(output.trimmed(), "assistant");
        }
    }
}

void InterpreterWorker::onNetworkReplyFinished()
{
    timeoutTimer->stop();
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        QByteArray responseData = reply->readAll();
        qDebug() << "[DEBUG] HTTP Status Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "[DEBUG] Full response data:" << responseData;
        
        if (reply->error() == QNetworkReply::NoError) {
            // Handle streaming response - each line is a JSON object
            QString responseText = QString::fromUtf8(responseData);
            QStringList lines = responseText.split('\n', Qt::SkipEmptyParts);
            QString fullResponse;
            
            for (const QString &line : lines) {
                if (line.trimmed().isEmpty()) continue;
                
                QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
                if (!doc.isNull()) {
                    QJsonObject obj = doc.object();
                    QString partialResponse = obj["response"].toString();
                    if (!partialResponse.isEmpty()) {
                        fullResponse += partialResponse;
                    }
                }
            }
            
            if (!fullResponse.isEmpty()) {
                emit outputReceived(fullResponse, "assistant");
            } else {
                // Fallback - try non-streaming format
                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                QJsonObject obj = doc.object();
                QString response = obj["response"].toString();
                if (!response.isEmpty()) {
                    emit outputReceived(response, "assistant");
                } else {
                    emit outputReceived("Model returned empty response. Try a different prompt or model.", "error");
                }
            }
        } else {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 500) {
                emit errorOccurred("Ollama server error (500): Model may be overloaded or out of memory. Try a smaller prompt or different model.");
            } else {
                emit errorOccurred(QString("Network error (%1): %2").arg(statusCode).arg(reply->errorString()));
            }
        }
        reply->deleteLater();
        emit processingFinished();
    }
}

void InterpreterWorker::onTimeout()
{
    qDebug() << "Timeout reached: Analysis taking too long.";
    stopProcessing();
    emit errorOccurred("Request timed out after 5 minutes");
    emit processingFinished();
}

void InterpreterWorker::startOpenInterpreterContainer()
{
    emit outputReceived("🚀 Starting open-interpreter container...", "system");
    
    // Try to start the open-interpreter environment
    process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &InterpreterWorker::onProcessFinished);
    connect(process, &QProcess::errorOccurred, this, &InterpreterWorker::onProcessError);
    connect(process, &QProcess::readyReadStandardOutput, this, &InterpreterWorker::onProcessOutput);
    
    // Start the open-interpreter with Ollama in the container/environment
    QString command = "enter";
    QStringList arguments;
    arguments << "open-interpreter" << "--" << "bash" << "-c";
    
    QString bashScript = QString(
        "export PATH=\"$HOME/.local/bin:$PATH\"; "
        "if ! pgrep -x ollama > /dev/null; then "
        "echo \"🚀 Starting Ollama...\"; "
        "nohup ollama serve > /tmp/ollama.log 2>&1 & "
        "sleep 3; "
        "fi; "
        "echo \"🤖 Starting Open Interpreter...\"; "
        "interpreter --model %1 --local"
    ).arg(model.contains("/") ? model : "ollama/" + model);
    
    arguments << bashScript;
    
    emit outputReceived(QString("Starting: %1 %2").arg(command, arguments.join(" ")), "system");
    process->start(command, arguments);
    
    // If that fails, fall back to direct API
    QTimer::singleShot(10000, this, [this]() {
        if (process && process->state() == QProcess::Running) {
            // Send our message to the running interpreter
            QString input = message + "\n";
            process->write(input.toUtf8());
        } else {
            emit outputReceived("⚠️ Container startup failed, falling back to direct API...", "system");
            fallbackToDirectAPI();
        }
    });
}

QString InterpreterWorker::createPythonScript() const
{
    return QString(R"(
import sys
import os
import tempfile
sys.path.insert(0, "/home/lou/.local/lib/python3.10/site-packages")

try:
    from interpreter import interpreter
    import requests
    import json
    
    # Check if we can reach ollama first
    try:
        response = requests.get("http://localhost:11434/api/tags", timeout=5)
        if response.status_code != 200:
            print("❌ Cannot connect to Ollama - make sure it's running")
            exit(1)
        print("✅ Connected to Ollama")
    except:
        print("❌ Cannot connect to Ollama - make sure it's running")
        exit(1)
    
    # Configure interpreter properly for local mode
    interpreter.offline = True
    interpreter.auto_run = True
    interpreter.verbose = True
    
    # Set up the LLM configuration for Ollama
    model_name = "%1".replace("ollama/", "")
    interpreter.llm.model = "ollama/" + model_name
    interpreter.llm.api_base = "http://localhost:11434"
    interpreter.llm.api_key = "fake_key"
    
    # Force local mode
    interpreter.local = True
    
    print(f"🚀 Starting Open Interpreter with {model_name}...")
    print(f"📝 Processing: %2")
    print("=" * 50)
    
    # Send the message to interpreter
    try:
        for chunk in interpreter.chat("%2", stream=True):
            if hasattr(chunk, 'content') and chunk.content:
                print(chunk.content, end='', flush=True)
            elif hasattr(chunk, 'language') and hasattr(chunk, 'code'):
                print(f"\n\n```{chunk.language}")
                print(chunk.code)
                print("```\n")
            elif hasattr(chunk, 'output') and chunk.output:
                print(f"Output: {chunk.output}")
            elif isinstance(chunk, dict):
                if 'content' in chunk and chunk['content']:
                    print(chunk['content'], end='', flush=True)
                elif 'language' in chunk and 'code' in chunk:
                    print(f"\n\n```{chunk['language']}")
                    print(chunk['code'])
                    print("```\n")
                elif 'output' in chunk and chunk['output']:
                    print(f"Output: {chunk['output']}")
            elif isinstance(chunk, str):
                print(chunk, end='', flush=True)
    except Exception as chat_error:
        print(f"Error during chat: {chat_error}")
        
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
)").arg(model, message);
}
