#include "powermanager.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>

PowerManager::PowerManager(QObject *parent)
    : QObject(parent)
    , m_currentProfile(PowerProfile::Balanced)
    , m_available(false)
    , m_hasCpufreq(false)
    , m_hasPowerProfiles(false)
    , m_batteryTimer(new QTimer(this))
    , m_lastBatteryLevel(-1)
    , m_lastACStatus(false)
{
    detectAvailableMethods();
    startBatteryMonitoring();
}

void PowerManager::detectAvailableMethods()
{
    // Check for power-profiles-daemon (GNOME/systemd)
    QProcess ppd;
    ppd.start("powerprofilesctl", QStringList() << "list");
    ppd.waitForFinished(2000);
    if (ppd.exitCode() == 0) {
        m_hasPowerProfiles = true;
        qDebug() << "Power profiles daemon detected";
    }
    
    // Check for cpufreq userspace access
    QDir cpufreqDir("/sys/devices/system/cpu/cpu0/cpufreq");
    if (cpufreqDir.exists()) {
        QFile scalingGov("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (scalingGov.exists()) {
            m_hasCpufreq = true;
            
            // Get available governors
            QFile availableGov("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors");
            if (availableGov.open(QIODevice::ReadOnly)) {
                QString governors = availableGov.readAll().trimmed();
                m_availableGovernors = governors.split(' ', Qt::SkipEmptyParts);
                qDebug() << "Available CPU governors:" << m_availableGovernors;
            }
        }
    }
    
    m_available = m_hasPowerProfiles || m_hasCpufreq;
    
    if (!m_available) {
        qWarning() << "No power management methods available";
        emit error("No power management system detected");
    }
}

bool PowerManager::setPowerProfile(PowerProfile profile)
{
    bool success = false;
    
    if (m_hasPowerProfiles) {
        success = setPowerProfileUserspace(profile);
    } else if (m_hasCpufreq) {
        success = setPowerProfileCpufreq(profile);
    }
    
    if (success) {
        m_currentProfile = profile;
        emit powerProfileChanged(profile);
        qDebug() << "Power profile changed to:" << powerProfileToString(profile);
    } else {
        emit error("Failed to set power profile");
        qWarning() << "Failed to set power profile to:" << powerProfileToString(profile);
    }
    
    return success;
}

bool PowerManager::setPowerProfileUserspace(PowerProfile profile)
{
    QString profileName;
    switch (profile) {
        case PowerProfile::Performance:
            profileName = "performance";
            break;
        case PowerProfile::Balanced:
            profileName = "balanced";
            break;
        case PowerProfile::PowerSave:
            profileName = "power-saver";
            break;
    }
    
    QProcess ppd;
    ppd.start("powerprofilesctl", QStringList() << "set" << profileName);
    ppd.waitForFinished(3000);
    
    if (ppd.exitCode() == 0) {
        qDebug() << "Power profile set via powerprofilesctl:" << profileName;
        return true;
    } else {
        qWarning() << "powerprofilesctl failed:" << ppd.readAllStandardError();
        return false;
    }
}

bool PowerManager::setPowerProfileCpufreq(PowerProfile profile)
{
    QString governor = getGovernorForProfile(profile);
    
    if (!m_availableGovernors.contains(governor)) {
        // Fallback to available governors
        if (m_availableGovernors.contains("ondemand")) {
            governor = "ondemand";
        } else if (m_availableGovernors.contains("schedutil")) {
            governor = "schedutil";
        } else if (!m_availableGovernors.isEmpty()) {
            governor = m_availableGovernors.first();
        } else {
            return false;
        }
    }
    
    // Try using cpupower if available (may work without sudo for reading)
    QProcess cpupower;
    cpupower.start("cpupower", QStringList() << "frequency-info" << "-g");
    cpupower.waitForFinished(2000);
    
    if (cpupower.exitCode() == 0) {
        // cpupower is available, try to use it for frequency scaling
        QProcess setCpupower;
        setCpupower.start("cpupower", QStringList() << "frequency-set" << "-g" << governor);
        setCpupower.waitForFinished(3000);
        
        if (setCpupower.exitCode() == 0) {
            qDebug() << "CPU governor set via cpupower:" << governor;
            return true;
        }
    }
    
    // Try userspace method - write to sysfs (may require permissions)
    QString cpuPath = "/sys/devices/system/cpu/cpu%1/cpufreq/scaling_governor";
    bool anySuccess = false;
    
    QDir cpuDir("/sys/devices/system/cpu");
    QStringList cpuDirs = cpuDir.entryList(QStringList() << "cpu[0-9]*", QDir::Dirs);
    
    for (const QString &cpu : cpuDirs) {
        QString govFile = QString("/sys/devices/system/cpu/%1/cpufreq/scaling_governor").arg(cpu);
        QFile file(govFile);
        
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << governor;
            file.close();
            anySuccess = true;
        }
    }
    
    if (anySuccess) {
        qDebug() << "CPU governor set via sysfs:" << governor;
        return true;
    }
    
    qWarning() << "Failed to set CPU governor, may need elevated permissions";
    return false;
}

