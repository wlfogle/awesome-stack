#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>

class PowerManager : public QObject
{
    Q_OBJECT

public:
    enum class PowerProfile {
        Performance = 0,
        Balanced = 1,
        PowerSave = 2
    };

    explicit PowerManager(QObject *parent = nullptr);

    bool setPowerProfile(PowerProfile profile);
    PowerProfile currentProfile() const;
    bool isAvailable() const;
    
    // Battery information
    int batteryLevel() const;
    bool isOnACPower() const;
    QString batteryStatus() const;
    
    // CPU frequency information
    QStringList getCPUFrequencyInfo();
    QString getCurrentGovernor();
    QStringList getAvailableGovernors();

signals:
    void powerProfileChanged(PowerProfile profile);
    void batteryLevelChanged(int level);
    void powerSourceChanged(bool onAC);
    void error(const QString &message);

private slots:
    void updateBatteryStatus();

private:
    void detectAvailableMethods();
    bool setPowerProfileUserspace(PowerProfile profile);
    bool setPowerProfileCpufreq(PowerProfile profile);
    void startBatteryMonitoring();
    
    QString powerProfileToString(PowerProfile profile) const;
    QString getGovernorForProfile(PowerProfile profile) const;

private:
    PowerProfile m_currentProfile;
    bool m_available;
    bool m_hasCpufreq;
    bool m_hasPowerProfiles;
    
    // Battery monitoring
    QTimer *m_batteryTimer;
    int m_lastBatteryLevel;
    bool m_lastACStatus;
    
    // Available governors
    QStringList m_availableGovernors;
};

#endif // POWERMANAGER_H
