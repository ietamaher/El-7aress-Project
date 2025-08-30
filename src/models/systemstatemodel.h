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
 * @author ieta_maher
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

    // =================================================================
    // 1. CORE SYSTEM & MODE MANAGEMENT
    // =================================================================
    /**
     * @brief Gets the current system state data.
     * @return The current SystemStateData structure.
     */
    virtual SystemStateData data() const { return m_currentStateData; }
    
    /**
     * @brief Updates the entire system state with new data.
     * @param newState The new system state data to apply.
     */
    void updateData(const SystemStateData &newState);

    /**
     * @brief Sets the motion control mode of the system.
     * @param newMode The new motion mode to apply.
     */
    virtual void setMotionMode(MotionMode newMode);

    /**
     * @brief Sets the operational mode of the system.
     * @param newOpMode The new operational mode to apply.
     */
    virtual void setOpMode(OperationalMode newOpMode);


    // =================================================================
    // 2. USER INTERFACE CONTROLS
    // =================================================================
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
    

    // =================================================================
    // 3. GIMBAL, WEAPON, AND TRACKING CONTROL
    // =================================================================
    /**
     * @brief Sets the dead man switch state for safety control.
     * @param pressed True if the dead man switch is pressed, false otherwise.
     */
    virtual void setDeadManSwitch(bool pressed);
    
    /**
     * @brief Sets the active camera type (day or night vision).
     * @param pressed True if day camera is active, false for night camera.
     */
    void setActiveCameraIsDay(bool pressed);

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

    /**
     * @brief Updates the tracked target information.
     * @param cameraIndex The index of the camera that produced the tracking result.
     * @param hasLock True if the tracker has a lock on the target.
     * @param centerX_px The x-coordinate of the center of the tracked target in pixels.
     * @param centerY_px The y-coordinate of the center of the tracked target in pixels.
     * @param width_px The width of the tracked target in pixels.
     * @param height_px The height of the tracked target in pixels.
     * @param velocityX_px_s The velocity of the tracked target in the x-direction in pixels per second.
     * @param velocityY_px_s The velocity of the tracked target in the y-direction in pixels per second.
     * @param state The state of the tracker.
     */
    void updateTrackingResult(int cameraIndex, bool hasLock,
                              float centerX_px, float centerY_px,
                              float width_px, float height_px,
                              float velocityX_px_s, float velocityY_px_s,
                              VPITrackingState state);


    /**
     * @brief Starts the tracking acquisition process.
     */
    virtual void startTrackingAcquisition();

    /**
     * @brief Requests the tracker to lock on the target.
     */
    virtual void requestTrackerLockOn();

    /**
     * @brief Stops the tracking process.
     */
    virtual void stopTracking();

    /**
     * @brief Adjusts the size of the acquisition box.
     * @param dW The change in width.
     * @param dH The change in height.
     */
    void adjustAcquisitionBoxSize(float dW, float dH);

    /**
     * @brief Enters the surveillance mode.
     */
    void enterSurveillanceMode();

    /**
     * @brief Enters the idle mode.
     */
    void enterIdleMode();

    /**
     * @brief Commands engagement.
     * @param start True to start engagement, false to stop.
     */
    virtual void commandEngagement(bool start);


    // =================================================================
    // 4. FIRE CONTROL AND SAFETY ZONES
    // =================================================================
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


    // =================================================================
    // 5. BALLISTIC COMPENSATION
    // =================================================================
    // Lead Angle
    /**
     * @brief Sets whether lead angle compensation is active for moving targets.
     * @param active True to activate lead angle compensation, false to deactivate.
     */
    virtual void setLeadAngleCompensationActive(bool active);
    
    /**
     * @brief Updates the calculated lead angle offsets for target compensation.
     * @param offsetAz Calculated azimuth offset in degrees.
     * @param offsetEl Calculated elevation offset in degrees.
     * @param status Current status of the lead angle calculation.
     */
    void updateCalculatedLeadOffsets(float offsetAz, float offsetEl, LeadAngleStatus status);
    // Zeroing
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
    // Windage
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


    // =================================================================
    // 6. AREA ZONE MANAGEMENT
    // =================================================================
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


    // =================================================================
    // 7. AUTO SECTOR SCAN MANAGEMENT
    // =================================================================
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
    virtual void selectNextAutoSectorScanZone();
    
    /**
     * @brief Selects the previous automatic sector scan zone in sequence.
     */
    virtual void selectPreviousAutoSectorScanZone();


    // =================================================================
    // 8. TARGET REFERENCE POINT (TRP) MANAGEMENT
    // =================================================================
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
    virtual void selectNextTRPLocationPage();
    
    /**
     * @brief Selects the previous target reference point location page for display.
     */
    virtual void selectPreviousTRPLocationPage();


    // =================================================================
    // 9. RADAR CONTROL
    // =================================================================
    /**
     * @brief Selects the next radar track.
     */
    void selectNextRadarTrack();
    
    /**
     * @brief Selects the previous radar track.
     */
    void selectPreviousRadarTrack();
    
    /**
     * @brief Commands the gimbal to slew to the selected radar track.
     */
    void commandSlewToSelectedRadarTrack();


    // =================================================================
    // 10. CONFIGURATION FILE MANAGEMENT
    // =================================================================
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


