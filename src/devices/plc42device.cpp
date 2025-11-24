#include "plc42device.h"
#include <QModbusDataUnit>
#include <QVariant>
#include <QModbusReply>
#include <QSerialPort>
#include <QMutexLocker>
#include <QtMath>

#define NUM_HOLDING_REGS 10

// Constructor: Inherits from ModbusDeviceBase, eliminating duplicate initialization code.
Plc42Device::Plc42Device(const QString &device,
                         int baudRate,
                         int slaveId,
                         QSerialPort::Parity parity,
                         QObject *parent)
    : ModbusDeviceBase(device, baudRate, slaveId, parity, parent)
{
    // Set PLC42-specific polling interval (200ms)
    setPollInterval(50);
    
    // Initialize PLC42 data structure
    m_currentData = Plc42Data();
}

// Destructor: Base class handles cleanup automatically.
Plc42Device::~Plc42Device()
{
    // Base class destructor will handle Modbus device cleanup
}

// Implementation of pure virtual method from ModbusDeviceBase
void Plc42Device::readData()
{
    readDigitalInputs();
    readHoldingData();
}

// Implementation of pure virtual method from ModbusDeviceBase
void Plc42Device::onDataReadComplete()
{
    // Update connection status in the data structure
    Plc42Data newData = m_currentData;
    newData.isConnected = isConnected();
    updatePlc42Data(newData);
}

// Implementation of pure virtual method from ModbusDeviceBase
void Plc42Device::onWriteComplete()
{
    // Handle any post-write operations if needed
    // Currently no specific action required for PLC42
}

// Reads digital inputs (Discrete Inputs) from the PLC.
void Plc42Device::readDigitalInputs()
{
    if (!isConnected())
        return;

    QModbusDataUnit readUnit(QModbusDataUnit::DiscreteInputs,
                             0,
                             7);

    if (auto *reply = sendReadRequest(readUnit)) {
       // connect(reply, &QModbusReply::finished,
         //       this, &Plc42Device::onDigitalInputsReadReady);
        connectReplyFinished(reply, [this](QModbusReply* r){ onDigitalInputsReadReady(r); });
    } else {
        logError("Read digital inputs error: Failed to send request");
        emit errorOccurred("Read digital inputs error: Failed to send request");
    }
}

// Handles the response for digital input read requests.
void Plc42Device::onDigitalInputsReadReady(QModbusReply *reply)
{
    if (!reply)
        return;

    stopTimeoutTimer();

    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        Plc42Data newData = m_currentData;

        // Update individual digital input fields based on their respective indices.
        if (unit.valueCount() >= 8) {
            newData.stationUpperSensor  = (unit.value(0) != 0);
            newData.stationLowerSensor  = (unit.value(1) != 0);
            newData.emergencyStopActive = (unit.value(2) != 0);
            newData.ammunitionLevel     = (unit.value(3) != 0);
            newData.hatchState       = (unit.value(4) != 0);
            newData.freeGimbalState       = (unit.value(5) != 0);
            newData.stationInput3       = (unit.value(6) != 0);
            newData.solenoidActive      = (unit.value(7) != 0);
        } else {
            logError("Insufficient digital input values.");
        }

        newData.isConnected = isConnected();
        updatePlc42Data(newData);
    } else {
        logError("Digital inputs read error: " + reply->errorString());
        emit errorOccurred(reply->errorString());
        
        // Update connection status
        Plc42Data newData = m_currentData;
        newData.isConnected = false;
        updatePlc42Data(newData);
    }
    reply->deleteLater();
}

// Reads holding registers from the PLC.
void Plc42Device::readHoldingData()
{
    if (!isConnected())
        return;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters,
                             0,
                             10);

    if (auto *reply = sendReadRequest(readUnit)) {
       // connect(reply, &QModbusReply::finished,
              //  this, &Plc42Device::onHoldingDataReadReady);
         connectReplyFinished(reply, [this](QModbusReply* r){ onHoldingDataReadReady(r); });   
    } else {
        logError("Read holding registers error: Failed to send request");
        emit errorOccurred("Read holding registers error: Failed to send request");
    }
}

