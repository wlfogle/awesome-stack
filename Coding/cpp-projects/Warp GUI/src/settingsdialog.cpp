#include "settingsdialog.h"
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QColorDialog>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_tabWidget(nullptr)
    , m_settings(new QSettings(this))
{
    setWindowTitle("Settings");
    setFixedSize(600, 500);
    setModal(true);
    
    // Initialize colors with defaults
    m_backgroundColor = QColor(25, 25, 25);
    m_textColor = QColor(255, 255, 255);
    m_promptColor = QColor(42, 130, 218);
    
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupGeneralTab();
    setupAppearanceTab();
    setupTerminalTab();
    setupShortcutsTab();
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_resetButton = new QPushButton("Reset to Defaults");
    m_applyButton = new QPushButton("Apply");
    m_cancelButton = new QPushButton("Cancel");
    m_okButton = new QPushButton("OK");
    
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect buttons
    connect(m_okButton, &QPushButton::clicked, [this]() {
        applySettings();
        accept();
    });
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::applySettings);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::resetToDefaults);
}

void SettingsDialog::setupGeneralTab()
{
    QWidget *generalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(generalTab);
    
    // Shell settings
    QGroupBox *shellGroup = new QGroupBox("Shell Settings");
    QFormLayout *shellLayout = new QFormLayout(shellGroup);
    
    m_shellComboBox = new QComboBox();
    m_shellComboBox->addItems({"fish", "bash", "zsh", "sh"});
    shellLayout->addRow("Default Shell:", m_shellComboBox);
    
    QHBoxLayout *workingDirLayout = new QHBoxLayout();
    m_workingDirEdit = new QLineEdit();
    m_workingDirEdit->setText(QDir::currentPath());
    m_browseDirButton = new QPushButton("Browse...");
    workingDirLayout->addWidget(m_workingDirEdit);
    workingDirLayout->addWidget(m_browseDirButton);
    shellLayout->addRow("Working Directory:", workingDirLayout);
    
    connect(m_browseDirButton, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Working Directory", 
                                                       m_workingDirEdit->text());
        if (!dir.isEmpty()) {
            m_workingDirEdit->setText(dir);
        }
    });
    
    layout->addWidget(shellGroup);
    
    // Startup settings
    QGroupBox *startupGroup = new QGroupBox("Startup Settings");
    QFormLayout *startupLayout = new QFormLayout(startupGroup);
    
    m_startupTabCheckBox = new QCheckBox("Open new tab on startup");
    m_startupTabCheckBox->setChecked(true);
    startupLayout->addRow(m_startupTabCheckBox);
    
    m_historyLimitSpinBox = new QSpinBox();
    m_historyLimitSpinBox->setRange(100, 10000);
    m_historyLimitSpinBox->setValue(1000);
    m_historyLimitSpinBox->setSuffix(" commands");
    startupLayout->addRow("Command History Limit:", m_historyLimitSpinBox);
    
    layout->addWidget(startupGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(generalTab, "General");
}

void SettingsDialog::setupAppearanceTab()
{
    QWidget *appearanceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(appearanceTab);
    
    // Font settings
    QGroupBox *fontGroup = new QGroupBox("Font Settings");
    QFormLayout *fontLayout = new QFormLayout(fontGroup);
    
    m_fontComboBox = new QFontComboBox();
    m_fontComboBox->setCurrentFont(QFont("monospace"));
    fontLayout->addRow("Font Family:", m_fontComboBox);
    
    m_fontSizeSpinBox = new QSpinBox();
    m_fontSizeSpinBox->setRange(8, 72);
    m_fontSizeSpinBox->setValue(10);
    fontLayout->addRow("Font Size:", m_fontSizeSpinBox);
    
    m_boldFontCheckBox = new QCheckBox("Bold Font");
    fontLayout->addRow(m_boldFontCheckBox);
    
    connect(m_fontComboBox, &QFontComboBox::currentFontChanged, this, &SettingsDialog::onFontChanged);
    connect(m_fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onFontChanged);
    connect(m_boldFontCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onFontChanged);
    
    layout->addWidget(fontGroup);
    
    // Theme settings
    QGroupBox *themeGroup = new QGroupBox("Theme Settings");
    QFormLayout *themeLayout = new QFormLayout(themeGroup);
    
    m_themeComboBox = new QComboBox();
    m_themeComboBox->addItems({"Dark", "Light", "Custom"});
    themeLayout->addRow("Theme:", m_themeComboBox);
    
    // Color buttons
    m_backgroundColorButton = new QPushButton();
    m_backgroundColorButton->setFixedSize(60, 30);
    m_backgroundColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_backgroundColor.name()));
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &SettingsDialog::selectBackgroundColor);
    themeLayout->addRow("Background Color:", m_backgroundColorButton);
    
    m_textColorButton = new QPushButton();
    m_textColorButton->setFixedSize(60, 30);
    m_textColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_textColor.name()));
    connect(m_textColorButton, &QPushButton::clicked, this, &SettingsDialog::selectTextColor);
    themeLayout->addRow("Text Color:", m_textColorButton);
    
    m_promptColorButton = new QPushButton();
    m_promptColorButton->setFixedSize(60, 30);
    m_promptColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_promptColor.name()));
    connect(m_promptColorButton, &QPushButton::clicked, this, &SettingsDialog::selectPromptColor);
    themeLayout->addRow("Prompt Color:", m_promptColorButton);
    
    // Opacity slider
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    m_opacitySlider = new QSlider(Qt::Horizontal);
    m_opacitySlider->setRange(50, 100);
    m_opacitySlider->setValue(100);
    m_opacityLabel = new QLabel("100%");
    opacityLayout->addWidget(m_opacitySlider);
    opacityLayout->addWidget(m_opacityLabel);
    connect(m_opacitySlider, &QSlider::valueChanged, this, &SettingsDialog::onOpacityChanged);
    themeLayout->addRow("Window Opacity:", opacityLayout);
    
    layout->addWidget(themeGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(appearanceTab, "Appearance");
}

