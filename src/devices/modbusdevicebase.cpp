#include "modbusdevicebase.h"
#include <QDebug>
#include <QMutexLocker>
#include <QVariant>
#include <QtMath>
#include <QVariant>
#include <QPointer> 

ModbusDeviceBase::ModbusDeviceBase(const QString &device,
                                   int baudRate,
                                   int slaveId,
                                   QSerialPort::Parity parity,
                                   QObject *parent)
    : QObject(parent),
    m_device(device),
    m_baudRate(baudRate),
    m_slaveId(slaveId),
    m_parity(parity),
    m_modbusDevice(new QModbusRtuSerialClient(this)),
    m_pollTimer(new QTimer(this)),
    m_timeoutTimer(new QTimer(this)),
    m_reconnectAttempts(0)
{
    setupModbusConnection();
    connectSignals();
}

ModbusDeviceBase::~ModbusDeviceBase()
{
    // First, stop all timers to prevent further signal emissions
    if (m_pollTimer) {
        m_pollTimer->stop();
        m_pollTimer->disconnect(); // Disconnect all signals from timer
    }
    
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->disconnect(); // Disconnect all signals from timer
    }
    
    // Disconnect the device and stop all communications
    if (m_modbusDevice) {
        // Disconnect all signals from the modbus device first
        m_modbusDevice->disconnect();
        
        // Then disconnect the device itself
        if (m_modbusDevice->state() != QModbusDevice::UnconnectedState) {
            m_modbusDevice->disconnectDevice();
        }
    }
    //disconnectDevice();

}

void ModbusDeviceBase::setupModbusConnection()
{
    // Set Modbus connection parameters for serial port
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter, m_device);
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter, m_baudRate);
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter, QSerialPort::Data8);
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter, QSerialPort::OneStop);
    m_modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter, m_parity);  // Use configurable parity

    // Set default Modbus communication timeouts and retries
    m_modbusDevice->setTimeout(500);
    m_modbusDevice->setNumberOfRetries(3);

    // Configure timers
    m_pollTimer->setInterval(100); // Default poll interval
    m_timeoutTimer->setSingleShot(true);
}

void ModbusDeviceBase::connectSignals()
{
    // Connect Modbus client state and error signals to slots
    connect(m_modbusDevice, &QModbusClient::stateChanged,
            this, &ModbusDeviceBase::onStateChanged);
    connect(m_modbusDevice, &QModbusClient::errorOccurred,
            this, &ModbusDeviceBase::onErrorOccurred);

    // Connect poll timer to readData slot for periodic data acquisition
    connect(m_pollTimer, &QTimer::timeout,
            this, &ModbusDeviceBase::readData);

    // Connect timeout timer to handleTimeout slot for response timeouts
    connect(m_timeoutTimer, &QTimer::timeout,
            this, &ModbusDeviceBase::handleTimeout);
}

bool ModbusDeviceBase::connectDevice()
{
    // Disconnect if already connected or in a transient state
    if (m_modbusDevice->state() != QModbusDevice::UnconnectedState) {
        m_modbusDevice->disconnectDevice();
    }

    // Attempt to connect and log any errors
    if (!m_modbusDevice->connectDevice()) {
        logError(QString("Failed to connect to Modbus device: %1").arg(m_modbusDevice->errorString()));
        qDebug() << QString("Failed to connect to Modbus device: %1").arg(m_modbusDevice->errorString());
        return false;
    }

    logMessage("Attempting to connect to Modbus device...");
    return true;
}

void ModbusDeviceBase::disconnectDevice()
{
    if (m_modbusDevice && m_modbusDevice->state() != QModbusDevice::UnconnectedState) {
        m_modbusDevice->disconnectDevice();
    }
    stopPolling();
    stopTimeoutTimer();
}

bool ModbusDeviceBase::isConnected() const
{
    return m_modbusDevice && m_modbusDevice->state() == QModbusDevice::ConnectedState;
}

void ModbusDeviceBase::setTimeout(int timeoutMs)
{
    if (m_modbusDevice) {
        m_modbusDevice->setTimeout(timeoutMs);
    }
}

void ModbusDeviceBase::setRetries(int retries)
{
    if (m_modbusDevice) {
        m_modbusDevice->setNumberOfRetries(retries);
    }
}

void ModbusDeviceBase::setPollInterval(int intervalMs)
{
    m_pollTimer->setInterval(intervalMs);
}

