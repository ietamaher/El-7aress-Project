#ifndef SYSTEMSTATEMODEL_H
#define SYSTEMSTATEMODEL_H

/**
 * @file SystemStateModel.h
 * @brief Central state management class for Remote Controlled Weapon System (RCWS)
 * 
 * This class manages all aspects of the RCWS system state and provides organized
 * functionality across multiple operational domains.
 * 
 * MAIN CATEGORIES:
 * 1. Core System Data Management - Basic data access and updates
 * 2. User Interface Controls - UI styling, reticle, safety switches
 * 3. Weapon Control and Tracking - Weapon movement and tracking controls
 * 4. Fire Control and Safety Zones - No-fire/no-traverse zone management
 * 5. Lead Angle Compensation - Moving target compensation
 * 6. Area Zone Management - Restricted area definitions
 * 7. Auto Sector Scan Management - Automated scanning zones
 * 8. Target Reference Point (TRP) Management - Reference point system
 * 9. Configuration File Management - Save/load functionality
 * 10. Weapon Zeroing Procedures - Ballistic calibration
 * 11. Windage Compensation - Environmental compensation
 * 
 * FOR SIGNALS AND SLOTS:
 * • Core System Signals - Basic system state notifications
 * • Zone Management Signals - Zone change notifications
 * • Gimbal and Positioning Signals - Position updates
 * • Ballistic Compensation Signals - Zeroing/windage/lead angle states
 * • Hardware Interface Slots - PLC and servo data handlers
 * • Sensor Data Slots - Camera, LRF, gyro data handlers
 * • Joystick Control Slots - User input handling
 * • System Mode Control Slots - Operational mode changes
 * 
 * @author [Your Name]
 * @date 19 Juin 2025
 * @version 1.0
 */

#include <QObject>
#include <QColor>
#include <vector>
#include <QString>
#include <QDebug> // Include for qDebug
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QIODevice>


#include "systemstatedata.h"
#include "daycameradatamodel.h"
#include "gyrodatamodel.h"
#include "joystickdatamodel.h"
#include "lensdatamodel.h"
#include "lrfdatamodel.h"
#include "radardatamodel.h"
#include "nightcameradatamodel.h"
#include "plc21datamodel.h"
#include "plc42datamodel.h"
#include "servoactuatordatamodel.h"
#include "servodriverdatamodel.h"
#include "../TimestampLogger.h"
#include "../utils/reticleaimpointcalculator.h"

#include <cmath> 
#include <algorithm> // Include for std::find_if, std::remove_if, std::max
#include <limits> // Include for std::numeric_limits
#include <QElapsedTimer>
#include <QDateTime>
#include <cmath>

// Constants for stationary detection
static constexpr double STATIONARY_GYRO_LIMIT = 0.5;           // Max gyro magnitude (deg/s) for stationary
static constexpr double STATIONARY_ACCEL_DELTA_LIMIT = 0.01;   // Max accel change (G) for stationary
static constexpr int STATIONARY_TIME_MS = 2000;                // Required stationary time (2 seconds)

class SystemStateModel : public QObject
{
    Q_OBJECT
public:
    explicit SystemStateModel(QObject *parent = nullptr);

    // --- Core System Data Management ---
    /**
     * @brief Gets the current system state data.
     * @return The current SystemStateData structure.
     */
    SystemStateData data() const { return m_currentStateData; }
    
    /**
     * @brief Updates the entire system state with new data.
     * @param newState The new system state data to apply.
     */
    void updateData(const SystemStateData &newState);

    // --- User Interface Controls ---
    /**
     * @brief Sets the color style for the user interface.
     * @param style The color to be used for UI styling.
     */
    void setColorStyle(const QColor &style);
    
    /**
     * @brief Sets the reticle style for the targeting system.
     * @param type The type of reticle to display.
     */
    void setReticleStyle(const ReticleType &type);
    
    /**
     * @brief Sets the dead man switch state for safety control.
     * @param pressed True if the dead man switch is pressed, false otherwise.
     */
    void setDeadManSwitch(bool pressed);
    
    /**
     * @brief Sets the active camera type (day or night vision).
     * @param pressed True if day camera is active, false for night camera.
     */
    void setActiveCameraIsDay(bool pressed);

    // --- Weapon Control and Tracking ---
    /**
     * @brief Sets the down track button state for weapon control.
     * @param pressed True if down track is pressed, false otherwise.
     */
    void setDownTrack(bool pressed);
    