// Handles the response for holding register read requests.
void Plc42Device::onHoldingDataReadReady(QModbusReply *reply)
{
    if (!reply)
        return;

    stopTimeoutTimer();

    if (reply->error() == QModbusDevice::NoError) {
        QModbusDataUnit unit = reply->result();
        Plc42Data newData = m_currentData;

        if (unit.valueCount() >= 7) {
            newData.solenoidMode       = unit.value(0);
            newData.gimbalOpMode       = unit.value(1);
            
            // Combine two 16-bit registers into a 32-bit value for azimuth speed.
            uint16_t azLow  = unit.value(2);
            uint16_t azHigh = unit.value(3);
            newData.azimuthSpeed = (static_cast<uint32_t>(azHigh) << 16) | azLow;

            // Combine two 16-bit registers into a 32-bit value for elevation speed.
            uint16_t elLow  = unit.value(4);
            uint16_t elHigh = unit.value(5);
            newData.elevationSpeed = (static_cast<uint32_t>(elHigh) << 16) | elLow;

            newData.azimuthDirection   = unit.value(6);
            
            // Additional registers if available
            if (unit.valueCount() >= 10) {
                newData.elevationDirection = unit.value(7);
                newData.solenoidState      = unit.value(8);
                newData.resetAlarm         = unit.value(9);
            }
        } else {
            logError("Insufficient holding register values.");
        }

        newData.isConnected = isConnected();
        updatePlc42Data(newData);
    } else {
        logError("Holding data read error: " + reply->errorString());
        emit errorOccurred(reply->errorString());
        
        // Update connection status
        Plc42Data newData = m_currentData;
        newData.isConnected = false;
        updatePlc42Data(newData);
    }
    reply->deleteLater();
}

// Writes the cached holding register values to the PLC.
void Plc42Device::writeRegisterData()
{
    if (!isConnected())
        return;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, 
                              0, 
                              NUM_HOLDING_REGS);
    
    writeUnit.setValue(0, m_currentData.solenoidMode);
    writeUnit.setValue(1, m_currentData.gimbalOpMode);

    // Split 32-bit azimuth speed into two 16-bit registers.
    uint16_t azLow  = static_cast<uint16_t>(m_currentData.azimuthSpeed & 0xFFFF);
    uint16_t azHigh = static_cast<uint16_t>((m_currentData.azimuthSpeed >> 16) & 0xFFFF);
    writeUnit.setValue(2, azLow);
    writeUnit.setValue(3, azHigh);

    // Split 32-bit elevation speed into two 16-bit registers.
    uint16_t elLow  = static_cast<uint16_t>(m_currentData.elevationSpeed & 0xFFFF);
    uint16_t elHigh = static_cast<uint16_t>((m_currentData.elevationSpeed >> 16) & 0xFFFF);
    writeUnit.setValue(4, elLow);
    writeUnit.setValue(5, elHigh);

    writeUnit.setValue(6, m_currentData.azimuthDirection);
    writeUnit.setValue(7, m_currentData.elevationDirection);
    writeUnit.setValue(8, m_currentData.solenoidState);
    writeUnit.setValue(9, m_currentData.resetAlarm);

    if (auto *reply = sendWriteRequest(writeUnit)) {
        //connect(reply, &QModbusReply::finished, this, &Plc42Device::onWriteReady);
        connectReplyFinished(reply, [this](QModbusReply* r){ onWriteReady(r); });

    } else {
        logError("Error writing holding registers: Failed to send request");
        emit errorOccurred("Error writing holding registers: Failed to send request");
    }
}

// Control methods - these update local data and write to device
void Plc42Device::setSolenoidMode(uint16_t mode)
{
    Plc42Data newData = m_currentData;
    newData.solenoidMode = mode;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setGimbalMotionMode(uint16_t mode)
{
    Plc42Data newData = m_currentData;
    newData.gimbalOpMode = mode;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setAzimuthSpeedHolding(uint32_t speed)
{
    Plc42Data newData = m_currentData;
    newData.azimuthSpeed = speed;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setElevationSpeedHolding(uint32_t speed)
{
    Plc42Data newData = m_currentData;
    newData.elevationSpeed = speed;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setAzimuthDirection(uint16_t direction)
{
    Plc42Data newData = m_currentData;
    newData.azimuthDirection = direction;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setElevationDirection(uint16_t direction)
{
    Plc42Data newData = m_currentData;
    newData.elevationDirection = direction;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setSolenoidState(uint16_t state)
{
    Plc42Data newData = m_currentData;
    newData.solenoidState = state;
    updatePlc42Data(newData);
    writeRegisterData();
}

void Plc42Device::setResetAlarm(uint16_t alarm)
{
    Plc42Data newData = m_currentData;
    newData.resetAlarm = alarm;
    updatePlc42Data(newData);
    writeRegisterData();
}

// Handles the response for write requests.
void Plc42Device::onWriteReady(QModbusReply *reply)
{
    if (reply) {
        if (reply->error() != QModbusDevice::NoError) {
            logError("Write response error: " + reply->errorString());
            emit errorOccurred(reply->errorString());
        } else {
            // Notify base class that write completed successfully
            onWriteComplete();
        }
        reply->deleteLater();
    }
}

// Updates the internal PLC42 data and emits a signal if data has changed.
void Plc42Device::updatePlc42Data(const Plc42Data &newData)
{
    if (newData != m_currentData) {
        m_currentData = newData;
        emit plc42DataChanged(m_currentData);
    }
}
