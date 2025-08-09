#ifndef ENHANCED_RGB_CONTROLLER_H
#define ENHANCED_RGB_CONTROLLER_H

#include <QtCore>
#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QDateTime>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <random>
#include <cmath>
#include <atomic>

class EnhancedRGBController : public QObject {
    Q_OBJECT

public:
    enum EffectType {
        RAINBOW_WAVE,
        BREATHING,
        REACTIVE,
        RIPPLE,
        WAVE,
        STATIC,
        CUSTOM,
        DISCO,
        FIRE,
        MATRIX,
        GRADIENT,
        SNAKE
    };

    struct RGBProfile {
        QString name;
        EffectType effect;
        QColor primaryColor;
        QColor secondaryColor;
        int speed;
        int brightness;
        QJsonObject customSettings;
    };

    struct KeyPosition {
        int row;
        int col;
        int index;
        QString name;
    };

    EnhancedRGBController(const QString &devicePath = "/dev/hidraw1", QObject* parent = nullptr);
    ~EnhancedRGBController();

    // Basic Controls
    bool checkPermissions() const;
    bool sendKeyCommand(int keyIndex, int red, int green, int blue);
    bool setKeyColor(const QString &keyName, int red, int green, int blue);
    bool setGroupColor(const QString &groupName, int red, int green, int blue);
    void setAllKeys(int red, int green, int blue);
    void clearAllKeys();
    
    // Advanced Effects
    void startEffect(EffectType effect, const QJsonObject &settings = QJsonObject());
    void stopEffect();
    void rainbowWaveEffect(int duration = 20, int speed = 50);
    void breathingEffect(int red, int green, int blue, int duration = 10, int speed = 100);
    void reactiveEffect(const QColor &color, int fadeTime = 2000);
    void rippleEffect(const QColor &centerColor, const QColor &outerColor, int speed = 50);
    void waveEffect(const QString &direction = "horizontal", const QColor &color = QColor(255, 0, 255), int speed = 50);
    void discoEffect(int duration = 30);
    void fireEffect(int duration = 30);
    void matrixEffect(const QColor &color = QColor(0, 255, 0), int duration = 30);
    void gradientEffect(const QColor &startColor, const QColor &endColor, const QString &direction = "horizontal");
    void snakeEffect(const QColor &color = QColor(255, 255, 0), int speed = 100);
    
    // Profile Management
    void saveProfile(const QString &name, const RGBProfile &profile);
    RGBProfile loadProfile(const QString &name);
    QStringList getAvailableProfiles();
    void applyProfile(const QString &name);
    
    // Key Mapping and Groups
    QMap<QString, int> getKeyboardMap() const { return keyboardMap; }
    QMap<QString, QList<QString>> getKeyGroups() const { return keyGroups; }
    QList<KeyPosition> getKeyPositions() const { return keyPositions; }
    
    // Reactive to system events
    void setReactiveMode(bool enabled) { reactiveMode = enabled; }
    void onKeyPress(const QString &keyName);
    void onSystemEvent(const QString &eventType, const QJsonObject &data);
    
    // Performance settings
    void setUpdateRate(int hz) { updateRate = hz; }
    void setBrightness(int brightness) { globalBrightness = qBound(0, brightness, 255); }
    int getBrightness() const { return globalBrightness; }
    
public slots:
    void processEffectFrame();
    void handleReactiveKey(const QString &keyName);
    
signals:
    void effectStarted(EffectType effect);
    void effectStopped();
    void profileApplied(const QString &name);
    void keyPressed(const QString &keyName);
    
private slots:
    void updateEffect();
    
private:
    QString devicePath;
    QMap<QString, int> keyboardMap;
    QMap<QString, QList<QString>> keyGroups;
    QList<KeyPosition> keyPositions;
    QMap<QString, RGBProfile> profiles;
    
    // Effect system
    QTimer* effectTimer;
    std::atomic<bool> effectRunning;
    EffectType currentEffect;
    QJsonObject currentEffectSettings;
    qint64 effectStartTime;
    int updateRate;
    int globalBrightness;
    bool reactiveMode;
    
    // Thread safety
    QMutex deviceMutex;
    
    // Random generator for effects
    std::mt19937 randomGen;
    std::uniform_real_distribution<float> randomFloat;
    
    void initializeKeyMappings();
    void initializeKeyPositions();
    std::tuple<int, int, int> hsvToRgb(float h, float s, float v) const;
    std::tuple<float, float, float> rgbToHsv(int r, int g, int b) const;
    QColor interpolateColors(const QColor &color1, const QColor &color2, float ratio) const;
    
    // Effect implementations
    void updateRainbowWave();
    void updateBreathing();
    void updateReactive();
    void updateRipple();
    void updateWave();
    void updateDisco();
    void updateFire();
    void updateMatrix();
    void updateGradient();
    void updateSnake();
    
    // Hardware communication
    bool writeRawCommand(const QByteArray &command);
    void applyBrightnessToColor(int &r, int &g, int &b) const;
    
    // Key layout helpers
    KeyPosition getKeyPosition(const QString &keyName) const;
    QList<QString> getAdjacentKeys(const QString &keyName) const;
    float getKeyDistance(const QString &key1, const QString &key2) const;
};

#endif // ENHANCED_RGB_CONTROLLER_H
