#ifndef DAYCAMERACONTROLDEVICE_H
#define DAYCAMERACONTROLDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QtGlobal>
#include "baseserialdevice.h"

struct DayCameraData {
    bool isConnected = false;
    bool errorState = false;
    quint8 cameraStatus = 0;

    // Zoom control
    bool zoomMovingIn = false;
    bool zoomMovingOut = false;
    quint16 zoomPosition = 0;   // 14-bit max for VISCA

    // Focus control
    bool autofocusEnabled = true;
    quint16 focusPosition = 0;  // 12-bit max

    // Field of view (Sony FCB-EV7520A: 1280×720 native → 1024×768 cropped, NOT square!)
    float currentHFOV = 11.0;  // Horizontal FOV (2.3° - 63.7° zoom range)
    float currentVFOV = 11.0;  // Vertical FOV (calculated from HFOV and aspect ratio)

    bool operator!=(const DayCameraData &other) const {
        return (isConnected != other.isConnected ||
                errorState != other.errorState ||
                cameraStatus != other.cameraStatus ||
                zoomMovingIn != other.zoomMovingIn ||
                zoomMovingOut != other.zoomMovingOut ||
                zoomPosition != other.zoomPosition ||
                autofocusEnabled != other.autofocusEnabled ||
                focusPosition != other.focusPosition ||
                !qFuzzyCompare(currentHFOV, other.currentHFOV) ||
                !qFuzzyCompare(currentVFOV, other.currentVFOV));
    }
};

class DayCameraControlDevice : public BaseSerialDevice
{
    Q_OBJECT

public:
    explicit DayCameraControlDevice(QObject *parent = nullptr);
    
    // Camera-specific interface
    DayCameraData currentData() const;
    
    // Zoom controls
    void zoomIn();
    void zoomOut();
    void zoomStop();
    void setZoomPosition(quint16 position);
    
    // Focus controls
    void focusNear();
    void focusFar();
    void focusStop();
    void setFocusAuto(bool enabled);
    void setFocusPosition(quint16 position);
    
    // Status
    void getCameraStatus();

signals:
    void dayCameraDataChanged(const DayCameraData &data);

protected:
    // Implement base class pure virtual methods
    void configureSerialPort() override;
    void processIncomingData() override;
    void onConnectionEstablished() override;
    void onConnectionLost() override;

private:
    // Pelco-D protocol helpers
    QByteArray buildPelcoD(quint8 address, quint8 cmd1, quint8 cmd2,
                          quint8 data1, quint8 data2) const;
    void sendPelcoDCommand(quint8 cmd1, quint8 cmd2, quint8 data1 = 0, quint8 data2 = 0);
    
    void updateDayCameraData(const DayCameraData &newData);
    double computeHFOVfromZoom(quint16 zoomPos) const;
    
    DayCameraData m_currentData;
    QByteArray m_lastSentCommand;
    
    static const quint8 CAMERA_ADDRESS = 0x01;
};

#endif // DAYCAMERACONTROLDEVICE_H
