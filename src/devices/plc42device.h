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
 * @brief PLC42 data structure - UPDATED FOR HOME POSITION DETECTION
 * 
 * HARDWARE MAPPING (MDUINO 42+ Station Control):
 * Digital Inputs (8 total):
 *   DI0 (I0_0) → stationUpperSensor (Upper limit)
 *   DI1 (I0_1) → stationLowerSensor (Lower limit)
 *   DI2 (I0_2) → hatchState (Hatch position)
 *   DI3 (I0_3) → freeGimbalState (FREE toggle switch - LOCAL CONTROL)
 *   DI4 (I0_4) → ammunitionLevel (Ammo sensor)
 *   DI5 (I0_5) → Reserved (future E-STOP button)
 *   DI6 (I0_6) → azimuthHomeComplete ⭐ NEW (Az HOME-END from Oriental Motor)
 *   DI7 (I0_7) → elevationHomeComplete ⭐ NEW (El HOME-END from Oriental Motor)
 * 
 * Holding Registers (10 total):
 *   HR0: solenoidMode (1=Single, 2=Burst, 3=Continuous)
 *   HR1: gimbalOpMode (0=Manual, 1=Stop, 3=Home, 4=Free)
 *   HR2-3: azimuthSpeed (32-bit)
 *   HR4-5: elevationSpeed (32-bit)
 *   HR6: azimuthDirection
 *   HR7: elevationDirection
 *   HR8: solenoidState (0=OFF, 1=ON)
 *   HR9: resetAlarm (0=Normal, 1=Reset)
 */
struct Plc42Data {
    bool isConnected = false;

    // =========================================================================
    // DISCRETE INPUTS (8 inputs)
    // =========================================================================
    bool stationUpperSensor = false;       ///< DI0: Upper limit sensor
    bool stationLowerSensor = false;       ///< DI1: Lower limit sensor
    bool hatchState = false;               ///< DI2: Hatch state
    bool freeGimbalState = false;          ///< DI3: FREE mode toggle (LOCAL)
    bool ammunitionLevel = false;          ///< DI4: Ammunition level
    // DI5: Reserved (future E-STOP button)
    bool azimuthHomeComplete = false;      ///< DI6: Az HOME-END signal ⭐ NEW
    bool elevationHomeComplete = false;    ///< DI7: El HOME-END signal ⭐ NEW
    
    // Derived values (computed in parser)
    bool emergencyStopActive = false;      ///< Derived: gimbalOpMode == 1
    bool solenoidActive = false;           ///< Derived: solenoidState != 0

    // =========================================================================
    // HOLDING REGISTERS (10 registers)
    // =========================================================================
    uint16_t solenoidMode = 0;          ///< HR0: Fire mode
    uint16_t gimbalOpMode = 0;          ///< HR1: Gimbal operation mode
    uint32_t azimuthSpeed = 0;          ///< HR2-3: Azimuth speed (32-bit)
    uint32_t elevationSpeed = 0;        ///< HR4-5: Elevation speed (32-bit)
    uint16_t azimuthDirection = 0;      ///< HR6: Azimuth direction
    uint16_t elevationDirection = 0;    ///< HR7: Elevation direction
    uint16_t solenoidState = 0;         ///< HR8: Trigger command
    uint16_t resetAlarm = 0;            ///< HR9: Error reset

    bool operator!=(const Plc42Data &other) const {
        return (isConnected != other.isConnected ||
                stationUpperSensor != other.stationUpperSensor ||
                stationLowerSensor != other.stationLowerSensor ||
                hatchState != other.hatchState ||
                freeGimbalState != other.freeGimbalState ||
                ammunitionLevel != other.ammunitionLevel ||
                azimuthHomeComplete != other.azimuthHomeComplete ||      // ⭐ NEW
                elevationHomeComplete != other.elevationHomeComplete ||  // ⭐ NEW
                emergencyStopActive != other.emergencyStopActive ||
                solenoidActive != other.solenoidActive ||
                solenoidMode != other.solenoidMode ||
                gimbalOpMode != other.gimbalOpMode ||
                azimuthSpeed != other.azimuthSpeed ||
                elevationSpeed != other.elevationSpeed ||
                azimuthDirection != other.azimuthDirection ||
                elevationDirection != other.elevationDirection ||
                solenoidState != other.solenoidState ||
                resetAlarm != other.resetAlarm);
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
    void onCommunicationWatchdogTimeout();
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
    void resetCommunicationWatchdog();
    void setConnectionState(bool connected);
    // Register address constants
     static constexpr int DIGITAL_INPUTS_START_ADDRESS  = 0;
    static constexpr int DIGITAL_INPUTS_COUNT          = 13;
    static constexpr int HOLDING_REGISTERS_START       = 0;
    static constexpr int HOLDING_REGISTERS_COUNT       = 7;
    static constexpr int HOLDING_REGISTERS_START_ADDRESS = 10;
    // Member variables
    Plc42Data m_currentData;
    QTimer* m_communicationWatchdog;
    static constexpr int COMMUNICATION_TIMEOUT_MS = 3000;
};

#endif // PLC42DEVICE_H
