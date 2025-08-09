#include "mainwindow.h"
#include "packagemanager.h"
#include "searchthread.h"
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QSplitter>
#include <QInputDialog>

// ============================================================================
// SEARCH TAB IMPLEMENTATION
// ============================================================================

QWidget* MainWindow::createSearchTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Create Search sub-tabs
    QTabWidget *searchTabs = new QTabWidget();
    layout->addWidget(searchTabs);
    
    // Quick Search Tab
    QWidget *quickSearchTab = createQuickSearchTab();
    searchTabs->addTab(quickSearchTab, "🔍 Quick Search");
    
    // Advanced Search Tab
    QWidget *advancedSearchTab = createAdvancedSearchTab();
    searchTabs->addTab(advancedSearchTab, "🎯 Advanced Search");
    
    // Search Results Tab
    QWidget *resultsTab = createSearchResultsTab();
    searchTabs->addTab(resultsTab, "📋 Search Results");
    
    // Search History Tab
    QWidget *historyTab = createSearchHistoryTab();
    searchTabs->addTab(historyTab, "📚 Search History");
    
    setupSearchConnections();
    return widget;
}

QWidget* MainWindow::createQuickSearchTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Quick search input
    QGroupBox *searchGroup = new QGroupBox("🔍 Quick Package Search");
    QVBoxLayout *searchLayout = new QVBoxLayout(searchGroup);
    
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("Enter package name to search...");
    m_searchInput->setStyleSheet("QLineEdit { font-size: 14px; padding: 8px; }");
    inputLayout->addWidget(m_searchInput);
    
    m_searchButton = new QPushButton("🔍 Search");
    m_searchButton->setStyleSheet("QPushButton { font-size: 14px; padding: 8px 16px; }");
    inputLayout->addWidget(m_searchButton);
    
    searchLayout->addLayout(inputLayout);
    
    // Search options
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    m_aiSearchCheck = new QCheckBox("Use AI-powered search");
    m_aiSearchCheck->setChecked(true);
    optionsLayout->addWidget(m_aiSearchCheck);
    
    m_includeAURCheck = new QCheckBox("Include AUR packages");
    m_includeAURCheck->setChecked(true);
    optionsLayout->addWidget(m_includeAURCheck);
    
    m_includeFlatpakCheck = new QCheckBox("Include Flatpak");
    optionsLayout->addWidget(m_includeFlatpakCheck);
    
    optionsLayout->addStretch();
    searchLayout->addLayout(optionsLayout);
    
    layout->addWidget(searchGroup);
    
    // Popular packages
    QGroupBox *popularGroup = new QGroupBox("⭐ Popular Packages");
    QGridLayout *popularLayout = new QGridLayout(popularGroup);
    
    QStringList popularPackages = {
        "🌐 Firefox", "🎬 VLC", "💻 VS Code", "🎨 GIMP",
        "🗃️ LibreOffice", "🎮 Steam", "🐳 Docker", "📝 Git"
    };
    
    QStringList packageNames = {
        "firefox", "vlc", "code", "gimp",
        "libreoffice-fresh", "steam", "docker", "git"
    };
    
    for (int i = 0; i < popularPackages.size(); ++i) {
        QPushButton *btn = new QPushButton(popularPackages[i]);
        btn->setProperty("packageName", packageNames[i]);
        btn->setStyleSheet("QPushButton { padding: 8px; margin: 2px; }");
        connect(btn, &QPushButton::clicked, [this, btn]() {
            QString packageName = btn->property("packageName").toString();
            searchForPopularPackage(packageName);
        });
        popularLayout->addWidget(btn, i / 4, i % 4);
    }
    
    layout->addWidget(popularGroup);
    layout->addStretch();
    
    return widget;
}

