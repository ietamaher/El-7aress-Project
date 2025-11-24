#include "imudevice.h"
#include <QDebug>
#include <QMutexLocker>
#include <QtEndian>

ImuDevice::ImuDevice(QObject *parent)
    : BaseSerialDevice(parent),
    m_pollTimer(new QTimer(this)),
    m_gyroBiasTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &ImuDevice::onPollTimer);

    m_gyroBiasTimer->setSingleShot(true);
    m_gyroBiasTimer->setInterval(GYRO_BIAS_TIMEOUT_MS);
    connect(m_gyroBiasTimer, &QTimer::timeout, this, &ImuDevice::onGyroBiasTimeout);
}

ImuDevice::~ImuDevice()
{
    m_pollTimer->stop();
    m_gyroBiasTimer->stop();
}

void ImuDevice::configureSerialPort()
{
    // 3DM-GX3-25 serial configuration
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void ImuDevice::processIncomingData()
{
    // Handle gyro bias response (19 bytes)
    if (m_waitingForGyroBias && m_readBuffer.size() >= 19) {
        logMessage("Gyro bias capture completed");
        m_waitingForGyroBias = false;
        m_gyroBiasTimer->stop();
        m_readBuffer.remove(0, 19);  // Clear gyro bias response

        // Start normal polling
        m_pollTimer->start(m_pollIntervalMs);
        return;
    }

    // Process normal data responses (0xCF = 31 bytes)
    while (m_readBuffer.size() >= RESPONSE_SIZE)
    {
        QByteArray response = m_readBuffer.left(RESPONSE_SIZE);
        m_readBuffer.remove(0, RESPONSE_SIZE);

        parseResponse(response);
    }
}

void ImuDevice::onConnectionEstablished()
{
    ImuData newData = m_currentData;
    newData.isConnected = true;
    updateImuData(newData);
    logMessage("IMU device connection established.");

    // Capture gyro bias first (device must be stationary!)
    logMessage("**IMPORTANT**: Device must be stationary for gyro bias capture!");
    captureGyroBias();
}

void ImuDevice::onConnectionLost()
{
    m_pollTimer->stop();
    m_gyroBiasTimer->stop();
    m_waitingForGyroBias = false;

    ImuData newData = m_currentData;
    newData.isConnected = false;
    updateImuData(newData);
    logMessage("IMU device connection lost.");
}

ImuData ImuDevice::currentData() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

void ImuDevice::setPollInterval(int intervalMs)
{
    m_pollIntervalMs = intervalMs;
    if (m_pollTimer->isActive()) {
        m_pollTimer->setInterval(intervalMs);
    }
}

// =================================
// PROTOCOL IMPLEMENTATION
// =================================

void ImuDevice::captureGyroBias()
{
    logMessage("Capturing gyro bias (10 seconds)...");
    m_waitingForGyroBias = true;

    // Build 0xCD command: [0xCD, 0xC1, 0x29, TimeH, TimeL]
    QByteArray cmd;
    cmd.append(static_cast<char>(CMD_GYRO_BIAS));
    cmd.append(static_cast<char>(0xC1));
    cmd.append(static_cast<char>(0x29));

    quint16 timeMs = 10000;  // 10 seconds
    cmd.append(static_cast<char>((timeMs >> 8) & 0xFF));  // TimeH
    cmd.append(static_cast<char>(timeMs & 0xFF));         // TimeL

    sendData(cmd);
    m_gyroBiasTimer->start();
}

void ImuDevice::onGyroBiasTimeout()
{
    if (m_waitingForGyroBias) {
        logMessage("Gyro bias capture timed out - proceeding anyway");
        m_waitingForGyroBias = false;
        m_pollTimer->start(m_pollIntervalMs);
    }
}

void ImuDevice::onPollTimer()
{
    sendReadRequest();
}

void ImuDevice::sendReadRequest()
{
    if (!isConnected()) {
        return;
    }

    // Send 0xCF command (single byte)
    QByteArray cmd;
    cmd.append(static_cast<char>(CMD_READ_DATA));
    sendData(cmd);
}

void ImuDevice::parseResponse(const QByteArray &response)
{
    if (response.size() < RESPONSE_SIZE) {
        logError("IMU response too short");
        return;
    }

    // Verify command echo (byte 0 should be 0xCF)
    if (static_cast<quint8>(response.at(0)) != CMD_READ_DATA) {
        logError("Invalid IMU response header");
        return;
    }

    ImuData newData = currentData();

    // Parse 0xCF response structure (31 bytes total)
    // Bytes 1-4: Roll (float, big-endian)
    // Bytes 5-8: Pitch (float, big-endian)
    // Bytes 9-12: Yaw (float, big-endian)
    // Bytes 13-16: Angular Rate X (float, big-endian)
    // Bytes 17-20: Angular Rate Y (float, big-endian)
    // Bytes 21-24: Angular Rate Z (float, big-endian)
    // Additional data may follow...

    auto parseFloat = [](const QByteArray &data, int offset) -> float {
        if (offset + 4 > data.size()) return 0.0f;

        quint32 raw = (static_cast<quint8>(data.at(offset)) << 24) |
                      (static_cast<quint8>(data.at(offset + 1)) << 16) |
                      (static_cast<quint8>(data.at(offset + 2)) << 8) |
                      (static_cast<quint8>(data.at(offset + 3)));

        float value;
        memcpy(&value, &raw, sizeof(value));
        return value;
    };

    newData.imuRollDeg = parseFloat(response, 1) * (180.0 / M_PI);
    newData.imuPitchDeg = parseFloat(response, 5) * (180.0 / M_PI);
    newData.imuYawDeg = parseFloat(response, 9) * (180.0 / M_PI);

    newData.angRateX_dps = parseFloat(response, 13) * (180.0 / M_PI);
    newData.angRateY_dps = parseFloat(response, 17) * (180.0 / M_PI);
    newData.angRateZ_dps = parseFloat(response, 21) * (180.0 / M_PI);

    // Additional fields can be parsed if present in response
    // For now, acceleration and temperature are not available in 0xCF response

    updateImuData(newData);
}

void ImuDevice::updateImuData(const ImuData &newData)
{
    bool dataChanged = false;
    {
        QMutexLocker locker(&m_mutex);
        if (newData != m_currentData) {
            m_currentData = newData;
            dataChanged = true;
        }
    }

    if (dataChanged) {
        emit imuDataChanged(m_currentData);
    }
}
