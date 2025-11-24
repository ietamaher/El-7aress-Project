#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../controllers/gimbalcontroller.h"
#include "../controllers/weaponcontroller.h"
#include "../controllers/cameracontroller.h"
#include "../controllers/joystickcontroller.h"

// Feature ViewModels
#include "../features/models/osdviewmodel.h"
#include "../features/models/menuviewmodel.h"
#include "../features/models/zeroingviewmodel.h"
#include "../features/models/windageviewmodel.h"
#include "../features/models/environmentalviewmodel.h"
#include "../features/models/zonedefinitionviewmodel.h"
#include "../features/models/zonemapviewmodel.h"
#include "../features/models/systemstatusviewmodel.h"
#include "../features/models/aboutviewmodel.h"
#include "../features/models/areazoneparameterviewmodel.h"
#include "../features/models/sectorscanparameterviewmodel.h"
#include "../features/models/trpparameterviewmodel.h"

// Feature Controllers
#include "../features/controllers/applicationcontroller.h"
#include "../features/controllers/mainmenucontroller.h"
#include "../features/controllers/reticlemenucontroller.h"
#include "../features/controllers/colormenucontroller.h"
#include "../features/controllers/osdcontroller.h"
#include "../features/controllers/zeroingcontroller.h"
#include "../features/controllers/windagecontroller.h"
#include "../features/controllers/environmentalcontroller.h"
#include "../features/controllers/zonedefinitioncontroller.h"
#include "../features/controllers/systemstatuscontroller.h"
#include "../features/controllers/aboutcontroller.h"

#include <QDebug> // Example include, add others as needed
#include <QMessageBox>
#include <QDBusInterface>
#include <QDBusReply>
#include <QCoreApplication>
#include <QStatusBar>     // Use status bar


