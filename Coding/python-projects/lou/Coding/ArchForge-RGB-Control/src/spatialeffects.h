#ifndef SPATIALEFFECTS_H
#define SPATIALEFFECTS_H

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QPoint>
#include <QMap>
#include <QMath>

class RGBCommandBatcher;

struct KeyPosition {
    int row;
    int col;
    float x;  // Physical X position (0-1)
    float y;  // Physical Y position (0-1)
    int keyIndex;
};

class SpatialEffects : public QObject
{
    Q_OBJECT

public:
    explicit SpatialEffects(RGBCommandBatcher *batcher, QObject *parent = nullptr);

    void startWaveEffect(const QColor &color, float speed = 1.0f, float brightness = 1.0f);
    void startRainbowWave(float speed = 1.0f, float brightness = 1.0f);
    void startBreathingEffect(const QColor &color, float speed = 1.0f, float brightness = 1.0f);
    void startRippleEffect(const QColor &color, float speed = 1.0f, float brightness = 1.0f);
    void stopEffect();

    bool isRunning() const { return m_running; }

signals:
    void effectFinished();

private slots:
    void updateEffect();

private:
    void initializeKeyboardLayout();
    void applyWaveEffect();
    void applyRainbowWave();
    void applyBreathingEffect();
    void applyRippleEffect();
    
    float calculateDistance(const KeyPosition &key1, const KeyPosition &key2);
    QColor interpolateColor(const QColor &color1, const QColor &color2, float t);
    QColor waveColorAt(float distance, float time);

private:
    RGBCommandBatcher *m_batcher;
    QTimer *m_timer;
    
    // Effect parameters
    bool m_running;
    QString m_currentEffect;
    QColor m_primaryColor;
    float m_speed;
    float m_brightness;
    float m_time;
    
    // Keyboard layout
    QMap<QString, KeyPosition> m_keyLayout;
    KeyPosition m_escapeKey;  // Wave origin
    
    // Effect update interval
    static const int UPDATE_INTERVAL = 50; // 20 FPS
};

#endif // SPATIALEFFECTS_H
