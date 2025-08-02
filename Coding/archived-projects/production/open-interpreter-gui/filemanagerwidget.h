#ifndef FILEMANAGERWIDGET_H
#define FILEMANAGERWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include "codeeditor.h"

class FileManagerWidget : public QWidget {
    Q_OBJECT

public:
    explicit FileManagerWidget(QWidget *parent = nullptr);

private slots:
    void onOpenFileClicked();
    void onOpenFolderClicked();
    void onFileItemSelectionChanged();

private:
    QTreeWidget *fileTree;
    CodeEditor *fileViewer;
    QPushButton *openFileButton;
    QPushButton *openFolderButton;

private:
    void setupUi();
    void populateFileTree(const QString &folderPath);
    void addItemsToTree(QTreeWidgetItem *parentItem, const QString &path);

signals:
    void folderOpened(const QString &folderPath);
};

#endif // FILEMANAGERWIDGET_H
