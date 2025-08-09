#include "codeeditor.h"

CodeEditor::CodeEditor(QWidget *parent)
    : QTextEdit(parent)
{
    setFont(QFont("Consolas", 10));
    setupStyles();
}

void CodeEditor::setupStyles()
{
    setStyleSheet(
        "QTextEdit {"
        "    background-color: #0f0f0f;"
        "    color: #e0e0e0;"
        "    border: 1px solid #333333;"
        "    border-radius: 5px;"
        "    padding: 5px;"
        "}"
    );
}
