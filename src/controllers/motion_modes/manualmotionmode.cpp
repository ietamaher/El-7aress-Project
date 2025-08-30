#include "manualmotionmode.h"
#include "../gimbalcontroller.h"
#include "models/systemstatemodel.h"
#include <QDebug>
#include <QtGlobal>

ManualMotionMode::ManualMotionMode(QObject* parent)
    : GimbalMotionModeBase(parent)
{
}

void ManualMotionMode::enterMode(GimbalController* controller)
{
    qDebug() << "[ManualMotionMode] Enter";
    
    if (!controller) return;
    m_currentAzVelocityCmd = 0.0;
    m_currentElVelocityCmd = 0.0;   

    m_filteredAzJoystick = 0.0;
    m_filteredElJoystick = 0.0;

    // Set initial acceleration for both servos
    if (auto azServo = controller->azimuthServo()) {
        setAcceleration(azServo);
    }
    if (auto elServo = controller->elevationServo()) {
        setAcceleration(elServo);
    }
}

void ManualMotionMode::exitMode(GimbalController* controller)
{
    qDebug() << "[ManualMotionMode] Exit";
    stopServos(controller);
}

 
void ManualMotionMode::update(GimbalController* controller)
{
    if (!checkSafetyConditions(controller)) {
        stopServos(controller);
        return;
    }

    SystemStateData data = controller->systemStateModel()->data();

    // 1. Calculate TARGET velocity in the motor's native units
    static constexpr double MAX_SPEED_HZ = 25000.0;
    double speedPercent = data.gimbalSpeed / 100.0;
    double maxCurrentSpeedHz = speedPercent * MAX_SPEED_HZ;

    // 2. Get processed joystick values
    double rawAzJoystick = data.joystickAzValue;
    double rawElJoystick = data.joystickElValue;

    double shaped_stick_Azinput = processJoystickInput(rawAzJoystick, m_filteredAzJoystick);
    double shaped_stick_Elinput = processJoystickInput(rawElJoystick, m_filteredElJoystick);

    // 3. Calculate target speeds
    double targetAzSpeedHz = shaped_stick_Azinput * maxCurrentSpeedHz;
    double targetElSpeedHz = shaped_stick_Elinput * maxCurrentSpeedHz; 

    // 2. Define our control parameters
    static constexpr double MAX_ACCEL_HZ_PER_SEC = 15000.0; // Accel in Hz/s^2
    static constexpr double DEADBAND_HZ = 100.0;           // deadband idea

    // Apply deadband to the target speed
    if (std::abs(targetAzSpeedHz) < DEADBAND_HZ) targetAzSpeedHz = 0.0;
    if (std::abs(targetElSpeedHz) < DEADBAND_HZ) targetElSpeedHz = 0.0;

    // 3. Apply the STATE-AWARE Rate Limiter
    double maxChangeHz = MAX_ACCEL_HZ_PER_SEC * UPDATE_INTERVAL_S;

    // --- Azimuth Axis ---
    if (std::abs(targetAzSpeedHz) > std::abs(m_currentAzSpeedCmd_Hz)) {
        // ACCELERATING: Ramp up smoothly
        double error = targetAzSpeedHz - m_currentAzSpeedCmd_Hz;
        m_currentAzSpeedCmd_Hz += qBound(-maxChangeHz, error, maxChangeHz);
    } else {
        // DECELERATING / STOPPING: Command directly for a crisp response
        m_currentAzSpeedCmd_Hz = targetAzSpeedHz;
    }

    // --- Elevation Axis ---
    if (std::abs(targetElSpeedHz) > std::abs(m_currentElSpeedCmd_Hz)) {
        // ACCELERATING: Ramp up smoothly
        double error = targetElSpeedHz - m_currentElSpeedCmd_Hz;
        m_currentElSpeedCmd_Hz += qBound(-maxChangeHz, error, maxChangeHz);
    } else {
        // DECELERATING / STOPPING: Command directly
        m_currentElSpeedCmd_Hz = targetElSpeedHz;
    }

    // 4. Convert back to deg/s for the stabilization function
    const double azStepsPerDegree = 222500.0 / 360.0;
    const double elStepsPerDegree = 200000.0 / 360.0;

    double azVelocityDegS = m_currentAzSpeedCmd_Hz / azStepsPerDegree;
    double elVelocityDegS = m_currentElSpeedCmd_Hz / elStepsPerDegree;

    // 5. Send final command
    sendStabilizedServoCommands(controller, azVelocityDegS, elVelocityDegS);

}

