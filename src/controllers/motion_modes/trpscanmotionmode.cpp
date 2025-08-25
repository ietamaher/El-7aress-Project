#include "trpscanmotionmode.h"
#include "../gimbalcontroller.h" // For GimbalController and SystemStateData
#include <QDebug>
#include <cmath> // For std::sqrt

TRPScanMotionMode::TRPScanMotionMode()
    : m_currentState(State::Idle)
    , m_currentTrpIndex(0)
{
    // Configure PID gains for responsive and smooth stopping at waypoints.
    // These might need tuning based on your gimbal\"s physical characteristics.
    m_azPid.Kp = 1.2; m_azPid.Ki = 0.1; m_azPid.Kd = 0.1; m_azPid.maxIntegral = 20.0;
    m_elPid.Kp = 1.2; m_elPid.Ki = 0.1; m_elPid.Kd = 0.1; m_elPid.maxIntegral = 20.0;
}

void TRPScanMotionMode::setActiveTRPPage(const std::vector<TargetReferencePoint>& trpPage)
{
    qDebug() << "[TRPScanMotionMode] Active TRP page set with" << trpPage.size() << "points.";
    m_trpPage = trpPage;
    // Reset progress, ready to start when `enterMode` is called.
    m_currentTrpIndex = 0;
    m_currentState = m_trpPage.empty() ? State::Idle : State::Moving;
}

void TRPScanMotionMode::enterMode(GimbalController* controller)
{
    qDebug() << "[TRPScanMotionMode] Enter";
    if (m_trpPage.empty()) {
        qWarning() << "TRPScanMotionMode: No TRP page set. Exiting scan.";
        // The GimbalController will set mode to Idle, so we just stop.
        stopServos(controller);
        m_currentState = State::Idle;
        return;
    }

    // Reset state to start the path from the beginning
    m_currentTrpIndex = 0;
    m_currentState = State::Moving;
    m_azPid.reset();
    m_elPid.reset();

    if (controller) {
        // Set an aggressive acceleration for point-to-point moves
        if (auto azServo = controller->azimuthServo()) setAcceleration(azServo, 200000);
        if (auto elServo = controller->elevationServo()) setAcceleration(elServo, 200000);
    }
    qDebug() << "[TRPScanMotionMode] Starting path, moving to point 0.";
}

void TRPScanMotionMode::exitMode(GimbalController* controller)
{
    qDebug() << "[TRPScanMotionMode] Exit";
    stopServos(controller);
    m_currentState = State::Idle;
}

// ===================================================================================
// =================== REFACTORED UPDATE METHOD WITH MOTION PROFILING ================
// ===================================================================================
void TRPScanMotionMode::update(GimbalController* controller)
{
    if (!controller) return;

    // --- Main State Machine Logic ---
    switch (m_currentState)
    {
        case State::Idle: {
            stopServos(controller);
            return; // Do nothing
        }

        case State::Halted: {
            // We are waiting at a TRP. Check if the halt timer has expired.
            const auto& currentTrp = m_trpPage[m_currentTrpIndex];
            
            // Assuming `haltTime` is in seconds.
            if (m_haltTimer.isValid() && m_haltTimer.elapsed() >= static_cast<qint64>(currentTrp.haltTime * 1000.0)) {
                qDebug() << "[TRPScanMotionMode] Halt time finished at point" << m_currentTrpIndex;
                
                // Advance to the next point in the path
                m_currentTrpIndex++;

                // --- MODIFIED LOGIC FOR LOOPING ---
                if (m_currentTrpIndex >= m_trpPage.size()) {
                    // Reached the end of the path. Loop back to the beginning.
                    qDebug() << "[TRPScanMotionMode] Path loop finished. Returning to point 0.";
                    m_currentTrpIndex = 0; 
                }
                // --- END OF MODIFIED LOGIC ---

                // This part now executes for both advancing to the next point and looping back.
                qDebug() << "[TRPScanMotionMode] Moving to point" << m_currentTrpIndex;
                m_currentState = State::Moving;
                // Reset PIDs for the next deceleration phase.
                m_azPid.reset();
                m_elPid.reset();
            }
            // While halted, the servos should remain stopped.
            return;
        }

        case State::Moving: {
            // This is the core motion logic.
            if (m_currentTrpIndex >= m_trpPage.size()) {
                m_currentState = State::Idle; // Safety guard
                return;
            }

            const auto& targetTrp = m_trpPage[m_currentTrpIndex];
            SystemStateData data = controller->systemStateModel()->data();

            double errAz = targetTrp.azimuth - data.gimbalAz; // Azimuth still uses encoder
            double errEl = targetTrp.elevation - data.imuPitchDeg; // Elevation now uses IMU Pitch

            // Normalize Azimuth error for shortest path
            while (errAz > 180.0)  errAz -= 360.0;
            while (errAz < -180.0) errAz += 360.0;

            // Corrected to use both Az and El errors for 2D distance.
            //double distanceToTarget = std::sqrt(errAz * errAz + errEl * errEl);
            double distanceToTarget = std::sqrt(errEl * errEl); //errAz * errAz + 
            // --- 1. ARRIVAL CHECK ---
            if (distanceToTarget < ARRIVAL_THRESHOLD_DEG) {
                qDebug() << "[TRPScanMotionMode] Arrived at point" << m_currentTrpIndex;
                stopServos(controller);
                m_currentState = State::Halted;
                m_haltTimer.start(); // Start the halt timer
                return; // End this update cycle
            }

            // --- 2. MOTION PROFILE LOGIC ---
            double desiredAzVelocity = 0.0;
            double desiredElVelocity = 0.0;
            // Assuming your struct has a `scanSpeed` member. Adjust if name is different.
            double travelSpeed = 15;//targetTrp.scanSpeed; 

            if (travelSpeed <= 0) { // If speed is 0, just use PID to go to position
                 desiredAzVelocity = pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
                 desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);
            } else if (distanceToTarget < DECELERATION_DISTANCE_DEG) {
                // DECELERATION ZONE: Use PID to slow down smoothly
                qDebug() << "TRP: Decelerating. Dist:" << distanceToTarget;
                desiredAzVelocity = pidCompute(m_azPid, errAz, UPDATE_INTERVAL_S);
                desiredElVelocity = pidCompute(m_elPid, errEl, UPDATE_INTERVAL_S);
            } else {
                // CRUISING ZONE: Move at constant speed
                double dirAz = errAz / distanceToTarget;
                double dirEl = errEl / distanceToTarget;
                desiredAzVelocity = dirAz * travelSpeed;
                desiredElVelocity = dirEl * travelSpeed;

                // CRITICAL: Reset PID during cruise to prevent integral windup
                m_azPid.reset();
                m_elPid.reset();
            }

            sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity);
            break;
        }
    }
}
