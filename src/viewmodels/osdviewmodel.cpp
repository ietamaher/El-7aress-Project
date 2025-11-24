#include "osdviewmodel.h"
#include "../devices/cameravideostreamdevice.h" // For FrameData
#include "../utils/inference.h" // For YoloDetection
#include "../models/systemstatemodel.h" // For enums

#include <QDebug>
#include <QVariantMap>

OsdViewModel::OsdViewModel(QObject *parent)
    : QObject(parent)
    , m_accentColor(QColor("#46E2A5")) // Default green
    , m_modeText("MODE: IDLE")
    , m_motionText("MOTION: MAN")
    , m_rateText("RATE: SINGLE SHOT")
    , m_stabText("STAB: OFF")
    , m_cameraText("CAM: DAY")
    , m_fovText("FOV: 45.0°")
    , m_lrfText(" -- m")
    , m_statusText("SYS: --- SAF NRD")
    , m_speedText("0.0%")
    , m_zeroingVisible(false)
    , m_zeroingText("Z")
    , m_windageVisible(false)
    , m_windageText("W")
    , m_environmentVisible(false)
    , m_environmentText("ENV")
    , m_detectionVisible(false)
    , m_detectionText("DETECTION: OFF")
    , m_scanNameVisible(false)
    , m_scanNameText("")
    , m_leadAngleVisible(false)
    , m_leadAngleText("")
    , m_lacActive(false)
    , m_leadAngleOffsetAz(0.0f)
    , m_leadAngleOffsetEl(0.0f)
    , m_azimuth(0.0f)
    , m_elevation(0.0f)
    , m_vehicleHeading(0.0f)
    , m_imuConnected(false)
    , m_imuRollDeg(0.0)
    , m_imuPitchDeg(0.0)
    , m_imuYawDeg(0.0)
    , m_imuTemp(0.0)
    , m_gyroX(0.0)
    , m_gyroY(0.0)
    , m_gyroZ(0.0)
    , m_accelX(0.0)
    , m_accelY(0.0)
    , m_accelZ(0.0)
    , m_reticleType(1) // BoxCrosshair
    , m_reticleOffsetX(0.0f)
    , m_reticleOffsetY(0.0f)
    , m_currentFov(45.0f)
    , m_ccipVisible(false)
    , m_ccipX(512.0f)
    , m_ccipY(384.0f)
    , m_ccipStatus("Off")
    , m_trackingActive(false)
    , m_trackingBoxVisible(false)
    , m_trackingBox(QRectF())
    , m_trackingBoxColor("yellow")
    , m_trackingBoxDashed(false)
    , m_trackingConfidence(0.0f)
    , m_acquisitionBoxVisible(false)
    , m_acquisitionBox(QRectF())
    , m_rangeMeters(0.0f)
    , m_zoneWarningVisible(false)
    , m_zoneWarningText("")
    , m_startupMessageVisible(false)
    , m_startupMessageText("")
    , m_errorMessageVisible(false)
    , m_errorMessageText("")
    , m_ammunitionLevel(true)
    , m_dayCameraConnected(true)
    , m_dayCameraError(false)
    , m_nightCameraConnected(true)
    , m_nightCameraError(false)
    , m_azServoConnected(true)
    , m_azFault(false)
    , m_elServoConnected(true)
    , m_elFault(false)
    , m_lrfConnected(true)
    , m_lrfFault(false)
    , m_lrfOverTemp(false)
    , m_actuatorConnected(true)
    , m_actuatorFault(false)
    , m_plc21Connected(true)
    , m_plc42Connected(true)
    , m_joystickConnected(true)
{
}

