#include "filemanagerwidget.h"

FileManagerWidget::FileManagerWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FileManagerWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Toolbar
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    openFileButton = new QPushButton("Open File");
    openFolderButton = new QPushButton("Open Folder");
    toolbarLayout->addWidget(openFileButton);
    toolbarLayout->addWidget(openFolderButton);
    toolbarLayout->addStretch();
    layout->addLayout(toolbarLayout);
    
    // File tree
    fileTree = new QTreeWidget();
    fileTree->setHeaderLabel("Project Files");
    layout->addWidget(fileTree);
    
    // File content editor (now editable!)
    fileViewer = new CodeEditor();
    fileViewer->setReadOnly(false); // Make it editable
    layout->addWidget(fileViewer);
    
    // Connections
    connect(openFileButton, &QPushButton::clicked, this, &FileManagerWidget::onOpenFileClicked);
    connect(openFolderButton, &QPushButton::clicked, this, &FileManagerWidget::onOpenFolderClicked);
    connect(fileTree, &QTreeWidget::itemSelectionChanged, this, &FileManagerWidget::onFileItemSelectionChanged);
}

void FileManagerWidget::onOpenFileClicked()
{
    QString startDir = getLastUsedDirectory();
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", startDir, 
        "Code Files (*.cpp *.h *.hpp *.c *.cc *.cxx *.py *.js *.ts *.java *.cs *.php *.rb *.go *.rs);;All Files (*)");
    
    if (!filePath.isEmpty()) {
        // Save the directory of the selected file
        setLastUsedDirectory(QFileInfo(filePath).absolutePath());
        
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            fileViewer->setPlainText(in.readAll());
            file.close();
            
            // Track current file for real-time analysis
            currentFilePath = filePath;

            // Add to file tree
            QTreeWidgetItem *item = new QTreeWidgetItem(fileTree);
            item->setText(0, QFileInfo(filePath).fileName());
            fileTree->addTopLevelItem(item);
        } else {
            QMessageBox::warning(this, "Error", "Failed to open file.");
        }
    }
}

void FileManagerWidget::onOpenFolderClicked()
{
    QString startDir = getLastUsedDirectory();
    QString folderPath = QFileDialog::getExistingDirectory(this, "Open Folder", startDir);
    
    if (!folderPath.isEmpty()) {
        // Save the selected folder path
        setLastUsedDirectory(folderPath);
        
        populateFileTree(folderPath);
        emit folderOpened(folderPath);
    }
}

void FileManagerWidget::populateFileTree(const QString &folderPath)
{
    fileTree->clear();
    lastOpenedFolder = folderPath; // Store for building file paths
    
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(fileTree);
    rootItem->setText(0, QFileInfo(folderPath).fileName());
    addItemsToTree(rootItem, folderPath);
    rootItem->setExpanded(true);
}

void FileManagerWidget::addItemsToTree(QTreeWidgetItem *parentItem, const QString &path)
{
    QDir dir(path);
    QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::Name);

    foreach (QFileInfo info, list) {
        QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
        item->setText(0, info.fileName());
        if (info.isDir()) {
            addItemsToTree(item, info.filePath());
        }
    }
}

void FileManagerWidget::onFileItemSelectionChanged()
{
    QList<QTreeWidgetItem*> selectedItems = fileTree->selectedItems();
    if (!selectedItems.isEmpty()) {
        QTreeWidgetItem *item = selectedItems.first();
        QString fileName = item->text(0);
        
        // Build full file path from tree structure
        QString filePath = buildFilePathFromItem(item);
        
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            loadFileContent(filePath);
        }
    }
}

QString FileManagerWidget::buildFilePathFromItem(QTreeWidgetItem *item)
{
    if (!item) return QString();
    
    QStringList pathParts;
    QTreeWidgetItem *currentItem = item;
    
    // Build path from bottom to top
    while (currentItem) {
        pathParts.prepend(currentItem->text(0));
        currentItem = currentItem->parent();
    }
    
    // Remove the root folder name and build absolute path
    if (pathParts.size() > 1) {
        pathParts.removeFirst(); // Remove root folder name
        return lastOpenedFolder + "/" + pathParts.join("/");
    }
    
    return QString();
}

void FileManagerWidget::loadFileContent(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        fileViewer->setPlainText(content);
        file.close();
        
        // Update current file path
        currentFilePath = filePath;
        
        qDebug() << "📁 Loaded file:" << QFileInfo(filePath).fileName();
    } else {
        fileViewer->setPlainText(QString("Error: Could not read file %1").arg(filePath));
    }
}


QString FileManagerWidget::getLastUsedDirectory() const
{
    QSettings settings;
    QString lastDir = settings.value("filemanager/lastUsedDirectory", 
                                   QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).toString();
    
    // Verify the directory still exists
    if (QDir(lastDir).exists()) {
        return lastDir;
    } else {
        // Fall back to home directory if saved directory no longer exists
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
}

void FileManagerWidget::setLastUsedDirectory(const QString &directory)
{
    if (!directory.isEmpty() && QDir(directory).exists()) {
        QSettings settings;
        settings.setValue("filemanager/lastUsedDirectory", directory);
        settings.sync(); // Ensure the setting is written immediately
    }
}
