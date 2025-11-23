#include "gimbalmotionmodebase.h"
#include "../gimbalcontroller.h"
#include "devices/servodriverdevice.h"
#include "models/systemstatemodel.h"
#include <QDebug>
#include <algorithm>

// =========================================================================
// REGISTER DEFINITIONS for AZD-KX Direct Data Operation (from your manual)
// =========================================================================
namespace AzdReg {
    // These are all 16-bit register addresses
    constexpr quint16 OpType      = 0x005A; // Operation Type (2 registers)
    constexpr quint16 OpSpeed     = 0x005E; // Operating Speed (2 registers, signed +/- 4,000,000 Hz)
    constexpr quint16 OpAccel     = 0x0060; // Starting/Changing Speed Rate (2 registers)
    constexpr quint16 OpDecel     = 0x0062; // Stopping Deceleration (2 registers)
    constexpr quint16 OpTrigger   = 0x0066; // Trigger (2 registers)
}
 

void GimbalMotionModeBase::configureVelocityMode(ServoDriverDevice* driverInterface)
{
    if (!driverInterface) return;

    // This function sets up the driver for continuous speed control.
    // It should be called from the enterMode() of each motion class.

    // 1. Set Operation Type to 16: Continuous operation (speed control)
    QVector<quint16> opTypeData = {0x0000, 0x0010}; // 16 is 0x10
    driverInterface->writeData(AzdReg::OpType, opTypeData);

    // 2. Set a reasonable default acceleration/deceleration rate.
    // From manual, 1,000,000 = 1000 kHz/s. Let's set 1500 kHz/s = 1,500,000
    quint32 accel = 150000;//0;
    QVector<quint16> accelData = {
        static_cast<quint16>((accel >> 16) & 0xFFFF),
        static_cast<quint16>(accel & 0xFFFF)
    };
    driverInterface->writeData(AzdReg::OpAccel, accelData);
    driverInterface->writeData(AzdReg::OpDecel, accelData); // Use same for decel
}

void GimbalMotionModeBase::writeVelocityCommand(ServoDriverDevice* driverInterface, 
                                              double finalVelocity, 
                                              double scalingFactor)
{
    if (!driverInterface) return;

    // 1. Convert physical velocity (deg/s) to motor speed (Hz)
    // The OpSpeed register is a SIGNED 32-bit integer.
    qint32 speedHz = static_cast<qint32>(finalVelocity * scalingFactor);

    // 2. Split the signed 32-bit speed into two 16-bit registers
    QVector<quint16> speedData = {
        static_cast<quint16>((speedHz >> 16) & 0xFFFF), // Upper 16 bits
        static_cast<quint16>(speedHz & 0xFFFF)        // Lower 16 bits
    };
    driverInterface->writeData(AzdReg::OpSpeed, speedData);

    // 3. Trigger the speed update.
    // From manual, trigger value -4 (FFFF FFFCh) updates the operating speed.
    QVector<quint16> triggerData = {0xFFFF, 0xFFFC};
    driverInterface->writeData(AzdReg::OpTrigger, triggerData);
}

// --- NEW: Gyro Bias Estimation --- 
void GimbalMotionModeBase::updateGyroBias(const SystemStateData& systemState)
{
    // This function should be called from the main controller update loop
    // before any motion mode update is called.

    // Static variables to maintain state across calls
    static double sum = 0;
    static int count = 0;
    static QDateTime lastResetTime = QDateTime::currentDateTime();

    // Only estimate bias if the vehicle is stationary
    if (systemState.isVehicleStationary) {
        sum += systemState.GyroZ; // Accumulate raw GyroZ data
        count++;

        // Average over a set number of samples (e.g., 50 samples at 20Hz = 2.5s)
        // This 50-sample window is based on the 50ms update interval (50 * 0.05s = 2.5s)
        if (count >= 50) {
            m_gyroBiasZ = sum / count; // Calculate the new bias
            sum = 0; // Reset accumulator
            count = 0; // Reset counter
            lastResetTime = QDateTime::currentDateTime(); // Reset timer
            qDebug() << "[Gimbal] New Gyro Z Bias calculated:" << m_gyroBiasZ;
        }
    } else {
        // If we start moving, reset the accumulator and counter
        sum = 0;
        count = 0;
        lastResetTime = QDateTime::currentDateTime();
    }
}