void OsdViewModel::updateFromFrameData(const FrameData &data)
{
    // ========================================================================
    // VISUAL STYLING - Update from FrameData
    // ========================================================================
    if (m_accentColor != data.colorStyle) {
        m_accentColor = data.colorStyle;
        emit accentColorChanged();
    }

    // ========================================================================
    // MODE AND MOTION
    // ========================================================================
    QString newModeText = "MODE: " + formatMode(static_cast<int>(data.currentOpMode));
    if (m_modeText != newModeText) {
        m_modeText = newModeText;
        emit modeTextChanged();
    }

    QString newMotionText = "MOTION: " + formatMotionMode(static_cast<int>(data.motionMode));
    if (m_motionText != newMotionText) {
        m_motionText = newMotionText;
        emit motionTextChanged();
    }

    // ========================================================================
    // FIRE MODE
    // ========================================================================
    QString newRateText = "RATE: " + formatFireMode(static_cast<int>(data.fireMode));
    if (m_rateText != newRateText) {
        m_rateText = newRateText;
        emit rateTextChanged();
    }

    // ========================================================================
    // STABILIZATION
    // ========================================================================
    QString newStabText = data.stabEnabled ? "STAB: ON" : "STAB: OFF";
    if (m_stabText != newStabText) {
        m_stabText = newStabText;
        emit stabTextChanged();
    }

    // ========================================================================
    // CAMERA TYPE
    // ========================================================================
    QString newCameraText = "CAM: " + QString(data.cameraIndex == 0 ? "DAY" : "THERMAL");
    if (m_cameraText != newCameraText) {
        m_cameraText = newCameraText;
        emit cameraTextChanged();
    }

    // ========================================================================
    // FOV
    // ========================================================================
    QString newFovText = QString("FOV: %1°").arg(data.cameraFOV, 0, 'f', 1);
    if (m_fovText != newFovText) {
        m_fovText = newFovText;
        emit fovTextChanged();
    }

    if (m_currentFov != data.cameraFOV) {
        m_currentFov = data.cameraFOV;
        emit currentFovChanged();
    }

    // ========================================================================
    // LRF DISTANCE
    // ========================================================================
    QString newLrfText = (data.lrfDistance > 0)
                             ? QString("%1 m").arg(static_cast<int>(data.lrfDistance))
                             : " -- m";
    if (m_lrfText != newLrfText) {
        m_lrfText = newLrfText;
        emit lrfTextChanged();
    }

    if (m_rangeMeters != data.lrfDistance) {
        m_rangeMeters = data.lrfDistance;
        emit rangeMetersChanged();
    }

    // ========================================================================
    // SYSTEM STATUS
    // ========================================================================
    QString chargedStr = data.sysCharged ? "CHG" : "---";
    QString armedStr = data.gunArmed ? "ARM" : "SAF";
    QString readyStr = data.sysReady ? "RDY" : "NRD";
    QString newStatusText = QString("SYS: %1 %2 %3").arg(chargedStr, armedStr, readyStr);
    if (m_statusText != newStatusText) {
        m_statusText = newStatusText;
        emit statusTextChanged();
    }

    // ========================================================================
    // AMMUNITION LEVEL
    // ========================================================================
    if (m_ammunitionLevel != data.stationAmmunitionLevel) {
        m_ammunitionLevel = data.stationAmmunitionLevel;
        emit ammunitionLevelChanged();
    }

    // ========================================================================
    // SPEED
    // ========================================================================
    QString newSpeedText = QString("%1%").arg(data.speed, 0, 'f', 1);
    if (m_speedText != newSpeedText) {
        m_speedText = newSpeedText;
        emit speedTextChanged();
    }

    // ========================================================================
    // AZIMUTH & ELEVATION
    // ========================================================================
    if (m_azimuth != data.azimuth) {
        m_azimuth = data.azimuth;
        emit azimuthChanged();
    }

    if (m_elevation != data.elevation) {
        m_elevation = data.elevation;
        emit elevationChanged();
    }

    // ========================================================================
    // IMU SENSOR DATA
    // ========================================================================
    bool imuDataChange = false;

    if (m_imuConnected != data.imuConnected) {
        m_imuConnected = data.imuConnected;
        emit imuConnectedChanged();
        imuDataChange = true;
    }

    if (data.imuConnected) {
        if (m_imuRollDeg != data.imuRollDeg) {
            m_imuRollDeg = data.imuRollDeg;
            imuDataChange = true;
        }
        if (m_imuPitchDeg != data.imuPitchDeg) {
            m_imuPitchDeg = data.imuPitchDeg;
            imuDataChange = true;
        }
        if (m_imuYawDeg != data.imuYawDeg) {
            m_imuYawDeg = data.imuYawDeg;
            imuDataChange = true;
        }
        if (m_imuTemp != data.imuTemp) {
            m_imuTemp = data.imuTemp;
            imuDataChange = true;
        }
        if (m_gyroX != data.gyroX) {
            m_gyroX = data.gyroX;
            imuDataChange = true;
        }
        if (m_gyroY != data.gyroY) {
            m_gyroY = data.gyroY;
            imuDataChange = true;
        }
        if (m_gyroZ != data.gyroZ) {
            m_gyroZ = data.gyroZ;
            imuDataChange = true;
        }
        if (m_accelX != data.accelX) {
            m_accelX = data.accelX;
            imuDataChange = true;
        }
        if (m_accelY != data.accelY) {
            m_accelY = data.accelY;
            imuDataChange = true;
        }
        if (m_accelZ != data.accelZ) {
            m_accelZ = data.accelZ;
            imuDataChange = true;
        }
    }

    if (imuDataChange) {
        emit imuDataChanged();
    }

    // ========================================================================
    // ZEROING PROCEDURE
    // ========================================================================
    bool newZeroingVisible = data.zeroingModeActive || data.zeroingAppliedToBallistics;
    if (m_zeroingVisible != newZeroingVisible) {
        m_zeroingVisible = newZeroingVisible;
        emit zeroingVisibleChanged();
    }

    if (newZeroingVisible) {
        QString newZeroingText;
        if (data.zeroingModeActive) {
            newZeroingText = "ZEROING ACTIVE";
        } else if (data.zeroingAppliedToBallistics) {
            newZeroingText = QString("Z: Az%1° El%2°")
                                 .arg(data.zeroingAzimuthOffset, 0, 'f', 2)
                                 .arg(data.zeroingElevationOffset, 0, 'f', 2);
        }
        if (m_zeroingText != newZeroingText) {
            m_zeroingText = newZeroingText;
            emit zeroingTextChanged();
        }
    }

    // ========================================================================
    // WINDAGE PROCEDURE
    // ========================================================================
    bool newWindageVisible = data.windageModeActive || data.windageAppliedToBallistics;
    if (m_windageVisible != newWindageVisible) {
        m_windageVisible = newWindageVisible;
        emit windageVisibleChanged();
    }

    if (newWindageVisible) {
        QString newWindageText;
        if (data.windageModeActive) {
            newWindageText = "WINDAGE SETUP";
        } else if (data.windageAppliedToBallistics) {
            // Show wind speed and crosswind component
            newWindageText = QString("W: %1kt (%2m/s X)")
                                 .arg(data.windageSpeedKnots, 0, 'f', 1)
                                 .arg(data.calculatedCrosswindMS, 0, 'f', 1);
        }
        if (m_windageText != newWindageText) {
            m_windageText = newWindageText;
            emit windageTextChanged();
        }
    }

    // ========================================================================
    // DETECTION
    // ========================================================================
    bool newDetectionVisible = data.detectionEnabled;
    if (m_detectionVisible != newDetectionVisible) {
        m_detectionVisible = newDetectionVisible;
        emit detectionVisibleChanged();
    }

    QString newDetectionText = data.detectionEnabled ? "DETECTION: ON" : "DETECTION: OFF";
    if (m_detectionText != newDetectionText) {
        m_detectionText = newDetectionText;
        emit detectionTextChanged();
    }

    // Update detection boxes
    QVariantList newDetectionBoxes;
    for (const auto &detection : data.detections) {
        newDetectionBoxes.append(createDetectionBox(detection));
    }
    if (m_detectionBoxes != newDetectionBoxes) {
        m_detectionBoxes = newDetectionBoxes;
        emit detectionBoxesChanged();
    }

    // ========================================================================
    // SCAN PATTERN
    // ========================================================================
    bool newScanVisible = !data.currentScanName.isEmpty();
    if (m_scanNameVisible != newScanVisible) {
        m_scanNameVisible = newScanVisible;
        emit scanNameVisibleChanged();
    }

    if (m_scanNameText != data.currentScanName) {
        m_scanNameText = data.currentScanName;
        emit scanNameTextChanged();
    }

    // ========================================================================
    // LEAD ANGLE COMPENSATION
    // ========================================================================
    bool newLeadVisible = !data.leadStatusText.isEmpty();
    if (m_leadAngleVisible != newLeadVisible) {
        m_leadAngleVisible = newLeadVisible;
        emit leadAngleVisibleChanged();
    }

    if (m_leadAngleText != data.leadStatusText) {
        m_leadAngleText = data.leadStatusText;
        emit leadAngleTextChanged();
    }

    bool newLacActive = data.leadAngleActive;
    if (m_lacActive != newLacActive) {
        m_lacActive = newLacActive;
        emit lacActiveChanged();
    }

    if (m_leadAngleOffsetAz != data.leadAngleOffsetAz_deg ||
        m_leadAngleOffsetEl != data.leadAngleOffsetEl_deg) {
        m_leadAngleOffsetAz = data.leadAngleOffsetAz_deg;
        m_leadAngleOffsetEl = data.leadAngleOffsetEl_deg;
        emit leadAngleOffsetsChanged();
    }

    // ========================================================================
    // RETICLE POSITION (Gun Boresight with Zeroing Only)
    // ========================================================================
    // Convert image coordinates to screen-centered offsets
    float screenCenterX = 512.0f; // Assuming 1024x768 -> center is 512, 384
    float screenCenterY = 384.0f;

    float newReticleOffsetX = data.reticleAimpointImageX_px - screenCenterX;
    float newReticleOffsetY = data.reticleAimpointImageY_px - screenCenterY;

    if (m_reticleOffsetX != newReticleOffsetX) {
        m_reticleOffsetX = newReticleOffsetX;
        emit reticlePositionChanged();
    }

    if (m_reticleOffsetY != newReticleOffsetY) {
        m_reticleOffsetY = newReticleOffsetY;
        emit reticlePositionChanged();
    }

    // Update reticle type from FrameData
    int newReticleType = static_cast<int>(data.reticleType);
    if (m_reticleType != newReticleType) {
        m_reticleType = newReticleType;
        emit reticleTypeChanged();
    }

    // ========================================================================
    // CCIP (Bullet Impact Point with Zeroing + Lead)
    // ========================================================================
    // CCIP is visible when lead angle is active
    bool newCcipVisible = data.leadAngleActive;
    if (m_ccipVisible != newCcipVisible) {
        m_ccipVisible = newCcipVisible;
        emit ccipVisibleChanged();
    }

    if (newCcipVisible) {
        // Use the CCIP impact point from FrameData
        float newCcipX = data.ccipImpactImageX_px;
        float newCcipY = data.ccipImpactImageY_px;

        if (m_ccipX != newCcipX || m_ccipY != newCcipY) {
            m_ccipX = newCcipX;
            m_ccipY = newCcipY;
            emit ccipPositionChanged();
        }

        // Update CCIP status based on lead angle status
        QString newCcipStatus;
        switch (data.leadAngleStatus) {
        case LeadAngleStatus::On:
            newCcipStatus = "Active";
            break;
        case LeadAngleStatus::Lag:
            newCcipStatus = "Lag";
            break;
        case LeadAngleStatus::ZoomOut:
            newCcipStatus = "Zoom Too High";
            break;
        /*case LeadAngleStatus::RangeInvalid:
            newCcipStatus = "Range Invalid";
            break;
        case LeadAngleStatus::TrackingLost:
            newCcipStatus = "Tracking Lost";
            break;*/
        default:
            newCcipStatus = "Off";
            break;
        }

        if (m_ccipStatus != newCcipStatus) {
            m_ccipStatus = newCcipStatus;
            emit ccipStatusChanged();
        }
    } else {
        if (m_ccipStatus != "Off") {
            m_ccipStatus = "Off";
            emit ccipStatusChanged();
        }
    }

    // ========================================================================
    // TRACKING
    // ========================================================================
    bool newTrackingActive = data.trackerInitialized &&
                             (data.trackingState != VPI_TRACKING_STATE_LOST);
    if (m_trackingActive != newTrackingActive) {
        m_trackingActive = newTrackingActive;
        emit trackingActiveChanged();
    }

    bool newTrackingBoxVisible = newTrackingActive;
    if (m_trackingBoxVisible != newTrackingBoxVisible) {
        m_trackingBoxVisible = newTrackingBoxVisible;
        emit trackingBoxVisibleChanged();
    }

    QRectF newTrackingBox(data.trackingBbox.x(),
                          data.trackingBbox.y(),
                          data.trackingBbox.width(),
                          data.trackingBbox.height());
    if (m_trackingBox != newTrackingBox) {
        m_trackingBox = newTrackingBox;
        emit trackingBoxChanged();
    }

    // Tracking box color based on tracking state
    QString newBoxColor = "yellow"; // Default
    bool newDashed = false;

    switch (data.trackingState) {
    case VPI_TRACKING_STATE_TRACKED:
        newBoxColor = "green";
        newDashed = false;
        break;
    case VPI_TRACKING_STATE_LOST:
        newBoxColor = "red";
        newDashed = true;
        break;
    default:
        newBoxColor = "yellow";
        newDashed = false;
        break;
    }

    if (m_trackingBoxColor != newBoxColor) {
        m_trackingBoxColor = newBoxColor;
        emit trackingBoxColorChanged();
    }

    if (m_trackingBoxDashed != newDashed) {
        m_trackingBoxDashed = newDashed;
        emit trackingBoxDashedChanged();
    }

    // Use actual tracking confidence from FrameData
    if (m_trackingConfidence != data.trackingConfidence) {
        m_trackingConfidence = data.trackingConfidence;
        emit trackingConfidenceChanged();
    }

    // ========================================================================
    // ACQUISITION BOX (for tracking phase)
    // ========================================================================
    bool newAcqVisible = (data.currentTrackingPhase == TrackingPhase::Acquisition);
    if (m_acquisitionBoxVisible != newAcqVisible) {
        m_acquisitionBoxVisible = newAcqVisible;
        emit acquisitionBoxVisibleChanged();
    }

    QRectF newAcqBox(data.acquisitionBoxX_px,
                     data.acquisitionBoxY_px,
                     data.acquisitionBoxW_px,
                     data.acquisitionBoxH_px);
    if (m_acquisitionBox != newAcqBox) {
        m_acquisitionBox = newAcqBox;
        emit acquisitionBoxChanged();
    }

    // ========================================================================
    // ZONE WARNINGS
    // ========================================================================
    bool newZoneWarningVisible = data.isReticleInNoFireZone || data.gimbalStoppedAtNTZLimit;
    if (m_zoneWarningVisible != newZoneWarningVisible) {
        m_zoneWarningVisible = newZoneWarningVisible;
        emit zoneWarningChanged();
    }

    QString newZoneWarningText;
    if (data.isReticleInNoFireZone) {
        newZoneWarningText = "NO FIRE ZONE";
    } else if (data.gimbalStoppedAtNTZLimit) {
        newZoneWarningText = "NO TRAVERSE ZONE";
    }
    if (m_zoneWarningText != newZoneWarningText) {
        m_zoneWarningText = newZoneWarningText;
        emit zoneWarningTextChanged();
    }
}

