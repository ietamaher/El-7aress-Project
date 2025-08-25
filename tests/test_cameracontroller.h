#ifndef TEST_CAMERACONTROLLER_H
#define TEST_CAMERACONTROLLER_H

#include <QObject>

// Forward declarations of mock objects and the class under test
class CameraController;
class MockDayCameraControlDevice;
class MockNightCameraControlDevice;
class MockCameraVideoStreamDevice;
class MockLensDevice;
class MockSystemStateModel;

class TestCameraController : public QObject
{
    Q_OBJECT

public:
    TestCameraController();
    ~TestCameraController();

private slots:
    // Test lifecycle functions
    void init();
    void cleanup();

    // Test cases
    void testConstruction();
    void testZoomIn_DayCameraActive();
    void testZoomIn_NightCameraActive();
    void testStartTracking_DayCameraActive();
    void testStopTracking_DayCameraActive();
    void testSwitchCamera_StopsTrackingOnInactive();

private:
    // Mocks
    MockDayCameraControlDevice* m_mockDayControl;
    MockNightCameraControlDevice* m_mockNightControl;
    MockCameraVideoStreamDevice* m_mockDayProcessor;
    MockCameraVideoStreamDevice* m_mockNightProcessor;
    MockLensDevice* m_mockLensDevice;
    MockSystemStateModel* m_mockStateModel;

    // Class under test
    CameraController* m_controller;
};

#endif // TEST_CAMERACONTROLLER_H
