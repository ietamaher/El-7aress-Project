/**
 * @file servodriverdevice.h
 * @brief Declaration of the ServoDriverDevice class for Modbus RTU communication with a servo driver.
 *
 * The ServoDriverDevice class is responsible for handling communication between a servo motor driver
 * and the application via the Modbus RTU protocol over a serial connection. This includes establishing
 * the connection, performing regular data polling (position and temperature), issuing commands,
 * and managing alarms (status monitoring, clearing, and history retrieval).
 *
 * @section FunctionalOverview Functional Overview
 * - **Modbus Communication**: Connects and exchanges Modbus RTU messages.
 * - **Data Acquisition**: Periodically reads position and temperature values.
 * - **Command Execution**: Sends commands to control the servo motor.
 * - **Alarm Handling**: Supports detection, acknowledgment, clearing, and history review.
 * - **State Tracking**: Maintains up-to-date servo state and connection status.
 * - **Thread Safety**: Uses mutex protection for concurrent access.
 * - **Error and Timeout Handling**: Handles communication issues gracefully.
 *
 * @section SignalSlotMap Signals and Slots Map
 * - **Connection Handling**: `stateChanged` -> `onStateChanged`
 * - **Error Monitoring**: `errorOccurred` -> `onErrorOccurred`
 * - **Data Polling**: Timers trigger data reads (`readPositionData`, `readTemperatureData`)
 * - **Read Completion**: Modbus replies handled by slots (`onPositionReadReady`, etc.)
 * - **Command Completion**: Handled by `onWriteReady`
 * - **Timeout Handling**: `timeout` -> `handleTimeout`
 * - **State Updates**: `servoDataChanged`, `alarmDetected`, etc.
 *
 * @author ieta_maher
 * @date 2025-06-20
 * @version 1.1
 */

#ifndef SERVODRIVERDEVICE_H
#define SERVODRIVERDEVICE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QModbusRtuSerialClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QtGlobal>
#include "../TimestampLogger.h"
#include "modbusdevicebase.h"

/**
 * @struct ServoData
 * @brief Unified data structure describing all relevant servo states.
 */
struct ServoData {
    bool isConnected = false;
    float position = 0.0f;
    float rpm = 0.0f;
    float torque = 0.0f;
    float motorTemp = 0.0f;
    float driverTemp = 0.0f;
    bool fault = false;


    bool operator!=(const ServoData &other) const {
        // Use epsilon-based comparison for floating-point values to avoid signal flooding
        // due to insignificant precision differences
        const float POSITION_EPSILON = 1.0f;    // 1 encoder count (≈0.0016°, very precise)
        const float RPM_EPSILON = 1.0f;         // 1 RPM
        const float TORQUE_EPSILON = 0.1f;      // 0.1% torque
        const float TEMP_EPSILON = 0.1f;        // 0.1°C

        return (isConnected != other.isConnected ||
                std::abs(position - other.position) > POSITION_EPSILON ||
                std::abs(rpm - other.rpm) > RPM_EPSILON ||
                std::abs(torque - other.torque) > TORQUE_EPSILON ||
                std::abs(motorTemp - other.motorTemp) > TEMP_EPSILON ||
                std::abs(driverTemp - other.driverTemp) > TEMP_EPSILON ||
                fault != other.fault);
    }
};

/**
 * @brief Structure containing detailed information about an alarm.
 */
struct AlarmInfo {
    uint16_t code;
    QString alarmName;
    QString description;
    QString solution;
    bool isCritical;
};

/**
 * @brief Class for managing Modbus RTU-based communication with a servo driver device.
 */
class ServoDriverDevice : public ModbusDeviceBase
{
    Q_OBJECT

public:
    /**
     * @brief Constructor.
     * @param identifier Device name or label.
     * @param device Serial port path (e.g. "/dev/ttyUSB0").
     * @param baudRate Communication baud rate.
     * @param slaveId Modbus slave address.
     * @param parent Optional parent QObject.
     */
    explicit ServoDriverDevice(const QString &identifier,
                               const QString &device,
                               int baudRate,
                                int slaveId,
                                QSerialPort::Parity parity,
                               QObject *parent = nullptr);

