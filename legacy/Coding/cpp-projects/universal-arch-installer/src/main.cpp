#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QScreen>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLoggingCategory>
#include <QDebug>

#include "mainwindow.h"

// Enable logging
Q_LOGGING_CATEGORY(universalInstaller, "universal.installer")

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Universal Arch Installer");
    app.setApplicationVersion("2.0.0");
    app.setApplicationDisplayName("Universal Arch Linux Installer");
    app.setOrganizationName("Universal Installer");
    app.setOrganizationDomain("universal-installer.org");
    
    // Setup command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription("Universal Arch Linux Package Installer with GUI");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption themeOption({"t", "theme"}, 
        "Set application theme (dark, light, system)", "theme", "dark");
    parser.addOption(themeOption);
    
    QCommandLineOption verboseOption({"v", "verbose"}, "Enable verbose logging");
    parser.addOption(verboseOption);
    
    QCommandLineOption debugOption({"d", "debug"}, "Enable debug mode");
    parser.addOption(debugOption);
    
    parser.process(app);
    
    // Setup logging
    if (parser.isSet(verboseOption)) {
        QLoggingCategory::setFilterRules("universal.installer.debug=true");
    }
    
    if (parser.isSet(debugOption)) {
        QLoggingCategory::setFilterRules("*.debug=true");
    }
    
    qCDebug(universalInstaller) << "Starting Universal Arch Installer" << app.applicationVersion();
    
    // Check if running on Arch Linux
    if (!QDir("/etc/pacman.conf").exists()) {
        QMessageBox::warning(nullptr, "System Check", 
            "This application is designed for Arch Linux systems.\n"
            "Some features may not work correctly on other distributions.");
    }
    
    // Create configuration directory
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/universal-arch-installer";
    QDir().mkpath(configDir);
    
    // Setup application icon and theme
    app.setWindowIcon(QIcon(":/icons/universal-installer.png"));
    
    QString theme = parser.value(themeOption);
    if (theme == "dark") {
        app.setStyle(QStyleFactory::create("Fusion"));
        // Dark theme will be applied by MainWindow
    } else if (theme == "light") {
        app.setStyle(QStyleFactory::create("Fusion"));
    } else {
        // Use system default
        app.setStyle(QStyleFactory::create("Fusion"));
    }
    
    // Create and show main window
    MainWindow *window = new MainWindow();
    window->resize(1200, 800);  // Set a reasonable default size
    
    // Center window on screen
    QScreen *screen = app.primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = (screenGeometry.width() - window->width()) / 2;
        int y = (screenGeometry.height() - window->height()) / 2;
        window->move(x, y);
    }
    
    window->show();
    
    qCDebug(universalInstaller) << "Main window shown, entering event loop";
    
    // Run application
    int result = app.exec();
    
    qCDebug(universalInstaller) << "Application exiting with code:" << result;
    
    return result;
}
