#ifndef IMUDEVICE_H
#define IMUDEVICE_H

#include "baseserialdevice.h"
#include <QTimer>
#include <QMutex>

/**
 * @brief Structure to hold all data from the IMU sensor.
 */
struct ImuData {
    bool isConnected = false;

    // Processed angles (from Kalman filter)
    double rollDeg = 0.0;
    double pitchDeg = 0.0;
    double yawDeg = 0.0;

    // Physical state
    double temperature = 0.0;

    // Raw IMU data
    double accelX_g = 0.0;
    double accelY_g = 0.0;
    double accelZ_g = 0.0;
    double angRateX_dps = 0.0;
    double angRateY_dps = 0.0;
    double angRateZ_dps = 0.0;

    bool operator!=(const ImuData &other) const {
        // Use epsilon-based comparison for floating-point values to avoid signal flooding
        // due to insignificant precision differences
        const double ANGLE_EPSILON = 0.01;      // 0.01° = 36 arcseconds (very precise)
        const double TEMP_EPSILON = 0.1;        // 0.1°C
        const double ACCEL_EPSILON = 0.001;     // 0.001g (very sensitive)
        const double GYRO_EPSILON = 0.01;       // 0.01 deg/s

        return (isConnected != other.isConnected ||
                std::abs(rollDeg - other.rollDeg) > ANGLE_EPSILON ||
                std::abs(pitchDeg - other.pitchDeg) > ANGLE_EPSILON ||
                std::abs(yawDeg - other.yawDeg) > ANGLE_EPSILON ||
                std::abs(temperature - other.temperature) > TEMP_EPSILON ||
                std::abs(accelX_g - other.accelX_g) > ACCEL_EPSILON ||
                std::abs(accelY_g - other.accelY_g) > ACCEL_EPSILON ||
                std::abs(accelZ_g - other.accelZ_g) > ACCEL_EPSILON ||
                std::abs(angRateX_dps - other.angRateX_dps) > GYRO_EPSILON ||
                std::abs(angRateY_dps - other.angRateY_dps) > GYRO_EPSILON ||
                std::abs(angRateZ_dps - other.angRateZ_dps) > GYRO_EPSILON);
    }
};

class ImuDevice : public BaseSerialDevice {
    Q_OBJECT

public:
    explicit ImuDevice(QObject *parent = nullptr);
    ~ImuDevice() override;

    ImuData currentData() const;

    void setPollInterval(int intervalMs);

signals:
    void imuDataChanged(const ImuData &data);

protected:
    void configureSerialPort() override;
    void processIncomingData() override;
    void onConnectionEstablished() override;
    void onConnectionLost() override;

private slots:
    void onPollTimer();
    void onGyroBiasTimeout();
    void onCommunicationWatchdogTimeout();

private:
    void captureGyroBias();
    void sendReadRequest();
    void parseResponse(const QByteArray &response);
    void updateImuData(const ImuData &newData);
    void resetCommunicationWatchdog();
    void setConnectionState(bool connected);

    ImuData m_currentData;
    mutable QMutex m_mutex;

    QTimer *m_pollTimer;
    QTimer *m_communicationWatchdog;
    QTimer *m_gyroBiasTimer;

    bool m_waitingForGyroBias = false;
    int m_pollIntervalMs = 10;  // Default 100Hz

    // Protocol constants
    static constexpr int COMMUNICATION_TIMEOUT_MS = 3000;  // 3 seconds
    static constexpr int GYRO_BIAS_TIMEOUT_MS = 12000;     // 12 seconds
    static constexpr int RESPONSE_SIZE = 31;               // 0xCF response size
    static constexpr quint8 CMD_READ_DATA = 0xCF;
    static constexpr quint8 CMD_GYRO_BIAS = 0xCD;
};

#endif // IMUDEVICE_H