/**
 * @file modbusdevicebase.h
 * @brief Declaration of the ModbusDeviceBase abstract class for Modbus RTU communication.
 *
 * This abstract base class provides common functionality for Modbus RTU communication
 * with industrial devices via serial links. It encapsulates connection management,
 * error handling, automatic reconnection mechanisms, and basic read/write operations.
 * Derived classes implement device-specific data parsing and control logic.
 *
 * @section Categories Functional Categories
 * - **Connection Management**: Establishes and maintains Modbus RTU serial connections.
 * - **Communication Setup**: Configures serial port parameters and Modbus settings.
 * - **Error Handling**: Processes Modbus errors and communication failures.
 * - **Automatic Reconnection**: Implements exponential backoff reconnection strategy.
 * - **Polling Management**: Controls periodic data acquisition from devices.
 * - **Thread Safety**: Uses mutex for thread-safe access to shared resources.
 * - **Timeout Management**: Handles communication timeouts and recovery.
 * - **Logging**: Provides standardized error and status message logging.
 *
 * @section SignalsAndSlots Signals and Slots Organization
 * - **State Management**: `stateChanged` (QModbusClient) -> `onStateChanged` (ModbusDeviceBase)
 * - **Error Handling**: `errorOccurred` (QModbusClient) -> `onErrorOccurred` (ModbusDeviceBase)
 * - **Periodic Reading**: `timeout` (m_pollTimer) -> `readData` (derived class)
 * - **Timeout Handling**: `timeout` (m_timeoutTimer) -> `handleTimeout` (ModbusDeviceBase)
 * - **Connection Notification**: `connectionStateChanged` (ModbusDeviceBase)
 * - **Error Notification**: `errorOccurred` (ModbusDeviceBase)
 * - **Log Notification**: `logMessage` (ModbusDeviceBase)
 * - **Reconnection Notification**: `maxReconnectionAttemptsReached` (ModbusDeviceBase)
 *
 * @section Usage Usage Pattern
 * Derived classes should:
 * 1. Implement pure virtual methods: `readData()`, `onDataReadComplete()`, `onWriteComplete()`
 * 2. Use `sendReadRequest()` and `sendWriteRequest()` for Modbus operations
 * 3. Call `startPolling()` after successful connection
 * 4. Handle device-specific data parsing in read/write completion handlers
 *
 * @author ieta_maher
 * @date 2025-06-21
 * @version 1.0
 */

#ifndef MODBUSDEVICEBASE_H
#define MODBUSDEVICEBASE_H

#include <QObject>
#include <QModbusRtuSerialClient>
#include <QModbusReply>
#include <QTimer>
#include <QMutex>
#include <QString>
#include <QSerialPort>

/**
 * @brief Abstract base class for Modbus RTU device communication.
 *
 * This class provides a foundation for implementing Modbus RTU communication
 * with various industrial devices. It handles common operations such as
 * connection management, error handling, reconnection logic, and provides
 * helper methods for read/write operations.
 *
 * Derived classes must implement device-specific data handling methods
 * and define their own data structures and Modbus register mappings.
 */
class ModbusDeviceBase : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for the ModbusDeviceBase class.
     * @param device Serial port name (e.g., "COM1" or "/dev/ttyUSB0").
     * @param baudRate Baud rate for serial communication (typically 9600, 19200, 38400, 115200).
     * @param slaveId Modbus slave ID of the target device (1-247).
     * @param parity Serial port parity setting (default: EvenParity).
     * @param parent QObject parent for memory management.
     */
    explicit ModbusDeviceBase(const QString &device,
                             int baudRate,
                             int slaveId,
                             QSerialPort::Parity parity = QSerialPort::EvenParity,
                             QObject *parent = nullptr);
    
    /**
     * @brief Virtual destructor for proper cleanup in derived classes.
     */
    virtual ~ModbusDeviceBase();

    // Connection Management
    /**
     * @brief Attempts to establish a connection with the Modbus device.
     * @return True if the connection attempt is successfully initiated, false otherwise.
     */
    virtual bool connectDevice();
    
    /**
     * @brief Disconnects the Modbus device and stops all timers.
     */
    virtual void disconnectDevice();
    
    // Connection Information Getters
    /**
     * @brief Returns the configured serial device name.
     * @return Serial port name (e.g., "COM1", "/dev/ttyUSB0").
     */
    QString device() const { return m_device; }
    
    /**
     * @brief Returns the configured baud rate.
     * @return Baud rate value.
     */
    int baudRate() const { return m_baudRate; }
    
    /**
     * @brief Returns the configured Modbus slave ID.
     * @return Slave ID (1-247).
     */
    int slaveId() const { return m_slaveId; }
    
    /**
     * @brief Returns the configured parity setting.
     * @return Parity setting.
     */
    QSerialPort::Parity parity() const { return m_parity; }
    
    /**
     * @brief Checks if the device is currently connected.
     * @return True if connected, false otherwise.
     */
    bool isConnected() const;

    // Runtime Configuration
    /**
     * @brief Sets the Modbus communication timeout.
     * @param timeoutMs Timeout in milliseconds.
     */
    void setTimeout(int timeoutMs);
    
    /**
     * @brief Sets the number of retry attempts for failed communications.
     * @param retries Number of retry attempts.
     */
    void setRetries(int retries);
    
    /**
     * @brief Sets the polling interval for periodic data reading.
     * @param intervalMs Polling interval in milliseconds.
     */
    void setPollInterval(int intervalMs);

