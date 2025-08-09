#include "spatialeffects.h"
#include "rgbcommandbatcher.h"
#include <QDebug>
#include <qmath.h>

SpatialEffects::SpatialEffects(RGBCommandBatcher *batcher, QObject *parent)
    : QObject(parent)
    , m_batcher(batcher)
    , m_timer(new QTimer(this))
    , m_running(false)
    , m_speed(1.0f)
    , m_brightness(1.0f)
    , m_time(0.0f)
{
    initializeKeyboardLayout();
    
    m_timer->setInterval(UPDATE_INTERVAL);
    connect(m_timer, &QTimer::timeout, this, &SpatialEffects::updateEffect);
}

void SpatialEffects::initializeKeyboardLayout()
{
    // Define physical keyboard layout with proper spatial coordinates
    // This maps keys to their physical positions on a standard keyboard
    
    // Row 0: Function keys
    m_keyLayout["esc"] = {0, 0, 0.0f, 0.0f, 0x01};
    m_keyLayout["f1"] = {0, 2, 0.15f, 0.0f, 0x3B};
    m_keyLayout["f2"] = {0, 3, 0.20f, 0.0f, 0x3C};
    m_keyLayout["f3"] = {0, 4, 0.25f, 0.0f, 0x3D};
    m_keyLayout["f4"] = {0, 5, 0.30f, 0.0f, 0x3E};
    m_keyLayout["f5"] = {0, 7, 0.40f, 0.0f, 0x3F};
    m_keyLayout["f6"] = {0, 8, 0.45f, 0.0f, 0x40};
    m_keyLayout["f7"] = {0, 9, 0.50f, 0.0f, 0x41};
    m_keyLayout["f8"] = {0, 10, 0.55f, 0.0f, 0x42};
    m_keyLayout["f9"] = {0, 12, 0.65f, 0.0f, 0x43};
    m_keyLayout["f10"] = {0, 13, 0.70f, 0.0f, 0x44};
    m_keyLayout["f11"] = {0, 14, 0.75f, 0.0f, 0x57};
    m_keyLayout["f12"] = {0, 15, 0.80f, 0.0f, 0x58};

    // Row 1: Number row
    m_keyLayout["1"] = {1, 1, 0.05f, 0.2f, 0x02};
    m_keyLayout["2"] = {1, 2, 0.10f, 0.2f, 0x03};
    m_keyLayout["3"] = {1, 3, 0.15f, 0.2f, 0x04};
    m_keyLayout["4"] = {1, 4, 0.20f, 0.2f, 0x05};
    m_keyLayout["5"] = {1, 5, 0.25f, 0.2f, 0x06};
    m_keyLayout["6"] = {1, 6, 0.30f, 0.2f, 0x07};
    m_keyLayout["7"] = {1, 7, 0.35f, 0.2f, 0x08};
    m_keyLayout["8"] = {1, 8, 0.40f, 0.2f, 0x09};
    m_keyLayout["9"] = {1, 9, 0.45f, 0.2f, 0x0A};
    m_keyLayout["0"] = {1, 10, 0.50f, 0.2f, 0x0B};

    // Row 2: QWERTY row
    m_keyLayout["tab"] = {2, 0, 0.0f, 0.35f, 0x0F};
    m_keyLayout["q"] = {2, 1, 0.08f, 0.35f, 0x10};
    m_keyLayout["w"] = {2, 2, 0.13f, 0.35f, 0x11};
    m_keyLayout["e"] = {2, 3, 0.18f, 0.35f, 0x12};
    m_keyLayout["r"] = {2, 4, 0.23f, 0.35f, 0x13};
    m_keyLayout["t"] = {2, 5, 0.28f, 0.35f, 0x14};
    m_keyLayout["y"] = {2, 6, 0.33f, 0.35f, 0x15};
    m_keyLayout["u"] = {2, 7, 0.38f, 0.35f, 0x16};
    m_keyLayout["i"] = {2, 8, 0.43f, 0.35f, 0x17};
    m_keyLayout["o"] = {2, 9, 0.48f, 0.35f, 0x18};
    m_keyLayout["p"] = {2, 10, 0.53f, 0.35f, 0x19};

    // Row 3: ASDF row  
    m_keyLayout["capslock"] = {3, 0, 0.0f, 0.5f, 0x3A};
    m_keyLayout["a"] = {3, 1, 0.09f, 0.5f, 0x1E};
    m_keyLayout["s"] = {3, 2, 0.14f, 0.5f, 0x1F};
    m_keyLayout["d"] = {3, 3, 0.19f, 0.5f, 0x20};
    m_keyLayout["f"] = {3, 4, 0.24f, 0.5f, 0x21};
    m_keyLayout["g"] = {3, 5, 0.29f, 0.5f, 0x22};
    m_keyLayout["h"] = {3, 6, 0.34f, 0.5f, 0x23};
    m_keyLayout["j"] = {3, 7, 0.39f, 0.5f, 0x24};
    m_keyLayout["k"] = {3, 8, 0.44f, 0.5f, 0x25};
    m_keyLayout["l"] = {3, 9, 0.49f, 0.5f, 0x26};
    m_keyLayout["enter"] = {3, 11, 0.60f, 0.5f, 0x1C};

    // Row 4: ZXCV row
    m_keyLayout["shift"] = {4, 0, 0.0f, 0.65f, 0x2A};
    m_keyLayout["z"] = {4, 2, 0.12f, 0.65f, 0x2C};
    m_keyLayout["x"] = {4, 3, 0.17f, 0.65f, 0x2D};
    m_keyLayout["c"] = {4, 4, 0.22f, 0.65f, 0x2E};
    m_keyLayout["v"] = {4, 5, 0.27f, 0.65f, 0x2F};
    m_keyLayout["b"] = {4, 6, 0.32f, 0.65f, 0x30};
    m_keyLayout["n"] = {4, 7, 0.37f, 0.65f, 0x31};
    m_keyLayout["m"] = {4, 8, 0.42f, 0.65f, 0x32};

    // Row 5: Bottom row
    m_keyLayout["ctrl"] = {5, 0, 0.0f, 0.8f, 0x1D};
    m_keyLayout["alt"] = {5, 2, 0.15f, 0.8f, 0x38};
    m_keyLayout["space"] = {5, 5, 0.30f, 0.8f, 0x39};

    // Arrow keys
    m_keyLayout["left"] = {5, 13, 0.75f, 0.8f, 0xCB};
    m_keyLayout["down"] = {5, 14, 0.80f, 0.8f, 0xD0};
    m_keyLayout["right"] = {5, 15, 0.85f, 0.8f, 0xCD};
    m_keyLayout["up"] = {4, 14, 0.80f, 0.65f, 0xC8};

    // Numeric keypad
    m_keyLayout["kp_7"] = {1, 17, 0.90f, 0.2f, 0x47};
    m_keyLayout["kp_8"] = {1, 18, 0.95f, 0.2f, 0x48};
    m_keyLayout["kp_9"] = {1, 19, 1.0f, 0.2f, 0x49};
    m_keyLayout["kp_4"] = {2, 17, 0.90f, 0.35f, 0x4B};
    m_keyLayout["kp_5"] = {2, 18, 0.95f, 0.35f, 0x4C};
    m_keyLayout["kp_6"] = {2, 19, 1.0f, 0.35f, 0x4D};
    m_keyLayout["kp_1"] = {3, 17, 0.90f, 0.5f, 0x4F};
    m_keyLayout["kp_2"] = {3, 18, 0.95f, 0.5f, 0x50};
    m_keyLayout["kp_3"] = {3, 19, 1.0f, 0.5f, 0x51};
    m_keyLayout["kp_0"] = {4, 17, 0.90f, 0.65f, 0x52};
    m_keyLayout["kp_decimal"] = {4, 18, 0.95f, 0.65f, 0x4E};
    m_keyLayout["kp_plus"] = {2, 20, 1.05f, 0.35f, 0x53};
    m_keyLayout["kp_minus"] = {1, 20, 1.05f, 0.2f, 0x4A};
    m_keyLayout["kp_enter"] = {4, 20, 1.05f, 0.65f, 0x9C};

    // Set escape as the wave origin
    m_escapeKey = m_keyLayout["esc"];
    
    qDebug() << "Initialized spatial keyboard layout with" << m_keyLayout.size() << "keys";
}

