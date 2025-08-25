#include "plc21device.h"
#include <QSerialPort>
#include <QVariant>
#include <QDebug>
#include <QMutexLocker>
#include <QtMath>

// Constructor: Initializes using base class constructor
Plc21Device::Plc21Device(const QString &device,
                         int baudRate,
                         int slaveId,
                         QSerialPort::Parity parity,
                         QObject *parent)
    : ModbusDeviceBase(device, baudRate, slaveId, parity, parent)
{
    // Set PLC-specific poll interval (50ms instead of default 100ms)
    setPollInterval(50);

    // Connect to base class signals
    connect(this, &ModbusDeviceBase::connectionStateChanged,
            this, &Plc21Device::onConnectionStateChanged);
}

// Destructor: Base class handles disconnection
Plc21Device::~Plc21Device()
{
    // No specific cleanup needed, base class destructor will handle it
}

// Override the pure virtual readData method from base class
void Plc21Device::readData()
{
    if (!isConnected())
        return;

    // Read Digital Inputs
    readDigitalInputs();

    // Read Analog Inputs
    readAnalogInputs();
}

// Override the virtual onDataReadComplete method from base class
void Plc21Device::onDataReadComplete()
{
    // This is called when connection state changes
    // Update panel data connection status
    Plc21PanelData newData = m_currentPanelData;
    newData.isConnected = isConnected();
    updatePanelData(newData);
}

void Plc21Device::onWriteComplete()
{
    // This method can be used to handle post-write operations if needed
    // Currently, it does nothing but can be extended in the future
}

// Read digital inputs using base class method
void Plc21Device::readDigitalInputs()
{
    QModbusDataUnit readUnit(QModbusDataUnit::DiscreteInputs,
                             DIGITAL_INPUTS_START_ADDRESS,
                             DIGITAL_INPUTS_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        //connect(reply, &QModbusReply::finished,  this, &Plc21Device::onDigitalInputsReadReady);
        connectReplyFinished(reply, [this](QModbusReply* r){ onDigitalInputsReadReady(r); });

    }
}

// Read analog inputs using base class method
void Plc21Device::readAnalogInputs()
{
    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters,
                             ANALOG_INPUTS_START_ADDRESS,
                             ANALOG_INPUTS_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        //connect(reply, &QModbusReply::finished,  this, &Plc21Device::onAnalogInputsReadReady);
        connectReplyFinished(reply, [this](QModbusReply* r){ onAnalogInputsReadReady(r); });
    }
}

// Handle connection state changes from base class
void Plc21Device::onConnectionStateChanged(bool connected)
{
    if (connected) {
        logMessage("PLC Modbus connection established.");
        resetReconnectionAttempts();
    } else {
        logMessage("PLC Modbus device disconnected.");
    }

    // Update panel data
    Plc21PanelData newData = m_currentPanelData;
    newData.isConnected = connected;
    updatePanelData(newData);
}

// Handles the response for digital input read requests
void Plc21Device::onDigitalInputsReadReady(QModbusReply *reply)
{
    if (!reply)
        return;

    stopTimeoutTimer(); // Stop timeout timer from base class

    QMutexLocker locker(&m_mutex);
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        QVector<bool> rawDigital;
        for (int i = 0; i < unit.valueCount(); ++i)
            rawDigital.append(unit.value(i) != 0);
        m_digitalInputs = rawDigital;

        Plc21PanelData newData = m_currentPanelData;
        // Update individual digital input fields based on their respective indices
        if (unit.valueCount() > 0) {
            newData.authorizeSw = (unit.value(0) != 0);
        }
        if (unit.valueCount() > 1) {
            newData.menuValSw = (unit.value(1) != 0);
        }
        if (unit.valueCount() > 2) {
            newData.menuDownSW = (unit.value(2) != 0);
        }
        if (unit.valueCount() > 3) {
            newData.menuUpSW = (unit.value(3) != 0);
        }
        if (unit.valueCount() > 4) {
            newData.switchCameraSW = (unit.value(4) != 0);
        }
        if (unit.valueCount() > 5) {
            newData.enableStabilizationSW = (unit.value(5) != 0);
        }
        if (unit.valueCount() > 6) {
            newData.homePositionSW = (unit.value(6) != 0);
        }
        if (unit.valueCount() > 8) {
            newData.loadAmmunitionSW = (unit.value(8) != 0);
        }
        if (unit.valueCount() > 9) {
            newData.armGunSW = (unit.value(9) != 0);
        }
        if (unit.valueCount() > 10) {
            newData.enableStationSW = (unit.value(10) != 0);
        }

        newData.isConnected = true;
        updatePanelData(newData);
    } else {
        logError(QString("Digital inputs response error: %1").arg(reply->errorString()));

        // Mark as disconnected on error
        Plc21PanelData newData = m_currentPanelData;
        newData.isConnected = false;
        updatePanelData(newData);
    }

    //reply->deleteLater();
}