MainWindow::MainWindow(GimbalController *gimbal,
                       WeaponController *weapon,
                       CameraController *camera,
                       JoystickController *joystick,
                       SystemStateModel *stateModel,
                       CameraVideoStreamDevice *dayProcessor,
                       CameraVideoStreamDevice *nightProcessor,
                       QWidget *parent)
    : QMainWindow(parent), // Base class initializer first
    // UI Pointer (initialized by setupUi)
    ui(new Ui::MainWindow),
    // Layouts and Widgets (initialize pointers to nullptr or manage ownership)
    m_layout(nullptr), // Initialize if managed by MainWindow
    m_displayStack(nullptr),
    m_cameraContainer(nullptr),
    m_dayWidgetPlaceholder(nullptr),
    m_nightWidgetPlaceholder(nullptr),
    m_currentDisplayWidget(nullptr),
    // Controllers (store pointers)
    m_gimbalCtrl(gimbal),
    m_weaponCtrl(weapon),
    m_cameraCtrl(camera),
    m_joystickCtrl(joystick),
    // State Management
    m_stateModel(stateModel),
    m_oldState(), // Default construct SystemStateData
    // Video Processing & OSD (using QPointer for safety)
    m_dayProcessor(dayProcessor),
    m_nightProcessor(nightProcessor),
    m_osdRenderer_day(nullptr), // Initialize QPointer to nullptr
    m_osdRenderer_night(nullptr),
    // State Flags (initialize with default values)
    m_isDayCameraActive(true),
    m_activeCameraIndex(0),
    m_menuActive(false),
    m_reticleMenuActive(false),
    m_colorMenuActive(false),
    m_systemStatusActive(false),
    m_settingsMenuActive(false),
    m_aboutActive(false),
    m_trackingActive_cam1(false),
    m_trackingActive_cam2(false),
    m_detectionActive_cam1(false),
    m_detectionActive_cam2(false),
    m_brightnessControlActive(false),
    // Pointers to Managed Widgets (initialize to nullptr)
    m_menuWidget(nullptr),
    m_reticleMenuWidget(nullptr),
    m_colorMenuWidget(nullptr),
    m_systemStatusWidget(nullptr),
    m_zoneDefinitionControllerWidget(nullptr),
    m_zeroingWidget(nullptr),
    m_windageWidget(nullptr),
    m_radarWidget(nullptr),
    m_settingsMenuWidget(nullptr),
    m_aboutWidget(nullptr),
    m_brightnessMenuWidget(nullptr),
    m_brightnessSlider(nullptr),
    m_brightnessLabel(nullptr),
    // Timers & Update Handling
    updateTimer(nullptr), // Initialize timer pointers
    statusTimer(nullptr),
    pendingTrackIds(), // Default construct QSet
    updatePending(false),
    // Brightness Control State
    m_currentBrightness(70), // Default value
    m_displayOutput("DP-0") // Default value
{
    ui->setupUi(this);

    // ========================================================================
    // FEATURE ARCHITECTURE: ViewModels + Controllers
    // ========================================================================

    qDebug() << "[MainWindow] Initializing feature architecture...";

    // Step 1: Create all ViewModels
    m_osdViewModel = new OsdViewModel(this);
    m_mainMenuViewModel = new MenuViewModel(this);
    m_reticleMenuViewModel = new MenuViewModel(this);
    m_colorMenuViewModel = new MenuViewModel(this);
    m_zeroingViewModel = new ZeroingViewModel(this);
    m_windageViewModel = new WindageViewModel(this);
    m_environmentalViewModel = new EnvironmentalViewModel(this);
    m_zoneDefinitionViewModel = new ZoneDefinitionViewModel(this);
    m_zoneMapViewModel = new ZoneMapViewModel(this);
    m_systemStatusViewModel = new SystemStatusViewModel(this);
    m_aboutViewModel = new AboutViewModel(this);
    m_areaZoneParameterViewModel = new AreaZoneParameterViewModel(this);
    m_sectorScanParameterViewModel = new SectorScanParameterViewModel(this);
    m_trpParameterViewModel = new TRPParameterViewModel(this);

    qDebug() << "[MainWindow] ViewModels created (14 total)";

    // Step 2: Create all Controllers
    m_osdController = new OsdController(this);
    m_mainMenuController = new MainMenuController(this);
    m_reticleMenuController = new ReticleMenuController(this);
    m_colorMenuController = new ColorMenuController(this);
    m_zeroingController = new ZeroingController(this);
    m_windageController = new WindageController(this);
    m_environmentalController = new EnvironmentalController(this);
    m_zoneDefinitionController = new ZoneDefinitionController(this);
    m_systemStatusController = new SystemStatusController(this);
    m_aboutController = new AboutController(this);
    m_applicationController = new ApplicationController(this);

    qDebug() << "[MainWindow] Controllers created (11 total)";

    // Step 3: Wire ViewModels to Controllers
    m_osdController->setViewModel(m_osdViewModel);
    m_osdController->setStateModel(m_stateModel);

    m_mainMenuController->setViewModel(m_mainMenuViewModel);
    m_mainMenuController->setStateModel(m_stateModel);

    m_reticleMenuController->setViewModel(m_reticleMenuViewModel);
    m_reticleMenuController->setStateModel(m_stateModel);

    m_colorMenuController->setViewModel(m_colorMenuViewModel);
    m_colorMenuController->setStateModel(m_stateModel);

    m_zeroingController->setViewModel(m_zeroingViewModel);
    m_zeroingController->setStateModel(m_stateModel);

    m_windageController->setViewModel(m_windageViewModel);
    m_windageController->setStateModel(m_stateModel);

    m_environmentalController->setViewModel(m_environmentalViewModel);
    m_environmentalController->setStateModel(m_stateModel);

    m_zoneDefinitionController->setViewModel(m_zoneDefinitionViewModel);
    m_zoneDefinitionController->setMapViewModel(m_zoneMapViewModel);
    m_zoneDefinitionController->setParameterViewModels(m_areaZoneParameterViewModel,
                                                       m_sectorScanParameterViewModel,
                                                       m_trpParameterViewModel);
    m_zoneDefinitionController->setStateModel(m_stateModel);

    m_systemStatusController->setViewModel(m_systemStatusViewModel);
    m_systemStatusController->setStateModel(m_stateModel);

    m_aboutController->setViewModel(m_aboutViewModel);

    qDebug() << "[MainWindow] ViewModels wired to Controllers";

    // Step 4: Wire all controllers to ApplicationController
    m_applicationController->setMainMenuController(m_mainMenuController);
    m_applicationController->setReticleMenuController(m_reticleMenuController);
    m_applicationController->setColorMenuController(m_colorMenuController);
    m_applicationController->setZeroingController(m_zeroingController);
    m_applicationController->setWindageController(m_windageController);
    m_applicationController->setEnvironmentalController(m_environmentalController);
    m_applicationController->setZoneDefinitionController(m_zoneDefinitionController);
    m_applicationController->setSystemStatusController(m_systemStatusController);
    m_applicationController->setAboutController(m_aboutController);
    m_applicationController->setSystemStateModel(m_stateModel);

    qDebug() << "[MainWindow] Controllers wired to ApplicationController";

    // Step 5: Initialize all controllers
    m_osdController->initialize();
    m_mainMenuController->initialize();
    m_reticleMenuController->initialize();
    m_colorMenuController->initialize();
    m_zeroingController->initialize();
    m_windageController->initialize();
    m_environmentalController->initialize();
    m_zoneDefinitionController->initialize();
    m_systemStatusController->initialize();
    m_aboutController->initialize();
    m_applicationController->initialize();

    qDebug() << "[MainWindow] All controllers initialized";

    // Step 6: Setup QML View
    m_videoImageProvider = new VideoImageProvider();
    m_qmlView = new QQuickView();
    m_qmlView->setResizeMode(QQuickView::SizeRootObjectToView);
    m_qmlView->engine()->addImageProvider(QLatin1String("video"), m_videoImageProvider);

    // Step 7: Expose all ViewModels to QML
    QQmlContext *context = m_qmlView->rootContext();
    context->setContextProperty("osdViewModelInstance", m_osdViewModel);
    context->setContextProperty("mainMenuViewModel", m_mainMenuViewModel);
    context->setContextProperty("reticleMenuViewModel", m_reticleMenuViewModel);
    context->setContextProperty("colorMenuViewModel", m_colorMenuViewModel);
    context->setContextProperty("zeroingViewModel", m_zeroingViewModel);
    context->setContextProperty("windageViewModel", m_windageViewModel);
    context->setContextProperty("environmentalViewModel", m_environmentalViewModel);
    context->setContextProperty("zoneDefinitionViewModel", m_zoneDefinitionViewModel);
    context->setContextProperty("zoneMapViewModel", m_zoneMapViewModel);
    context->setContextProperty("systemStatusViewModel", m_systemStatusViewModel);
    context->setContextProperty("aboutViewModel", m_aboutViewModel);
    context->setContextProperty("areaZoneParameterViewModel", m_areaZoneParameterViewModel);
    context->setContextProperty("sectorScanParameterViewModel", m_sectorScanParameterViewModel);
    context->setContextProperty("trpParameterViewModel", m_trpParameterViewModel);

    // Step 8: Expose ApplicationController as appController (for button handlers)
    context->setContextProperty("appController", m_applicationController);

    qDebug() << "[MainWindow] All ViewModels exposed to QML context";


    // Load main QML file
    m_qmlView->setSource(QUrl("qrc:/qml/main.qml"));

    // Check for QML loading errors
    if (m_qmlView->status() == QQuickView::Error) {
        qCritical() << "[MainWindow] QML loading FAILED with errors:";
        for (const QQmlError &error : m_qmlView->errors()) {
            qCritical() << "  " << error.toString();
        }
    } else {
        qDebug() << "[MainWindow] QML loaded successfully - status:" << m_qmlView->status();
    }

    // Embed QML view in Qt Widget UI (if using hybrid approach)
    m_qmlContainer = QWidget::createWindowContainer(m_qmlView, this);
    m_qmlContainer->setMinimumSize(1024, 768);
    m_qmlContainer->setMaximumSize(1024, 768);
    m_qmlContainer->setObjectName("qmlContainer"); // For debugging

    // CRITICAL: Hide old videoLabel if it exists (to prevent blocking QML)
    if (ui->videoLabel) {
        qDebug() << "[MainWindow] Hiding old videoLabel to show QML view";
        ui->videoLabel->hide();
        ui->videoLabel->setVisible(false);
    }

    // SOLUTION: Set parent and explicit geometry instead of relying on layout
    // This ensures the QML container is always visible and properly sized
    if (ui->cameraContainerWidget) {
        qDebug() << "[MainWindow] Setting QML container as child of cameraContainerWidget";
        m_qmlContainer->setParent(ui->cameraContainerWidget);

        // Make container fill the parent widget
        m_qmlContainer->setGeometry(0, 0,
                                  ui->cameraContainerWidget->width(),
                                  ui->cameraContainerWidget->height());

        // Ensure parent widget is also visible
        ui->cameraContainerWidget->show();
    } else {
        // Fallback: attach to main window directly
        qDebug() << "[MainWindow] Attaching QML container to main window (fallback)";
        m_qmlContainer->setParent(this);
        m_qmlContainer->setGeometry(0, 0, 1024, 768);
    }

    // Force QML container to be visible and on top
    m_qmlContainer->show();
    m_qmlContainer->raise();
    m_qmlContainer->setFocus();

    qDebug() << "[MainWindow] QML container created - visible:" << m_qmlContainer->isVisible()
             << "size:" << m_qmlContainer->size()
             << "geometry:" << m_qmlContainer->geometry();
    // --- Create OSD Renderers ---
    // Determine output dimensions (e.g., 960x720 from 4:3 crop)
    // These should ideally come from config or CameraVideoStreamDevice itself
    const int outputWidth = 1024;
    const int outputHeight = 768;
    m_osdRenderer_day = new OsdRenderer(outputWidth, outputHeight, this); // Parent to main window
    m_osdRenderer_night = new OsdRenderer(outputWidth, outputHeight, this);
    // Optionally set initial static text like camera type
    m_osdRenderer_day->updateCameraType("DAY");
    m_osdRenderer_night->updateCameraType("THERMAL");


    // --- Connect Signals ---
    if (m_stateModel) {
        connect(m_stateModel, &SystemStateModel::dataChanged,
                this, &MainWindow::onSystemStateChanged, Qt::QueuedConnection); // Use Queued for safety
        // Set initial state from model
        m_oldState = m_stateModel->data();
        m_activeCameraIndex = m_oldState.activeCameraIsDay ? 0 : 1;
        m_isDayCameraActive = m_oldState.activeCameraIsDay;
    }

    // Connect CameraVideoStreamDevice signals to handleFrameData
    if (m_dayProcessor) {
        connect(m_dayProcessor, &CameraVideoStreamDevice::frameDataReady,
                this, &MainWindow::handleFrameData, Qt::QueuedConnection);

    }
    if (m_nightProcessor) {
        connect(m_nightProcessor, &CameraVideoStreamDevice::frameDataReady,
                this, &MainWindow::handleFrameData, Qt::QueuedConnection);
        /*onnect(m_nightProcessor, &CameraVideoStreamDevice::processingError,
                this, &MainWindow::handleError, Qt::QueuedConnection); // Connect error/status
        connect(m_nightProcessor, &CameraVideoStreamDevice::statusUpdate,
                this, &MainWindow::updateStatus, Qt::QueuedConnection);*/
    }

    connect(m_joystickCtrl, &JoystickController::trackSelectButtonPressed,
    this, &MainWindow::onTrackSelectButtonPressed);

    connect(m_gimbalCtrl, &GimbalController::azAlarmDetected, this, &MainWindow::onAlarmDetected);
    connect(m_gimbalCtrl, &GimbalController::azAlarmCleared, this, &MainWindow::onAlarmCleared);
    connect(m_gimbalCtrl, &GimbalController::elAlarmDetected, this, &MainWindow::onAlarmDetected);
    connect(m_gimbalCtrl, &GimbalController::elAlarmCleared, this, &MainWindow::onAlarmCleared);
    connect(m_cameraCtrl, &CameraController::stateChanged, this, &MainWindow::onCameraStateChanged);

    if (m_cameraCtrl) {
        connect(m_cameraCtrl, &CameraController::statusUpdated,
                this, &MainWindow::onCameraControllerStatus);
    }

    // Ensure the camera container has a predictable size
    ui->cameraContainerWidget->setMinimumSize(640, 480);

    // Temporary placeholders (will be replaced by setupCameraDisplays)
    m_dayWidgetPlaceholder = new QWidget(this);
    m_nightWidgetPlaceholder = new QWidget(this);

    // Set up the camera displays
    setupCameraDisplays();
    // Initially show the day camera
    SystemStateData newData;
    onActiveCameraChanged(newData.activeCameraIsDay);

    // Set up the update timer
    updateTimer = new QTimer(this);
    updateTimer->setInterval(500);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::processPendingUpdates);
    updateTimer->start();

    // Connect tracker signals



}
void MainWindow::onCameraControllerStatus(const QString& message) { //statusBar()->showMessage(message, 4000);
    }