QWidget* MainWindow::createAdvancedSearchTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Advanced filters
    QGroupBox *filtersGroup = new QGroupBox("🎯 Advanced Search Filters");
    QFormLayout *filtersLayout = new QFormLayout(filtersGroup);
    
    m_advPackageName = new QLineEdit();
    m_advPackageName->setPlaceholderText("Package name or keywords...");
    filtersLayout->addRow("Package Name:", m_advPackageName);
    
    m_advCategoryCombo = new QComboBox();
    m_advCategoryCombo->addItems({
        "All Categories", "Development", "System", "Multimedia", "Games", 
        "Internet", "Office", "Graphics", "Education", "Science", 
        "Utilities", "Security", "Terminal", "Other"
    });
    filtersLayout->addRow("Category:", m_advCategoryCombo);
    
    m_advMethodCombo = new QComboBox();
    m_advMethodCombo->addItems({
        "All Methods", "Pacman", "YAY", "Paru", "Pikaur", "Flatpak", "Snap", "PIP"
    });
    filtersLayout->addRow("Install Method:", m_advMethodCombo);
    
    m_advDescription = new QLineEdit();
    m_advDescription->setPlaceholderText("Search in description...");
    filtersLayout->addRow("Description:", m_advDescription);
    
    // Size filter
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("Min:");
    m_minSizeSpinBox = new QSpinBox();
    m_minSizeSpinBox->setRange(0, 10000);
    m_minSizeSpinBox->setSuffix(" MB");
    sizeLayout->addWidget(minLabel);
    sizeLayout->addWidget(m_minSizeSpinBox);
    
    QLabel *maxLabel = new QLabel("Max:");
    m_maxSizeSpinBox = new QSpinBox();
    m_maxSizeSpinBox->setRange(0, 10000);
    m_maxSizeSpinBox->setValue(1000);
    m_maxSizeSpinBox->setSuffix(" MB");
    sizeLayout->addWidget(maxLabel);
    sizeLayout->addWidget(m_maxSizeSpinBox);
    
    filtersLayout->addRow("Package Size:", sizeLayout);
    
    // Search buttons
    QHBoxLayout *searchButtons = new QHBoxLayout();
    QPushButton *advancedSearchBtn = new QPushButton("🔍 Advanced Search");
    connect(advancedSearchBtn, &QPushButton::clicked, this, &MainWindow::performAdvancedSearch);
    searchButtons->addWidget(advancedSearchBtn);
    
    QPushButton *clearFiltersBtn = new QPushButton("🧹 Clear Filters");
    connect(clearFiltersBtn, &QPushButton::clicked, this, &MainWindow::clearSearchFilters);
    searchButtons->addWidget(clearFiltersBtn);
    
    searchButtons->addStretch();
    filtersLayout->addRow(searchButtons);
    
    layout->addWidget(filtersGroup);
    
    // Saved searches
    QGroupBox *savedGroup = new QGroupBox("💾 Saved Searches");
    QVBoxLayout *savedLayout = new QVBoxLayout(savedGroup);
    
    QHBoxLayout *savedControls = new QHBoxLayout();
    QPushButton *saveSearchBtn = new QPushButton("💾 Save Current Search");
    connect(saveSearchBtn, &QPushButton::clicked, this, &MainWindow::saveCurrentSearch);
    savedControls->addWidget(saveSearchBtn);
    
    QPushButton *manageSavedBtn = new QPushButton("📂 Manage Saved");
    connect(manageSavedBtn, &QPushButton::clicked, this, &MainWindow::manageSavedSearches);
    savedControls->addWidget(manageSavedBtn);
    
    savedControls->addStretch();
    savedLayout->addLayout(savedControls);
    
    m_savedSearchesList = new QListWidget();
    connect(m_savedSearchesList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem *item) {
        QString searchData = item->data(Qt::UserRole).toString();
        // Load saved search parameters
        QJsonDocument doc = QJsonDocument::fromJson(searchData.toUtf8());
        QJsonObject obj = doc.object();
        m_advPackageName->setText(obj["name"].toString());
        m_advDescription->setText(obj["description"].toString());
        m_advCategoryCombo->setCurrentText(obj["category"].toString());
        m_advMethodCombo->setCurrentText(obj["method"].toString());
        performAdvancedSearch();
    });
    savedLayout->addWidget(m_savedSearchesList);
    
    layout->addWidget(savedGroup);
    layout->addStretch();
    
    loadSavedSearches();
    return widget;
}

