#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QTimer>
#include <QColorDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include <QDateTime>
#include <QScrollArea>
#include <QEventLoop>
#include <random>
#include <cmath>
#include <tuple>
#include <thread>
#include <atomic>

// Round Progress Bar Widget (Nyx-inspired)
class QRoundProgressBar : public QWidget {
    Q_OBJECT

public:
    QRoundProgressBar(QWidget* parent = nullptr, int fontSize = 20, 
                     const QColor& defaultColor = QColor(60, 63, 65, 255),
                     const QColor& progressColor = QColor(0, 150, 255, 255),
                     const QColor& innerBgColor = QColor(40, 42, 45, 255))
        : QWidget(parent), fontSize(fontSize), width(0.2), value(0), maxValue(100),
          defaultColor(defaultColor), progressColor(progressColor), innerBgColor(innerBgColor) {
        setMinimumSize(100, 100);
    }

    void setValue(int val) {
        value = qBound(0, val, 100);
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QRect rect = this->rect();
        int size = qMin(rect.width(), rect.height());
        int x = (rect.width() - size) / 2;
        int y = (rect.height() - size) / 2;
        
        QRect squareRect(x, y, size, size);
        QRect innerRect(x + size/4, y + size/4, size/2, size/2);
        
        // Background circle
        painter.setBrush(QBrush(defaultColor));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(squareRect);
        
        // Progress arc
        if (value > 0) {
            painter.setBrush(QBrush(progressColor));
            int angle = static_cast<int>(360 * 16 * (value / 100.0));
            painter.drawPie(squareRect, 90 * 16, -angle);
        }
        
        // Inner circle
        painter.setBrush(QBrush(innerBgColor));
        painter.drawEllipse(innerRect);
        
        // Text
        painter.setPen(QColor(255, 255, 255));
        QFont font;
        font.setPointSize(fontSize);
        font.setBold(true);
        painter.setFont(font);
        
        QString text = QString("%1%").arg(static_cast<int>(value));
        painter.drawText(squareRect, Qt::AlignCenter, text);
    }

private:
    int fontSize;
    float width;
    float value;
    float maxValue;
    QColor defaultColor;
    QColor progressColor;
    QColor innerBgColor;
};

// System Data Updater Thread (Professional threading model)
class SystemDataUpdater : public QThread {
    Q_OBJECT

public:
    SystemDataUpdater(int updateInterval = 1000, QObject* parent = nullptr)
        : QThread(parent), updateInterval(updateInterval), running(false) {}

    void run() override {
        running = true;
        while (running) {
            try {
                QJsonObject data;
                data["cpu_percent"] = getCpuUsage();
                data["cpu_temp"] = getCpuTemperature();
                data["memory"] = getMemoryUsage();
                data["temperatures"] = getTemperatures();
                data["fan_speeds"] = getFanSpeeds();
                data["timestamp"] = QDateTime::currentSecsSinceEpoch();
                
                emit dataUpdated(data);
                msleep(updateInterval);
            } catch (...) {
                msleep(5000);
            }
        }
    }

    void stop() {
        running = false;
        wait();
    }

signals:
    void dataUpdated(const QJsonObject& data);

private:
    int updateInterval;
    std::atomic<bool> running;

    double getCpuUsage() {
        static long long lastIdle = 0, lastTotal = 0;
        
        QFile file("/proc/stat");
        if (!file.open(QIODevice::ReadOnly)) return 0.0;
        
        QString line = file.readLine();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() < 5) return 0.0;
        
        long long idle = parts[4].toLongLong();
        long long total = 0;
        for (int i = 1; i < parts.size(); ++i) {
            total += parts[i].toLongLong();
        }
        
        long long idleDiff = idle - lastIdle;
        long long totalDiff = total - lastTotal;
        
        lastIdle = idle;
        lastTotal = total;
        
        if (totalDiff == 0) return 0.0;
        return (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;
    }

