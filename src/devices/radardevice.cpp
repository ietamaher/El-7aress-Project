#include "radardevice.h"
#include <QDebug>
#include <QStringList>

RadarDevice::RadarDevice(QObject *parent)
    : BaseSerialDevice(parent)
{
}

void RadarDevice::configureSerialPort()
{
    // NMEA 0183 standard typically uses 4800 baud, 8-N-1
    m_serialPort->setBaudRate(QSerialPort::Baud4800);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

void RadarDevice::processIncomingData()
{
    // NMEA sentences end with <CR><LF> (\r\n)
    m_readBuffer.append(m_serialPort->readAll());

    while (m_readBuffer.contains("\r\n")) {
        int endIndex = m_readBuffer.indexOf("\r\n");
        QByteArray rawSentence = m_readBuffer.left(endIndex);
        m_readBuffer.remove(0, endIndex + 2); // +2 for \r\n
        // NMEA sentences start with '$'
        if (rawSentence.startsWith("$")) {
            if (validateChecksum(rawSentence)) {
                QString sentence = QString(rawSentence).trimmed();
                QStringList parts = sentence.split("*");
                QString dataPart = parts.at(0);

                if (dataPart.startsWith("$RATTM")) {
                    RadarData plot = parseRATTM(dataPart.toUtf8());
                    // For now, we'll just add to a list. Later, we might update existing targets.
                    // This simple implementation assumes new plots are always added.
                    // A more robust solution would involve matching IDs and updating.
                    m_trackedTargets.append(plot);
                    emit radarPlotsUpdated(m_trackedTargets);
                }
            } else {
                logError("NMEA checksum mismatch: " + QString(rawSentence));
            }
        }
    }
}

bool RadarDevice::validateChecksum(const QByteArray &sentence)
{
    int asteriskIndex = sentence.indexOf("*");
    if (asteriskIndex == -1 || asteriskIndex + 2 >= sentence.length()) {
        return false; // No checksum or incomplete checksum
    }

    QByteArray dataToChecksum = sentence.mid(1, asteriskIndex - 1); // Exclude '$' and '*' and checksum
    quint8 calculatedChecksum = 0;
    for (char c : dataToChecksum) {
        calculatedChecksum ^= static_cast<quint8>(c);
    }

    bool ok;
    quint8 receivedChecksum = sentence.mid(asteriskIndex + 1, 2).toUInt(&ok, 16);

    return ok && (calculatedChecksum == receivedChecksum);
}

RadarData RadarDevice::parseRATTM(const QByteArray &sentence)
{
    RadarData plot;
    QStringList fields = QString(sentence).split(",");

    if (fields.size() >= 10) { // $RATTM,x,x,x,x,x,x,x,x,x*CS<CR><LF>
        plot.id = fields.at(1).toUInt();
        plot.azimuthDegrees = fields.at(2).toFloat(); // Bearing
        plot.rangeMeters = fields.at(3).toFloat() * 1852.0; // Convert nautical miles to meters (1 NM = 1852 meters)
        // fields.at(4) is 'T' or 'M' for True/Magnetic bearing, we ignore for now
        plot.relativeCourseDegrees = fields.at(5).toFloat();
        plot.relativeSpeedMPS = fields.at(6).toFloat() * 0.514444; // Convert knots to m/s (1 knot = 0.514444 m/s)
        // Remaining fields are not used in this basic implementation
    } else {
        logError("Malformed $RATTM sentence: " + QString(sentence));
    }
    return plot;
}
