#include "monitoringclass.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstdlib>
#include <string>

const std::string LOG_FILE = "/var/log/archbackuppro/monitor.log";
const std::string DATA_DIR = "/var/lib/archbackuppro";
const std::string PID_FILE = "/run/archbackuppro/monitor.pid";

MonitoringClass* monitorInstance = nullptr;

void signalHandler(int signal) {
    if (monitorInstance) {
        monitorInstance->stopMonitoring();
        std::cout << "Received signal " << signal << ", shutting down monitoring daemon." << std::endl;
    }
    
    // Remove PID file
    unlink(PID_FILE.c_str());
    exit(0);
}

bool checkExistingInstance() {
    std::ifstream pidFile(PID_FILE);
    if (pidFile.is_open()) {
        std::string pidStr;
        std::getline(pidFile, pidStr);
        pidFile.close();
        
        if (!pidStr.empty()) {
            pid_t existingPid = std::stoi(pidStr);
            if (kill(existingPid, 0) == 0) {
                std::cerr << "ERROR: Another instance is already running (PID: " << existingPid << ")" << std::endl;
                return false;
            } else {
                std::cout << "WARNING: Stale PID file found, removing it" << std::endl;
                unlink(PID_FILE.c_str());
            }
        }
    }
    return true;
}

void writePidFile() {
    // Create directory if it doesn't exist
    std::string pidDir = PID_FILE.substr(0, PID_FILE.find_last_of('/'));
    mkdir(pidDir.c_str(), 0755);
    
    std::ofstream pidFile(PID_FILE);
    if (pidFile.is_open()) {
        pidFile << getpid();
        pidFile.close();
    } else {
        std::cerr << "ERROR: Could not create PID file: " << PID_FILE << std::endl;
    }
}

void daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << "Fork failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        // Parent process
        exit(EXIT_SUCCESS);
    }
    
    // Child process becomes session leader
    if (setsid() < 0) {
        std::cerr << "setsid failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Fork again to prevent acquiring a controlling terminal
    pid = fork();
    
    if (pid < 0) {
        std::cerr << "Second fork failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        // First child process
        exit(EXIT_SUCCESS);
    }
    
    // Set umask for file creation
    umask(0);
    
    // Change working directory to root
    if (chdir("/") < 0) {
        std::cerr << "chdir failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -d, --daemon    Run as daemon (default)" << std::endl;
    std::cout << "  -f, --foreground    Run in foreground" << std::endl;
    std::cout << "  -h, --help      Show this help message" << std::endl;
    std::cout << "  -v, --version   Show version information" << std::endl;
}

int main(int argc, char* argv[]) {
    bool runAsDaemon = true;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-f" || arg == "--foreground") {
            runAsDaemon = false;
        } else if (arg == "-d" || arg == "--daemon") {
            runAsDaemon = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "ArchBackupPro Monitoring Daemon v1.0.0" << std::endl;
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Check for existing instance
    if (!checkExistingInstance()) {
        return 1;
    }
    
    // Set up signal handlers
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);
    
    // Daemonize if requested
    if (runAsDaemon) {
        daemonize();
    }
    
    // Write PID file
    writePidFile();
    
    // Create monitoring instance
    monitorInstance = new MonitoringClass(LOG_FILE, DATA_DIR);
    
    try {
        // Start monitoring
        monitorInstance->startMonitoring();
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        delete monitorInstance;
        unlink(PID_FILE.c_str());
        return 1;
    }
    
    // Cleanup
    delete monitorInstance;
    unlink(PID_FILE.c_str());
    
    return 0;
}