// ========================================================================
// SETTERS FOR DEVICE HEALTH
// ========================================================================

void OsdViewModel::setDayCameraConnected(bool connected)
{
    if (m_dayCameraConnected != connected) {
        m_dayCameraConnected = connected;
        emit dayCameraConnectedChanged();
    }
}

void OsdViewModel::setDayCameraError(bool error)
{
    if (m_dayCameraError != error) {
        m_dayCameraError = error;
        emit dayCameraErrorChanged();
    }
}

void OsdViewModel::setNightCameraConnected(bool connected)
{
    if (m_nightCameraConnected != connected) {
        m_nightCameraConnected = connected;
        emit nightCameraConnectedChanged();
    }
}

void OsdViewModel::setNightCameraError(bool error)
{
    if (m_nightCameraError != error) {
        m_nightCameraError = error;
        emit nightCameraErrorChanged();
    }
}

void OsdViewModel::setAzServoConnected(bool connected)
{
    if (m_azServoConnected != connected) {
        m_azServoConnected = connected;
        emit azServoConnectedChanged();
    }
}

void OsdViewModel::setAzFault(bool fault)
{
    if (m_azFault != fault) {
        m_azFault = fault;
        emit azFaultChanged();
    }
}

void OsdViewModel::setElServoConnected(bool connected)
{
    if (m_elServoConnected != connected) {
        m_elServoConnected = connected;
        emit elServoConnectedChanged();
    }
}

