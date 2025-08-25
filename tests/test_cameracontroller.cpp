#include "test_cameracontroller.h"
#include <QtTest>

// Class under test
#include "../src/controllers/cameracontroller.h"

// Mocks
#include "mocks/mock_daycameracontroldevice.h"
#include "mocks/mock_nightcameracontroldevice.h"
#include "mocks/mock_cameravideostreamdevice.h"
#include "mocks/mock_lensdevice.h"
#include "mocks/mock_systemstatemodel.h"

TestCameraController::TestCameraController() {}

TestCameraController::~TestCameraController() {}

void TestCameraController::init()
{
    // Create mocks
    m_mockDayControl = new MockDayCameraControlDevice();
    m_mockNightControl = new MockNightCameraControlDevice();
    m_mockStateModel = new MockSystemStateModel();
    m_mockDayProcessor = new MockCameraVideoStreamDevice(0, m_mockStateModel);
    m_mockNightProcessor = new MockCameraVideoStreamDevice(1, m_mockStateModel);
    m_mockLensDevice = new MockLensDevice();

    // Create the controller with the mocks
    m_controller = new CameraController(
        m_mockDayControl,
        m_mockDayProcessor,
        m_mockNightControl,
        m_mockNightProcessor,
        m_mockLensDevice,
        m_mockStateModel
    );

    // Set initial state
    SystemStateData initialState;
    initialState.activeCameraIsDay = true;
    m_mockStateModel->setMockedData(initialState);
    m_controller->onSystemStateChanged(initialState); // Manually trigger initial state sync
}

void TestCameraController::cleanup()
{
    delete m_controller;
    delete m_mockDayControl;
    delete m_mockNightControl;
    delete m_mockDayProcessor;
    delete m_mockNightProcessor;
    delete m_mockLensDevice;
    delete m_mockStateModel;
}

void TestCameraController::testConstruction()
{
    QVERIFY(m_controller != nullptr);
    QVERIFY(m_controller->isDayCameraActive());
}

void TestCameraController::testZoomIn_DayCameraActive()
{
    // Arrange: Day camera is active by default in init()

    // Act
    m_controller->zoomIn();

    // Assert
    QCOMPARE(m_mockDayControl->zoomInCalled, 1);
    QCOMPARE(m_mockNightControl->setDigitalZoomCalled, 0);
}

void TestCameraController::testZoomIn_NightCameraActive()
{
    // Arrange
    SystemStateData nightState;
    nightState.activeCameraIsDay = false;
    m_mockStateModel->setMockedData(nightState);
    m_controller->onSystemStateChanged(nightState); // Switch to night camera

    // Act
    m_controller->zoomIn();

    // Assert
    QCOMPARE(m_mockDayControl->zoomInCalled, 0);
    QCOMPARE(m_mockNightControl->setDigitalZoomCalled, 1);
}

void TestCameraController::testStartTracking_DayCameraActive()
{
    // Arrange
    SystemStateData state;
    state.activeCameraIsDay = true;
    state.trackingActive = false;
    m_mockStateModel->setMockedData(state);
    m_controller->onSystemStateChanged(state);

    // Act
    m_controller->startTracking();

    // Assert
    QCOMPARE(m_mockDayProcessor->setTrackingEnabledCalled, 1);
    QVERIFY(m_mockDayProcessor->lastTrackingEnabledState);
    QCOMPARE(m_mockNightProcessor->setTrackingEnabledCalled, 0);
    QCOMPARE(m_mockStateModel->setTrackingStartedCalled, 1);
    QVERIFY(m_mockStateModel->lastTrackingStartedState);
}

void TestCameraController::testStopTracking_DayCameraActive()
{
    // Arrange
    SystemStateData state;
    state.activeCameraIsDay = true;
    state.trackingActive = true; // Tracking is active
    m_mockStateModel->setMockedData(state);
    m_controller->onSystemStateChanged(state);

    // Act
    m_controller->stopTracking();

    // Assert
    QCOMPARE(m_mockDayProcessor->setTrackingEnabledCalled, 1);
    QVERIFY(!m_mockDayProcessor->lastTrackingEnabledState);
    QCOMPARE(m_mockNightProcessor->setTrackingEnabledCalled, 0);
    QCOMPARE(m_mockStateModel->setTrackingStartedCalled, 1);
    QVERIFY(!m_mockStateModel->lastTrackingStartedState);
}

void TestCameraController::testSwitchCamera_StopsTrackingOnInactive()
{
    // Arrange: Start with day camera, tracking active
    SystemStateData dayState;
    dayState.activeCameraIsDay = true;
    dayState.trackingActive = true;
    m_mockStateModel->setMockedData(dayState);
    m_controller->onSystemStateChanged(dayState);

    // Act: Switch to night camera by simulating the model's signal
    SystemStateData nightState = dayState;
    nightState.activeCameraIsDay = false;
    m_controller->onSystemStateChanged(nightState);


    // Assert: Tracking should be stopped on the (now inactive) day processor
    QCOMPARE(m_mockDayProcessor->setTrackingEnabledCalled, 1);
    QVERIFY(!m_mockDayProcessor->lastTrackingEnabledState);
}
