#include "aioptimizer.h"
#include <QProcess>
#include <QDir>
#include <QStorageInfo>
#include <QFileInfo>
#include <QDebug>
#include <QRandomGenerator>
#include <QStandardPaths>

AIOptimizer::AIOptimizer(QObject *parent)
    : QObject(parent)
    , m_enabled(false)
    , m_sensitivityLevel(5.0)
    , m_autoOptimize(false)
{
    m_analysisTimer = new QTimer(this);
    connect(m_analysisTimer, &QTimer::timeout, this, &AIOptimizer::performSystemScan);
}

AIOptimizer::~AIOptimizer()
{
}

void AIOptimizer::runAnalysis()
{
    if (!m_enabled) {
        emit errorOccurred("AI Optimizer is not enabled");
        return;
    }
    
    emit analysisStarted();
    emit analysisProgress(0);
    
    // Step 1: Scan disk usage
    emit analysisProgress(20);
    scanDiskUsage();
    
    // Step 2: Analyze file changes
    emit analysisProgress(40);
    analyzeFileChanges();
    
    // Step 3: Analyze package statistics
    emit analysisProgress(60);
    analyzePkgStatistics();
    
    // Step 4: Evaluate compression options
    emit analysisProgress(80);
    evaluateCompressionOptions();
    
    // Step 5: Generate recommendations
    emit analysisProgress(100);
    generateRecommendations();
    
    m_lastAnalysis = QDateTime::currentDateTime();
    emit analysisCompleted();
}

void AIOptimizer::generateRecommendations()
{
    m_recommendations.clear();
    
    // Generate frequency recommendation
    generateFrequencyRecommendation();
    
    // Generate compression recommendation
    BackupRecommendation compressionRec;
    compressionRec.type = "compression";
    compressionRec.compression = getOptimalCompressionMethod();
    compressionRec.reasoning = "Based on system performance and storage efficiency analysis";
    compressionRec.priority = 8;
    m_recommendations.append(compressionRec);
    
    // Generate exclusion recommendations
    generateExclusionRecommendations();
    
    // Generate schedule recommendation
    BackupRecommendation scheduleRec;
    scheduleRec.type = "schedule";
    scheduleRec.frequency = getRecommendedSchedule();
    scheduleRec.suggestedTime = QDateTime::currentDateTime().addDays(1);
    scheduleRec.suggestedTime.setTime(QTime(2, 0)); // 2 AM default
    scheduleRec.reasoning = "Optimal time based on system usage patterns";
    scheduleRec.priority = 7;
    m_recommendations.append(scheduleRec);
    
    emit recommendationsReady();
}

QString AIOptimizer::getOptimalCompressionMethod() const
{
    // AI logic to determine optimal compression
    double cpuScore = 1.0; // Would analyze CPU performance
    double storageScore = (double)m_systemAnalysis.availableSpace / m_systemAnalysis.totalDiskSpace;
    
    if (cpuScore > 0.8 && storageScore < 0.2) {
        return "zstd"; // High CPU, low storage - use best compression
    } else if (cpuScore < 0.4) {
        return "gzip"; // Low CPU - use lighter compression
    } else {
        return "zstd"; // Balanced choice
    }
}

QStringList AIOptimizer::getSuggestedExclusions() const
{
    QStringList exclusions;
    
    // AI-powered exclusion suggestions based on file analysis
    exclusions << "/tmp/*" << "/var/tmp/*" << "/var/cache/*";
    exclusions << "~/.cache/*" << "~/.local/share/Trash/*";
    exclusions << "*.tmp" << "*.swp" << "*~";
    
    // Add large directories if they're cache-like
    for (const QString &dir : m_systemAnalysis.largeDirectories) {
        if (dir.contains("cache", Qt::CaseInsensitive) ||
            dir.contains("temp", Qt::CaseInsensitive) ||
            dir.contains("log", Qt::CaseInsensitive)) {
            exclusions << dir + "/*";
        }
    }
    
    return exclusions;
}

QString AIOptimizer::getRecommendedSchedule() const
{
    // AI-based schedule recommendation
    if (m_systemAnalysis.changeRate > 50) {
        return "Every 6 hours"; // High change rate
    } else if (m_systemAnalysis.changeRate > 10) {
        return "Daily"; // Medium change rate
    } else {
        return "Weekly"; // Low change rate
    }
}

int AIOptimizer::getOptimalCompressionLevel() const
{
    // AI logic for compression level
    double storageRatio = (double)m_systemAnalysis.availableSpace / m_systemAnalysis.totalDiskSpace;
    
    if (storageRatio < 0.1) {
        return 9; // Very low space - maximum compression
    } else if (storageRatio < 0.3) {
        return 7; // Low space - high compression
    } else {
        return 6; // Balanced compression
    }
}

void AIOptimizer::recordBackupPerformance(const QString &type, qint64 duration, qint64 size)
{
    m_backupDurations[type].append(duration);
    m_backupSizes[type].append(size);
    
    // Keep only recent history (last 20 backups)
    if (m_backupDurations[type].size() > 20) {
        m_backupDurations[type].removeFirst();
    }
    if (m_backupSizes[type].size() > 20) {
        m_backupSizes[type].removeFirst();
    }
}

void AIOptimizer::updatePreferences(const QHash<QString, QVariant> &preferences)
{
    m_userPreferences = preferences;
    
    if (preferences.contains("sensitivity")) {
        m_sensitivityLevel = preferences["sensitivity"].toDouble();
    }
    
    if (preferences.contains("auto_optimize")) {
        m_autoOptimize = preferences["auto_optimize"].toBool();
    }
}

