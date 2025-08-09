#include "rgbfancontrol.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>

// RGB Effect Manager Implementation
RGBEffectManager::RGBEffectManager(QObject *parent)
    : QObject(parent)
    , m_effectTimer(new QTimer(this))
    , m_effectStep(0)
{
    m_effectTimer->setSingleShot(false);
    connect(m_effectTimer, &QTimer::timeout, this, &RGBEffectManager::updateEffect);
    
    initializeDevices();
}

bool RGBEffectManager::initializeDevices()
{
    m_devices.clear();
    
    // Scan for RGB devices in common locations
    QStringList devicePaths = {
        "/dev/hidraw0", "/dev/hidraw1", "/dev/hidraw2", "/dev/hidraw3",
        "/dev/usb/hiddev0", "/dev/usb/hiddev1",
        "/sys/class/leds"
    };
    
    for (const QString &path : devicePaths) {
        QFile device(path);
        if (device.exists()) {
            // Try to determine if this is an RGB device
            if (path.contains("hidraw")) {
                // Basic detection - in real implementation, would check device descriptors
                m_devices.append(path);
                qDebug() << "Found potential RGB device:" << path;
            }
        }
    }
    
    // Look for OpenRGB compatible devices
    QProcess openrgbCheck;
    openrgbCheck.start("which", QStringList() << "openrgb");
    openrgbCheck.waitForFinished(2000);
    
    if (openrgbCheck.exitCode() == 0) {
        qDebug() << "OpenRGB found - using for RGB control";
        m_devices.append("openrgb");
    }
    
    // Look for system-specific RGB controls
    QDir ledsDir("/sys/class/leds");
    if (ledsDir.exists()) {
        QStringList ledEntries = ledsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &led : ledEntries) {
            if (led.contains("rgb") || led.contains("led")) {
                m_devices.append("/sys/class/leds/" + led);
                qDebug() << "Found LED device:" << led;
            }
        }
    }
    
    return !m_devices.isEmpty();
}

QList<QString> RGBEffectManager::getAvailableDevices()
{
    return m_devices;
}

QList<QString> RGBEffectManager::getAvailableEffects()
{
    return {"Static", "Breathing", "Rainbow", "Wave", "Custom"};
}

bool RGBEffectManager::applyEffect(const RGBEffect &effect)
{
    m_currentEffect = effect;
    
    if (!effect.enabled) {
        m_effectTimer->stop();
        return true;
    }
    
    // Stop any current effect
    m_effectTimer->stop();
    m_effectStep = 0;
    
    if (effect.type == "static") {
        // Apply static color immediately
        for (const QString &device : m_devices) {
            sendCommand(device, createStaticColorCommand(effect.primaryColor, effect.brightness));
        }
    } else if (effect.type == "breathing") {
        // Start breathing effect timer
        int interval = 50 + (100 - effect.speed); // Faster speed = shorter interval
        m_effectTimer->start(interval);
        updateEffect();
    } else if (effect.type == "rainbow") {
        // Start rainbow effect timer
        int interval = 30 + (100 - effect.speed);
        m_effectTimer->start(interval);
        updateEffect();
    } else if (effect.type == "wave") {
        // Start wave effect timer
        int interval = 40 + (100 - effect.speed);
        m_effectTimer->start(interval);
        updateEffect();
    }
    
    return true;
}

bool RGBEffectManager::sendCommand(const QString &device, const QByteArray &command)
{
    if (device == "openrgb") {
        // Use OpenRGB command line interface
        QProcess openrgb;
        QStringList args;
        
        // Convert command to OpenRGB format
        // This is a simplified implementation
        args << "--mode" << "static" << "--color" << "ff0000";
        
        openrgb.start("openrgb", args);
        openrgb.waitForFinished(3000);
        
        return openrgb.exitCode() == 0;
    } else if (device.startsWith("/sys/class/leds/")) {
        // Use sysfs LED interface
        QFile brightnessFile(device + "/brightness");
        if (brightnessFile.open(QIODevice::WriteOnly)) {
            brightnessFile.write(QByteArray::number(255)); // Max brightness
            brightnessFile.close();
            return true;
        }
    } else if (device.startsWith("/dev/hidraw")) {
        // Direct HID communication
        QFile hidDevice(device);
        if (hidDevice.open(QIODevice::WriteOnly)) {
            qint64 written = hidDevice.write(command);
            hidDevice.close();
            return written > 0;
        }
    }
    
    return false;
}

