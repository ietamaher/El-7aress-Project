#include "imudevice.h"
#include <QModbusDataUnit>
#include <QDebug>
#include <QtEndian>
#include <QMutexLocker>

ImuDevice::ImuDevice(const QString &device, int baudRate, int slaveId, QObject *parent)
    // The SST810 protocol specifies NO parity. This is crucial.
    : ModbusDeviceBase(device, baudRate, slaveId, QSerialPort::NoParity, parent)
{
    connect(this, &ModbusDeviceBase::connectionStateChanged, this, &ImuDevice::handleConnectionChange);
    
    setPollInterval(50);
}

ImuDevice::~ImuDevice() {
    disconnectDevice();
}

ImuData ImuDevice::getCurrentData() const {
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

void ImuDevice::handleConnectionChange(bool connected) {
    ImuData newData = getCurrentData();
    newData.isConnected = connected;
    updateImuData(newData);
}

void ImuDevice::readData() {
    // Read all 18 registers (9 float values) in a single request.
    QModbusDataUnit readUnit(QModbusDataUnit::InputRegisters,
                             ALL_DATA_START_ADDRESS,
                             ALL_DATA_REGISTER_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        connectReplyFinished(reply, [this](QModbusReply* r) { onReadReady(r); });
    }
}

void ImuDevice::onReadReady(QModbusReply *reply) {
    stopTimeoutTimer();
    if (!reply) return;

    if (reply->error() != QModbusDevice::NoError) {
        logError(QString("IMU Read Error: %1 (Code: 0x%2)")
                     .arg(reply->errorString())
                     .arg(reply->rawResult().exceptionCode(), 2, 16, QChar('0')));
    } else {
        parseModbusResponse(reply->result());
    }

    onDataReadComplete();
}

void ImuDevice::parseModbusResponse(const QModbusDataUnit &dataUnit) {
    if (dataUnit.valueCount() != ALL_DATA_REGISTER_COUNT) {
        logError(QString("IMU: Incorrect register count. Expected %1, got %2.")
                     .arg(ALL_DATA_REGISTER_COUNT).arg(dataUnit.valueCount()));
        return;
    }

    ImuData newData = getCurrentData();

    // Helper lambda to parse a 4-byte Big Endian float from two 16-bit registers.
    auto parseFloat = [&](int index) -> float {
        // Combine two 16-bit registers into one 32-bit value.
        quint16 high = dataUnit.value(index);
        quint16 low = dataUnit.value(index + 1);
        quint32 combined = (static_cast<quint32>(high) << 16) | low;

        // Reinterpret the bits of the 32-bit integer as a float.
        float value;
        memcpy(&value, &combined, sizeof(value));
        return value;
    };

    // --- Map registers to data structure fields ---
    // Per the tech support email, ALL values are floats.
    // The device uses the standard X-Y axis labels, but this may not map
    // directly to aerospace imuRollDeg/imuPitchDeg conventions.
    // X-Angle -> Pitch, Y-Angle -> Roll is a common mapping.
    newData.imuPitchDeg         = parseFloat(0);  // 0x03E8 - 0x03E9: X-axis angle
    newData.imuRollDeg          = parseFloat(2);  // 0x03EA - 0x03EB: Y-axis angle
    newData.temperature   = parseFloat(4) / 10.0; // 0x03EC - 0x03ED: Temperature (with scaling)

    newData.accelX_g      = parseFloat(6);  // 0x03EE - 0x03EF: X-axis accelerometer
    newData.accelY_g      = parseFloat(8);  // 0x03F0 - 0x03F1: Y-axis accelerometer
    newData.accelZ_g      = parseFloat(10); // 0x03F2 - 0x03F3: Z-axis accelerometer

    newData.angRateX_dps  = parseFloat(12); // 0x03F4 - 0x03F5: X-axis gyroscope (Pitch Rate)
    newData.angRateY_dps  = parseFloat(14); // 0x03F6 - 0x03F7: Y-axis gyroscope (Roll Rate)
    newData.angRateZ_dps  = parseFloat(16); // 0x03F8 - 0x03F9: Z-axis gyroscope (Yaw Rate)

    updateImuData(newData);

    /*    qDebug().noquote().nospace()
        << "[IMU] imuPitchDeg=" << newData.imuPitchDeg
        << " imuRollDeg=" << newData.imuRollDeg
        << " temp=" << newData.temperature
        << " accel=(" << newData.accelX_g << ", "
                      << newData.accelY_g << ", "
                      << newData.accelZ_g << ")"
        << " gyro=(" << newData.angRateX_dps << ", "
                     << newData.angRateY_dps << ", "
                     << newData.angRateZ_dps << ")";*/
}

void ImuDevice::onDataReadComplete() { /* No-op for single read request */ }
void ImuDevice::onWriteComplete()    { /* No-op, read-only device */ }

void ImuDevice::updateImuData(const ImuData &newData) {
    QMutexLocker locker(&m_mutex);
    if (newData != m_currentData) {
        m_currentData = newData;
        locker.unlock();
        emit imuDataChanged(m_currentData);
    }
}