void SpatialEffects::startWaveEffect(const QColor &color, float speed, float brightness)
{
    stopEffect();
    
    m_currentEffect = "wave";
    m_primaryColor = color;
    m_speed = speed;
    m_brightness = brightness;
    m_time = 0.0f;
    m_running = true;
    
    m_timer->start();
    qDebug() << "Started spatial wave effect from ESC key";
}

void SpatialEffects::startRainbowWave(float speed, float brightness)
{
    stopEffect();
    
    m_currentEffect = "rainbow";
    m_speed = speed;
    m_brightness = brightness;
    m_time = 0.0f;
    m_running = true;
    
    m_timer->start();
    qDebug() << "Started rainbow wave effect";
}

void SpatialEffects::startBreathingEffect(const QColor &color, float speed, float brightness)
{
    stopEffect();
    
    m_currentEffect = "breathing";
    m_primaryColor = color;
    m_speed = speed;
    m_brightness = brightness;
    m_time = 0.0f;
    m_running = true;
    
    m_timer->start();
    qDebug() << "Started breathing effect";
}

void SpatialEffects::startRippleEffect(const QColor &color, float speed, float brightness)
{
    stopEffect();
    
    m_currentEffect = "ripple";
    m_primaryColor = color;
    m_speed = speed;
    m_brightness = brightness;
    m_time = 0.0f;
    m_running = true;
    
    m_timer->start();
    qDebug() << "Started ripple effect from ESC key";
}

