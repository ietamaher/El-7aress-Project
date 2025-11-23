// tests/tst_joystickcontroller.cpp

#include <QtTest>
#include <QObject>
#include <QSignalSpy>

// Include real headers, since they are now correctly found by the build system
#include "models/systemstatedata.h"
#include "controllers/joystickcontroller.h"
#include "models/systemstatemodel.h"
#include "controllers/weaponcontroller.h"
#include "controllers/cameracontroller.h"
#include "models/joystickdatamodel.h"

// --- Minimal Mock Definitions ---

class MockJoystickDataModel : public JoystickDataModel
{
    Q_OBJECT
public:
    MockJoystickDataModel(QObject* parent = nullptr) : JoystickDataModel(parent) {}
    void sendButtonPress(int button, bool pressed) { emit buttonPressed(button, pressed); }
    void sendAxisMove(int axis, float value) { emit axisMoved(axis, value); }
    void sendHatMove(int hat, int value) { emit hatMoved(hat, value); }
};

class MockSystemStateModel : public SystemStateModel
{
    Q_OBJECT
public:
    MockSystemStateModel(QObject* parent = nullptr) : SystemStateModel(parent) { reset(); }

    // This method hides the base class's non-virtual data() method
     SystemStateData data() const override {
         qDebug() << ">>>> MOCK SystemStateModel::data() CALLED! stationEnabled is:" << m_testData.stationEnabled;
         return m_testData;
     }

    SystemStateData m_testData;
    bool setDeadManSwitchCalled = false;
    bool setMotionModeCalled = false;
    bool commandEngagementCalled = false;
    bool startTrackingAcquisitionCalled = false;
    bool requestTrackerLockOnCalled = false;
    bool stopTrackingCalled = false;
    bool selectNextAutoSectorScanZoneCalled = false;
    bool setLeadAngleCompensationActiveCalled = false;
    MotionMode lastSetMotionMode = MotionMode::Idle;
    bool lastEngagementState = false;

    void reset() {
        m_testData = SystemStateData(); // Resets everything
        resetFlags();
    }

    void resetFlags() { // Resets just the call trackers
        setDeadManSwitchCalled = false;
        setMotionModeCalled = false;
        commandEngagementCalled = false;
        startTrackingAcquisitionCalled = false;
        requestTrackerLockOnCalled = false;
        stopTrackingCalled = false;
        selectNextAutoSectorScanZoneCalled = false;
        setLeadAngleCompensationActiveCalled = false;
        lastSetMotionMode = MotionMode::Idle;
        lastEngagementState = false;
    }

public slots:
    // These are public slots, but not virtual in the base class, so we don't use 'override'
    void setDeadManSwitch(bool pressed)  override {
        setDeadManSwitchCalled = true;
        m_testData.deadManSwitchActive = pressed;
    }
    void setMotionMode(MotionMode newMode)  override {
        setMotionModeCalled = true;
        lastSetMotionMode = newMode;
        m_testData.motionMode = newMode;
    }
    void commandEngagement(bool start)  override {
        commandEngagementCalled = true;
        lastEngagementState = start;
    }
    void startTrackingAcquisition()  override {
        startTrackingAcquisitionCalled = true;
        m_testData.currentTrackingPhase = TrackingPhase::Acquisition;
    }
    void requestTrackerLockOn()  override {
        requestTrackerLockOnCalled = true;
        m_testData.currentTrackingPhase = TrackingPhase::Tracking_LockPending;
    }
    void stopTracking()  override {
        stopTrackingCalled = true;
        m_testData.currentTrackingPhase = TrackingPhase::Off;
    }
    void selectNextAutoSectorScanZone()  override {
        selectNextAutoSectorScanZoneCalled = true;
    }
    void setLeadAngleCompensationActive(bool active)  override {
        setLeadAngleCompensationActiveCalled = true;
        m_testData.leadAngleCompensationActive = active;
    }
};

class MockWeaponController : public WeaponController
{
    Q_OBJECT
public:
    MockWeaponController(SystemStateModel* model) : WeaponController(model, nullptr, nullptr, nullptr) {}
    bool startFiringCalled = false;
    bool stopFiringCalled = false;
    // Not virtual in base class, so no 'override'
    void startFiring() override { startFiringCalled = true; }
    void stopFiring() override { stopFiringCalled = true; }
    void reset() { startFiringCalled = false; stopFiringCalled = false; }
};

