#include "fancontroller.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

FanController::FanController(QObject *parent)
    : QObject(parent)
    , m_currentMode(FanMode::Auto)
    , m_nbfcAvailable(false)
    , m_fancontrolAvailable(false)
{
    detectFanControlMethods();
}

void FanController::detectFanControlMethods()
{
    // Check for NBFC
    QProcess nbfcCheck;
    nbfcCheck.start("which", QStringList() << "nbfc");
    nbfcCheck.waitForFinished(1000);
    if (nbfcCheck.exitCode() == 0) {
        m_nbfcAvailable = true;
        qDebug() << "NBFC detected and available";
    }
    
    // Check for fancontrol
    QProcess fancontrolCheck;
    fancontrolCheck.start("which", QStringList() << "fancontrol");
    fancontrolCheck.waitForFinished(1000);
    if (fancontrolCheck.exitCode() == 0) {
        m_fancontrolAvailable = true;
        qDebug() << "fancontrol detected and available";
    }
    
    // Check for direct PWM control
    checkDirectPWMControl();
    
    if (!m_nbfcAvailable && !m_fancontrolAvailable && m_pwmDevices.isEmpty()) {
        qWarning() << "No fan control methods available";
        emit error("No fan control system detected");
    }
}

void FanController::checkDirectPWMControl()
{
    // Look for PWM devices in /sys/class/hwmon
    QDir hwmonDir("/sys/class/hwmon");
    if (!hwmonDir.exists()) {
        return;
    }
    
    QStringList hwmonDirs = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &hwmonName : hwmonDirs) {
        QString hwmonPath = "/sys/class/hwmon/" + hwmonName;
        
        // Look for PWM files
        QDir deviceDir(hwmonPath);
        QStringList pwmFiles = deviceDir.entryList(QStringList() << "pwm*", QDir::Files);
        
        for (const QString &pwmFile : pwmFiles) {
            if (pwmFile.startsWith("pwm") && !pwmFile.contains("_")) {
                QString pwmPath = hwmonPath + "/" + pwmFile;
                m_pwmDevices.append(pwmPath);
                qDebug() << "Found PWM device:" << pwmPath;
            }
        }
    }
}

bool FanController::setFanMode(FanMode mode)
{
    bool success = false;
    
    if (m_nbfcAvailable) {
        success = setFanModeNBFC(mode);
    } else if (m_fancontrolAvailable) {
        success = setFanModeFancontrol(mode);
    } else if (!m_pwmDevices.isEmpty()) {
        success = setFanModeDirect(mode);
    }
    
    if (success) {
        m_currentMode = mode;
        emit fanModeChanged(mode);
        qDebug() << "Fan mode changed to:" << static_cast<int>(mode);
    } else {
        emit error("Failed to set fan mode");
        qWarning() << "Failed to set fan mode to:" << static_cast<int>(mode);
    }
    
    return success;
}

bool FanController::setFanModeNBFC(FanMode mode)
{
    QString nbfcMode;
    switch (mode) {
        case FanMode::Silent:
            nbfcMode = "silent";
            break;
        case FanMode::Auto:
            nbfcMode = "auto";
            break;
        case FanMode::Performance:
            nbfcMode = "performance";
            break;
        default:
            return false;
    }
    
    QProcess nbfcProcess;
    nbfcProcess.start("nbfc", QStringList() << "set" << "-a" << nbfcMode);
    nbfcProcess.waitForFinished(3000);
    
    if (nbfcProcess.exitCode() == 0) {
        qDebug() << "NBFC mode set to:" << nbfcMode;
        return true;
    } else {
        qWarning() << "NBFC failed:" << nbfcProcess.readAllStandardError();
        return false;
    }
}

bool FanController::setFanModeFancontrol(FanMode mode)
{
    // fancontrol typically uses configuration files
    // This is a simplified implementation
    
    QString configPath = "/etc/fancontrol";
    if (!QFile::exists(configPath)) {
        qWarning() << "fancontrol config not found at" << configPath;
        return false;
    }
    
    // For now, just restart fancontrol service
    QProcess systemctl;
    systemctl.start("systemctl", QStringList() << "restart" << "fancontrol");
    systemctl.waitForFinished(5000);
    
    return systemctl.exitCode() == 0;
}

bool FanController::setFanModeDirect(FanMode mode)
{
    int pwmValue;
    switch (mode) {
        case FanMode::Silent:
            pwmValue = 100; // Low speed
            break;
        case FanMode::Auto:
            pwmValue = 150; // Medium speed
            break;
        case FanMode::Performance:
            pwmValue = 255; // Full speed
            break;
        default:
            return false;
    }
    
    bool anySuccess = false;
    for (const QString &pwmDevice : m_pwmDevices) {
        QFile pwmFile(pwmDevice);
        if (pwmFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&pwmFile);
            out << pwmValue;
            pwmFile.close();
            anySuccess = true;
            qDebug() << "Set PWM device" << pwmDevice << "to" << pwmValue;
        } else {
            qWarning() << "Failed to write to PWM device:" << pwmDevice;
        }
    }
    
    return anySuccess;
}

QList<FanInfo> FanController::getFanInfo()
{
    QList<FanInfo> fans;
    
    // Look for fan RPM sensors
    QDir hwmonDir("/sys/class/hwmon");
    if (!hwmonDir.exists()) {
        return fans;
    }
    
    QStringList hwmonDirs = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &hwmonName : hwmonDirs) {
        QString hwmonPath = "/sys/class/hwmon/" + hwmonName;
        
        // Look for fan input files
        QDir deviceDir(hwmonPath);
        QStringList fanFiles = deviceDir.entryList(QStringList() << "fan*_input", QDir::Files);
        
        for (const QString &fanFile : fanFiles) {
            QString fanPath = hwmonPath + "/" + fanFile;
            QFile file(fanPath);
            
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString rpmText = in.readAll().trimmed();
                bool ok;
                int rpm = rpmText.toInt(&ok);
                
                if (ok) {
                    FanInfo fanInfo;
                    fanInfo.name = fanFile;
                    fanInfo.rpm = rpm;
                    fanInfo.devicePath = fanPath;
                    fans.append(fanInfo);
                }
            }
        }
    }
    
    return fans;
}

FanController::FanMode FanController::currentMode() const
{
    return m_currentMode;
}

bool FanController::isAvailable() const
{
    return m_nbfcAvailable || m_fancontrolAvailable || !m_pwmDevices.isEmpty();
}