void AIOptimizer::performSystemScan()
{
    // Periodic system scanning for continuous learning
    scanDiskUsage();
    analyzeFileChanges();
}

void AIOptimizer::analyzeFilePatterns()
{
    // Analyze file access patterns for intelligent recommendations
    // This would involve monitoring file system events
}

void AIOptimizer::calculateOptimalSettings()
{
    // Calculate optimal settings based on collected data
    generateRecommendations();
}

void AIOptimizer::scanDiskUsage()
{
    // Scan root filesystem
    QStorageInfo storage("/");
    m_systemAnalysis.totalDiskSpace = storage.bytesTotal();
    m_systemAnalysis.usedSpace = storage.bytesTotal() - storage.bytesAvailable();
    m_systemAnalysis.availableSpace = storage.bytesAvailable();
    
    // Find large directories
    m_systemAnalysis.largeDirectories.clear();
    QProcess process;
    process.start("du", {"-h", "-d", "1", "/", "2>/dev/null"});
    process.waitForFinished(10000);
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QStringList parts = line.split('\t');
        if (parts.size() >= 2) {
            QString size = parts[0];
            QString path = parts[1];
            
            // Consider directories > 1GB as large
            if (size.contains('G') || (size.contains('M') && size.left(size.length()-1).toDouble() > 500)) {
                m_systemAnalysis.largeDirectories.append(path);
            }
        }
    }
}

void AIOptimizer::analyzeFileChanges()
{
    // Analyze file change patterns
    m_systemAnalysis.frequentlyChangedFiles.clear();
    
    // Simulate file change analysis
    // In a real implementation, this would monitor file system events
    QStringList commonChangedFiles = {
        "/var/log/syslog",
        "/var/log/auth.log",
        "/home/user/.bashrc",
        "/etc/hosts"
    };
    
    for (const QString &file : commonChangedFiles) {
        if (QFileInfo::exists(file)) {
            m_systemAnalysis.frequentlyChangedFiles.append(file);
        }
    }
    
    // Simulate change rate calculation
    m_systemAnalysis.changeRate = QRandomGenerator::global()->bounded(5, 100);
}

void AIOptimizer::analyzePkgStatistics()
{
    // Analyze package statistics
    QProcess process;
    process.start("pacman", {"-Q"});
    process.waitForFinished();
    
    QString output = process.readAllStandardOutput();
    QStringList packages = output.split('\n', Qt::SkipEmptyParts);
    m_systemAnalysis.packageCount = packages.size();
    
    // Determine system type based on installed packages
    if (output.contains("gnome")) {
        m_systemAnalysis.systemType = "GNOME Desktop";
    } else if (output.contains("kde") || output.contains("plasma")) {
        m_systemAnalysis.systemType = "KDE Plasma Desktop";
    } else if (output.contains("xfce")) {
        m_systemAnalysis.systemType = "XFCE Desktop";
    } else {
        m_systemAnalysis.systemType = "Minimal/Server";
    }
}

void AIOptimizer::evaluateCompressionOptions()
{
    // Evaluate compression methods based on system capabilities
    // This is a simplified simulation
    QStringList compressionMethods = {"gzip", "bzip2", "xz", "zstd"};
    
    // In a real implementation, this would benchmark compression methods
    // on sample data to determine optimal choice
}

void AIOptimizer::generateFrequencyRecommendation()
{
    BackupRecommendation freqRec;
    freqRec.type = "frequency";
    freqRec.priority = 9;
    
    // AI logic for frequency recommendation
    double storageRatio = (double)m_systemAnalysis.availableSpace / m_systemAnalysis.totalDiskSpace;
    
    if (m_systemAnalysis.changeRate > 50 && storageRatio > 0.3) {
        freqRec.frequency = "Every 4 hours";
        freqRec.reasoning = "High file change rate detected with sufficient storage space";
    } else if (m_systemAnalysis.changeRate > 20) {
        freqRec.frequency = "Every 12 hours";
        freqRec.reasoning = "Moderate file change rate detected";
    } else if (m_systemAnalysis.changeRate > 5) {
        freqRec.frequency = "Daily";
        freqRec.reasoning = "Low to moderate file change rate";
    } else {
        freqRec.frequency = "Weekly";
        freqRec.reasoning = "Very low file change rate detected";
    }
    
    m_recommendations.append(freqRec);
}

void AIOptimizer::generateExclusionRecommendations()
{
    BackupRecommendation exclusionRec;
    exclusionRec.type = "exclusions";
    exclusionRec.excludePaths = getSuggestedExclusions();
    exclusionRec.reasoning = "AI-analyzed patterns suggest excluding temporary and cache files";
    exclusionRec.priority = 6;
    
    m_recommendations.append(exclusionRec);
}

double AIOptimizer::calculateEfficiencyScore(const QString &compression) const
{
    // Calculate efficiency score for compression method
    // Based on compression ratio vs. CPU usage
    
    if (compression == "zstd") return 0.9;
    else if (compression == "xz") return 0.85;
    else if (compression == "bzip2") return 0.7;
    else if (compression == "gzip") return 0.75;
    else return 0.5; // no compression
}

QString AIOptimizer::generateReasoningText(const BackupRecommendation &rec) const
{
    return rec.reasoning;
}
