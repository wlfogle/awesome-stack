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
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QParallelAnimationGroup>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>
#include <random>

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
        // Try hidraw1 first as specified
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
        
        // Simple command structure for all keys
        QByteArray basicCommand(16, 0x00);
        basicCommand[0] = 0xCC;
        basicCommand[1] = 0x01;
        basicCommand[3] = static_cast<char>(red);
        basicCommand[4] = static_cast<char>(green);
        basicCommand[5] = static_cast<char>(blue);
        
        // Write command to all keys
        for (int keyIndex = 0; keyIndex <= 0xFF; ++keyIndex) {
            basicCommand[2] = static_cast<char>(keyIndex);
            hidrawFile.write(basicCommand);
        }
        
        hidrawFile.close();
        qDebug() << "Set all keys to RGB:" << red << green << blue << "via" << devicePath;
    }
    
    void clearAllKeys() {
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
        
        QByteArray clearCommand(16, 0x00);
        clearCommand[0] = 0xCC;
        clearCommand[1] = 0x01;
        
        for (int keyIndex = 0x00; keyIndex <= 0xFF; ++keyIndex) {
            clearCommand[2] = static_cast<char>(keyIndex);
            hidrawFile.write(clearCommand);
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
