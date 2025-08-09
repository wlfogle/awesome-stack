#ifndef SETTINGS_WIDGET_H
#define SETTINGS_WIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget();

private slots:
    void showAbout();
    void saveSettings();
    void resetSettings();
    void loadDefaultSettings();

private:
    void setupUI();
    void setupConnections();
    
    // Tab creation methods
    QWidget* createAboutTab();
    
    // Main components
    QTabWidget *m_tabWidget;
    
    // About tab components
    QLabel *m_appNameLabel;
    QLabel *m_versionLabel;
    QLabel *m_descriptionLabel;
    QLabel *m_authorLabel;
    QLabel *m_licenseLabel;
    QTextEdit *m_creditsText;
    
    // Settings components
    QPushButton *m_saveSettingsButton;
    QPushButton *m_resetSettingsButton;
    QPushButton *m_loadDefaultsButton;
};

#endif // SETTINGS_WIDGET_H