QWidget* MainWindow::createSearchResultsTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // Results controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(new QLabel("Sort by:"));
    
    m_resultsSortCombo = new QComboBox();
    m_resultsSortCombo->addItems({"Relevance", "Name", "Popularity", "Size", "Date"});
    connect(m_resultsSortCombo, &QComboBox::currentTextChanged, 
            this, &MainWindow::sortSearchResults);
    controlsLayout->addWidget(m_resultsSortCombo);
    
    m_resultsFilterInput = new QLineEdit();
    m_resultsFilterInput->setPlaceholderText("Filter results...");
    connect(m_resultsFilterInput, &QLineEdit::textChanged,
            this, &MainWindow::filterSearchResults);
    controlsLayout->addWidget(m_resultsFilterInput);
    
    QPushButton *exportResultsBtn = new QPushButton("📤 Export Results");
    connect(exportResultsBtn, &QPushButton::clicked, this, &MainWindow::exportSearchHistory);
    controlsLayout->addWidget(exportResultsBtn);
    
    controlsLayout->addStretch();
    layout->addLayout(controlsLayout);
    
    // Results table
    setupSearchResultsTable();
    layout->addWidget(m_resultsTable);
    
    // Results status
    QHBoxLayout *statusLayout = new QHBoxLayout();
    m_searchStatus = new QLabel("Ready to search packages...");
    m_searchStatus->setStyleSheet("QLabel { color: #666; font-style: italic; }");
    statusLayout->addWidget(m_searchStatus);
    
    statusLayout->addStretch();
    
    QPushButton *installSelectedBtn = new QPushButton("📦 Install Selected");
    connect(installSelectedBtn, &QPushButton::clicked, this, &MainWindow::installSelectedResults);
    installSelectedBtn->setEnabled(false);
    statusLayout->addWidget(installSelectedBtn);
    
    layout->addLayout(statusLayout);
    
    return widget;
}

QWidget* MainWindow::createSearchHistoryTab()
{
    QWidget *widget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    
    // History controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    QPushButton *clearHistoryBtn = new QPushButton("🗑️ Clear History");
    connect(clearHistoryBtn, &QPushButton::clicked, this, &MainWindow::clearSearchHistory);
    controlsLayout->addWidget(clearHistoryBtn);
    
    QPushButton *exportHistoryBtn = new QPushButton("📤 Export History");
    connect(exportHistoryBtn, &QPushButton::clicked, this, &MainWindow::exportSearchHistory);
    controlsLayout->addWidget(exportHistoryBtn);
    
    controlsLayout->addStretch();
    layout->addLayout(controlsLayout);
    
    // History table
    setupSearchHistoryTable();
    layout->addWidget(m_historyTable);
    
    // Load search history
    loadSearchHistory();
    
    return widget;
}

// ============================================================================
// SEARCH TAB HELPER METHODS
// ============================================================================

void MainWindow::setupSearchConnections()
{
    // Connect search input return key to search
    connect(m_searchInput, &QLineEdit::returnPressed, this, &MainWindow::performSearch);
    connect(m_searchButton, &QPushButton::clicked, this, &MainWindow::performSearch);
    
    // Connect to search thread signals (if available)
    if (m_searchThread) {
        connect(m_searchThread, QOverload<int, const QList<PackageInfo>&>::of(&SearchThread::searchCompleted),
                [this](int requestId, const QList<PackageInfo> &results) {
                    Q_UNUSED(requestId)
                    onSearchCompleted(results);
                });
        connect(m_searchThread, QOverload<int, const QString&>::of(&SearchThread::searchError),
                [this](int requestId, const QString &error) {
                    Q_UNUSED(requestId)
                    onSearchError(error);
                });
    }
}

