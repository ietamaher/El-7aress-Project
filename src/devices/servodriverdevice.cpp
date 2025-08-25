#include "servodriverdevice.h"
#include <QModbusReply>
#include <QSerialPort>
#include <QVariant>
#include <QDebug>

ServoDriverDevice::ServoDriverDevice(const QString &identifier,
                                   const QString &device,
                                   int baudRate,
                                    int slaveId,
                                    QSerialPort::Parity parity,
                                   QObject *parent)
    : ModbusDeviceBase(device, baudRate, slaveId, parity, parent),
      m_identifier(identifier),
      m_temperatureTimer(new QTimer(this))
{
    // Initialize alarm map
    initializeAlarmMap();
    
    // Setup temperature timer
    setupTemperatureTimer();
    
    // Set servo-specific parameters
    setTimeout(100);  // 100ms timeout for servo
    setRetries(3);
    setPollInterval(50);  // 50ms for position reading
    
    // Connect temperature timer
    connect(m_temperatureTimer, &QTimer::timeout, 
            this, &ServoDriverDevice::readTemperatureData);
}

ServoDriverDevice::~ServoDriverDevice()
{
    if (m_temperatureTimer) {
        m_temperatureTimer->stop();
    }
}

void ServoDriverDevice::setupTemperatureTimer()
{
    m_temperatureTimer->setInterval(5000); // 30 seconds
    m_temperatureTimer->start();
}

void ServoDriverDevice::readData()
{
    // This is called by the base class polling timer
    // Read position data every poll cycle
    if (!isConnected()) {
        return;
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, 
                            POSITION_START_ADDR, 
                            POSITION_REG_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished, 
                this, &ServoDriverDevice::onPositionReadReady);
    } else {
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }
}

void ServoDriverDevice::onDataReadComplete()
{
    // Called when connection state changes
    ServoData sd = m_currentData;
    sd.isConnected = isConnected();
    
    if (isConnected()) {
        // Start temperature reading when connected
        if (m_temperatureEnabled && !m_temperatureTimer->isActive()) {
            m_temperatureTimer->start();
        }
        emit logMessage(QString("[%1] Connected and data reading started.").arg(m_identifier));
    } else {
        // Stop temperature reading when disconnected
        if (m_temperatureTimer->isActive()) {
            m_temperatureTimer->stop();
        }
        emit logMessage(QString("[%1] Disconnected.").arg(m_identifier));
    }
    
    updateServoData(sd);
}

void ServoDriverDevice::onWriteComplete()
{
    // Called after write operations complete
    emit logMessage(QString("[%1] Write operation completed.").arg(m_identifier));
}

void ServoDriverDevice::readTemperatureData()
{
    if (!isConnected()) {
        return;
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters,
                            TEMPERATURE_START_ADDR,
                            TEMPERATURE_REG_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished,
                this, &ServoDriverDevice::onTemperatureReadReady);
    } else {
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }
}

void ServoDriverDevice::onPositionReadReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    stopTimeoutTimer(); // Stop timeout timer since we got a response

    if (reply->error() == QModbusDevice::NoError) {
        QModbusDataUnit unit = reply->result();
        
        if (unit.valueCount() >= POSITION_REG_COUNT) {
            ServoData newData = m_currentData;
            newData.isConnected = true;

            // Combine two 16-bit registers into 32-bit position value
            int32_t positionRaw = (static_cast<int32_t>(unit.value(0)) << 16) | unit.value(1);
            newData.position = static_cast<float>(positionRaw);

            updateServoData(newData);
        } else {
            qWarning() << QString("[%1] Insufficient position data: %2 registers")
                          .arg(m_identifier).arg(unit.valueCount());
        }
    } else {
        logError(QString("Position read error: %1").arg(reply->errorString()));
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }

    reply->deleteLater();
}

