#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QTextEdit>
#include <QFont>

class CodeEditor : public QTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);

private:
    void setupStyles();
};

#endif // CODEEDITOR_H