    /**
     * @brief Sets the down switch state for weapon control.
     * @param pressed True if down switch is pressed, false otherwise.
     */
    void setDownSw(bool pressed);
    
    /**
     * @brief Sets the up track button state for weapon control.
     * @param pressed True if up track is pressed, false otherwise.
     */
    void setUpTrack(bool pressed);
    
    /**
     * @brief Sets the up switch state for weapon control.
     * @param pressed True if up switch is pressed, false otherwise.
     */
    void setUpSw(bool pressed);

    // --- Fire Control and Safety Zones ---
    /**
     * @brief Sets whether the current aim point is in a no-fire zone.
     * @param isInZone True if the point is in a restricted fire zone, false otherwise.
     */
    void setPointInNoFireZone(bool isInZone);
    
    /**
     * @brief Sets whether the current aim point is in a no-traverse zone.
     * @param isInZone True if the point is in a restricted traverse zone, false otherwise.
     */
    void setPointInNoTraverseZone(bool isInZone);
    
    /**
     * @brief Checks if a target position is within any no-fire zone.
     * @param targetAz Target azimuth angle in degrees.
     * @param targetEl Target elevation angle in degrees.
     * @param targetRange Target range in meters (optional, default -1.0f).
     * @return True if the target is in a no-fire zone, false otherwise.
     */
    bool isPointInNoFireZone(float targetAz, float targetEl, float targetRange = -1.0f) const;
    
    /**
     * @brief Checks if a target azimuth is within any no-traverse zone at current elevation.
     * @param targetAz Target azimuth angle in degrees.
     * @param currentEl Current elevation angle in degrees.
     * @return True if the target azimuth is in a no-traverse zone, false otherwise.
     */
    bool isPointInNoTraverseZone(float targetAz, float currentEl) const;
    
    /**
     * @brief Checks if an intended azimuth movement would hit a no-traverse zone limit.
     * @param currentAz Current azimuth angle in degrees.
     * @param currentEl Current elevation angle in degrees.
     * @param intendedMoveAz Intended azimuth movement in degrees.
     * @return True if the movement would hit a limit, false otherwise.
     */
    bool isAtNoTraverseZoneLimit(float currentAz, float currentEl, float intendedMoveAz) const;

    // --- Lead Angle Compensation ---
    /**
     * @brief Sets whether lead angle compensation is active for moving targets.
     * @param active True to activate lead angle compensation, false to deactivate.
     */
    void setLeadAngleCompensationActive(bool active);
    
    /**
     * @brief Updates the calculated lead angle offsets for target compensation.
     * @param offsetAz Calculated azimuth offset in degrees.
     * @param offsetEl Calculated elevation offset in degrees.
     * @param status Current status of the lead angle calculation.
     */
    void updateCalculatedLeadOffsets(float offsetAz, float offsetEl, LeadAngleStatus status);

    // --- Area Zone Management ---
    /**
     * @brief Adds a new area zone to the system.
     * @param zone The area zone to add (ID will be assigned automatically).
     * @return True if the zone was added successfully, false otherwise.
     */
    bool addAreaZone(AreaZone zone);
    
    /**
     * @brief Modifies an existing area zone.
     * @param id The identifier of the zone to modify.
     * @param updatedZoneData The new zone data to apply.
     * @return True if the modification was successful, false otherwise.
     */
    bool modifyAreaZone(int id, const AreaZone& updatedZoneData);
    
    /**
     * @brief Deletes an area zone by its identifier.
     * @param id The identifier of the zone to delete.
     * @return True if the deletion was successful, false otherwise.
     */
    bool deleteAreaZone(int id);
    
    /**
     * @brief Gets all area zones in the system.
     * @return A constant reference to the vector of area zones.
     */
    const std::vector<AreaZone>& getAreaZones() const;
    
    /**
     * @brief Gets a specific area zone by its identifier.
     * @param id The identifier of the zone to retrieve.
     * @return Pointer to the zone if found, nullptr otherwise.
     */
    AreaZone* getAreaZoneById(int id);

    // --- Auto Sector Scan Management ---
    /**
     * @brief Adds a new automatic sector scan zone to the system.
     * @param zone The sector scan zone to add (ID will be assigned automatically).
     * @return True if the zone was added successfully, false otherwise.
     */
    bool addSectorScanZone(AutoSectorScanZone zone);
    
    /**
     * @brief Modifies an existing automatic sector scan zone.
     * @param id The identifier of the zone to modify.
     * @param updatedZoneData The new zone data to apply.
     * @return True if the modification was successful, false otherwise.
     */
    bool modifySectorScanZone(int id, const AutoSectorScanZone& updatedZoneData);
    
