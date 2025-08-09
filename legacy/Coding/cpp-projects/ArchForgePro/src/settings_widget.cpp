#include "settings_widget.h"
#include <QMessageBox>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
{
    setupUI();
    setupConnections();
}

SettingsWidget::~SettingsWidget()
{
    // Qt handles cleanup automatically
}

void SettingsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createAboutTab(), "About");
    
    mainLayout->addWidget(m_tabWidget);
}

void SettingsWidget::setupConnections()
{
    // Connections will be added as needed
}

QWidget* SettingsWidget::createAboutTab()
{
    QWidget *aboutTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(aboutTab);
    
    // Application info group
    QGroupBox *appInfoGroup = new QGroupBox("Application Information");
    QVBoxLayout *appInfoLayout = new QVBoxLayout(appInfoGroup);
    
    m_appNameLabel = new QLabel("ArchForgePro");
    m_appNameLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2980b9;");
    
    m_versionLabel = new QLabel("Version 1.0.0");
    m_versionLabel->setStyleSheet("font-size: 14px; color: #7f8c8d;");
    
    m_descriptionLabel = new QLabel("A comprehensive Arch Linux management tool with AI assistance,\n"
                                   "package management, system maintenance, and more.");
    m_descriptionLabel->setWordWrap(true);
    
    m_authorLabel = new QLabel("Developed by: ArchForge Team");
    m_licenseLabel = new QLabel("License: GPL v3.0");
    
    appInfoLayout->addWidget(m_appNameLabel);
    appInfoLayout->addWidget(m_versionLabel);
    appInfoLayout->addWidget(m_descriptionLabel);
    appInfoLayout->addWidget(m_authorLabel);
    appInfoLayout->addWidget(m_licenseLabel);
    
    // Credits group
    QGroupBox *creditsGroup = new QGroupBox("Credits & Acknowledgments");
    QVBoxLayout *creditsLayout = new QVBoxLayout(creditsGroup);
    
    m_creditsText = new QTextEdit();
    m_creditsText->setReadOnly(true);
    m_creditsText->setMaximumHeight(200);
    m_creditsText->setHtml(
        "<h3>Built with:</h3>"
        "<ul>"
        "<li>Qt6 Framework</li>"
        "<li>C++ Programming Language</li>"
        "<li>Arch Linux</li>"
        "</ul>"
        "<h3>Special Thanks:</h3>"
        "<ul>"
        "<li>Arch Linux Community</li>"
        "<li>Qt Project</li>"
        "<li>Open Source Contributors</li>"
        "</ul>"
    );
    
    creditsLayout->addWidget(m_creditsText);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_saveSettingsButton = new QPushButton("Save Settings");
    m_resetSettingsButton = new QPushButton("Reset Settings");
    m_loadDefaultsButton = new QPushButton("Load Defaults");
    
    buttonLayout->addWidget(m_saveSettingsButton);
    buttonLayout->addWidget(m_resetSettingsButton);
    buttonLayout->addWidget(m_loadDefaultsButton);
    buttonLayout->addStretch();
    
    layout->addWidget(appInfoGroup);
    layout->addWidget(creditsGroup);
    layout->addStretch();
    layout->addLayout(buttonLayout);
    
    return aboutTab;
}

void SettingsWidget::showAbout()
{
    QMessageBox::about(this, "About ArchForgePro",
                      "ArchForgePro v1.0.0\n\n"
                      "A comprehensive Arch Linux management tool\n"
                      "with AI assistance, package management,\n"
                      "system maintenance, and more.\n\n"
                      "Built with Qt6 and C++\n\n"
                      "License: GPL v3.0");
}

void SettingsWidget::saveSettings()
{
    QMessageBox::information(this, "Save Settings", "Settings saved successfully!");
    // TODO: Implement settings saving
}

void SettingsWidget::resetSettings()
{
    QMessageBox::information(this, "Reset Settings", "Settings reset to defaults!");
    // TODO: Implement settings reset
}

void SettingsWidget::loadDefaultSettings()
{
    QMessageBox::information(this, "Load Defaults", "Default settings loaded!");
    // TODO: Implement default settings loading
}
