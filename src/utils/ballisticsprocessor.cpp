#include "ballisticsprocessor.h"

#include <cmath>
#include <QDebug>


const float GRAVITY_MPS2 = 9.80665f; // Standard gravity

BallisticsProcessor::BallisticsProcessor() {}

LeadCalculationResult BallisticsProcessor::calculateLeadAngle(
    float targetRangeMeters,
    float targetAngularRateAzDegS,
    float targetAngularRateElDegS,
    float currentMuzzleVelocityMPS, // Now we might use this for a better TOF
    float projectileTimeOfFlightGuessS, // Still useful as an initial guess or if accurately provided
    float currentCameraFovHorizontalDegrees)
{
    LeadCalculationResult result;
    result.status = LeadAngleStatus::On; // Default to On if calculation proceeds

    if (targetRangeMeters <= 0.1f) { // Use a small epsilon for range
        result.status = LeadAngleStatus::Off;
        return result;
    }

    // --- Time of Flight (TOF) ---
    // If projectileTimeOfFlightGuessS is not provided accurately, calculate a basic one.
    // This is still very simplified as it ignores drag.
    float tofS = projectileTimeOfFlightGuessS;
    if (tofS <= 0.0f && currentMuzzleVelocityMPS > 0.0f) {
        tofS = targetRangeMeters / currentMuzzleVelocityMPS;
    }
    if (tofS <= 0.0f) { // Still no valid TOF
        result.status = LeadAngleStatus::Off;
        return result;
    }

    // --- Lead for Target Motion ---
    float targetAngularRateAzRadS = targetAngularRateAzDegS * (M_PI / 180.0);
    float targetAngularRateElRadS = targetAngularRateElDegS * (M_PI / 180.0);

    float motionLeadAzRad = targetAngularRateAzRadS * tofS;
    float motionLeadElRad = targetAngularRateElRadS * tofS;

    // --- Lead for Projectile Drop (Gravity) ---
    // Simple calculation ignoring air resistance. Weapon bore must be elevated.
    // Drop (meters) = 0.5 * g * TOF^2
    // Elevation angle (radians) to compensate for drop = atan(drop / range_horizontal)
    // Assuming targetRangeMeters is slant range, for very flat trajectories horizontal_range ~ slant_range.
    float projectileDropMeters = 0.5f * GRAVITY_MPS2 * tofS * tofS;
    float dropCompensationElRad = 0.0f;
    if (targetRangeMeters > 0.1f) { // Avoid division by zero
        dropCompensationElRad = std::atan(projectileDropMeters / targetRangeMeters);
    }

    // --- Total Lead ---
    // Azimuth lead is only from target motion (in this simplified model)
    float totalLeadAzRad = motionLeadAzRad;
    // Elevation lead is motion lead + compensation for projectile drop
    // If target is moving upwards (positive motionLeadElRad), we need to lead even higher.
    // Drop compensation always means aiming higher.
    float totalLeadElRad = motionLeadElRad + dropCompensationElRad;


    result.leadAzimuthDegrees = static_cast<float>(totalLeadAzRad * (180.0 / M_PI));
    result.leadElevationDegrees = static_cast<float>(totalLeadElRad * (180.0 / M_PI));

    // --- Apply Limits and Status ---
    bool lag = false;
    if (std::abs(result.leadAzimuthDegrees) > MAX_LEAD_ANGLE_DEGREES) {
        result.leadAzimuthDegrees = std::copysign(MAX_LEAD_ANGLE_DEGREES, result.leadAzimuthDegrees);
        lag = true;
    }
    // Note: Max lead for elevation might be different due to physical gun limits
    if (std::abs(result.leadElevationDegrees) > MAX_LEAD_ANGLE_DEGREES) { // Using same MAX_LEAD for El for now
        result.leadElevationDegrees = std::copysign(MAX_LEAD_ANGLE_DEGREES, result.leadElevationDegrees);
        lag = true;
    }

    if (lag) {
        result.status = LeadAngleStatus::Lag;
    }

    // Check for ZOOM OUT condition (only if not already lagging, perhaps)
    if (result.status != LeadAngleStatus::Lag) { // Don't override LAG with ZOOM_OUT if both occur
        // Check against half FOV. Assuming vertical FOV is proportional or using HFOV for both for simplicity
        float vfov_approx = currentCameraFovHorizontalDegrees * (1.0f); // Simplified: assuming square view for check
        if (currentCameraFovHorizontalDegrees > 0 && vfov_approx > 0) {
             if (std::abs(result.leadAzimuthDegrees) > (currentCameraFovHorizontalDegrees / 2.0f) ||
                 std::abs(result.leadElevationDegrees) > (vfov_approx / 2.0f) ) {
                 result.status = LeadAngleStatus::ZoomOut;
             }
        }
    }

    qDebug() << "Ballistics: R:" << targetRangeMeters << "TOF:" << tofS
             << "Rates Az:" << targetAngularRateAzDegS << "El:" << targetAngularRateElDegS
             << "DropCompElRad:" << dropCompensationElRad
             << "=> Lead Az:" << result.leadAzimuthDegrees << "El:" << result.leadElevationDegrees
             << "Status:" << static_cast<int>(result.status);

    return result;
}