void MainWindow::setupSearchResultsTable()
{
    m_resultsTable = new QTableWidget();
    m_resultsTable->setColumnCount(7);
    m_resultsTable->setHorizontalHeaderLabels({
        "✓", "Name", "Method", "Version", "Description", "Category", "Actions"
    });
    
    // Set column widths
    m_resultsTable->setColumnWidth(0, 30);  // Checkbox
    m_resultsTable->setColumnWidth(1, 150); // Name
    m_resultsTable->setColumnWidth(2, 80);  // Method
    m_resultsTable->setColumnWidth(3, 80);  // Version
    m_resultsTable->setColumnWidth(5, 100); // Category
    m_resultsTable->setColumnWidth(6, 120); // Actions
    
    m_resultsTable->horizontalHeader()->setStretchLastSection(false);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch); // Description
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setAlternatingRowColors(true);
    m_resultsTable->setSortingEnabled(true);
    
    connect(m_resultsTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onPackageSelectionChanged);
}

void MainWindow::setupSearchHistoryTable()
{
    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(5);
    m_historyTable->setHorizontalHeaderLabels({
        "Search Query", "Results Found", "Search Time", "Date", "Actions"
    });
    
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->horizontalHeader()->setStretchLastSection(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    connect(m_historyTable, &QTableWidget::cellDoubleClicked, [this](int row, int column) {
        Q_UNUSED(column)
        QString query = m_historyTable->item(row, 0)->text();
        m_searchInput->setText(query);
        performSearch();
    });
}

void MainWindow::performSearch()
{
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty()) {
        updateSearchStatus("Please enter a search query");
        return;
    }
    
    updateSearchStatus("Searching for packages...");
    m_searchButton->setEnabled(false);
    m_searchButton->setText("🔄 Searching...");
    
    // Create search request
    SearchRequest request;
    request.query = query;
    request.useAI = m_aiSearchCheck->isChecked();
    
    // Add methods based on checkboxes
    if (m_includeAURCheck->isChecked()) {
        request.methods << "YAY" << "PARU";
    }
    if (m_includeFlatpakCheck->isChecked()) {
        request.methods << "FLATPAK";
    }
    
    // Start search
    if (m_searchThread) {
        int requestId = m_searchThread->searchPackages(request);
        Q_UNUSED(requestId)
    } else {
        // Use PackageManager directly for testing
        QList<PackageInfo> results = m_packageManager->searchPackages(query, request.useAI);
        onSearchCompleted(results);
    }
}

void MainWindow::performAdvancedSearch()
{
    QString query = m_advPackageName->text().trimmed();
    if (query.isEmpty()) {
        updateSearchStatus("Please enter a package name for advanced search");
        return;
    }
    
    updateSearchStatus("Performing advanced search...");
    
    // Create advanced search request
    SearchRequest request;
    request.query = query;
    request.description = m_advDescription->text();
    request.category = static_cast<PackageCategory>(m_advCategoryCombo->currentIndex());
    request.minSize = m_minSizeSpinBox->value();
    request.maxSize = m_maxSizeSpinBox->value();
    
    // Add specific method if selected
    QString method = m_advMethodCombo->currentText();
    if (method != "All Methods") {
        request.methods << method.toUpper();
    }
    
    // Start search
    if (m_searchThread) {
        int requestId = m_searchThread->searchPackages(request);
        Q_UNUSED(requestId)
    } else {
        // Use PackageManager directly for testing
        QList<PackageInfo> results = m_packageManager->searchPackages(query, false);
        onSearchCompleted(results);
    }
}

void MainWindow::onSearchCompleted(const QList<PackageInfo> &results)
{
    m_searchResults = results;
    displaySearchResults(results);
    
    // Update UI
    m_searchButton->setEnabled(true);
    m_searchButton->setText("🔍 Search");
    
    QString status = QString("Found %1 packages").arg(results.size());
    updateSearchStatus(status);
    
    // Add to history
    QString query = m_searchInput->text();
    if (!query.isEmpty()) {
        addSearchToHistory(query, results.size());
    }
}

void MainWindow::onSearchError(const QString &error)
{
    m_searchButton->setEnabled(true);
    m_searchButton->setText("🔍 Search");
    updateSearchStatus("Search error: " + error);
    
    QMessageBox::warning(this, "Search Error", 
                        "Search failed: " + error);
}