PowerProfile PowerManager::currentProfile() const
{
    return m_currentProfile;
}

bool PowerManager::isAvailable() const
{
    return m_available;
}

int PowerManager::batteryLevel() const
{
    QDir powerSupplyDir("/sys/class/power_supply");
    QStringList batteries = powerSupplyDir.entryList(QStringList() << "BAT*", QDir::Dirs);
    
    if (batteries.isEmpty()) {
        return -1; // No battery found
    }
    
    QString batteryPath = "/sys/class/power_supply/" + batteries.first();
    
    QFile capacityFile(batteryPath + "/capacity");
    if (capacityFile.open(QIODevice::ReadOnly)) {
        QString capacity = capacityFile.readAll().trimmed();
        return capacity.toInt();
    }
    
    return -1;
}

bool PowerManager::isOnACPower() const
{
    QDir powerSupplyDir("/sys/class/power_supply");
    QStringList adapters = powerSupplyDir.entryList(QStringList() << "A[CD]*", QDir::Dirs);
    
    for (const QString &adapter : adapters) {
        QString adapterPath = "/sys/class/power_supply/" + adapter;
        QFile onlineFile(adapterPath + "/online");
        
        if (onlineFile.open(QIODevice::ReadOnly)) {
            QString online = onlineFile.readAll().trimmed();
            if (online == "1") {
                return true;
            }
        }
    }
    
    return false;
}

QString PowerManager::batteryStatus() const
{
    QDir powerSupplyDir("/sys/class/power_supply");
    QStringList batteries = powerSupplyDir.entryList(QStringList() << "BAT*", QDir::Dirs);
    
    if (batteries.isEmpty()) {
        return "No battery";
    }
    
    QString batteryPath = "/sys/class/power_supply/" + batteries.first();
    
    QFile statusFile(batteryPath + "/status");
    if (statusFile.open(QIODevice::ReadOnly)) {
        return statusFile.readAll().trimmed();
    }
    
    return "Unknown";
}

QStringList PowerManager::getCPUFrequencyInfo()
{
    QStringList info;
    
    // Try to get CPU frequency information
    QProcess lscpu;
    lscpu.start("lscpu");
    lscpu.waitForFinished(2000);
    
    if (lscpu.exitCode() == 0) {
        QString output = lscpu.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        for (const QString &line : lines) {
            if (line.contains("CPU MHz") || line.contains("CPU max MHz") || 
                line.contains("CPU min MHz") || line.contains("CPU(s)")) {
                info.append(line.trimmed());
            }
        }
    }
    
    // Add current governor
    QString governor = getCurrentGovernor();
    if (!governor.isEmpty()) {
        info.append(QString("Current Governor: %1").arg(governor));
    }
    
    return info;
}

QString PowerManager::getCurrentGovernor()
{
    QFile govFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
    if (govFile.open(QIODevice::ReadOnly)) {
        return govFile.readAll().trimmed();
    }
    
    return QString();
}

QStringList PowerManager::getAvailableGovernors()
{
    return m_availableGovernors;
}

void PowerManager::startBatteryMonitoring()
{
    m_batteryTimer->setInterval(10000); // Update every 10 seconds
    connect(m_batteryTimer, &QTimer::timeout, this, &PowerManager::updateBatteryStatus);
    m_batteryTimer->start();
    
    // Initial update
    updateBatteryStatus();
}

void PowerManager::updateBatteryStatus()
{
    int currentLevel = batteryLevel();
    bool currentACStatus = isOnACPower();
    
    if (currentLevel != m_lastBatteryLevel && currentLevel >= 0) {
        m_lastBatteryLevel = currentLevel;
        emit batteryLevelChanged(currentLevel);
    }
    
    if (currentACStatus != m_lastACStatus) {
        m_lastACStatus = currentACStatus;
        emit powerSourceChanged(currentACStatus);
    }
}

QString PowerManager::powerProfileToString(PowerProfile profile) const
{
    switch (profile) {
        case PowerProfile::Performance:
            return "Performance";
        case PowerProfile::Balanced:
            return "Balanced";
        case PowerProfile::PowerSave:
            return "Power Save";
    }
    return "Unknown";
}

QString PowerManager::getGovernorForProfile(PowerProfile profile) const
{
    switch (profile) {
        case PowerProfile::Performance:
            return "performance";
        case PowerProfile::Balanced:
            return "ondemand";
        case PowerProfile::PowerSave:
            return "powersave";
    }
    return "ondemand";
}