// Helper for initial UI configuration
void MainWindow::initializeUI()
{


    // Set initial UI state based on active camera
    updateUIForActiveCamera();

    // Ensure the video label exists and maybe set a background/placeholder
    if (!ui->videoLabel) {
         qCritical() << "UI setup error: videoLabel is missing!";
         // Handle error - maybe add a label programmatically?
    } else {
         // Optional: Set a placeholder color or text
         ui->videoLabel->setText("Waiting for video signal...");
         ui->videoLabel->setAlignment(Qt::AlignCenter);
         // ui->videoLabel->setStyleSheet("background-color: black; color: gray;");
    }

    // Setup timer (keep if used)
    updateTimer = new QTimer(this);
    updateTimer->setInterval(500);
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::processPendingUpdates);
    updateTimer->start();
}

// *** Core Video Update Slot ***
void MainWindow::handleFrameData(const FrameData &data)
{
    // Only process data for the currently selected camera view
    if (data.cameraIndex != m_activeCameraIndex) {
        return;
    }

    if (data.baseImage.isNull()) {
        return;
    }

    // ========================================================================
    // NEW: UPDATE VIDEO IMAGE PROVIDER (for QML video display)
    // ========================================================================
    if (m_videoImageProvider) {
        m_videoImageProvider->updateFrame(data.baseImage);

        // Force QML Image to refresh (emit signal or use timer in QML)
        // The QML Timer already handles this via Date.now() in the source URL
    }

    // ========================================================================
    // NEW: UPDATE OSD via OsdController (for QML OSD overlay)
    // ========================================================================
    if (m_osdController) {
        m_osdController->onFrameDataReady(data);
    }

    // ========================================================================
    // LEGACY: Keep OsdRenderer for comparison/fallback (OPTIONAL - remove after testing)
    // ========================================================================
    /*

    // Only process data for the currently selected camera view
    if (data.cameraIndex != m_activeCameraIndex) {
        return; // Ignore data from the non-active processor
    }

    // Select the correct OSD renderer
    OsdRenderer *currentRenderer = (data.cameraIndex == 0) ? m_osdRenderer_day : m_osdRenderer_night;
    if (!currentRenderer) {
        // This shouldn't happen if constructor succeeded
        return;
    }
     if (data.baseImage.isNull()) {
        // Handle null image (e.g., show "No Signal" on the label)
        if(ui->videoLabel) ui->videoLabel->setText(QString("No Signal - Cam %1").arg(data.cameraIndex + 1));
        return;
    }

    // --- Update OSD State ---
    // (This part is the same as provided in previous answers)
    currentRenderer->updateMode(data.currentOpMode);
    currentRenderer->updateMotionMode(data.motionMode);
    currentRenderer->updateStabilization(data.stabEnabled);
    currentRenderer->updateLrfDistance(data.lrfDistance);
    currentRenderer->updateSystemStatus(data.sysCharged, data.sysArmed, data.sysReady);
    currentRenderer->updateFiringMode(data.fireMode);
    currentRenderer->updateFov(data.cameraFOV);
    currentRenderer->updateSpeed(data.speed);
    currentRenderer->updateAzimuth(data.azimuth);
    currentRenderer->updateElevation(data.elevation);
    //currentRenderer->updateTrackingState(data.trackingState);

    currentRenderer->updateColorStyle(m_stateModel->data().colorStyle);
    bool showTrackingBox =data.trackerInitialized && (data.trackingState != VPI_TRACKING_STATE_LOST);
    currentRenderer->updateTrackingBox(
        static_cast<float>(data.trackingBbox.x()),
        static_cast<float>(data.trackingBbox.y()),
        static_cast<float>(data.trackingBbox.width()),
        static_cast<float>(data.trackingBbox.height()) );    // data.trackingEnabled && 
    //,        showTrackingBox

    if (data.detectionEnabled) { // Only update if detection was run for this frame
        currentRenderer->updateDetectionBoxes(data.detections);
    } else {
        currentRenderer->updateDetectionBoxes({}); // Send empty vector to clear old boxes
    }
    currentRenderer->updateZeroingDisplay(data.zeroingModeActive,
                                          data.zeroingAppliedToBallistics,
                                          data.zeroingAzimuthOffset,
                                          data.zeroingElevationOffset);
        currentRenderer->updateAppliedZeroingOffsets(data.zeroingAppliedToBallistics,
                                                 data.zeroingAzimuthOffset,
                                                 data.zeroingElevationOffset);                                      
    currentRenderer->updateWindageDisplay(data.windageModeActive,
                                          data.windageAppliedToBallistics,
                                          data.windageSpeedKnots);                                          
     currentRenderer->updateZoneWarning(data.isReticleInNoFireZone, data.gimbalStoppedAtNTZLimit);                                     
    currentRenderer->updateReticleType(m_stateModel->data().reticleType);
    currentRenderer->updateReticlePosition(data.reticleAimpointImageX_px,
                                           data.reticleAimpointImageY_px);
    QRectF acquisitionBox(data.acquisitionBoxX_px, data.acquisitionBoxY_px,
                          data.acquisitionBoxW_px, data.acquisitionBoxH_px);

    // The old `data.trackingBbox` (a QRect) can be used for the tracked box.
    // Let's ensure we use QRectF for consistency.
    QRectF trackedBbox = data.trackingBbox;

    currentRenderer->updateTrackingPhaseDisplay(
        data.currentTrackingPhase,
        data.trackerHasValidTarget,
        acquisitionBox,
        trackedBbox
        );
    // Pass pre-formatted status texts
    currentRenderer->updateLeadStatusText(data.leadStatusText);
    currentRenderer->updateCurrentScanNameDisplay(data.currentScanName);
    /*    // --- Lead Angle Compensation Data ---
    bool leadAngleCompensationActive = false;      // Master switch for LAC feature
    LeadAngleStatus currentLeadAngleStatus = LeadAngleStatus::Off;
    float leadAngleOffsetAz = 0.0f; // Calculated lead offset in Azimuth (degrees/mils)
    float leadAngleOffsetEl = 0.0f; // Calculated lead offset in Elevation (degrees/mils)

    // Inputs for BallisticsProcessor (these might also be part of target data if tracking)
    float currentTargetRange = 0.0f; // From LRF or tracking data
    // Simplified target motion: assume relative angular rates seen by the sensor

    float currentTargetAngularRateEl = 0.0f; // deg/sec or rad/sec
    float muzzleVelocityMPS = 900.0f; // Example muzzle velocity, adjust as needed*/

   /* currentRenderer->updateLeadAngleDisplay(
        m_stateModel->data().leadAngleCompensationActive,
        m_stateModel->data().currentLeadAngleStatus,
        m_stateModel->data().leadAngleOffsetAz,
        m_stateModel->data().leadAngleOffsetEl);

    // --- Render OSD onto the base image ---
    QImage finalImage = currentRenderer->renderOsd(data.baseImage);

    // --- Display the final image in the UI Label ---
    if (!finalImage.isNull() && ui->videoLabel) {
        // Check if label size is valid before scaling
        QSize labelSize = ui->videoLabel->size();
        if (labelSize.isValid() && labelSize.width() > 0 && labelSize.height() > 0) {
             ui->videoLabel->setPixmap(QPixmap::fromImage(finalImage).scaled(
                                      labelSize,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));
        } else {
             // If label size isn't ready yet, set pixmap without scaling
             ui->videoLabel->setPixmap(QPixmap::fromImage(finalImage));
        }
    } else if (ui->videoLabel) {
         ui->videoLabel->setText(QString("Render Error Cam %1").arg(data.cameraIndex + 1));
    }*/
}

