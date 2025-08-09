#ifndef AIOPTIMIZER_H
#define AIOPTIMIZER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QTimer>
#include <QHash>

struct BackupRecommendation {
    QString type;
    QString frequency;
    QString compression;
    QStringList excludePaths;
    QString reasoning;
    int priority;
    QDateTime suggestedTime;
};

struct SystemAnalysis {
    qint64 totalDiskSpace;
    qint64 usedSpace;
    qint64 availableSpace;
    int fileCount;
    int packageCount;
    QString systemType;
    QStringList largeDirectories;
    QStringList frequentlyChangedFiles;
    double changeRate; // files changed per day
};

class AIOptimizer : public QObject
{
    Q_OBJECT

public:
    explicit AIOptimizer(QObject *parent = nullptr);
    ~AIOptimizer();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void runAnalysis();
    void generateRecommendations();
    
    SystemAnalysis getSystemAnalysis() const { return m_systemAnalysis; }
    QList<BackupRecommendation> getRecommendations() const { return m_recommendations; }
    
    QString getOptimalCompressionMethod() const;
    QStringList getSuggestedExclusions() const;
    QString getRecommendedSchedule() const;
    int getOptimalCompressionLevel() const;
    
    // Learning and adaptation
    void recordBackupPerformance(const QString &type, qint64 duration, qint64 size);
    void updatePreferences(const QHash<QString, QVariant> &preferences);

signals:
    void analysisStarted();
    void analysisProgress(int percentage);
    void analysisCompleted();
    void recommendationsReady();
    void errorOccurred(const QString &error);

private slots:
    void performSystemScan();
    void analyzeFilePatterns();
    void calculateOptimalSettings();

private:
    void scanDiskUsage();
    void analyzeFileChanges();
    void analyzePkgStatistics();
    void evaluateCompressionOptions();
    void generateFrequencyRecommendation();
    void generateExclusionRecommendations();
    double calculateEfficiencyScore(const QString &compression) const;
    QString generateReasoningText(const BackupRecommendation &rec) const;
    
    bool m_enabled;
    SystemAnalysis m_systemAnalysis;
    QList<BackupRecommendation> m_recommendations;
    QTimer *m_analysisTimer;
    
    // Historical data for learning
    QHash<QString, QList<qint64>> m_backupDurations; // backup type -> list of durations
    QHash<QString, QList<qint64>> m_backupSizes;     // backup type -> list of sizes
    QHash<QString, QVariant> m_userPreferences;
    
    // AI parameters
    double m_sensitivityLevel;
    bool m_autoOptimize;
    QDateTime m_lastAnalysis;
};

#endif // AIOPTIMIZER_H
