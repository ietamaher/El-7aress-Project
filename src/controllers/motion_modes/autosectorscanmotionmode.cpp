#include "autosectorscanmotionmode.h"
#include "../gimbalcontroller.h" // For GimbalController
#include "../../devices/servodriverdevice.h" // For ServoDriverDevice
#include <QDebug>
#include <cmath>    // For std::abs
#include <QtGlobal> // For qBound

AutoSectorScanMotionMode::AutoSectorScanMotionMode(QObject* parent)
    : GimbalMotionModeBase(parent), m_scanZoneSet(false), m_movingToPoint2(true)
{
    // --- UNIFORMITY: Configure PID gains in the constructor ---
    // Gains for a smooth, continuous scan might be different from TRP scan.
    // We'll use the same values for now, but they can be tuned independently.
    m_azPid.Kp = 1; m_azPid.Ki = 0.01; m_azPid.Kd = 0.05; m_azPid.maxIntegral = 20.0;
    m_elPid.Kp = 1; m_elPid.Ki = 0.01; m_elPid.Kd = 0.05; m_elPid.maxIntegral = 20.0;
}

void AutoSectorScanMotionMode::enterMode(GimbalController* controller) {
    qDebug() << "[AutoSectorScanMotionMode] Enter";
    if (!m_scanZoneSet || !m_activeScanZone.isEnabled) {
        qWarning() << "AutoSectorScanMotionMode: No active scan zone set or zone disabled. Exiting scan.";
        if (controller) controller->setMotionMode(MotionMode::Idle);
        return;
    }

    // Reset PID controllers to start fresh
    m_azPid.reset();
    m_elPid.reset();
    
    // Set the initial direction and target
    m_movingToPoint2 = true; // Always start by moving towards point 2
    m_targetAz = m_activeScanZone.az2;
    m_targetEl = m_activeScanZone.el2;

    if (controller) {
        // Set a slower, smoother acceleration for scanning motion
        if (auto azServo = controller->azimuthServo()) setAcceleration(azServo, 1000000);
        if (auto elServo = controller->elevationServo()) setAcceleration(elServo, 1000000);
    }
}

void AutoSectorScanMotionMode::exitMode(GimbalController* controller) {
    qDebug() << "[AutoSectorScanMotionMode] Exit";
    stopServos(controller);
    m_scanZoneSet = false; // Reset state for the next time the mode is activated
}

void AutoSectorScanMotionMode::setActiveScanZone(const AutoSectorScanZone& scanZone) {
    m_activeScanZone = scanZone;
    m_scanZoneSet = true;
    qDebug() << "[AutoSectorScanMotionMode] Active scan zone set to ID:" << scanZone.id;
}


// ===================================================================================
// =================== FULLY UPDATED AND UNIFORM UPDATE METHOD =====================
// ===================================================================================
/*void AutoSectorScanMotionMode::update(GimbalController* controller) {
    // Top-level guard clauses for mode-specific state
    if (!controller || !m_scanZoneSet || !m_activeScanZone.isEnabled) {
        // If the scan zone is disabled while we are in this mode, stop and let the
        // main controller logic switch to Idle.
        stopServos(controller);
        if (controller && m_scanZoneSet && !m_activeScanZone.isEnabled) {
            controller->setMotionMode(MotionMode::Idle);
        }
        return;
    }

    SystemStateData data = controller->systemStateModel()->data();
    double errAz = m_targetAz - data.gimbalAz;
    double errEl = m_targetEl - data.gimbalEl;

    // --- ROBUSTNESS: Use a 2D distance check for arrival ---
    double distanceToTarget = std::sqrt( errEl * errEl); //errAz * errAz +

    if (distanceToTarget < ARRIVAL_THRESHOLD_DEG) {
        qDebug() << "AutoSectorScan: Reached point" << (m_movingToPoint2 ? "2" : "1");
        
        // Toggle direction
        m_movingToPoint2 = !m_movingToPoint2;
        if (m_movingToPoint2) {
            m_targetAz = m_activeScanZone.az2;
            m_targetEl = m_activeScanZone.el2;
        } else {
            m_targetAz = m_activeScanZone.az1;
            m_targetEl = m_activeScanZone.el1;
        }
        
        // Reset PID controllers to prevent integral windup from any previous steady-state error
        m_azPid.reset();
        m_elPid.reset();
        
        // Recalculate error for the new target for this update cycle
        errAz = m_targetAz - data.gimbalAz;
        errEl = m_targetEl - data.gimbalEl;
    }

    // --- UNIFORMITY: This block is now identical in pattern to other modes ---
    // Outer Loop: PID calculates the DESIRED world velocity to move to the next scan point
    double desiredAzVelocity = 0; //pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
    double desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);
    
    // Limit the velocity vector's magnitude to the defined scan speed
    double desiredSpeedDegS = m_activeScanZone.scanSpeed;
    double totalVelocityMag = std::sqrt(desiredAzVelocity * desiredAzVelocity + desiredElVelocity * desiredElVelocity);
    if (totalVelocityMag > desiredSpeedDegS && desiredSpeedDegS > 0) {
        desiredAzVelocity = (desiredAzVelocity / totalVelocityMag) * desiredSpeedDegS;
        desiredElVelocity = (desiredElVelocity / totalVelocityMag) * desiredSpeedDegS;
    }
     qDebug() << "AreaScan: desiredElVelocity " << desiredElVelocity << " totalVelocityMag " << totalVelocityMag   ;    
    // Pass the final desired world velocity to the base class for stabilization and hardware output.
    sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity);
}
*/

