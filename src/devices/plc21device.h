/**
 * @file plc21device.h
 * @brief Declaration of the Plc21Device class for Modbus RTU communication with a PLC21.
 *
 * This class manages the connection, reading, and writing of data (digital and analog inputs/outputs)
 * for a PLC21 programmable logic controller via Modbus RTU protocol over a serial link. It provides
 * automatic reconnection mechanisms and error handling to ensure robust communication.
 *
 * @section Categories Functional Categories
 * - **Modbus Communication**: Manages Modbus RTU connection and requests.
 * - **Data Reading**: Acquires digital inputs (Discrete Inputs) and analog inputs (Holding Registers).
 * - **Data Writing**: Controls digital outputs (Coils).
 * - **State Management**: Tracks connection status and panel data.
 * - **Error Handling**: Processes Modbus errors and communication issues.
 * - **Automatic Reconnection**: Attempts to reconnect upon communication loss.
 * - **Synchronization**: Uses mutex for thread-safe access to shared data.
 *
 * @section SignalsAndSlots Signals and Slots Organization
 * - **State Communication**: `stateChanged` (QModbusClient) -> `onStateChanged` (Plc21Device)
 * - **Error Handling**: `errorOccurred` (QModbusClient) -> `onErrorOccurred` (Plc21Device)
 * - **Periodic Reading**: `timeout` (m_readTimer) -> `readData` (Plc21Device)
 * - **Read Responses**: `finished` (QModbusReply) -> `onDigitalInputsReadReady`, `onAnalogInputsReadReady` (Plc21Device)
 * - **Write Responses**: `finished` (QModbusReply) -> `onWriteReady` (Plc21Device)
 * - **Timeout Handling**: `timeout` (m_timeoutTimer) -> `handleTimeout` (Plc21Device)
 * - **Error Notification**: `errorOccurred` (Plc21Device)
 * - **Log Notification**: `logMessage` (Plc21Device)
 * - **Reconnection Notification**: `maxReconnectionAttemptsReached` (Plc21Device)
 * - **Data Change Notification**: `panelDataChanged` (Plc21Device)
 *
 * @author ieta_maher
 * @date 2025-06-20
 * @version 1.0
 */

#ifndef PLC21DEVICE_H
#define PLC21DEVICE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QModbusRtuSerialClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <QVector>
#include "modbusdevicebase.h"

/**
 * @brief Structure to hold the panel data from the PLC21.
 *
 * This structure aggregates all digital inputs (buttons, switches)
 * and analog inputs (modes, temperatures) read from the PLC21.
 * It also includes a connection indicator and comparison operators
 * to detect state changes.
 */
struct Plc21PanelData {
    bool isConnected       = false; ///< Device connection status.

    // Digital Inputs
    bool armGunSW          = false; ///< State of the gun arming switch.
    bool loadAmmunitionSW  = false; ///< State of the ammunition loading switch.
    bool enableStationSW   = false; ///< State of the station enable switch.
    bool homePositionSW    = false; ///< State of the home position switch.
    bool enableStabilizationSW = false; ///< State of the stabilization enable switch.
    bool authorizeSw       = false; ///< State of the authorization switch.
    bool switchCameraSW    = false; ///< State of the camera switch.
    bool menuUpSW          = false; ///< State of the 'Menu Up' button.
    bool menuDownSW        = false; ///< State of the 'Menu Down' button.
    bool menuValSw         = false; ///< State of the 'Menu Validate' button.

    // Analog Inputs (Holding Registers)
    int  speedSW           = 2;     ///< Value of the speed switch.
    int  fireMode          = 0;     ///< Current fire mode.
    int  panelTemperature  = 0;     ///< Panel temperature.

    /**
     * @brief Equality comparison operator for Plc21PanelData.
     * @param other The other Plc21PanelData object to compare.
     * @return True if all members are equal, false otherwise.
     */
    bool operator==(const Plc21PanelData &other) const {
        return (
            isConnected       == other.isConnected &&
            armGunSW          == other.armGunSW &&
            loadAmmunitionSW  == other.loadAmmunitionSW &&
            enableStationSW   == other.enableStationSW &&
            homePositionSW    == other.homePositionSW &&
            enableStabilizationSW == other.enableStabilizationSW &&
            authorizeSw       == other.authorizeSw &&
            switchCameraSW    == other.switchCameraSW &&
            menuUpSW          == other.menuUpSW &&
            menuDownSW        == other.menuDownSW &&
            menuValSw         == other.menuValSw &&
            speedSW           == other.speedSW &&
            fireMode          == other.fireMode &&
            panelTemperature  == other.panelTemperature
            );
    }

    /**
     * @brief Inequality comparison operator for Plc21PanelData.
     * @param other The other Plc21PanelData object to compare.
     * @return True if at least one member is different, false otherwise.
     */
    bool operator!=(const Plc21PanelData &other) const {
        return !(*this == other);
    }
};

/**
 * @brief The Plc21Device class manages Modbus communication with a PLC21.
 *
 * It encapsulates the logic for connection, periodic input reading,
 * output writing, and error/reconnection management.
 */
 class Plc21Device : public ModbusDeviceBase
{
    Q_OBJECT

public:
    // PLC-specific Modbus register addresses and counts
    static constexpr int DIGITAL_INPUTS_START_ADDRESS = 0;
    static constexpr int DIGITAL_INPUTS_COUNT = 13;
    static constexpr int ANALOG_INPUTS_START_ADDRESS = 0;
    static constexpr int ANALOG_INPUTS_COUNT = 6;
    static constexpr int DIGITAL_OUTPUTS_START_ADDRESS = 0;
    static constexpr int DIGITAL_OUTPUTS_COUNT = 8;

    explicit Plc21Device(const QString &device,
                         int baudRate,
                         int slaveId,
                         QSerialPort::Parity parity,
                         QObject *parent = nullptr);
    ~Plc21Device() override;

    // Data access methods
    QVector<bool> digitalInputs() const;
    QVector<uint16_t> analogInputs() const;
    void setDigitalOutputs(const QVector<bool> &outputs);

protected:
    // Override pure virtual methods from base class
    void readData() override;
    void onDataReadComplete() override;

private slots:
    // Connection state handling
    void onConnectionStateChanged(bool connected);
    
    // Data handling methods
    void onWriteComplete();
    void onDigitalInputsReadReady(QModbusReply *reply);
    void onAnalogInputsReadReady(QModbusReply *reply);
    void onWriteReady(QModbusReply *reply);
private:
    // Helper methods
    void readDigitalInputs();
    void readAnalogInputs();
    void writeData();
    void updatePanelData(const Plc21PanelData &newData);

    // Data storage
    QVector<bool> m_digitalInputs;
    QVector<uint16_t> m_analogInputs;
    QVector<bool> m_digitalOutputs;
    
    // Current panel data state
    Plc21PanelData m_currentPanelData;
    
    // Thread safety
    mutable QMutex m_mutex;

signals:
    void panelDataChanged(const Plc21PanelData &data);
};

#endif // PLC21DEVICE_H