signals:
    // --- Core System Signals ---
    void dataChanged(const SystemStateData &newState);
    void colorStyleChanged(const QColor &style);
    void reticleStyleChanged(const ReticleType &type);
    // --- Zone Management Signals ---
    void zonesChanged();
    // --- Gimbal and Positioning Signals ---
    void gimbalPositionChanged(float az, float el);
    // --- Ballistic Compensation Signals ---
    void zeroingStateChanged(bool active, float azOffset, float elOffset);
    void windageStateChanged(bool active, float speed);
    void leadAngleStateChanged(bool active, LeadAngleStatus status, float offsetAz, float offsetEl);

public slots:
    // --- Hardware Interface Slots ---
    void onPlc21DataChanged(const Plc21PanelData &pData);
    void onPlc42DataChanged(const Plc42Data &pData);
    void onServoAzDataChanged(const ServoData &azData);
    void onServoElDataChanged(const ServoData &elData);
    void onServoActuatorDataChanged(const ServoActuatorData &actuatorData);
    // --- Sensor Data Slots ---
    void onLrfDataChanged(const LrfData &lrfData);
    void onDayCameraDataChanged(const DayCameraData &dayData);
    void onGyroDataChanged(const ImuData &gyroData);
    void onLensDataChanged(const LensData &lensData);
    void onNightCameraDataChanged(const NightCameraData &nightData);
    void onRadarPlotsUpdated(const QVector<RadarData>& plots); // To receive data from RadarDevice
    // --- Joystick Control Slots ---
    void onJoystickAxisChanged(int axis, float normalizedValue);
    void onJoystickButtonChanged(int button, bool pressed);
    void onJoystickHatChanged(int hat, int value);
    
    
private:
    SystemStateData m_currentStateData; // Central data store

    // ID Counters for zones
    int m_nextAreaZoneId;
    int m_nextSectorScanId;
    int m_nextTRPId;

    // --- Private Helper Methods ---
    int getNextAreaZoneId() { return m_nextAreaZoneId++; }
    int getNextSectorScanId() { return m_nextSectorScanId++; }
    int getNextTRPId() { return m_nextTRPId++; }
    void updateNextIdsAfterLoad();
    void recalculateDerivedAimpointData();
    void updateCameraOpticsAndActivity(int width, int height, float dayHfov, float nightHfov, bool isDayActive);
    void updateCurrentScanName();
    void processStateTransitions(const SystemStateData& oldData, SystemStateData& newData);
    void enterEmergencyStopMode();
    void updateStationaryStatus(SystemStateData& data);
};

#endif // SYSTEMSTATEMODEL_H
