#ifndef RGBFANCONTROL_WIDGET_H
#define RGBFANCONTROL_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QProgressBar>
#include <QListWidget>

class RgbFanControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RgbFanControlWidget(QWidget *parent = nullptr);
    ~RgbFanControlWidget();

private slots:
    // Keyboard RGB slots
    void setKeyboardBrightness(int brightness);
    void setKeyboardColor();
    void setKeyboardEffect();
    void saveKeyboardProfile();
    void loadKeyboardProfile();
    
    // Fan control slots
    void setFanSpeed(int speed);
    void setFanProfile();
    void enableFanAutoControl(bool enabled);
    void saveFanProfile();
    void loadFanProfile();

private:
    void setupUI();
    void setupConnections();
    
    // Tab creation methods
    QWidget* createKeyboardTab();
    QWidget* createFansTab();
    
    // Main components
    QTabWidget *m_tabWidget;
    
    // Keyboard RGB components
    QSlider *m_keyboardBrightnessSlider;
    QSpinBox *m_keyboardBrightnessSpin;
    QPushButton *m_keyboardColorButton;
    QComboBox *m_keyboardEffectCombo;
    QComboBox *m_keyboardProfileCombo;
    QPushButton *m_saveKeyboardProfileButton;
    QPushButton *m_loadKeyboardProfileButton;
    QLabel *m_keyboardStatusLabel;
    
    // Fan control components
    QSlider *m_fanSpeedSlider;
    QSpinBox *m_fanSpeedSpin;
    QComboBox *m_fanProfileCombo;
    QCheckBox *m_fanAutoControlCheck;
    QPushButton *m_saveFanProfileButton;
    QPushButton *m_loadFanProfileButton;
    QProgressBar *m_fanRpmBar;
    QLabel *m_fanStatusLabel;
    QListWidget *m_fanList;
    
    // Current settings
    QColor m_currentKeyboardColor;
    QString m_currentKeyboardEffect;
    int m_currentFanSpeed;
    QString m_currentFanProfile;
};

#endif // RGBFANCONTROL_WIDGET_H
