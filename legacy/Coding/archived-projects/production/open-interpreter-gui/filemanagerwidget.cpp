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
    
    // File content viewer
    fileViewer = new CodeEditor();
    fileViewer->setReadOnly(true);
    layout->addWidget(fileViewer);
    
    // Connections
    connect(openFileButton, &QPushButton::clicked, this, &FileManagerWidget::onOpenFileClicked);
    connect(openFolderButton, &QPushButton::clicked, this, &FileManagerWidget::onOpenFolderClicked);
    connect(fileTree, &QTreeWidget::itemSelectionChanged, this, &FileManagerWidget::onFileItemSelectionChanged);
}

void FileManagerWidget::onOpenFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", QString(), "All Files (*)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            fileViewer->setPlainText(in.readAll());
            file.close();

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
    QString folderPath = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (!folderPath.isEmpty()) {
        populateFileTree(folderPath);
        emit folderOpened(folderPath);
    }
}

void FileManagerWidget::populateFileTree(const QString &folderPath)
{
    fileTree->clear();
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
    // This function handles file tree item selection changes
    // For now, we'll leave it empty but it's required for compilation
}