void ServoDriverDevice::onTemperatureReadReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    stopTimeoutTimer();

    if (reply->error() == QModbusDevice::NoError) {
        QModbusDataUnit unit = reply->result();
        
        if (unit.valueCount() >= TEMPERATURE_REG_COUNT) {
            ServoData newData = m_currentData;
            newData.isConnected = true;

            // Driver temperature (registers 0-1)
            int32_t driverTempRaw = (static_cast<int32_t>(unit.value(0)) << 16) | unit.value(1);
            newData.driverTemp = static_cast<float>(driverTempRaw) * 0.1f;

            // Motor temperature (registers 2-3)
            int32_t motorTempRaw = (static_cast<int32_t>(unit.value(2)) << 16) | unit.value(3);
            newData.motorTemp = static_cast<float>(motorTempRaw) * 0.1f;

            updateServoData(newData);
        } else {
            qWarning() << QString("[%1] Insufficient temperature data: %2 registers")
                          .arg(m_identifier).arg(unit.valueCount());
        }
    } else {
        logError(QString("Temperature read error: %1").arg(reply->errorString()));
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }

    reply->deleteLater();
}

void ServoDriverDevice::writeData(int startAddress, const QVector<quint16> &values)
{
    if (!isConnected()) {
        logError("Cannot write: device not connected");
        return;
    }

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, startAddress, values);

    if (auto *reply = sendWriteRequest(writeUnit)) {
        connect(reply, &QModbusReply::finished, 
                this, &ServoDriverDevice::onWriteReady);
    } else {
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }
}

void ServoDriverDevice::onWriteReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    if (reply->error() == QModbusDevice::NoError) {
        emit logMessage(QString("[%1] Write operation succeeded.").arg(m_identifier));
        onWriteComplete();
    } else {
        logError(QString("Write error: %1").arg(reply->errorString()));
        ServoData sd = m_currentData;
        sd.isConnected = false;
        updateServoData(sd);
    }

    reply->deleteLater();
}

void ServoDriverDevice::readAlarmStatus()
{
    if (!isConnected()) return;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters,
                            ALARM_STATUS_ADDR,
                            ALARM_STATUS_REG_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished,
                this, &ServoDriverDevice::onAlarmReadReady);
    } else {
        logError("Failed to read alarm status");
    }
}

