#include <QApplication>
#include "mainwindow.h"
#include "monitoringmanager.h"
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("ArchBackupPro");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ArchForge");
    
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
        // You can choose to continue without monitoring or exit
        // For now, we'll continue without monitoring
    } else {
        std::cout << "Monitoring daemon is active. Real-time system monitoring enabled." << std::endl;
        
        // Optionally show daemon status
        std::cout << "\nDaemon Status:" << std::endl;
        std::cout << monitoringManager.getDaemonStatus() << std::endl;
    }
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