void GimbalMotionModeBase::sendStabilizedServoCommands(GimbalController* controller,
                                                       double desiredAzVelocity,
                                                       double desiredElVelocity)
{
    // --- Step 1: Get current system state ---
    SystemStateData systemState = controller->systemStateModel()->data();

    double finalAzVelocity = desiredAzVelocity;
    double finalElVelocity = desiredElVelocity;

    // --- Step 2: Apply stabilization if enabled ---
    if (systemState.enableStabilization) { // Changed from enableStabilization to isStabilizationActive
        double azCorrection = 0.0;
        double elCorrection = 0.0;

        // Call the helper function with the necessary data from the system state.
        calculateStabilizationCorrection(systemState.gimbalAz, systemState.gimbalEl,
                                         systemState.GyroX, systemState.GyroY, systemState.GyroZ,
                                         azCorrection, elCorrection);

        // Add the calculated correction to the desired velocity.
        finalAzVelocity += azCorrection;
        finalElVelocity += elCorrection;
    }

    // --- Step 3: Apply system-wide velocity limits ---
    finalAzVelocity = qBound(-MAX_VELOCITY, finalAzVelocity, MAX_VELOCITY);
    finalElVelocity = qBound(-MAX_VELOCITY, finalElVelocity, MAX_VELOCITY);

    // --- Step 4: Convert to servo steps and send commands ---
    const double azStepsPerDegree = 222500.0 / 360.0;
    const double elStepsPerDegree = 200000.0 / 360.0;

    // --- CRITICAL FIX from previous review: Send the FINAL calculated velocities ---
    if (auto azServo = controller->azimuthServo()) {
        writeVelocityCommand(azServo, finalAzVelocity, azStepsPerDegree);
    }
    if (auto elServo = controller->elevationServo()) {
        writeVelocityCommand(elServo, -finalElVelocity, elStepsPerDegree);
    }
}

double GimbalMotionModeBase::pidCompute(PIDController& pid, double error, double setpoint, double measurement, bool derivativeOnMeasurement, double dt)
{
    // Proportional term
    double proportional = pid.Kp * error;

    // Integral term with windup protection
    pid.integral += error * dt;
    pid.integral = qBound(-pid.maxIntegral, pid.integral, pid.maxIntegral);
    double integral = pid.Ki * pid.integral;

    // Derivative term
    double derivative = 0.0;
    if (dt > 1e-6) {
        if (derivativeOnMeasurement) {
            // Derivative on Measurement: avoids "kick" on setpoint changes.
            // Note the negative sign: the derivative must oppose the direction of change.
            derivative = -pid.Kd * (measurement - pid.previousMeasurement) / dt;
        } else {
            // Derivative on Error: the classic implementation.
            derivative = pid.Kd * (error - pid.previousError) / dt;
        }
    }

    // Store history for the next cycle
    pid.previousError = error;
    pid.previousMeasurement = measurement;

    return proportional + integral + derivative;
}

// Implementation of the original, simpler PID function (overload)
// This function now calls the more advanced one with the correct parameters.
double GimbalMotionModeBase::pidCompute(PIDController& pid, double error, double dt)
{
    // We call the main function with dummy values for setpoint/measurement
    // and explicitly set derivativeOnMeasurement to false.
    // Setpoint and measurement are not used when derivativeOnMeasurement is false,
    // so their values don't matter.
    return pidCompute(pid, error, 0.0, 0.0, false, dt);
}

 
void GimbalMotionModeBase::stopServos(GimbalController* controller)
{
    if (!controller) return;
    // Send a zero-velocity command through the new architecture.
    // If stabilization is on, this will still actively hold the gimbal steady.
    sendStabilizedServoCommands(controller, 0.0, 0.0);
}

