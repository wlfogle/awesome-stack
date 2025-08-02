#include "monitoringmanager.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <sstream>

MonitoringManager::MonitoringManager() {
    // Determine the path of the current executable directory
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        std::string execPath(buffer);
        std::filesystem::path execDir = std::filesystem::path(execPath).parent_path();
        
        // Look for daemon executable in various locations
        std::vector<std::string> possiblePaths = {
            execDir / "archbackuppro-monitoring-daemon",
            execDir / "bin" / "archbackuppro-monitoring-daemon",
            execDir / ".." / "build-monitoring" / "bin" / "archbackuppro-monitoring-daemon",
            execDir / ".." / "src" / "build-monitoring" / "bin" / "archbackuppro-monitoring-daemon"
        };
        
        for (const auto& path : possiblePaths) {
            if (std::filesystem::exists(path)) {
                m_executablePath = path;
                break;
            }
        }
        
        // Look for service file
        std::vector<std::string> servicePaths = {
            execDir / "archbackuppro-monitoring-daemon.service",
            execDir / ".." / "archbackuppro-monitoring-daemon.service"
        };
        
        for (const auto& path : servicePaths) {
            if (std::filesystem::exists(path)) {
                m_servicePath = path;
                break;
            }
        }
    }
    
    m_installPath = "/usr/local/bin/archbackuppro-monitoring-daemon";
}

MonitoringManager::~MonitoringManager() {
    // Nothing to cleanup
}

bool MonitoringManager::ensureMonitoringDaemon() {
    std::cout << "Checking monitoring daemon status..." << std::endl;
    
    // Check if daemon is installed
    if (!isDaemonInstalled()) {
        std::cout << "Monitoring daemon not found. Installing..." << std::endl;
        if (!installDaemon()) {
            std::cerr << "Failed to install monitoring daemon!" << std::endl;
            return false;
        }
        std::cout << "Monitoring daemon installed successfully." << std::endl;
    } else {
        std::cout << "Monitoring daemon is already installed." << std::endl;
    }
    
    // Check if daemon is running
    if (!isDaemonRunning()) {
        std::cout << "Starting monitoring daemon..." << std::endl;
        if (!startDaemon()) {
            std::cerr << "Failed to start monitoring daemon!" << std::endl;
            return false;
        }
        std::cout << "Monitoring daemon started successfully." << std::endl;
    } else {
        std::cout << "Monitoring daemon is already running." << std::endl;
    }
    
    return true;
}

bool MonitoringManager::isDaemonInstalled() {
    return std::filesystem::exists(m_installPath) && 
           std::filesystem::exists("/etc/systemd/system/archbackuppro-monitoring-daemon.service");
}

bool MonitoringManager::isDaemonRunning() {
    std::string output;
    bool result = executeCommandWithOutput("systemctl is-active archbackuppro-monitoring-daemon 2>/dev/null", output);
    return result && output.find("active") != std::string::npos;
}

bool MonitoringManager::installDaemon() {
    if (!isRoot()) {
        std::cerr << "Root privileges required for daemon installation. Attempting with sudo..." << std::endl;
        
        // Try to use sudo for installation
        std::string sudoCommand = "sudo bash -c '";
        
        // Create installation script content
        std::stringstream installScript;
        installScript << "set -e; ";
        installScript << "mkdir -p /var/log/archbackuppro /var/lib/archbackuppro; ";
        installScript << "mkdir -p /run/archbackuppro; ";
        
        if (!m_executablePath.empty()) {
            installScript << "cp \"" << m_executablePath << "\" /usr/local/bin/; ";
            installScript << "chmod +x /usr/local/bin/archbackuppro-monitoring-daemon; ";
        }
        
        if (!m_servicePath.empty()) {
            installScript << "cp \"" << m_servicePath << "\" /etc/systemd/system/; ";
            installScript << "chmod 644 /etc/systemd/system/archbackuppro-monitoring-daemon.service; ";
        }
        
        installScript << "systemctl daemon-reload; ";
        installScript << "systemctl enable archbackuppro-monitoring-daemon; ";
        installScript << "echo Installation completed successfully";
        
        sudoCommand += installScript.str() + "'";
        
        std::cout << "Executing installation with sudo..." << std::endl;
        return executeCommand(sudoCommand);
    }
    
    // If running as root, install directly
    if (!createDirectories()) return false;
    if (!copyDaemonExecutable()) return false;
    if (!copyServiceFile()) return false;
    if (!enableService()) return false;
    
    return true;
}

bool MonitoringManager::startDaemon() {
    if (isRoot()) {
        return executeCommand("systemctl start archbackuppro-monitoring-daemon");
    } else {
        return executeCommand("sudo systemctl start archbackuppro-monitoring-daemon");
    }
}

bool MonitoringManager::stopDaemon() {
    if (isRoot()) {
        return executeCommand("systemctl stop archbackuppro-monitoring-daemon");
    } else {
        return executeCommand("sudo systemctl stop archbackuppro-monitoring-daemon");
    }
}

std::string MonitoringManager::getDaemonStatus() {
    std::string output;
    if (executeCommandWithOutput("systemctl status archbackuppro-monitoring-daemon --no-pager", output)) {
        return output;
    }
    return "Failed to get daemon status";
}

bool MonitoringManager::executeCommand(const std::string& command) {
    int result = system(command.c_str());
    return result == 0;
}

bool MonitoringManager::executeCommandWithOutput(const std::string& command, std::string& output) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    
    char buffer[128];
    output.clear();
    while (fgets(buffer, 128, pipe) != nullptr) {
        output += buffer;
    }
    
    int result = pclose(pipe);
    return result == 0;
}

bool MonitoringManager::isRoot() {
    return getuid() == 0;
}

bool MonitoringManager::createDirectories() {
    try {
        std::filesystem::create_directories("/var/log/archbackuppro");
        std::filesystem::create_directories("/var/lib/archbackuppro");
        std::filesystem::create_directories("/run/archbackuppro");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directories: " << e.what() << std::endl;
        return false;
    }
}

bool MonitoringManager::copyDaemonExecutable() {
    if (m_executablePath.empty()) {
        std::cerr << "Daemon executable not found!" << std::endl;
        return false;
    }
    
    try {
        std::filesystem::copy_file(m_executablePath, m_installPath, 
                                 std::filesystem::copy_options::overwrite_existing);
        std::filesystem::permissions(m_installPath, 
                                   std::filesystem::perms::owner_all | 
                                   std::filesystem::perms::group_read | 
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read | 
                                   std::filesystem::perms::others_exec);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy daemon executable: " << e.what() << std::endl;
        return false;
    }
}

bool MonitoringManager::copyServiceFile() {
    if (m_servicePath.empty()) {
        std::cerr << "Service file not found!" << std::endl;
        return false;
    }
    
    try {
        std::filesystem::copy_file(m_servicePath, "/etc/systemd/system/archbackuppro-monitoring-daemon.service",
                                 std::filesystem::copy_options::overwrite_existing);
        std::filesystem::permissions("/etc/systemd/system/archbackuppro-monitoring-daemon.service",
                                   std::filesystem::perms::owner_read | 
                                   std::filesystem::perms::owner_write |
                                   std::filesystem::perms::group_read |
                                   std::filesystem::perms::others_read);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy service file: " << e.what() << std::endl;
        return false;
    }
}

bool MonitoringManager::enableService() {
    return executeCommand("systemctl daemon-reload") && 
           executeCommand("systemctl enable archbackuppro-monitoring-daemon");
}