class MockCameraController : public CameraController
{
    Q_OBJECT
public:
    MockCameraController() : CameraController(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) {}
    bool zoomInCalled = false;
    bool zoomOutCalled = false;
    bool zoomStopCalled = false;
    // Not virtual in base class, so no 'override'
    void zoomIn() override { zoomInCalled = true; }
    void zoomOut() override { zoomOutCalled = true; }
    void zoomStop() override { zoomStopCalled = true; }
    void reset() { zoomInCalled = false; zoomOutCalled = false; zoomStopCalled = false; }
};


class TestJoystickController : public QObject
{
    Q_OBJECT

public:
    TestJoystickController();
    ~TestJoystickController();

private slots:
    void init();
    void cleanup();

    // Test cases
    void testInitialization();
    void testFireButtonAction();
    void testFireButtonAction_data(); // <<< FIXED: Added declaration
    void testActionWhenStationDisabled();
    void testTrackingButtonLifecycle();
    void testMotionModeCycling();

private:
    JoystickController* m_controller;
    MockJoystickDataModel* m_mockJoystickModel;
    MockSystemStateModel* m_mockStateModel;
    MockWeaponController* m_mockWeaponController;
    MockCameraController* m_mockCameraController;
    GimbalController* m_mockGimbalController;
};

TestJoystickController::TestJoystickController() {}
TestJoystickController::~TestJoystickController() {}

void TestJoystickController::init()
{
    m_mockJoystickModel = new MockJoystickDataModel(this);
    m_mockStateModel = new MockSystemStateModel(this);
    m_mockWeaponController = new MockWeaponController(m_mockStateModel);
    m_mockCameraController = new MockCameraController();
    m_mockGimbalController = nullptr;

    m_controller = new JoystickController(
        m_mockJoystickModel,
        m_mockStateModel,
        m_mockGimbalController,
        m_mockCameraController,
        m_mockWeaponController,
        this
    );
}

void TestJoystickController::cleanup()
{
    delete m_controller;
    delete m_mockCameraController;
    delete m_mockWeaponController;
    delete m_mockStateModel;
    delete m_mockJoystickModel;
}

void TestJoystickController::testInitialization()
{
    QVERIFY(m_controller != nullptr);
}

void TestJoystickController::testFireButtonAction_data()
{
    QTest::addColumn<int>("button");
    QTest::addColumn<bool>("pressed");
    QTest::addColumn<bool>("shouldStartFiring");
    QTest::addColumn<bool>("shouldStopFiring");

    QTest::newRow("Press Fire")   << 5 << true  << true  << false;
    QTest::newRow("Release Fire") << 5 << false << false << true;
}

void TestJoystickController::testFireButtonAction()
{
    QFETCH(int, button);
    QFETCH(bool, pressed);
    QFETCH(bool, shouldStartFiring);
    QFETCH(bool, shouldStopFiring);

    // --- FIX: SET THE PRE-CONDITIONS FOR THE TEST ---
    m_mockStateModel->m_testData.stationEnabled = true;

    // Based on the debug output, the real WeaponController checks m_systemArmed.
    // Let's create a mock version of that check.
    // In the real code, this is likely set in WeaponController::armWeapon.
    // For the test, we can simulate it. Let's assume there's a member
    // 'm_systemArmed' in WeaponController that we can't access.
    // The joystick controller calls commandEngagement. Let's look at its code.
    // The logic is inside the REAL WeaponController, so we must make it pass.
    // For now, let's assume `armWeapon` must be called.
    // Let's modify the real WeaponController to allow mocking `m_systemArmed`
    // OR, we fix the test setup.

    // The easiest fix is to tell the test that the system is armed.
    // Let's look at WeaponController.cpp: the condition is probably on `m_systemArmed`.
    // Since we can't change that from the test, we should mock `armWeapon` and
    // ensure it's called.

    // Let's assume your WeaponController::startFiring() checks `m_systemArmed`.
    // Since we are now correctly calling the MOCK `startFiring`, that check is bypassed.
    // So no change is needed here once the virtual/override fix is in place.

    // Act
    m_mockJoystickModel->sendButtonPress(button, pressed);

    // Assert
    QCOMPARE(m_mockWeaponController->startFiringCalled, shouldStartFiring);
    QCOMPARE(m_mockWeaponController->stopFiringCalled, shouldStopFiring);
}