void GimbalMotionModeBase::writeServoCommands(ServoDriverDevice* driverInterface, double finalVelocity, float scalingFactor)
{
    if (!driverInterface) return;

    // Determine direction from the sign of the final velocity
    quint16 direction;
    if (finalVelocity > 0.01) { // Added deadband
        direction = DIRECTION_REVERSE; // This mapping depends on wiring
    } else if (finalVelocity < -0.01) {
        direction = DIRECTION_FORWARD;
    } else {
        direction = DIRECTION_STOP;
    }

    // Calculate speed (magnitude of velocity) scaled to servo units
    quint32 speedCommand = static_cast<quint32>(std::abs(finalVelocity) * scalingFactor);
    quint32 clampedSpeed = std::min(speedCommand, MAX_SPEED);
    
    // Split speed into two 16-bit registers
    quint16 upperBits = static_cast<quint16>((clampedSpeed >> 16) & 0xFFFF);
    quint16 lowerBits = static_cast<quint16>(clampedSpeed & 0xFFFF);

    driverInterface->writeData(SPEED_REGISTER, {upperBits, lowerBits});
    driverInterface->writeData(DIRECTION_REGISTER, {direction});
}


void GimbalMotionModeBase::writeTargetPosition(ServoDriverDevice* driverInterface, 
                                             long targetPositionInSteps)
{
    if (!driverInterface) return;

    // Oriental Motor drivers often take a 32-bit position.
    // We need to split it into two 16-bit registers.
    quint16 upperSteps = static_cast<quint16>((targetPositionInSteps >> 16) & 0xFFFF);
    quint16 lowerSteps = static_cast<quint16>(targetPositionInSteps & 0xFFFF);
    
    // PSEUDO-CODE: Register addresses would come from the AZD-KX manual.
    static constexpr quint16 TARGET_POS_UPPER_REG = 0x0100; // Example address
    static constexpr quint16 TARGET_POS_LOWER_REG = 0x0102; // Example address
    static constexpr quint16 EXECUTE_MOVE_REG     = 0x007D; // Example address for "GO" command

    // Write the new target position
    driverInterface->writeData(TARGET_POS_UPPER_REG, {upperSteps});
    driverInterface->writeData(TARGET_POS_LOWER_REG, {lowerSteps});
    
    // Trigger the move
    driverInterface->writeData(EXECUTE_MOVE_REG, {0x0001}); // Example "Start Move" command
}
 

void GimbalMotionModeBase::setAcceleration(ServoDriverDevice* driverInterface, quint32 acceleration)
{
    if (!driverInterface) return;
    
    quint32 clamped = std::min(acceleration, MAX_ACCELERATION);
    quint16 upper = static_cast<quint16>((clamped >> 16) & 0xFFFF);
    quint16 lower = static_cast<quint16>(clamped & 0xFFFF);

    QVector<quint16> accelData = {upper, lower};
    
    // Write to all acceleration registers
    for (quint16 reg : ACCEL_REGISTERS) {
        driverInterface->writeData(reg, accelData);
    }
}

bool GimbalMotionModeBase::checkSafetyConditions(GimbalController* controller)
{
    if (!controller || !controller->systemStateModel()) {
        return false;
    }
    
    SystemStateData data = controller->systemStateModel()->data();

        bool deadManSwitchOk = true;
    if (controller->currentMotionModeType() == MotionMode::Manual ||
        controller->currentMotionModeType() == MotionMode::AutoTrack) {
        deadManSwitchOk = data.deadManSwitchActive;
    }

    return data.stationEnabled &&
           !data.emergencyStopActive &&   deadManSwitchOk;
}

bool GimbalMotionModeBase::checkElevationLimits(double currentEl, double targetVelocity,
                                               bool upperLimit, bool lowerLimit)
{
    // Check upper limits
    if ((currentEl >= MAX_ELEVATION_ANGLE || upperLimit) && (targetVelocity > 0)) {
        return false;
    }
    
    // Check lower limits
    if ((currentEl <= MIN_ELEVATION_ANGLE || lowerLimit) && (targetVelocity < 0)) {
        return false;
    }
    
    return true;
}