    /**
     * @brief Deletes an automatic sector scan zone by its identifier.
     * @param id The identifier of the zone to delete.
     * @return True if the deletion was successful, false otherwise.
     */
    bool deleteSectorScanZone(int id);
    
    /**
     * @brief Gets all automatic sector scan zones in the system.
     * @return A constant reference to the vector of sector scan zones.
     */
    const std::vector<AutoSectorScanZone>& getSectorScanZones() const;
    
    /**
     * @brief Gets a specific sector scan zone by its identifier.
     * @param id The identifier of the zone to retrieve.
     * @return Pointer to the zone if found, nullptr otherwise.
     */
    AutoSectorScanZone* getSectorScanZoneById(int id);
    
    /**
     * @brief Selects the next automatic sector scan zone in sequence.
     */
    void selectNextAutoSectorScanZone();
    
    /**
     * @brief Selects the previous automatic sector scan zone in sequence.
     */
    void selectPreviousAutoSectorScanZone();

    // --- Target Reference Point (TRP) Management ---
    /**
     * @brief Adds a new target reference point to the system.
     * @param trp The target reference point to add (ID will be assigned automatically).
     * @return True if the TRP was added successfully, false otherwise.
     */
    bool addTRP(TargetReferencePoint trp);
    
    /**
     * @brief Modifies an existing target reference point.
     * @param id The identifier of the TRP to modify.
     * @param updatedTRPData The new TRP data to apply.
     * @return True if the modification was successful, false otherwise.
     */
    bool modifyTRP(int id, const TargetReferencePoint& updatedTRPData);
    
    /**
     * @brief Deletes a target reference point by its identifier.
     * @param id The identifier of the TRP to delete.
     * @return True if the deletion was successful, false otherwise.
     */
    bool deleteTRP(int id);
    
    /**
     * @brief Gets all target reference points in the system.
     * @return A constant reference to the vector of target reference points.
     */
    const std::vector<TargetReferencePoint>& getTargetReferencePoints() const;
    
    /**
     * @brief Gets a specific target reference point by its identifier.
     * @param id The identifier of the TRP to retrieve.
     * @return Pointer to the TRP if found, nullptr otherwise.
     */
    TargetReferencePoint* getTRPById(int id);
    
    /**
     * @brief Selects the next target reference point location page for display.
     */
    void selectNextTRPLocationPage();
    
    /**
     * @brief Selects the previous target reference point location page for display.
     */
    void selectPreviousTRPLocationPage();

    // --- Configuration File Management ---
    /**
     * @brief Saves all zones (area, sector scan, TRP) to a configuration file.
     * @param filePath The path to the file where zones will be saved.
     * @return True if the save operation was successful, false otherwise.
     */
    bool saveZonesToFile(const QString& filePath);
    
    /**
     * @brief Loads all zones (area, sector scan, TRP) from a configuration file.
     * @param filePath The path to the file from which zones will be loaded.
     * @return True if the load operation was successful, false otherwise.
     */
    bool loadZonesFromFile(const QString& filePath);

    // --- Weapon Zeroing Procedures ---
    /**
     * @brief Starts the weapon zeroing procedure for ballistic calibration.
     */
    void startZeroingProcedure();
    
    /**
     * @brief Applies zeroing adjustment based on reticle movement.
     * @param deltaAz Azimuth adjustment in degrees.
     * @param deltaEl Elevation adjustment in degrees.
     */
    void applyZeroingAdjustment(float deltaAz, float deltaEl);
    
    /**
     * @brief Finalizes the zeroing procedure and applies adjustments to ballistics.
     */
    void finalizeZeroing();
    
    /**
     * @brief Clears all zeroing adjustments and resets to default values.
     */
    void clearZeroing();

    // --- Windage Compensation ---
    /**
     * @brief Starts the windage compensation procedure for environmental conditions.
     */
    void startWindageProcedure();
    
    /**
     * @brief Sets the wind speed for windage calculations.
     * @param knots Wind speed in knots.
     */
    void setWindageSpeed(float knots);
    
    /**
     * @brief Finalizes the windage procedure and applies compensation to ballistics.
     */
    void finalizeWindage();
    