void SettingsDialog::setupTerminalTab()
{
    QWidget *terminalTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(terminalTab);
    
    // Editor settings
    QGroupBox *editorGroup = new QGroupBox("Editor Settings");
    QFormLayout *editorLayout = new QFormLayout(editorGroup);
    
    m_tabSizeSpinBox = new QSpinBox();
    m_tabSizeSpinBox->setRange(1, 16);
    m_tabSizeSpinBox->setValue(4);
    editorLayout->addRow("Tab Size:", m_tabSizeSpinBox);
    
    m_wrapLinesCheckBox = new QCheckBox("Wrap long lines");
    m_wrapLinesCheckBox->setChecked(true);
    editorLayout->addRow(m_wrapLinesCheckBox);
    
    m_showLineNumbersCheckBox = new QCheckBox("Show line numbers");
    editorLayout->addRow(m_showLineNumbersCheckBox);
    
    layout->addWidget(editorGroup);
    
    // Cursor settings
    QGroupBox *cursorGroup = new QGroupBox("Cursor Settings");
    QFormLayout *cursorLayout = new QFormLayout(cursorGroup);
    
    m_cursorShapeComboBox = new QComboBox();
    m_cursorShapeComboBox->addItems({"Block", "Underline", "Beam"});
    cursorLayout->addRow("Cursor Shape:", m_cursorShapeComboBox);
    
    m_blinkingCursorCheckBox = new QCheckBox("Blinking cursor");
    m_blinkingCursorCheckBox->setChecked(true);
    cursorLayout->addRow(m_blinkingCursorCheckBox);
    
    layout->addWidget(cursorGroup);
    
    // Terminal behavior
    QGroupBox *behaviorGroup = new QGroupBox("Terminal Behavior");
    QFormLayout *behaviorLayout = new QFormLayout(behaviorGroup);
    
    m_scrollbackLinesSpinBox = new QSpinBox();
    m_scrollbackLinesSpinBox->setRange(100, 100000);
    m_scrollbackLinesSpinBox->setValue(10000);
    m_scrollbackLinesSpinBox->setSuffix(" lines");
    behaviorLayout->addRow("Scrollback Lines:", m_scrollbackLinesSpinBox);
    
    layout->addWidget(behaviorGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(terminalTab, "Terminal");
}

void SettingsDialog::setupShortcutsTab()
{
    QWidget *shortcutsTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(shortcutsTab);
    
    QLabel *info = new QLabel("Keyboard shortcuts configuration will be implemented in a future version.");
    info->setWordWrap(true);
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);
    
    layout->addStretch();
    
    m_tabWidget->addTab(shortcutsTab, "Shortcuts");
}

void SettingsDialog::loadSettings()
{
    // Load general settings
    m_shellComboBox->setCurrentText(m_settings->value("general/shell", "fish").toString());
    m_workingDirEdit->setText(m_settings->value("general/workingDir", QDir::currentPath()).toString());
    m_startupTabCheckBox->setChecked(m_settings->value("general/startupTab", true).toBool());
    m_historyLimitSpinBox->setValue(m_settings->value("general/historyLimit", 1000).toInt());
    
    // Load appearance settings
    QString fontFamily = m_settings->value("appearance/fontFamily", "monospace").toString();
    m_fontComboBox->setCurrentFont(QFont(fontFamily));
    m_fontSizeSpinBox->setValue(m_settings->value("appearance/fontSize", 10).toInt());
    m_boldFontCheckBox->setChecked(m_settings->value("appearance/boldFont", false).toBool());
    m_themeComboBox->setCurrentText(m_settings->value("appearance/theme", "Dark").toString());
    
    // Load colors
    m_backgroundColor = m_settings->value("appearance/backgroundColor", QColor(25, 25, 25)).value<QColor>();
    m_textColor = m_settings->value("appearance/textColor", QColor(255, 255, 255)).value<QColor>();
    m_promptColor = m_settings->value("appearance/promptColor", QColor(42, 130, 218)).value<QColor>();
    
    // Update color buttons
    m_backgroundColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_backgroundColor.name()));
    m_textColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_textColor.name()));
    m_promptColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_promptColor.name()));
    
    m_opacitySlider->setValue(m_settings->value("appearance/opacity", 100).toInt());
    
    // Load terminal settings
    m_tabSizeSpinBox->setValue(m_settings->value("terminal/tabSize", 4).toInt());
    m_wrapLinesCheckBox->setChecked(m_settings->value("terminal/wrapLines", true).toBool());
    m_showLineNumbersCheckBox->setChecked(m_settings->value("terminal/showLineNumbers", false).toBool());
    m_cursorShapeComboBox->setCurrentText(m_settings->value("terminal/cursorShape", "Block").toString());
    m_blinkingCursorCheckBox->setChecked(m_settings->value("terminal/blinkingCursor", true).toBool());
    m_scrollbackLinesSpinBox->setValue(m_settings->value("terminal/scrollbackLines", 10000).toInt());
}

