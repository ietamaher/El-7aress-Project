#include "daycameracontroldevice.h"
#include <QDebug>
#include <QTimer>

DayCameraControlDevice::DayCameraControlDevice(QObject *parent)
    : BaseSerialDevice(parent)
{
    // Initialize camera-specific data
    m_currentData.isConnected = false;
}

void DayCameraControlDevice::configureSerialPort()
{
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void DayCameraControlDevice::processIncomingData()
{
    // Process Pelco-D response frames (7 bytes each)
    while (m_readBuffer.size() >= 7) {
        // Verify SYNC byte
        if (static_cast<quint8>(m_readBuffer.at(0)) != 0xFF) {
            logError(QString("Invalid SYNC byte: 0x%1")
                    .arg(static_cast<quint8>(m_readBuffer.at(0)), 2, 16, QChar('0')));
            m_readBuffer.remove(0, 1);
            continue;
        }

        // Extract 7-byte frame
        QByteArray frame = m_readBuffer.left(7);
        m_readBuffer.remove(0, 7);

        // Parse frame fields
        quint8 addr = static_cast<quint8>(frame.at(1));
        quint8 resp1 = static_cast<quint8>(frame.at(2));
        quint8 resp2 = static_cast<quint8>(frame.at(3));
        quint8 data1 = static_cast<quint8>(frame.at(4));
        quint8 data2 = static_cast<quint8>(frame.at(5));
        quint8 recvChecksum = static_cast<quint8>(frame.at(6));

        // Verify checksum
        quint8 calcChecksum = (addr + resp1 + resp2 + data1 + data2) & 0xFF;
        if (recvChecksum != calcChecksum) {
            logError(QString("Checksum mismatch: received 0x%1, calculated 0x%2")
                    .arg(recvChecksum, 2, 16, QChar('0'))
                    .arg(calcChecksum, 2, 16, QChar('0')));
            continue;
        }

        // Process valid frame
        DayCameraData newData = m_currentData;
        
        if (resp2 == 0xA7) {
            // Zoom position response
            quint16 zoomPos = (data1 << 8) | data2;
            newData.zoomPosition = zoomPos;
            newData.currentHFOV = computeHFOVfromZoom(zoomPos);
        } else if (resp2 == 0x63) {
            // Focus position response
            quint16 focusPos = (data1 << 8) | data2;
            newData.focusPosition = focusPos;
        }
        
        updateDayCameraData(newData);
        m_lastSentCommand.clear();
    }
}

void DayCameraControlDevice::onConnectionEstablished()
{
    DayCameraData newData = m_currentData;
    newData.isConnected = true;
    newData.errorState = false;
    updateDayCameraData(newData);
}

void DayCameraControlDevice::onConnectionLost()
{
    DayCameraData newData = m_currentData;
    newData.isConnected = false;
    newData.errorState = true;
    updateDayCameraData(newData);
}

DayCameraData DayCameraControlDevice::currentData() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

QByteArray DayCameraControlDevice::buildPelcoD(quint8 address, quint8 cmd1, quint8 cmd2,
                                              quint8 data1, quint8 data2) const
{
    QByteArray packet;
    packet.append((char)0xFF);
    packet.append((char)address);
    packet.append((char)cmd1);
    packet.append((char)cmd2);
    packet.append((char)data1);
    packet.append((char)data2);

    quint8 checksum = (address + cmd1 + cmd2 + data1 + data2) & 0xFF;
    packet.append((char)checksum);
    return packet;
}

void DayCameraControlDevice::sendPelcoDCommand(quint8 cmd1, quint8 cmd2, quint8 data1, quint8 data2)
{
    if (!isConnected()) {
        logError("Cannot send camera command: not connected");
        return;
    }
    
    QByteArray command = buildPelcoD(CAMERA_ADDRESS, cmd1, cmd2, data1, data2);
    m_lastSentCommand = command;
    sendData(command);
}

void DayCameraControlDevice::zoomIn()
{
    DayCameraData newData = m_currentData;
    newData.zoomMovingIn = true;
    newData.zoomMovingOut = false;
    updateDayCameraData(newData);

    sendPelcoDCommand(0x00, 0x20); // Zoom Tele
}

void DayCameraControlDevice::zoomOut()
{
    DayCameraData newData = m_currentData;
    newData.zoomMovingOut = true;
    newData.zoomMovingIn = false;
    updateDayCameraData(newData);

    sendPelcoDCommand(0x00, 0x40); // Zoom Wide
}

void DayCameraControlDevice::zoomStop()
{
    DayCameraData newData = m_currentData;
    newData.zoomMovingIn = false;
    newData.zoomMovingOut = false;
    updateDayCameraData(newData);

    sendPelcoDCommand(0x00, 0x00); // Stop
}

void DayCameraControlDevice::setZoomPosition(quint16 position)
{
    DayCameraData newData = m_currentData;
    newData.zoomPosition = position;
    newData.zoomMovingIn = false;
    newData.zoomMovingOut = false;
    updateDayCameraData(newData);

    quint8 high = (position >> 8) & 0xFF;
    quint8 low = position & 0xFF;
    sendPelcoDCommand(0x00, 0xA7, high, low);
}

void DayCameraControlDevice::focusNear()
{
    sendPelcoDCommand(0x01, 0x00); // Focus Near
}

void DayCameraControlDevice::focusFar()
{
    sendPelcoDCommand(0x00, 0x02); // Focus Far
}

void DayCameraControlDevice::focusStop()
{
    sendPelcoDCommand(0x00, 0x00); // Stop
}

void DayCameraControlDevice::setFocusAuto(bool enabled)
{
    DayCameraData newData = m_currentData;
    newData.autofocusEnabled = enabled;
    updateDayCameraData(newData);

    if (enabled) {
        sendPelcoDCommand(0x01, 0x63); // Enable autofocus (vendor-specific)
    } else {
        sendPelcoDCommand(0x01, 0x64); // Disable autofocus (vendor-specific)
    }
}

void DayCameraControlDevice::setFocusPosition(quint16 position)
{
    DayCameraData newData = m_currentData;
    newData.focusPosition = position;
    updateDayCameraData(newData);

    quint8 high = (position >> 8) & 0xFF;
    quint8 low = position & 0xFF;
    sendPelcoDCommand(0x00, 0x63, high, low);
}

void DayCameraControlDevice::getCameraStatus()
{
    sendPelcoDCommand(0x00, 0xA7); // Request zoom position
}

double DayCameraControlDevice::computeHFOVfromZoom(quint16 zoomPos) const
{
    const quint16 maxZoom = 0x4000;
    double fraction = qMin((double)zoomPos / maxZoom, 1.0);

    double wideHFOV = 63.7;
    double teleHFOV = 2.3;
    return wideHFOV - (wideHFOV - teleHFOV) * fraction;
}

void DayCameraControlDevice::updateDayCameraData(const DayCameraData &newData)
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
        emit dayCameraDataChanged(m_currentData);
    }
}
