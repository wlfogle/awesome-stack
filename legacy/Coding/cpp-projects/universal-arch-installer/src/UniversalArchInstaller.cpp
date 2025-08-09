// UniversalArchInstaller.cpp
// Primary C++ implementation utilizing features from the Python code
#include "UniversalArchInstaller.h"

UniversalArchInstaller::UniversalArchInstaller() {
    m_mainWindow = new MainWindow();
    m_packageManager = new PackageManager();
    m_searchThread = new SearchThread();
    m_performanceMonitor = new PerformanceMonitor();
    m_aiManager = new AIManager();

    initialize();
}

void UniversalArchInstaller::initialize() {
    // Here initialize connections and setups as per the required features
    connect(m_mainWindow, SIGNAL(requestSearchPackages(QString)),
            m_packageManager, SLOT(searchPackages(QString)));
    // More connects similar to above for AI and performance features
}

QList<PackageInfo> UniversalArchInstaller::searchWithAI(const QString &query) {
    QList<PackageInfo> packages = m_packageManager->searchPackages(query);
    if (m_aiManager) {
        packages = m_aiManager->applyRanking(query, packages);
    }
    return packages;
}

void UniversalArchInstaller::manageInstallation(const QString &packageName) {
    m_packageManager->installPackage(PackageInfo{.name = packageName});
}