void MainWindow::displaySearchResults(const QList<PackageInfo> &packages)
{
    m_resultsTable->setRowCount(packages.size());
    
    for (int i = 0; i < packages.size(); ++i) {
        const PackageInfo &pkg = packages[i];
        
        // Checkbox with better visibility
        QCheckBox *checkBox = new QCheckBox();
        checkBox->setStyleSheet("QCheckBox { margin: 5px; background-color: white; border: 1px solid #ccc; }");
        QWidget *checkboxWidget = new QWidget();
        QHBoxLayout *checkboxLayout = new QHBoxLayout(checkboxWidget);
        checkboxLayout->addWidget(checkBox);
        checkboxLayout->setAlignment(Qt::AlignCenter);
        checkboxLayout->setContentsMargins(5, 5, 5, 5);
        m_resultsTable->setCellWidget(i, 0, checkboxWidget);
        
        // Package details
        m_resultsTable->setItem(i, 1, new QTableWidgetItem(pkg.name));
        m_resultsTable->setItem(i, 2, new QTableWidgetItem(pkg.methodString()));
        m_resultsTable->setItem(i, 3, new QTableWidgetItem(pkg.version));
        m_resultsTable->setItem(i, 4, new QTableWidgetItem(pkg.description));
        m_resultsTable->setItem(i, 5, new QTableWidgetItem(pkg.categoryString()));
        
        // Actions buttons
        QWidget *actionsWidget = new QWidget();
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(4, 2, 4, 2);
        
        QPushButton *installBtn = new QPushButton("📦");
        installBtn->setToolTip("Install package");
        installBtn->setMaximumWidth(30);
        installBtn->setProperty("packageName", pkg.name);
        installBtn->setProperty("packageMethod", static_cast<int>(pkg.method));
        connect(installBtn, &QPushButton::clicked, [this, installBtn]() {
            QString packageName = installBtn->property("packageName").toString();
            // Install single package
            PackageInfo pkg;
            pkg.name = packageName;
            m_packageManager->installPackage(pkg);
        });
        actionsLayout->addWidget(installBtn);
        
        QPushButton *infoBtn = new QPushButton("ℹ️");
        infoBtn->setToolTip("Package information");
        infoBtn->setMaximumWidth(30);
        infoBtn->setProperty("packageIndex", i);
        connect(infoBtn, &QPushButton::clicked, [this, infoBtn]() {
            int index = infoBtn->property("packageIndex").toInt();
            if (index < m_searchResults.size()) {
                showPackageInfo(m_searchResults[index]);
            }
        });
        actionsLayout->addWidget(infoBtn);
        
        QPushButton *queueBtn = new QPushButton("➕");
        queueBtn->setToolTip("Add to install queue");
        queueBtn->setMaximumWidth(30);
        queueBtn->setProperty("packageIndex", i);
        connect(queueBtn, &QPushButton::clicked, [this, queueBtn]() {
            int index = queueBtn->property("packageIndex").toInt();
            if (index < m_searchResults.size()) {
                m_installQueue.append(m_searchResults[index]);
                updateInstallQueueStats();
            }
        });
        actionsLayout->addWidget(queueBtn);
        
        m_resultsTable->setCellWidget(i, 6, actionsWidget);
    }
    
    m_resultsTable->resizeRowsToContents();
}

void MainWindow::updateSearchStatus(const QString &status)
{
    m_searchStatus->setText(status);
    statusBar()->showMessage(status, 3000);
}

void MainWindow::addSearchToHistory(const QString &query, int results)
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QDir().mkpath(configDir);
    
    QString historyFile = configDir + "/search_history.json";
    QJsonArray historyArray;
    
    // Load existing history
    QFile file(historyFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        historyArray = doc.array();
        file.close();
    }
    
    // Add new entry
    QJsonObject newEntry;
    newEntry["query"] = query;
    newEntry["results"] = results;
    newEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    newEntry["searchTime"] = "< 1s"; // Could be calculated
    
    historyArray.prepend(newEntry);
    
    // Keep only last 100 entries
    while (historyArray.size() > 100) {
        historyArray.removeLast();
    }
    
    // Save history
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(historyArray);
        file.write(doc.toJson());
        file.close();
    }
    
    // Refresh history table
    loadSearchHistory();
}

