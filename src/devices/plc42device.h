/**
 * @file plc42device.h
 * @brief Declaration of the Plc42Device class for Modbus RTU communication with a PLC42.
 *
 * This class manages the connection, reading of digital inputs and holding registers,
 * as well as writing to holding registers of a PLC42 programmable logic controller via Modbus RTU protocol.
 * It ensures connection state management and error handling for reliable communication.
 *
 * @section Categories Functional Categories
 * - **Modbus Communication**: Manages Modbus RTU connection and requests.
 * - **Data Reading**: Acquires digital inputs (Discrete Inputs) and holding registers (Holding Registers).
 * - **Data Writing**: Controls holding registers.
 * - **State Management**: Tracks connection status and PLC data.
 * - **Error Handling**: Processes Modbus errors and communication issues.
 * - **Synchronization**: Uses mutex for thread-safe access to shared data.
 *
 * @section SignalsAndSlots Signals and Slots Organization
 * - **State Communication**: `stateChanged` (QModbusClient) -> `onStateChanged` (Plc42Device)
 * - **Error Handling**: `errorOccurred` (QModbusClient) -> `onErrorOccurred` (Plc42Device)
 * - **Periodic Reading**: `timeout` (m_pollTimer) -> `readData` (Plc42Device)
 * - **Read Responses**: `finished` (QModbusReply) -> `onDigitalInputsReadReady`, `onHoldingDataReadReady` (Plc42Device)
 * - **Write Responses**: `finished` (QModbusReply) -> `onWriteReady` (Plc42Device)
 * - **Timeout Handling**: `timeout` (m_timeoutTimer) -> `handleTimeout` (Plc42Device)
 * - **Error Notification**: `errorOccurred` (Plc42Device)
 * - **Log Notification**: `logMessage` (Plc42Device)
 * - **Data Change Notification**: `plc42DataChanged` (Plc42Device)
 *
 * @author ieta_maher
 * @date 2025-06-20
 * @version 1.0
 */

#ifndef PLC42DEVICE_H
#define PLC42DEVICE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QModbusRtuSerialClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QVector>
#include "modbusdevicebase.h"


/**
 * @brief Combined structure representing all PLC42 data (digital inputs + holding registers).
 *
 * This structure aggregates the states of digital sensors and the values of holding
 * registers read from and written to the PLC42. It includes a connection indicator
 * and comparison operators to detect state changes.
 */
struct Plc42Data {
    bool isConnected             = false; ///< Device connection status.

    // Discrete inputs
    bool stationUpperSensor      = false; ///< State of the station upper sensor.
    bool stationLowerSensor      = false; ///< State of the station lower sensor.
    bool emergencyStopActive     = false; ///< State of the emergency stop.
    bool ammunitionLevel         = false; ///< State of the ammunition level.
    bool stationInput1           = false; ///< State of station input 1.
    bool stationInput2           = false; ///< State of station input 2.
    bool stationInput3           = false; ///< State of station input 3.
    bool solenoidActive          = false; ///< State of solenoid activation.

    // Holding registers
    uint16_t solenoidMode        = 0;     ///< Solenoid mode.
    uint16_t gimbalOpMode        = 0;     ///< Gimbal operating mode.
    uint32_t azimuthSpeed        = 0;     ///< Azimuth speed (32-bit value).
    uint32_t elevationSpeed      = 0;     ///< Elevation speed (32-bit value).
    uint16_t azimuthDirection    = 0;     ///< Azimuth direction.
    uint16_t elevationDirection  = 0;     ///< Elevation direction.
    uint16_t solenoidState       = 0;     ///< Solenoid state.
    uint16_t resetAlarm          = 0;     ///< Alarm reset command.

    /**
     * @brief Equality comparison operator for Plc42Data.
     * @param other The other Plc42Data object to compare.
     * @return True if all members are equal, false otherwise.
     */
    bool operator==(const Plc42Data &other) const {
        return (
            isConnected             == other.isConnected &&
            stationUpperSensor      == other.stationUpperSensor &&
            stationLowerSensor      == other.stationLowerSensor &&
            emergencyStopActive     == other.emergencyStopActive &&
            ammunitionLevel         == other.ammunitionLevel &&
            stationInput1           == other.stationInput1 &&
            stationInput2           == other.stationInput2 &&
            stationInput3           == other.stationInput3 &&
            solenoidActive          == other.solenoidActive &&
            solenoidMode            == other.solenoidMode &&
            gimbalOpMode            == other.gimbalOpMode &&
            azimuthSpeed            == other.azimuthSpeed &&
            elevationSpeed          == other.elevationSpeed &&
            azimuthDirection        == other.azimuthDirection &&
            elevationDirection      == other.elevationDirection &&
            solenoidState           == other.solenoidState &&
            resetAlarm              == other.resetAlarm
            );
    }

