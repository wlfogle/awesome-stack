#include "modelconfigwidget.h"

ModelConfigWidget::ModelConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ModelConfigWidget::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Model selection group
    QGroupBox *modelGroup = new QGroupBox("Model Selection");
    QVBoxLayout *modelLayout = new QVBoxLayout(modelGroup);
    
    modelCombo = new QComboBox();
    modelCombo->addItem("ollama/codellama:7b");
    modelCombo->addItem("ollama/deepseek-coder:6.7b");
    modelCombo->addItem("ollama/codegemma:7b");
    modelCombo->addItem("ollama/llama3.1:8b");
    modelCombo->addItem("ollama/qwen2.5-coder:7b");
    modelCombo->addItem("ollama/starcoder2:7b");
    modelCombo->addItem("ollama/magicoder:7b");
    modelCombo->setCurrentText("ollama/codellama:7b");
    
    modelLayout->addWidget(new QLabel("Select Model:"));
    modelLayout->addWidget(modelCombo);
    layout->addWidget(modelGroup);
    
    // Settings group
    QGroupBox *settingsGroup = new QGroupBox("Settings");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);
    
    autoRunCheckBox = new QCheckBox("Auto-run code");
    autoRunCheckBox->setChecked(false);
    settingsLayout->addWidget(autoRunCheckBox);
    
    containerModeCheckBox = new QCheckBox("Use Distrobox container");
    containerModeCheckBox->setChecked(true);
    settingsLayout->addWidget(containerModeCheckBox);
    
    layout->addWidget(settingsGroup);
    
    // Ollama status group
    QGroupBox *statusGroup = new QGroupBox("Ollama Status");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    statusLabel = new QLabel("Checking...");
    checkStatusButton = new QPushButton("Check Status");
    startOllamaButton = new QPushButton("Start Ollama");
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(checkStatusButton);
    statusLayout->addWidget(startOllamaButton);
    layout->addWidget(statusGroup);
    
    layout->addStretch();
    
    // Connect signals
    connect(checkStatusButton, &QPushButton::clicked, this, &ModelConfigWidget::onCheckStatusClicked);
    connect(startOllamaButton, &QPushButton::clicked, this, &ModelConfigWidget::onStartOllamaClicked);
}

QString ModelConfigWidget::getCurrentModel() const
{
    return modelCombo->currentText();
}

void ModelConfigWidget::setCurrentModel(const QString &model)
{
    int index = modelCombo->findText(model);
    if (index >= 0) {
        modelCombo->setCurrentIndex(index);
    }
}

bool ModelConfigWidget::isContainerModeEnabled() const
{
    return containerModeCheckBox->isChecked();
}

bool ModelConfigWidget::isAutoRunEnabled() const
{
    return autoRunCheckBox->isChecked();
}

void ModelConfigWidget::onCheckStatusClicked()
{
    emit checkStatusRequested();
}

void ModelConfigWidget::onStartOllamaClicked()
{
    emit startOllamaRequested();
}
