#include <QApplication>
#include <QPalette>
#include <QColor>
#include <QStyle>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Open Interpreter GUI");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Local AI Tools");
    
    // Set dark theme
    app.setStyle("Fusion");
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(30, 30, 30));
    palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
    palette.setColor(QPalette::Base, QColor(15, 15, 15));
    palette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
    palette.setColor(QPalette::ToolTipBase, QColor(15, 15, 15));
    palette.setColor(QPalette::ToolTipText, QColor(220, 220, 220));
    palette.setColor(QPalette::Text, QColor(220, 220, 220));
    palette.setColor(QPalette::Button, QColor(30, 30, 30));
    palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
    palette.setColor(QPalette::BrightText, QColor(255, 0, 0));
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    app.setPalette(palette);
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