void SpatialEffects::stopEffect()
{
    if (m_running) {
        m_timer->stop();
        m_running = false;
        emit effectFinished();
        qDebug() << "Stopped spatial effect";
    }
}

void SpatialEffects::updateEffect()
{
    if (!m_running || !m_batcher || !m_batcher->isRunning()) {
        stopEffect();
        return;
    }
    
    m_time += UPDATE_INTERVAL / 1000.0f * m_speed;
    
    if (m_currentEffect == "wave") {
        applyWaveEffect();
    } else if (m_currentEffect == "rainbow") {
        applyRainbowWave();
    } else if (m_currentEffect == "breathing") {
        applyBreathingEffect();
    } else if (m_currentEffect == "ripple") {
        applyRippleEffect();
    }
}

void SpatialEffects::applyWaveEffect()
{
    // Calculate wave propagation from ESC key
    float waveSpeed = 0.5f; // Wave travels at 0.5 units per second
    float waveLength = 0.3f; // Wave length in spatial units
    
    for (auto it = m_keyLayout.begin(); it != m_keyLayout.end(); ++it) {
        const KeyPosition &keyPos = it.value();
        
        // Calculate distance from ESC key
        float distance = calculateDistance(m_escapeKey, keyPos);
        
        // Calculate wave phase at this position
        float phase = (distance / waveLength) - (m_time * waveSpeed);
        float amplitude = qCos(phase * 2.0f * M_PI);
        
        // Apply distance falloff
        float falloff = qExp(-distance * 1.5f);
        amplitude *= falloff;
        
        // Ensure amplitude is positive (wave intensity)
        amplitude = qMax(0.0f, amplitude);
        
        // Apply color with wave intensity
        int red = static_cast<int>(m_primaryColor.red() * amplitude * m_brightness);
        int green = static_cast<int>(m_primaryColor.green() * amplitude * m_brightness);
        int blue = static_cast<int>(m_primaryColor.blue() * amplitude * m_brightness);
        
        m_batcher->addCommand(keyPos.keyIndex, red, green, blue, 0);
    }
}