// Slot to react to system state changes
void MainWindow::onSystemStateChanged(const SystemStateData &newData)
{
    // --- Handle Camera Switching ---
    // Check if the active camera flag changed in the model
    if (m_oldState.activeCameraIsDay != newData.activeCameraIsDay) {
        qDebug() << "MainWindow: Detected camera switch in state model.";
        // Update local cache and active index
        m_isDayCameraActive = newData.activeCameraIsDay;
        m_activeCameraIndex = m_isDayCameraActive ? 0 : 1;
        updateUIForActiveCamera(); // Update buttons/labels

        // **Important**: Tracking stop on the *old* camera is now handled
        // by CameraController::onSystemStateChanged reacting to this same signal.
        // No need to duplicate the stopTracking invokeMethod call here.
    }

    // --- Handle Tracking State Changes for UI ---
    if (m_oldState.trackingActive != newData.trackingActive) {
         qDebug() << "MainWindow: Detected tracking state change in model to" << newData.trackingActive;
         updateUIForActiveCamera(); // Update tracking button text/status
         // Show/hide track list widget based on tracking state

    }

    // --- Handle Other State Changes (Keep existing logic) ---
    if (!m_oldState.menuUp && newData.menuUp) onUpSwChanged();
    if (!m_oldState.menuDown && newData.menuDown) onDownSwChanged();
    if (!m_oldState.menuVal && newData.menuVal) onMenuValSwChanged();
    if (m_oldState.authorized != newData.authorized) closeAppAndHardware();
    if (m_oldState.upTrackButton != newData.upTrackButton) onUpTrackChanged(newData.upTrackButton);
    if (m_oldState.downTrackButton != newData.downTrackButton) onDownTrackChanged(newData.downTrackButton);
    if (m_oldState.colorStyle != newData.colorStyle) { /* Update UI based on color */ }


    // Check No Fire Zone for current reticle aimpoint
    // Note: Reticle Az/El might be gimbalAz/El + zeroing offsets + ballistic lead.
    // For simplicity, let's use gimbalAz/El as the primary aimpoint for now.
    // You'd ideally use the *actual calculated reticle aimpoint in world coordinates*.
    float aimAz = newData.gimbalAz; // Or newData.reticleAz
    float aimEl = newData.gimbalEl; // Or newData.reticleEl
    float range = newData.lrfDistance > 0 ? newData.lrfDistance : -1.0f; // Use LRF if available

    bool inNFZ = m_stateModel->isPointInNoFireZone(aimAz, aimEl, range);
    if (newData.isReticleInNoFireZone != inNFZ) {
        m_stateModel->setPointInNoFireZone(inNFZ);
        qDebug() << "newData.isReticleInNoFireZone = inNFZ";
    }

    // The gimbalStoppedAtNTZLimit flag would be set by the GimbalController
    // when it actually stops movement due to an NTZ.
    // So, no direct check here, just read it from newData if GimbalController updates it.
    // --- Manage Radar Target List UI ---
    bool shouldShowRadarWidget = (newData.motionMode == MotionMode::RadarSlew);
    bool isRadarWidgetVisible = (m_radarWidget && m_radarWidget->isVisible());

    if (shouldShowRadarWidget && !isRadarWidgetVisible) {
        showRadarTargetWidget(); // Use a helper function
    } else if (!shouldShowRadarWidget && isRadarWidgetVisible) {
        if (m_radarWidget) m_radarWidget->close(); // This will delete it due to WA_DeleteOnClose
    }
    // --- End Radar UI Management ---

    // Update the cached state AFTER comparisons
    m_oldState = newData;
}

// Helper to update UI labels/buttons based on active camera
void MainWindow::updateUIForActiveCamera()
{
    // Use m_activeCameraIndex and m_stateModel->data().trackingActive
    bool trackingState = m_stateModel ? m_stateModel->data().trackingActive : false;
    QString activeCamName = (m_activeCameraIndex == 0) ? "Day" : "Night";

    /*if(ui->toggleTrackingButton) {
        ui->toggleTrackingButton->setText(trackingState ? "Disable Tracking" : "Enable Tracking");
    }*/
    // Update other labels using ui->... (ensure names match .ui file)
    if(ui->trackingStatusLabel) { // Example name
        ui->trackingStatusLabel->setText(QString("Cam %1 Tracking: %2").arg(m_activeCameraIndex+1).arg(trackingState ? "ON" : "OFF"));
    }

     // Update main status bar
    // statusBar()->showMessage(QString("Active: %1 Camera").arg(activeCamName), 3000);
}

/*void MainWindow::switchCameraWidget(QWidget* fromWidget, QWidget* toWidget)
{
    // Get the layout from the camera container widget
    QLayout* layout = ui->cameraContainerWidget->layout();
    if (!layout) {
        qWarning() << "No layout found in cameraContainerWidget!";
        return;
    }

    // Hide the old widget
    if (fromWidget) {
        fromWidget->hide(); // Hide the widget but do not delete it
    }

    // Add the new widget if it's not already in the layout
    if (toWidget && !toWidget->parent()) {
        layout->addWidget(toWidget); // Add the widget to the layout
    }

    // Show and bring the new widget to the front
    if (toWidget) {
        toWidget->show();
        toWidget->raise(); // Bring to front
    }

    // Force layout update (optional)
    layout->update();
}*/

void MainWindow::setupCameraDisplays()
{
    if (!m_cameraCtrl) {
        qWarning() << "Camera controller is null!";
        return;
    }

    // Create the stacked widget
    m_displayStack = new QStackedWidget(this);


    // Clear the current layout of the camera container
    QLayout* oldLayout = ui->cameraContainerWidget->layout();
    if (oldLayout) {
        // Remove all widgets from the old layout
        while (oldLayout->count() > 0) {
            QLayoutItem* item = oldLayout->takeAt(0);
            if (item->widget()) {
                item->widget()->setParent(nullptr);
            }
            delete item;
        }
        delete oldLayout;
    }

    // Create a new layout for the camera container
    QVBoxLayout* newLayout = new QVBoxLayout(ui->cameraContainerWidget);
    newLayout->setContentsMargins(0, 0, 0, 0);
    newLayout->setSpacing(0);
    ui->cameraContainerWidget->setLayout(newLayout);

    // Add the stacked widget to the new layout
    newLayout->addWidget(m_displayStack);


    // Clean up any placeholders
    if (m_dayWidgetPlaceholder) {
        delete m_dayWidgetPlaceholder;
        m_dayWidgetPlaceholder = nullptr;
    }
    if (m_nightWidgetPlaceholder) {
        delete m_nightWidgetPlaceholder;
        m_nightWidgetPlaceholder = nullptr;
    }

    // Connect to camera controller signals for state changes
    connect(m_cameraCtrl, &CameraController::stateChanged,
            this, &MainWindow::onCameraStateChanged, Qt::UniqueConnection);
}

void MainWindow::onCameraStateChanged()
{
    if (!m_cameraCtrl || !m_displayStack) return;

}

void MainWindow::onActiveCameraChanged(bool isDay)
{

}

MainWindow::~MainWindow()
{
    // Stop update timer
    if (updateTimer && updateTimer->isActive()) {
        updateTimer->stop();
    }

    delete ui;
}

void MainWindow::showRadarTargetWidget() {
    if (m_radarWidget) return; // Already showing

    m_radarWidget = new RadarTargetListWidget(m_stateModel, this);
    m_radarWidget->setColorStyleChanged(m_stateModel->data().colorStyle);
    // Connect the widget's destruction to our cleanup slot
    connect(m_radarWidget, &QWidget::destroyed, this, &MainWindow::onModalWidgetClosed);

    /*connect(m_radarWidget, &RadarTargetListWidget::widgetClosed, this, [this](){
        // When radar widget wants to close, revert to a safe mode
        m_radarWidget = nullptr; // It self-deletes
        m_stateModel->setMotionMode(MotionMode::Manual); // Go back to manual
    });*/
    // Position it appropriately on the screen
    m_radarWidget->setGeometry(10, 180, 350, 400); // Example position
    m_radarWidget->show();
}
void MainWindow::onModalWidgetClosed() {
    // The sender() is the widget that was just destroyed
    QObject* destroyedWidget = sender();
    qDebug() << "MainWindow::onModalWidgetClosed - A widget was destroyed.";

    if (destroyedWidget == m_radarWidget) {
        qDebug() << "It was the Radar Widget. Setting pointer to null.";
        m_radarWidget = nullptr; // <<< CRITICAL: NULLIFY THE POINTER
    }
    else if (destroyedWidget == m_menuWidget) {
        qDebug() << "It was the Main Menu Widget. Setting pointer to null.";
        m_menuWidget = nullptr;
    }
    // ... add cases for your other widgets that use this pattern ...
}

//m_cameraCtrl->setActiveCamera(m_isDayCameraActive);
void MainWindow::onUpSwChanged()
{
    // Delegate to ApplicationController (handles all menu/overlay navigation)
    if (m_applicationController) {
        m_applicationController->onUpButtonPressed();
    }

    // Legacy Qt Widget fallback (for widgets not yet integrated with ApplicationController)
    if (m_radarWidget && m_radarWidget->isVisible()) {
        m_radarWidget->moveSelectionUp();
    }
}