void ServoDriverDevice::onAlarmReadReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    stopTimeoutTimer();

    if (reply->error() == QModbusDevice::NoError) {
        QModbusDataUnit unit = reply->result();
        if (unit.valueCount() >= 2) {
            // Combine upper and lower registers for alarm code
            m_currentAlarmCode = (unit.value(0) << 16) | unit.value(1);
            
            if (m_currentAlarmCode != 0) {
                QString alarmDescription = getAlarmDescription(m_currentAlarmCode);
                emit alarmDetected(m_currentAlarmCode, alarmDescription);
            }
        }
    } else {
        logError(QString("Alarm read error: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
}

bool ServoDriverDevice::clearAlarm()
{
    if (!isConnected()) return false;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, ALARM_RESET_ADDR, 2);
    writeUnit.setValue(0, 0); // Upper register
    writeUnit.setValue(1, 1); // Lower register - execute command

    auto *reply = sendWriteRequest(writeUnit);
    if (!reply) {
        logError("Failed to send alarm reset command");
        return false;
    }

    // Handle the reply
    connect(reply, &QModbusReply::finished, this, [this, reply]() {
        if (reply->error() == QModbusDevice::NoError) {
            // Reset the register back to 0
            QModbusDataUnit resetUnit(QModbusDataUnit::HoldingRegisters, ALARM_RESET_ADDR, 2);
            resetUnit.setValue(0, 0);
            resetUnit.setValue(1, 0);
            
            if (auto *resetReply = sendWriteRequest(resetUnit)) {
                connect(resetReply, &QModbusReply::finished, resetReply, &QModbusReply::deleteLater);
            }
            
            m_currentAlarmCode = 0;
            emit alarmCleared();
            emit logMessage(QString("[%1] Alarm cleared successfully.").arg(m_identifier));
        } else {
            logError(QString("Failed to clear alarm: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });

    return true;
}

void ServoDriverDevice::readAlarmHistory()
{
    if (!isConnected()) return;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters,
                            ALARM_HISTORY_ADDR,
                            ALARM_HISTORY_REG_COUNT);

    if (auto *reply = sendReadRequest(readUnit)) {
        connect(reply, &QModbusReply::finished,
                this, &ServoDriverDevice::onAlarmHistoryReady);
    } else {
        logError("Failed to read alarm history");
    }
}

void ServoDriverDevice::onAlarmHistoryReady()
{
    auto *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    stopTimeoutTimer();

    if (reply->error() == QModbusDevice::NoError) {
        QModbusDataUnit unit = reply->result();
        QList<uint16_t> alarmHistory;
        
        // Process alarm history (2 registers per entry)
        for (int i = 0; i < unit.valueCount(); i += 2) {
            if (i + 1 < unit.valueCount()) {
                uint32_t alarmCode = (static_cast<uint32_t>(unit.value(i)) << 16) | unit.value(i + 1);
                if (alarmCode != 0) { // Only add non-zero alarm codes
                    alarmHistory.append(static_cast<uint16_t>(alarmCode));
                }
            }
        }
        emit alarmHistoryRead(alarmHistory);
    } else {
        logError(QString("Alarm history read error: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
}

bool ServoDriverDevice::clearAlarmHistory()
{
    if (!isConnected()) return false;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, ALARM_HISTORY_CLEAR_ADDR, 2);
    writeUnit.setValue(0, 0); // Upper register
    writeUnit.setValue(1, 1); // Lower register - execute command

    auto *reply = sendWriteRequest(writeUnit);
    if (!reply) {
        logError("Failed to send clear alarm history command");
        return false;
    }

    // Handle the reply
    connect(reply, &QModbusReply::finished, this, [this, reply]() {
        if (reply->error() == QModbusDevice::NoError) {
            // Reset the register back to 0
            QModbusDataUnit resetUnit(QModbusDataUnit::HoldingRegisters, ALARM_HISTORY_CLEAR_ADDR, 2);
            resetUnit.setValue(0, 0);
            resetUnit.setValue(1, 0);
            
            if (auto *resetReply = sendWriteRequest(resetUnit)) {
                connect(resetReply, &QModbusReply::finished, resetReply, &QModbusReply::deleteLater);
            }
            
            emit alarmHistoryCleared();
            emit logMessage(QString("[%1] Alarm history cleared successfully.").arg(m_identifier));
        } else {
            logError(QString("Failed to clear alarm history: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });

    return true;
}

void ServoDriverDevice::enableTemperatureReading(bool enable)
{
    m_temperatureEnabled = enable;
    
    if (enable && isConnected()) {
        if (!m_temperatureTimer->isActive()) {
            m_temperatureTimer->start();
        }
    } else {
        if (m_temperatureTimer->isActive()) {
            m_temperatureTimer->stop();
        }
    }
}

void ServoDriverDevice::setTemperatureInterval(int intervalMs)
{
    m_temperatureTimer->setInterval(intervalMs);
}

QString ServoDriverDevice::getAlarmDescription(uint16_t alarmCode)
{
    if (m_alarmMap.contains(alarmCode)) {
        return m_alarmMap.value(alarmCode).alarmName;
    }
    return QString("Unknown Alarm Code: 0x%1").arg(alarmCode, 4, 16, QChar('0'));
}

void ServoDriverDevice::updateServoData(const ServoData &newData)
{
    if (newData != m_currentData) {
        m_currentData = newData;
        emit servoDataChanged(m_currentData);
    }
}

void ServoDriverDevice::initializeAlarmMap()
{
    // Initialize alarm map with servo-specific alarm codes
    m_alarmMap[0x0001] = {0x0001, "Overcurrent Alarm", "Motor current exceeded limit.", "Check motor wiring, reduce load.", false};
    m_alarmMap[0x0002] = {0x0002, "Overvoltage Alarm", "DC bus voltage too high.", "Check power supply, reduce regeneration.", false};
    m_alarmMap[0x0003] = {0x0003, "Undervoltage Alarm", "DC bus voltage too low.", "Check power supply, increase input voltage.", false};
    m_alarmMap[0x0004] = {0x0004, "Overheat Alarm", "Motor or driver temperature too high.", "Check cooling, reduce load.", false};
    m_alarmMap[0x0005] = {0x0005, "Encoder Error", "Encoder signal abnormal.", "Check encoder wiring, replace encoder.", false};
    m_alarmMap[0x0006] = {0x0006, "Communication Error", "Modbus communication lost.", "Check serial connection, verify baud rate.", true};
    // Add more alarms as needed
}
