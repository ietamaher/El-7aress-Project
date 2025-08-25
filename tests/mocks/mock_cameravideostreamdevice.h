#ifndef MOCK_CAMERAVIDEOSTREAMDEVICE_H
#define MOCK_CAMERAVIDEOSTREAMDEVICE_H

#include "../../src/devices/cameravideostreamdevice.h"

class MockCameraVideoStreamDevice : public CameraVideoStreamDevice
{
    Q_OBJECT

public:
    MockCameraVideoStreamDevice(int cameraIndex = 0, SystemStateModel* model = nullptr)
        : CameraVideoStreamDevice(cameraIndex, "mock", 640, 480, model)
    {
        setTrackingEnabledCalled = 0;
        setDetectionEnabledCalled = 0;
        onSystemStateChangedCalled = 0;
        lastTrackingEnabledState = false;
        lastDetectionEnabledState = false;
    }

    // Call counters and state
    int setTrackingEnabledCalled;
    int setDetectionEnabledCalled;
    int onSystemStateChangedCalled;
    bool lastTrackingEnabledState;
    bool lastDetectionEnabledState;
    SystemStateData lastSystemStateData;

public slots:
    void setTrackingEnabled(bool enabled) override {
        setTrackingEnabledCalled++;
        lastTrackingEnabledState = enabled;
    }

    void setDetectionEnabled(bool enabled) override {
        setDetectionEnabledCalled++;
        lastDetectionEnabledState = enabled;
    }

    void onSystemStateChanged(const SystemStateData &newState) override {
        onSystemStateChangedCalled++;
        lastSystemStateData = newState;
    }

protected:
    void run() override {
        // Do nothing, we don't want to start a thread
    }
};

#endif // MOCK_CAMERAVIDEOSTREAMDEVICE_H
