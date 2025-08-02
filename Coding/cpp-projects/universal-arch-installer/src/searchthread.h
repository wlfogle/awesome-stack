#ifndef SEARCHTHREAD_H
#define SEARCHTHREAD_H

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QQueue>
#include <QTimer>
#include <QElapsedTimer>
#include <QSet>
#include <QHash>
#include <algorithm>

#include "packageinfo.h"

class PackageManager;

struct SearchRequest {
    QString query;
    bool useAI;
    QStringList methods;
    PackageCategory category;
    QString description;
    int minSize;
    int maxSize;
    int requestId;
    
    SearchRequest() : 
        useAI(false), 
        category(PackageCategory::OTHER),
        minSize(0),
        maxSize(0),
        requestId(0) 
    {}
};

class SearchThread : public QThread
{
    Q_OBJECT

public:
    explicit SearchThread(QObject *parent = nullptr);
    ~SearchThread();

    // Public interface
    int searchPackages(const SearchRequest &request);
    void cancelSearch(int requestId = -1);
    void cancelAllSearches();
    
    // Configuration
    void setMaxResults(int maxResults);
    void setSearchTimeout(int timeoutMs);
    void setCacheEnabled(bool enabled);
    void setCacheTimeout(int timeoutMs);

    // Status
    bool isSearching() const;
    int getPendingSearchCount() const;
    QString getCurrentSearchQuery() const;

signals:
    void searchStarted(int requestId, const QString &query);
    void searchCompleted(int requestId, const QList<PackageInfo> &results);
    void searchProgress(int requestId, const QString &status, int percentage);
    void searchError(int requestId, const QString &error);
    void searchCancelled(int requestId);
    
    void allSearchesCompleted();
    void queueEmpty();

protected:
    void run() override;

private slots:
    void onSearchTimeout();

private:
    // Core search operations
    QList<PackageInfo> performSearch(const SearchRequest &request);
    QList<PackageInfo> searchWithFilters(const SearchRequest &request);
    QList<PackageInfo> combineAndDeduplicateResults(const QList<QList<PackageInfo>> &resultLists);
    
    // AI and ranking
    QList<PackageInfo> applyAIEnhancement(const QString &query, QList<PackageInfo> packages);
    QList<PackageInfo> applySorting(QList<PackageInfo> packages, const QString &sortBy = "relevance");
    double calculatePackageScore(const QString &query, const PackageInfo &package);
    
    // Filtering
    QList<PackageInfo> applyFilters(const QList<PackageInfo> &packages, const SearchRequest &request);
    bool matchesFilters(const PackageInfo &package, const SearchRequest &request);
    
    // Caching
    QString generateCacheKey(const SearchRequest &request);
    bool getCachedResults(const QString &cacheKey, QList<PackageInfo> &results);
    void setCachedResults(const QString &cacheKey, const QList<PackageInfo> &results);
    void clearExpiredCache();
    
    // Utilities
    void emitProgress(int requestId, const QString &status, int percentage);
    void emitError(int requestId, const QString &error);
    bool shouldCancelCurrentSearch();
    
    // Members
    PackageManager *m_packageManager;
    
    // Thread synchronization
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<SearchRequest> m_searchQueue;
    
    // Current search state
    SearchRequest m_currentRequest;
    bool m_searching;
    bool m_cancelled;
    QSet<int> m_cancelledRequests;
    
    // Configuration
    int m_maxResults;
    int m_searchTimeoutMs;
    bool m_cacheEnabled;
    int m_cacheTimeoutMs;
    
    // Cache
    struct CacheEntry {
        QList<PackageInfo> results;
        QDateTime timestamp;
    };
    QHash<QString, CacheEntry> m_cache;
    
    // Performance tracking
    QElapsedTimer m_searchTimer;
    QTimer *m_timeoutTimer;
    
    // Request tracking
    int m_nextRequestId;
    static const int MAX_CACHE_ENTRIES = 100;
    static const int DEFAULT_SEARCH_TIMEOUT_MS = 60000; // 1 minute
    static const int DEFAULT_CACHE_TIMEOUT_MS = 600000; // 10 minutes
    static const int DEFAULT_MAX_RESULTS = 500;
};

#endif // SEARCHTHREAD_H