QByteArray RGBEffectManager::createStaticColorCommand(const QColor &color, int brightness)
{
    QByteArray command;
    
    // Generic RGB command format (varies by device)
    // This is a simplified example
    command.append(0x01); // Command prefix
    command.append(static_cast<char>(color.red() * brightness / 100));
    command.append(static_cast<char>(color.green() * brightness / 100));
    command.append(static_cast<char>(color.blue() * brightness / 100));
    
    return command;
}

void RGBEffectManager::updateEffect()
{
    if (m_currentEffect.type == "breathing") {
        generateBreathingEffect();
    } else if (m_currentEffect.type == "rainbow") {
        generateRainbowEffect();
    } else if (m_currentEffect.type == "wave") {
        generateWaveEffect();
    }
    
    m_effectStep++;
    if (m_effectStep > 360) m_effectStep = 0; // Reset for cyclical effects
}

void RGBEffectManager::generateBreathingEffect()
{
    // Create breathing effect by varying brightness
    double factor = (sin(m_effectStep * M_PI / 180.0) + 1.0) / 2.0; // 0-1 range
    int currentBrightness = static_cast<int>(m_currentEffect.brightness * factor);
    
    QByteArray command = createStaticColorCommand(m_currentEffect.primaryColor, currentBrightness);
    
    for (const QString &device : m_devices) {
        sendCommand(device, command);
    }
}

void RGBEffectManager::generateRainbowEffect()
{
    // Generate rainbow colors by cycling through hue
    QColor rainbowColor;
    rainbowColor.setHsv(m_effectStep, 255, 255); // Cycle hue, full saturation and value
    
    QByteArray command = createStaticColorCommand(rainbowColor, m_currentEffect.brightness);
    
    for (const QString &device : m_devices) {
        sendCommand(device, command);
    }
}

void RGBEffectManager::generateWaveEffect()
{
    // Create wave effect by transitioning between primary and secondary colors
    double factor = (sin(m_effectStep * M_PI / 180.0) + 1.0) / 2.0; // 0-1 range
    
    QColor waveColor;
    waveColor.setRed(static_cast<int>(m_currentEffect.primaryColor.red() * (1.0 - factor) + 
                                     m_currentEffect.secondaryColor.red() * factor));
    waveColor.setGreen(static_cast<int>(m_currentEffect.primaryColor.green() * (1.0 - factor) + 
                                       m_currentEffect.secondaryColor.green() * factor));
    waveColor.setBlue(static_cast<int>(m_currentEffect.primaryColor.blue() * (1.0 - factor) + 
                                      m_currentEffect.secondaryColor.blue() * factor));
    
    QByteArray command = createStaticColorCommand(waveColor, m_currentEffect.brightness);
    
    for (const QString &device : m_devices) {
        sendCommand(device, command);
    }
}

// Fan Control Manager Implementation
FanControlManager::FanControlManager(QObject *parent)
    : QObject(parent)
{
    initializeFans();
}

bool FanControlManager::initializeFans()
{
    m_fanDevices.clear();
    
    // Scan for fan devices in common locations
    QStringList fanPaths = {
        "/sys/class/hwmon",
        "/proc/acpi/fan"
    };
    
    // Check hwmon devices
    QDir hwmonDir("/sys/class/hwmon");
    if (hwmonDir.exists()) {
        QStringList hwmonEntries = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &hwmon : hwmonEntries) {
            QString hwmonPath = "/sys/class/hwmon/" + hwmon;
            
            // Look for PWM controls
            QDir deviceDir(hwmonPath);
            QStringList entries = deviceDir.entryList(QDir::Files);
            for (const QString &entry : entries) {
                if (entry.startsWith("pwm") && entry.endsWith("_enable")) {
                    QString pwmDevice = hwmonPath + "/" + entry.left(4); // e.g., pwm1
                    m_fanDevices.append(pwmDevice);
                    qDebug() << "Found fan control:" << pwmDevice;
                }
            }
        }
    }
    
    // Check for fancontrol configuration
    if (QFile::exists("/etc/fancontrol")) {
        qDebug() << "Found fancontrol configuration";
    }
    
    // Look for lm-sensors
    QProcess sensorsCheck;
    sensorsCheck.start("which", QStringList() << "sensors");
    sensorsCheck.waitForFinished(2000);
    
    if (sensorsCheck.exitCode() == 0) {
        qDebug() << "lm-sensors found - enhanced monitoring available";
    }
    
    return !m_fanDevices.isEmpty();
}

