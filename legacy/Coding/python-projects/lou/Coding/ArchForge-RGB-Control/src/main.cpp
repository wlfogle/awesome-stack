#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QPalette>
#include "mainwindow.h"

/**
 * @brief ArchForge RGB Control Application
 * 
 * Enhanced RGB and system control application for OriginPC laptops
 * with integrated fan control, power management, and monitoring.
 */

Q_LOGGING_CATEGORY(appLog, "archforge.main")

void setupLogging()
{
    // Setup logging to file for debugging
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logPath);
    
    static QFile debugFile(logPath + "/archforge-rgb.log");
    if (debugFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &context, const QString &msg) {
            static QTextStream stream(&debugFile);
            stream << QDateTime::currentDateTime().toString(Qt::ISODate) 
                   << " [" << type << "] " << msg << Qt::endl;
            stream.flush();
            
            // Also output to console
            QTextStream console(type == QtDebugMsg ? stdout : stderr);
            console << msg << Qt::endl;
        });
    }
}

void setupApplication(QApplication &app)
{
    // Application metadata
    app.setApplicationName("ArchForge RGB Control");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("ArchForge RGB Control Center");
    app.setOrganizationName("ArchForge");
    app.setOrganizationDomain("archforge.dev");
    
    // High DPI support (automatic in Qt6)
    
    // Set a modern style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Dark theme palette
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
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Setup logging and application
    setupLogging();
    setupApplication(app);
    
    qCInfo(appLog) << "Starting ArchForge RGB Control Center";
    qCInfo(appLog) << "Qt Version:" << QT_VERSION_STR;
    qCInfo(appLog) << "Build Date:" << __DATE__ << __TIME__;
    
    // Check for RGB device permissions
    QFile rgbDevice("/dev/hidraw1");
    if (!rgbDevice.exists()) {
        qCWarning(appLog) << "RGB device /dev/hidraw1 not found";
    } else if (!rgbDevice.open(QIODevice::ReadWrite)) {
        qCWarning(appLog) << "Insufficient permissions for RGB device";
        qCInfo(appLog) << "Run: sudo chmod 666 /dev/hidraw1";
    } else {
        qCInfo(appLog) << "RGB device accessible";
        rgbDevice.close();
    }
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    qCInfo(appLog) << "Application window created and shown";
    
    // Run the application
    int result = app.exec();
    
    qCInfo(appLog) << "Application exiting with code:" << result;
    return result;
}
