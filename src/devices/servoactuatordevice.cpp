#include "servoactuatordevice.h"
#include <QDebug>
#include <QRegularExpression>
#include <QTextStream>
#include <cmath>

//================================================================================
// ActuatorStatus and ServoActuatorData Implementation
//================================================================================

// Initialize the static constant map for the ActuatorStatus struct
const QMap<int, QString> ActuatorStatus::STATUS_BIT_MAP = {
    {0, "Optically isolated digital input"}, {1, "Relative Humidity > ovHumid"},
    {2, "Temperature > ovTemp"}, {3, "Emergency shutdown (Latching)"},
    {4, "Supply voltage > upper limit"}, {5, "Motor control is enabled"},
    {6, "Trajectory generator is active"}, {7, "Direction is extending"},
    {8, "Position < spMin"}, {9, "Position > spMax"},
    {10, "Input signal < min value"}, {11, "Input signal > max value"},
    {12, "Position error < atTargWin"}, {13, "Position error > ovErrP"},
    {14, "Speed > ovSpeed"}, {15, "Torque > ovTorq"},
    {16, "Position > posGrtr"}, {17, "Position < posLess"},
    {18, "Bridge driver fault indication is active (Latching)"}, {19, "USB is connected"},
    {20, "Run against retracted stop (Latching)"}, {21, "Run against extended stop (Latching)"},
    {22, "Supply voltage < lower limit (Latching)"}, {23, "Supply voltage > upper limit (Latching)"},
    {24, "Bridge driver fault has occurred (Latching)"}, {25, "Bridge current feedback saturated (Latching)"},
    {26, "4-20mA input < lower limit"}, {27, "4-20mA output out of range"},
    {28, "Internal disk modified (Latching)"}, {29, "HARDWARE.TXT error (Latching)"},
    {30, "CONFIG.TXT error (Latching)"}, {31, "Critical config error, MOTOR OFF (Latching)"}
};

// The parsing logic now lives inside the ActuatorStatus struct
void ActuatorStatus::parse(const QString &hexStatus) {
    // Clear previous status
    activeStatusMessages.clear();
    isMotorOff = false;
    isLatchingFaultActive = false;

    bool ok;
    quint32 statusValue = hexStatus.toUInt(&ok, 16);
    if (!ok) {
        activeStatusMessages.append("Invalid Hex Status Received");
        return;
    }

    for (int i = 0; i < 32; ++i) {
        if ((statusValue >> i) & 1) { // Check if the i-th bit is set
            QString message = STATUS_BIT_MAP.value(i, QString("Unknown Bit %1").arg(i));
            activeStatusMessages.append(message);
            if (message.contains("(Latching)")) {
                isLatchingFaultActive = true;
                if (i == 3 || i == 31) {
                    isMotorOff = true;
                }
            }
        }
    }
}

// Implementation for the ServoActuatorData inequality operator
bool ServoActuatorData::operator!=(const ServoActuatorData &other) const {
    return (isConnected != other.isConnected ||
            qFuzzyCompare(position_mm, other.position_mm) == false ||
            qFuzzyCompare(velocity_mm_s, other.velocity_mm_s) == false ||
            qFuzzyCompare(temperature_c, other.temperature_c) == false ||
            qFuzzyCompare(busVoltage_v, other.busVoltage_v) == false ||
            qFuzzyCompare(torque_percent, other.torque_percent) == false ||
            status != other.status);
}


//================================================================================
// ServoActuatorDevice Implementation
//================================================================================

ServoActuatorDevice::ServoActuatorDevice(QObject *parent)
    : BaseSerialDevice(parent), m_timeoutTimer(new QTimer(this)) {
    // Constructor is now very clean
    connect(m_timeoutTimer, &QTimer::timeout, this, &ServoActuatorDevice::handleTimeout);
}

//================================================================================
// Serial Port and Connection Management
//================================================================================