    /**
     * @brief Inequality comparison operator for Plc42Data.
     * @param other The other Plc42Data object to compare.
     * @return True if at least one member is different, false otherwise.
     */
    bool operator!=(const Plc42Data &other) const {
        return !(*this == other);
    }
};

/**
 * @brief PLC42 device communication class.
 *
 * Inherits from ModbusDeviceBase and implements PLC42-specific functionality
 * for reading digital inputs and holding registers, and writing control parameters.
 */
class Plc42Device : public ModbusDeviceBase
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the Plc42Device class.
     * @param device Serial port name (e.g., "COM1" or "/dev/ttyUSB0").
     * @param baudRate Baud rate for serial communication.
     * @param slaveId Modbus slave ID of the PLC42 device.
     * @param parent QObject parent for memory management.
     */
    explicit Plc42Device(const QString &device,
                         int baudRate,
                         int slaveId,
                         QSerialPort::Parity parity,
                         QObject *parent = nullptr);
    
    /**
     * @brief Destructor.
     */
    virtual ~Plc42Device();

    // Data access
    /**
     * @brief Returns the current PLC42 data structure.
     * @return Current device data.
     */
    Plc42Data currentData() const { return m_currentData; }

    // Control methods
    /**
     * @brief Sets the solenoid mode.
     * @param mode Solenoid operating mode.
     */
    void setSolenoidMode(uint16_t mode);
    
    /**
     * @brief Sets the gimbal motion mode.
     * @param mode Gimbal operating mode.
     */
    void setGimbalMotionMode(uint16_t mode);
    
    /**
     * @brief Sets the azimuth speed.
     * @param speed Azimuth movement speed.
     */
    void setAzimuthSpeedHolding(uint32_t speed);
    
    /**
     * @brief Sets the elevation speed.
     * @param speed Elevation movement speed.
     */
    void setElevationSpeedHolding(uint32_t speed);
    
    /**
     * @brief Sets the azimuth direction.
     * @param direction Azimuth movement direction.
     */
    void setAzimuthDirection(uint16_t direction);
    
    /**
     * @brief Sets the elevation direction.
     * @param direction Elevation movement direction.
     */
    void setElevationDirection(uint16_t direction);
    
    /**
     * @brief Sets the solenoid state.
     * @param state Solenoid activation state.
     */
    void setSolenoidState(uint16_t state);
    
    /**
     * @brief Sets the alarm reset command.
     * @param alarm Alarm reset value.
     */
    void setResetAlarm(uint16_t alarm);

signals:
    /**
     * @brief Emitted when PLC42 data changes.
     * @param data The updated PLC42 data structure.
     */
    void plc42DataChanged(const Plc42Data &data);

protected:
    // Implementation of pure virtual methods from ModbusDeviceBase
    /**
     * @brief Reads data from the PLC42 device.
     * Overrides the pure virtual method from ModbusDeviceBase.
     */
    void readData() override;
    
    /**
     * @brief Called when data read operations complete.
     * Overrides the pure virtual method from ModbusDeviceBase.
     */
    void onDataReadComplete() override;
    
    /**
     * @brief Called when write operations complete.
     * Overrides the pure virtual method from ModbusDeviceBase.
     */
    void onWriteComplete() override;

private slots:
    /**
     * @brief Handles the response for digital input read requests.
     */
    void onDigitalInputsReadReady(QModbusReply *reply);
    
    /**
     * @brief Handles the response for holding register read requests.
     */
    void onHoldingDataReadReady(QModbusReply *reply);
    
    /**
     * @brief Handles the response for write requests.
     */
    void onWriteReady(QModbusReply *reply);

private:
    // PLC42-specific methods
    /**
     * @brief Reads digital inputs from the PLC.
     */
    void readDigitalInputs();
    
    /**
     * @brief Reads holding registers from the PLC.
     */
    void readHoldingData();
    
    /**
     * @brief Writes the cached holding register values to the PLC.
     */
    void writeRegisterData();
    
    /**
     * @brief Updates the internal PLC42 data and emits signal if changed.
     * @param newData The new data to update.
     */
    void updatePlc42Data(const Plc42Data &newData);

    // Register address constants
     static constexpr int DIGITAL_INPUTS_START_ADDRESS  = 0;
    static constexpr int DIGITAL_INPUTS_COUNT          = 13;
    static constexpr int HOLDING_REGISTERS_START       = 0;
    static constexpr int HOLDING_REGISTERS_COUNT       = 7;
    static constexpr int HOLDING_REGISTERS_START_ADDRESS = 10;
    // Member variables
    Plc42Data m_currentData;
};

#endif // PLC42DEVICE_H