void SpatialEffects::applyRainbowWave()
{
    float waveSpeed = 0.3f;
    float colorCycle = 2.0f; // Full color cycle in 2 seconds
    
    for (auto it = m_keyLayout.begin(); it != m_keyLayout.end(); ++it) {
        const KeyPosition &keyPos = it.value();
        
        // Calculate distance from ESC key
        float distance = calculateDistance(m_escapeKey, keyPos);
        
        // Calculate hue based on distance and time
        float hue = fmod((distance * 200.0f) + (m_time * colorCycle * 360.0f), 360.0f);
        
        // Calculate wave intensity
        float phase = distance - (m_time * waveSpeed);
        float amplitude = 0.5f + 0.5f * qCos(phase * 8.0f);
        
        // Apply distance falloff
        float falloff = qExp(-distance * 1.0f);
        amplitude *= falloff * m_brightness;
        
        // Convert HSV to RGB
        QColor color = QColor::fromHsvF(hue / 360.0f, 1.0f, amplitude);
        
        m_batcher->addCommand(keyPos.keyIndex, color.red(), color.green(), color.blue(), 0);
    }
}

void SpatialEffects::applyBreathingEffect()
{
    // Global breathing effect
    float breathCycle = 3.0f; // 3 second breath cycle
    float phase = fmod(m_time / breathCycle, 1.0f);
    
    // Create smooth breathing curve
    float intensity = 0.3f + 0.7f * (0.5f + 0.5f * qCos(phase * 2.0f * M_PI));
    intensity *= m_brightness;
    
    int red = static_cast<int>(m_primaryColor.red() * intensity);
    int green = static_cast<int>(m_primaryColor.green() * intensity);
    int blue = static_cast<int>(m_primaryColor.blue() * intensity);
    
    // Apply to all keys
    for (auto it = m_keyLayout.begin(); it != m_keyLayout.end(); ++it) {
        const KeyPosition &keyPos = it.value();
        m_batcher->addCommand(keyPos.keyIndex, red, green, blue, 0);
    }
}

void SpatialEffects::applyRippleEffect()
{
    // Multiple ripples emanating from ESC
    float rippleSpeed = 0.8f;
    float rippleInterval = 1.5f; // New ripple every 1.5 seconds
    
    for (auto it = m_keyLayout.begin(); it != m_keyLayout.end(); ++it) {
        const KeyPosition &keyPos = it.value();
        
        float distance = calculateDistance(m_escapeKey, keyPos);
        float totalIntensity = 0.0f;
        
        // Calculate multiple ripples
        for (int ripple = 0; ripple < 3; ++ripple) {
            float rippleTime = m_time - (ripple * rippleInterval);
            if (rippleTime < 0) continue;
            
            float rippleDistance = rippleTime * rippleSpeed;
            float distanceDiff = qAbs(distance - rippleDistance);
            
            if (distanceDiff < 0.1f) {
                float intensity = qExp(-distanceDiff * 20.0f);
                totalIntensity += intensity;
            }
        }
        
        totalIntensity = qMin(1.0f, totalIntensity) * m_brightness;
        
        int red = static_cast<int>(m_primaryColor.red() * totalIntensity);
        int green = static_cast<int>(m_primaryColor.green() * totalIntensity);
        int blue = static_cast<int>(m_primaryColor.blue() * totalIntensity);
        
        m_batcher->addCommand(keyPos.keyIndex, red, green, blue, 0);
    }
}

float SpatialEffects::calculateDistance(const KeyPosition &key1, const KeyPosition &key2)
{
    float dx = key1.x - key2.x;
    float dy = key1.y - key2.y;
    return qSqrt(dx * dx + dy * dy);
}

QColor SpatialEffects::interpolateColor(const QColor &color1, const QColor &color2, float t)
{
    t = qBound(0.0f, t, 1.0f);
    
    int red = static_cast<int>(color1.red() * (1.0f - t) + color2.red() * t);
    int green = static_cast<int>(color1.green() * (1.0f - t) + color2.green() * t);
    int blue = static_cast<int>(color1.blue() * (1.0f - t) + color2.blue() * t);
    
    return QColor(red, green, blue);
}

QColor SpatialEffects::waveColorAt(float distance, float time)
{
    float hue = fmod((distance * 100.0f) + (time * 50.0f), 360.0f);
    return QColor::fromHsvF(hue / 360.0f, 1.0f, 1.0f);
}
