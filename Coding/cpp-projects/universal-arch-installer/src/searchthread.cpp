#include "searchthread.h"
#include "packagemanager.h"
#include <QDebug>

SearchThread::SearchThread(QObject *parent) : QThread(parent) {
    m_packageManager = nullptr; // Will be set by main thread
    m_searching = false;
    m_cancelled = false;
    m_nextRequestId = 1;
    m_maxResults = DEFAULT_MAX_RESULTS;
    m_searchTimeoutMs = DEFAULT_SEARCH_TIMEOUT_MS;
    m_cacheEnabled = true;
    m_cacheTimeoutMs = DEFAULT_CACHE_TIMEOUT_MS;
    
    m_timeoutTimer = new QTimer();
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SearchThread::onSearchTimeout);
    m_timeoutTimer->moveToThread(this);
}

SearchThread::~SearchThread() {
    cancelAllSearches();
    if (isRunning()) {
        quit();
        wait();
    }
}

int SearchThread::searchPackages(const SearchRequest &request) {
    QMutexLocker locker(&m_mutex);
    
    SearchRequest req = request;
    req.requestId = m_nextRequestId++;
    
    m_searchQueue.enqueue(req);
    
    if (!isRunning()) {
        start();
    } else {
        m_condition.wakeOne();
    }
    
    return req.requestId;
}

void SearchThread::cancelSearch(int requestId) {
    QMutexLocker locker(&m_mutex);
    m_cancelledRequests.insert(requestId);
    
    if (m_currentRequest.requestId == requestId) {
        m_cancelled = true;
    }
}

void SearchThread::cancelAllSearches() {
    QMutexLocker locker(&m_mutex);
    m_cancelled = true;
    m_searchQueue.clear();
    m_cancelledRequests.clear();
}

bool SearchThread::isSearching() const {
    QMutexLocker locker(&m_mutex);
    return m_searching;
}

int SearchThread::getPendingSearchCount() const {
    QMutexLocker locker(&m_mutex);
    return m_searchQueue.size();
}

QString SearchThread::getCurrentSearchQuery() const {
    QMutexLocker locker(&m_mutex);
    return m_currentRequest.query;
}

void SearchThread::run() {
    while (!isInterruptionRequested()) {
        m_mutex.lock();
        
        if (m_searchQueue.isEmpty()) {
            m_condition.wait(&m_mutex, 100); // Add timeout to prevent infinite wait
        }
        
        if (m_searchQueue.isEmpty()) {
            m_mutex.unlock();
            continue;
        }
        
        m_currentRequest = m_searchQueue.dequeue();
        m_searching = true;
        m_cancelled = false;
        m_mutex.unlock();
        
        if (m_cancelledRequests.contains(m_currentRequest.requestId)) {
            emit searchCancelled(m_currentRequest.requestId);
            continue;
        }
        
        emit searchStarted(m_currentRequest.requestId, m_currentRequest.query);
        
        // Start timeout timer
        m_timeoutTimer->start(m_searchTimeoutMs);
        
        // Perform the actual search
        QList<PackageInfo> results = performSearch(m_currentRequest);
        
        // Stop timeout timer
        m_timeoutTimer->stop();
        
        m_mutex.lock();
        m_searching = false;
        m_mutex.unlock();
        
        if (!m_cancelled && !m_cancelledRequests.contains(m_currentRequest.requestId)) {
            emit searchCompleted(m_currentRequest.requestId, results);
        } else {
            emit searchCancelled(m_currentRequest.requestId);
        }
        
        // Clean up cancelled requests
        m_mutex.lock();
        m_cancelledRequests.remove(m_currentRequest.requestId);
        
        if (m_searchQueue.isEmpty()) {
            emit allSearchesCompleted();
            emit queueEmpty();
        }
        m_mutex.unlock();
    }
}

QList<PackageInfo> SearchThread::performSearch(const SearchRequest &request) {
    emitProgress(request.requestId, "Starting search...", 10);
    
    // Use PackageManager to perform the search
    QList<PackageInfo> results = m_packageManager->searchPackages(request.query, request.useAI);
    
    emitProgress(request.requestId, "Applying filters...", 50);
    
    // Apply filters
    results = applyFilters(results, request);
    
    emitProgress(request.requestId, "Ranking results...", 80);
    
    // Apply AI enhancement if requested
    if (request.useAI) {
        results = applyAIEnhancement(request.query, results);
    }
    
    // Apply sorting
    results = applySorting(results);
    
    emitProgress(request.requestId, "Finalizing results...", 100);
    
    // Limit results
    if (results.size() > m_maxResults) {
        results = results.mid(0, m_maxResults);
    }
    
    return results;
}

QList<PackageInfo> SearchThread::applyFilters(const QList<PackageInfo> &packages, const SearchRequest &request) {
    QList<PackageInfo> filtered;
    
    for (const PackageInfo &pkg : packages) {
        if (matchesFilters(pkg, request)) {
            filtered.append(pkg);
        }
    }
    
    return filtered;
}

bool SearchThread::matchesFilters(const PackageInfo &package, const SearchRequest &request) {
    // Category filter
    if (request.category != PackageCategory::OTHER && package.category != request.category) {
        return false;
    }
    
    // Description filter
    if (!request.description.isEmpty()) {
        if (!package.description.contains(request.description, Qt::CaseInsensitive)) {
            return false;
        }
    }
    
    // Size filter
    if (request.minSize > 0 || request.maxSize > 0) {
        // Parse package size (assuming format like "10 MB")
        QString sizeStr = package.size;
        sizeStr.remove(" MB").remove(" KB").remove(" GB");
        bool ok;
        int sizeInMB = sizeStr.toInt(&ok);
        if (!ok) sizeInMB = 0;
        
        if (request.minSize > 0 && sizeInMB < request.minSize) {
            return false;
        }
        if (request.maxSize > 0 && sizeInMB > request.maxSize) {
            return false;
        }
    }
    
    return true;
}

QList<PackageInfo> SearchThread::applyAIEnhancement(const QString &query, QList<PackageInfo> packages) {
    Q_UNUSED(query)
    // Mock AI enhancement - just return packages sorted by popularity
    std::sort(packages.begin(), packages.end(), [](const PackageInfo &a, const PackageInfo &b) {
        return a.popularity > b.popularity;
    });
    return packages;
}

QList<PackageInfo> SearchThread::applySorting(QList<PackageInfo> packages, const QString &sortBy) {
    if (sortBy == "name") {
        std::sort(packages.begin(), packages.end(), [](const PackageInfo &a, const PackageInfo &b) {
            return a.name < b.name;
        });
    } else if (sortBy == "popularity") {
        std::sort(packages.begin(), packages.end(), [](const PackageInfo &a, const PackageInfo &b) {
            return a.popularity > b.popularity;
        });
    }
    // Default is relevance (already sorted by search algorithm)
    
    return packages;
}

void SearchThread::emitProgress(int requestId, const QString &status, int percentage) {
    emit searchProgress(requestId, status, percentage);
}

void SearchThread::emitError(int requestId, const QString &error) {
    emit searchError(requestId, error);
}

void SearchThread::onSearchTimeout() {
    QMutexLocker locker(&m_mutex);
    if (m_searching) {
        m_cancelled = true;
        emitError(m_currentRequest.requestId, "Search timeout");
    }
}