double ManualMotionMode::processJoystickInput(double rawInput, double& filteredValue) {
    const double alpha = 0.4;
    const double exponent = 1.5;

    // If joystick is released (input is near zero), reset the filter to prevent drift.
    if (qFuzzyCompare(rawInput, 0.0)) {
        filteredValue = 0.0;
        return 0.0;
    }
    
    filteredValue = (alpha * rawInput) + (1.0 - alpha) * filteredValue;
    
    double shaped = std::pow(std::abs(filteredValue), exponent);
    return (filteredValue < 0) ? -shaped : shaped;
}





/*
    // The top-level safety check is now performed by GimbalController::update()
    // before this method is called. We can proceed directly to the mode's logic.

    SystemStateData data = controller->systemStateModel()->data();
    
    float angularVelocity = data.gimbalSpeed * SPEED_MULTIPLIER;
    
    double azInput = data.joystickAzValue;
    double elInput = data.joystickElValue;

    if (!checkElevationLimits(data.gimbalEl, elInput, 
                             data.upperLimitSensorActive, 
                             data.lowerLimitSensorActive)) {
        elInput = 0.0;
        qDebug() << "[ManualMotionMode] Elevation limit reached";
    }

     // The SPEED_MULTIPLIER now directly converts the speed setting to max deg/s
    // Example: If gimbalSpeed is 1-10, multiplier could be 3.0 to get a max of 30 deg/s
    static constexpr float SPEED_MULTIPLIER = 1.0f;
    float maxSpeedDegS = data.gimbalSpeed * SPEED_MULTIPLIER;
    double targetAzVelocity = data.joystickAzValue * maxSpeedDegS;
    double targetElVelocity = data.joystickElValue * maxSpeedDegS;
    
    // 2. Apply a proper, state-aware Rate Limiter.
    double maxVelocityChange = MAX_MANUAL_ACCEL_DEGS2 * UPDATE_INTERVAL_S;

    // --- Azimuth Axis ---
    if (std::abs(targetAzVelocity) > std::abs(m_currentAzVelocityCmd)) {
        // CASE 1: ACCELERATING or REVERSING
        // We are commanding a higher speed than the current one.
        // Ramp up smoothly to prevent current spikes.
        double error = targetAzVelocity - m_currentAzVelocityCmd;
        m_currentAzVelocityCmd += qBound(-maxVelocityChange, error, maxVelocityChange);
    } else {
        // CASE 2: DECELERATING or STOPPING
        // We are commanding a lower speed, or zero.
        // In this case, we command the target speed *directly*.
        // This allows the motor driver's own deceleration ramp to take over,
        // resulting in a crisp, predictable stop.
        m_currentAzVelocityCmd = targetAzVelocity;
    }

    // --- Elevation Axis (identical logic) ---
    if (std::abs(targetElVelocity) > std::abs(m_currentElVelocityCmd)) {
        // CASE 1: ACCELERATING or REVERSING
        double error = targetElVelocity - m_currentElVelocityCmd;
        m_currentElVelocityCmd += qBound(-maxVelocityChange, error, maxVelocityChange);
    } else {
        // CASE 2: DECELERATING or STOPPING
        m_currentElVelocityCmd = targetElVelocity;
    }

    // 3. Send the properly-ramped command to the driver.
    sendStabilizedServoCommands(controller, int(m_currentAzVelocityCmd), m_currentElVelocityCmd);
    qDebug() << "Joystick El Value  & desiredElVelocity " << data.joystickElValue << " | " <<  m_currentElVelocityCmd;

    //double desiredAzVelocity = (data.joystickAzValue != 0.0) ? std::copysign(maxSpeedDegS, data.joystickAzValue) : 0.0;
    //double desiredElVelocity = (data.joystickElValue != 0.0) ? std::copysign(maxSpeedDegS, data.joystickElValue) : 0.0;
   
    // Pass the desired world velocity to the base class.
    // It will handle stabilization, limit checks, and hardware communication.
     //   sendStabilizedServoCommands(controller, 0.0, 0.0);
    //sendStabilizedServoCommands(controller, desiredAzVelocity, desiredElVelocity);
    //LOG_TS_ELAPSED("ManualMotionMode", "Processed ManualMotionMode");

*/
