#ifndef MODELCONFIGWIDGET_H
#define MODELCONFIGWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ModelConfigWidget : public QWidget {
    Q_OBJECT

public:
    explicit ModelConfigWidget(QWidget *parent = nullptr);
    
    QString getCurrentModel() const;
    void setCurrentModel(const QString &model);
    bool isContainerModeEnabled() const;
    bool isAutoRunEnabled() const;
    
    QLabel* getStatusLabel() const { return statusLabel; }
    QPushButton* getCheckStatusButton() const { return checkStatusButton; }
    QPushButton* getStartOllamaButton() const { return startOllamaButton; }

private slots:
    void onCheckStatusClicked();
    void onStartOllamaClicked();

signals:
    void checkStatusRequested();
    void startOllamaRequested();

private:
    QComboBox *modelCombo;
    QCheckBox *autoRunCheckBox;
    QCheckBox *containerModeCheckBox;
    QLabel *statusLabel;
    QPushButton *checkStatusButton;
    QPushButton *startOllamaButton;
    
    void setupUi();
};

#endif // MODELCONFIGWIDGET_H