    /**
     * @brief Clears all windage compensation and resets to default values.
     */
    void clearWindage();
    /*void updateTrackedTargetInfo(int cameraIndex, bool isValid, float centerX_px, float centerY_px,
                                 float width_px, float height_px,
                                 float velocityX_px_s, float velocityY_px_s,
                                 VPITrackingState state);*/
    void updateTrackingResult(int cameraIndex, bool hasLock,
                              float centerX_px, float centerY_px,
                              float width_px, float height_px,
                              float velocityX_px_s, float velocityY_px_s,
                              VPITrackingState state);
 

    void startTrackingAcquisition();
    void requestTrackerLockOn();
    void stopTracking();
    void updateTrackingResult(bool hasLock, ...); // From VideoProcessor
    void adjustAcquisitionBoxSize(float dW, float dH);
    
    // ---  State Transition Methods ---
    // These are called by JoystickController/MainWindow based on user input
    void enterSurveillanceMode(); // Called when station is enabled
    void enterIdleMode();         // Called when station is disabled
    void commandEngagement(bool start); // True to start, false to stop
    //void enterEmergencyStopMode();
    // No need for enterTrackingMode(), as this is now handled by the TrackingPhase machine inside updateTrackingResult

signals:
    // --- Core System Signals ---
    /**
     * @brief Emitted when system state data changes.
     * @param newState The new system state data.
     */
    void dataChanged(const SystemStateData &newState);
    
    /**
     * @brief Emitted when UI color style changes.
     * @param style The new color style.
     */
    void colorStyleChanged(const QColor &style);
    
    /**
     * @brief Emitted when reticle style changes.
     * @param type The new reticle type.
     */
    void reticleStyleChanged(const ReticleType &type);

    // --- Zone Management Signals ---
    /**
     * @brief Emitted after any zone list modification (add, modify, delete).
     */
    void zonesChanged();

    // --- Gimbal and Positioning Signals ---
    /**
     * @brief Emitted when gimbal position changes.
     * @param az New azimuth position in degrees.
     * @param el New elevation position in degrees.
     */
    void gimbalPositionChanged(float az, float el);

    // --- Ballistic Compensation Signals ---
    /**
     * @brief Emitted when zeroing state changes.
     * @param active True if zeroing mode is active, false otherwise.
     * @param azOffset Current azimuth offset in degrees.
     * @param elOffset Current elevation offset in degrees.
     */
    void zeroingStateChanged(bool active, float azOffset, float elOffset);
    
    /**
     * @brief Emitted when windage state changes.
     * @param active True if windage mode is active, false otherwise.
     * @param speed Current wind speed in knots.
     */
    void windageStateChanged(bool active, float speed);
    
    /**
     * @brief Emitted when lead angle compensation state changes.
     * @param active True if lead angle compensation is active, false otherwise.
     * @param status Current lead angle calculation status.
     * @param offsetAz Current azimuth lead offset in degrees.
     * @param offsetEl Current elevation lead offset in degrees.
     */
    void leadAngleStateChanged(bool active, LeadAngleStatus status, float offsetAz, float offsetEl);

public slots:
    // --- Hardware Interface Slots ---
    /**
     * @brief Handles changes in PLC21 panel data.
     * @param pData The new PLC21 panel data.
     */
    void onPlc21DataChanged(const Plc21PanelData &pData);
    
    /**
     * @brief Handles changes in PLC42 data.
     * @param pData The new PLC42 data.
     */
    void onPlc42DataChanged(const Plc42Data &pData);
    
    /**
     * @brief Handles changes in servo azimuth data.
     * @param azData The new azimuth servo data.
     */
    void onServoAzDataChanged(const ServoData &azData);
    
    /**
     * @brief Handles changes in servo elevation data.
     * @param elData The new elevation servo data.
     */
    void onServoElDataChanged(const ServoData &elData);
    
    /**
     * @brief Handles changes in servo actuator data.
     * @param actuatorData The new servo actuator data.
     */
    void onServoActuatorDataChanged(const ServoActuatorData &actuatorData);

    // --- Sensor Data Slots ---
    /**
     * @brief Handles changes in laser range finder data.
     * @param lrfData The new LRF data.
     */
    void onLrfDataChanged(const LrfData &lrfData);
    
    /**
     * @brief Handles changes in day camera data.
     * @param dayData The new day camera data.
     */
    void onDayCameraDataChanged(const DayCameraData &dayData);
    
    /**
     * @brief Handles changes in gyroscope data.
     * @param gyroData The new gyroscope data.
     */
    void onGyroDataChanged(const ImuData &gyroData);
    
    /**
     * @brief Handles changes in lens data.
     * @param lensData The new lens data.
     */
    void onLensDataChanged(const LensData &lensData);
    
