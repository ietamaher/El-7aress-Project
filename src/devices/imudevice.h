#ifndef IMUDEVICE_H
#define IMUDEVICE_H

#include "modbusdevicebase.h"
#include <QVector>
#include <cstdint>
#include <QMutex>

/**
 * @brief Structure to hold all data from the SST810 Dynamic Inclinometer (IMU).
 *
 * This structure contains the processed angle/temperature data, raw IMU data,
 * and the connection status, all received as floating-point values.
 */
struct ImuData {
    // Connection Status
    bool isConnected = false;

    // Processed Angles (from Kalman Filter)
    double imuRollDeg = 0.0;        ///< Processed Roll angle in degrees.
    double imuPitchDeg = 0.0;       ///< Processed Pitch angle in degrees.
    // Note: Yaw is on the device label but its register is not in the documentation.
    double imuYawDeg = 0.0;         ///< (Placeholder) Processed relative Yaw angle in degrees.

    // Sensor Physical State
    double temperature = 0.0; ///< Sensor temperature in degrees Celsius.

    // "Raw" IMU Data (received as floats in physical units)
    double accelX_g = 0.0;      ///< X-axis acceleration in g's.
    double accelY_g = 0.0;      ///< Y-axis acceleration in g's.
    double accelZ_g = 0.0;      ///< Z-axis acceleration in g's.
    double angRateX_dps = 0.0;  ///< X-axis angular rate (imuPitchDeg rate) in degrees/sec.
    double angRateY_dps = 0.0;  ///< Y-axis angular rate (imuRollDeg rate) in degrees/sec.
    double angRateZ_dps = 0.0;  ///< Z-axis angular rate (imuYawDeg rate) in degrees/sec.

    // Comparison operator to easily detect changes.
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

/**
 * @brief The ImuDevice class manages Modbus RTU communication with an SST810 inclinometer.
 *
 * This class handles the connection, periodic reading of angle, temperature,
 * and raw IMU data from an SST810 sensor using the Modbus RTU protocol.
 * It is built upon the ModbusDeviceBase to provide robust communication with
 * automatic reconnection and error handling.
 */
class ImuDevice : public ModbusDeviceBase {
    Q_OBJECT

public:
    // SST810 Modbus registers: Start at X-Angle and read all 18 registers.
    static constexpr int ALL_DATA_START_ADDRESS = 0x03E8;
    static constexpr int ALL_DATA_REGISTER_COUNT = 18; // 9 values * 2 registers/value

    explicit ImuDevice(const QString &device, int baudRate, int slaveId, QObject *parent = nullptr);
    ~ImuDevice() override;

    ImuData getCurrentData() const;

signals:
    void imuDataChanged(const ImuData &data);

protected:
    void readData() override;
    void onDataReadComplete() override;
    void onWriteComplete() override;

private slots:
    void onReadReady(QModbusReply *reply);

private:
    void parseModbusResponse(const QModbusDataUnit &dataUnit);
    void updateImuData(const ImuData &newData);
    void handleConnectionChange(bool connected);

    ImuData m_currentData;
    mutable QMutex m_mutex;
};

#endif // IMUDEVICE_H