    double getCpuTemperature() {
        QDir thermalDir("/sys/class/thermal");
        QStringList zones = thermalDir.entryList(QStringList() << "thermal_zone*", QDir::Dirs);
        
        double maxTemp = 0.0;
        for (const QString& zone : zones) {
            QString tempPath = QString("/sys/class/thermal/%1/temp").arg(zone);
            QFile tempFile(tempPath);
            if (tempFile.open(QIODevice::ReadOnly)) {
                int tempMilliC = tempFile.readAll().trimmed().toInt();
                double tempC = tempMilliC / 1000.0;
                maxTemp = qMax(maxTemp, tempC);
            }
        }
        return maxTemp;
    }

    QJsonObject getMemoryUsage() {
        QFile file("/proc/meminfo");
        if (!file.open(QIODevice::ReadOnly)) return QJsonObject();
        
        QString content = file.readAll();
        QRegularExpression totalRegex("MemTotal:\\s+(\\d+)");
        QRegularExpression availRegex("MemAvailable:\\s+(\\d+)");
        
        auto totalMatch = totalRegex.match(content);
        auto availMatch = availRegex.match(content);
        
        QJsonObject mem;
        if (totalMatch.hasMatch() && availMatch.hasMatch()) {
            qint64 total = totalMatch.captured(1).toLongLong();
            qint64 available = availMatch.captured(1).toLongLong();
            mem["total"] = total;
            mem["available"] = available;
            mem["used"] = total - available;
            mem["percent"] = (total - available) * 100.0 / total;
        }
        return mem;
    }

    QJsonArray getTemperatures() {
        QJsonArray temps;
        
        // CPU temperatures
        QDir hwmonDir("/sys/class/hwmon");
        QStringList hwmonDevices = hwmonDir.entryList(QStringList() << "hwmon*", QDir::Dirs);
        
        for (const QString& device : hwmonDevices) {
            QString hwmonPath = QString("/sys/class/hwmon/%1").arg(device);
            QDir deviceDir(hwmonPath);
            QStringList tempFiles = deviceDir.entryList(QStringList() << "temp*_input", QDir::Files);
            
            for (const QString& tempFile : tempFiles) {
                QFile file(hwmonPath + "/" + tempFile);
                if (file.open(QIODevice::ReadOnly)) {
                    int tempMilliC = file.readAll().trimmed().toInt();
                    if (tempMilliC > 0) {
                        QJsonObject tempObj;
                        tempObj["name"] = QString("Sensor %1").arg(tempFile);
                        tempObj["temperature"] = tempMilliC / 1000.0;
                        temps.append(tempObj);
                    }
                }
            }
        }
        return temps;
    }

    QJsonArray getFanSpeeds() {
        QJsonArray fans;
        
        QDir hwmonDir("/sys/class/hwmon");
        QStringList hwmonDevices = hwmonDir.entryList(QStringList() << "hwmon*", QDir::Dirs);
        
        for (const QString& device : hwmonDevices) {
            QString hwmonPath = QString("/sys/class/hwmon/%1").arg(device);
            QDir deviceDir(hwmonPath);
            QStringList fanFiles = deviceDir.entryList(QStringList() << "fan*_input", QDir::Files);
            
            for (const QString& fanFile : fanFiles) {
                QFile file(hwmonPath + "/" + fanFile);
                if (file.open(QIODevice::ReadOnly)) {
                    int rpm = file.readAll().trimmed().toInt();
                    if (rpm > 0) {
                        QJsonObject fanObj;
                        fanObj["name"] = QString("Fan %1").arg(fanFile.mid(3, 1));
                        fanObj["rpm"] = rpm;
                        fans.append(fanObj);
                    }
                }
            }
        }
        return fans;
    }
};

// Enhanced RGB Controller with complete keyboard mapping
class EnhancedRGBController : public QObject {
    Q_OBJECT

public:
    EnhancedRGBController(const QString& devicePath = "/dev/hidraw1", QObject* parent = nullptr)
        : QObject(parent), devicePath(devicePath) {
        initializeKeyMappings();
    }

