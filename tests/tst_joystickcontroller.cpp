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


// A struct to hold all the mocks and the controller under test
struct TestFixture
{
    MockJoystickDataModel* mockJoystickModel;
    MockSystemStateModel* mockStateModel;
    MockWeaponController* mockWeaponController;
    MockCameraController* mockCameraController;
    GimbalController* mockGimbalController; // Remains null as in original
    JoystickController* controller;
    QObject* parent; // Parent for memory management

    TestFixture(QObject* parent = nullptr) : parent(parent)
    {
        mockJoystickModel = new MockJoystickDataModel(parent);
        mockStateModel = new MockSystemStateModel(parent);
        mockWeaponController = new MockWeaponController(mockStateModel);
        mockCameraController = new MockCameraController();
        mockGimbalController = nullptr; // Explicitly null

        controller = new JoystickController(
            mockJoystickModel,
            mockStateModel,
            mockGimbalController,
            mockCameraController,
            mockWeaponController,
            parent
        );
    }

    ~TestFixture()
    {
        // Qt's parent-child memory management handles deletion
        // No need to delete controller, it will be deleted when parent is.
        delete mockCameraController; // This one has no parent
        // The rest are children of 'parent' and will be auto-deleted.
    }

    void setStationEnabled(bool enabled)
    {
        mockStateModel->m_testData.stationEnabled = enabled;
    }
};


class TestJoystickController : public QObject
{
    Q_OBJECT

public:
    TestJoystickController() {}
    ~TestJoystickController() {}

private:
    // Define constants for button mappings to avoid magic numbers
    const int FIRE_BUTTON = 5;
    const int TRACK_BUTTON = 4;
    const int MODE_CYCLE_BUTTON = 11;

    TestFixture* m_fixture;

private slots:
    void init();
    void cleanup();

    // Test cases
    void testInitialization();
    void testFireButtonAction_data();
    void testFireButtonAction();
    void testActionWhenStationDisabled();
    void testTrackingButtonLifecycle();
    void testMotionModeCycling_data();
    void testMotionModeCycling();
};

void TestJoystickController::init()
{
    m_fixture = new TestFixture(this);
}

void TestJoystickController::cleanup()
{
    delete m_fixture;
}

void TestJoystickController::testInitialization()
{
    QVERIFY(m_fixture->controller != nullptr);
}

void TestJoystickController::testFireButtonAction_data()
{
    QTest::addColumn<int>("button");
    QTest::addColumn<bool>("pressed");
    QTest::addColumn<bool>("shouldStartFiring");
    QTest::addColumn<bool>("shouldStopFiring");

    QTest::newRow("Press Fire")   << FIRE_BUTTON << true  << true  << false;
    QTest::newRow("Release Fire") << FIRE_BUTTON << false << false << true;
}

void TestJoystickController::testFireButtonAction()
{
    QFETCH(int, button);
    QFETCH(bool, pressed);
    QFETCH(bool, shouldStartFiring);
    QFETCH(bool, shouldStopFiring);

    // Arrange
    m_fixture->setStationEnabled(true);

    // Act
    m_fixture->mockJoystickModel->sendButtonPress(button, pressed);

    // Assert
    QCOMPARE(m_fixture->mockWeaponController->startFiringCalled, shouldStartFiring);
    QCOMPARE(m_fixture->mockWeaponController->stopFiringCalled, shouldStopFiring);
}

void TestJoystickController::testActionWhenStationDisabled()
{
    // Arrange: Set station to disabled
    m_fixture->setStationEnabled(false);
    m_fixture->mockStateModel->resetFlags();

    // Act: Simulate pressing the fire button
    m_fixture->mockJoystickModel->sendButtonPress(FIRE_BUTTON, true);

    // Assert: Verify that no action was taken on the weapon controller
    QVERIFY(!m_fixture->mockWeaponController->startFiringCalled);

    // Act: Simulate trying to cycle modes
    m_fixture->mockJoystickModel->sendButtonPress(MODE_CYCLE_BUTTON, true);

    // Assert: Verify that motion mode was not changed
    QVERIFY(!m_fixture->mockStateModel->setMotionModeCalled);
}

void TestJoystickController::testTrackingButtonLifecycle()
{
    // --- STEP 1: Test transitioning from Off to Acquisition ---
    m_fixture->mockStateModel->reset();
    m_fixture->setStationEnabled(true);
    m_fixture->mockStateModel->m_testData.deadManSwitchActive = true;
    m_fixture->mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Off;

    m_fixture->mockJoystickModel->sendButtonPress(TRACK_BUTTON, true);

    QVERIFY(m_fixture->mockStateModel->startTrackingAcquisitionCalled);
    QCOMPARE(m_fixture->mockStateModel->data().currentTrackingPhase, TrackingPhase::Acquisition);

    // Simulate a delay longer than the double-click interval for the next single press
    QTest::qWait(1100);

    // --- STEP 2: Test transitioning from Acquisition to LockPending ---
    m_fixture->mockStateModel->resetFlags();
    m_fixture->setStationEnabled(true);
    m_fixture->mockStateModel->m_testData.deadManSwitchActive = true;
    m_fixture->mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Acquisition;

    m_fixture->mockJoystickModel->sendButtonPress(TRACK_BUTTON, true);

    QVERIFY(m_fixture->mockStateModel->requestTrackerLockOnCalled);
    QCOMPARE(m_fixture->mockStateModel->data().currentTrackingPhase, TrackingPhase::Tracking_LockPending);

    // --- STEP 3: Test aborting tracking with a double-click ---
    m_fixture->mockStateModel->resetFlags();
    m_fixture->setStationEnabled(true);
    m_fixture->mockStateModel->m_testData.deadManSwitchActive = true;
    m_fixture->mockStateModel->m_testData.currentTrackingPhase = TrackingPhase::Tracking_ActiveLock;

    m_fixture->mockJoystickModel->sendButtonPress(TRACK_BUTTON, true); // 1st click
    m_fixture->mockJoystickModel->sendButtonPress(TRACK_BUTTON, true); // 2nd click

    QVERIFY(m_fixture->mockStateModel->stopTrackingCalled);
    QCOMPARE(m_fixture->mockStateModel->data().currentTrackingPhase, TrackingPhase::Off);
}

void TestJoystickController::testMotionModeCycling_data()
{
    QTest::addColumn<MotionMode>("fromMode");
    QTest::addColumn<MotionMode>("toMode");

    QTest::newRow("Manual -> AutoSectorScan") << MotionMode::Manual << MotionMode::AutoSectorScan;
    QTest::newRow("AutoSectorScan -> TRPScan") << MotionMode::AutoSectorScan << MotionMode::TRPScan;
    QTest::newRow("TRPScan -> RadarSlew") << MotionMode::TRPScan << MotionMode::RadarSlew;
    QTest::newRow("RadarSlew -> Manual") << MotionMode::RadarSlew << MotionMode::Manual;
}

void TestJoystickController::testMotionModeCycling()
{
    QFETCH(MotionMode, fromMode);
    QFETCH(MotionMode, toMode);

    // Arrange
    m_fixture->setStationEnabled(true);
    m_fixture->mockStateModel->m_testData.motionMode = fromMode;
    m_fixture->mockStateModel->resetFlags();

    // Act
    m_fixture->mockJoystickModel->sendButtonPress(MODE_CYCLE_BUTTON, true);

    // Assert
    QVERIFY(m_fixture->mockStateModel->setMotionModeCalled);
    QCOMPARE(m_fixture->mockStateModel->lastSetMotionMode, toMode);
}

QTEST_MAIN(TestJoystickController)

#include "tst_joystickcontroller.moc"
