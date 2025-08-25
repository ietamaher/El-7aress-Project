#include "gyrodevice.h"
#include <QModbusDataUnit>
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker>

ImuDevice::ImuDevice(const QString &device, int baudRate, int slaveId, QObject *parent)
    // The SST810 protocol specifies NO parity. This is crucial.
    : ModbusDeviceBase(device, baudRate, slaveId, QSerialPort::NoParity, parent)
{
    // Connect the base class's connection signal to our handler
    connect(this, &ModbusDeviceBase::connectionStateChanged,
            this, &ImuDevice::handleConnectionChange);
}

ImuDevice::~ImuDevice()
{
    disconnectDevice();
}

GyroData ImuDevice::getCurrentData() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

void ImuDevice::handleConnectionChange(bool connected)
{
    GyroData newData = getCurrentData();
    newData.isConnected = connected;
    updateGyroData(newData);
}

// This method is called periodically by the base class's poll timer.
void ImuDevice::readData()
{
    // Define the Modbus read request.
    // Function code 0x04 for SST810 is QModbusDataUnit::InputRegisters.
    QModbusDataUnit readUnit(QModbusDataUnit::InputRegisters,
                             ALL_DATA_START_ADDRESS,
                             ALL_DATA_REGISTER_COUNT);

    // Send the request and connect the reply's finished signal to our handler.
    if (auto *reply = sendReadRequest(readUnit)) {
        connectReplyFinished(reply, [this](QModbusReply* r) {
            onReadReady(r);
        });
    }
}

void ImuDevice::onReadReady(QModbusReply *reply)
{
    // Stop the timeout timer since we received a response.
    stopTimeoutTimer();

    if (!reply) {
        logError("Read reply is null.");
        return;
    }

    if (reply->error() != QModbusDevice::NoError) {
        logError(QString("Gyro Read Error: %1 (Modbus exception: 0x%2)")
                     .arg(reply->errorString())
                     .arg(reply->rawResult().exceptionCode(), 2, 16, QChar('0')));
    } else {
        // If successful, parse the data.
        parseModbusResponse(reply->result());
    }

    // This cycle is complete.
    onDataReadComplete();
}

void ImuDevice::parseModbusResponse(const QModbusDataUnit &dataUnit)
{
    // Ensure we received the expected number of registers.
    if (dataUnit.valueCount() != ALL_DATA_REGISTER_COUNT) {
        logError(QString("Incorrect number of registers received. Expected %1, got %2.")
                     .arg(ALL_DATA_REGISTER_COUNT)
                     .arg(dataUnit.valueCount()));
        return;
    }

    GyroData newData = getCurrentData();

    // Helper lambda to parse a 4-byte Big Endian float from two 16-bit registers.
    auto parseFloat = [&](int index) -> float {
        quint16 high = dataUnit.value(index);
        quint16 low = dataUnit.value(index + 1);
        quint32 combined = (static_cast<quint32>(high) << 16) | low;
        return qFromBigEndian(combined);
    };

    // Helper lambda to parse a 4-byte Big Endian signed int from two 16-bit registers.
    auto parseInt32 = [&](int index) -> qint32 {
        quint16 high = dataUnit.value(index);
        quint16 low = dataUnit.value(index + 1);
        quint32 combined = (static_cast<quint32>(high) << 16) | low;
        return qFromBigEndian(combined);
    };

    // --- PARSE PROCESSED DATA (FLOATS) ---
    // Registers 0x03E8 - 0x03E9 (indices 0-1): X-Angle (Pitch)
    newData.imuPitchDeg = parseFloat(0);
    // Registers 0x03EA - 0x03EB (indices 2-3): Y-Angle (Roll)
    newData.imuRollDeg = parseFloat(2);
    // Registers 0x03EC - 0x03ED (indices 4-5): Temperature
    newData.temperature = parseFloat(4) / 10.0; // Apply scaling factor

    // --- PARSE RAW IMU DATA (INT32) ---
    // The protocol document doesn't explicitly mention Yaw, but the datasheet does.
    // It's often the next value. We assume it's here until confirmed by the manufacturer.
    // Let's assume for now the raw data starts at a later, undocumented address.
    // The documented raw data starts at 0x03EE.

    // Registers 0x03EE - 0x03EF (indices 6-7): Raw Accel X
    newData.rawAccelX = parseInt32(6);
    // Registers 0x03F0 - 0x03F1 (indices 8-9): Raw Accel Y
    newData.rawAccelY = parseInt32(8);
    // Registers 0x03F2 - 0x03F3 (indices 10-11): Raw Accel Z
    newData.rawAccelZ = parseInt32(10);
    // Registers 0x03F4 - 0x03F5 (indices 12-13): Raw Gyro X
    newData.rawGyroX = parseInt32(12);
    // Registers 0x03F6 - 0x03F7 (indices 14-15): Raw Gyro Y
    newData.rawGyroY = parseInt32(14);
    // Registers 0x03F8 - 0x03F9 (indices 16-17): Raw Gyro Z
    newData.rawGyroZ = parseInt32(16);

    // Note: The Yaw data register is not in this protocol doc. If you get the full
    // documentation, you may need to add another read request for it or adjust the
    // start address and count if it is contiguous.

    updateGyroData(newData);
}

// Called after each read cycle finishes, whether successful or not.
// We can leave this empty as we only perform one read operation per cycle.
void ImuDevice::onDataReadComplete()
{
    // This function can be used to coordinate multiple read requests.
    // Since we only have one, there is nothing to do here.
}

// This device does not write any data.
void ImuDevice::onWriteComplete()
{
    // No-op
}

void ImuDevice::updateGyroData(const GyroData &newData)
{
    QMutexLocker locker(&m_mutex);
    if (newData != m_currentData) {
        m_currentData = newData;
        locker.unlock(); // Unlock before emitting the signal
        emit gyroDataChanged(m_currentData);
    }
}