#include "systemcontroller.h"

/* INclude Devices */
#include "../devices/daycameracontroldevice.h"
#include "../devices/cameravideostreamdevice.h"
#include "../devices/imudevice.h"
#include "../devices/joystickdevice.h"
#include "../devices/lensdevice.h"
#include "../devices/lrfdevice.h"
#include "../devices/nightcameracontroldevice.h"
#include "../devices/plc21device.h"
#include "../devices/plc42device.h"
#include "../devices/servoactuatordevice.h"
#include "../devices/servodriverdevice.h"

/* INclude Models */
#include "../models/gyrodatamodel.h"
#include "../models/joystickdatamodel.h"
#include "../models/lensdatamodel.h"
#include "../models/lrfdatamodel.h"
#include "../models/plc21datamodel.h"
#include "../models/plc42datamodel.h"
#include "../models/servoactuatordatamodel.h"
#include "../models/servodriverdatamodel.h"
#include "../models/systemstatemodel.h"

/* INclude Controllers */
#include "../controllers/gimbalcontroller.h"
#include "../controllers/weaponcontroller.h"
#include "../controllers/cameracontroller.h"
#include "../controllers/joystickcontroller.h"

#include "../ui/mainwindow.h"

SystemController::SystemController(QObject *parent)
    : QObject(parent)
{
}

SystemController::~SystemController()
{
    if (m_dayVideoProcessor && m_dayVideoProcessor->isRunning()) m_dayVideoProcessor->stop();
    if (m_nightVideoProcessor && m_nightVideoProcessor->isRunning()) m_nightVideoProcessor->stop();
    bool stopped1 = m_dayVideoProcessor ? m_dayVideoProcessor->wait(2000) : true;
    bool stopped2 = m_nightVideoProcessor ? m_nightVideoProcessor->wait(2000) : true;
    if (!stopped1) qWarning() << "CameraVideoStreamDevice Cam 1 did not stop gracefully.";
    if (!stopped2) qWarning() << "CameraVideoStreamDevice Cam 2 did not stop gracefully.";

    if (m_servoAzThread && m_servoAzThread->isRunning()) {
        m_servoAzThread->quit();
    }
    if (m_servoElThread && m_servoElThread->isRunning()) {
        m_servoElThread->quit();
    }

    // Wait for threads to finish (add reasonable timeout)
    bool azStopped = m_servoAzThread ? m_servoAzThread->wait(1000) : true;
    bool elStopped = m_servoElThread ? m_servoElThread->wait(1000) : true;

    if (!azStopped) qWarning() << "Azimuth Servo Thread did not stop gracefully.";
    if (!elStopped) qWarning() << "Elevation Servo Thread did not stop gracefully.";

    
}

