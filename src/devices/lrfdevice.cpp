// --- lrfdevice.cpp ---

#include "lrfdevice.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMutexLocker>

LRFDevice::LRFDevice(QObject *parent)
    : BaseSerialDevice(parent),
      m_statusTimer(new QTimer(this))
{
    // A periodic self-check is good practice
    connect(m_statusTimer, &QTimer::timeout, this, &LRFDevice::sendSelfCheck);
}

void LRFDevice::configureSerialPort()
{
    // Per Jioptics documentation: 115200 bps, 8-N-1
    m_serialPort->setBaudRate(QSerialPort::Baud115200);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void LRFDevice::processIncomingData()
{
    // Protocol uses fixed 9-byte packets.
    while (m_readBuffer.size() >= PACKET_SIZE)
    {
        // Find the start of a packet
        int headerIndex = m_readBuffer.indexOf(FRAME_HEADER);
        if (headerIndex == -1) {
             // No header found, clear buffer to prevent overflow with junk data
            m_readBuffer.clear();
            return;
        }

        // Discard any data before the header
        if (headerIndex > 0) {
            m_readBuffer.remove(0, headerIndex);
        }

        // We might not have a full packet yet after removing junk
        if (m_readBuffer.size() < PACKET_SIZE) {
            return;
        }

        // Check for correct Device Code
        if ((quint8)m_readBuffer.at(1) != DeviceCode::LRF) {
            // Invalid packet, remove the false header and search again
            m_readBuffer.remove(0, 1);
            continue;
        }

        // We have a potential 9-byte packet
        QByteArray packet = m_readBuffer.left(PACKET_SIZE);
        m_readBuffer.remove(0, PACKET_SIZE);

        if (verifyChecksum(packet)) {
            handleResponse(packet);
        } else {
            logError("Checksum mismatch in LRF packet");
        }
    }
}

void LRFDevice::onConnectionEstablished()
{
    LrfData newData = m_currentData;
    newData.isConnected = true;
    updateLrfData(newData);
    logMessage("LRF device connection established.");
    // Start periodic status checks once connected
    m_statusTimer->start(30000); // every 30 seconds
}

void LRFDevice::onConnectionLost()
{
    m_statusTimer->stop();
    LrfData newData = m_currentData;
    newData.isConnected = false;
    newData.isFault = true; // Assume fault on connection loss
    updateLrfData(newData);
    logMessage("LRF device connection lost.");
}

LrfData LRFDevice::currentData() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

// =================================
// PUBLIC COMMAND SENDING INTERFACE
// =================================

void LRFDevice::sendSelfCheck() { sendCommand(CommandCode::SelfTest); }
void LRFDevice::sendSingleRanging() { sendCommand(CommandCode::SingleRanging); }
void LRFDevice::sendContinuousRanging1Hz() { sendCommand(CommandCode::ContinuousRanging1Hz); }
void LRFDevice::sendContinuousRanging5Hz() { sendCommand(CommandCode::ContinuousRanging5Hz); }
void LRFDevice::sendContinuousRanging10Hz() { sendCommand(CommandCode::ContinuousRanging10Hz); }
void LRFDevice::stopRanging() { sendCommand(CommandCode::LaserStop); }
void LRFDevice::queryAccumulatedLaserCount() { sendCommand(CommandCode::PulseCountReport); }
void LRFDevice::queryProductInfo() { sendCommand(CommandCode::ProductIdentificationReport); }
void LRFDevice::queryTemperature() { sendCommand(CommandCode::TemperatureReading); }

// =================================
// INTERNAL PROTOCOL HELPERS
// =================================

void LRFDevice::sendCommand(quint8 commandCode, const QByteArray& params)
{
    if (!isConnected()) {
        logError("Cannot send command: LRF not connected.");
        return;
    }
    QByteArray fullCommand = buildCommand(commandCode, params);
    sendData(fullCommand);
}

QByteArray LRFDevice::buildCommand(quint8 commandCode, const QByteArray& params) const
{
    QByteArray packet;
    packet.reserve(PACKET_SIZE);

    // 1. Frame Header
    packet.append(FRAME_HEADER);
    // 2. Device Code
    packet.append(DeviceCode::LRF);

    // 3-8. Message Body (6 bytes)
    QByteArray body;
    body.append(commandCode);
    body.append(params);
    packet.append(body);

    // 9. Checksum
    quint8 checksum = calculateChecksum(body);
    packet.append(checksum);

    return packet;
}

quint8 LRFDevice::calculateChecksum(const QByteArray &body) const
{
    // Checksum is sum of the 6-byte body (bytes 3-8 of the packet)
    quint8 sum = 0;
    for (char byte : body) {
        sum += static_cast<quint8>(byte);
    }
    return sum;
}

bool LRFDevice::verifyChecksum(const QByteArray &packet) const
{
    if (packet.size() != PACKET_SIZE) return false;

    QByteArray body = packet.mid(2, 6);
    quint8 receivedChecksum = static_cast<quint8>(packet.at(8));
    quint8 calculatedChecksum = calculateChecksum(body);

    return (receivedChecksum == calculatedChecksum);
}

void LRFDevice::handleResponse(const QByteArray &response)
{
    quint8 responseCode = static_cast<quint8>(response.at(2)); // Byte 3 is the command code

    switch (responseCode) {
    case 0x00:   // treat as self-test response as wel
    case ResponseCode::SelfTest:
        handleSelfCheckResponse(response);
        break;
    case ResponseCode::SingleRanging:
    case ResponseCode::ContinuousRanging1Hz:
    case ResponseCode::ContinuousRanging5Hz:
    case ResponseCode::ContinuousRanging10Hz:
        handleRangingResponse(response);
        break;
    case ResponseCode::LaserStop:
        handleStopRangingResponse(response);
        break;
    case ResponseCode::PulseCountReport:
        handlePulseCountResponse(response);
        break;
    case ResponseCode::ProductIdentificationReport:
        handleProductInfoResponse(response);
        break;
    case ResponseCode::TemperatureReading:
        handleTemperatureResponse(response);
        break;
    default:
        logError(QString("Unknown LRF response code: 0x%1")
                     .arg(responseCode, 2, 16, QChar('0')).toUpper());
        break;
    }
}

// =================================
// RESPONSE HANDLERS
// =================================

void LRFDevice::handleSelfCheckResponse(const QByteArray &response)
{
    LrfData newData = currentData();
    quint8 status1 = static_cast<quint8>(response.at(3)); // Byte 4: Status1
    quint8 status0 = static_cast<quint8>(response.at(4)); // Byte 5: Status0

    newData.rawStatusByte = status0;
    newData.isFault = (status1 == 0x01);
    newData.noEcho = (status0 & 0x08) >> 3; // Bit 3
    newData.laserNotOut = (status0 & 0x10) >> 4; // Bit 4
    newData.isOverTemperature = (status0 & 0x20) >> 5; // Bit 5

    updateLrfData(newData);
    logMessage(QString("Self-check response received. Fault: %1").arg(newData.isFault ? "Yes" : "No"));
}

void LRFDevice::handleRangingResponse(const QByteArray &response)
{
    LrfData newData = currentData();
    quint8 status0 = static_cast<quint8>(response.at(3)); // Byte 4: Status0

    newData.rawStatusByte = status0;
    newData.isFault = (status0 == 0x01); // 0x00: normal, 0x01: fault
    newData.noEcho = (status0 & 0x08) >> 3;
    newData.laserNotOut = (status0 & 0x10) >> 4;
    newData.isOverTemperature = (status0 & 0x20) >> 5;

    quint8 dist_h = static_cast<quint8>(response.at(5)); // Byte 6: DIS_H
    quint8 dist_l = static_cast<quint8>(response.at(6)); // Byte 7: DIS_L
    newData.lastDistance = (dist_h << 8) | dist_l;
    
    // Per doc: when measurement is invalid, value is 0.
    newData.isLastRangingValid = (newData.lastDistance > 0);

    newData.pulseCount = static_cast<quint8>(response.at(7)); // Byte 8: pulse count

    updateLrfData(newData);
}

void LRFDevice::handlePulseCountResponse(const QByteArray &response)
{
    LrfData newData = currentData();
    // Doc 6.2.5 has a typo. It should be PNUM_H/L, not DIS_H/L.
    // Byte 6: PNUM_L, Byte 7: PNUM_H
    quint8 pnum_l = static_cast<quint8>(response.at(5));
    quint8 pnum_h = static_cast<quint8>(response.at(6));
    
    quint16 pulse_base = (pnum_h << 8) | pnum_l;
    
    // Per Jioptics doc (Remark 3): actual count = count * 100
    // But other doc says `(DIS_H*256+DIS_L)*100`. Assuming it's PNUM.
    // Let's store the full value.
    newData.laserCount = static_cast<quint32>(pulse_base) * 100;
    
    updateLrfData(newData);
    logMessage(QString("Laser pulse count: %1").arg(newData.laserCount));
}

void LRFDevice::handleProductInfoResponse(const QByteArray &response)
{
    quint8 productId = static_cast<quint8>(response.at(3)); // Byte 4: ID
    quint8 versionByte = static_cast<quint8>(response.at(4)); // Byte 5: Version

    int mainVersion = (versionByte & 0xF0) >> 4;
    int subVersion = (versionByte & 0x0F);
    QString versionString = QString("%1.%2").arg(mainVersion).arg(subVersion);
    
    logMessage(QString("Product Info - ID: 0x%1, Version: %2")
        .arg(productId, 2, 16, QChar('0')).toUpper()
        .arg(versionString));

    emit productInfoReceived(productId, versionString);
}

void LRFDevice::handleTemperatureResponse(const QByteArray &response)
{
    LrfData newData = currentData();
    quint8 tempByte = static_cast<quint8>(response.at(4)); // Byte 5: Temp

    // Temp is signed, 2's complement is not specified, assuming sign-magnitude
    // Bit7: Sign bit, Bit6-0: Data bits
    qint8 tempValue = tempByte & 0x7F;
    if (tempByte & 0x80) { // Check sign bit
        tempValue = -tempValue;
    }
    
    newData.temperature = tempValue;
    newData.isTempValid = true;

    updateLrfData(newData);
    logMessage(QString("Temperature reading: %1 C").arg(newData.temperature));
}

void LRFDevice::handleStopRangingResponse(const QByteArray &response)
{
    // The response is just an acknowledgement. No data to parse.
    logMessage("Stop ranging acknowledged by LRF.");
}

void LRFDevice::updateLrfData(const LrfData &newData)
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
        emit lrfDataChanged(m_currentData);
    }
}
