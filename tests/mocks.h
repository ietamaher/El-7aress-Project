#ifndef MOCKS_H
#define MOCKS_H

#include <QObject>
#include "models/joystickdatamodel.h"
#include "models/systemstatemodel.h"
#include "controllers/gimbalcontroller.h"
#include "controllers/cameracontroller.h"
#include "controllers/weaponcontroller.h"

// --- Mock JoystickDataModel ---
class MockJoystickDataModel : public JoystickDataModel
{
    Q_OBJECT
public:
    MockJoystickDataModel(QObject *parent = nullptr) : JoystickDataModel(parent) {}
    // We don't need to override any methods, just emit signals
};

// --- Mock SystemStateModel ---
class MockSystemStateModel : public SystemStateModel
{
    Q_OBJECT
public:
    MockSystemStateModel(QObject *parent = nullptr) : SystemStateModel(parent) {}
    virtual ~MockSystemStateModel() {}

    Q_INVOKABLE void adjustAcquisitionBoxSize(float dW, float dH) {
        adjustAcquisitionBoxSize_called = true;
        last_dW = dW;
        last_dH = dH;
    }

    Q_INVOKABLE void stopTracking() {
        stopTracking_called = true;
    }

    Q_INVOKABLE void startTrackingAcquisition() {
        startTrackingAcquisition_called = true;
    }

    Q_INVOKABLE void requestTrackerLockOn() {
        requestTrackerLockOn_called = true;
    }

    Q_INVOKABLE void setMotionMode(MotionMode mode) {
        setMotionMode_called = true;
        last_mode = mode;
    }

    Q_INVOKABLE void commandEngagement(bool engage) {
        commandEngagement_called = true;
        last_engage = engage;
    }

    Q_INVOKABLE void setDeadManSwitch(bool active) {
        setDeadManSwitch_called = true;
        last_deadManSwitch = active;
    }

    Q_INVOKABLE void setUpSw(bool active) {
        setUpSw_called = true;
        last_upSw = active;
    }

    Q_INVOKABLE void setDownSw(bool active) {
        setDownSw_called = true;
        last_downSw = active;
    }

    Q_INVOKABLE void setUpTrack(bool active) {
        setUpTrack_called = true;
        last_upTrack = active;
    }

    Q_INVOKABLE void setDownTrack(bool active) {
        setDownTrack_called = true;
        last_downTrack = active;
    }

    Q_INVOKABLE void selectNextTRPLocationPage() {
        selectNextTRPLocationPage_called = true;
    }

    Q_INVOKABLE void selectPreviousTRPLocationPage() {
        selectPreviousTRPLocationPage_called = true;
    }

    Q_INVOKABLE void selectNextAutoSectorScanZone() {
        selectNextAutoSectorScanZone_called = true;
    }

    Q_INVOKABLE void selectPreviousAutoSectorScanZone() {
        selectPreviousAutoSectorScanZone_called = true;
    }

    Q_INVOKABLE void setLeadAngleCompensationActive(bool active) {
        setLeadAngleCompensationActive_called = true;
        last_leadAngleCompensationActive = active;
    }

    bool adjustAcquisitionBoxSize_called = false;
    float last_dW = 0.0f;
    float last_dH = 0.0f;
    bool stopTracking_called = false;
    bool startTrackingAcquisition_called = false;
    bool requestTrackerLockOn_called = false;
    bool setMotionMode_called = false;
    MotionMode last_mode = MotionMode::Manual;
    bool commandEngagement_called = false;
    bool last_engage = false;
    bool setDeadManSwitch_called = false;
    bool last_deadManSwitch = false;
    bool setUpSw_called = false;
    bool last_upSw = false;
    bool setDownSw_called = false;
    bool last_downSw = false;
    bool setUpTrack_called = false;
    bool last_upTrack = false;
    bool setDownTrack_called = false;
    bool last_downTrack = false;
    bool selectNextTRPLocationPage_called = false;
    bool selectPreviousTRPLocationPage_called = false;
    bool selectNextAutoSectorScanZone_called = false;
    bool selectPreviousAutoSectorScanZone_called = false;
    bool setLeadAngleCompensationActive_called = false;
    bool last_leadAngleCompensationActive = false;
};

// --- Mock GimbalController ---
class MockGimbalController : public GimbalController
{
    Q_OBJECT
public:
    MockGimbalController(QObject* parent = nullptr) : GimbalController(nullptr, nullptr, nullptr, nullptr, parent) {}
    virtual ~MockGimbalController() {}
    // Since GimbalController's methods are not directly called with specific testable values in the joystick controller,
    // we don't need to override them for now. We can add them if needed.
};

// --- Mock CameraController ---
class MockCameraController : public CameraController
{
    Q_OBJECT
public:
    explicit MockCameraController(QObject* parent = nullptr) : CameraController(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, parent) {}
    virtual ~MockCameraController() {}

    Q_INVOKABLE void zoomIn() {
        zoomIn_called = true;
    }

    Q_INVOKABLE void zoomOut() {
        zoomOut_called = true;
    }

    Q_INVOKABLE void zoomStop() {
        zoomStop_called = true;
    }

    Q_INVOKABLE void nextVideoLUT() {
        nextVideoLUT_called = true;
    }

    Q_INVOKABLE void prevVideoLUT() {
        prevVideoLUT_called = true;
    }

    bool zoomIn_called = false;
    bool zoomOut_called = false;
    bool zoomStop_called = false;
    bool nextVideoLUT_called = false;
    bool prevVideoLUT_called = false;
};

// --- Mock WeaponController ---
class MockWeaponController : public WeaponController
{
    Q_OBJECT
public:
    MockWeaponController(SystemStateModel* model, GimbalController* gimbal, QObject* parent = nullptr) : WeaponController(model, nullptr, nullptr, parent) {}
    virtual ~MockWeaponController() {}

    Q_INVOKABLE void startFiring() {
        startFiring_called = true;
    }

    Q_INVOKABLE void stopFiring() {
        stopFiring_called = true;
    }

    Q_INVOKABLE void updateFireControlSolution() {
        updateFireControlSolution_called = true;
    }


    bool startFiring_called = false;
    bool stopFiring_called = false;
    bool updateFireControlSolution_called = false;
};


#endif // MOCKS_H
