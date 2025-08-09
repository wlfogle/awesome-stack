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
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <random>
#include <cmath>

// Enhanced RGB Controller for comprehensive keyboard control
class EnhancedRGBController : public QObject {
    Q_OBJECT

public:
    EnhancedRGBController(const QString& devicePath = "/dev/hidraw1", QObject* parent = nullptr)
        : QObject(parent), devicePath(devicePath) {
        initializeKeyMappings();
    }

    bool checkPermissions() const {
        QFileInfo fileInfo(devicePath);
        return fileInfo.exists() && fileInfo.isWritable();
    }

    bool sendKeyCommand(int keyIndex, int red, int green, int blue) {
        QFile device(devicePath);
        if (!device.open(QIODevice::WriteOnly))
            return false;

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
        if (keyIndex == -1)
            return false;
        return sendKeyCommand(keyIndex, red, green, blue);
    }

    bool setGroupColor(const QString& groupName, int red, int green, int blue) {
        if (!keyGroups.contains(groupName))
            return false;
        
        bool success = true;
        for (const QString& key : keyGroups[groupName]) {
            if (!setKeyColor(key, red, green, blue))
                success = false;
            QThread::msleep(2); // Small delay for smoother effects
        }
        return success;
    }

    bool clearAllKeys() {
        // Standard clear - 2 passes
        for (int pass = 0; pass < 2; ++pass) {
            for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
                sendKeyCommand(keyIndex, 0, 0, 0);
            }
        }

        // SUPER AGGRESSIVE kp_plus clearing (this key is stubborn!)
        QList<int> kpPlusIndices = {0x53, 0x33, 0x73, 0x93, 0xB3, 0xD3, 0xF3};
        
        for (int pass = 0; pass < 8; ++pass) { // 8 passes for kp_plus specifically
            for (int idx : kpPlusIndices) {
                // Clear exact index
                sendKeyCommand(idx, 0, 0, 0);
                
                // Clear surrounding area (wide net)
                for (int offset = -8; offset <= 8; ++offset) {
                    int clearIdx = qBound(0, idx + offset, 0xFF);
                    sendKeyCommand(clearIdx, 0, 0, 0);
                }
                QThread::msleep(10); // Small delay between attempts
            }
        }

        return true;
    }

    void setAllKeys(int red, int green, int blue) {
        for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
            sendKeyCommand(keyIndex, red, green, blue);
        }
    }

    // Advanced effect methods
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
            float brightness = (sin(time * 2) + 1) / 2; // 0 to 1
            
            int r = static_cast<int>(red * brightness);
            int g = static_cast<int>(green * brightness);
            int b = static_cast<int>(blue * brightness);
            
            setAllKeys(r, g, b);
            QThread::msleep(50);
        }
    }

    QMap<QString, QList<QString>> getKeyGroups() const { return keyGroups; }
    QMap<QString, int> getKeyboardMap() const { return keyboardMap; }