void SystemController::initializeSystem()
{
    // 1) Create devices
    const int sourceWidth = 1280;
    const int sourceHeight = 720;
    const QString dayDevicePath = "/dev/video0";
    const QString nightDevicePath = "/dev/video1";

   m_dayCamControl = new DayCameraControlDevice(this);
    m_gyroDevice = new ImuDevice("/dev/ttyUSB2" , 115200, 1, this);
    m_joystickDevice = new JoystickDevice(this);
    m_lensDevice   = new LensDevice(this);
    m_lrfDevice   = new LRFDevice(this);
    m_nightCamControl = new NightCameraControlDevice(this);
    m_plc21Device = new Plc21Device("/dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BC046FABCD-if00", 115200, 31, QSerialPort::EvenParity, this);
    m_plc42Device = new Plc42Device("/dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BC046FABCD-if02", 115200, 31, QSerialPort::EvenParity, this);

    m_servoActuatorDevice = new ServoActuatorDevice(this);
    m_servoAzThread = new QThread(this);
    m_servoAzDevice = new ServoDriverDevice("az", "/dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BC046FABCD-if04", 230400, 2, QSerialPort::NoParity, nullptr);

    m_servoElThread = new QThread(this); 
    m_servoElDevice = new ServoDriverDevice("el", "/dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BC046FABCD-if06", 230400, 1, QSerialPort::NoParity, nullptr);

    // 2) Create data models
    m_dayCamControlModel    = new DayCameraDataModel(this);
    m_gyroModel             = new GyroDataModel(this);
    m_joystickModel         = new JoystickDataModel(this);
    m_lensModel             = new LensDataModel(this);
    m_lrfModel              = new LrfDataModel(this);
    m_nightCamControlModel  = new NightCameraDataModel(this);
    m_plc21Model            = new Plc21DataModel(this);
    m_plc42Model            = new Plc42DataModel(this);
    m_servoActuatorModel    = new ServoActuatorDataModel(this);
    m_servoAzModel          = new ServoDriverDataModel(this);
    m_servoElModel          = new ServoDriverDataModel(this);

    // 3) Wire devices to their models
    connect(m_dayCamControl,        &DayCameraControlDevice::dayCameraDataChanged,
            m_dayCamControlModel,   &DayCameraDataModel::updateData);

    connect(m_gyroDevice,    &ImuDevice::imuDataChanged,
            m_gyroModel, &GyroDataModel::updateData);

    connect(m_joystickDevice, &JoystickDevice::axisMoved,
            m_joystickModel,  &JoystickDataModel::onRawAxisMoved);

    connect(m_joystickDevice, &JoystickDevice::buttonPressed,
            m_joystickModel,  &JoystickDataModel::onRawButtonChanged);

    connect(m_joystickDevice, &JoystickDevice::hatMoved,
            m_joystickModel,  &JoystickDataModel::onRawHatMoved);

    connect(m_lensDevice,   &LensDevice::lensDataChanged,
            m_lensModel,    &LensDataModel::updateData);

    connect(m_lrfDevice,   &LRFDevice::lrfDataChanged,
            m_lrfModel,    &LrfDataModel::updateData);

    connect(m_nightCamControl,      &NightCameraControlDevice::nightCameraDataChanged,
            m_nightCamControlModel, &NightCameraDataModel::updateData);

    connect(m_plc21Device, &Plc21Device::panelDataChanged,
            m_plc21Model,  &Plc21DataModel::updateData);

    connect(m_plc42Device, &Plc42Device::plc42DataChanged,
            m_plc42Model,  &Plc42DataModel::updateData);

    connect(m_servoActuatorDevice,   &ServoActuatorDevice::actuatorDataChanged,
            m_servoActuatorModel,    &ServoActuatorDataModel::updateData);

    connect(m_servoAzDevice, &ServoDriverDevice::servoDataChanged,
            m_servoAzModel,  &ServoDriverDataModel::updateData);

    connect(m_servoElDevice, &ServoDriverDevice::servoDataChanged,
            m_servoElModel,  &ServoDriverDataModel::updateData);

    // i need to complete other if needed !!!


    // 4) Create m_stateModel
    m_systemStateModel = new SystemStateModel(this);
    m_dayVideoProcessor = new CameraVideoStreamDevice(0, dayDevicePath, sourceWidth, sourceHeight, m_systemStateModel,  nullptr); // index 0 for day
    m_nightVideoProcessor = new CameraVideoStreamDevice(1, nightDevicePath, sourceWidth, sourceHeight, m_systemStateModel, nullptr); // index 1 for night

    // 5) Connect sub-models to m_stateModel
    connect(m_dayCamControlModel, &DayCameraDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onDayCameraDataChanged);

    connect(m_gyroModel,   &GyroDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onGyroDataChanged);

    connect(m_joystickModel, &JoystickDataModel::axisMoved,
            m_systemStateModel, &SystemStateModel::onJoystickAxisChanged);

    connect(m_joystickModel, &JoystickDataModel::buttonPressed,
            m_systemStateModel, &SystemStateModel::onJoystickButtonChanged);
        
    connect(m_joystickModel, &JoystickDataModel::hatMoved,
            m_systemStateModel, &SystemStateModel::onJoystickHatChanged);

    connect(m_lensModel,   &LensDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onLensDataChanged);

    connect(m_lrfModel,   &LrfDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onLrfDataChanged);

    connect(m_nightCamControlModel, &NightCameraDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onNightCameraDataChanged);

    connect(m_plc21Model, &Plc21DataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onPlc21DataChanged);

    connect(m_plc42Model, &Plc42DataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onPlc42DataChanged);

    connect(m_servoActuatorModel,   &ServoActuatorDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onServoActuatorDataChanged);

    connect(m_servoAzModel, &ServoDriverDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onServoAzDataChanged);

    connect(m_servoElModel, &ServoDriverDataModel::dataChanged,
            m_systemStateModel, &SystemStateModel::onServoElDataChanged);

            if (m_systemStateModel && m_dayVideoProcessor) {
                connect(m_systemStateModel, &SystemStateModel::dataChanged,
                        m_dayVideoProcessor, &CameraVideoStreamDevice::onSystemStateChanged,
                        Qt::QueuedConnection); // Queued connection is crucial
            }
             if (m_systemStateModel && m_nightVideoProcessor) {
                connect(m_systemStateModel, &SystemStateModel::dataChanged,
                        m_nightVideoProcessor, &CameraVideoStreamDevice::onSystemStateChanged,
                        Qt::QueuedConnection); // Queued connection is crucial
            }
    //stateMachine->initialize();

    // 6) Create controllers
    m_gimbalController  = new GimbalController(m_servoAzDevice, m_servoElDevice, m_plc42Device, m_systemStateModel, this);
    m_weaponController  = new WeaponController(m_systemStateModel, m_servoActuatorDevice, m_plc42Device, this);
    m_cameraController = new CameraController(m_dayCamControl,
                                              m_dayVideoProcessor,
                                              m_nightCamControl,
                                              m_nightVideoProcessor,
                                              m_lensDevice,
                                              m_systemStateModel);

 

    m_joystickController = new JoystickController(m_joystickModel,
                                                     m_systemStateModel,
                                                     m_gimbalController,
                                                     m_cameraController,
                                                     m_weaponController,
                                                     this);



    // 7) Create

    // Link m_stateModel to pipeline for OSD


    // 8) Start up devices if needed
    m_dayCamControl->openSerialPort("/dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BCD9DCABCD-if00");  //   /dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BCD9DCABCD-if00
    m_gyroDevice->connectDevice();
    //m_lensDevice->openSerialPort("/dev/ttyUSB1");
    m_lrfDevice->openSerialPort("/dev/ttyUSB1");
    m_nightCamControl->openSerialPort("/dev/serial/by-id/usb-1a86_USB_Single_Serial_56D1123075-if00"); //  /dev/serial/by-id/usb-WCH.CN_USB_Quad_Serial_BCD9DCABCD-if02
    m_plc21Device->connectDevice();
    m_plc42Device->connectDevice();
    m_servoActuatorDevice->openSerialPort("/dev/ttyUSB0");
    if (m_servoAzDevice) m_servoAzDevice->connectDevice();
    if (m_servoElDevice) m_servoElDevice->connectDevice();


    //
    m_dayCamControl->zoomOut();
    m_dayCamControl->zoomStop(); // i added this to get initial zoom position and calculate FOV !!!
    m_nightCamControl->setDigitalZoom(0);
    //m_joystickDevice->printJoystickGUIDs();
    if (m_dayVideoProcessor) m_dayVideoProcessor->start();
    if (m_nightVideoProcessor) m_nightVideoProcessor->start();
    m_gimbalController->clearAlarms(); // Clear any existing alarms on startup

}

void SystemController::showMainWindow()
{
    // Optionally create + show main UI
    m_mainWindow = new MainWindow(m_gimbalController,
                                  m_weaponController,
                                  m_cameraController,
                                  m_joystickController,
                                  m_systemStateModel,
                                  m_dayVideoProcessor,
                                  m_nightVideoProcessor);
    //m_mainWindow->show();
    m_mainWindow->showFullScreen();
}

