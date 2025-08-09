#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QFontComboBox>
#include <QColorDialog>
#include <QLabel>
#include <QSlider>
#include <QSettings>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void applySettings();
    void resetToDefaults();
    void selectBackgroundColor();
    void selectTextColor();
    void selectPromptColor();
    void onFontChanged();
    void onOpacityChanged(int value);

private:
    void setupUI();
    void setupGeneralTab();
    void setupAppearanceTab();
    void setupTerminalTab();
    void setupShortcutsTab();
    void loadSettings();
    void saveSettings();
    
    // UI Components
    QTabWidget *m_tabWidget;
    QPushButton *m_applyButton;
    QPushButton *m_cancelButton;
    QPushButton *m_okButton;
    QPushButton *m_resetButton;
    
    // General settings
    QComboBox *m_shellComboBox;
    QLineEdit *m_workingDirEdit;
    QPushButton *m_browseDirButton;
    QCheckBox *m_startupTabCheckBox;
    QSpinBox *m_historyLimitSpinBox;
    
    // Appearance settings
    QFontComboBox *m_fontComboBox;
    QSpinBox *m_fontSizeSpinBox;
    QCheckBox *m_boldFontCheckBox;
    QComboBox *m_themeComboBox;
    QPushButton *m_backgroundColorButton;
    QPushButton *m_textColorButton;
    QPushButton *m_promptColorButton;
    QSlider *m_opacitySlider;
    QLabel *m_opacityLabel;
    
    // Terminal settings
    QSpinBox *m_tabSizeSpinBox;
    QCheckBox *m_wrapLinesCheckBox;
    QCheckBox *m_showLineNumbersCheckBox;
    QComboBox *m_cursorShapeComboBox;
    QCheckBox *m_blinkingCursorCheckBox;
    QSpinBox *m_scrollbackLinesSpinBox;
    
    // Colors
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_promptColor;
    
    // Settings storage
    QSettings *m_settings;
};

#endif // SETTINGSDIALOG_H