void MainWindow::loadSearchHistory()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QString historyFile = configDir + "/search_history.json";
    
    QFile file(historyFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray historyArray = doc.array();
    file.close();
    
    m_historyTable->setRowCount(historyArray.size());
    
    for (int i = 0; i < historyArray.size(); ++i) {
        QJsonObject entry = historyArray[i].toObject();
        
        m_historyTable->setItem(i, 0, new QTableWidgetItem(entry["query"].toString()));
        m_historyTable->setItem(i, 1, new QTableWidgetItem(QString::number(entry["results"].toInt())));
        m_historyTable->setItem(i, 2, new QTableWidgetItem(entry["searchTime"].toString()));
        
        QDateTime timestamp = QDateTime::fromString(entry["timestamp"].toString(), Qt::ISODate);
        m_historyTable->setItem(i, 3, new QTableWidgetItem(timestamp.toString("yyyy-MM-dd hh:mm")));
        
        // Actions
        QWidget *actionsWidget = new QWidget();
        QHBoxLayout *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(4, 2, 4, 2);
        
        QPushButton *repeatBtn = new QPushButton("🔄");
        repeatBtn->setToolTip("Repeat search");
        repeatBtn->setMaximumWidth(30);
        repeatBtn->setProperty("query", entry["query"].toString());
        connect(repeatBtn, &QPushButton::clicked, [this, repeatBtn]() {
            QString query = repeatBtn->property("query").toString();
            m_searchInput->setText(query);
            performSearch();
        });
        actionsLayout->addWidget(repeatBtn);
        
        m_historyTable->setCellWidget(i, 4, actionsWidget);
    }
}

void MainWindow::loadSavedSearches()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QString savedFile = configDir + "/saved_searches.json";
    
    QFile file(savedFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray savedArray = doc.array();
    file.close();
    
    m_savedSearchesList->clear();
    
    for (const QJsonValue &value : savedArray) {
        QJsonObject search = value.toObject();
        QString name = search["name"].toString();
        
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, QJsonDocument(search).toJson(QJsonDocument::Compact));
        m_savedSearchesList->addItem(item);
    }
}

void MainWindow::saveCurrentSearch()
{
    QString name = QInputDialog::getText(this, "Save Search", 
                                        "Enter a name for this search:");
    if (name.isEmpty()) return;
    
    QJsonObject search;
    search["name"] = name;
    search["package"] = m_advPackageName->text();
    search["description"] = m_advDescription->text();
    search["category"] = m_advCategoryCombo->currentText();
    search["method"] = m_advMethodCombo->currentText();
    search["minSize"] = m_minSizeSpinBox->value();
    search["maxSize"] = m_maxSizeSpinBox->value();
    search["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QDir().mkpath(configDir);
    QString savedFile = configDir + "/saved_searches.json";
    
    QJsonArray savedArray;
    QFile file(savedFile);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        savedArray = doc.array();
        file.close();
    }
    
    savedArray.append(search);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(savedArray);
        file.write(doc.toJson());
        file.close();
    }
    
    loadSavedSearches();
}

void MainWindow::manageSavedSearches()
{
    // Open a dialog to manage saved searches
    QMessageBox::information(this, "Manage Saved Searches", 
                            "Saved searches management dialog would open here.");
}

void MainWindow::clearSearchFilters()
{
    m_advPackageName->clear();
    m_advDescription->clear();
    m_advCategoryCombo->setCurrentIndex(0);
    m_advMethodCombo->setCurrentIndex(0);
    m_minSizeSpinBox->setValue(0);
    m_maxSizeSpinBox->setValue(1000);
}

void MainWindow::searchForPopularPackage(const QString &package)
{
    m_searchInput->setText(package);
    performSearch();
}

