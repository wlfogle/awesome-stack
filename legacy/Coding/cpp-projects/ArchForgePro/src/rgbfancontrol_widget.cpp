#include "rgbfancontrol_widget.h"
#include <QMessageBox>

RgbFanControlWidget::RgbFanControlWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_currentKeyboardColor(Qt::white)
    , m_currentKeyboardEffect("Static")
    , m_currentFanSpeed(50)
    , m_currentFanProfile("Balanced")
{
    setupUI();
    setupConnections();
}

RgbFanControlWidget::~RgbFanControlWidget()
{
    // Qt handles cleanup automatically
}

void RgbFanControlWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createKeyboardTab(), "Keyboard");
    m_tabWidget->addTab(createFansTab(), "Fans");
    
    mainLayout->addWidget(m_tabWidget);
}

void RgbFanControlWidget::setupConnections()
{
    // Keyboard connections
    connect(m_keyboardBrightnessSlider, &QSlider::valueChanged, this, &RgbFanControlWidget::setKeyboardBrightness);
    connect(m_keyboardColorButton, &QPushButton::clicked, this, &RgbFanControlWidget::setKeyboardColor);
    connect(m_keyboardEffectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RgbFanControlWidget::setKeyboardEffect);
    connect(m_saveKeyboardProfileButton, &QPushButton::clicked, this, &RgbFanControlWidget::saveKeyboardProfile);
    connect(m_loadKeyboardProfileButton, &QPushButton::clicked, this, &RgbFanControlWidget::loadKeyboardProfile);
    
    // Fan connections
    connect(m_fanSpeedSlider, &QSlider::valueChanged, this, &RgbFanControlWidget::setFanSpeed);
    connect(m_fanAutoControlCheck, &QCheckBox::toggled, this, &RgbFanControlWidget::enableFanAutoControl);
    connect(m_saveFanProfileButton, &QPushButton::clicked, this, &RgbFanControlWidget::saveFanProfile);
    connect(m_loadFanProfileButton, &QPushButton::clicked, this, &RgbFanControlWidget::loadFanProfile);
}

QWidget* RgbFanControlWidget::createKeyboardTab()
{
    QWidget *keyboardTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(keyboardTab);
    
    // Brightness control group
    QGroupBox *brightnessGroup = new QGroupBox("Brightness Control");
    QHBoxLayout *brightnessLayout = new QHBoxLayout(brightnessGroup);
    
    brightnessLayout->addWidget(new QLabel("Brightness:"));
    m_keyboardBrightnessSlider = new QSlider(Qt::Horizontal);
    m_keyboardBrightnessSlider->setRange(0, 100);
    m_keyboardBrightnessSlider->setValue(75);
    
    m_keyboardBrightnessSpin = new QSpinBox();
    m_keyboardBrightnessSpin->setRange(0, 100);
    m_keyboardBrightnessSpin->setValue(75);
    m_keyboardBrightnessSpin->setSuffix("%");
    
    brightnessLayout->addWidget(m_keyboardBrightnessSlider);
    brightnessLayout->addWidget(m_keyboardBrightnessSpin);
    
    // Color control group
    QGroupBox *colorGroup = new QGroupBox("Color Control");
    QHBoxLayout *colorLayout = new QHBoxLayout(colorGroup);
    
    colorLayout->addWidget(new QLabel("Color:"));
    m_keyboardColorButton = new QPushButton("Select Color");
    m_keyboardColorButton->setStyleSheet("background-color: white;");
    colorLayout->addWidget(m_keyboardColorButton);
    colorLayout->addStretch();
    
    // Effect control group
    QGroupBox *effectGroup = new QGroupBox("Effect Control");
    QHBoxLayout *effectLayout = new QHBoxLayout(effectGroup);
    
    effectLayout->addWidget(new QLabel("Effect:"));
    m_keyboardEffectCombo = new QComboBox();
    m_keyboardEffectCombo->addItems({"Static", "Breathing", "Wave", "Rainbow", "Reactive", "Spectrum"});
    effectLayout->addWidget(m_keyboardEffectCombo);
    effectLayout->addStretch();
    
    // Profile management group
    QGroupBox *profileGroup = new QGroupBox("Profile Management");
    QHBoxLayout *profileLayout = new QHBoxLayout(profileGroup);
    
    profileLayout->addWidget(new QLabel("Profile:"));
    m_keyboardProfileCombo = new QComboBox();
    m_keyboardProfileCombo->addItems({"Default", "Gaming", "Work", "Custom 1", "Custom 2"});
    
    m_saveKeyboardProfileButton = new QPushButton("Save Profile");
    m_loadKeyboardProfileButton = new QPushButton("Load Profile");
    
    profileLayout->addWidget(m_keyboardProfileCombo);
    profileLayout->addWidget(m_saveKeyboardProfileButton);
    profileLayout->addWidget(m_loadKeyboardProfileButton);
    
    // Status
    m_keyboardStatusLabel = new QLabel("Keyboard RGB: Ready");
    
    layout->addWidget(brightnessGroup);
    layout->addWidget(colorGroup);
    layout->addWidget(effectGroup);
    layout->addWidget(profileGroup);
    layout->addStretch();
    layout->addWidget(m_keyboardStatusLabel);
    
    return keyboardTab;
}

