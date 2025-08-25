#include "baseserialdevice.h"
#include <QDebug>

BaseSerialDevice::BaseSerialDevice(QObject *parent)
    : QObject(parent),
    m_serialPort(new QSerialPort(this)),
    m_isConnected(false),
    m_reconnectAttempts(0),
    m_reconnectTimer(new QTimer(this))
{
    // Connect serial port signals
    connect(m_serialPort, &QSerialPort::readyRead,
            this, &BaseSerialDevice::onSerialDataReady);
    connect(m_serialPort, &QSerialPort::errorOccurred,
            this, &BaseSerialDevice::handleSerialError);

    // Setup reconnection timer
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &BaseSerialDevice::attemptReconnection);
}

BaseSerialDevice::~BaseSerialDevice()
{
    shutdown();
}

bool BaseSerialDevice::openSerialPort(const QString &portName)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_lastPortName = portName;
    m_serialPort->setPortName(portName);

    // Let derived class configure the port
    configureSerialPort();

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        logMessage(QString("Serial port opened: %1").arg(portName));
        m_reconnectAttempts = 0;
        setConnectionState(true);
        onConnectionEstablished();
        return true;
    } else {
        logError(QString("Failed to open serial port: %1 - %2")
                     .arg(portName, m_serialPort->errorString()));
        setConnectionState(false);
        return false;
    }
}

void BaseSerialDevice::closeSerialPort()
{
    if (m_serialPort->isOpen()) {
        logMessage(QString("Closing serial port: %1").arg(m_serialPort->portName()));
        m_serialPort->close();
        setConnectionState(false);
    }
}

void BaseSerialDevice::shutdown()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    closeSerialPort();
}

bool BaseSerialDevice::isConnected() const
{
    return m_serialPort && m_serialPort->isOpen() && m_isConnected;
}

void BaseSerialDevice::logMessage(const QString &message)
{
    emit logMessages(message);
    qDebug() << metaObject()->className() << ":" << message;
}

void BaseSerialDevice::logError(const QString &message)
{
    emit logMessages(message);
    emit errorOccurred(message);
    qWarning() << metaObject()->className() << ":" << message;
}

void BaseSerialDevice::sendData(const QByteArray &data)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        logError("Cannot send data: serial port not open");
        return;
    }

    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten != data.size()) {
        logError("Failed to write all data to serial port");
    }
    m_serialPort->flush();
}

bool BaseSerialDevice::waitForResponse(int timeoutMs)
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return false;
    }
    return m_serialPort->waitForReadyRead(timeoutMs);
}

void BaseSerialDevice::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    logError(QString("Serial port error: %1").arg(m_serialPort->errorString()));

    // Handle critical errors that indicate connection loss
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::DeviceNotFoundError) {

        setConnectionState(false);
        onConnectionLost();
        closeSerialPort();

        // Attempt reconnection if enabled
        if (shouldAttemptReconnection() &&
            m_reconnectAttempts < getMaxReconnectAttempts()) {

            int delay = getReconnectDelayMs(m_reconnectAttempts);
            m_reconnectTimer->start(delay);
        } else if (m_reconnectAttempts >= getMaxReconnectAttempts()) {
            logError("Maximum reconnection attempts reached");
        }
    }
}

void BaseSerialDevice::attemptReconnection()
{
    if (!m_serialPort->isOpen() && !m_lastPortName.isEmpty()) {
        m_reconnectAttempts++;
        logMessage(QString("Attempting reconnection... (Attempt %1)")
                       .arg(m_reconnectAttempts));

        if (openSerialPort(m_lastPortName)) {
            logMessage(QString("Reconnected to port %1").arg(m_lastPortName));
        } else if (m_reconnectAttempts < getMaxReconnectAttempts()) {
            // Schedule another attempt
            int delay = getReconnectDelayMs(m_reconnectAttempts);
            m_reconnectTimer->start(delay);
        } else {
            logError("Maximum reconnection attempts reached");
        }
    }
}

void BaseSerialDevice::onSerialDataReady()
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return;
    }

    m_readBuffer.append(m_serialPort->readAll());
    processIncomingData();
}

void BaseSerialDevice::setConnectionState(bool connected)
{
    if (m_isConnected != connected) {
        m_isConnected = connected;
        emit connectionStateChanged(connected);
    }
}