void ServoActuatorDevice::configureSerialPort() {
    m_serialPort->setBaudRate(QSerialPort::Baud115200); // Correct baud rate
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void ServoActuatorDevice::onConnectionEstablished() {
    m_currentData.isConnected = true;
    emit actuatorDataChanged(m_currentData);
    logMessage("Servo actuator connected. Machine Mode 2.");


}

void ServoActuatorDevice::onConnectionLost() {
    m_currentData.isConnected = false;
    emit actuatorDataChanged(m_currentData);
    logMessage("Servo actuator disconnected.");
}

//================================================================================
// Public Interface Methods
//================================================================================

ServoActuatorData ServoActuatorDevice::currentData() const {
    QMutexLocker locker(&m_mutex);
    return m_currentData;
}

// --- Motion Control ---
void ServoActuatorDevice::moveToPosition(double position_mm) {
    //int counts = millimetersToSensorCounts(position_mm);
    sendCommand(QString("TA%1").arg(position_mm));
}

void ServoActuatorDevice::setMaxSpeed(double speed_mm_s) {
    int counts = speedToSensorCounts(speed_mm_s);
    sendCommand(QString("SP%1").arg(counts));
}

void ServoActuatorDevice::setAcceleration(double accel_mm_s2) {
    int counts = accelToSensorCounts(accel_mm_s2);
    sendCommand(QString("AC%1").arg(counts));
}

void ServoActuatorDevice::setMaxTorque(double percent) {
    int counts = torquePercentToSensorCounts(percent);
    sendCommand(QString("MT%1").arg(counts));
}

void ServoActuatorDevice::stopMove() { sendCommand("TK"); }
void ServoActuatorDevice::holdCurrentPosition() { sendCommand("PC"); }

// --- Status & Diagnostics ---
void ServoActuatorDevice::checkAllStatus() {
    // Queue up all standard polling commands. They will be sent one by one.
    m_commandQueue.append("SR");
    m_commandQueue.append("AP");
    m_commandQueue.append("VL");
    m_commandQueue.append("TQ");
    m_commandQueue.append("RT1"); // Poll temperature in Celsius
    m_commandQueue.append("BV");

    // If no command is pending, start the queue
    if(m_pendingCommand.isEmpty()) {
        sendCommand(m_commandQueue.takeFirst());
    }
}
void ServoActuatorDevice::checkStatusRegister() { sendCommand("SR"); }
void ServoActuatorDevice::checkPosition() { sendCommand("AP"); }
void ServoActuatorDevice::checkVelocity() { sendCommand("VL"); }
void ServoActuatorDevice::checkTorque() { sendCommand("TQ"); }
void ServoActuatorDevice::checkTemperature() { sendCommand("RT1"); }
void ServoActuatorDevice::checkBusVoltage() { sendCommand("BV"); }

// --- System & Configuration ---
void ServoActuatorDevice::saveSettings() { sendCommand("CW321"); }
void ServoActuatorDevice::clearFaults() { sendCommand("ZF"); }
void ServoActuatorDevice::reboot() { sendCommand("ZR321"); }

//================================================================================
// Internal Mechanics (Command Sending, Parsing, Timeouts)
//================================================================================

void ServoActuatorDevice::sendCommand(const QString &command) {
    if (!isConnected()) {
        logError("Cannot send command: not connected.");
        return;
    }

    // Prevent sending a new command while one is already waiting for a response
    if (!m_pendingCommand.isEmpty()) {
        m_commandQueue.append(command);
        return;
    }

    QString stringToChecksum = command + " ";
    QString checksum = calculateChecksum(stringToChecksum);
    QString fullCommand = stringToChecksum + checksum + "\r";

    m_pendingCommand = command;
    sendData(fullCommand.toLatin1());
    m_timeoutTimer->start(1000); // 1-second timeout
}

void ServoActuatorDevice::processIncomingData() {
    while (m_readBuffer.contains('\r')) {
        int endIndex = m_readBuffer.indexOf('\r');
        QByteArray responseData = m_readBuffer.left(endIndex).trimmed();
        m_readBuffer.remove(0, endIndex + 1);

        QString response = QString::fromLatin1(responseData);
        if (response.isEmpty()) continue;

        int lastSpaceIndex = response.lastIndexOf(' ');
        if (lastSpaceIndex == -1) {
            logError("Malformed response (no checksum): " + response);
            continue;
        }

        QString mainResponse = response.left(lastSpaceIndex);
        QString receivedChecksum = response.mid(lastSpaceIndex + 1);


        QString stringToValidate = mainResponse + " ";
        QString calculatedChecksum = calculateChecksum(stringToValidate);
        
        if (receivedChecksum.toUpper() != calculatedChecksum.toUpper()) {
            logError(QString("Checksum Mismatch! Response: '%1', Calculated Checksum: '%2'")
                         .arg(response, calculatedChecksum));
            continue; // Discard corrupted data
        }

        // If we reach here, the response is valid.
        m_timeoutTimer->stop();
        ServoActuatorData newData = m_currentData;

        if (mainResponse.startsWith('A')) { // ACK
            QStringList parts = mainResponse.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            QString dataPart = (parts.size() > 1) ? parts[1] : "";

            if (m_pendingCommand == "SR") {
                // Delegate parsing to the data model
                newData.status.parse(dataPart);

                // Check for critical faults and emit the signal if necessary
                if (newData.status.isMotorOff) {
                    QStringList criticalFaults;
                    for(const auto& msg : newData.status.activeStatusMessages){
                        if(msg.contains("(Latching)") && (msg.contains("Emergency") || msg.contains("MOTOR OFF"))){ // Check for specific critical messages
                            criticalFaults.append(msg);
                        }
                    }
                    emit criticalFaultOccurred(criticalFaults);
                }
            } else if (m_pendingCommand == "AP") {
                newData.position_mm = sensorCountsToMillimeters(dataPart.toInt());
            } else if (m_pendingCommand == "VL") {
                newData.velocity_mm_s = sensorCountsToSpeed(dataPart.toInt());
            } else if (m_pendingCommand == "TQ") {
                newData.torque_percent = sensorCountsToTorquePercent(dataPart.toInt());
            } else if (m_pendingCommand == "RT1") {
                newData.temperature_c = dataPart.toDouble();
            } else if (m_pendingCommand == "BV") {
                newData.busVoltage_v = dataPart.toDouble() / 1000.0; // Assuming response is in mV
            }
            // Other commands like TA, SP, AC, etc., just return 'A' with no data.

        } else if (mainResponse.startsWith('N')) { // NACK
            logError(QString("Command Failed: '%1'. Actuator response: %2")
                         .arg(m_pendingCommand, mainResponse));
            emit commandError(QString("Command '%1' was rejected.").arg(m_pendingCommand));
        }

        m_pendingCommand.clear();
        updateActuatorData(newData);

        // If there are more commands in the queue, send the next one
        if(!m_commandQueue.isEmpty()) {
            QTimer::singleShot(20, this, [this](){
                if(!m_commandQueue.isEmpty()) sendCommand(m_commandQueue.takeFirst());
            });
        }
    }
}

void ServoActuatorDevice::handleTimeout() {
    logError(QString("Timeout waiting for response to command: %1").arg(m_pendingCommand));
    emit commandError(QString("Timeout on command: %1").arg(m_pendingCommand));
    m_pendingCommand.clear();

    // Try to process the next command in the queue if any
    if(!m_commandQueue.isEmpty()) {
        QTimer::singleShot(20, this, [this](){
            if(!m_commandQueue.isEmpty()) sendCommand(m_commandQueue.takeFirst());
        });
    }
}

void ServoActuatorDevice::updateActuatorData(const ServoActuatorData &newData) {
    QMutexLocker locker(&m_mutex);
    if (newData != m_currentData) {
        m_currentData = newData;
        locker.unlock(); // Unlock before emitting the signal
        emit actuatorDataChanged(m_currentData);
    }
}

QString ServoActuatorDevice::calculateChecksum(const QString &command) const {
    quint16 sum = 0;
    for (const QChar &ch : command) {
        sum += ch.toLatin1();
    }
    quint8 checksum_val = sum % 256;
    return QString("%1").arg(checksum_val, 2, 16, QChar('0')).toUpper();
}

//================================================================================
// Unit Conversion Functions
//================================================================================

double ServoActuatorDevice::sensorCountsToMillimeters(int counts) const {
    return static_cast<double>(counts - RETRACTED_ENDSTOP_OFFSET) * SCREW_LEAD_MM / COUNTS_PER_REVOLUTION;
}

int ServoActuatorDevice::millimetersToSensorCounts(double millimeters) const {
    double counts = (millimeters * COUNTS_PER_REVOLUTION / SCREW_LEAD_MM) + RETRACTED_ENDSTOP_OFFSET;
    return static_cast<int>(round(counts));
}

int ServoActuatorDevice::speedToSensorCounts(double speed_mm_s) const {
    // Conversion for speed is more complex, this is a simplified placeholder.
    // Refer to Appendix B in the manual for the exact formula.
    // Assuming a direct linear conversion for this example.
    double rev_per_sec = speed_mm_s / SCREW_LEAD_MM;
    return static_cast<int>(round(rev_per_sec * COUNTS_PER_REVOLUTION));
}

double ServoActuatorDevice::sensorCountsToSpeed(int counts) const {
    // Placeholder, see note above.
    double rev_per_sec = static_cast<double>(counts) / COUNTS_PER_REVOLUTION;
    return rev_per_sec * SCREW_LEAD_MM;
}

int ServoActuatorDevice::accelToSensorCounts(double accel_mm_s2) const {
    // Placeholder, see note above.
    double rev_per_sec2 = accel_mm_s2 / SCREW_LEAD_MM;
    return static_cast<int>(round(rev_per_sec2 * COUNTS_PER_REVOLUTION));
}

double ServoActuatorDevice::sensorCountsToTorquePercent(int counts) const {
    // Max torque value is 32767 for 100%
    return (static_cast<double>(counts) / 32767.0) * 100.0;
}

int ServoActuatorDevice::torquePercentToSensorCounts(double percent) const {
    return static_cast<int>(round((percent / 100.0) * 32767.0));
}


