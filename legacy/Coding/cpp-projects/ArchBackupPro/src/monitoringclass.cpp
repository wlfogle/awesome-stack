#include "monitoringclass.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <filesystem>

MonitoringClass::MonitoringClass(const std::string &logFile, const std::string &dataDir)
    : logFile(logFile), dataDir(dataDir), running(false) {
    // Create necessary directories
    std::filesystem::create_directories(std::filesystem::path(logFile).parent_path());
    std::filesystem::create_directories(dataDir);
}

MonitoringClass::~MonitoringClass() {
    stopMonitoring();
}

void MonitoringClass::logMessage(const std::string &message) {
    auto now = std::time(nullptr);
    auto localTime = *std::localtime(&now);
    
    std::ofstream logFileStream(logFile, std::ios::app);
    if (logFileStream.is_open()) {
        logFileStream << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
        logFileStream.close();
    }
    
    // Also output to console
    std::cout << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
}

void MonitoringClass::startMonitoring() {
    running = true;
    logMessage("INFO: ArchBackupPro monitoring daemon started (PID: " + std::to_string(getpid()) + ")");
    logMessage("INFO: Logging to " + logFile);
    logMessage("INFO: Data directory: " + dataDir);
    
    while (running) {
        monitorPackages();
        monitorConfigs();
        monitorResources();
        monitorServices();
        checkBackupSuggestions();
        
        // Sleep for 5 minutes (300 seconds)
        std::this_thread::sleep_for(std::chrono::seconds(300));
    }
}

void MonitoringClass::stopMonitoring() {
    running = false;
    logMessage("INFO: Monitoring daemon shutting down");
}

void MonitoringClass::monitorPackages() {
    std::string command = "pacman -Q | sha256sum | cut -d' ' -f1";
    std::string currentHash;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, 128, pipe) != nullptr) {
            currentHash = std::string(buffer);
            currentHash.erase(currentHash.find_last_not_of(" \n\r\t") + 1);
        }
        pclose(pipe);
    }
    
    std::string storedHashFile = dataDir + "/packages.hash";
    std::ifstream hashFile(storedHashFile);
    std::string storedHash;
    
    if (hashFile.is_open()) {
        std::getline(hashFile, storedHash);
        hashFile.close();
        
        if (currentHash != storedHash) {
            logMessage("CHANGE: Package list has changed");
            
            // Check for recent installations
            std::string dateCmd = "pacman -Qi | grep -A1 \"Install Date\" | grep \"$(date '+%Y-%m-%d')\"";
            int result = system(dateCmd.c_str());
            if (result == 0) {
                logMessage("INFO: Recent package installation detected");
            }
        }
    }
    
    // Save current hash
    std::ofstream outFile(storedHashFile);
    if (outFile.is_open()) {
        outFile << currentHash;
        outFile.close();
    }
}

void MonitoringClass::monitorConfigs() {
    std::vector<std::string> configDirs = {"/etc", std::string(getenv("HOME")) + "/.config"};
    std::string changesFile = dataDir + "/config_changes.log";
    
    for (const auto& dir : configDirs) {
        if (std::filesystem::exists(dir)) {
            std::string findCmd = "find " + dir + " -type f -mmin -10 2>/dev/null | wc -l";
            
            FILE* pipe = popen(findCmd.c_str(), "r");
            if (pipe) {
                char buffer[128];
                int recentChanges = 0;
                if (fgets(buffer, 128, pipe) != nullptr) {
                    recentChanges = std::stoi(std::string(buffer));
                }
                pclose(pipe);
                
                if (recentChanges > 0) {
                    logMessage("CHANGE: " + std::to_string(recentChanges) + " configuration files modified in " + dir);
                    
                    // Log the actual changed files
                    std::string listCmd = "find " + dir + " -type f -mmin -10 2>/dev/null | head -5 >> " + changesFile;
                    system(listCmd.c_str());
                }
            }
        }
    }
}

void MonitoringClass::monitorResources() {
    // Monitor CPU usage
    std::string cpuCmd = "top -bn1 | grep \"Cpu(s)\" | awk '{print $2}' | sed 's/%us,//'";
    FILE* pipe = popen(cpuCmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, 128, pipe) != nullptr) {
            double cpuUsage = std::stod(std::string(buffer));
            if (cpuUsage > 80.0) {
                logMessage("WARNING: High CPU usage: " + std::to_string(cpuUsage) + "%");
            }
        }
        pclose(pipe);
    }
    
    // Monitor memory usage
    std::string memCmd = "free | grep Mem | awk '{printf \"%.1f\", ($3/$2) * 100.0}'";
    pipe = popen(memCmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, 128, pipe) != nullptr) {
            double memUsage = std::stod(std::string(buffer));
            if (memUsage > 80.0) {
                logMessage("WARNING: High memory usage: " + std::to_string(memUsage) + "%");
            }
        }
        pclose(pipe);
    }
    
    // Monitor disk usage
    std::string diskCmd = "df / | tail -1 | awk '{print $5}' | sed 's/%//'";
    pipe = popen(diskCmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, 128, pipe) != nullptr) {
            int diskUsage = std::stoi(std::string(buffer));
            if (diskUsage > 80) {
                logMessage("WARNING: High disk usage: " + std::to_string(diskUsage) + "%");
            }
        }
        pclose(pipe);
    }
}

void MonitoringClass::monitorServices() {
    std::string failedCmd = "systemctl --failed --no-legend | wc -l";
    FILE* pipe = popen(failedCmd.c_str(), "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, 128, pipe) != nullptr) {
            int failedServices = std::stoi(std::string(buffer));
            if (failedServices > 0) {
                logMessage("WARNING: " + std::to_string(failedServices) + " systemd services have failed");
                
                // List the failed services
                std::string listCmd = "systemctl --failed --no-legend | head -3";
                FILE* listPipe = popen(listCmd.c_str(), "r");
                if (listPipe) {
                    char lineBuffer[256];
                    while (fgets(lineBuffer, 256, listPipe) != nullptr) {
                        std::string line(lineBuffer);
                        line.erase(line.find_last_not_of(" \n\r\t") + 1);
                        logMessage("FAILED: " + line);
                    }
                    pclose(listPipe);
                }
            }
        }
        pclose(pipe);
    }
}

void MonitoringClass::checkBackupSuggestions() {
    std::string lastBackupFile = dataDir + "/last_backup.timestamp";
    std::time_t currentTime = std::time(nullptr);
    
    std::ifstream backupFile(lastBackupFile);
    if (backupFile.is_open()) {
        std::string timestampStr;
        std::getline(backupFile, timestampStr);
        backupFile.close();
        
        std::time_t lastBackup = std::stol(timestampStr);
        int daysSinceBackup = (currentTime - lastBackup) / 86400;
        
        if (daysSinceBackup > 7) {
            logMessage("SUGGESTION: Last backup was " + std::to_string(daysSinceBackup) + " days ago, consider running a backup");
        }
    } else {
        logMessage("SUGGESTION: No backup history found, consider running an initial backup");
        
        // Create the initial timestamp file
        std::ofstream outFile(lastBackupFile);
        if (outFile.is_open()) {
            outFile << std::to_string(currentTime);
            outFile.close();
        }
    }
}
