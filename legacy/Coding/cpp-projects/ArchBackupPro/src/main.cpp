#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include "mainwindow.h"
#include "monitoringmanager.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    QApplication::setApplicationName("ArchBackupPro");
    QApplication::setApplicationVersion("0.0.1");
    QApplication::setApplicationDisplayName("ArchBackupPro - Comprehensive Backup Solution");
    QApplication::setOrganizationName("ArchBackupPro");
    QApplication::setOrganizationDomain("archbackuppro.org");
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Comprehensive backup and restore solution for Arch Linux with AI enhancements");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption minimizedOption(QStringList() << "m" << "minimized",
                                      "Start minimized to system tray");
    parser.addOption(minimizedOption);
    
    QCommandLineOption backupOption(QStringList() << "b" << "backup",
                                   "Start backup immediately", "type", "incremental");
    parser.addOption(backupOption);
    
    QCommandLineOption locationOption(QStringList() << "l" << "location",
                                     "Backup location", "path");
    parser.addOption(locationOption);
    
    parser.process(app);
    
    // Check for system requirements (Arch Linux or Arch-based distributions)
    if (!QFile("/etc/pacman.conf").exists()) {
        QMessageBox::critical(nullptr, "System Requirements", 
                            "ArchBackupPro requires Arch Linux or an Arch-based distribution with pacman package manager.\n"
                            "This system does not appear to have pacman installed.");
        return 1;
    }
    
    // Create backup directory if it doesn't exist
    QString defaultBackupDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/ArchBackups";
    if (!QDir().mkpath(defaultBackupDir)) {
        QMessageBox::warning(nullptr, "Directory Creation", 
                           "Could not create default backup directory: " + defaultBackupDir);
    }
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Apply dark theme
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    // Initialize monitoring manager and ensure daemon is running
    std::cout << "Initializing ArchBackupPro monitoring system..." << std::endl;
    
    MonitoringManager monitoringManager;
    
    // This will automatically:
    // 1. Check if daemon is installed
    // 2. Install it if not present (using sudo if needed)
    // 3. Start the daemon if not running
    if (!monitoringManager.ensureMonitoringDaemon()) {
        std::cerr << "Warning: Failed to initialize monitoring daemon. "
                  << "Real-time monitoring will not be available." << std::endl;
        // Continue without monitoring
    } else {
        std::cout << "Monitoring daemon is active. Real-time system monitoring enabled." << std::endl;
    }
    
    // Create and show main window
    MainWindow window;
    
    // Handle command line options
    if (parser.isSet(minimizedOption)) {
        // Start minimized but don't show the window
        window.hide();
    } else {
        window.show();
    }
    
    // Handle immediate backup request
    if (parser.isSet(backupOption)) {
        QString backupType = parser.value(backupOption);
        if (backupType == "full") {
            QMetaObject::invokeMethod(&window, "startFullBackup", Qt::QueuedConnection);
        } else if (backupType == "incremental") {
            QMetaObject::invokeMethod(&window, "startIncrementalBackup", Qt::QueuedConnection);
        } else if (backupType == "packages") {
            QMetaObject::invokeMethod(&window, "startPackageBackup", Qt::QueuedConnection);
        } else if (backupType == "settings") {
            QMetaObject::invokeMethod(&window, "startSettingsBackup", Qt::QueuedConnection);
        }
    }
    
    return app.exec();
}
