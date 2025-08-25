// --- lrfdevice.h ---

#ifndef LRFDEVICE_H
#define LRFDEVICE_H

#include "baseserialdevice.h"
#include <QObject>
#include <QSerialPort>
#include <QMutex>
#include <QByteArray>
#include <QTimer>

struct LrfData {
    bool isConnected = false; ///< True if the LRF device is connected.

    // Last ranging values
    quint16 lastDistance = 0;     ///< Last distance measurement in meters.
    bool isLastRangingValid = false; ///< True if the last distance measurement is valid.
    quint8 pulseCount = 0;        ///< Pulse count associated with the last measurement (0-255).

    // System Status Flags (from status byte)
    quint8 rawStatusByte = 0;     ///< The raw status byte from the last response.
    bool isFault = false;         ///< General fault status (from Status0 or Status1).
    bool noEcho = false;          ///< Bit3: 1 = no echo, 0 = echo.
    bool laserNotOut = false;     ///< Bit4: 1 = laser not out, 0 = light out.
    bool isOverTemperature = false; ///< Bit5: 1 = temperature sensor over temp, 0 = normal.

    // Temperature
    bool isTempValid = false;     ///< True if temperature has been successfully read.
    qint8 temperature = 0;        ///< Ambient temperature in degrees C (-55 to +125).

    // Laser usage
    quint32 laserCount = 0;       ///< Accumulated laser shots count.

    bool operator==(const LrfData &other) const {
        return (isConnected == other.isConnected &&
                lastDistance == other.lastDistance &&
                isLastRangingValid == other.isLastRangingValid &&
                pulseCount == other.pulseCount &&
                rawStatusByte == other.rawStatusByte &&
                isFault == other.isFault &&
                noEcho == other.noEcho &&
                laserNotOut == other.laserNotOut &&
                isOverTemperature == other.isOverTemperature &&
                isTempValid == other.isTempValid &&
                temperature == other.temperature &&
                laserCount == other.laserCount);
    }

    bool operator!=(const LrfData &other) const {
        return !(*this == other);
    }
};

class LRFDevice : public BaseSerialDevice
{
    Q_OBJECT

public:
    explicit LRFDevice(QObject *parent = nullptr);

    // LRF-specific interface
    LrfData currentData() const;
    void sendSelfCheck();
    void sendSingleRanging();
    void sendContinuousRanging1Hz();
    void sendContinuousRanging5Hz();
    void sendContinuousRanging10Hz();
    void stopRanging();
    void queryAccumulatedLaserCount();
    void queryProductInfo();
    void queryTemperature();

signals:
    void lrfDataChanged(const LrfData &data);
    void productInfoReceived(quint8 productId, const QString& softwareVersion);

protected:
    // Implement base class pure virtual methods
    void configureSerialPort() override;
    void processIncomingData() override;
    void onConnectionEstablished() override;
    void onConnectionLost() override;

private:
    // LRF-specific methods
    void sendCommand(quint8 commandCode, const QByteArray& params = QByteArray(5, 0x00));
    QByteArray buildCommand(quint8 commandCode, const QByteArray& params) const;
    quint8 calculateChecksum(const QByteArray &body) const;
    bool verifyChecksum(const QByteArray &packet) const;
    void handleResponse(const QByteArray &response);
    void updateLrfData(const LrfData &newData);

    // Response handlers
    void handleSelfCheckResponse(const QByteArray &response);
    void handleRangingResponse(const QByteArray &response);
    void handlePulseCountResponse(const QByteArray &response);
    void handleProductInfoResponse(const QByteArray &response);
    void handleTemperatureResponse(const QByteArray &response);
    void handleStopRangingResponse(const QByteArray &response);

    // Protocol constants
    static const int PACKET_SIZE = 9;
    static const quint8 FRAME_HEADER = 0xEE;

    enum DeviceCode : quint8 {
        LRF = 0x07 ///< Laser Range Finder device code.
    };

    // See documentation table 6.1.1
    enum CommandCode : quint8 {
        SelfTest = 0x01,
        ContinuousRanging5Hz = 0x02,
        ContinuousRanging10Hz = 0x04,
        LaserStop = 0x05,
        TemperatureReading = 0x06,
        PulseCountReport = 0x0A,
        SingleRanging = 0x0B,
        ContinuousRanging1Hz = 0x0C,
        ProductIdentificationReport = 0x10,
    };

    // Response codes are the same as command codes
    using ResponseCode = CommandCode;

    LrfData m_currentData;         ///< Current state of the LRF device data.
    QTimer *m_statusTimer;         ///< Timer for periodic status checks.
};

#endif // LRFDEVICE_H