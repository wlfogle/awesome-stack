#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QStringList>

struct FanInfo {
    QString name;
    int rpm;
    QString devicePath;
};

class FanController : public QObject
{
    Q_OBJECT

public:
    enum class FanMode {
        Silent = 0,
        Auto = 1,
        Performance = 2
    };

    explicit FanController(QObject *parent = nullptr);

    bool setFanMode(FanMode mode);
    FanMode currentMode() const;
    bool isAvailable() const;
    QList<FanInfo> getFanInfo();

signals:
    void fanModeChanged(FanMode mode);
    void error(const QString &message);

private:
    void detectFanControlMethods();
    void checkDirectPWMControl();
    bool setFanModeNBFC(FanMode mode);
    bool setFanModeFancontrol(FanMode mode);
    bool setFanModeDirect(FanMode mode);

private:
    FanMode m_currentMode;
    bool m_nbfcAvailable;
    bool m_fancontrolAvailable;
    QStringList m_pwmDevices;
};

#endif // FANCONTROLLER_H
