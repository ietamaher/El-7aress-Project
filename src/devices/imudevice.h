#ifndef IMUDEVICE_H
#define IMUDEVICE_H

#include "baseserialdevice.h"
#include <QTimer>
#include <QMutex>

/**
 * @brief Structure to hold all data from the IMU sensor.
 */
struct ImuData {
    // Connection Status
    bool isConnected = false;

    // Processed Angles
    double imuRollDeg = 0.0;
    double imuPitchDeg = 0.0;
    double imuYawDeg = 0.0;

    // Sensor Physical State
    double temperature = 0.0;

    // "Raw" IMU Data
    double accelX_g = 0.0;
    double accelY_g = 0.0;
    double accelZ_g = 0.0;
    double angRateX_dps = 0.0;
    double angRateY_dps = 0.0;
    double angRateZ_dps = 0.0;

    bool operator==(const ImuData &other) const;
    bool operator!=(const ImuData &other) const { return !(*this == other); }
};

inline bool ImuData::operator==(const ImuData &other) const {
    return (isConnected == other.isConnected &&
            imuRollDeg == other.imuRollDeg &&
            imuPitchDeg == other.imuPitchDeg &&
            imuYawDeg == other.imuYawDeg &&
            temperature == other.temperature &&
            accelX_g == other.accelX_g &&
            accelY_g == other.accelY_g &&
            accelZ_g == other.accelZ_g &&
            angRateX_dps == other.angRateX_dps &&
            angRateY_dps == other.angRateY_dps &&
            angRateZ_dps == other.angRateZ_dps);
}

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

private:
    void captureGyroBias();
    void sendReadRequest();
    void parseResponse(const QByteArray &response);
    void updateImuData(const ImuData &newData);

    ImuData m_currentData;
    mutable QMutex m_mutex;

    QTimer *m_pollTimer;
    QTimer *m_gyroBiasTimer;

    bool m_waitingForGyroBias = false;
    int m_pollIntervalMs = 10;  // Default 100Hz

    // Protocol constants
    static constexpr int GYRO_BIAS_TIMEOUT_MS = 12000;  // 12 seconds
    static constexpr int RESPONSE_SIZE = 31;  // 0xCF response size
    static constexpr quint8 CMD_READ_DATA = 0xCF;
    static constexpr quint8 CMD_GYRO_BIAS = 0xCD;
};

#endif // IMUDEVICE_H