// ===================================================================================
// =================== REFACTORED UPDATE METHOD WITH MOTION PROFILING ================
// ===================================================================================
void AutoSectorScanMotionMode::update(GimbalController* controller) {
    // Top-level guard clauses remain the same
    if (!controller || !m_scanZoneSet || !m_activeScanZone.isEnabled) {
        stopServos(controller);
        if (controller && m_scanZoneSet && !m_activeScanZone.isEnabled) {
            controller->setMotionMode(MotionMode::Idle);
        }
        return;
    }

    SystemStateData data = controller->systemStateModel()->data();
    double errAz = m_targetAz - data.gimbalAz; // Azimuth still uses encoder
    double errEl = m_targetEl - data.imuPitchDeg; // Elevation now uses IMU Pitch

    // Use a 2D distance for robust arrival and deceleration checks.
    // Corrected to use both Az and El errors for 2D distance.
    double distanceToTarget = std::sqrt(errAz * errAz + errEl * errEl); //std::sqrt( errEl * errEl); //errAz * errAz +

    // --- 1. ENDPOINT HANDLING LOGIC ---
    // Check if we have arrived at the current target.
    if (distanceToTarget < ARRIVAL_THRESHOLD_DEG) {
        qDebug() << "AutoSectorScan: Reached point" << (m_movingToPoint2 ? "2" : "1");
        
        // Toggle direction for the next sweep
        m_movingToPoint2 = !m_movingToPoint2;
        if (m_movingToPoint2) {
            m_targetAz = m_activeScanZone.az2;
            m_targetEl = m_activeScanZone.el2;
        } else {
            m_targetAz = m_activeScanZone.az1;
            m_targetEl = m_activeScanZone.el1;
        }
        
        // Reset PIDs for the new sweep. This is crucial to prevent integral windup 
        // and ensure a clean start for the next deceleration phase.
        m_azPid.reset();
        m_elPid.reset();
        
        // Recalculate error and distance for the new target immediately
        errAz = m_targetAz - data.gimbalAz;
        errEl = m_targetEl - data.imuPitchDeg; // Recalculate with IMU Pitch
        distanceToTarget = std::sqrt(errAz * errAz + errEl * errEl); //        distanceToTarget = std::sqrt( errEl * errEl); //errAz * errAz +
    }

    // --- 2. MOTION PROFILE LOGIC (THE FIX) ---
    double desiredAzVelocity = 0.0;
    double desiredElVelocity = 0.0;

    // If the scan speed is zero or negative, just use PID to hold position at target.
    if (m_activeScanZone.scanSpeed <= 0) {
         desiredAzVelocity = pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
         desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);
    }
    // Check if we are in the "Deceleration Zone"
    else if (distanceToTarget < DECELERATION_DISTANCE_DEG) {
        // STATE: DECELERATION. We are close to the target.
        // Use the PID controller to slow down and stop smoothly at the endpoint.
        // This is the *correct* use of a position PID in this context.
        qDebug() << "AreaScan: Decelerating with PID. Distance:" << distanceToTarget;
        desiredAzVelocity = pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
        desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);

    } else {
        // STATE: CRUISING. We are far from the target.
        // Move at the constant defined scan speed.
        
        // Calculate the direction vector (a "unit vector") towards the target.
        // The total distance is 'distanceToTarget', which we already calculated.
        double dirAz = errAz / distanceToTarget;
        double dirEl = errEl / distanceToTarget;

        // Set the desired velocity to be the scan speed in the correct direction.
        desiredAzVelocity = dirAz * m_activeScanZone.scanSpeed * 0.1 ;
        desiredElVelocity = dirEl * m_activeScanZone.scanSpeed  * 0.1;

        // IMPORTANT: While cruising, we must continuously reset the PID controller.
     // If we don't, the integral term will build up massively ("windup")
        // because there's a large, persistent error, and it will cause a huge
        // velocity jump when we switch to the deceleration state.
        m_azPid.reset();
        m_elPid.reset();
    }
    
    qDebug() << "AreaScan: Desired Vel (Az, El):" << desiredAzVelocity << "," << desiredElVelocity;

    // The velocity is already at the scan speed, so no need to limit it again.
    // The PID output in the deceleration phase will naturally be less than the scan speed.
    // If you need a hard safety limit, you can re-apply it, but the logic above should suffice.

    // Pass the final desired world velocity to the base class.
    sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity);
}