QList<QString> FanControlManager::getAvailableFans()
{
    return m_fanDevices;
}

bool FanControlManager::applyProfile(const FanProfile &profile)
{
    m_currentProfile = profile;
    
    if (!profile.enabled) {
        return true;
    }
    
    // Apply the fan profile - this would typically set up automatic control
    // based on temperature thresholds
    
    qDebug() << "Applied fan profile:" << profile.name;
    return true;
}

bool FanControlManager::setFanSpeed(const QString &fan, int pwmValue)
{
    // Clamp PWM value to valid range (0-255 or 0-100 depending on system)
    pwmValue = qMax(0, qMin(255, pwmValue));
    
    if (fan.startsWith("/sys/class/hwmon/")) {
        // Write to sysfs PWM interface
        QFile pwmFile(fan);
        if (pwmFile.open(QIODevice::WriteOnly)) {
            qint64 written = pwmFile.write(QByteArray::number(pwmValue));
            pwmFile.close();
            
            if (written > 0) {
                m_lastFanSpeeds[fan] = pwmValue;
                return true;
            }
        }
    }
    
    return false;
}

void FanControlManager::updateFanSpeeds(const SystemData &data)
{
    if (!m_currentProfile.enabled) {
        return;
    }
    
    // Find the highest CPU temperature
    double maxTemp = 0.0;
    for (const auto &temp : data.cpuTemps) {
        maxTemp = qMax(maxTemp, temp.second);
    }
    
    // Calculate fan speed based on temperature curve
    int targetPwm = calculateFanSpeed(maxTemp);
    
    // Apply to all fan devices
    for (const QString &fan : m_fanDevices) {
        setFanSpeed(fan, targetPwm);
    }
}

int FanControlManager::calculateFanSpeed(double temperature)
{
    if (m_currentProfile.tempToPwmCurve.isEmpty()) {
        return 50; // Default 50% speed
    }
    
    // Find the appropriate PWM value for the current temperature
    auto it = m_currentProfile.tempToPwmCurve.lowerBound(static_cast<int>(temperature));
    
    if (it == m_currentProfile.tempToPwmCurve.begin()) {
        // Temperature is below the lowest threshold
        return it.value();
    } else if (it == m_currentProfile.tempToPwmCurve.end()) {
        // Temperature is above the highest threshold
        --it;
        return it.value();
    } else {
        // Interpolate between two points
        auto upper = it;
        --it;
        auto lower = it;
        
        double tempRange = upper.key() - lower.key();
        double pwmRange = upper.value() - lower.value();
        double tempOffset = temperature - lower.key();
        
        return static_cast<int>(lower.value() + (pwmRange * tempOffset / tempRange));
    }
}

bool FanControlManager::writeToFanDevice(const QString &device, int value)
{
    return setFanSpeed(device, value);
}

int FanControlManager::readFromFanDevice(const QString &device)
{
    if (device.startsWith("/sys/class/hwmon/")) {
        QFile pwmFile(device);
        if (pwmFile.open(QIODevice::ReadOnly)) {
            QByteArray data = pwmFile.readAll();
            pwmFile.close();
            return data.trimmed().toInt();
        }
    }
    
    return 0;
}

// Configuration Manager Implementation
ConfigurationManager::ConfigurationManager(QObject *parent)
    : QObject(parent)
{
    m_configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/ArchBackupPro/RGBFanControl";
    QDir().mkpath(m_configDir);
    
    m_settings = new QSettings(m_configDir + "/config.ini", QSettings::IniFormat, this);
    
    createDefaultProfiles();
}

QMap<QString, RGBEffect> ConfigurationManager::loadRGBProfiles()
{
    QMap<QString, RGBEffect> profiles;
    
    QString profilesFile = m_configDir + "/rgb_profiles.json";
    QFile file(profilesFile);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject rootObj = doc.object();
        
        for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
            RGBEffect effect;
            QJsonObject effectObj = it.value().toObject();
            
            effect.name = it.key();
            effect.type = effectObj["type"].toString();
            effect.primaryColor = QColor(effectObj["primaryColor"].toString());
            effect.secondaryColor = QColor(effectObj["secondaryColor"].toString());
            effect.brightness = effectObj["brightness"].toInt();
            effect.speed = effectObj["speed"].toInt();
            effect.enabled = effectObj["enabled"].toBool();
            
            profiles[it.key()] = effect;
        }
    }
    
    return profiles;
}

