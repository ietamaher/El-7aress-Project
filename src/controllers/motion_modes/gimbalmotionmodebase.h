#ifndef GIMBALMOTIONMODEBASE_H
#define GIMBALMOTIONMODEBASE_H

#include <QObject>
#include <QtMath>
#include "../../models/systemstatedata.h" // Include for SystemStateData

// Forward declare GimbalController
class GimbalController;

// Low-pass filter class for gyroscope data
class GyroLowPassFilter {
private:
    double alpha;           // Filter coefficient (0 < alpha < 1)
    double filteredValue;   // Current filtered value
    bool initialized;       // Whether filter has been initialized

public:
    GyroLowPassFilter(double cutoffFreq = 10.0, double sampleRate = 100.0) : initialized(false) {
        // Calculate alpha from cutoff frequency and sample rate
        // alpha = dt / (RC + dt), where RC = 1 / (2 * pi * cutoff_freq)
        double dt = 1.0 / sampleRate;
        double RC = 1.0 / (2.0 * M_PI * cutoffFreq);
        alpha = dt / (RC + dt);

        // Clamp alpha to reasonable bounds
        alpha = qBound(0.01, alpha, 0.99);
    }

    double update(double newValue) {
        if (!initialized) {
            filteredValue = newValue;
            initialized = true;
            return filteredValue;
        }

        // Low-pass filter: y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
        filteredValue = alpha * newValue + (1.0 - alpha) * filteredValue;
        return filteredValue;
    }

    void reset() {
        initialized = false;
        filteredValue = 0.0;
    }

    bool isInitialized() const { return initialized; }
};

class GimbalMotionModeBase : public QObject
{
    Q_OBJECT
public:
    explicit GimbalMotionModeBase(QObject* parent = nullptr)
        : QObject(parent)
        , m_gyroXFilter(5.0, 20.0)  // 5Hz cutoff, 20Hz sample rate
        , m_gyroYFilter(5.0, 20.0)
        , m_gyroZFilter(5.0, 20.0)
    {}

    virtual ~GimbalMotionModeBase() = default;

    // Called when we enter this mode
    virtual void enterMode(GimbalController* /*controller*/) {}

    // Called when we exit this mode
    virtual void exitMode(GimbalController* /*controller*/) {}

    // Called periodically (e.g. from GimbalController::update())
    virtual void update(GimbalController* /*controller*/) {}
    void stopServos(GimbalController* controller);
    bool checkSafetyConditions(GimbalController* controller);
    /**
     * @brief Updates the Z-axis gyro bias if the vehicle is stationary.
     * This function should be called periodically from the main controller loop.
     * @param systemState The current system state data.
     */
    void updateGyroBias(const SystemStateData& systemState);
protected:


    /**
     * @brief Calculates and sends final servo commands, incorporating full kinematic stabilization.
     * @param controller Pointer to the GimbalController to access system state (IMU, angles).
     * @param desiredAzVelocity The desired azimuth velocity relative to a stable world frame (deg/s).
     * @param desiredElVelocity The desired elevation velocity relative to a stable world frame (deg/s).
     */
    void sendStabilizedServoCommands(GimbalController* controller,
                                     double desiredAzVelocity,
                                     double desiredElVelocity);
    // --- UNIFIED PID CONTROLLER ---
    struct PIDController {
        double Kp = 0.0;
        double Ki = 0.0;
        double Kd = 0.0;
        double integral = 0.0;
        double maxIntegral = 1.0;
        double previousError = 0.0;
        double previousMeasurement = 0.0;

        void reset() {
            integral = 0.0;
            previousError = 0.0;
        }
    };
    struct PIDOutput {
        double p_term = 0.0;
        double i_term = 0.0;
        double d_term = 0.0;
        double total = 0.0;
    };
    double pidCompute(PIDController& pid, double error, double setpoint, double measurement, bool derivativeOnMeasurement, double dt);

    // We can provide a convenient overload for the old "derivative on error" method
    // This way, you don't have to change your existing code in the scanning modes.
    double pidCompute(PIDController& pid, double error, double dt);
    // Helper methods for common operations
    //       double joystickInput, quint16 angularVelocity);

    void writeServoCommands(class ServoDriverDevice* driverInterface, double finalVelocity, float scalingFactor = 250.0f);
    void writeTargetPosition(ServoDriverDevice* driverInterface,  long targetPositionInSteps);
    void setAcceleration(class ServoDriverDevice* driverInterface, quint32 acceleration = DEFAULT_ACCELERATION);
    bool checkElevationLimits(double currentEl, double targetVelocity, bool upperLimit, bool lowerLimit);
    /**
     * @brief Configures the AZD-KX driver for continuous velocity control mode.
     *        This should be called once when a motion mode is entered.
     */
    void configureVelocityMode(class ServoDriverDevice* driverInterface);

    /**
     * @brief Writes a new speed command to the driver in real-time.
     * @param finalVelocity The calculated velocity in degrees/second.
     * @param scalingFactor Converts deg/s to Hz for the driver.
     */
    void writeVelocityCommand(class ServoDriverDevice* driverInterface,
                              double finalVelocity,
                              double scalingFactor);
    // --- COMMON CONSTANTS ---
    // Servo Control
    static constexpr quint32 DEFAULT_ACCELERATION = 100000;
    static constexpr quint32 MAX_ACCELERATION = 1000000000;
    static constexpr quint32 MAX_SPEED = 30000;

    // Servo register addresses
    static constexpr quint16 SPEED_REGISTER = 0x0480;
    static constexpr quint16 DIRECTION_REGISTER = 0x007D;
    static constexpr quint16 ACCEL_REGISTERS[] = {0x2A4, 0x282, 0x600, 0x680};

    // Direction commands
    static constexpr quint16 DIRECTION_FORWARD = 0x4000;
    static constexpr quint16 DIRECTION_REVERSE = 0x8000;
    static constexpr quint16 DIRECTION_STOP = 0x0000;

    // Motion limits
    static constexpr double MIN_ELEVATION_ANGLE = -10.0;
    static constexpr double MAX_ELEVATION_ANGLE = 50.0;
    static constexpr double MAX_VELOCITY = 30.0; // General velocity limit deg/s

    // Scaling factors
    static constexpr float SPEED_SCALING_FACTOR_SCAN = 250.0f;
    static constexpr float SPEED_SCALING_FACTOR_TRP_SCAN = 250.0f;

    // Common PID/Scan constants
    static constexpr double ARRIVAL_THRESHOLD_DEG = 0.5;   // How close to consider a point "reached"
    static constexpr double UPDATE_INTERVAL_S = 0.05;      // 50ms update interval

private:
    // Helper for angle conversions
    static inline double degToRad(double deg) { return deg * (M_PI / 180.0); }

    // Gyro filters for stabilization
    GyroLowPassFilter m_gyroXFilter;
    GyroLowPassFilter m_gyroYFilter;
    GyroLowPassFilter m_gyroZFilter;

    // Gyro bias for all three axes
    double m_gyroBiasX = 0.0;
    double m_gyroBiasY = 0.0;
    double m_gyroBiasZ = 0.0;

    void calculateStabilizationCorrection(double currentAz_deg, double currentEl_deg,
                                                            double gyroX_dps_raw, double gyroY_dps_raw, double gyroZ_dps_raw,
                                                            double& azCorrection_dps, double& elCorrection_dps);
};

#endif // GIMBALMOTIONMO


