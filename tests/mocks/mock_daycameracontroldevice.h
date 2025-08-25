#ifndef MOCK_DAYCAMERACONTROLDEVICE_H
#define MOCK_DAYCAMERACONTROLDEVICE_H

#include "../../src/devices/daycameracontroldevice.h"

class MockDayCameraControlDevice : public DayCameraControlDevice
{
    Q_OBJECT

public:
    MockDayCameraControlDevice() {
        // Reset call counters
        zoomInCalled = 0;
        zoomOutCalled = 0;
        zoomStopCalled = 0;
        focusNearCalled = 0;
        focusFarCalled = 0;
        focusStopCalled = 0;
        setFocusAutoCalled = 0;
    }

    // Call counters
    int zoomInCalled;
    int zoomOutCalled;
    int zoomStopCalled;
    int focusNearCalled;
    int focusFarCalled;
    int focusStopCalled;
    int setFocusAutoCalled;
    bool lastFocusAutoState;

    // Override methods to track calls
    void zoomIn() override { zoomInCalled++; }
    void zoomOut() override { zoomOutCalled++; }
    void zoomStop() override { zoomStopCalled++; }
    void focusNear() override { focusNearCalled++; }
    void focusFar() override { focusFarCalled++; }
    void focusStop() override { focusStopCalled++; }
    void setFocusAuto(bool enabled) override {
        setFocusAutoCalled++;
        lastFocusAutoState = enabled;
    }
};

#endif // MOCK_DAYCAMERACONTROLDEVICE_H