QWidget* RgbFanControlWidget::createFansTab()
{
    QWidget *fansTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(fansTab);
    
    // Fan list group
    QGroupBox *fanListGroup = new QGroupBox("Available Fans");
    QVBoxLayout *fanListLayout = new QVBoxLayout(fanListGroup);
    
    m_fanList = new QListWidget();
    m_fanList->addItems({"CPU Fan", "Case Fan 1", "Case Fan 2", "GPU Fan"});
    fanListLayout->addWidget(m_fanList);
    
    // Speed control group
    QGroupBox *speedGroup = new QGroupBox("Fan Speed Control");
    QVBoxLayout *speedLayout = new QVBoxLayout(speedGroup);
    
    QHBoxLayout *speedControlLayout = new QHBoxLayout();
    speedControlLayout->addWidget(new QLabel("Speed:"));
    
    m_fanSpeedSlider = new QSlider(Qt::Horizontal);
    m_fanSpeedSlider->setRange(0, 100);
    m_fanSpeedSlider->setValue(50);
    
    m_fanSpeedSpin = new QSpinBox();
    m_fanSpeedSpin->setRange(0, 100);
    m_fanSpeedSpin->setValue(50);
    m_fanSpeedSpin->setSuffix("%");
    
    speedControlLayout->addWidget(m_fanSpeedSlider);
    speedControlLayout->addWidget(m_fanSpeedSpin);
    
    // Auto control
    m_fanAutoControlCheck = new QCheckBox("Enable Automatic Fan Control");
    m_fanAutoControlCheck->setChecked(true);
    
    // RPM display
    QHBoxLayout *rpmLayout = new QHBoxLayout();
    rpmLayout->addWidget(new QLabel("Current RPM:"));
    m_fanRpmBar = new QProgressBar();
    m_fanRpmBar->setRange(0, 3000);
    m_fanRpmBar->setValue(1500);
    m_fanRpmBar->setFormat("%v RPM");
    rpmLayout->addWidget(m_fanRpmBar);
    
    speedLayout->addLayout(speedControlLayout);
    speedLayout->addWidget(m_fanAutoControlCheck);
    speedLayout->addLayout(rpmLayout);
    
    // Profile management group
    QGroupBox *profileGroup = new QGroupBox("Fan Profile Management");
    QHBoxLayout *profileLayout = new QHBoxLayout(profileGroup);
    
    profileLayout->addWidget(new QLabel("Profile:"));
    m_fanProfileCombo = new QComboBox();
    m_fanProfileCombo->addItems({"Silent", "Balanced", "Performance", "Custom"});
    m_fanProfileCombo->setCurrentText("Balanced");
    
    m_saveFanProfileButton = new QPushButton("Save Profile");
    m_loadFanProfileButton = new QPushButton("Load Profile");
    
    profileLayout->addWidget(m_fanProfileCombo);
    profileLayout->addWidget(m_saveFanProfileButton);
    profileLayout->addWidget(m_loadFanProfileButton);
    
    // Status
    m_fanStatusLabel = new QLabel("Fan Control: Active");
    
    layout->addWidget(fanListGroup);
    layout->addWidget(speedGroup);
    layout->addWidget(profileGroup);
    layout->addStretch();
    layout->addWidget(m_fanStatusLabel);
    
    return fansTab;
}

// Keyboard RGB slots
void RgbFanControlWidget::setKeyboardBrightness(int brightness)
{
    m_keyboardBrightnessSpin->setValue(brightness);
    m_keyboardStatusLabel->setText(QString("Keyboard Brightness: %1%").arg(brightness));
    // TODO: Implement actual brightness control
}

void RgbFanControlWidget::setKeyboardColor()
{
    QColor color = QColorDialog::getColor(m_currentKeyboardColor, this, "Select Keyboard Color");
    if (color.isValid()) {
        m_currentKeyboardColor = color;
        m_keyboardColorButton->setStyleSheet(QString("background-color: %1;").arg(color.name()));
        // TODO: Implement actual color control
    }
}

void RgbFanControlWidget::setKeyboardEffect()
{
    m_currentKeyboardEffect = m_keyboardEffectCombo->currentText();
    m_keyboardStatusLabel->setText(QString("Keyboard Effect: %1").arg(m_currentKeyboardEffect));
    // TODO: Implement actual effect control
}

void RgbFanControlWidget::saveKeyboardProfile()
{
    QMessageBox::information(this, "Save Profile", "Keyboard profile saved successfully!");
    // TODO: Implement profile saving
}

void RgbFanControlWidget::loadKeyboardProfile()
{
    QMessageBox::information(this, "Load Profile", "Keyboard profile loaded successfully!");
    // TODO: Implement profile loading
}

// Fan control slots
void RgbFanControlWidget::setFanSpeed(int speed)
{
    m_fanSpeedSpin->setValue(speed);
    m_currentFanSpeed = speed;
    m_fanStatusLabel->setText(QString("Fan Speed: %1%").arg(speed));
    // TODO: Implement actual fan speed control
}

void RgbFanControlWidget::setFanProfile()
{
    m_currentFanProfile = m_fanProfileCombo->currentText();
    m_fanStatusLabel->setText(QString("Fan Profile: %1").arg(m_currentFanProfile));
    // TODO: Implement profile application
}

void RgbFanControlWidget::enableFanAutoControl(bool enabled)
{
    m_fanSpeedSlider->setEnabled(!enabled);
    m_fanSpeedSpin->setEnabled(!enabled);
    
    if (enabled) {
        m_fanStatusLabel->setText("Fan Control: Automatic");
    } else {
        m_fanStatusLabel->setText("Fan Control: Manual");
    }
    // TODO: Implement auto control toggle
}

void RgbFanControlWidget::saveFanProfile()
{
    QMessageBox::information(this, "Save Profile", "Fan profile saved successfully!");
    // TODO: Implement profile saving
}

void RgbFanControlWidget::loadFanProfile()
{
    QMessageBox::information(this, "Load Profile", "Fan profile loaded successfully!");
    // TODO: Implement profile loading
}