void MainWindow::onDownSwChanged()
{
    // Delegate to ApplicationController (handles all menu/overlay navigation)
    if (m_applicationController) {
        m_applicationController->onDownButtonPressed();
    }

    // Legacy Qt Widget fallback (for widgets not yet integrated with ApplicationController)
    if (m_radarWidget && m_radarWidget->isVisible()) {
        m_radarWidget->moveSelectionDown();
    }
}

void MainWindow::onMenuValSwChanged()
{
    // Delegate to ApplicationController (handles all menu/overlay navigation)
    if (m_applicationController) {
        m_applicationController->onMenuValButtonPressed();
    }

    // Legacy Qt Widget fallback (for widgets not yet integrated with ApplicationController)
    if (m_radarWidget && m_radarWidget->isVisible()) {
        m_radarWidget->selectCurrentItem();
    }
}

void MainWindow::showIdleMenu()
{
    if (m_menuActive) return;
    m_menuActive = true;

    QStringList menuOptions;
    menuOptions << "--- RETICLE & DISPLAY ---"
                << "Personalize Reticle"
                << "Personalize Colors"
                << "Adjust Brightness"
                << "--- BALLISTICS ---"
                << "Zeroing"
                << "Clear Active Zero"
                << "Windage"
                << "Clear Active Windage"
                << "--- SYSTEM ---"
                << "Zone Definitions"
                << "System Status"
                << "--- INFO ---"
                << "Help/About"
                << "Return ...";

    // Create menu with title and description
    QString menuTitle = "Main Menu";
    QString menuDescription = "Navigate Through options";

    m_menuWidget = new CustomMenuWidget(menuOptions, m_stateModel, this,
                                        menuTitle, menuDescription);

    // Configure the menu professionally
    m_menuWidget->setColorStyleChanged(m_stateModel->data().colorStyle);
    m_menuWidget->setNavigationHints(true);

    // Optional: Set auto-close after 30 seconds of inactivity
    m_menuWidget->setAutoCloseTimeout(20000); 

    m_menuWidget->resize(400, 550);

    // Connect existing signals (names unchanged)
    connect(m_menuWidget, &CustomMenuWidget::optionSelected,
            this, &MainWindow::handleMenuOptionSelected);
    connect(m_menuWidget, &CustomMenuWidget::menuClosed,
            this, &MainWindow::handleMenuClosed);

    // Optional: Connect to new professional signals for better feedback
    connect(m_menuWidget, &CustomMenuWidget::selectionChanged,
            [this](int index, const QString &text) {
                // Optional: Update status bar or provide audio feedback
                qDebug() << "Menu selection changed to:" << text << "at index" << index;
            });

    connect(m_menuWidget, &CustomMenuWidget::menuAboutToClose,
            [this]() {
                // Optional: Cleanup or save state before menu closes
                qDebug() << "Menu is about to close - performing cleanup";
            });

    m_menuWidget->show();
}

void MainWindow::handleMenuOptionSelected(const QString &option)
{
    if (option == "Return ...") {
        // Close the menu
        if (m_menuWidget) {
            m_menuWidget->close();
        }
    } else if (option == "System Status") {
        showSystemStatus();
    } else if (option == "Personalize Reticle") {
        personalizeReticle();
    } else if (option == "Personalize Colors") {
        personalizeColor();
    } else if (option == "Zeroing") {
        showZeroingWidget();
    } else if (option == "Clear Active Zero") {
        m_stateModel->clearZeroing();
    } else if (option == "Windage") {
        showWindageWidget();
    } else if (option == "Clear Active Windage") {
        m_stateModel->clearWindage();
    } else if (option == "Zone Definitions") { // Or whatever you call it
        showZoneDefinitionScreen();
    } else if (option == "Adjust Brightness") {
        adjustBrightness();
    } else if (option == "Help/About") {
        showHelpAbout();
    }
}


void MainWindow::handleMenuClosed() {
    m_menuActive = false;
    m_menuWidget = nullptr;
}

void MainWindow::showSystemStatus() {

    if (m_systemStatusActive || m_systemStatusWidget) return; // Already showing

    m_systemStatusActive = true;
    m_systemStatusWidget = new SystemStatusWidget(m_stateModel, this); // 'this' for parentage
    m_systemStatusWidget->setColorStyleChanged(m_stateModel->data().colorStyle);
    m_systemStatusWidget->resize(800, 600); // Adjust size as needed

    connect(m_systemStatusWidget, &SystemStatusWidget::menuClosed, this, [this]() {
        m_systemStatusActive = false;
        if (m_systemStatusWidget) {
            m_systemStatusWidget->deleteLater(); // Safe deletion
            m_systemStatusWidget = nullptr;
        }
        showIdleMenu(); // Return to main menu
    });

    connect(m_systemStatusWidget, &SystemStatusWidget::clearAlarmsRequested,
        this, &MainWindow::handleClearAlarmsRequest);

    m_systemStatusWidget->show();
    m_systemStatusWidget->setFocus(); // Give it focus for key events
}

void MainWindow::handleClearAlarmsRequest() {
    qDebug() << "MainWindow: Received clearAlarmsRequested from SystemStatusWidget.";
    m_gimbalCtrl->clearAlarms();
}

void MainWindow::personalizeReticle()
{
    if (m_reticleMenuActive) return;

    m_reticleMenuActive = true;
    
    QStringList reticleOptions;
    for (int i = 0; i < static_cast<int>(ReticleType::COUNT); ++i) {
        reticleOptions << reticleTypeToString(static_cast<ReticleType>(i));
    }
    reticleOptions << "Return ...";

    QString menuTitle = "Personalize Reticle";
    m_reticleMenuWidget = new CustomMenuWidget(reticleOptions, m_stateModel, this, menuTitle);
    m_reticleMenuWidget->setColorStyleChanged(m_stateModel->data().colorStyle);
    m_reticleMenuWidget->resize(350, 350);
    
    // Show the widget first
    m_reticleMenuWidget->show();

    // Then set the current selection using QTimer::singleShot to ensure it's processed after show()
    ReticleType currentReticle = m_stateModel->data().reticleType;
    QString currentReticleName = reticleTypeToString(currentReticle);
    int currentIndex = reticleOptions.indexOf(currentReticleName);

    QTimer::singleShot(0, [this, currentIndex]() {
        if (m_reticleMenuWidget && currentIndex >= 0) {
            m_reticleMenuWidget->setCurrentSelection(currentIndex);
        }
    });
    // Update OSD as user navigates
    connect(m_reticleMenuWidget, &CustomMenuWidget::currentItemChanged, this, [this](const QString &currentItem) {
        if (currentItem != "Return ...") {
            ReticleType previewType = stringToReticleType(currentItem);
            m_stateModel->setReticleStyle(previewType);
            qDebug() << "Previewing reticle type:" << currentItem;
        }
    });

    connect(m_reticleMenuWidget, &CustomMenuWidget::optionSelected, this, [this](const QString &option) {
        if (option == "Return ...") {
            m_reticleMenuWidget->close();
            showIdleMenu();
        } else {
            ReticleType previewType = stringToReticleType(option);

            m_stateModel->setReticleStyle(previewType);
            qDebug() << "Selected reticle type:" << option;
            showIdleMenu();
        }

    });

    connect(m_reticleMenuWidget, &CustomMenuWidget::menuClosed, this, [this]() {
        m_reticleMenuActive = false;
        m_reticleMenuWidget = nullptr;
    });

    //m_reticleMenuWidget->show();
}