void ModbusDeviceBase::onStateChanged(QModbusDevice::State state)
{
    if (state == QModbusDevice::ConnectedState) {
        logMessage("Modbus connection established.");
        emit connectionStateChanged(true);
        startPolling();
        resetReconnectionAttempts();
        // Don't call onDataReadComplete() here - it causes issues with derived classes
    } else if (state == QModbusDevice::UnconnectedState) {
        logMessage("Modbus device disconnected.");
        emit connectionStateChanged(false);
        stopPolling();
        // Don't call onDataReadComplete() here - it causes issues with derived classes
    }
}

void ModbusDeviceBase::onErrorOccurred(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError)
        return;

    logError(QString("Modbus error: %1").arg(m_modbusDevice->errorString()));
    emit errorOccurred(m_modbusDevice->errorString());
}

void ModbusDeviceBase::handleTimeout()
{
    logError("Timeout waiting for response from Modbus device.");
    emit errorOccurred("Timeout waiting for response from Modbus device.");

    // Check if maximum reconnection attempts have been reached
    if (m_reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
        logError("Maximum reconnection attempts reached. Stopping reconnection attempts.");
        emit maxReconnectionAttemptsReached();
        return;
    }

    attemptReconnection();
}

void ModbusDeviceBase::attemptReconnection()
{
    m_reconnectAttempts++;

    // Calculate exponential backoff delay for reconnection
    int delay = BASE_RECONNECT_DELAY_MS * static_cast<int>(qPow(2, m_reconnectAttempts - 1));

    logMessage(QString("Attempting to reconnect... (Attempt %1, Delay %2 ms)")
                   .arg(m_reconnectAttempts)
                   .arg(delay));

    if (m_modbusDevice) {
        m_modbusDevice->disconnectDevice();
        QTimer::singleShot(delay, this, &ModbusDeviceBase::connectDevice);
    }
}

void ModbusDeviceBase::logError(const QString &message)
{
    emit logMessage(message);
    //qDebug() << "ModbusDeviceBase:" << message;
}

void ModbusDeviceBase::startPolling()
{
    if (!m_pollTimer->isActive()) {
        m_pollTimer->start();
    }
}

void ModbusDeviceBase::stopPolling()
{
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
}

void ModbusDeviceBase::startTimeoutTimer(int timeoutMs)
{
    if (!m_timeoutTimer->isActive()) {
        m_timeoutTimer->start(timeoutMs);
    }
}

void ModbusDeviceBase::stopTimeoutTimer()
{
    if (m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
    }
}

QModbusReply* ModbusDeviceBase::sendReadRequest(const QModbusDataUnit &readUnit)
{
    if (!m_modbusDevice || m_modbusDevice->state() != QModbusDevice::ConnectedState) {
        logError("Cannot send read request: device not connected");
        return nullptr;
    }

    QMutexLocker locker(&m_mutex);

    QModbusReply *reply = m_modbusDevice->sendReadRequest(readUnit, m_slaveId);
    if (reply && !reply->isFinished()) {
        startTimeoutTimer();
        return reply;
    } else if (reply) {
        reply->deleteLater();
        logError("Read request failed: reply finished immediately");
    } else {
        logError(QString("Read request error: %1").arg(m_modbusDevice->errorString()));
        emit errorOccurred(m_modbusDevice->errorString());
    }

    return nullptr;
}

QModbusReply* ModbusDeviceBase::sendWriteRequest(const QModbusDataUnit &writeUnit)
{
    if (!m_modbusDevice || m_modbusDevice->state() != QModbusDevice::ConnectedState) {
        logError("Cannot send write request: device not connected");
        return nullptr;
    }

    QMutexLocker locker(&m_mutex);

    QModbusReply *reply = m_modbusDevice->sendWriteRequest(writeUnit, m_slaveId);
    if (reply && !reply->isFinished()) {
        return reply;
    } else if (reply) {
        reply->deleteLater();
        logError("Write request failed: reply finished immediately");
    } else {
        logError(QString("Write request error: %1").arg(m_modbusDevice->errorString()));
        emit errorOccurred(m_modbusDevice->errorString());
    }

    return nullptr;
}

void ModbusDeviceBase::connectReplyFinished(QModbusReply *reply, std::function<void(QModbusReply*)> slotFunction)
{
    if (!reply) return;

    // Use QPointer to safely capture 'this'. If 'this' is destroyed, 'self' will become null.
    QPointer<ModbusDeviceBase> self = this;
    connect(reply, &QModbusReply::finished, this, [self, reply, slotFunction]() {
        if (self) {
            // Only call the slot function if the ModbusDeviceBase object still exists
            slotFunction(reply);
        }
        // Always ensure the reply is deleted, regardless of whether 'self' exists
        reply->deleteLater();
    });
}