signals:
    /**
     * @brief Emitted when a log message needs to be recorded.
     * @param message The log message string.
     */
    void logMessage(const QString &message);
    
    /**
     * @brief Emitted when a communication or device error occurs.
     * @param error The error description string.
     */
    void errorOccurred(const QString &error);
    
    /**
     * @brief Emitted when the connection state changes.
     * @param connected True if connected, false if disconnected.
     */
    void connectionStateChanged(bool connected);
    
    /**
     * @brief Emitted when maximum reconnection attempts have been reached.
     * This indicates that automatic reconnection has been abandoned.
     */
    void maxReconnectionAttemptsReached();

protected slots:
    /**
     * @brief Handles changes in the Modbus connection state.
     * @param state The new connection state.
     */
    virtual void onStateChanged(QModbusDevice::State state);
    
    /**
     * @brief Handles errors reported by the Modbus device.
     * @param error The error type that occurred.
     */
    virtual void onErrorOccurred(QModbusDevice::Error error);
    
    /**
     * @brief Handles communication timeouts and initiates reconnection.
     */
    virtual void handleTimeout();

protected:
    // Pure Virtual Methods - Must be implemented by derived classes
    /**
     * @brief Pure virtual method for reading data from the Modbus device.
     * Derived classes must implement this to define their specific read operations.
     */
    virtual void readData() = 0;
    
    /**
     * @brief Pure virtual method called when data read operations complete.
     * Derived classes use this to update their internal state and emit signals.
     */
    virtual void onDataReadComplete() = 0;
    
    /**
     * @brief Pure virtual method called when write operations complete.
     * Derived classes use this to handle write completion events.
     */
    virtual void onWriteComplete() = 0;

    // Utility Methods for Derived Classes
    /**
     * @brief Logs an error message and emits logMessage signal.
     * @param message The error message to log.
     */
    void logError(const QString &message);
    
    /**
     * @brief Starts the polling timer for periodic data acquisition.
     */
    void startPolling();
    
    /**
     * @brief Stops the polling timer.
     */
    void stopPolling();
    
    /**
     * @brief Starts the timeout timer for communication timeout detection.
     * @param timeoutMs Timeout duration in milliseconds (default: 1000ms).
     */
    void startTimeoutTimer(int timeoutMs = 1000);
    
    /**
     * @brief Stops the timeout timer.
     */
    void stopTimeoutTimer();
    
    // Modbus Communication Helper Methods
    /**
     * @brief Sends a read request to the Modbus device.
     * @param readUnit The Modbus data unit specifying what to read.
     * @return Pointer to the QModbusReply object, or nullptr if the request failed.
     */
    QModbusReply* sendReadRequest(const QModbusDataUnit &readUnit);
    
    /**
     * @brief Sends a write request to the Modbus device.
     * @param writeUnit The Modbus data unit specifying what to write.
     * @return Pointer to the QModbusReply object, or nullptr if the request failed.
     */
    QModbusReply* sendWriteRequest(const QModbusDataUnit &writeUnit);
    
    // Reconnection Management
    /**
     * @brief Initiates an automatic reconnection attempt with exponential backoff.
     */
    void attemptReconnection();
    
    /**
     * @brief Resets the reconnection attempt counter to zero.
     */
    void resetReconnectionAttempts() { m_reconnectAttempts = 0; }

    /**
     * @brief Safely connects a QModbusReply's finished signal to a slot function.
     * Ensures the slot is only called if the receiving object (this) still exists.
     * Handles reply deletion.
     * @param reply The QModbusReply object.
     * @param slotFunction The lambda or function to execute when the reply finishes.
     */
    void connectReplyFinished(QModbusReply *reply, std::function<void(QModbusReply*)> slotFunction);
    
    // Protected Member Variables
    /**
     * @brief Serial device name (e.g., "COM1", "/dev/ttyUSB0").
     */
    QString m_device;
    
    /**
     * @brief Serial communication baud rate.
     */
    int m_baudRate;
    
    /**
     * @brief Modbus slave ID of the target device.
     */
    int m_slaveId;
    
    /**
     * @brief Serial port parity setting.
     */
    QSerialPort::Parity m_parity;
    
    /**
     * @brief Pointer to the Modbus RTU serial client object.
     */
    QModbusRtuSerialClient *m_modbusDevice;
    
    /**
     * @brief Timer for periodic data polling.
     */
    QTimer *m_pollTimer;
    
    /**
     * @brief Timer for detecting communication timeouts.
     */
    QTimer *m_timeoutTimer;
    
    /**
     * @brief Mutex for thread-safe access to shared data.
     */
    mutable QMutex m_mutex;

    // Reconnection Parameters
    /**
     * @brief Current number of reconnection attempts.
     */
    int m_reconnectAttempts;
    
    /**
     * @brief Maximum allowed reconnection attempts before giving up.
     */
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    
    /**
     * @brief Base delay in milliseconds for reconnection attempts.
     * Actual delay uses exponential backoff: BASE_DELAY * 2^(attempt-1)
     */
    static constexpr int BASE_RECONNECT_DELAY_MS = 1000;

private:
    /**
     * @brief Configures the Modbus connection parameters.
     * Sets up serial port settings (baud rate, data bits, stop bits, parity).
     */
    void setupModbusConnection();
    
    /**
     * @brief Connects internal signals and slots for the base class functionality.
     */
    void connectSignals();
};

#endif // MODBUSDEVICEBASE_H