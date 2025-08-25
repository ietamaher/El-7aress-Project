#ifndef MOCK_NIGHTCAMERACONTROLDEVICE_H
#define MOCK_NIGHTCAMERACONTROLDEVICE_H

#include "../../src/devices/nightcameracontroldevice.h"

class MockNightCameraControlDevice : public NightCameraControlDevice
{
    Q_OBJECT

public:
    MockNightCameraControlDevice() {
        performFFCCalled = 0;
        setDigitalZoomCalled = 0;
        setVideoModeLUTCalled = 0;
        lastDigitalZoomLevel = 0;
        lastVideoModeLUT = 0;
    }

    // Call counters and state
    int performFFCCalled;
    int setDigitalZoomCalled;
    int setVideoModeLUTCalled;
    quint8 lastDigitalZoomLevel;
    quint16 lastVideoModeLUT;

    // Override methods to track calls
    void performFFC() override { performFFCCalled++; }
    void setDigitalZoom(quint8 zoomLevel) override {
        setDigitalZoomCalled++;
        lastDigitalZoomLevel = zoomLevel;
    }
    void setVideoModeLUT(quint16 mode) override {
        setVideoModeLUTCalled++;
        lastVideoModeLUT = mode;
    }
};

#endif // MOCK_NIGHTCAMERACONTROLDEVICE_H
