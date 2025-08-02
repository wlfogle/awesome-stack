#ifndef MONITORINGCLASS_H
#define MONITORINGCLASS_H

#include <string>

class MonitoringClass {
public:
    MonitoringClass(const std::string &logFile, const std::string &dataDir);
    ~MonitoringClass();

    void startMonitoring();
    void stopMonitoring();

private:
    void logMessage(const std::string &message);
    void monitorPackages();
    void monitorConfigs();
    void monitorResources();
    void monitorServices();
    void checkBackupSuggestions();

    std::string logFile;
    std::string dataDir;
    bool running;
};

#endif // MONITORINGCLASS_H