    /**
     * @brief Handles changes in night camera data.
     * @param nightData The new night camera data.
     */
    void onNightCameraDataChanged(const NightCameraData &nightData);

    // --- Joystick Control Slots ---
    /**
     * @brief Handles joystick axis movement changes.
     * @param axis The axis number that changed.
     * @param normalizedValue The normalized value of the axis (-1.0 to 1.0).
     */
    void onJoystickAxisChanged(int axis, float normalizedValue);
    
    /**
     * @brief Handles joystick button press/release changes.
     * @param button The button number that changed.
     * @param pressed True if the button is pressed, false if released.
     */
    void onJoystickButtonChanged(int button, bool pressed);

    void onJoystickHatChanged(int hat, int value);
    // --- System Mode Control Slots ---
    /**
     * @brief Sets the motion control mode of the system.
     * @param newMode The new motion mode to apply.
     */
    void setMotionMode(MotionMode newMode);
    
    /**
     * @brief Sets the operational mode of the system.
     * @param newOpMode The new operational mode to apply.
     */
    void setOpMode(OperationalMode newOpMode);
    
    /**
     * @brief Sets whether tracking restart is requested.
     * @param restart True if tracking restart is requested, false otherwise.
     */
    void setTrackingRestartRequested(bool restart);
    
    /**
     * @brief Sets whether tracking has started.
     * @param start True if tracking has started, false otherwise.
     */
    void setTrackingStarted(bool start);
    //void onEmergencyStopActivated(bool isActive);
    void onRadarPlotsUpdated(const QVector<RadarData>& plots); // To receive data from RadarDevice
    void selectNextRadarTrack();
    void selectPreviousRadarTrack();
    void commandSlewToSelectedRadarTrack(); // Tell the gimbal to move
    void updateStationaryStatus(SystemStateData& data);
    
private:
    SystemStateData m_currentStateData; // Central data store

    // ID Counters for zones
    int m_nextAreaZoneId;
    int m_nextSectorScanId;
    int m_nextTRPId;

    // --- Private Helper Methods ---
    /**
     * @brief Gets the next available area zone ID and increments the counter.
     * @return The next available area zone ID.
     */
    int getNextAreaZoneId() { return m_nextAreaZoneId++; }
    
    /**
     * @brief Gets the next available sector scan zone ID and increments the counter.
     * @return The next available sector scan zone ID.
     */
    int getNextSectorScanId() { return m_nextSectorScanId++; }
    
    /**
     * @brief Gets the next available TRP ID and increments the counter.
     * @return The next available TRP ID.
     */
    int getNextTRPId() { return m_nextTRPId++; }

    /**
     * @brief Updates the next ID counters after loading data from file.
     */
    void updateNextIdsAfterLoad();
    
    /**
     * @brief Recalculates derived aimpoint data based on current system state.
     */
    void recalculateDerivedAimpointData();
    
    /**
     * @brief Updates camera optics parameters.
     * @param width Camera resolution width in pixels.
     * @param height Camera resolution height in pixels.
     * @param hfov Horizontal field of view in degrees.
     */
    void updateCameraOptics(int width, int height, float hfov);
    
    /**
     * @brief Updates camera optics and activity status for both day and night cameras.
     * @param width Camera resolution width in pixels.
     * @param height Camera resolution height in pixels.
     * @param dayHfov Day camera horizontal field of view in degrees.
     * @param nightHfov Night camera horizontal field of view in degrees.
     * @param isDayActive True if day camera is active, false for night camera.
     */
    void updateCameraOpticsAndActivity(int width, int height, float dayHfov, float nightHfov, bool isDayActive);
    
    /**
     * @brief Updates the lead angle system state with calculated values.
     * @param leadActive True if lead angle compensation is active.
     * @param leadStatus Current status of lead angle calculations.
     * @param calcLeadAzDeg Calculated lead azimuth in degrees.
     * @param calcLeadElDeg Calculated lead elevation in degrees.
     */
    void updateLeadAngleSystemState(bool leadActive, LeadAngleStatus leadStatus, float calcLeadAzDeg, float calcLeadElDeg);

    /**
     * @brief Updates the current scan zone name for display purposes.
     */
    void updateCurrentScanName();
    //void processStateTransitions(const SystemStateData& oldData, const SystemStateData& newData);
    void processStateTransitions(const SystemStateData& oldData, SystemStateData& newData);

    void enterEmergencyStopMode(); // You already have this
};

#endif // SYSTEMSTATEMODEL_H