    // Complete keyboard mapping
    QMap<QString, int> keyboardMap;
    QMap<QString, QList<QString>> keyGroups;

    bool checkPermissions() const {
        QFileInfo fileInfo(devicePath);
        return fileInfo.exists() && fileInfo.isWritable();
    }

    bool sendKeyCommand(int keyIndex, int red, int green, int blue) {
        QFile device(devicePath);
        if (!device.open(QIODevice::WriteOnly)) return false;

        QByteArray command(16, 0x00);
        command[0] = 0xCC;
        command[1] = 0x01;
        command[2] = static_cast<char>(keyIndex);
        command[3] = static_cast<char>(red);
        command[4] = static_cast<char>(green);
        command[5] = static_cast<char>(blue);

        device.write(command);
        device.flush();
        return true;
    }

    bool setKeyColor(const QString& keyName, int red, int green, int blue) {
        int keyIndex = keyboardMap.value(keyName.toLower(), -1);
        if (keyIndex == -1) return false;
        return sendKeyCommand(keyIndex, red, green, blue);
    }

    bool setGroupColor(const QString& groupName, int red, int green, int blue) {
        if (!keyGroups.contains(groupName)) return false;
        
        bool success = true;
        for (const QString& key : keyGroups[groupName]) {
            if (!setKeyColor(key, red, green, blue)) success = false;
            QThread::msleep(2);
        }
        return success;
    }

    void setAllKeys(int red, int green, int blue) {
        QFile device(devicePath);
        if (!device.open(QIODevice::WriteOnly)) return;

        for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
            QByteArray command(16, 0x00);
            command[0] = 0xCC;
            command[1] = 0x01;
            command[2] = static_cast<char>(keyIndex);
            command[3] = static_cast<char>(red);
            command[4] = static_cast<char>(green);
            command[5] = static_cast<char>(blue);
            
            device.write(command);
            device.flush();
        }
        device.close();
    }

    void clearAllKeys() {
        setAllKeys(0, 0, 0);
    }

    // Advanced Effects
    void rainbowWaveEffect(int duration = 20) {
        auto start = QDateTime::currentMSecsSinceEpoch();
        while (QDateTime::currentMSecsSinceEpoch() - start < duration * 1000) {
            for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
                float time = (QDateTime::currentMSecsSinceEpoch() - start) / 1000.0f;
                float hue = fmod((keyIndex * 10 + time * 50), 360.0f);
                auto [r, g, b] = hsvToRgb(hue / 360.0f, 1.0f, 1.0f);
                sendKeyCommand(keyIndex, r, g, b);
            }
            QThread::msleep(50);
        }
    }

    void breathingEffect(int red, int green, int blue, int duration = 10) {
        auto start = QDateTime::currentMSecsSinceEpoch();
        while (QDateTime::currentMSecsSinceEpoch() - start < duration * 1000) {
            float time = (QDateTime::currentMSecsSinceEpoch() - start) / 1000.0f;
            float brightness = (sin(time * 2) + 1) / 2;
            
            int r = static_cast<int>(red * brightness);
            int g = static_cast<int>(green * brightness);
            int b = static_cast<int>(blue * brightness);
            
            setAllKeys(r, g, b);
            QThread::msleep(50);
        }
    }

