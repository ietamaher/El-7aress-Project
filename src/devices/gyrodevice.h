#ifndef GYRODEVICE_H
#define GYRODEVICE_H

#include "modbusdevicebase.h"
#include <QVector>
#include <cstdint>

/**
 * @brief Structure to hold all data from the SST810 Dynamic Inclinometer.
 *
 * This structure contains the processed angle and temperature data,
 * raw IMU data (accelerometer and gyroscope), and the connection status.
 */
struct GyroData {
    // Connection Status
    bool isConnected = false;

    // Processed Data (from Kalman Filter)
    double imuRollDeg = 0.0;        ///< Processed Roll angle in degrees (from X-Axis).
    double imuPitchDeg = 0.0;       ///< Processed Pitch angle in degrees (from Y-Axis).
    double imuYawDeg = 0.0;         ///< Processed relative Yaw angle in degrees (from Z-Axis Gyro).
    double temperature = 0.0; ///< Sensor temperature in degrees Celsius.

    // Raw Sensor Data
    // NOTE: The protocol document is unclear on the exact type (int16/int32).
    // Based on the register layout (2 registers per value), int32 is assumed.
    // The scaling factors to convert these to g or °/s are NOT in the document
    // and must be obtained from the manufacturer.
    int32_t rawAccelX = 0;    ///< Raw X-axis accelerometer value.
    int32_t rawAccelY = 0;    ///< Raw Y-axis accelerometer value.
    int32_t rawAccelZ = 0;    ///< Raw Z-axis accelerometer value.
    int32_t rawGyroX = 0;     ///< Raw X-axis gyroscope value.
    int32_t rawGyroY = 0;     ///< Raw Y-axis gyroscope value.
    int32_t rawGyroZ = 0;     ///< Raw Z-axis gyroscope value.

    // Comparison operators to easily detect changes.
    bool operator==(const GyroData &other) const {
        return (isConnected == other.isConnected &&
                imuRollDeg == other.imuRollDeg &&
                imuPitchDeg == other.imuPitchDeg &&
                imuYawDeg == other.imuYawDeg &&
                temperature == other.temperature &&
                rawAccelX == other.rawAccelX &&
                rawAccelY == other.rawAccelY &&
                rawAccelZ == other.rawAccelZ &&
                rawGyroX == other.rawGyroX &&
                rawGyroY == other.rawGyroY &&
                rawGyroZ == other.rawGyroZ);
    }

    bool operator!=(const GyroData &other) const {
        return !(*this == other);
    }
};

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
    // SST810 Modbus register addresses and counts
    // We read all 18 registers in one go, from X-Angle to Z-Gyro.
    static constexpr int ALL_DATA_START_ADDRESS = 0x03E8;
    static constexpr int ALL_DATA_REGISTER_COUNT = 18; // 9 values * 2 registers/value

    /**
     * @brief Constructor for the ImuDevice class.
     * @param device Serial port name (e.g., "COM3" or "/dev/ttyUSB0").
     * @param baudRate Baud rate (for SST810, typically 115200).
     * @param slaveId Modbus slave ID of the sensor (default is 1).
     * @param parent QObject parent for memory management.
     */
    explicit ImuDevice(const QString &device,
                         int baudRate,
                         int slaveId,
                         QObject *parent = nullptr);
    ~ImuDevice() override;

    // Public data access
    GyroData getCurrentData() const;

signals:
    /**
     * @brief Emitted whenever the gyro data (including connection status) changes.
     * @param data The new, complete GyroData struct.
     */
    void gyroDataChanged(const GyroData &data);

protected:
    // Override pure virtual methods from ModbusDeviceBase
    void readData() override;
    void onDataReadComplete() override;
    void onWriteComplete() override;

private slots:
    // Slot to handle the response from our read request
    void onReadReady(QModbusReply *reply);

private:
    // Helper methods
    void parseModbusResponse(const QModbusDataUnit &dataUnit);
    void updateGyroData(const GyroData &newData);
    void handleConnectionChange(bool connected);

    GyroData m_currentData;
    mutable QMutex m_mutex;
};

#endif // GYRODEVICE_H