void MainWindow::filterSearchResults(const QString &filter)
{
    for (int i = 0; i < m_resultsTable->rowCount(); ++i) {
        bool show = true;
        if (!filter.isEmpty()) {
            QString name = m_resultsTable->item(i, 1)->text();
            QString desc = m_resultsTable->item(i, 4)->text();
            show = name.contains(filter, Qt::CaseInsensitive) || 
                   desc.contains(filter, Qt::CaseInsensitive);
        }
        m_resultsTable->setRowHidden(i, !show);
    }
}

void MainWindow::sortSearchResults(const QString &sortBy)
{
    int column = 1; // Default to name
    if (sortBy == "Relevance") column = 1;
    else if (sortBy == "Name") column = 1;
    else if (sortBy == "Method") column = 2;
    else if (sortBy == "Version") column = 3;
    else if (sortBy == "Category") column = 5;
    
    m_resultsTable->sortItems(column);
}

void MainWindow::installSelectedResults()
{
    QList<PackageInfo> selectedPackages;
    
    for (int i = 0; i < m_resultsTable->rowCount(); ++i) {
        // Get the checkbox widget container
        QWidget *checkboxWidget = m_resultsTable->cellWidget(i, 0);
        if (checkboxWidget) {
            // Find the checkbox inside the container widget
            QCheckBox *checkBox = checkboxWidget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked() && i < m_searchResults.size()) {
                selectedPackages.append(m_searchResults[i]);
            }
        }
    }
    
    if (selectedPackages.isEmpty()) {
        QMessageBox::information(this, "No Selection", 
                                "Please select packages to install.");
        return;
    }
    
    // Handle single vs batch install logic
    if (selectedPackages.size() == 1) {
        // Single package - pass to single install tab
        addPackagesToInstall(selectedPackages);
        QString message = QString("Added package '%1' to single install tab.").arg(selectedPackages.first().name);
        QMessageBox::information(this, "Switched to Single Install", message);
    } else {
        // Multiple packages - pass to batch install tab
        addPackagesToInstall(selectedPackages);
        QString message = QString("Added %1 packages to batch install tab.").arg(selectedPackages.size());
        QMessageBox::information(this, "Switched to Batch Install", message);
    }
}

void MainWindow::clearSearchHistory()
{
    if (QMessageBox::question(this, "Clear History", 
                             "Are you sure you want to clear the search history?") 
        == QMessageBox::Yes) {
        
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                           + "/universal-arch-installer";
        QString historyFile = configDir + "/search_history.json";
        QFile::remove(historyFile);
        
        m_historyTable->setRowCount(0);
        updateSearchStatus("Search history cleared");
    }
}

void MainWindow::exportSearchHistory()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Export Search History", 
        QDir::homePath() + "/search_history.csv",
        "CSV Files (*.csv)");
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Query,Results,Search Time,Date\n";
            
            for (int i = 0; i < m_historyTable->rowCount(); ++i) {
                out << m_historyTable->item(i, 0)->text() << ","
                    << m_historyTable->item(i, 1)->text() << ","
                    << m_historyTable->item(i, 2)->text() << ","
                    << m_historyTable->item(i, 3)->text() << "\n";
            }
            
            file.close();
            QMessageBox::information(this, "Export Complete", 
                                    "Search history exported successfully.");
        }
    }
}

// Helper method to show package information
void MainWindow::showPackageInfo(const PackageInfo &package)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Package Information");
    msgBox.setTextFormat(Qt::RichText);
    
    QString info = QString(
        "<h3>%1</h3>"
        "<p><b>Version:</b> %2</p>"
        "<p><b>Method:</b> %3</p>"
        "<p><b>Category:</b> %4</p>"
        "<p><b>Description:</b> %5</p>"
        "<p><b>Size:</b> %6</p>"
        "<p><b>Maintainer:</b> %7</p>"
        "<p><b>License:</b> %8</p>"
    ).arg(package.name)
     .arg(package.version)
     .arg(package.methodString())
     .arg(package.categoryString())
     .arg(package.description)
     .arg(package.size)
     .arg(package.maintainer)
     .arg(package.license);
    
    msgBox.setText(info);
    msgBox.exec();
}
