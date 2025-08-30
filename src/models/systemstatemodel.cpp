#include "systemstatemodel.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm> // For std::find_if, std::sort (if needed)
#include <set>       // For getting unique page numbers


SystemStateModel::SystemStateModel(QObject *parent)
    : QObject(parent),
      m_nextAreaZoneId(1), // Start IDs from 1
      m_nextSectorScanId(1),
      m_nextTRPId(1)
{
    // Initialize m_currentStateData with defaults if needed
    clearZeroing(); // Zero is lost on power down
    clearWindage(); // Windage is zero on startup
    // Connect signals from sub-models to slots here (as was likely intended)
    loadZonesFromFile("zones.json"); // Load initial zones from file if exists

    // --- POPULATE DUMMY RADAR DATA FOR TESTING ---
    QVector<SimpleRadarPlot> dummyPlots;
    dummyPlots.append({101, 45.0f, 1500.0f, 180.0f, 0.0f});   // ID 101, NE quadrant, 1.5km, stationary (course away)
    dummyPlots.append({102, 110.0f, 850.0f, 290.0f, 5.0f});   // ID 102, SE quadrant, 850m, moving slowly
    dummyPlots.append({103, 315.0f, 2200.0f, 120.0f, 15.0f}); // ID 103, NW quadrant, 2.2km, moving moderately
    dummyPlots.append({104, 260.0f, 500.0f, 80.0f, 25.0f});   // ID 104, SW quadrant, 500m, moving quickly
    dummyPlots.append({105, 5.0f, 3100.0f, 175.0f, -2.0f});  // ID 105, Directly ahead, 3.1km, moving away slowly
    dummyPlots.append({106, 178.0f, 4500.0f, 0.0f, 2.0f});   // ID 106, Directly behind, 4.5km, moving towards

    // Create a new SystemStateData object, populate it, and set it in the model
     SystemStateData initialData = m_currentStateData;
    initialData.radarPlots = dummyPlots;
    updateData(initialData);
}

// =================================================================
// 1. CORE SYSTEM & MODE MANAGEMENT
// =================================================================
void SystemStateModel::updateData(const SystemStateData &newState) {

    SystemStateData oldData = m_currentStateData;

    if (oldData == newState) {
        return;
    }
    if (m_currentStateData != newState) {
        bool gimbalChanged = !qFuzzyCompare(m_currentStateData.gimbalAz, newState.gimbalAz) ||
                             !qFuzzyCompare(m_currentStateData.gimbalEl, newState.gimbalEl);

        m_currentStateData = newState;
        processStateTransitions(oldData, m_currentStateData);
        emit dataChanged(m_currentStateData);

        if (gimbalChanged) {
            emit gimbalPositionChanged(m_currentStateData.gimbalAz, m_currentStateData.gimbalEl);
        }
    }
}

void SystemStateModel::setMotionMode(MotionMode newMode) {
    if(m_currentStateData.motionMode != newMode) {
        m_currentStateData.previousMotionMode = m_currentStateData.motionMode;
        if (m_currentStateData.motionMode == MotionMode::AutoSectorScan || m_currentStateData.motionMode == MotionMode::TRPScan) {
            m_currentStateData.currentScanName = "";
        }
        m_currentStateData.motionMode = newMode;

        emit dataChanged(m_currentStateData);
         if (newMode == MotionMode::AutoSectorScan || newMode == MotionMode::TRPScan) {
            updateCurrentScanName();
        }
    }
}

void SystemStateModel::setOpMode(OperationalMode newOpMode) { if(m_currentStateData.opMode != newOpMode) { m_currentStateData.previousOpMode = m_currentStateData.opMode; m_currentStateData.opMode = newOpMode; emit dataChanged(m_currentStateData); } }

// =================================================================
// 2. USER INTERFACE CONTROLS
// =================================================================
void SystemStateModel::setColorStyle(const QColor &style)
{
    SystemStateData newData = m_currentStateData;
    newData.colorStyle = style;
    newData.osdColorStyle = ColorUtils::fromQColor(style);
    emit colorStyleChanged(style);
    updateData(newData);
}

void SystemStateModel::setReticleStyle(const ReticleType &type)
{
    SystemStateData newData = m_currentStateData;
    newData.reticleType = type;
    updateData(newData);
    emit reticleStyleChanged(type);
}