void SettingsDialog::saveSettings()
{
    // Save general settings
    m_settings->setValue("general/shell", m_shellComboBox->currentText());
    m_settings->setValue("general/workingDir", m_workingDirEdit->text());
    m_settings->setValue("general/startupTab", m_startupTabCheckBox->isChecked());
    m_settings->setValue("general/historyLimit", m_historyLimitSpinBox->value());
    
    // Save appearance settings
    m_settings->setValue("appearance/fontFamily", m_fontComboBox->currentFont().family());
    m_settings->setValue("appearance/fontSize", m_fontSizeSpinBox->value());
    m_settings->setValue("appearance/boldFont", m_boldFontCheckBox->isChecked());
    m_settings->setValue("appearance/theme", m_themeComboBox->currentText());
    m_settings->setValue("appearance/backgroundColor", m_backgroundColor);
    m_settings->setValue("appearance/textColor", m_textColor);
    m_settings->setValue("appearance/promptColor", m_promptColor);
    m_settings->setValue("appearance/opacity", m_opacitySlider->value());
    
    // Save terminal settings
    m_settings->setValue("terminal/tabSize", m_tabSizeSpinBox->value());
    m_settings->setValue("terminal/wrapLines", m_wrapLinesCheckBox->isChecked());
    m_settings->setValue("terminal/showLineNumbers", m_showLineNumbersCheckBox->isChecked());
    m_settings->setValue("terminal/cursorShape", m_cursorShapeComboBox->currentText());
    m_settings->setValue("terminal/blinkingCursor", m_blinkingCursorCheckBox->isChecked());
    m_settings->setValue("terminal/scrollbackLines", m_scrollbackLinesSpinBox->value());
    
    m_settings->sync();
}

void SettingsDialog::applySettings()
{
    saveSettings();
    QMessageBox::information(this, "Settings", "Settings have been applied successfully!");
}

void SettingsDialog::resetToDefaults()
{
    int ret = QMessageBox::question(this, "Reset Settings", 
                                   "Are you sure you want to reset all settings to defaults?",
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_settings->clear();
        
        // Reset to defaults
        m_shellComboBox->setCurrentText("fish");
        m_workingDirEdit->setText(QDir::currentPath());
        m_startupTabCheckBox->setChecked(true);
        m_historyLimitSpinBox->setValue(1000);
        
        m_fontComboBox->setCurrentFont(QFont("monospace"));
        m_fontSizeSpinBox->setValue(10);
        m_boldFontCheckBox->setChecked(false);
        m_themeComboBox->setCurrentText("Dark");
        
        m_backgroundColor = QColor(25, 25, 25);
        m_textColor = QColor(255, 255, 255);
        m_promptColor = QColor(42, 130, 218);
        
        m_backgroundColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_backgroundColor.name()));
        m_textColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_textColor.name()));
        m_promptColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(m_promptColor.name()));
        
        m_opacitySlider->setValue(100);
        
        m_tabSizeSpinBox->setValue(4);
        m_wrapLinesCheckBox->setChecked(true);
        m_showLineNumbersCheckBox->setChecked(false);
        m_cursorShapeComboBox->setCurrentText("Block");
        m_blinkingCursorCheckBox->setChecked(true);
        m_scrollbackLinesSpinBox->setValue(10000);
    }
}

void SettingsDialog::selectBackgroundColor()
{
    QColor color = QColorDialog::getColor(m_backgroundColor, this, "Select Background Color");
    if (color.isValid()) {
        m_backgroundColor = color;
        m_backgroundColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(color.name()));
    }
}

void SettingsDialog::selectTextColor()
{
    QColor color = QColorDialog::getColor(m_textColor, this, "Select Text Color");
    if (color.isValid()) {
        m_textColor = color;
        m_textColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(color.name()));
    }
}

void SettingsDialog::selectPromptColor()
{
    QColor color = QColorDialog::getColor(m_promptColor, this, "Select Prompt Color");
    if (color.isValid()) {
        m_promptColor = color;
        m_promptColorButton->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(color.name()));
    }
}

void SettingsDialog::onFontChanged()
{
    // Preview font changes if needed
    // This could update a preview widget in the future
}

void SettingsDialog::onOpacityChanged(int value)
{
    m_opacityLabel->setText(QString("%1%").arg(value));
}