inline QString MainWindow::reticleTypeToString(ReticleType type) {
    switch(type) {
        case ReticleType::Basic: return "Basic";
        case ReticleType::BoxCrosshair: return "Box Crosshair";
        case ReticleType::StandardCrosshair: return "Standard Crosshair";
        case ReticleType::PrecisionCrosshair: return "Precision Crosshair";
        case ReticleType::MilDot: return "Mil-Dot";
        default: return "Unknown";
    }
}
// Function to convert string back to enum
inline ReticleType MainWindow::stringToReticleType(const QString& style) {
    if (style == "Basic") return ReticleType::Basic;
    if (style == "Box Crosshair") return ReticleType::BoxCrosshair;
    if (style == "Standard Crosshair") return ReticleType::StandardCrosshair;
    if (style == "Precision Crosshair") return ReticleType::PrecisionCrosshair;
    if (style == "Mil-Dot") return ReticleType::MilDot;
    return ReticleType::Basic; // Default fallback
}
void MainWindow::personalizeColor()
{
    if (m_colorMenuActive) return;

    m_colorMenuActive = true;

    QStringList colorleOptions;
    for (int i = 0; i < static_cast<int>(ColorStyle::COUNT); ++i) {
        colorleOptions << ColorUtils::toString(static_cast<ColorStyle>(i));
    }
    colorleOptions << "Return ...";

    QString menuTitle = "Personalize Color";
    m_colorMenuWidget = new CustomMenuWidget(colorleOptions, m_stateModel, this, menuTitle);
    m_colorMenuWidget->setColorStyleChanged(m_stateModel->data().colorStyle);

    m_colorMenuWidget->resize(350, 250);


    m_colorMenuWidget->show();

    ColorStyle currentColorStyle = m_stateModel->data().osdColorStyle;
    QString currentcolorStyleName = ColorUtils::toString(currentColorStyle);
    int currentIndex = colorleOptions.indexOf(currentcolorStyleName);

    QTimer::singleShot(0, [this, currentIndex]() {
        if (m_colorMenuWidget && currentIndex >= 0) {
            m_colorMenuWidget->setCurrentSelection(currentIndex);
        }
    });
    // Update OSD as user navigates
    connect(m_colorMenuWidget, &CustomMenuWidget::currentItemChanged, this, [this](const QString &currentItem) {
        if (currentItem != "Return ...") {
            m_stateModel->setColorStyle(ColorUtils::toQColor(ColorUtils::fromString(currentItem)));
        }
    });

    connect(m_colorMenuWidget, &CustomMenuWidget::optionSelected, this, [this](const QString &option) {
        if (option == "Return ...") {
            showIdleMenu();
        } else {
            // Set the color style based on the selected option
            m_stateModel->setColorStyle(ColorUtils::toQColor(ColorUtils::fromString(option)));
            qDebug() << "Selected color style:" << option;
            showIdleMenu();
        }
    });

    connect(m_colorMenuWidget, &CustomMenuWidget::menuClosed, this, [this]() {
        m_colorMenuActive = false;
        m_colorMenuWidget = nullptr;
    });

}

inline QString MainWindow::colorStyleToString(ColorStyle style) {
    return ColorUtils::toString(style);
}

 

void MainWindow::showZeroingWidget() {
    if (m_zeroingWidget) { // Already open, maybe bring to front
        m_zeroingWidget->raise();
        m_zeroingWidget->activateWindow();
        return;
    }

    // Create and show the zeroing widget
    m_zeroingWidget = new ZeroingWidget(m_stateModel, this); // Parent to MainWindow
    m_zeroingWidget->setColorStyleChanged(m_stateModel->data().colorStyle);
    connect(m_zeroingWidget, &ZeroingWidget::zeroingProcedureFinished,
            this, &MainWindow::handleZeroingClosed);
    m_zeroingWidget->setAttribute(Qt::WA_DeleteOnClose); // Optional: auto-delete when closed
    m_zeroingWidget->show();
}

void MainWindow::handleZeroingClosed() {
    if (m_zeroingWidget == sender()) { // Ensure it's our widget
        m_zeroingWidget = nullptr; // It will be deleted if WA_DeleteOnClose is set
    }
    showIdleMenu(); // Or whatever is appropriate
}

void MainWindow::showWindageWidget() {
    if (m_windageWidget) { // Already open, maybe bring to front
        m_windageWidget->raise();
        m_windageWidget->activateWindow();
        return;
    }

    // Create and show the windage widget
    m_windageWidget = new WindageWidget(m_stateModel, this); // Parent to MainWindow
    m_windageWidget->setColorStyleChanged(m_stateModel->data().colorStyle);

    connect(m_windageWidget, &WindageWidget::windageProcedureFinished,
            this, &MainWindow::handleWindageClosed);
    m_windageWidget->setAttribute(Qt::WA_DeleteOnClose); // Optional: auto-delete when closed
    m_windageWidget->show();
}

void MainWindow::handleWindageClosed() {
    if (m_windageWidget == sender()) { // Ensure it's our widget
        m_windageWidget = nullptr; // It will be deleted if WA_DeleteOnClose is set
    }
    showIdleMenu(); // Or whatever is appropriate
}

void MainWindow::showZoneDefinitionScreen() {
    if (m_zoneDefinitionControllerWidget) { // Already open, maybe bring to front
        m_zoneDefinitionControllerWidget->raise();
        m_zoneDefinitionControllerWidget->activateWindow();
        return;
    }

    // Assuming m_stateModel is your SystemStateModel instance
    m_zoneDefinitionControllerWidget = new ZoneDefinitionWidget(m_stateModel, this); // Parent to MainWindow
    // Set initial state if needed, or ZoneDefinitionWidget handles it

    // Connect signals for closing, etc.
    connect(m_zoneDefinitionControllerWidget, &ZoneDefinitionWidget::widgetClosed, // Or a custom closed() signal
            this, &MainWindow::handleZoneDefinitionClosed);
    //connect(m_zoneDefinitionControllerWidget, &ZoneDefinitionWidget::requestGimbalMove, // If widget needs to command gimbal
    //        m_gimbalCtrl, &GimbalController::moveToTarget); // Example

    // You might want to make it modal or ensure it's on top
    m_zoneDefinitionControllerWidget->setWindowModality(Qt::ApplicationModal); // Or WindowModal
    m_zoneDefinitionControllerWidget->setAttribute(Qt::WA_DeleteOnClose); // Optional: auto-delete when closed
    m_zoneDefinitionControllerWidget->show();
}

void MainWindow::handleZoneDefinitionClosed() {
 
    if (m_zoneDefinitionControllerWidget == sender()) { // Ensure it's our widget
        m_zoneDefinitionControllerWidget = nullptr; // It will be deleted if WA_DeleteOnClose is set
    }
    showIdleMenu(); // Or whatever is appropriate
}