void OsdViewModel::setElFault(bool fault)
{
    if (m_elFault != fault) {
        m_elFault = fault;
        emit elFaultChanged();
    }
}

void OsdViewModel::setLrfConnected(bool connected)
{
    if (m_lrfConnected != connected) {
        m_lrfConnected = connected;
        emit lrfConnectedChanged();
    }
}

void OsdViewModel::setLrfFault(bool fault)
{
    if (m_lrfFault != fault) {
        m_lrfFault = fault;
        emit lrfFaultChanged();
    }
}

void OsdViewModel::setLrfOverTemp(bool overTemp)
{
    if (m_lrfOverTemp != overTemp) {
        m_lrfOverTemp = overTemp;
        emit lrfOverTempChanged();
    }
}

void OsdViewModel::setActuatorConnected(bool connected)
{
    if (m_actuatorConnected != connected) {
        m_actuatorConnected = connected;
        emit actuatorConnectedChanged();
    }
}

void OsdViewModel::setActuatorFault(bool fault)
{
    if (m_actuatorFault != fault) {
        m_actuatorFault = fault;
        emit actuatorFaultChanged();
    }
}

void OsdViewModel::setPlc21Connected(bool connected)
{
    if (m_plc21Connected != connected) {
        m_plc21Connected = connected;
        emit plc21ConnectedChanged();
    }
}

