#ifndef SERVOACTUATORDEVICE_H
#define SERVOACTUATORDEVICE_H

#include "baseserialdevice.h"
#include <QObject>
#include <QStringList>
#include <QMap>

// The ActuatorStatus struct is now self-contained and responsible for its own data.
struct ActuatorStatus {
    bool isMotorOff = false;
    bool isLatchingFaultActive = false;
    QStringList activeStatusMessages;

    // --- The map is now a static constant member of the struct ---
    static const QMap<int, QString> STATUS_BIT_MAP;

    // --- The struct now knows how to parse itself from a hex string ---
    void parse(const QString &hexStatus);

    bool operator!=(const ActuatorStatus &other) const {
        return (isMotorOff != other.isMotorOff ||
                isLatchingFaultActive != other.isLatchingFaultActive ||
                activeStatusMessages != other.activeStatusMessages);
    }
};

// The main data struct is now cleaner, without the raw hex string.
struct ServoActuatorData {
    bool isConnected = false;
    double position_mm = 0.0;
    double velocity_mm_s = 0.0;
    double temperature_c = 0.0;
    double busVoltage_v = 0.0;
    double torque_percent = 0.0;
    ActuatorStatus status; // Contains all parsed status details

    bool operator!=(const ServoActuatorData &other) const;
};

class ServoActuatorDevice : public BaseSerialDevice {
    Q_OBJECT

public:
    explicit ServoActuatorDevice(QObject *parent = nullptr);

    // Public interface remains the same
    ServoActuatorData currentData() const;
    void moveToPosition(double position_mm);
    void setMaxSpeed(double speed_mm_s);
    void setAcceleration(double accel_mm_s2);
    void setMaxTorque(double percent);
    void stopMove();
    void holdCurrentPosition();
    void checkAllStatus();
    void checkStatusRegister();
    void checkPosition();
    void checkVelocity();
    void checkTorque();
    void checkTemperature();
    void checkBusVoltage();
    void saveSettings();
    void clearFaults();
    void reboot();

signals:
    void actuatorDataChanged(const ServoActuatorData &data);
    void commandError(const QString &errorDetails);
    void criticalFaultOccurred(const QStringList &faults);

protected:
    // Base class implementations (unchanged)
    void configureSerialPort() override;
    void processIncomingData() override;
    void onConnectionEstablished() override;
    void onConnectionLost() override;

private:
    // Private helper functions (now simpler)
    void sendCommand(const QString &command);
    void updateActuatorData(const ServoActuatorData &newData);
    void handleTimeout();
    QString calculateChecksum(const QString &command) const;

    // Unit conversion functions (unchanged)
    double sensorCountsToMillimeters(int counts) const;
    int millimetersToSensorCounts(double millimeters) const;
    int speedToSensorCounts(double speed_mm_s) const;
    double sensorCountsToSpeed(int counts) const;
    int accelToSensorCounts(double accel_mm_s2) const;
    double sensorCountsToTorquePercent(int counts) const;
    int torquePercentToSensorCounts(double percent) const;

    // Member variables
    ServoActuatorData m_currentData;
    QTimer *m_timeoutTimer;
    QString m_pendingCommand;
    QList<QString> m_commandQueue;

    // --- Physical Constants ---
    static constexpr double SCREW_LEAD_MM = 3.175;
    static constexpr int COUNTS_PER_REVOLUTION = 1024;
    static constexpr int RETRACTED_ENDSTOP_OFFSET = 1024;

};

#endif // SERVOACTUATORDEVICE_H