void TestJoystickController::testActionWhenStationDisabled()
{
    // Arrange: Set station to disabled in the model
    m_mockStateModel->resetFlags(); // Ensure a clean state
    m_mockStateModel->m_testData.stationEnabled = false;

    // Act: Simulate pressing the fire button
    m_mockJoystickModel->sendButtonPress(5, true);

    // Assert: Verify that no action was taken on the weapon controller
    QVERIFY(!m_mockWeaponController->startFiringCalled);

    // Arrange for the next part of the test
    m_mockStateModel->m_testData.stationEnabled = false;

    // Act: Simulate trying to cycle modes
    m_mockJoystickModel->sendButtonPress(11, true);

    // Assert: Verify that motion mode was not changed
    QVERIFY(!m_mockStateModel->setMotionModeCalled);
}

void TestJoystickController::testTrackingButtonLifecycle()
{
    // --- STEP 1: Test transitioning from Off to Acquisition ---
    m_mockStateModel->reset();
    m_mockStateModel->m_testData.stationEnabled = true;
    m_mockStateModel->m_testData.deadManSwitchActive = true;
    m_mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Off;

    m_mockJoystickModel->sendButtonPress(4, true);

    QVERIFY(m_mockStateModel->startTrackingAcquisitionCalled);
    QCOMPARE(m_mockStateModel->data().currentTrackingPhase, TrackingPhase::Acquisition);

    // --- THIS IS THE FIX ---
    // Simulate a delay longer than the double-click interval to test the next single press
    QTest::qWait(1100); // Wait for 1.1 seconds (DOUBLE_CLICK_INTERVAL_MS is 1000)

    // --- STEP 2: Test transitioning from Acquisition to LockPending ---
    m_mockStateModel->resetFlags();
    m_mockStateModel->m_testData.stationEnabled = true;
    m_mockStateModel->m_testData.deadManSwitchActive = true;
    m_mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Acquisition;

    m_mockJoystickModel->sendButtonPress(4, true);

    QVERIFY(m_mockStateModel->requestTrackerLockOnCalled);
    QCOMPARE(m_mockStateModel->data().currentTrackingPhase, TrackingPhase::Tracking_LockPending);

    // --- STEP 3: Test aborting tracking with an actual double-click ---
    m_mockStateModel->resetFlags();
    m_mockStateModel->m_testData.stationEnabled = true;
    m_mockStateModel->m_testData.deadManSwitchActive = true;
    m_mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Tracking_ActiveLock;

    m_mockJoystickModel->sendButtonPress(4, true); // No wait here
    m_mockJoystickModel->sendButtonPress(4, true);

    QVERIFY(m_mockStateModel->stopTrackingCalled);
    QCOMPARE(m_mockStateModel->data().currentTrackingPhase, TrackingPhase::Off);
}

void TestJoystickController::testMotionModeCycling()
{
    m_mockStateModel->m_testData.stationEnabled = true;
    m_mockStateModel->m_testData.motionMode = MotionMode::Manual;

    m_mockJoystickModel->sendButtonPress(11, true);
    QVERIFY(m_mockStateModel->setMotionModeCalled);
    QCOMPARE(m_mockStateModel->lastSetMotionMode, MotionMode::AutoSectorScan);
    m_mockStateModel->resetFlags();
    m_mockStateModel->m_testData.motionMode = MotionMode::AutoSectorScan;

    m_mockJoystickModel->sendButtonPress(11, true);
    QVERIFY(m_mockStateModel->setMotionModeCalled);
    QCOMPARE(m_mockStateModel->lastSetMotionMode, MotionMode::TRPScan);
    m_mockStateModel->resetFlags();
    m_mockStateModel->m_testData.motionMode = MotionMode::TRPScan;

    m_mockJoystickModel->sendButtonPress(11, true);
    QVERIFY(m_mockStateModel->setMotionModeCalled);
    QCOMPARE(m_mockStateModel->lastSetMotionMode, MotionMode::RadarSlew);
    m_mockStateModel->resetFlags();
    m_mockStateModel->m_testData.motionMode = MotionMode::RadarSlew;

    m_mockJoystickModel->sendButtonPress(11, true);
    QVERIFY(m_mockStateModel->setMotionModeCalled);
    QCOMPARE(m_mockStateModel->lastSetMotionMode, MotionMode::Manual);
}

QTEST_MAIN(TestJoystickController)

#include "tst_joystickcontroller.moc"
