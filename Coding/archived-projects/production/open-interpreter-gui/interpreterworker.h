#ifndef INTERPRETERWORKER_H
#define INTERPRETERWORKER_H

#include <QObject>
#include <QThread>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

class InterpreterWorker : public QObject {
    Q_OBJECT

public:
    explicit InterpreterWorker(const QString &message, 
                              const QString &model, 
                              bool containerMode = true,
                              QObject *parent = nullptr);

public slots:
    void startProcessing();
    void stopProcessing();
    
public:
    bool isRunning() const;

signals:
    void outputReceived(const QString &output, const QString &type);
    void processingFinished();
    void errorOccurred(const QString &error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onProcessOutput();
    void onNetworkReplyFinished();
    void onTimeout();

private:
    QString message;
    QString model;
    bool containerMode;
    QProcess *process;
    QTimer *timeoutTimer;
    QNetworkAccessManager *networkManager;
    
    void setupProcess();
    void fallbackToDirectAPI();
    void startOpenInterpreterContainer();
    QString createPythonScript() const;
};

#endif // INTERPRETERWORKER_H