// Handles the response for analog input read requests
void Plc21Device::onAnalogInputsReadReady(QModbusReply *reply)
{
    if (!reply)
        return;

    stopTimeoutTimer(); // Stop timeout timer from base class

    QMutexLocker locker(&m_mutex);
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        QVector<uint16_t> rawAnalog;
        for (int i = 0; i < unit.valueCount(); ++i)
            rawAnalog.append(unit.value(i));
        m_analogInputs = rawAnalog;

        Plc21PanelData newData = m_currentPanelData;
        // Update individual analog input fields based on their respective indices
        if (unit.valueCount() > 0) {
            newData.fireMode = unit.value(0);
        }
        if (unit.valueCount() > 1) {
            newData.speedSW = unit.value(1);
        }
        if (unit.valueCount() > 2) {
            newData.panelTemperature = unit.value(2);
        }

        newData.isConnected = true;
        updatePanelData(newData);
    } else {
        logError(QString("Analog inputs response error: %1").arg(reply->errorString()));

        // Mark as disconnected on error
        Plc21PanelData newData = m_currentPanelData;
        newData.isConnected = false;
        updatePanelData(newData);
    }

    reply->deleteLater();
}

// Writes the cached digital output values to the PLC
void Plc21Device::writeData()
{
    if (!isConnected())
        return;

    QMutexLocker locker(&m_mutex);
    QVector<bool> coilValues;
    // Ensure we don't write more coils than defined by DIGITAL_OUTPUTS_COUNT
    for (int i = 0; i < m_digitalOutputs.size() && i < DIGITAL_OUTPUTS_COUNT; ++i) {
        coilValues.append(m_digitalOutputs.at(i));
    }

    QModbusDataUnit writeUnit(QModbusDataUnit::Coils,
                              DIGITAL_OUTPUTS_START_ADDRESS,
                              coilValues.size());
    for (int i = 0; i < coilValues.size(); ++i) {
        writeUnit.setValue(i, coilValues.at(i));
    }

    if (auto *reply = sendWriteRequest(writeUnit)) {
        //connect(reply, &QModbusReply::finished,  this, &Plc21Device::onWriteReady);
        connectReplyFinished(reply, [this](QModbusReply* r){ onWriteReady(r); });
    }
}

// Handles the response for write requests
void Plc21Device::onWriteReady(QModbusReply *reply)
{
    if (!reply)
        return;
 

    if (reply->error() != QModbusDevice::NoError) {
        logError(QString("Write response error: %1").arg(reply->errorString()));
    } else {
        logMessage("Write to PLC completed successfully.");
    }

    reply->deleteLater();
}

// Returns the current digital input values
QVector<bool> Plc21Device::digitalInputs() const
{
    QMutexLocker locker(&m_mutex);
    return m_digitalInputs;
}

// Returns the current analog input values
QVector<uint16_t> Plc21Device::analogInputs() const
{
    QMutexLocker locker(&m_mutex);
    return m_analogInputs;
}

// Sets the digital output values and triggers a write operation
void Plc21Device::setDigitalOutputs(const QVector<bool> &outputs)
{
    {
        QMutexLocker locker(&m_mutex);
        m_digitalOutputs = outputs;
    }
    writeData(); // Trigger write immediately
}

// Updates the internal panel data and emits a signal if data has changed
void Plc21Device::updatePanelData(const Plc21PanelData &newData)
{
    if (newData != m_currentPanelData) {
        m_currentPanelData = newData;
        emit panelDataChanged(m_currentPanelData);
    }
}
