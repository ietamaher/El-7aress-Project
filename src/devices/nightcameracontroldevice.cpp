#include "nightcameracontroldevice.h"
#include <QDebug>

NightCameraControlDevice::NightCameraControlDevice(QObject *parent)
    : BaseSerialDevice(parent),
      m_communicationWatchdog(new QTimer(this))
{
    m_currentData.isConnected = false;
    
    // Communication watchdog setup
    m_communicationWatchdog->setSingleShot(true);
    m_communicationWatchdog->setInterval(COMMUNICATION_TIMEOUT_MS);
    connect(m_communicationWatchdog, &QTimer::timeout,
            this, &NightCameraControlDevice::onCommunicationWatchdogTimeout);
}

NightCameraControlDevice::~NightCameraControlDevice() {
    if (m_statusCheckTimer) {
        m_statusCheckTimer->stop();
    }
    if (m_communicationWatchdog) {
        m_communicationWatchdog->stop();
    }
    shutdown();
}

void NightCameraControlDevice::configureSerialPort()
{
    m_serialPort->setBaudRate(QSerialPort::Baud57600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void NightCameraControlDevice::onConnectionEstablished()
{
    // Update data state
    setConnectionState(true);
    
    NightCameraData newData = m_currentData;
    newData.errorState = false;
    updateNightCameraData(newData);

    // Initialize status check timer
    if (!m_statusCheckTimer) {
        m_statusCheckTimer = new QTimer(this);
        connect(m_statusCheckTimer, &QTimer::timeout, this, &NightCameraControlDevice::checkCameraStatus);
    }
    m_statusCheckTimer->start(m_statusCheckIntervalMs);
    
    // Start watchdog
    m_communicationWatchdog->start();

    emit statusChanged(true);
    logMessage("Night camera connection established");
}

void NightCameraControlDevice::onConnectionLost()
{
    // Stop timers
    if (m_statusCheckTimer) {
        m_statusCheckTimer->stop();
    }
    if (m_communicationWatchdog) {
        m_communicationWatchdog->stop();
    }

    // Update data state
    setConnectionState(false);
    
    NightCameraData newData = m_currentData;
    newData.errorState = true;
    updateNightCameraData(newData);

    emit statusChanged(false);
    logMessage("Night camera connection lost");
}

void NightCameraControlDevice::checkCameraStatus()
{
    getCameraStatus();
}

void NightCameraControlDevice::performFFC() {
    NightCameraData newData = m_currentData;
    newData.ffcInProgress = true;
    updateNightCameraData(newData);

    QByteArray command = buildCommand(0x0B, QByteArray::fromHex("0001"));
    sendData(command);
}

void NightCameraControlDevice::setDigitalZoom(quint8 zoomLevel) {
    NightCameraData newData = m_currentData;
    newData.digitalZoomEnabled = (zoomLevel > 0);
    newData.digitalZoomLevel = zoomLevel;
    newData.currentHFOV = (zoomLevel > 0) ? 5.2 : 10.4;
    updateNightCameraData(newData);

    QByteArray zoomArg = (zoomLevel > 0) ? QByteArray::fromHex("0004") : QByteArray::fromHex("0000");
    QByteArray command = buildCommand(0x0F, zoomArg);
    sendData(command);
}

void NightCameraControlDevice::setVideoModeLUT(quint16 mode) {
    NightCameraData newData = m_currentData;
    newData.videoMode = mode;
    updateNightCameraData(newData);
    
    if (mode > 12) {
        mode = 12;
    }
    QByteArray modeArg = QByteArray::fromHex(QByteArray::number(mode, 16).rightJustified(4, '0'));
    QByteArray command = buildCommand(0x10, modeArg);
    sendData(command);
}

void NightCameraControlDevice::getCameraStatus() {
    QByteArray command = buildCommand(0x06, QByteArray::fromHex("0000"));
    sendData(command);
}

void NightCameraControlDevice::updateNightCameraData(const NightCameraData &newData)
{
    if (newData != m_currentData) {
        m_currentData = newData;
        emit nightCameraDataChanged(m_currentData);
    }
}

QByteArray NightCameraControlDevice::buildCommand(quint8 function, const QByteArray &data) {
    QByteArray packet;
    packet.append(static_cast<char>(0x6E)); // Process Code
    packet.append(static_cast<char>(0x00)); // Status
    packet.append(static_cast<char>(0x00)); // Reserved
    packet.append(function); // Function Code

    // Byte Count (2 bytes, big-endian)
    quint16 byteCount = static_cast<quint16>(data.size());
    packet.append(static_cast<quint8>((byteCount >> 8) & 0xFF)); // MSB
    packet.append(static_cast<quint8>(byteCount & 0xFF));        // LSB

    // CRC1 (Header CRC)
    quint16 crc1 = calculateCRC(packet, 6);
    packet.append(static_cast<quint8>((crc1 >> 8) & 0xFF)); // MSB
    packet.append(static_cast<quint8>(crc1 & 0xFF));        // LSB

    // Data bytes
    packet.append(data);

    // CRC2 (Full Packet CRC)
    quint16 crc2 = calculateCRC(packet, packet.size());
    packet.append(static_cast<quint8>((crc2 >> 8) & 0xFF)); // MSB
    packet.append(static_cast<quint8>(crc2 & 0xFF));        // LSB

    return packet;
}

quint16 NightCameraControlDevice::calculateCRC(const QByteArray &data, int length) {
    quint16 crc = 0x0000;
    for (int i = 0; i < length; ++i) {
        crc ^= static_cast<quint8>(data[i]) << 8;
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void NightCameraControlDevice::processIncomingData() {


    // Tau2 packets have a minimum length of 10 bytes: 6-byte header + CRC1 + data + CRC2
    while (m_readBuffer.size() >= 10) {
        // Check the Process Code (0x6E) in the first byte
        if (static_cast<quint8>(m_readBuffer.at(0)) != 0x6E) {
            m_readBuffer.remove(0, 1); // Remove invalid byte and retry
            continue;
        }

        // Extract Byte Count (MSB + LSB)
        if (m_readBuffer.size() < 6) {
            // Wait for the full header to arrive
            break;
        }

        quint16 byteCount = (static_cast<quint8>(m_readBuffer.at(4)) << 8) |
                            static_cast<quint8>(m_readBuffer.at(5));
        int totalPacketSize = 6 + byteCount + 2 + 2; // Header + Data + CRC1 + CRC2

        if (m_readBuffer.size() < totalPacketSize) {
            // Wait for the full packet to arrive
            break;
        }

        // Extract the full packet
        QByteArray packet = m_readBuffer.left(totalPacketSize);
        m_readBuffer.remove(0, totalPacketSize);

        // Verify CRCs
        if (!verifyCRC(packet)) {
            logError("CRC mismatch in incoming packet.");
            continue;
        }

        // Handle the response packet
        handleResponse(packet);
    }
}

void NightCameraControlDevice::handleResponse(const QByteArray &response) {
    if (response.isEmpty()) {
        logError("No response received from Night Camera.");
        return;
    }
    // We received valid data - reset watchdog
    setConnectionState(true);
    resetCommunicationWatchdog();
    // Extract Process Code
    if (static_cast<quint8>(response.at(0)) != 0x6E) {
        logError("Invalid Process Code in response.");
        return;
    }

    // Extract Status Byte
    quint8 statusByte = static_cast<quint8>(response.at(1));
    if (statusByte != 0x00) {
        handleStatusError(statusByte);
        return;
    }

    // Extract Function Code and Handle
    quint8 functionCode = static_cast<quint8>(response.at(3));
    quint16 byteCount = (static_cast<quint8>(response.at(4)) << 8) |
                        static_cast<quint8>(response.at(5));
    QByteArray data = response.mid(8, byteCount);

    // Handle different response types
    switch (functionCode) {
    case 0x06: handleStatusResponse(data); break;
    case 0x0F: handleVideoModeResponse(data); break;
    case 0x10: handleVideoLUTResponse(data); break;
    case 0x0B: handleFFCResponse(data); break;
    default: 
        logError(QString("Unhandled function code: 0x%1").arg(functionCode, 2, 16, QChar('0')).toUpper()); 
        break;
    }
}

bool NightCameraControlDevice::verifyCRC(const QByteArray &packet) {
    if (packet.size() < 10) return false;

    // Extract received CRCs
    quint16 receivedCRC1 = (static_cast<quint8>(packet[6]) << 8) |
                           static_cast<quint8>(packet[7]);
    quint16 receivedCRC2 = (static_cast<quint8>(packet[packet.size() - 2]) << 8) |
                           static_cast<quint8>(packet[packet.size() - 1]);

    // Calculate CRC1 (first 6 bytes) and CRC2 (entire packet minus last 2 bytes)
    quint16 calculatedCRC1 = calculateCRC(packet, 6);
    quint16 calculatedCRC2 = calculateCRC(packet, packet.size() - 2);

    // Validate CRC1 and CRC2
    if (calculatedCRC1 != receivedCRC1) {
        logError(QString("CRC1 mismatch: calculated = %1, received = %2")
                .arg(calculatedCRC1).arg(receivedCRC1));
        return false;
    }

    if (calculatedCRC2 != receivedCRC2) {
        logError(QString("CRC2 mismatch: calculated = %1, received = %2")
                .arg(calculatedCRC2).arg(receivedCRC2));
        return false;
    }

    return true;
}

void NightCameraControlDevice::handleVideoModeResponse(const QByteArray &data) {
    if (data.size() < 2) {
        logError("Invalid Video Mode response.");
        return;
    }

    quint16 mode = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
    logMessage(QString("Video Mode Response: Mode = %1").arg(mode));
    emit responseReceived(data);
}

void NightCameraControlDevice::handleVideoLUTResponse(const QByteArray &data) {
    if (data.size() < 2) {
        logError("Invalid Video LUT response.");
        return;
    }

    quint16 lut = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
    logMessage(QString("Video LUT Response: LUT = %1").arg(lut));
    emit responseReceived(data);
}

void NightCameraControlDevice::handleStatusResponse(const QByteArray &data) {
    if (data.isEmpty()) {
        logError("Invalid STATUS_REQUEST response.");
        return;
    }

    quint8 cameraStatus = static_cast<quint8>(data[0]);
    logMessage(QString("Camera Status Response: Status = %1").arg(cameraStatus));

    NightCameraData newData = m_currentData;
    newData.cameraStatus = cameraStatus;
    updateNightCameraData(newData);
}

void NightCameraControlDevice::handleFFCResponse(const QByteArray &data) {
    logMessage("Flat Field Correction Response received.");
    emit responseReceived(data);

    NightCameraData newData = m_currentData;
    newData.ffcInProgress = false;
    updateNightCameraData(newData);
}

void NightCameraControlDevice::handleStatusError(quint8 statusByte) {
    QString errorMessage;

    switch (statusByte) {
    case 0x01: errorMessage = "Camera is busy processing a command."; break;
    case 0x02: errorMessage = "Camera is not ready."; break;
    case 0x03: errorMessage = "Data out of range error."; break;
    case 0x04: errorMessage = "Checksum error in header or message body."; break;
    case 0x05: errorMessage = "Undefined process code."; break;
    case 0x06: errorMessage = "Undefined function code."; break;
    case 0x07: errorMessage = "Command execution timeout."; break;
    case 0x09: errorMessage = "Byte count mismatch."; break;
    case 0x0A: errorMessage = "Feature not enabled in the current configuration."; break;
    default:
        errorMessage = QString("Unknown status byte: 0x%1")
                           .arg(statusByte, 2, 16, QChar('0')).toUpper();
        break;
    }

    logError(errorMessage);

    NightCameraData newData = m_currentData;
    newData.errorState = true;
    updateNightCameraData(newData);
}

void NightCameraControlDevice::resetCommunicationWatchdog()
{
    m_communicationWatchdog->start();
}

void NightCameraControlDevice::setConnectionState(bool connected)
{
    NightCameraData newData = m_currentData;
    if (newData.isConnected != connected) {
        newData.isConnected = connected;
        updateNightCameraData(newData);

        if (connected) {
            logMessage("Night camera connected");
        } else {
            logMessage("Night camera disconnected");
        }
    }
}

void NightCameraControlDevice::onCommunicationWatchdogTimeout()
{
    logMessage(QString("Communication timeout - no valid camera data received for %1 ms")
               .arg(COMMUNICATION_TIMEOUT_MS));
    setConnectionState(false);
}