// =================================================================
// 3. GIMBAL, WEAPON, AND TRACKING CONTROL
// =================================================================
void SystemStateModel::setDeadManSwitch(bool pressed) { if(m_currentStateData.deadManSwitchActive != pressed) { m_currentStateData.deadManSwitchActive = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setActiveCameraIsDay(bool pressed) { if(m_currentStateData.activeCameraIsDay != pressed) { m_currentStateData.activeCameraIsDay = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setDownTrack(bool pressed) { if(m_currentStateData.downTrack != pressed) { m_currentStateData.downTrack = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setDownSw(bool pressed) { if(m_currentStateData.menuDown != pressed) { m_currentStateData.menuDown = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setUpTrack(bool pressed) { if(m_currentStateData.upTrack != pressed) { m_currentStateData.upTrack = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setUpSw(bool pressed) { if(m_currentStateData.menuUp != pressed) { m_currentStateData.menuUp = pressed; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setTrackingRestartRequested(bool restart) { if(m_currentStateData.requestTrackingRestart != restart) { m_currentStateData.requestTrackingRestart = restart; emit dataChanged(m_currentStateData); } }
void SystemStateModel::setTrackingStarted(bool start) { if(m_currentStateData.startTracking != start) { m_currentStateData.startTracking = start; emit dataChanged(m_currentStateData); } }

void SystemStateModel::updateTrackingResult(
    int cameraIndex,
    bool hasLock,
    float centerX_px, float centerY_px,
    float width_px, float height_px,
    float velocityX_px_s, float velocityY_px_s,
    VPITrackingState trackerState)
{
    int activeCameraIndex = m_currentStateData.activeCameraIsDay ? 0 : 1;
    if (cameraIndex != activeCameraIndex) {
        return;
    }

    SystemStateData& data = m_currentStateData;
    bool stateDataChanged = false;

    bool newTrackerHasValidTarget = (trackerState == VPI_TRACKING_STATE_TRACKED);

    if (data.trackerHasValidTarget != newTrackerHasValidTarget) { data.trackerHasValidTarget = newTrackerHasValidTarget; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetCenterX_px, centerX_px)) { data.trackedTargetCenterX_px = centerX_px; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetCenterY_px, centerY_px)) { data.trackedTargetCenterY_px = centerY_px; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetWidth_px, width_px)) { data.trackedTargetWidth_px = width_px; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetHeight_px, height_px)) { data.trackedTargetHeight_px = height_px; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetVelocityX_px_s, velocityX_px_s)) { data.trackedTargetVelocityX_px_s = velocityX_px_s; stateDataChanged = true; }
    if (!qFuzzyCompare(data.trackedTargetVelocityY_px_s, velocityY_px_s)) { data.trackedTargetVelocityY_px_s = velocityY_px_s; stateDataChanged = true; }
    if (data.trackedTargetState != trackerState) { data.trackedTargetState = trackerState; stateDataChanged = true; }

    TrackingPhase oldPhase = data.currentTrackingPhase;

    switch (data.currentTrackingPhase) {
        case TrackingPhase::Off:
            if (trackerState != VPI_TRACKING_STATE_LOST) {
                qWarning() << "[MODEL] Received tracking data while in Off phase. Resetting model tracking state.";
                data.trackerHasValidTarget = false;
                data.trackedTargetState = VPI_TRACKING_STATE_LOST;
                data.motionMode = MotionMode::Manual;
            }
            break;

        case TrackingPhase::Acquisition:
            if (trackerState != VPI_TRACKING_STATE_LOST) {
                qWarning() << "[MODEL] Received tracking data (" << static_cast<int>(trackerState) << ") while in Acquisition phase. Ignoring for phase transition.";
            }
            break;

        case TrackingPhase::Tracking_LockPending:
         qDebug() << "Ttracker State " << static_cast<int>(trackerState) << " in LockPending phase.";
            if (trackerState == VPI_TRACKING_STATE_TRACKED) {
                data.currentTrackingPhase = TrackingPhase::Tracking_ActiveLock;
                data.opMode = OperationalMode::Tracking;
                data.motionMode = MotionMode::AutoTrack;
                qInfo() << "[MODEL] Valid Lock Acquired! Phase -> ActiveLock (" << static_cast<int>(data.currentTrackingPhase) << ")";
            } else if (trackerState == VPI_TRACKING_STATE_LOST) {
                data.currentTrackingPhase = TrackingPhase::Off;
                data.opMode = OperationalMode::Idle;
                data.motionMode = MotionMode::Manual;
                data.trackerHasValidTarget = false;
                qWarning() << "[MODEL] Tracker failed to acquire lock (LOST). Returning to Off (" << static_cast<int>(data.currentTrackingPhase) << ").";
            } else if (trackerState == VPI_TRACKING_STATE_NEW) {
                qDebug() << "[MODEL] In LockPending, tracker initialized (NEW). Waiting for lock.";
            } else {
                qWarning() << "[MODEL] In LockPending, received unexpected VPI state: " << static_cast<int>(trackerState) << ". Staying in LockPending.";
            }
            break;

        case TrackingPhase::Tracking_ActiveLock:
            if (trackerState == VPI_TRACKING_STATE_LOST) {
                data.currentTrackingPhase = TrackingPhase::Tracking_Coast;
                data.opMode = OperationalMode::Tracking;
                data.motionMode = MotionMode::Manual;
                data.trackerHasValidTarget = false;
                qWarning() << "[MODEL] Target lost during active tracking. Transitioning to Coast (" << static_cast<int>(data.currentTrackingPhase) << ").";
            } else if (trackerState == VPI_TRACKING_STATE_TRACKED) {
                qDebug() << "[MODEL] ActiveLock: Target still tracked.";
            } else {
                qWarning() << "[MODEL] In ActiveLock, received unexpected VPI state: " << static_cast<int>(trackerState) << ". Staying in ActiveLock but might indicate issue.";
            }
            break;

        case TrackingPhase::Tracking_Coast:
            if (trackerState == VPI_TRACKING_STATE_TRACKED) {
                data.currentTrackingPhase = TrackingPhase::Tracking_ActiveLock;
                data.opMode = OperationalMode::Tracking;
                data.motionMode = MotionMode::AutoTrack;
                qInfo() << "[MODEL] Target Re-acquired! Phase -> ActiveLock (" << static_cast<int>(data.currentTrackingPhase) << ")";
            } else if (trackerState == VPI_TRACKING_STATE_LOST) {
                qDebug() << "[MODEL] In Coast: Target still lost.";
            } else if (trackerState == VPI_TRACKING_STATE_NEW) {
                qDebug() << "[MODEL] In Coast: Tracker re-initialized (NEW). Waiting for re-acquisition.";
            }
            break;

        case TrackingPhase::Tracking_Firing:
            qDebug() << "[MODEL] In Firing phase. Ignoring tracking state for phase transition.";
            break;

        default:
            qWarning() << "[MODEL] Unknown TrackingPhase: " << static_cast<int>(data.currentTrackingPhase);
            break;
    }

    if (oldPhase != data.currentTrackingPhase) {
        stateDataChanged = true;
    }

    if (stateDataChanged) {
        qDebug() << "[MODEL-OUT] Emitting dataChanged. New Phase:" << static_cast<int>(data.currentTrackingPhase)
                 << "Valid Target:" << data.trackerHasValidTarget;
         qDebug() << "trackedTarget_position: (" << data.trackedTargetCenterX_px << ", " << data.trackedTargetCenterY_px << ")";

        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::startTrackingAcquisition() {
    SystemStateData& data = m_currentStateData;
    if (data.currentTrackingPhase == TrackingPhase::Off) {
        data.currentTrackingPhase = TrackingPhase::Acquisition;
        float reticleCenterX = data.reticleAimpointImageX_px;
        float reticleCenterY = data.reticleAimpointImageY_px;

        qDebug() << "[MODEL] Starting Acquisition. Centering initial box on reticle at:"
                 << reticleCenterX << "," << reticleCenterY;

        float defaultBoxW = 100.0f;
        float defaultBoxH = 100.0f;
        data.acquisitionBoxW_px = defaultBoxW;
        data.acquisitionBoxH_px = defaultBoxH;
        data.acquisitionBoxX_px = reticleCenterX - (defaultBoxW / 2.0f);
        data.acquisitionBoxY_px = reticleCenterY - (defaultBoxH / 2.0f);

        data.acquisitionBoxX_px = qBound(0.0f, data.acquisitionBoxX_px, static_cast<float>(data.currentImageWidthPx) - data.acquisitionBoxW_px);
        data.acquisitionBoxY_px = qBound(0.0f, data.acquisitionBoxY_px, static_cast<float>(data.currentImageHeightPx) - data.acquisitionBoxH_px);

        data.opMode = OperationalMode::Surveillance;
        data.motionMode = MotionMode::Manual;

        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::requestTrackerLockOn() {
    SystemStateData& data = m_currentStateData;
    if (data.currentTrackingPhase == TrackingPhase::Acquisition) {
        data.currentTrackingPhase = TrackingPhase::Tracking_LockPending;
        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::stopTracking() {
    SystemStateData& data = m_currentStateData;
    if (data.currentTrackingPhase != TrackingPhase::Off) {
        data.currentTrackingPhase = TrackingPhase::Off;
        data.trackerHasValidTarget = false;
        data.opMode = OperationalMode::Surveillance;
        data.motionMode = MotionMode::Manual;
        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::adjustAcquisitionBoxSize(float dW, float dH) {
    SystemStateData& data = m_currentStateData;
    if (data.currentTrackingPhase == TrackingPhase::Acquisition) {
        data.acquisitionBoxW_px += dW;
        data.acquisitionBoxH_px += dH;
        data.acquisitionBoxW_px = qBound(20.0f, data.acquisitionBoxW_px, static_cast<float>(data.currentImageWidthPx * 0.8f));
        data.acquisitionBoxH_px = qBound(20.0f, data.acquisitionBoxH_px, static_cast<float>(data.currentImageHeightPx * 0.8f));
        data.acquisitionBoxX_px = (data.currentImageWidthPx / 2.0f) - (data.acquisitionBoxW_px / 2.0f);
        data.acquisitionBoxY_px = (data.currentImageHeightPx / 2.0f) - (data.acquisitionBoxH_px / 2.0f);
        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::enterSurveillanceMode() {
    SystemStateData& data = m_currentStateData;
    if (!data.stationEnabled || data.opMode == OperationalMode::Surveillance) return;

    qDebug() << "[MODEL] Transitioning to Surveillance Mode.";
    data.opMode = OperationalMode::Surveillance;
    data.motionMode = MotionMode::Manual;
    emit dataChanged(m_currentStateData);
}

void SystemStateModel::enterIdleMode() {
    SystemStateData& data = m_currentStateData;
    if (data.opMode == OperationalMode::Idle) return;

    qDebug() << "[MODEL] Transitioning to Idle Mode.";
    data.opMode = OperationalMode::Idle;
    data.motionMode = MotionMode::Idle;
    if (data.currentTrackingPhase != TrackingPhase::Off) {
        stopTracking();
    }
    emit dataChanged(m_currentStateData);
}

void SystemStateModel::commandEngagement(bool start) {
    SystemStateData& data = m_currentStateData;
    if (start) {
        if (data.opMode == OperationalMode::Engagement || !data.gunArmed) {
            return;
        }
        qDebug() << "[MODEL] Entering Engagement Mode.";
        data.previousOpMode = data.opMode;
        data.previousMotionMode = data.motionMode;
        data.opMode = OperationalMode::Engagement;
    } else {
        if (data.opMode != OperationalMode::Engagement) return;
        qDebug() << "[MODEL] Exiting Engagement Mode, reverting to previous state.";
        data.opMode = data.previousOpMode;
        data.motionMode = data.previousMotionMode;
    }
    emit dataChanged(m_currentStateData);
}


// =================================================================
// 4. FIRE CONTROL AND SAFETY ZONES
// =================================================================
void SystemStateModel::setPointInNoFireZone(bool inZone) {
    m_currentStateData.isReticleInNoFireZone = inZone;
    emit dataChanged(m_currentStateData);
}

void SystemStateModel::setPointInNoTraverseZone(bool inZone) {
    m_currentStateData.isReticleInNoTraverseZone = inZone;
    emit dataChanged(m_currentStateData);
}

bool SystemStateModel::isPointInNoFireZone(float targetAz, float targetEl, float targetRange) const {
    for (const auto& zone : m_currentStateData.areaZones) {
        if (zone.isEnabled && zone.type == ZoneType::NoFire) {
            bool azMatch = isAzimuthInRange(targetAz, zone.startAzimuth, zone.endAzimuth);
            bool elMatch = (targetEl >= zone.minElevation && targetEl <= zone.maxElevation);
            bool rangeMatch = true;
            if (azMatch && elMatch && rangeMatch) {
                return true;
            }
        }
    }
    return false;
}

bool SystemStateModel::isPointInNoTraverseZone(float targetAz, float currentEl) const {
    for (const auto& zone : m_currentStateData.areaZones) {
        if (zone.isEnabled && zone.type == ZoneType::NoTraverse) {
            bool elInRange = (currentEl >= zone.minElevation && currentEl <= zone.maxElevation);
            if (elInRange && isAzimuthInRange(targetAz, zone.startAzimuth, zone.endAzimuth)) {
                return true;
            }
        }
    }
    return false;
}

// =================================================================
// 5. BALLISTIC COMPENSATION
// =================================================================
void SystemStateModel::setLeadAngleCompensationActive(bool active) {
    if (m_currentStateData.leadAngleCompensationActive != active) {
        m_currentStateData.leadAngleCompensationActive = active;
        if (!active) {
            m_currentStateData.currentLeadAngleStatus = LeadAngleStatus::Off;
            m_currentStateData.leadAngleOffsetAz = 0.0f;
            m_currentStateData.leadAngleOffsetEl = 0.0f;
        } else {
             m_currentStateData.currentLeadAngleStatus = LeadAngleStatus::On;
        }
        qDebug() << "Lead Angle Compensation active:" << active;
        if (!active) {
            m_currentStateData.currentLeadAngleStatus = LeadAngleStatus::Off;
            m_currentStateData.leadAngleOffsetAz = 0.0f;
            m_currentStateData.leadAngleOffsetEl = 0.0f;
        } else {
             m_currentStateData.currentLeadAngleStatus = LeadAngleStatus::On;
        }

        recalculateDerivedAimpointData();
        updateData(m_currentStateData);
    }
}

void SystemStateModel::updateCalculatedLeadOffsets(float angularLeadAz, float angularLeadEl, LeadAngleStatus statusFromCalc) {
    bool changed = false;

    if (!qFuzzyCompare(m_currentStateData.leadAngleOffsetAz, angularLeadAz)) {
        m_currentStateData.leadAngleOffsetAz = angularLeadAz;
        changed = true;
    }
    if (!qFuzzyCompare(m_currentStateData.leadAngleOffsetEl, angularLeadEl)) {
        m_currentStateData.leadAngleOffsetEl = angularLeadEl;
        changed = true;
    }
    if (m_currentStateData.currentLeadAngleStatus != statusFromCalc) {
        m_currentStateData.currentLeadAngleStatus = statusFromCalc;
        changed = true;
    }

    if (changed) {
        qDebug() << "SystemStateModel: Angular Lead Offsets received: Az" << angularLeadAz
                 << "El" << angularLeadEl << "Status:" << static_cast<int>(statusFromCalc)
                 << "LAC Active in model:" << m_currentStateData.leadAngleCompensationActive;

        recalculateDerivedAimpointData();
    }
    updateData(m_currentStateData);
}

void SystemStateModel::startZeroingProcedure() {
    if (!m_currentStateData.zeroingModeActive) {
        m_currentStateData.zeroingModeActive = true;
        qDebug() << "Zeroing procedure started.";
        emit dataChanged(m_currentStateData);
        emit zeroingStateChanged(true, m_currentStateData.zeroingAzimuthOffset, m_currentStateData.zeroingElevationOffset);
    }
}

void SystemStateModel::applyZeroingAdjustment(float deltaAz, float deltaEl) {
    if (m_currentStateData.zeroingModeActive) {
        m_currentStateData.zeroingAzimuthOffset += deltaAz;
        m_currentStateData.zeroingElevationOffset += deltaEl;

        qDebug() << "Zeroing adjustment applied. New offsets Az:" << m_currentStateData.zeroingAzimuthOffset
                 << "El:" << m_currentStateData.zeroingElevationOffset;
        emit dataChanged(m_currentStateData);
        emit zeroingStateChanged(true, m_currentStateData.zeroingAzimuthOffset, m_currentStateData.zeroingElevationOffset);
    }
}

void SystemStateModel::finalizeZeroing() {
    if (m_currentStateData.zeroingModeActive) {
        m_currentStateData.zeroingModeActive = false;
        m_currentStateData.zeroingAppliedToBallistics = true;
        qDebug() << "Zeroing procedure finalized. Offsets Az:" << m_currentStateData.zeroingAzimuthOffset
                 << "El:" << m_currentStateData.zeroingElevationOffset;
        emit dataChanged(m_currentStateData);
        emit zeroingStateChanged(false, m_currentStateData.zeroingAzimuthOffset, m_currentStateData.zeroingElevationOffset);
    }
}

void SystemStateModel::clearZeroing() {
    m_currentStateData.zeroingModeActive = false;
    m_currentStateData.zeroingAzimuthOffset = 0.0f;
    m_currentStateData.zeroingElevationOffset = 0.0f;
    m_currentStateData.zeroingAppliedToBallistics = false;
    qDebug() << "Zeroing cleared.";
    emit dataChanged(m_currentStateData);
    emit zeroingStateChanged(false, 0.0f, 0.0f);
}

void SystemStateModel::startWindageProcedure() {
    if (!m_currentStateData.windageModeActive) {
        m_currentStateData.windageModeActive = true;
        qDebug() << "Windage procedure started.";
        emit dataChanged(m_currentStateData);
        emit windageStateChanged(true, m_currentStateData.windageSpeedKnots);
    }
}

void SystemStateModel::setWindageSpeed(float knots) {
    if (m_currentStateData.windageModeActive) {
        m_currentStateData.windageSpeedKnots = qMax(0.0f, knots);
        qDebug() << "Windage speed set to:" << m_currentStateData.windageSpeedKnots << "knots";
        emit dataChanged(m_currentStateData);
        emit windageStateChanged(true, m_currentStateData.windageSpeedKnots);
    }
}

void SystemStateModel::finalizeWindage() {
    if (m_currentStateData.windageModeActive) {
        m_currentStateData.windageModeActive = false;
        m_currentStateData.windageAppliedToBallistics = (m_currentStateData.windageSpeedKnots > 0.001f);
        qDebug() << "Windage procedure finalized. Speed:" << m_currentStateData.windageSpeedKnots
                 << "Applied:" << m_currentStateData.windageAppliedToBallistics;
        emit dataChanged(m_currentStateData);
        emit windageStateChanged(false, m_currentStateData.windageSpeedKnots);
    }
}

void SystemStateModel::clearWindage() {
    m_currentStateData.windageModeActive = false;
    m_currentStateData.windageSpeedKnots = 0.0f;
    m_currentStateData.windageAppliedToBallistics = false;
    qDebug() << "Windage cleared.";
}

// =================================================================
// 6. AREA ZONE MANAGEMENT
// =================================================================
const std::vector<AreaZone>& SystemStateModel::getAreaZones() const {
    return m_currentStateData.areaZones;
}

AreaZone* SystemStateModel::getAreaZoneById(int id) {
    auto it = std::find_if(m_currentStateData.areaZones.begin(), m_currentStateData.areaZones.end(),
                           [id](const AreaZone& z){ return z.id == id; });
    return (it != m_currentStateData.areaZones.end()) ? &(*it) : nullptr;
}

bool SystemStateModel::addAreaZone(AreaZone zone) {
    zone.id = getNextAreaZoneId();
    m_currentStateData.areaZones.push_back(zone);
    qDebug() << "Added AreaZone with ID:" << zone.id;
    emit zonesChanged();
    return true;
}

bool SystemStateModel::modifyAreaZone(int id, const AreaZone& updatedZoneData) {
    AreaZone* zonePtr = getAreaZoneById(id);
    if (zonePtr) {
        *zonePtr = updatedZoneData;
        zonePtr->id = id;
        qDebug() << "Modified AreaZone with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "modifyAreaZone: ID not found:" << id;
        return false;
    }
}

bool SystemStateModel::deleteAreaZone(int id) {
    auto it = std::remove_if(m_currentStateData.areaZones.begin(), m_currentStateData.areaZones.end(),
                             [id](const AreaZone& z){ return z.id == id; });
    if (it != m_currentStateData.areaZones.end()) {
        m_currentStateData.areaZones.erase(it, m_currentStateData.areaZones.end());
        qDebug() << "Deleted AreaZone with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "deleteAreaZone: ID not found:" << id;
        return false;
    }
}

// =================================================================
// 7. AUTO SECTOR SCAN MANAGEMENT
// =================================================================
const std::vector<AutoSectorScanZone>& SystemStateModel::getSectorScanZones() const {
    return m_currentStateData.sectorScanZones;
}

AutoSectorScanZone* SystemStateModel::getSectorScanZoneById(int id) {
    auto it = std::find_if(m_currentStateData.sectorScanZones.begin(), m_currentStateData.sectorScanZones.end(),
                           [id](const AutoSectorScanZone& z){ return z.id == id; });
    return (it != m_currentStateData.sectorScanZones.end()) ? &(*it) : nullptr;
}

bool SystemStateModel::addSectorScanZone(AutoSectorScanZone zone) {
    zone.id = getNextSectorScanId();
    m_currentStateData.sectorScanZones.push_back(zone);
    qDebug() << "Added SectorScanZone with ID:" << zone.id;
    emit zonesChanged();
    return true;
}

bool SystemStateModel::modifySectorScanZone(int id, const AutoSectorScanZone& updatedZoneData) {
    AutoSectorScanZone* zonePtr = getSectorScanZoneById(id);
    if (zonePtr) {
        *zonePtr = updatedZoneData;
        zonePtr->id = id;
        qDebug() << "Modified SectorScanZone with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "modifySectorScanZone: ID not found:" << id;
        return false;
    }
}

bool SystemStateModel::deleteSectorScanZone(int id) {
    auto it = std::remove_if(m_currentStateData.sectorScanZones.begin(), m_currentStateData.sectorScanZones.end(),
                             [id](const AutoSectorScanZone& z){ return z.id == id; });
    if (it != m_currentStateData.sectorScanZones.end()) {
        m_currentStateData.sectorScanZones.erase(it, m_currentStateData.sectorScanZones.end());
        qDebug() << "Deleted SectorScanZone with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "deleteSectorScanZone: ID not found:" << id;
        return false;
    }
}

void SystemStateModel::selectNextAutoSectorScanZone() {
    SystemStateData& data = m_currentStateData;
    if (data.sectorScanZones.empty()) {
        data.activeAutoSectorScanZoneId = -1;
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }

    std::vector<int> enabledZoneIds;
    for (const auto& zone : data.sectorScanZones) {
        if (zone.isEnabled) {
            enabledZoneIds.push_back(zone.id);
        }
    }
    if (enabledZoneIds.empty()) {
        data.activeAutoSectorScanZoneId = -1;
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }
    std::sort(enabledZoneIds.begin(), enabledZoneIds.end());

    auto it = std::find(enabledZoneIds.begin(), enabledZoneIds.end(), data.activeAutoSectorScanZoneId);

    if (it == enabledZoneIds.end() || std::next(it) == enabledZoneIds.end()) {
        data.activeAutoSectorScanZoneId = enabledZoneIds.front();
    } else {
        data.activeAutoSectorScanZoneId = *std::next(it);
    }
    qDebug() << "Selected next Auto Sector Scan Zone ID:" << data.activeAutoSectorScanZoneId;

    updateCurrentScanName();
    emit dataChanged(data);
}

void SystemStateModel::selectPreviousAutoSectorScanZone() {
    SystemStateData& data = m_currentStateData;
    if (data.sectorScanZones.empty()) {
        data.activeAutoSectorScanZoneId = -1;
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }

    std::vector<int> enabledZoneIds;
    for (const auto& zone : data.sectorScanZones) {
        if (zone.isEnabled) {
            enabledZoneIds.push_back(zone.id);
        }
    }
    if (enabledZoneIds.empty()) {
        data.activeAutoSectorScanZoneId = -1;
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }
    std::sort(enabledZoneIds.begin(), enabledZoneIds.end());

    auto it = std::find(enabledZoneIds.begin(), enabledZoneIds.end(), data.activeAutoSectorScanZoneId);

    if (it == enabledZoneIds.end() || it == enabledZoneIds.begin()) {
        data.activeAutoSectorScanZoneId = enabledZoneIds.back();
    } else {
        data.activeAutoSectorScanZoneId = *std::prev(it);
    }
    qDebug() << "Selected previous Auto Sector Scan Zone ID:" << data.activeAutoSectorScanZoneId;
    updateCurrentScanName();
    emit dataChanged(data);
        updateData(data);
}

// =================================================================
// 8. TARGET REFERENCE POINT (TRP) MANAGEMENT
// =================================================================
const std::vector<TargetReferencePoint>& SystemStateModel::getTargetReferencePoints() const {
    return m_currentStateData.targetReferencePoints;
}

TargetReferencePoint* SystemStateModel::getTRPById(int id) {
    auto it = std::find_if(m_currentStateData.targetReferencePoints.begin(), m_currentStateData.targetReferencePoints.end(),
                           [id](const TargetReferencePoint& z){ return z.id == id; });
    return (it != m_currentStateData.targetReferencePoints.end()) ? &(*it) : nullptr;
}

bool SystemStateModel::addTRP(TargetReferencePoint trp) {
    trp.id = getNextTRPId();
    m_currentStateData.targetReferencePoints.push_back(trp);
    qDebug() << "Added TRP with ID:" << trp.id;
    emit zonesChanged();
    return true;
}

bool SystemStateModel::modifyTRP(int id, const TargetReferencePoint& updatedTRPData) {
    TargetReferencePoint* trpPtr = getTRPById(id);
    if (trpPtr) {
        *trpPtr = updatedTRPData;
        trpPtr->id = id;
        qDebug() << "Modified TRP with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "modifyTRP: ID not found:" << id;
        return false;
    }
}

bool SystemStateModel::deleteTRP(int id) {
    auto it = std::remove_if(m_currentStateData.targetReferencePoints.begin(), m_currentStateData.targetReferencePoints.end(),
                             [id](const TargetReferencePoint& z){ return z.id == id; });
    if (it != m_currentStateData.targetReferencePoints.end()) {
        m_currentStateData.targetReferencePoints.erase(it, m_currentStateData.targetReferencePoints.end());
        qDebug() << "Deleted TRP with ID:" << id;
        emit zonesChanged();
        return true;
    } else {
        qWarning() << "deleteTRP: ID not found:" << id;
        return false;
    }
}

void SystemStateModel::selectNextTRPLocationPage() {
    SystemStateData& data = m_currentStateData;

    std::set<int> definedPagesSet;
    for (const auto& trp : data.targetReferencePoints) {
        definedPagesSet.insert(trp.locationPage);
    }

    if (definedPagesSet.empty()) {
        qDebug() << "selectNextTRPLocationPage: No TRP pages defined at all.";
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }

    std::vector<int> sortedDefinedPages(definedPagesSet.begin(), definedPagesSet.end());

    auto it = std::find(sortedDefinedPages.begin(), sortedDefinedPages.end(), data.activeTRPLocationPage);

    if (it == sortedDefinedPages.end() || std::next(it) == sortedDefinedPages.end()) {
        data.activeTRPLocationPage = sortedDefinedPages.front();
    } else {
        data.activeTRPLocationPage = *std::next(it);
    }

    qDebug() << "Selected next TRP Location Page:" << data.activeTRPLocationPage;
    updateCurrentScanName();
    emit dataChanged(data);
}

void SystemStateModel::selectPreviousTRPLocationPage() {
    SystemStateData& data = m_currentStateData;

    std::set<int> definedPagesSet;
    for (const auto& trp : data.targetReferencePoints) {
        definedPagesSet.insert(trp.locationPage);
    }

    if (definedPagesSet.empty()) {
        qDebug() << "selectPreviousTRPLocationPage: No TRP pages defined at all.";
        updateCurrentScanName();
        emit dataChanged(data);
        return;
    }

    std::vector<int> sortedDefinedPages(definedPagesSet.begin(), sortedDefinedPages.end());

    auto it = std::find(sortedDefinedPages.begin(), sortedDefinedPages.end(), data.activeTRPLocationPage);

    if (it == sortedDefinedPages.end() || it == sortedDefinedPages.begin()) {
        data.activeTRPLocationPage = sortedDefinedPages.back();
    } else {
        data.activeTRPLocationPage = *std::prev(it);
    }

    qDebug() << "Selected previous TRP Location Page:" << data.activeTRPLocationPage;
    updateCurrentScanName();
    emit dataChanged(data);
}


// =================================================================
// 9. RADAR CONTROL
// =================================================================
void SystemStateModel::selectNextRadarTrack() {
    SystemStateData& data = m_currentStateData;
    if (data.radarPlots.isEmpty()) return;

    auto it = std::find_if(data.radarPlots.begin(), data.radarPlots.end(),
                           [&](const SimpleRadarPlot& p){
                               return p.id == data.selectedRadarTrackId;
                            }
                           );

    if (it == data.radarPlots.end() || std::next(it) == data.radarPlots.end()) {
        data.selectedRadarTrackId = data.radarPlots.front().id;
    } else {
        data.selectedRadarTrackId = (*std::next(it)).id;
    }
    qDebug() << "[MODEL] Selected Radar Track ID:" << data.selectedRadarTrackId;
    emit dataChanged(data);
}

void SystemStateModel::selectPreviousRadarTrack() {
    SystemStateData& data = m_currentStateData;
    if (data.radarPlots.isEmpty()) return;

    auto it = std::find_if(data.radarPlots.begin(), data.radarPlots.end(),
                           [&](const SimpleRadarPlot& p){
                               return p.id == data.selectedRadarTrackId;
                           }
                           );

    if (it == data.radarPlots.end() || it == data.radarPlots.begin()) {
        data.selectedRadarTrackId = data.radarPlots.back().id;
    } else {
        data.selectedRadarTrackId = (*std::prev(it)).id;
    }
    qDebug() << "[MODEL] Selected Radar Track ID:" << data.selectedRadarTrackId;
    emit dataChanged(data);
}

void SystemStateModel::commandSlewToSelectedRadarTrack() {
    SystemStateData& data = m_currentStateData;
    if (data.opMode != OperationalMode::Surveillance) return;

    if (data.selectedRadarTrackId != 0) {
        qDebug() << "[MODEL] Commanding gimbal to slew to Radar Track ID:" << data.selectedRadarTrackId;
        emit dataChanged(data);
    }
}


// =================================================================
// 10. CONFIGURATION FILE MANAGEMENT
// =================================================================
bool SystemStateModel::saveZonesToFile(const QString& filePath) {
    QJsonObject rootObject;
    rootObject["zoneFileVersion"] = 1;

    rootObject["nextAreaZoneId"] = m_nextAreaZoneId;
    rootObject["nextSectorScanId"] = m_nextSectorScanId;
    rootObject["nextTRPId"] = m_nextTRPId;

    QJsonArray areaZonesArray;
    for (const auto& zone : m_currentStateData.areaZones) {
        QJsonObject zoneObj;
        zoneObj["id"] = zone.id;
        zoneObj["type"] = static_cast<int>(zone.type);
        zoneObj["isEnabled"] = zone.isEnabled;
        zoneObj["isFactorySet"] = zone.isFactorySet;
        zoneObj["isOverridable"] = zone.isOverridable;
        zoneObj["startAzimuth"] = zone.startAzimuth;
        zoneObj["endAzimuth"] = zone.endAzimuth;
        zoneObj["minElevation"] = zone.minElevation;
        zoneObj["maxElevation"] = zone.maxElevation;
        zoneObj["minRange"] = zone.minRange;
        zoneObj["maxRange"] = zone.maxRange;
        zoneObj["name"] = zone.name;
        areaZonesArray.append(zoneObj);
    }
    rootObject["areaZones"] = areaZonesArray;

    QJsonArray sectorScanZonesArray;
    for (const auto& zone : m_currentStateData.sectorScanZones) {
        QJsonObject zoneObj;
        zoneObj["id"] = zone.id;
        zoneObj["isEnabled"] = zone.isEnabled;
        zoneObj["az1"] = zone.az1;
        zoneObj["el1"] = zone.el1;
        zoneObj["az2"] = zone.az2;
        zoneObj["el2"] = zone.el2;
        zoneObj["scanSpeed"] = zone.scanSpeed;
        sectorScanZonesArray.append(zoneObj);
    }
    rootObject["sectorScanZones"] = sectorScanZonesArray;

    QJsonArray trpsArray;
    for (const auto& trp : m_currentStateData.targetReferencePoints) {
        QJsonObject trpObj;
        trpObj["id"] = trp.id;
        trpObj["locationPage"] = trp.locationPage;
        trpObj["trpInPage"] = trp.trpInPage;
        trpObj["azimuth"] = trp.azimuth;
        trpObj["elevation"] = trp.elevation;
        trpObj["haltTime"] = trp.haltTime;
        trpsArray.append(trpObj);
    }
    rootObject["targetReferencePoints"] = trpsArray;

    QJsonDocument doc(rootObject);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << filePath << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Zones saved successfully to" << filePath;
    return true;
}

bool SystemStateModel::loadZonesFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for reading:" << filePath << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse zones file:" << filePath << parseError.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Invalid format: Root is not a JSON object in" << filePath;
        return false;
    }

    QJsonObject rootObject = doc.object();

    int fileVersion = rootObject.value("zoneFileVersion").toInt(0);
    if (fileVersion > 1) {
        qWarning() << "Warning: Loading zones from a newer file version (" << fileVersion << "). Compatibility not guaranteed.";
    }

    m_currentStateData.areaZones.clear();
    m_currentStateData.sectorScanZones.clear();
    m_currentStateData.targetReferencePoints.clear();

    m_nextAreaZoneId = rootObject.value("nextAreaZoneId").toInt(1);
    m_nextSectorScanId = rootObject.value("nextSectorScanId").toInt(1);
    m_nextTRPId = rootObject.value("nextTRPId").toInt(1);

    if (rootObject.contains("areaZones") && rootObject["areaZones"].isArray()) {
        QJsonArray areaZonesArray = rootObject["areaZones"].toArray();
        for (const QJsonValue &value : areaZonesArray) {
            if (value.isObject()) {
                QJsonObject zoneObj = value.toObject();
                AreaZone zone;
                zone.id = zoneObj.value("id").toInt(-1);
                zone.type = static_cast<ZoneType>(zoneObj.value("type").toInt(static_cast<int>(ZoneType::Safety)));
                zone.isEnabled = zoneObj.value("isEnabled").toBool(false);
                zone.isFactorySet = zoneObj.value("isFactorySet").toBool(false);
                zone.isOverridable = zoneObj.value("isOverridable").toBool(false);
                zone.startAzimuth = static_cast<float>(zoneObj.value("startAzimuth").toDouble(0.0));
                zone.endAzimuth = static_cast<float>(zoneObj.value("endAzimuth").toDouble(0.0));
                zone.minElevation = static_cast<float>(zoneObj.value("minElevation").toDouble(0.0));
                zone.maxElevation = static_cast<float>(zoneObj.value("maxElevation").toDouble(0.0));
                zone.minRange = static_cast<float>(zoneObj.value("minRange").toDouble(0.0));
                zone.maxRange = static_cast<float>(zoneObj.value("maxRange").toDouble(0.0));
                zone.name = zoneObj.value("name").toString("");

                if (zone.id != -1) {
                    m_currentStateData.areaZones.push_back(zone);
                } else {
                    qWarning() << "Skipping invalid AreaZone entry during load (missing or invalid ID).";
                }
            }
        }
    }

    if (rootObject.contains("sectorScanZones") && rootObject["sectorScanZones"].isArray()) {
        QJsonArray sectorScanZonesArray = rootObject["sectorScanZones"].toArray();
        for (const QJsonValue &value : sectorScanZonesArray) {
            if (value.isObject()) {
                QJsonObject zoneObj = value.toObject();
                AutoSectorScanZone zone;
                zone.id = zoneObj.value("id").toInt(-1);
                zone.isEnabled = zoneObj.value("isEnabled").toBool(false);
                zone.az1 = static_cast<float>(zoneObj.value("az1").toDouble(0.0));
                zone.el1 = static_cast<float>(zoneObj.value("el1").toDouble(0.0));
                zone.az2 = static_cast<float>(zoneObj.value("az2").toDouble(0.0));
                zone.el2 = static_cast<float>(zoneObj.value("el2").toDouble(0.0));
                zone.scanSpeed = static_cast<float>(zoneObj.value("scanSpeed").toDouble(50.0));

                if (zone.id != -1) {
                    m_currentStateData.sectorScanZones.push_back(zone);
                } else {
                    qWarning() << "Skipping invalid SectorScanZone entry during load (missing or invalid ID).";
                }
            }
        }
    }

    if (rootObject.contains("targetReferencePoints") && rootObject["targetReferencePoints"].isArray()) {
        QJsonArray trpsArray = rootObject["targetReferencePoints"].toArray();
        for (const QJsonValue &value : trpsArray) {
            if (value.isObject()) {
                QJsonObject trpObj = value.toObject();
                TargetReferencePoint trp;
                trp.id = trpObj.value("id").toInt(-1);
                trp.locationPage = trpObj.value("locationPage").toInt(1);
                trp.trpInPage = trpObj.value("trpInPage").toInt(1);
                trp.azimuth = static_cast<float>(trpObj.value("azimuth").toDouble(0.0));
                trp.elevation = static_cast<float>(trpObj.value("elevation").toDouble(0.0));
                trp.haltTime = static_cast<float>(trpObj.value("haltTime").toDouble(0.0));

                if (trp.id != -1) {
                    m_currentStateData.targetReferencePoints.push_back(trp);
                } else {
                    qWarning() << "Skipping invalid TRP entry during load (missing or invalid ID).";
                }
            }
        }
    }

    updateNextIdsAfterLoad();

    qDebug() << "Zones loaded successfully from" << filePath;
    emit zonesChanged();
    return true;
}

// =================================================================
// SLOTS
// =================================================================
void SystemStateModel::onPlc21DataChanged(const Plc21PanelData &pData)
{
    SystemStateData newData = m_currentStateData;

    newData.menuUp = pData.menuUpSW;
    newData.menuDown = pData.menuDownSW;
    newData.menuVal = pData.menuValSw;

    newData.stationEnabled =  pData.enableStationSW;
    newData.gunArmed = pData.armGunSW;
    newData.gotoHomePosition = pData.homePositionSW;
    newData.ammoLoaded = pData.loadAmmunitionSW;

    newData.authorized = pData.authorizeSw;
    newData.enableStabilization = pData.enableStabilizationSW;
    newData.activeCameraIsDay = pData.switchCameraSW;

    switch (pData.fireMode) {
    case 0:
        newData.fireMode =FireMode::SingleShot;
        break;
    case 1:
        newData.fireMode =FireMode::ShortBurst;
        break;
    case 2:
        newData.fireMode =FireMode::LongBurst;
        break;
    default:
        newData.fireMode =FireMode::Unknown;
        break;
    }

    newData.gimbalSpeed = pData.speedSW;

    updateData(newData);
}

void SystemStateModel::onPlc42DataChanged(const Plc42Data &pData)
{
    SystemStateData newData = m_currentStateData;
    newData.upperLimitSensorActive = pData.stationUpperSensor;
    newData.lowerLimitSensorActive = pData.stationLowerSensor;
    newData.emergencyStopActive = pData.emergencyStopActive;

    newData.stationAmmunitionLevel = pData.ammunitionLevel;
    newData.stationInput1 = pData.stationInput1;
    newData.stationInput2 = pData.stationInput2;
    newData.stationInput3 = pData.stationInput3;

    newData.solenoidMode     = pData.solenoidMode;
    newData.gimbalOpMode     = pData.gimbalOpMode;
    newData.azimuthSpeed     = pData.azimuthSpeed;
    newData.elevationSpeed   = pData.elevationSpeed;
    newData.azimuthDirection = pData.azimuthDirection;
    newData.elevationDirection = pData.elevationDirection;
    newData.solenoidState     = pData.solenoidState;


    newData.solenoidState = pData.solenoidState;
    newData.resetAlarm = pData.resetAlarm;

    updateData(newData);
}

void SystemStateModel::onServoAzDataChanged(const ServoData &azData) {
    m_currentStateData.gimbalAz = azData.position* 0.0016179775280;;
    m_currentStateData.azMotorTemp = azData.motorTemp;
    m_currentStateData.azDriverTemp = azData.driverTemp;
    emit dataChanged(m_currentStateData);
    emit gimbalPositionChanged(m_currentStateData.gimbalAz, m_currentStateData.gimbalEl);
}

void SystemStateModel::onServoElDataChanged(const ServoData &elData) {
    m_currentStateData.gimbalEl = elData.position * (-0.0018);
    m_currentStateData.elMotorTemp = elData.motorTemp;
    m_currentStateData.elDriverTemp = elData.driverTemp;
    emit dataChanged(m_currentStateData);
    emit gimbalPositionChanged(m_currentStateData.gimbalAz, m_currentStateData.gimbalEl);
}

void SystemStateModel::onServoActuatorDataChanged(const ServoActuatorData &actuatorData)
{
    SystemStateData newData = m_currentStateData;
    newData.actuatorPosition = actuatorData.position_mm;
    updateData(newData);
}

void SystemStateModel::onLrfDataChanged(const LrfData &lrfData)
{
    SystemStateData newData = m_currentStateData;
    newData.lrfDistance = lrfData.lastDistance;
    newData.lrfSystemStatus = lrfData.isFault;
    newData.isOverTemperature = lrfData.isOverTemperature;
    updateData(newData);
}

void SystemStateModel::onDayCameraDataChanged(const DayCameraData &dayData)
{
    SystemStateData newData = m_currentStateData;

    newData.dayZoomPosition = dayData.zoomPosition;
    newData.dayCurrentHFOV = dayData.currentHFOV;
    newData.dayCameraConnected = dayData.isConnected;
    newData.dayCameraError = dayData.errorState;
    newData.dayCameraStatus = dayData.cameraStatus;
    updateData(newData);

}

void SystemStateModel::onGyroDataChanged(const ImuData &gyroData)
{
    SystemStateData newData = m_currentStateData;
    newData.imuRollDeg = gyroData.imuRollDeg;
    newData.imuPitchDeg = gyroData.imuPitchDeg;
    newData.imuYawDeg = gyroData.imuYawDeg;
    newData.temperature = gyroData.temperature;
    newData.AccelX = gyroData.accelX_g;
    newData.AccelY = gyroData.accelY_g;
    newData.AccelZ = gyroData.accelZ_g;
    newData.GyroX = gyroData.angRateX_dps;
    newData.GyroY = gyroData.angRateY_dps;
    newData.GyroZ = gyroData.angRateZ_dps;

     updateStationaryStatus(newData);

    updateData(newData);
}

void SystemStateModel::onLensDataChanged(const LensData &lensData)
{
    SystemStateData newData = m_currentStateData;
     updateData(newData);
}

void SystemStateModel::onNightCameraDataChanged(const NightCameraData &nightData)
{
    SystemStateData newData = m_currentStateData;

    newData.nightZoomPosition = nightData.digitalZoomLevel;
    newData.nightCurrentHFOV = nightData.currentHFOV;
    newData.nightCameraConnected = nightData.isConnected;
    newData.nightCameraError = nightData.errorState;
    newData.nightCameraStatus = nightData.cameraStatus;
    updateData(newData);

}

void SystemStateModel::onRadarPlotsUpdated(const QVector<RadarData> &plots) {
QVector<SimpleRadarPlot> converted;
converted.reserve(plots.size());

    for (const RadarData &p : plots) {
        SimpleRadarPlot s;
        s.id = p.id;
        s.azimuth = p.azimuthDegrees;
        s.range = p.rangeMeters;
        s.relativeCourse = p.relativeCourseDegrees;
        s.relativeSpeed = p.relativeSpeedMPS;
        converted.append(s);
    }
    if (m_currentStateData.radarPlots != converted) {
        m_currentStateData.radarPlots = converted;
        updateData(m_currentStateData);
    }
}

void SystemStateModel::onJoystickAxisChanged(int axis, float normalizedValue)
{
    SystemStateData newData = m_currentStateData;

    if (axis == 0){
        newData.joystickAzValue = normalizedValue;
    } else if (axis == 1){
        newData.joystickElValue = normalizedValue;
    }

    updateData(newData);
}

void SystemStateModel::onJoystickButtonChanged(int button, bool pressed)
{
    SystemStateData newData = m_currentStateData;

    updateData(newData);
}

void SystemStateModel::onJoystickHatChanged(int hat, int direction)
{
    SystemStateData newData = m_currentStateData;

    if (hat == 0) {
        newData.joystickHatDirection = direction;
    }

    updateData(newData);
}

// =================================================================
// PRIVATE HELPER METHODS
// =================================================================

void SystemStateModel::updateNextIdsAfterLoad() {
    int maxAreaId = 0;
    for(const auto& zone : m_currentStateData.areaZones) {
        maxAreaId = std::max(maxAreaId, zone.id);
    }
    m_nextAreaZoneId = std::max(m_nextAreaZoneId, maxAreaId + 1);

    int maxSectorId = 0;
    for(const auto& zone : m_currentStateData.sectorScanZones) {
        maxSectorId = std::max(maxSectorId, zone.id);
    }
    m_nextSectorScanId = std::max(m_nextSectorScanId, maxSectorId + 1);

    int maxTRPId = 0;
    for(const auto& trp : m_currentStateData.targetReferencePoints) {
        maxTRPId = std::max(maxTRPId, trp.id);
    }
    m_nextTRPId = std::max(m_nextTRPId, maxTRPId + 1);

    qDebug() << "Next IDs updated after load: AreaZone=" << m_nextAreaZoneId
             << ", SectorScan=" << m_nextSectorScanId << ", TRP=" << m_nextTRPId;
}

void SystemStateModel::recalculateDerivedAimpointData() {
    SystemStateData& data = m_currentStateData;

    float activeHfov = data.activeCameraIsDay ? static_cast<float>(data.dayCurrentHFOV) : static_cast<float>(data.nightCurrentHFOV);

    QPointF newReticlePosPx = ReticleAimpointCalculator::calculateReticleImagePositionPx(
        data.zeroingAzimuthOffset,
        data.zeroingElevationOffset,
        data.zeroingAppliedToBallistics,
        data.leadAngleOffsetAz,
        data.leadAngleOffsetEl,
        data.leadAngleCompensationActive,
        data.currentLeadAngleStatus,
        activeHfov,
        data.currentImageWidthPx,
        data.currentImageHeightPx
    );

    bool reticlePosChanged = false;
    if (!qFuzzyCompare(data.reticleAimpointImageX_px, static_cast<float>(newReticlePosPx.x()))) {
        data.reticleAimpointImageX_px = static_cast<float>(newReticlePosPx.x());
        reticlePosChanged = true;
    }
    if (!qFuzzyCompare(data.reticleAimpointImageY_px, static_cast<float>(newReticlePosPx.y()))) {
        data.reticleAimpointImageY_px = static_cast<float>(newReticlePosPx.y());
        reticlePosChanged = true;
    }

    QString oldLeadStatusText = data.leadStatusText;
    QString oldZeroingStatusText = data.zeroingStatusText;

    if (data.zeroingAppliedToBallistics) data.zeroingStatusText = "Z";
    else if (data.zeroingModeActive) data.zeroingStatusText = "ZEROING";
    else data.zeroingStatusText = "";

    if (data.leadAngleCompensationActive) {
        switch(data.currentLeadAngleStatus) {
            case LeadAngleStatus::On: data.leadStatusText = "LEAD ANGLE ON"; break;
            case LeadAngleStatus::Lag: data.leadStatusText = "LEAD ANGLE LAG"; break;
            case LeadAngleStatus::ZoomOut: data.leadStatusText = "ZOOM OUT"; break;
            default: data.leadStatusText = "";
        }
    } else {
        data.leadStatusText = "";
    }

    bool statusTextChanged = (oldLeadStatusText != data.leadStatusText) || (oldZeroingStatusText != data.zeroingStatusText);

    if (reticlePosChanged || statusTextChanged) {
        qDebug() << "SystemStateModel: Recalculated Reticle. PosPx X:" << data.reticleAimpointImageX_px
                 << "Y:" << data.reticleAimpointImageY_px
                 << "LeadTxt:" << data.leadStatusText << "ZeroTxt:" << data.zeroingStatusText;
        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::updateCameraOpticsAndActivity(int width, int height, float dayHfov, float nightHfov, bool isDayActive) {
    bool changed = false;
    if (m_currentStateData.currentImageWidthPx != width)   { m_currentStateData.currentImageWidthPx = width; changed=true; }
    if (m_currentStateData.currentImageHeightPx != height) { m_currentStateData.currentImageHeightPx = height; changed=true; }
    if (!qFuzzyCompare(static_cast<float>(m_currentStateData.dayCurrentHFOV), dayHfov)) { m_currentStateData.dayCurrentHFOV = dayHfov; changed=true; }
    if (!qFuzzyCompare(static_cast<float>(m_currentStateData.nightCurrentHFOV), nightHfov)) { m_currentStateData.nightCurrentHFOV = nightHfov; changed=true; }
    if (m_currentStateData.activeCameraIsDay != isDayActive) {m_currentStateData.activeCameraIsDay = isDayActive; changed=true;}

    if(changed){
        recalculateDerivedAimpointData();
        emit dataChanged(m_currentStateData);
    }
}

void SystemStateModel::updateLeadAngleSystemState(bool leadActive, LeadAngleStatus leadStatus, float calcLeadAzDeg, float calcLeadElDeg)
{
}

void SystemStateModel::updateCurrentScanName() {
    SystemStateData& data = m_currentStateData;
    QString newScanName = "";

    if (data.motionMode == MotionMode::AutoSectorScan) {
        auto it = std::find_if(data.sectorScanZones.begin(), data.sectorScanZones.end(),
                               [&](const AutoSectorScanZone& z){ return z.id == data.activeAutoSectorScanZoneId && z.isEnabled; });
        if (it != data.sectorScanZones.end()) {
            newScanName = QString("SCAN: SECTOR %1").arg(QString::number(it->id));
        } else {
            newScanName = "SCAN: SECTOR (none)";
        }
    } else if (data.motionMode == MotionMode::TRPScan) {
        newScanName = QString("SCAN: TRP PAGE %1").arg(data.activeTRPLocationPage);
    } else {
        newScanName = "";
    }

    if (data.currentScanName != newScanName) {
        data.currentScanName = newScanName;
    }
}

void SystemStateModel::processStateTransitions(const SystemStateData& oldData, SystemStateData& newData)
{
    if (newData.emergencyStopActive && !oldData.emergencyStopActive) {
        enterEmergencyStopMode();
        return;
    }

    if (!newData.emergencyStopActive && oldData.emergencyStopActive) {
        enterIdleMode();
        return;
    }

    if (newData.emergencyStopActive) {
        return;
    }

    if (!newData.stationEnabled && oldData.stationEnabled) {
        enterIdleMode();
        return;
    }
    if (newData.stationEnabled && !oldData.stationEnabled) {
        if (newData.opMode == OperationalMode::Idle) {
            enterSurveillanceMode();
        }
    }
}

void SystemStateModel::enterEmergencyStopMode() {
    SystemStateData& data = m_currentStateData;
    if (data.opMode == OperationalMode::EmergencyStop) return;

    qCritical() << "[MODEL] ENTERING EMERGENCY STOP MODE!";

    data.opMode = OperationalMode::EmergencyStop;
    data.motionMode = MotionMode::Idle;
    data.trackingActive = false;
    data.currentTrackingPhase = TrackingPhase::Off;
    data.trackerHasValidTarget = false;
    data.leadAngleCompensationActive = false;

    emit dataChanged(m_currentStateData);
}

void SystemStateModel::updateStationaryStatus(SystemStateData& data)
{
    double gyroMagnitude = std::sqrt(data.GyroX * data.GyroX +
                                     data.GyroY * data.GyroY +
                                     data.GyroZ * data.GyroZ);

    double accelMagnitude = std::sqrt(data.AccelX * data.AccelX +
                                      data.AccelY * data.AccelY +
                                      data.AccelZ * data.AccelZ);

    double accelDelta = std::abs(accelMagnitude - data.previousAccelMagnitude);
    data.previousAccelMagnitude = accelMagnitude;

    if (gyroMagnitude < STATIONARY_GYRO_LIMIT && accelDelta < STATIONARY_ACCEL_DELTA_LIMIT)
    {
        if (data.stationaryStartTime.isNull()) {
            data.stationaryStartTime = QDateTime::currentDateTime();
        }

        qint64 elapsedMs = data.stationaryStartTime.msecsTo(QDateTime::currentDateTime());
        if (elapsedMs > STATIONARY_TIME_MS) {
            data.isVehicleStationary = true;
        }
    }
    else
    {
        data.isVehicleStationary = false;
        data.stationaryStartTime = QDateTime();
    }
}