void GimbalMotionModeBase::calculateStabilizationCorrection(double currentAz_deg, double currentEl_deg,
                                                            double gyroX_dps_raw, double gyroY_dps_raw, double gyroZ_dps_raw,
                                                            double& azCorrection_dps, double& elCorrection_dps)
{
    // === START OF DIAGNOSTIC LOGGING ===
    // Use a static variable to log only every Nth frame to avoid flooding the console.
    static int logCounter = 0;
    const int LOG_INTERVAL = 50; // Log every 5th call (e.g., 4 times per second if update is 20Hz)

    bool shouldLog = (logCounter++ % LOG_INTERVAL == 0); 

    // Validate inputs
    if (std::isnan(gyroX_dps_raw) || std::isnan(gyroY_dps_raw) || std::isnan(gyroZ_dps_raw) ||
        std::isnan(currentAz_deg) || std::isnan(currentEl_deg)) {
        azCorrection_dps = 0.0;
        elCorrection_dps = 0.0;
        return;
    }

    // --- Filter the raw DPS data ---
    // The member filters m_gyroXFilter etc. are used here.
    double gyroX_dps_filtered = m_gyroXFilter.update(gyroX_dps_raw);
    double gyroY_dps_filtered = m_gyroYFilter.update(gyroY_dps_raw);
    // Apply bias correction to Z-axis gyro BEFORE filtering and kinematic transformation
    double gyroZ_dps_corrected = gyroZ_dps_raw - m_gyroBiasZ;
    double gyroZ_dps_filtered = m_gyroZFilter.update(gyroZ_dps_corrected);

    // Map to platform motion axes (p, q, r)
    const double p_imu = gyroY_dps_filtered; // Roll
    const double q_imu = gyroX_dps_filtered; // Pitch
    const double r_imu = gyroZ_dps_filtered; // Yaw

    // Get current gimbal angles in radians
    const double currentAzRad = degToRad(currentAz_deg);
    const double currentElRad = degToRad(currentEl_deg);

    // Apply full kinematic transformation
    double platformEffectOnEl = (q_imu * cos(currentAzRad)) - (p_imu * sin(currentAzRad));

    double tanEl = tan(currentElRad); // Calculate tan() once
    double platformEffectOnAz;
    if (qAbs(cos(currentElRad)) < 1e-6) {
        platformEffectOnAz = r_imu;
    } else {
        platformEffectOnAz = r_imu + tanEl * (q_imu * sin(currentAzRad) + p_imu * cos(currentAzRad));
    }

    // The correction is the negative of the platform's effect
    azCorrection_dps = -platformEffectOnAz;
    elCorrection_dps = -platformEffectOnEl;

    // --- DETAILED LOGGING OUTPUT ---
    if (shouldLog) {
        // Use fixed-point notation for easy comparison
        qDebug().noquote().nospace()
            << "StabIn(GYRO):"
            << " X=" << QString::number(gyroX_dps_raw, 'f', 2)
            << " Y=" << QString::number(gyroY_dps_raw, 'f', 2)
            << " Z=" << QString::number(gyroZ_dps_raw, 'f', 2)
            << " | ElAngle=" << QString::number(currentEl_deg, 'f', 1)
            << " tan(El)=" << QString::number(tanEl, 'f', 2)
            << " | StabOut(CORR):"
            << " Az=" << QString::number(azCorrection_dps, 'f', 2)
            << " El=" << QString::number(elCorrection_dps, 'f', 2);
    }
    // ================================

    // Limit the correction velocity
    const double MAX_CORRECTION_DPS = 5.0;
    azCorrection_dps = qBound(-MAX_CORRECTION_DPS, azCorrection_dps, MAX_CORRECTION_DPS);
    elCorrection_dps = qBound(-MAX_CORRECTION_DPS, elCorrection_dps, MAX_CORRECTION_DPS);
}