void MainWindow::adjustBrightness() {
    if (m_brightnessControlActive) return;

    // Create a widget for brightness control
    m_brightnessControlActive = true;
    m_brightnessMenuWidget = new QWidget(this);
    m_brightnessMenuWidget->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    m_brightnessMenuWidget->setStyleSheet("background-color: rgba(30, 30, 30, 200); color: white; border: 1px solid gray;");

    QVBoxLayout* layout = new QVBoxLayout(m_brightnessMenuWidget);

    // Create a title
    QLabel* titleLabel = new QLabel("Adjust Brightness", m_brightnessMenuWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");

    // Create display info label
    QLabel* displayLabel = new QLabel(QString("Display: %1").arg(m_displayOutput), m_brightnessMenuWidget);
    displayLabel->setAlignment(Qt::AlignCenter);

    // Load current brightness
    detectDisplays();

    // Create brightness slider
    m_brightnessSlider = new QSlider(Qt::Horizontal, m_brightnessMenuWidget);
    m_brightnessSlider->setRange(10, 100); // Avoid completely dark screen
    m_brightnessSlider->setValue(m_currentBrightness);
    m_brightnessSlider->setEnabled(false); // We'll use UP/DOWN buttons instead

    // Create brightness value label
    m_brightnessLabel = new QLabel(QString("Brightness: %1%").arg(m_currentBrightness), m_brightnessMenuWidget);
    m_brightnessLabel->setAlignment(Qt::AlignCenter);

    // Instructions
    QLabel* instructionsLabel = new QLabel("Use UP/DOWN buttons to adjust brightness\nPress MENU to return", m_brightnessMenuWidget);
    instructionsLabel->setAlignment(Qt::AlignCenter);

    // Add widgets to layout
    layout->addWidget(titleLabel);
    layout->addWidget(displayLabel);
    layout->addWidget(m_brightnessLabel);
    layout->addWidget(m_brightnessSlider);
    layout->addWidget(instructionsLabel);

    // Set layout
    m_brightnessMenuWidget->setLayout(layout);

    // Size and position
    m_brightnessMenuWidget->resize(300, 200);
    m_brightnessMenuWidget->move((this->width() - m_brightnessMenuWidget->width()) / 2,
                            (this->height() - m_brightnessMenuWidget->height()) / 2);

    // Show the widget
    m_brightnessMenuWidget->show();
}

bool MainWindow::detectDisplays() {
    // Use xrandr to detect active displays
    QProcess process;
    process.start("sh", QStringList() << "-c" << "xrandr | grep ' connected'");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();

    if (output.isEmpty()) {
        // Fallback to default DP-0
        m_displayOutput = "DP-0";
        return false;
    }

    // Parse output to find DisplayPort output
    QStringList lines = output.split("\n", Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString display = line.split(" ").first();
        if (display.startsWith("DP-")) {
            m_displayOutput = display;
            return true;
        }
    }

    // If no DP output found, use the first connected display
    if (!lines.isEmpty()) {
        m_displayOutput = lines.first().split(" ").first();
        return true;
    }

    return false;
}

void MainWindow::setBrightness(int percentage) {
    // Ensure value is within range
    if (percentage < 10) percentage = 10; // Never go below 10%
    if (percentage > 100) percentage = 100;

    m_currentBrightness = percentage;

    // Update UI
    if (m_brightnessLabel) {
        m_brightnessLabel->setText(QString("Brightness: %1%").arg(percentage));
    }

    if (m_brightnessSlider) {
        m_brightnessSlider->setValue(percentage);
    }

    // Convert percentage to decimal (xrandr uses 0.0-1.0)
    double brightnessValue = percentage / 100.0;

    // Set brightness using xrandr
    QProcess process;
    process.start("xrandr", QStringList() << "--output" << m_displayOutput
                 << "--brightness" << QString::number(brightnessValue, 'f', 2));
    process.waitForFinished();
}


void MainWindow::increaseBrightness() {
    setBrightness(m_currentBrightness + 10);
}

void MainWindow::decreaseBrightness() {
    setBrightness(m_currentBrightness - 10);
}


void MainWindow::showHelpAbout() {
    // --- Application Info ---
    QString appName = "El 7arress RCWS"; // Or QCoreApplication::applicationName() if set
    //QString appVersion = "2025 - Version 4.5"; // Your specific versioning
    // You can also use QCoreApplication::applicationVersion() if you set it via a .rc file or in main.cpp
    QString appVersion = QCoreApplication::applicationVersion();
    if (appVersion.isEmpty()) {
         appVersion = "4.5 (Dev Build)"; // Fallback
     }

    // --- Build Information (Optional) ---
    // QString buildDateTime = QStringLiteral(__DATE__) + " " + QStringLiteral(__TIME__);
    // QString qtVersion = qVersion(); // Gets Qt version used to compile
    // QString compilerInfo;
    // #if defined(Q_CC_GNU)
    //     compilerInfo = QString("GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
    // #elif defined(Q_CC_CLANG)
    //     compilerInfo = QString("Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
    // #elif defined(Q_CC_MSVC)
    //     compilerInfo = QString("MSVC %1").arg(_MSC_VER);
    // #else
    //     compilerInfo = "Unknown Compiler";
    // #endif
    // QString buildAbi = QSysInfo::buildAbi();


    // --- Credits ---
    QString credits = "<b>Lead Developer:</b> Captain Maher BOUZAIEN\n"
                       "<b>Special Thanks:</b> EMAM, CRM\n";

    // --- Copyright & Licensing ---
    QString copyrightNotice = QString("Copyright © 2022-%1 Tunisian Ministry of Defense. All rights reserved.")
                                .arg(QDate::currentDate().year());
    QString licenseInfo = "This software is proprietary and confidential.\n"
                          "Unauthorized copying, distribution, or use is strictly prohibited.";
    // Or, if you have an open-source component or a specific license:
    // QString licensePath = QStandardPaths::locate(QStandardPaths::AppDataLocation, "LICENSE.txt", QStandardPaths::LocateFile);
    // if (!licensePath.isEmpty()) {
    //     QFile licenseFile(licensePath);
    //     if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    //         licenseInfo = "License Information:\n" + QString::fromUtf8(licenseFile.readAll());
    //         licenseFile.close();
    //     }
    // }


    // --- Constructing the "About" Text with HTML for basic formatting ---
    QString aboutText = QString(
        "<h2>%1</h2>"
        "<p><b>Version:</b> %2</p>"
        "<hr>"
        "<h4>Credits:</h4>"
        "<p>%3</p>"
        "<hr>"
        "<p>%4</p>"
        "<p><small>%5</small></p>"
         /*"<hr>"
         "<h4>Build Information:</h4>"
         "<p><small>Built: %6<br>"
         "Qt Version: %7<br>"
         "Compiler: %8<br>"
         "ABI: %9</small></p>"*/
    )
    .arg(appName)
    .arg(appVersion)
    .arg(credits.replace("\n", "<br>")) // Convert newlines to <br> for HTML
    .arg(copyrightNotice)
    .arg(licenseInfo.replace("\n", "<br>"));
    /*.arg(buildDateTime)
     .arg(qtVersion)
     .arg(compilerInfo)
     .arg(buildAbi);*/


    // --- Displaying the MessageBox ---
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle(QString("About %1").arg(appName));
    // aboutBox.setIconPixmap(QPixmap(":/icons/app_icon.png").scaled(64,64, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // Optional: Add an icon
    aboutBox.setTextFormat(Qt::RichText); // Enable HTML formatting
    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}



void MainWindow::closeAppAndHardware() {
    // 1. Perform any necessary cleanup first (e.g., closing hardware, saving state, etc.)
    // cleanupHardware(); // (Your cleanup code here)

    // 2. Create a DBus interface to request a system shutdown.
    QDBusInterface iface("org.freedesktop.login1",
                         "/org/freedesktop/login1",
                         "org.freedesktop.login1.Manager",
                         QDBusConnection::systemBus());

    if (!iface.isValid()) {
        qWarning() << "DBus interface for login1 is invalid:"
                   << QDBusConnection::systemBus().lastError().message();
    } else {
        // Declare the reply variable in this scope
        //QDBusReply<void> reply = iface.call("PowerOff", true);
        //if (!reply.isValid()) {
        //    qWarning() << "DBus call to PowerOff failed:" << reply.error().message();
        //}
    }

    // 3. Quit the Qt application.
    //qApp->quit();
}


void MainWindow::onSelectedTrackLost(int trackId)
{

}


void MainWindow::onTrackedIdsUpdated(const QSet<int>& trackIds)
{
}

void MainWindow::processPendingUpdates()
{

}

void MainWindow::onTrackIdSelected(QListWidgetItem* current, QListWidgetItem* previous)
{

}

int MainWindow::findItemIndexByData(QListWidget* listWidget, int data) const
{

}

void MainWindow::setTracklistColorStyle(const QString &style)
{

}

void MainWindow::onUpTrackChanged(bool state)
{

}

void MainWindow::onDownTrackChanged(bool state)
{

}

void MainWindow::onTrackSelectButtonPressed()
{
    bool* activeTrackingFlagPtr = (m_activeCameraIndex == 0) ? &m_trackingActive_cam1 : &m_trackingActive_cam2;
    CameraVideoStreamDevice* activeProcessor = (m_activeCameraIndex == 0) ? m_dayProcessor : m_nightProcessor;
    if (!activeProcessor) { qWarning() << "Cannot toggle tracking: Active processor is null."; return; }

    *activeTrackingFlagPtr = !(*activeTrackingFlagPtr);
    bool newState = *activeTrackingFlagPtr;
    qInfo() << "Cam" << m_activeCameraIndex << ":" << (newState ? "Requesting Enable Tracking" : "Requesting Disable Tracking");
    QMetaObject::invokeMethod(activeProcessor, "setTrackingEnabled", Qt::QueuedConnection, Q_ARG(bool, newState));
    updateUIForActiveCamera();
}

// **************  TEsting tools ********************

void MainWindow::on_opmode_clicked()
{
    m_stateModel->setOpMode(OperationalMode::Surveillance);
}


void MainWindow::on_fireOn_clicked()
{
    m_weaponCtrl->startFiring();
}


void MainWindow::on_fieOff_clicked()
{
    m_weaponCtrl->stopFiring();
}


void MainWindow::on_mode_clicked()
{
    /*if (m_stateModel->data().opMode == OperationalMode::Tracking) {
        m_stateModel->setOpMode(OperationalMode::Surveillance);
        m_stateModel->setMotionMode(MotionMode::Manual);
    }
    else {
        m_stateModel->setOpMode(OperationalMode::Tracking);
        m_stateModel->setMotionMode( MotionMode::AutoTrack );
    }*/

  

}


void MainWindow::on_track_clicked()
{
    if (m_stateModel->data().motionMode == MotionMode::ManualTrack) {
        m_cameraCtrl->startTracking();
        /*if (!m_stateModel->data().startTracking) {
            m_stateModel->setTrackingStarted(true);
            qDebug() << "Joystick pressed: starting tracking.";
        } else {
            // Toggle restart flag to force state update.
            m_stateModel->setTrackingRestartRequested(false);
            m_stateModel->setTrackingRestartRequested(true);
            qDebug() << "Joystick pressed: tracking restart requested.";
        }*/
    }
}


void MainWindow::on_motion_clicked()
{

    OperationalMode opMode = m_stateModel->data().opMode;
    MotionMode motionMode = m_stateModel->data().motionMode;

    if (opMode == OperationalMode::Surveillance) {
        // cycle between Manual and Pattern
        if (motionMode == MotionMode::Manual) {
            m_stateModel->setMotionMode(MotionMode::Pattern);
        } else {
            m_stateModel->setMotionMode(MotionMode::Manual);
        }

    } else if (opMode == OperationalMode::Tracking) {
        MotionMode nextMode = MotionMode::ManualTrack;
        // Only do AutoTrack if day camera
        if (m_stateModel->data().activeCameraIsDay) {
            nextMode = (motionMode == MotionMode::AutoTrack)
                ? MotionMode::ManualTrack
                : MotionMode::AutoTrack;
        }
        m_stateModel->setMotionMode(nextMode);

    }



}




/*void MainWindow::on_up_clicked()
{
    if (m_stateModel->data().opMode  == OperationalMode::Idle) {
        m_stateModel->setUpSw(true);
    } else if (m_stateModel->data().opMode  == OperationalMode::Tracking) {
        m_stateModel->setUpTrack(true);
    }
}*/


void MainWindow::on_down_clicked()
{
    if (m_stateModel->data().opMode  == OperationalMode::Idle) {
        m_stateModel->setDownSw(!m_stateModel->data().downTrackButton);
    } else if (m_stateModel->data().opMode  == OperationalMode::Tracking) {
        m_stateModel->setDownTrack(false);
        m_stateModel->setDownTrack(true);
    }
}


void MainWindow::on_autotrack_clicked()
{
    m_cameraCtrl->startTracking();

}




void MainWindow::on_day_clicked()
{
    // Let the state model handle the camera switching logic
    if (m_stateModel) {
        m_stateModel->setActiveCameraIsDay(!m_isDayCameraActive);
    }
}


void MainWindow::on_night_clicked()
{
    m_stateModel->setActiveCameraIsDay(false);

}

void MainWindow::on_quit_clicked()
{
    QCoreApplication::quit();

}

void MainWindow::onAlarmDetected(uint16_t alarmCode, const QString &description)
{
    qDebug() << "Alarm detected: " << alarmCode << description;
    // Update UI with alarm information
    // e.g. show alarm code and description in a dialog
    // QMessageBox::warning(this, "Alarm Detected", QString("Alarm %1: %2").arg(alarmCode).arg(description));
}

void MainWindow::onAlarmCleared()
{
    qDebug() << "Alarm cleared.";
}

void MainWindow::onAlarmHistoryRead(const QList<uint16_t> &alarmHistory)
{
}

void MainWindow::onAlarmHistoryCleared()
{
}

void MainWindow::on_read_clicked()
{
    m_gimbalCtrl->readAlarms();
}


void MainWindow::on_clear_clicked()
{
    m_gimbalCtrl->clearAlarms();
}



void MainWindow::on_toggleTrackingButton_clicked()
{
    bool* activeTrackingFlagPtr = (m_activeCameraIndex == 0) ? &m_trackingActive_cam1 : &m_trackingActive_cam2;
    CameraVideoStreamDevice* activeProcessor = (m_activeCameraIndex == 0) ? m_dayProcessor : m_nightProcessor;
    if (!activeProcessor) { qWarning() << "Cannot toggle tracking: Active processor is null."; return; }

    *activeTrackingFlagPtr = !(*activeTrackingFlagPtr);
    bool newState = *activeTrackingFlagPtr;
    qInfo() << "Cam" << m_activeCameraIndex << ":" << (newState ? "Requesting Enable Tracking" : "Requesting Disable Tracking");
    QMetaObject::invokeMethod(activeProcessor, "setTrackingEnabled", Qt::QueuedConnection, Q_ARG(bool, newState));
    updateUIForActiveCamera();
}


void MainWindow::on_detection_clicked()
{
    bool* activeDetectionFlagPtr = (m_activeCameraIndex == 0) ? &m_detectionActive_cam1 : &m_detectionActive_cam2;
    CameraVideoStreamDevice* activeProcessor = (m_activeCameraIndex == 0) ? m_dayProcessor : m_nightProcessor;
    if (!activeProcessor) { qWarning() << "Cannot toggle detection: Active processor is null."; return; }

    *activeDetectionFlagPtr = !(*activeDetectionFlagPtr);
    bool newState = *activeDetectionFlagPtr;
    qInfo() << "Cam" << m_activeCameraIndex << ":" << (newState ? "Requesting Enable Detection" : "Requesting Disable Dtection");
    QMetaObject::invokeMethod(activeProcessor, "setDetectionEnabled", Qt::QueuedConnection, Q_ARG(bool, newState));
    updateUIForActiveCamera();
}


void MainWindow::on_zoneDefinition_clicked()
{
    showZoneDefinitionScreen();

}


void MainWindow::on_dw_clicked()
{
    onDownSwChanged();
}


void MainWindow::on_up_clicked()
{
    onUpSwChanged();
}


void MainWindow::on_val_clicked()
{
    onMenuValSwChanged();
}


void MainWindow::on_LeadAngle_clicked()
{
    if (!m_stateModel) return;

    // PDF: "Hold the Palm Switch (2) and press the LEAD button (1)"
    // For simulation, a single toggle might be easier to implement from UI.
    // Let's assume this toggles the LAC master state.
    bool currentLACState = m_stateModel->data().leadAngleCompensationActive;
    m_stateModel->setLeadAngleCompensationActive(!currentLACState);

    if (m_weaponCtrl) {
        m_weaponCtrl->updateFireControlSolution();
    }
}


void MainWindow::on_pushButton_clicked()
{
    SystemStateData curr = m_stateModel->data();
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool isDoubleClick = (now - m_lastTrackButtonPressTime) < DOUBLE_CLICK_INTERVAL_MS;
    qDebug() << "Joystick: TRACK button pressed. Double-click detected:" << now - m_lastTrackButtonPressTime << "ms";
    m_lastTrackButtonPressTime = now;

    TrackingPhase currentPhase = curr.currentTrackingPhase;

    if (isDoubleClick) {
        qDebug() << "Joystick: TRACK button double-clicked. Aborting tracking.";
        m_stateModel->stopTracking(); // This should set phase to Off
        return;
    }

    // --- Single Press Logic ---
    switch (currentPhase) {
    case TrackingPhase::Off:
        // First press: Enter Acquisition mode
        qDebug() << "Joystick: TRACK button pressed. Entering Acquisition Phase.";
        m_stateModel->startTrackingAcquisition(); // This sets phase to Acquisition
        break;

    case TrackingPhase::Acquisition:
        // Second press: Request lock-on with the current acquisition gate
        qDebug() << "Joystick: TRACK button pressed. Requesting Tracker Lock-On.";
        m_stateModel->requestTrackerLockOn(); // This sets phase to Tracking_LockPending
        break;

    case TrackingPhase::Tracking_LockPending:
    case TrackingPhase::Tracking_ActiveLock:
    case TrackingPhase::Tracking_Coast:
    case TrackingPhase::Tracking_Firing:
        // A single press while in any active tracking phase might do nothing,
        // or it could be used to cycle targets if your tracker supports it.
        // For now, only a double-click will cancel.
        qDebug() << "Joystick: TRACK button pressed, but already in an active tracking phase. Double-click to cancel.";
        break;
    }
}

// ========================================================================
// RESIZE EVENT - Handle window/widget resize for QML container
// ========================================================================
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event); // Call base class implementation

    // Resize QML container to fill parent widget when window resizes
    if (m_qmlContainer && ui->cameraContainerWidget) {
        m_qmlContainer->setGeometry(0, 0,
                                    ui->cameraContainerWidget->width(),
                                    ui->cameraContainerWidget->height());
    }
}

