#ifndef NIGHTCAMERACONTROLDEVICE_H
#define NIGHTCAMERACONTROLDEVICE_H

#include "baseserialdevice.h"
#include <QTimer>

// Structure to hold camera data
struct NightCameraData {
    bool isConnected = false;
    bool errorState = false;
    bool ffcInProgress = false;
    bool digitalZoomEnabled = false;
    quint8 digitalZoomLevel = 0;
    double currentHFOV = 10.4;
    quint16 videoMode = 0;
    quint8 cameraStatus = 0;

    bool operator==(const NightCameraData &other) const {
        return (isConnected == other.isConnected &&
                errorState == other.errorState &&
                ffcInProgress == other.ffcInProgress &&
                digitalZoomEnabled == other.digitalZoomEnabled &&
                digitalZoomLevel == other.digitalZoomLevel &&
                currentHFOV == other.currentHFOV &&
                videoMode == other.videoMode &&
                cameraStatus == other.cameraStatus);
    }
    
    bool operator!=(const NightCameraData &other) const {
        return !(*this == other);
    }
};

class NightCameraControlDevice : public BaseSerialDevice {
    Q_OBJECT

public:
    explicit NightCameraControlDevice(QObject *parent = nullptr);
    ~NightCameraControlDevice();

    // Camera control methods
    void performFFC();
    void setDigitalZoom(quint8 zoomLevel);
    void setVideoModeLUT(quint16 mode);
    void getCameraStatus();

    // Data access
    const NightCameraData& getCurrentData() const { return m_currentData; }

signals:
    void nightCameraDataChanged(const NightCameraData &data);
    void responseReceived(const QByteArray &response);
    void statusChanged(bool isConnected);

protected:
    // BaseSerialDevice interface implementation
    void configureSerialPort() override;
    void processIncomingData() override;
    void onConnectionEstablished() override;
    void onConnectionLost() override;

    // Reconnection settings
    int getMaxReconnectAttempts() const override { return 3; }
    int getReconnectDelayMs(int attempt) const override { return 2000 + (attempt * 1000); }

private slots:
    void checkCameraStatus();

private:
    // Command building and CRC
    QByteArray buildCommand(quint8 function, const QByteArray &data);
    quint16 calculateCRC(const QByteArray &data, int length);
    bool verifyCRC(const QByteArray &packet);

    // Response handlers
    void handleResponse(const QByteArray &response);
    void handleStatusResponse(const QByteArray &data);
    void handleVideoModeResponse(const QByteArray &data);
    void handleVideoLUTResponse(const QByteArray &data);
    void handleFFCResponse(const QByteArray &data);
    void handleStatusError(quint8 statusByte);

    // Data management
    void updateNightCameraData(const NightCameraData &newData);

    // Member variables
    NightCameraData m_currentData;
    QTimer *m_statusCheckTimer = nullptr;
    static const int m_statusCheckIntervalMs = 5000;
};

#endif // NIGHTCAMERACONTROLDEVICE_H