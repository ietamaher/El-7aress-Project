#ifndef BALLISTICSPROCESSOR_H
#define BALLISTICSPROCESSOR_H

#include "../models/systemstatemodel.h" // For LeadAngleStatus and AmmunitionType enum (if defined there)

// Forward declare if SystemStateData is complex or to reduce includes
// struct SystemStateData;
// enum class AmmunitionType;

 
struct LeadCalculationResult {
    float leadAzimuthDegrees   = 0.0f; // The calculated lead offset in Azimuth (degrees)
    float leadElevationDegrees = 0.0f; // The calculated lead offset in Elevation (degrees)
                                       // (This might include bullet drop + moving target lead)
    LeadAngleStatus status     = LeadAngleStatus::Off; // Status of the calculation
};
class BallisticsProcessor
{
public:
    BallisticsProcessor();

    // Calculates lead based on current target and system parameters
    LeadCalculationResult calculateLeadAngle(
        float targetRangeMeters,
        float targetAngularRateAzDegS, // Relative angular rate of target AZ
        float targetAngularRateElDegS, // Relative angular rate of target EL
        // AmmunitionType ammoType, // Requires ammo parameters (e.g., muzzle velocity, BC)
        float currentMuzzleVelocityMPS, // Example input
        float projectileTimeOfFlightGuessS, // An initial guess, can be iterative
        float currentCameraFovHorizontalDegrees // Needed for ZOOM_OUT check
    );

private:
    // Constants for calculation (simplified)
    const float MAX_LEAD_ANGLE_DEGREES = 10.0f; // Example maximum lead allowed

    // Helper methods for more complex ballistics if needed
    // float calculateTOF(float range, float muzzleVelocity, float projectileDragCoeff);
    // Point2D predictTargetPosition(float currentRange, float angularRateAz, float angularRateEl, float tof);
};

#endif // BALLISTICSPROCESSOR_H