    /**
     * @brief Destructor.
     */
    ~ServoDriverDevice() override;

    // Getters
    QString identifier() const;
    ServoData currentData() const;
    uint16_t currentAlarmCode() const;

    /**
     * @brief Write multiple Modbus registers.
     * @param startAddress Starting register address.
     * @param values Values to write.
     */
    void writeData(int startAddress, const QVector<quint16> &values);

    /**
     * @brief Read the current alarm status.
     */
    void readAlarmStatus();

    /**
     * @brief Clear current alarm.
     * @return True if request was sent.
     */
    bool clearAlarm();

    /**
     * @brief Read the alarm history.
     */
    void readAlarmHistory();

    /**
     * @brief Clear the alarm history.
     * @return True if request was sent.
     */
    bool clearAlarmHistory();

    /**
     * @brief Get human-readable alarm description.
     * @param alarmCode Alarm code.
     * @return Description string.
     */
    QString getAlarmDescription(uint16_t alarmCode);

    /**
     * @brief Enable or disable periodic temperature reading.
     * @param enable True to enable, false to disable.
     */
    void enableTemperatureReading(bool enable);

    /**
     * @brief Set interval for temperature polling.
     * @param intervalMs Interval in milliseconds.
     */
    void setTemperatureInterval(int intervalMs);

signals:
    void servoDataChanged(const ServoData &data);
    void alarmDetected(uint16_t alarmCode, const QString &description);
    void alarmCleared();
    void alarmHistoryRead(const QList<uint16_t> &alarmHistory);
    void alarmHistoryCleared();

protected:
    /**
     * @brief Read servo data (position, temperature, etc.).
     */
    void readData() override;

    /**
     * @brief Called after successful data read.
     */
    void onDataReadComplete() override;

    /**
     * @brief Called after successful write operation.
     */
    void onWriteComplete() override;

private slots:
    void readTemperatureData();               ///< Periodically triggered to read temperature
    void onPositionReadReady();              ///< Slot called when position data reply is ready
    void onTemperatureReadReady();           ///< Slot called when temperature data reply is ready
    void onWriteReady();                     ///< Slot called when write response is received
    void onAlarmReadReady();                 ///< Slot called when alarm status is received
    void onAlarmHistoryReady();              ///< Slot called when alarm history data is received
    void onCommunicationWatchdogTimeout();

private:
    void initializeAlarmMap();               ///< Populates alarm information table
    void updateServoData(const ServoData &newData); ///< Compares and emits changed data
    void setupTemperatureTimer();            ///< Initializes and manages temperature polling
    void resetCommunicationWatchdog();
    void setConnectionState(bool connected);
    QString m_identifier;
    ServoData m_currentData;
    uint16_t m_currentAlarmCode = 0;

    QTimer *m_temperatureTimer;              ///< Timer for temperature polling
    bool m_temperatureEnabled = true;
    int m_temperatureCounter = 0;

    QMap<uint16_t, AlarmInfo> m_alarmMap;    ///< Alarm code to info mapping

    // Modbus register address definitions
    static constexpr int POSITION_START_ADDR = 204;
    static constexpr int POSITION_REG_COUNT = 2;
    static constexpr int TEMPERATURE_START_ADDR = 248;
    static constexpr int TEMPERATURE_REG_COUNT = 4;
    static constexpr int ALARM_STATUS_ADDR = 172;
    static constexpr int ALARM_STATUS_REG_COUNT = 20;
    static constexpr int ALARM_HISTORY_ADDR = 130;
    static constexpr int ALARM_HISTORY_REG_COUNT = 20;
    static constexpr int ALARM_RESET_ADDR = 388;
    static constexpr int ALARM_HISTORY_CLEAR_ADDR = 386;
    QTimer* m_communicationWatchdog;
    static constexpr int COMMUNICATION_TIMEOUT_MS = 3000;
};

#endif // SERVODRIVERDEVICE_H