bool ConfigurationManager::saveRGBProfiles(const QMap<QString, RGBEffect> &profiles)
{
    QString profilesFile = m_configDir + "/rgb_profiles.json";
    QFile file(profilesFile);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject rootObj;
        
        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            const RGBEffect &effect = it.value();
            QJsonObject effectObj;
            
            effectObj["type"] = effect.type;
            effectObj["primaryColor"] = effect.primaryColor.name();
            effectObj["secondaryColor"] = effect.secondaryColor.name();
            effectObj["brightness"] = effect.brightness;
            effectObj["speed"] = effect.speed;
            effectObj["enabled"] = effect.enabled;
            
            rootObj[it.key()] = effectObj;
        }
        
        QJsonDocument doc(rootObj);
        file.write(doc.toJson());
        return true;
    }
    
    return false;
}

QMap<QString, FanProfile> ConfigurationManager::loadFanProfiles()
{
    QMap<QString, FanProfile> profiles;
    
    QString profilesFile = m_configDir + "/fan_profiles.json";
    QFile file(profilesFile);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject rootObj = doc.object();
        
        for (auto it = rootObj.begin(); it != rootObj.end(); ++it) {
            FanProfile profile;
            QJsonObject profileObj = it.value().toObject();
            
            profile.name = it.key();
            profile.enabled = profileObj["enabled"].toBool();
            profile.hysteresis = profileObj["hysteresis"].toInt();
            profile.targetSensor = profileObj["targetSensor"].toString();
            
            QJsonObject curveObj = profileObj["curve"].toObject();
            for (auto curveIt = curveObj.begin(); curveIt != curveObj.end(); ++curveIt) {
                profile.tempToPwmCurve[curveIt.key().toInt()] = curveIt.value().toInt();
            }
            
            profiles[it.key()] = profile;
        }
    }
    
    return profiles;
}

bool ConfigurationManager::saveFanProfiles(const QMap<QString, FanProfile> &profiles)
{
    QString profilesFile = m_configDir + "/fan_profiles.json";
    QFile file(profilesFile);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject rootObj;
        
        for (auto it = profiles.begin(); it != profiles.end(); ++it) {
            const FanProfile &profile = it.value();
            QJsonObject profileObj;
            
            profileObj["enabled"] = profile.enabled;
            profileObj["hysteresis"] = profile.hysteresis;
            profileObj["targetSensor"] = profile.targetSensor;
            
            QJsonObject curveObj;
            for (auto curveIt = profile.tempToPwmCurve.begin(); curveIt != profile.tempToPwmCurve.end(); ++curveIt) {
                curveObj[QString::number(curveIt.key())] = curveIt.value();
            }
            profileObj["curve"] = curveObj;
            
            rootObj[it.key()] = profileObj;
        }
        
        QJsonDocument doc(rootObj);
        file.write(doc.toJson());
        return true;
    }
    
    return false;
}

QVariant ConfigurationManager::getSetting(const QString &key, const QVariant &defaultValue)
{
    return m_settings->value(key, defaultValue);
}

void ConfigurationManager::setSetting(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
    m_settings->sync();
}

void ConfigurationManager::createDefaultProfiles()
{
    // Create default RGB profiles if they don't exist
    QMap<QString, RGBEffect> rgbProfiles = loadRGBProfiles();
    if (rgbProfiles.isEmpty()) {
        RGBEffect staticRed;
        staticRed.name = "Static Red";
        staticRed.type = "static";
        staticRed.primaryColor = Qt::red;
        staticRed.brightness = 100;
        staticRed.speed = 50;
        staticRed.enabled = true;
        rgbProfiles["Static Red"] = staticRed;
        
        saveRGBProfiles(rgbProfiles);
    }
    
    // Create default fan profiles if they don't exist
    QMap<QString, FanProfile> fanProfiles = loadFanProfiles();
    if (fanProfiles.isEmpty()) {
        FanProfile silent;
        silent.name = "Silent";
        silent.tempToPwmCurve[30] = 20;
        silent.tempToPwmCurve[50] = 40;
        silent.tempToPwmCurve[70] = 70;
        silent.tempToPwmCurve[80] = 90;
        silent.enabled = true;
        silent.hysteresis = 3;
        fanProfiles["Silent"] = silent;
        
        saveFanProfiles(fanProfiles);
    }
}