void OsdViewModel::setPlc42Connected(bool connected)
{
    if (m_plc42Connected != connected) {
        m_plc42Connected = connected;
        emit plc42ConnectedChanged();
    }
}

void OsdViewModel::setJoystickConnected(bool connected)
{
    if (m_joystickConnected != connected) {
        m_joystickConnected = connected;
        emit joystickConnectedChanged();
    }
}

// ========================================================================
// SETTERS FOR MESSAGES
// ========================================================================

void OsdViewModel::setStartupMessage(const QString &message, bool visible)
{
    if (m_startupMessageText != message) {
        m_startupMessageText = message;
        emit startupMessageTextChanged();
    }
    if (m_startupMessageVisible != visible) {
        m_startupMessageVisible = visible;
        emit startupMessageVisibleChanged();
    }
}

void OsdViewModel::setErrorMessage(const QString &message, bool visible)
{
    if (m_errorMessageText != message) {
        m_errorMessageText = message;
        emit errorMessageTextChanged();
    }
    if (m_errorMessageVisible != visible) {
        m_errorMessageVisible = visible;
        emit errorMessageVisibleChanged();
    }
}

// ========================================================================
// SETTER FOR VEHICLE HEADING
// ========================================================================

void OsdViewModel::setVehicleHeading(float heading)
{
    if (m_vehicleHeading != heading) {
        m_vehicleHeading = heading;
        emit vehicleHeadingChanged();
    }
}

