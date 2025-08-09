#ifndef MONITORINGMANAGER_H
#define MONITORINGMANAGER_H

#include <string>

class MonitoringManager {
public:
    MonitoringManager();
    ~MonitoringManager();

    // Main method to ensure monitoring daemon is running
    bool ensureMonitoringDaemon();
    
    // Check if daemon is installed
    bool isDaemonInstalled();
    
    // Check if daemon is running
    bool isDaemonRunning();
    
    // Install the daemon
    bool installDaemon();
    
    // Start the daemon
    bool startDaemon();
    
    // Stop the daemon
    bool stopDaemon();
    
    // Get daemon status
    std::string getDaemonStatus();

private:
    bool executeCommand(const std::string& command);
    bool executeCommandWithOutput(const std::string& command, std::string& output);
    bool isRoot();
    bool createDirectories();
    bool copyDaemonExecutable();
    bool copyServiceFile();
    bool enableService();
    
    std::string m_executablePath;
    std::string m_servicePath;
    std::string m_installPath;
};

#endif // MONITORINGMANAGER_H
