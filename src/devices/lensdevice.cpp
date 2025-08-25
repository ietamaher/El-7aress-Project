#include "lensdevice.h"
#include <QDebug>
#include <QTimer>

LensDevice::LensDevice(QObject *parent)
    : BaseSerialDevice(parent)
{
    // m_currentData is auto-initialized to defaults from LensData struct
}

LensDevice::~LensDevice()
{
    shutdown();
}

void LensDevice::configureSerialPort()
{
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void LensDevice::processIncomingData()
{
    // Read available data
    QByteArray responseData = m_serialPort->readAll();
    while (m_serialPort->waitForReadyRead(10)) {
        responseData += m_serialPort->readAll();
    }

    QString response = QString::fromUtf8(responseData).trimmed();
    emit responseReceived(response);

    // Parse the response to see if it yields new focus/FOV/temperature
    parseLensResponse(response);
}

void LensDevice::onConnectionEstablished()
{
    LensData newData = m_currentData;
    newData.isConnected = true;
    newData.errorCode = 0;
    updateLensData(newData);

    logMessage("Lens device connection established");
}

void LensDevice::onConnectionLost()
{
    LensData newData = m_currentData;
    newData.isConnected = false;
    newData.errorCode = 1; // example error code
    updateLensData(newData);

    logMessage("Lens device connection lost");
}

QString LensDevice::sendCommand(const QString &command)
{
    if (!isConnected()) {
        logError("LensDevice: Serial port not open.");
        return QString();
    }

    // We can store the last command in the data struct for debugging
    LensData newData = m_currentData;
    newData.lastCommand = command;
    updateLensData(newData);

    // Write command (append CR, etc.)
    QString fullCmd = command + "\r";
    QByteArray cmdBytes = fullCmd.toUtf8();

    sendData(cmdBytes);

    // The original code was waiting for response here, but BaseSerialDevice::sendData does not return a value.
    // If a response is expected, it should be handled in processIncomingData.
    // For now, we'll just return an empty string as the original sendCommand did.
    return QString();
}

void LensDevice::parseLensResponse(const QString &rawResponse)
{
    LensData newData = m_currentData;

    // EXAMPLE: pretend the device returns something like "FOCUS=215 TEMP=38.2"
    if (rawResponse.contains("FOCUS=")) {
        int idx = rawResponse.indexOf("FOCUS=");
        QString focusStr = rawResponse.mid(idx + 6).section(" ", 0, 0);
        bool ok = false;
        int focusVal = focusStr.toInt(&ok);
        if (ok) {
            newData.focusPosition = focusVal;
        }
    }
    if (rawResponse.contains("TEMP=")) {
        int idx = rawResponse.indexOf("TEMP=");
        QString tempStr = rawResponse.mid(idx + 5).section(" ", 0, 0);
        bool ok = false;
        double tempVal = tempStr.toDouble(&ok);
        if (ok) {
            newData.lensTemperature = tempVal;
        }
    }

    // You can parse more fields here if the lens returns them.

    updateLensData(newData);
}

void LensDevice::updateLensData(const LensData &newData)
{
    if (newData != m_currentData) {
        m_currentData = newData;
        // Let other layers know
        emit lensDataChanged(m_currentData);
    }
}

// High-level lens control commands
void LensDevice::moveToWFOV()
{
    sendCommand("/MPAv 0, p");
}

void LensDevice::moveToNFOV()
{
    sendCommand("/MPAv 100, p");
}

void LensDevice::moveToIntermediateFOV(int percentage)
{
    QString cmd = QString("/MPAv %1, p").arg(percentage);
    sendCommand(cmd);
}

void LensDevice::moveToFocalLength(int efl)
{
    QString cmd = QString("/MPAv %1, F").arg(efl);
    sendCommand(cmd);
}

void LensDevice::moveToInfinityFocus()
{
    sendCommand("/MPAf 100, u");
}

void LensDevice::moveFocusNear(int amount)
{
    QString cmd = QString("/MPRf %1").arg(-amount);
    sendCommand(cmd);
}

void LensDevice::moveFocusFar(int amount)
{
    QString cmd = QString("/MPRf %1").arg(amount);
    sendCommand(cmd);
}

void LensDevice::getFocusPosition()
{
    sendCommand("/GMSf[2] 1");
}

void LensDevice::getLensTemperature()
{
    sendCommand("/GTV");
}

void LensDevice::resetController()
{
    sendCommand("/RST0 NEOS");
}

void LensDevice::homeAxis(int axis)
{
    QString cmd = QString("/HOM%1").arg(axis);
    sendCommand(cmd);
}

void LensDevice::turnOnTemperatureCompensation()
{
    sendCommand("/MDF[4] 2");
}

void LensDevice::turnOffTemperatureCompensation()
{
    sendCommand("/MDF[4] 0");
}

void LensDevice::turnOnRangeCompensation()
{
    sendCommand("/MDF[5] 2");
}

void LensDevice::turnOffRangeCompensation()
{
    sendCommand("/MDF[5] 0");
}