// ========================================================================
// HELPER METHODS
// ========================================================================

QString OsdViewModel::formatMode(int mode) const
{
    switch (static_cast<OperationalMode>(mode)) {
    case OperationalMode::Idle: return "IDLE";
    case OperationalMode::Surveillance: return "SURVEILLANCE";
    case OperationalMode::Tracking: return "TRACKING";
    case OperationalMode::Engagement: return "ENGAGEMENT";
    default: return "UNKNOWN";
    }
}

QString OsdViewModel::formatMotionMode(int motionMode) const
{
    switch (static_cast<MotionMode>(motionMode)) {
    case MotionMode::Manual: return "MANUAL";
    case MotionMode::Pattern: return "PATTERN";
    case MotionMode::RadarSlew: return "RADAR SLEW";
    case MotionMode::AutoTrack: return "AUTO TRACK";
    case MotionMode::ManualTrack: return "MANUAL TRACK";
    default: return "UNKNOWN";
    }
}

QString OsdViewModel::formatFireMode(int fireMode) const
{
    switch (static_cast<FireMode>(fireMode)) {
    case FireMode::SingleShot: return "SINGLE SHOT";
    case FireMode::ShortBurst: return "BURST";
    case FireMode::LongBurst: return "FULL AUTO";
    default: return "UNKNOWN";
    }
}

QVariantMap OsdViewModel::createDetectionBox(const YoloDetection &detection) const
{
    QVariantMap box;
    box["x"] = detection.box.x;
    box["y"] = detection.box.y;
    box["width"] = detection.box.width;
    box["height"] = detection.box.height;
    box["className"] = QString::fromStdString(detection.className);
    box["confidence"] = detection.confidence;

    // Color information (adjust based on your YoloDetection struct)
    // If you have detection.color as QColor or RGB values, use them here
    box["colorR"] = 70;   // Default green accent
    box["colorG"] = 226;
    box["colorB"] = 165;

    return box;
}