private:
    QString devicePath;
    QMap<QString, int> keyboardMap;
    QMap<QString, QList<QString>> keyGroups;

    void initializeKeyMappings() {
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
            
            // Keypad - Enhanced mapping
            {"numlock", 0x30}, {"kp_divide", 0x31}, {"kp_multiply", 0x32}, {"kp_minus", 0x33},
            {"kp_7", 0x50}, {"kp_8", 0x51}, {"kp_9", 0x52}, {"kp_plus", 0x53},
            {"kp_4", 0x70}, {"kp_5", 0x71}, {"kp_6", 0x72},
            {"kp_1", 0x90}, {"kp_2", 0x91}, {"kp_3", 0x92}, {"kp_enter", 0x93},
            {"kp_0", 0xB1}, {"kp_period", 0xB2}, {"kp_dot", 0xB2},
            
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
            {"lshift", 0x80}, {"lshft", 0x80}, {"z", 0x83}, {"x", 0x84}, {"c", 0x85},
            {"v", 0x86}, {"b", 0x87}, {"n", 0x88}, {"m", 0x89}, {"comma", 0x8A}, {",", 0x8A},
            {"period", 0x8B}, {".", 0x8B}, {"slash", 0x8C}, {"/", 0x8C},
            {"rshift", 0x8D}, {"rshft", 0x8D},
            
            // Arrow keys
            {"up", 0x8F}, {"up_arrow", 0x8F}, {"left", 0xAE}, {"left_arrow", 0xAE},
            {"down", 0xAF}, {"down_arrow", 0xAF}, {"right", 0xB0}, {"right_arrow", 0xB0},
            
            // Bottom row modifiers and spacebar
            {"lctrl", 0xA0}, {"lcontrol", 0xA0}, {"fn", 0xA2}, {"super", 0xA3}, {"win", 0xA3},
            {"lalt", 0xA4}, {"space_left", 0xA5}, {"space_center", 0xA6},
            {"space", 0xA8}, {"spacebar", 0xA8}, {"space_right", 0xA8}, {"space_far_right", 0xA9},
            {"ralt", 0xAA}, {"menu", 0xAB}, {"rctrl", 0xAC}, {"rcontrol", 0xAC},
        };

        keyGroups = {
            {"function_keys", {"f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12"}},
            {"number_row", {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="}},
            {"qwerty_row", {"tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"}},
            {"asdf_row", {"caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "enter"}},
            {"zxcv_row", {"lshift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "rshift"}},
            {"bottom_row", {"lctrl", "fn", "super", "lalt", "space_left", "space_center", "space", "space_far_right", "ralt", "menu", "rctrl"}},
            {"spacebar_full", {"space_left", "space_center", "space", "space_far_right"}},
            {"arrow_keys", {"up", "left", "down", "right"}},
            {"keypad", {"numlock", "kp_divide", "kp_multiply", "kp_minus", "kp_7", "kp_8", "kp_9", "kp_plus",
                       "kp_4", "kp_5", "kp_6", "kp_1", "kp_2", "kp_3", "kp_enter", "kp_0", "kp_period"}},
            {"letters", {"q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "a", "s", "d", "f", "g", "h", "j", "k", "l", "z", "x", "c", "v", "b", "n", "m"}},
            {"navigation", {"ins", "home", "pgup", "del", "end", "pgdn"}},
            {"special", {"esc", "prtsc", "scroll", "pause"}},
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

// System Monitor Thread for real-time data
class SystemMonitorThread : public QThread {
    Q_OBJECT

public:
    SystemMonitorThread(QObject* parent = nullptr) : QThread(parent), running(false) {}

    void run() override {
        running = true;
        while (running) {
            QJsonObject systemData;
            
            // CPU usage
            systemData["cpu_usage"] = getCpuUsage();
            
            // Memory usage
            systemData["memory_usage"] = getMemoryUsage();
            
            // Temperature data
            systemData["temperatures"] = getTemperatures();
            
            // Fan speeds
            systemData["fan_speeds"] = getFanSpeeds();
            
            emit dataUpdated(systemData);
            msleep(2000); // Update every 2 seconds
        }
    }

    void stop() {
        running = false;
        wait();
    }

signals:
    void dataUpdated(const QJsonObject& data);

private:
    bool running;

    double getCpuUsage() {
        // Read from /proc/stat for CPU usage
        QFile file("/proc/stat");
        if (file.open(QIODevice::ReadOnly)) {
            QString line = file.readLine();
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                long idle = parts[4].toLong();
                long total = 0;
                for (int i = 1; i < parts.size(); ++i) {
                    total += parts[i].toLong();
                }
                return (total - idle) * 100.0 / total;
            }
        }
        return 0.0;
    }

    double getMemoryUsage() {
        // Read from /proc/meminfo
        QFile file("/proc/meminfo");
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&file);
            QString content = stream.readAll();
            
            QRegularExpression totalRegex("MemTotal:\\s+(\\d+)");
            QRegularExpression availRegex("MemAvailable:\\s+(\\d+)");
            
            auto totalMatch = totalRegex.match(content);
            auto availMatch = availRegex.match(content);
            
            if (totalMatch.hasMatch() && availMatch.hasMatch()) {
                long total = totalMatch.captured(1).toLong();
                long available = availMatch.captured(1).toLong();
                return (total - available) * 100.0 / total;
            }
        }
        return 0.0;
    }

    QJsonArray getTemperatures() {
        QJsonArray temps;
        
        // Read thermal zones
        QDir thermalDir("/sys/class/thermal");
        QStringList thermalZones = thermalDir.entryList(QStringList() << "thermal_zone*", QDir::Dirs);
        
        for (const QString& zone : thermalZones) {
            QString tempPath = QString("/sys/class/thermal/%1/temp").arg(zone);
            QString typePath = QString("/sys/class/thermal/%1/type").arg(zone);
            
            QFile tempFile(tempPath);
            QFile typeFile(typePath);
            
            if (tempFile.open(QIODevice::ReadOnly) && typeFile.open(QIODevice::ReadOnly)) {
                int tempMilliC = tempFile.readAll().trimmed().toInt();
                QString type = typeFile.readAll().trimmed();
                
                QJsonObject tempObj;
                tempObj["name"] = type;
                tempObj["temperature"] = tempMilliC / 1000.0;
                temps.append(tempObj);
            }
        }
        
        return temps;
    }

    QJsonArray getFanSpeeds() {
        QJsonArray fans;
        
        // Read from hwmon
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

// Enhanced Control Center Main Window
class EnhancedControlCenter : public QMainWindow {
    Q_OBJECT

public:
    EnhancedControlCenter(QWidget* parent = nullptr) : QMainWindow(parent) {
        rgbController = new EnhancedRGBController("/dev/hidraw1", this);
        systemMonitor = new SystemMonitorThread(this);
        
        setupUI();
        setupSystemTray();
        setupConnections();
        
        // Start system monitoring
        connect(systemMonitor, &SystemMonitorThread::dataUpdated, this, &EnhancedControlCenter::updateSystemData);
        systemMonitor->start();
        
        applyDarkTheme();
    }

    ~EnhancedControlCenter() {
        systemMonitor->stop();
    }

private slots:
    void onQuickColorClicked() {
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (!button) return;
        
        QColor color = button->property("color").value<QColor>();
        rgbController->setAllKeys(color.red(), color.green(), color.blue());
        statusBar()->showMessage(QString("Applied %1 to all keys").arg(color.name()), 2000);
    }

    void onGroupColorClicked() {
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (!button) return;
        
        QString group = button->property("group").toString();
        QColor color = QColorDialog::getColor(Qt::white, this, QString("Select color for %1").arg(group));
        
        if (color.isValid()) {
            rgbController->setGroupColor(group, color.red(), color.green(), color.blue());
            statusBar()->showMessage(QString("Applied %1 to %2").arg(color.name(), group), 2000);
        }
    }

    void onEffectClicked() {
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        if (!button) return;
        
        QString effect = button->property("effect").toString();
        
        if (effect == "rainbow") {
            QThread* effectThread = QThread::create([this] {
                rgbController->rainbowWaveEffect(10);
            });
            effectThread->start();
            statusBar()->showMessage("Rainbow wave effect started", 2000);
        } else if (effect == "breathing_red") {
            QThread* effectThread = QThread::create([this] {
                rgbController->breathingEffect(255, 0, 0, 10);
            });
            effectThread->start();
            statusBar()->showMessage("Red breathing effect started", 2000);
        } else if (effect == "breathing_blue") {
            QThread* effectThread = QThread::create([this] {
                rgbController->breathingEffect(0, 0, 255, 10);
            });
            effectThread->start();
            statusBar()->showMessage("Blue breathing effect started", 2000);
        } else if (effect == "clear") {
            rgbController->clearAllKeys();
            statusBar()->showMessage("All keys cleared", 2000);
        }
    }

    void updateSystemData(const QJsonObject& data) {
        // Update CPU usage
        if (data.contains("cpu_usage")) {
            double cpuUsage = data["cpu_usage"].toDouble();
            cpuUsageLabel->setText(QString("CPU: %1%").arg(cpuUsage, 0, 'f', 1));
        }
        
        // Update memory usage
        if (data.contains("memory_usage")) {
            double memUsage = data["memory_usage"].toDouble();
            memoryUsageLabel->setText(QString("Memory: %1%").arg(memUsage, 0, 'f', 1));
        }
        
        // Update temperatures
        if (data.contains("temperatures")) {
            QJsonArray temps = data["temperatures"].toArray();
            QString tempText = "Temperatures:\n";
            for (const auto& tempValue : temps) {
                QJsonObject tempObj = tempValue.toObject();
                tempText += QString("%1: %2°C\n")
                    .arg(tempObj["name"].toString())
                    .arg(tempObj["temperature"].toDouble(), 0, 'f', 1);
            }
            temperatureDisplay->setPlainText(tempText);
        }
        
        // Update fan speeds
        if (data.contains("fan_speeds")) {
            QJsonArray fans = data["fan_speeds"].toArray();
            QString fanText = "Fan Speeds:\n";
            for (const auto& fanValue : fans) {
                QJsonObject fanObj = fanValue.toObject();
                fanText += QString("%1: %2 RPM\n")
                    .arg(fanObj["name"].toString())
                    .arg(fanObj["rpm"].toInt());
            }
            fanSpeedDisplay->setPlainText(fanText);
        }
    }

private:
    EnhancedRGBController* rgbController;
    SystemMonitorThread* systemMonitor;
    QSystemTrayIcon* trayIcon;
    
    // UI Elements
    QLabel* cpuUsageLabel;
    QLabel* memoryUsageLabel;
    QTextEdit* temperatureDisplay;
    QTextEdit* fanSpeedDisplay;

    void setupUI() {
        setWindowTitle("Enhanced Control Center - OriginPC EON17-X");
        setMinimumSize(800, 600);
        
        auto* centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        
        auto* mainLayout = new QVBoxLayout(centralWidget);
        
        // Header
        auto* headerLabel = new QLabel("🎮 Enhanced OriginPC EON17-X Control Center");
        headerLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #00ff88; text-align: center; padding: 10px;");
        headerLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(headerLabel);
        
        // Create tab widget
        auto* tabWidget = new QTabWidget;
        mainLayout->addWidget(tabWidget);
        
        // RGB Control Tab
        tabWidget->addTab(createRGBTab(), "🌈 RGB Control");
        
        // System Monitor Tab
        tabWidget->addTab(createSystemTab(), "📊 System Monitor");
        
        // Effects Tab
        tabWidget->addTab(createEffectsTab(), "✨ Effects");
        
        // Status bar
        statusBar()->showMessage("Ready - Enhanced Control Center for OriginPC EON17-X");
    }

    QWidget* createRGBTab() {
        auto* widget = new QWidget;
        auto* layout = new QVBoxLayout(widget);
        
        // Quick Colors
        auto* colorsGroup = new QGroupBox("Quick Colors");
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
            button->setProperty("color", colorPair.second);
            button->setStyleSheet(QString("QPushButton { background-color: rgb(%1,%2,%3); color: %4; border: 2px solid #666; "
                                         "border-radius: 8px; padding: 15px; font-weight: bold; font-size: 14px; }"
                                         "QPushButton:hover { border: 2px solid #00ff88; }")
                                 .arg(colorPair.second.red()).arg(colorPair.second.green()).arg(colorPair.second.blue())
                                 .arg(colorPair.second.lightness() > 128 ? "black" : "white"));
            
            connect(button, &QPushButton::clicked, this, &EnhancedControlCenter::onQuickColorClicked);
            colorsLayout->addWidget(button, i / 4, i % 4);
        }
        
        layout->addWidget(colorsGroup);
        
        // Key Groups
        auto* groupsGroup = new QGroupBox("Key Groups");
        auto* groupsLayout = new QGridLayout(groupsGroup);
        
        QStringList groups = {"function_keys", "number_row", "qwerty_row", "asdf_row", "zxcv_row", 
                             "arrow_keys", "keypad", "spacebar_full"};
        
        for (int i = 0; i < groups.size(); ++i) {
            const QString& group = groups[i];
            auto* button = new QPushButton(group.replace("_", " ").toUpper());
            button->setProperty("group", groups[i]); // Keep original name
            button->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                 "stop:0 #4CAF50, stop:1 #45a049); color: white; border: none; "
                                 "border-radius: 8px; padding: 10px; font-weight: bold; }"
                                 "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                 "stop:0 #45a049, stop:1 #3d8b40); }");
            
            connect(button, &QPushButton::clicked, this, &EnhancedControlCenter::onGroupColorClicked);
            groupsLayout->addWidget(button, i / 4, i % 4);
        }
        
        layout->addWidget(groupsGroup);
        
        return widget;
    }

    QWidget* createSystemTab() {
        auto* widget = new QWidget;
        auto* layout = new QVBoxLayout(widget);
        
        // System Info
        auto* infoGroup = new QGroupBox("System Information");
        auto* infoLayout = new QGridLayout(infoGroup);
        
        cpuUsageLabel = new QLabel("CPU: --");
        memoryUsageLabel = new QLabel("Memory: --");
        
        infoLayout->addWidget(new QLabel("System Usage:"), 0, 0);
        infoLayout->addWidget(cpuUsageLabel, 0, 1);
        infoLayout->addWidget(memoryUsageLabel, 0, 2);
        
        layout->addWidget(infoGroup);
        
        // Temperature Monitor
        auto* tempGroup = new QGroupBox("Temperature Monitor");
        auto* tempLayout = new QVBoxLayout(tempGroup);
        
        temperatureDisplay = new QTextEdit;
        temperatureDisplay->setMaximumHeight(150);
        temperatureDisplay->setReadOnly(true);
        tempLayout->addWidget(temperatureDisplay);
        
        layout->addWidget(tempGroup);
        
        // Fan Monitor
        auto* fanGroup = new QGroupBox("Fan Monitor");
        auto* fanLayout = new QVBoxLayout(fanGroup);
        
        fanSpeedDisplay = new QTextEdit;
        fanSpeedDisplay->setMaximumHeight(150);
        fanSpeedDisplay->setReadOnly(true);
        fanLayout->addWidget(fanSpeedDisplay);
        
        layout->addWidget(fanGroup);
        
        return widget;
    }

    QWidget* createEffectsTab() {
        auto* widget = new QWidget;
        auto* layout = new QVBoxLayout(widget);
        
        // Effects
        auto* effectsGroup = new QGroupBox("RGB Effects");
        auto* effectsLayout = new QGridLayout(effectsGroup);
        
        QList<QPair<QString, QString>> effects = {
            {"🌈 Rainbow Wave", "rainbow"},
            {"💓 Breathing Red", "breathing_red"},
            {"💙 Breathing Blue", "breathing_blue"},
            {"🧹 Clear All", "clear"}
        };
        
        for (int i = 0; i < effects.size(); ++i) {
            const auto& effect = effects[i];
            auto* button = new QPushButton(effect.first);
            button->setProperty("effect", effect.second);
            button->setStyleSheet("QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                 "stop:0 #ff6b6b, stop:1 #ee5a52); color: white; border: none; "
                                 "border-radius: 15px; padding: 15px 25px; font-weight: bold; font-size: 14px; }"
                                 "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                 "stop:0 #ff5252, stop:1 #d32f2f); }");
            
            connect(button, &QPushButton::clicked, this, &EnhancedControlCenter::onEffectClicked);
            effectsLayout->addWidget(button, i / 2, i % 2);
        }
        
        layout->addWidget(effectsGroup);
        
        return widget;
    }

    void setupSystemTray() {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QIcon(":/icons/app_icon.png")); // You can add an icon
        
        auto* trayMenu = new QMenu(this);
        trayMenu->addAction("Show", this, &QWidget::show);
        trayMenu->addAction("Hide", this, &QWidget::hide);
        trayMenu->addSeparator();
        trayMenu->addAction("Quit", qApp, &QApplication::quit);
        
        trayIcon->setContextMenu(trayMenu);
        trayIcon->show();
    }

    void setupConnections() {
        // Additional connections can be added here
    }

    void applyDarkTheme() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e1e; color: white; }"
            "QWidget { background-color: #1e1e1e; color: white; }"
            "QTabWidget::pane { border: 1px solid #555; }"
            "QTabWidget::tab-bar { alignment: center; }"
            "QTabBar::tab { background: #2b2b2b; color: white; border: 1px solid #555; "
            "padding: 8px 16px; margin-right: 2px; }"
            "QTabBar::tab:selected { background: #3d3d3d; }"
            "QTabBar::tab:hover { background: #404040; }"
            "QGroupBox { font-weight: bold; border: 2px solid #444; border-radius: 8px; "
            "margin: 10px 0; padding-top: 15px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 8px; }"
            "QLabel { color: white; }"
            "QTextEdit { background: #2b2b2b; border: 1px solid #555; color: white; }"
            "QStatusBar { background: #2b2b2b; color: white; }"
        );
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setApplicationName("Enhanced Control Center");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("OriginPC Tools");
    
    EnhancedControlCenter window;
    window.show();
    
    return app.exec();
}

#include "enhanced_main.moc"