private:
    QString devicePath;

    void initializeKeyMappings() {
        // Complete keyboard mapping from Python version
        keyboardMap = {
            // Special keys
            {"esc", 0x00},
            
            // Function keys
            {"f1", 0x01}, {"f2", 0x02}, {"f3", 0x03}, {"f4", 0x04},
            {"f5", 0x05}, {"f6", 0x06}, {"f7", 0x07}, {"f8", 0x08},
            {"f9", 0x09}, {"f10", 0x0A}, {"f11", 0x0B}, {"f12", 0x0C},
            {"prtsc", 0x0D}, {"scroll", 0x0E}, {"pause", 0x0F},
            
            // Navigation keys
            {"home", 0x10}, {"ins", 0x11}, {"pgup", 0x12}, {"pgdn", 0x13}, 
            {"del", 0x14}, {"end", 0x15},
            
            // Number row
            {"grave", 0x20}, {"`", 0x20},
            {"1", 0x21}, {"2", 0x22}, {"3", 0x23}, {"4", 0x24}, {"5", 0x25},
            {"6", 0x26}, {"7", 0x27}, {"8", 0x28}, {"9", 0x29}, {"0", 0x2A},
            {"minus", 0x2B}, {"-", 0x2B}, {"equals", 0x2D}, {"=", 0x2D},
            {"backspace", 0x2E}, {"bksp", 0x2E},
            
            // Keypad
            {"numlock", 0x30}, {"kp_divide", 0x31}, {"kp_multiply", 0x32}, {"kp_minus", 0x33},
            {"kp_7", 0x50}, {"kp_8", 0x51}, {"kp_9", 0x52}, {"kp_plus", 0x53},
            {"kp_4", 0x70}, {"kp_5", 0x71}, {"kp_6", 0x72},
            {"kp_1", 0x90}, {"kp_2", 0x91}, {"kp_3", 0x92}, {"kp_enter", 0x93},
            {"kp_0", 0xB1}, {"kp_period", 0xB2},
            
            // QWERTY row
            {"tab", 0x40}, {"q", 0x42}, {"w", 0x43}, {"e", 0x44}, {"r", 0x45},
            {"t", 0x46}, {"y", 0x47}, {"u", 0x48}, {"i", 0x49}, {"o", 0x4A},
            {"p", 0x4B}, {"lbracket", 0x4C}, {"[", 0x4C}, {"rbracket", 0x4D}, {"]", 0x4D},
            {"backslash", 0x4E}, {"\\", 0x4E},
            
            // ASDF row
            {"capslock", 0x60}, {"caps", 0x60}, {"a", 0x62}, {"s", 0x63}, {"d", 0x64},
            {"f", 0x65}, {"g", 0x66}, {"h", 0x67}, {"j", 0x68}, {"k", 0x69},
            {"l", 0x6A}, {"semicolon", 0x6B}, {";", 0x6B}, {"quote", 0x6C}, {"'", 0x6C},
            {"enter", 0x6E}, {"return", 0x6E},
            
            // ZXCV row
            {"lshift", 0x80}, {"z", 0x83}, {"x", 0x84}, {"c", 0x85},
            {"v", 0x86}, {"b", 0x87}, {"n", 0x88}, {"m", 0x89}, {"comma", 0x8A}, {",", 0x8A},
            {"period", 0x8B}, {".", 0x8B}, {"slash", 0x8C}, {"/", 0x8C},
            {"rshift", 0x8D},
            
            // Arrow keys
            {"up", 0x8F}, {"left", 0xAE}, {"down", 0xAF}, {"right", 0xB0},
            
            // Bottom row
            {"lctrl", 0xA0}, {"fn", 0xA2}, {"super", 0xA3}, {"lalt", 0xA4}, 
            {"space", 0xA8}, {"spacebar", 0xA8}, {"ralt", 0xAA}, {"menu", 0xAB}, {"rctrl", 0xAC}
        };

        keyGroups = {
            {"function_keys", {"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12"}},
            {"number_row", {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="}},
            {"qwerty_row", {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"}},
            {"asdf_row", {"caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "enter"}},
            {"zxcv_row", {"lshift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "rshift"}},
            {"bottom_row", {"lctrl", "fn", "super", "lalt", "space", "ralt", "menu", "rctrl"}},
            {"arrow_keys", {"up", "left", "down", "right"}},
            {"keypad", {"numlock", "kp_divide", "kp_multiply", "kp_minus", "kp_7", "kp_8", "kp_9", "kp_plus",
                       "kp_4", "kp_5", "kp_6", "kp_1", "kp_2", "kp_3", "kp_enter", "kp_0", "kp_period"}},
            {"letters", {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "a", "s", "d", "f", "g", "h", "j", "k", "l", "z", "x", "c", "v", "b", "n", "m"}},
            {"wasd", {"w", "a", "s", "d"}},
            {"all_keys", {}}
        };
    }

    std::tuple<int, int, int> hsvToRgb(float h, float s, float v) const {
        float r, g, b;
        int i = static_cast<int>(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
        }

        return std::make_tuple(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255));
    }
};

// Hardware control interface for OriginPC EON17-X
class HardwareController : public QObject {
    Q_OBJECT

public:
    struct RGBZone {
        QString name;
        QColor color;
        int brightness;
        bool enabled;
        QString sysfsPath;
    };

    struct FanData {
        QString name;
        int rpm;
        int temperature;
        int dutyCycle;
        bool autoMode;
        QString hwmonPath;
        QString pwmPath;
        QString rpmPath;
        QString tempPath;
    };

    struct SystemInfo {
        QString model;
        QString biosVersion;
        QString ecVersion;
        QString cpu;
        QString gpu;
        QString ram;
        QString storage;
    };

    HardwareController(QObject* parent = nullptr) : QObject(parent) {
        currentProfile = "Balanced";
        detectHardware();
        setupHardwareControl();
        
        // Setup update timer
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &HardwareController::updateSensors);
        updateTimer->start(2000);
    }

    // RGB Control Methods
    QList<RGBZone> getRGBZones() const { return rgbZones; }
    
    void setRGBZone(int index, const QColor& color, int brightness, bool enabled) {
        if (index >= 0 && index < rgbZones.size()) {
            rgbZones[index].color = color;
            rgbZones[index].brightness = brightness;
            rgbZones[index].enabled = enabled;
            applyRGBSettings();
        }
    }
    
    // Immediate RGB application methods for instant feedback
    void setAllKeysColor(const QColor& color, int brightness = 255) {
        setAllKeys(color, brightness);
    }
    
    void clearAllKeysImmediate() {
        clearAllKeys();
    }

    void applyRGBPreset(const QString& preset) {
        if (preset == "Gaming") {
            setRGBZone(0, QColor(255, 0, 0), 255, true);
            setRGBZone(1, QColor(0, 255, 0), 255, true);
            setRGBZone(2, QColor(0, 0, 255), 255, true);
            setRGBZone(3, QColor(255, 255, 0), 255, true);
        } else if (preset == "Work") {
            for (int i = 0; i < rgbZones.size(); ++i) {
                setRGBZone(i, QColor(255, 255, 255), 128, true);
            }
        } else if (preset == "Rainbow") {
            setRGBZone(0, QColor(255, 0, 0), 255, true);
            setRGBZone(1, QColor(0, 255, 0), 255, true);
            setRGBZone(2, QColor(0, 0, 255), 255, true);
            setRGBZone(3, QColor(255, 0, 255), 255, true);
        } else if (preset == "Lights") {
            for (int i = 0; i < rgbZones.size(); ++i) {
                setRGBZone(i, QColor(0, 0, 0), 0, false);
            }
        }
    }

    // Fan Control Methods
    QList<FanData> getFanData() const { return fanData; }
    
    void setFanSpeed(int fanIndex, int dutyCycle) {
        if (fanIndex >= 0 && fanIndex < fanData.size()) {
            fanData[fanIndex].dutyCycle = dutyCycle;
            fanData[fanIndex].autoMode = false;
            applyFanSettings();
        }
    }

    void setFanAutoMode(int fanIndex, bool autoMode) {
        if (fanIndex >= 0 && fanIndex < fanData.size()) {
            fanData[fanIndex].autoMode = autoMode;
            applyFanSettings();
        }
    }

    // Performance Profile Methods
    void applyPerformanceProfile(const QString& profile) {
        currentProfile = profile;
        applyProfileSettings();
        emit profileChanged(profile);
    }

    QString getCurrentProfile() const { return currentProfile; }

    // System Info
    SystemInfo getSystemInfo() const { return sysInfo; }

signals:
    void rgbSettingsApplied();
    void fanSettingsApplied();
    void profileChanged(const QString& profile);
    void sensorsUpdated();

private slots:
    void updateSensors() {
        emit sensorsUpdated();
    }

private:
    void detectHardware() {
        // Detect system information for OriginPC EON17-X
        sysInfo.model = readSystemFile("/sys/devices/virtual/dmi/id/product_name").trimmed();
        sysInfo.biosVersion = readSystemFile("/sys/devices/virtual/dmi/id/bios_version").trimmed();
        sysInfo.cpu = "Intel i9-13900HX";
        sysInfo.gpu = "NVIDIA RTX 4080 Laptop GPU";
        sysInfo.ram = "64 GB RAM";
        sysInfo.storage = "NVMe SSD";
        sysInfo.ecVersion = "1.07.09";
    }
    
    void setupHardwareControl() {
        // Initialize RGB zones for Clevo keyboard
        rgbZones = {
            {"WASD Keys", QColor(0, 255, 136), 255, true, "/dev/hidraw1"},
            {"Arrow Keys", QColor(0, 153, 255), 255, true, "/dev/hidraw1"},
            {"Function Keys", QColor(255, 107, 107), 255, true, "/dev/hidraw1"},
            {"Number Pad", QColor(240, 147, 251), 255, true, "/dev/hidraw1"}
        };
        
        // Initialize fan data 
        fanData = {
            {"CPU Fan", 2450, 67, 60, true, "/sys/class/hwmon/hwmon0", "/sys/class/hwmon/hwmon0/pwm1", "/sys/class/hwmon/hwmon0/fan1_input", "/sys/class/hwmon/hwmon0/temp1_input"},
            {"GPU Fan", 3200, 78, 80, true, "/sys/class/hwmon/hwmon1", "/sys/class/hwmon/hwmon1/pwm1", "/sys/class/hwmon/hwmon1/fan1_input", "/sys/class/hwmon/hwmon1/temp1_input"}
        };
        
        // Check if we have access to hidraw1
        QFileInfo hidrawInfo("/dev/hidraw1");
        if (!hidrawInfo.exists()) {
            qDebug() << "Warning: /dev/hidraw1 not found. RGB control may not work.";
        }
    }
    
    QString readSystemFile(const QString& path) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return file.readAll();
        }
        return "Unknown";
    }

    void applyRGBSettings() {
        writeClevoRGBControl();
        emit rgbSettingsApplied();
    }

    void applyFanSettings() {
        emit fanSettingsApplied();
    }

    void applyProfileSettings() {
        // Apply performance profile settings
        if (currentProfile == "Performance") {
            QProcess::execute("cpupower", QStringList() << "frequency-set" << "-g" << "performance");
        } else if (currentProfile == "Quiet") {
            QProcess::execute("cpupower", QStringList() << "frequency-set" << "-g" << "powersave");
        } else if (currentProfile == "Balanced") {
            QProcess::execute("cpupower", QStringList() << "frequency-set" << "-g" << "ondemand");
        }
    }

    void writeClevoRGBControl() {
        // Use hidraw1 by default as specified
        QString devicePath = "/dev/hidraw1";
        
        // Try hidraw1 first, then hidraw0 as fallback
        QFile hidrawFile(devicePath);
        if (!hidrawFile.open(QIODevice::WriteOnly)) {
            devicePath = "/dev/hidraw0";
            hidrawFile.setFileName(devicePath);
            if (!hidrawFile.open(QIODevice::WriteOnly)) {
                qDebug() << "Warning: Cannot access /dev/hidraw1 or /dev/hidraw0 for RGB control.";
                return;
            }
        }

        // Use the correct Clevo RGB command structure from reference
        for (int i = 0; i < rgbZones.size(); ++i) {
            const auto& zone = rgbZones[i];
            if (!zone.enabled) continue;

            // Calculate RGB values with brightness
            int red = (zone.color.red() * zone.brightness) / 255;
            int green = (zone.color.green() * zone.brightness) / 255;
            int blue = (zone.color.blue() * zone.brightness) / 255;

            // Clevo keyboard RGB command structure (from originpc-control reference)
            QByteArray command;
            command.resize(16);
            command[0] = 0xCC;  // Command header
            command[1] = 0x01;  // RGB subcommand
            command[2] = static_cast<char>(getKeyIndexForZone(i));  // Key index for zone
            command[3] = static_cast<char>(red);
            command[4] = static_cast<char>(green);
            command[5] = static_cast<char>(blue);
            // Fill remaining bytes with 0x00
            for (int j = 6; j < 16; ++j) {
                command[j] = 0x00;
            }

            hidrawFile.write(command);
            QThread::msleep(5);
        }
        hidrawFile.close();
        qDebug() << "RGB commands sent to" << devicePath;
    }
    
    int getKeyIndexForZone(int zoneIndex) {
        // Map zones to key indices based on Clevo keyboard layout
        switch (zoneIndex) {
            case 0: return 0x62; // WASD area (A key)
            case 1: return 0x8F; // Arrow keys area (Up arrow)
            case 2: return 0x01; // Function keys area (F1 key)
            case 3: return 0x53; // Number pad area (kp_plus)
            default: return 0x62;
        }
    }
    
    void setAllKeys(const QColor& color, int brightness) {
        // Use hidraw1 first (RGB keyboard on your OriginPC EON17-X)
        QString devicePath = "/dev/hidraw1";
        QFile hidrawFile(devicePath);
        if (!hidrawFile.open(QIODevice::WriteOnly)) {
            devicePath = "/dev/hidraw0";
            hidrawFile.setFileName(devicePath);
            if (!hidrawFile.open(QIODevice::WriteOnly)) {
                qDebug() << "Warning: Cannot access hidraw devices for RGB control.";
                return;
            }
        }
        
        // Calculate RGB values with brightness
        int red = (color.red() * brightness) / 255;
        int green = (color.green() * brightness) / 255;
        int blue = (color.blue() * brightness) / 255;
        
        // Use exact command structure from working Python implementation
        for (int keyIndex = 0; keyIndex <= 0xFF; ++keyIndex) {
            QByteArray command;
            command.resize(16);
            command[0] = 0xCC;
            command[1] = 0x01;
            command[2] = static_cast<char>(keyIndex);
            command[3] = static_cast<char>(red);
            command[4] = static_cast<char>(green);
            command[5] = static_cast<char>(blue);
            // Fill remaining 10 bytes with 0x00
            for (int j = 6; j < 16; ++j) {
                command[j] = 0x00;
            }
            
            hidrawFile.write(command);
            hidrawFile.flush(); // Important: flush after each command
        }
        
        hidrawFile.close();
        qDebug() << "Set all keys to RGB:" << red << green << blue << "via" << devicePath;
    }
    
    void clearAllKeys() {
        // Use hidraw1 first (RGB keyboard on your OriginPC EON17-X)
        QString devicePath = "/dev/hidraw1";
        QFile hidrawFile(devicePath);
        if (!hidrawFile.open(QIODevice::WriteOnly)) {
            devicePath = "/dev/hidraw0";
            hidrawFile.setFileName(devicePath);
            if (!hidrawFile.open(QIODevice::WriteOnly)) {
                qDebug() << "Warning: Cannot access hidraw devices for clearing.";
                return;
            }
        }
        
        // Use exact command structure from working Python implementation
        for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
            QByteArray command;
            command.resize(16);
            command[0] = 0xCC;
            command[1] = 0x01;
            command[2] = static_cast<char>(keyIndex);
            command[3] = 0x00; // Red = 0
            command[4] = 0x00; // Green = 0 
            command[5] = 0x00; // Blue = 0
            // Fill remaining 10 bytes with 0x00
            for (int j = 6; j < 16; ++j) {
                command[j] = 0x00;
            }
            
            hidrawFile.write(command);
            hidrawFile.flush(); // Important: flush after each command
        }
        
        hidrawFile.close();
        qDebug() << "Cleared all keys via" << devicePath;
    }

    QList<RGBZone> rgbZones;
    QList<FanData> fanData;
    SystemInfo sysInfo;
    QString currentProfile;
    QTimer* updateTimer;
};

// Simple main application window
class ClevoControlCenter : public QMainWindow {
    Q_OBJECT

public:
    ClevoControlCenter(QWidget* parent = nullptr) : QMainWindow(parent) {
        hwController = new HardwareController(this);
        setupUI();
    }

private:
    void setupUI() {
        setWindowTitle("Clevo Control Center - RGB Test");
        setMinimumSize(600, 400);
        
        auto* centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        
        auto* layout = new QVBoxLayout(centralWidget);
        
        // Header
        auto* headerLabel = new QLabel("🎮 Clevo RGB Control Test");
        headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #00ff88; text-align: center;");
        headerLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(headerLabel);
        
        // Quick Color Buttons
        auto* colorsGroup = new QGroupBox("Quick Colors (Immediate Test)");
        auto* colorsLayout = new QGridLayout(colorsGroup);
        
        QList<QPair<QString, QColor>> quickColors = {
            {"🔴 Red", QColor(255, 0, 0)},
            {"🟢 Green", QColor(0, 255, 0)},
            {"🔵 Blue", QColor(0, 0, 255)},
            {"🟡 Yellow", QColor(255, 255, 0)},
            {"🟣 Purple", QColor(128, 0, 128)},
            {"🟠 Orange", QColor(255, 165, 0)},
            {"🔘 White", QColor(255, 255, 255)},
            {"⚫ Clear", QColor(0, 0, 0)}
        };
        
        for (int i = 0; i < quickColors.size(); ++i) {
            const auto& colorPair = quickColors[i];
            auto* button = new QPushButton(colorPair.first);
            button->setStyleSheet(QString("QPushButton { background-color: rgb(%1,%2,%3); color: %4; border: 2px solid #666; "
                                         "border-radius: 8px; padding: 15px; font-weight: bold; font-size: 14px; }"
                                         "QPushButton:hover { border: 2px solid #00ff88; }")
                                 .arg(colorPair.second.red()).arg(colorPair.second.green()).arg(colorPair.second.blue())
                                 .arg(colorPair.second.lightness() > 128 ? "black" : "white"));
            
            connect(button, &QPushButton::clicked, [this, color = colorPair.second]() {
                hwController->setAllKeysColor(color, 255);
                statusBar()->showMessage(QString("Applied %1 to all keys").arg(color.name()), 2000);
            });
            
            colorsLayout->addWidget(button, i / 4, i % 4);
        }
        
        layout->addWidget(colorsGroup);
        
        // Status
        statusBar()->showMessage("Ready - Test RGB colors above");
        
        // Dark theme
        setStyleSheet(
            "QMainWindow { background-color: #1e1e1e; color: white; }"
            "QWidget { background-color: #1e1e1e; color: white; }"
            "QGroupBox { font-weight: bold; border: 2px solid #444; border-radius: 5px; margin: 10px 0; padding-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }"
            "QLabel { color: white; }"
            "QStatusBar { background: #2b2b2b; color: white; }"
        );
    }

    HardwareController* hwController;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    ClevoControlCenter window;
    window.show();
    
    return app.exec();
}

#include "main.moc"
