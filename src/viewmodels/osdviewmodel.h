#ifndef OSDVIEWMODEL_H
#define OSDVIEWMODEL_H

#include <QObject>
#include <QColor>
#include <QRectF>
#include <QVariantList>
#include <QString>

// Forward declarations
struct FrameData;
struct YoloDetection;

/**
 * @brief ViewModel that exposes all OSD data to QML
 *
 * This class bridges C++ data (from FrameData) to QML properties,
 * enabling declarative UI updates without manual rendering.
 * Refactored to fully align with FrameData structure.
 */
class OsdViewModel : public QObject
{
    Q_OBJECT

    // ========================================================================
    // VISUAL STYLING
    // ========================================================================
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)

    // ========================================================================
    // SYSTEM STATUS (Top-Left Block)
    // ========================================================================
    Q_PROPERTY(QString modeText READ modeText NOTIFY modeTextChanged)
    Q_PROPERTY(QString motionText READ motionText NOTIFY motionTextChanged)
    Q_PROPERTY(QString rateText READ rateText NOTIFY rateTextChanged)
    Q_PROPERTY(QString stabText READ stabText NOTIFY stabTextChanged)
    Q_PROPERTY(QString cameraText READ cameraText NOTIFY cameraTextChanged)
    Q_PROPERTY(QString fovText READ fovText NOTIFY fovTextChanged)
    Q_PROPERTY(QString lrfText READ lrfText NOTIFY lrfTextChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString speedText READ speedText NOTIFY speedTextChanged)

    // ========================================================================
    // PROCEDURES (Zeroing, Windage, Environment)
    // ========================================================================
    Q_PROPERTY(bool zeroingVisible READ zeroingVisible NOTIFY zeroingVisibleChanged)
    Q_PROPERTY(QString zeroingText READ zeroingText NOTIFY zeroingTextChanged)
    Q_PROPERTY(bool windageVisible READ windageVisible NOTIFY windageVisibleChanged)
    Q_PROPERTY(QString windageText READ windageText NOTIFY windageTextChanged)
    Q_PROPERTY(bool environmentVisible READ environmentVisible NOTIFY environmentVisibleChanged)
    Q_PROPERTY(QString environmentText READ environmentText NOTIFY environmentTextChanged)

    // ========================================================================
    // DETECTION
    // ========================================================================
    Q_PROPERTY(bool detectionVisible READ detectionVisible NOTIFY detectionVisibleChanged)
    Q_PROPERTY(QString detectionText READ detectionText NOTIFY detectionTextChanged)
    Q_PROPERTY(QVariantList detectionBoxes READ detectionBoxes NOTIFY detectionBoxesChanged)

    // ========================================================================
    // SCAN PATTERNS
    // ========================================================================
    Q_PROPERTY(bool scanNameVisible READ scanNameVisible NOTIFY scanNameVisibleChanged)
    Q_PROPERTY(QString scanNameText READ scanNameText NOTIFY scanNameTextChanged)

    // ========================================================================
    // LEAD ANGLE COMPENSATION
    // ========================================================================
    Q_PROPERTY(bool leadAngleVisible READ leadAngleVisible NOTIFY leadAngleVisibleChanged)
    Q_PROPERTY(QString leadAngleText READ leadAngleText NOTIFY leadAngleTextChanged)
    Q_PROPERTY(bool lacActive READ lacActive NOTIFY lacActiveChanged)
    Q_PROPERTY(float leadAngleOffsetAz READ leadAngleOffsetAz NOTIFY leadAngleOffsetsChanged)
    Q_PROPERTY(float leadAngleOffsetEl READ leadAngleOffsetEl NOTIFY leadAngleOffsetsChanged)

    // ========================================================================
    // GIMBAL POSITION
    // ========================================================================
    Q_PROPERTY(float azimuth READ azimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation NOTIFY elevationChanged)
    Q_PROPERTY(float vehicleHeading READ vehicleHeading NOTIFY vehicleHeadingChanged)

    // ========================================================================
    // IMU SENSOR DATA
    // ========================================================================
    Q_PROPERTY(bool imuConnected READ imuConnected NOTIFY imuConnectedChanged)
    Q_PROPERTY(double imuRollDeg READ imuRollDeg NOTIFY imuDataChanged)
    Q_PROPERTY(double imuPitchDeg READ imuPitchDeg NOTIFY imuDataChanged)
    Q_PROPERTY(double imuYawDeg READ imuYawDeg NOTIFY imuDataChanged)
    Q_PROPERTY(double imuTemp READ imuTemp NOTIFY imuDataChanged)
    Q_PROPERTY(double gyroX READ gyroX NOTIFY imuDataChanged)
    Q_PROPERTY(double gyroY READ gyroY NOTIFY imuDataChanged)
    Q_PROPERTY(double gyroZ READ gyroZ NOTIFY imuDataChanged)
    Q_PROPERTY(double accelX READ accelX NOTIFY imuDataChanged)
    Q_PROPERTY(double accelY READ accelY NOTIFY imuDataChanged)
    Q_PROPERTY(double accelZ READ accelZ NOTIFY imuDataChanged)

    // ========================================================================
    // RETICLE (Gun Boresight with Zeroing Only)
    // ========================================================================
    Q_PROPERTY(int reticleType READ reticleType NOTIFY reticleTypeChanged)
    Q_PROPERTY(float reticleOffsetX READ reticleOffsetX NOTIFY reticlePositionChanged)
    Q_PROPERTY(float reticleOffsetY READ reticleOffsetY NOTIFY reticlePositionChanged)
    Q_PROPERTY(float currentFov READ currentFov NOTIFY currentFovChanged)

    // ========================================================================
    // CCIP (Bullet Impact Point with Zeroing + Lead)
    // ========================================================================
    Q_PROPERTY(bool ccipVisible READ ccipVisible NOTIFY ccipVisibleChanged)
    Q_PROPERTY(float ccipX READ ccipX NOTIFY ccipPositionChanged)
    Q_PROPERTY(float ccipY READ ccipY NOTIFY ccipPositionChanged)
    Q_PROPERTY(QString ccipStatus READ ccipStatus NOTIFY ccipStatusChanged)

    // ========================================================================
    // TRACKING
    // ========================================================================
    Q_PROPERTY(bool trackingActive READ trackingActive NOTIFY trackingActiveChanged)
    Q_PROPERTY(bool isTrackingActive READ isTrackingActive NOTIFY trackingActiveChanged)
    Q_PROPERTY(bool trackingBoxVisible READ trackingBoxVisible NOTIFY trackingBoxVisibleChanged)
    Q_PROPERTY(QRectF trackingBox READ trackingBox NOTIFY trackingBoxChanged)
    Q_PROPERTY(QString trackingBoxColor READ trackingBoxColor NOTIFY trackingBoxColorChanged)
    Q_PROPERTY(bool trackingBoxDashed READ trackingBoxDashed NOTIFY trackingBoxDashedChanged)
    Q_PROPERTY(float trackingConfidence READ trackingConfidence NOTIFY trackingConfidenceChanged)
    Q_PROPERTY(float confidenceLevel READ confidenceLevel NOTIFY trackingConfidenceChanged)

    // ========================================================================
    // ACQUISITION BOX (For Tracking Phase)
    // ========================================================================
    Q_PROPERTY(bool acquisitionBoxVisible READ acquisitionBoxVisible NOTIFY acquisitionBoxVisibleChanged)
    Q_PROPERTY(QRectF acquisitionBox READ acquisitionBox NOTIFY acquisitionBoxChanged)

    // ========================================================================
    // RANGE/LRF
    // ========================================================================
    Q_PROPERTY(float rangeMeters READ rangeMeters NOTIFY rangeMetersChanged)

    // ========================================================================
    // ZONE WARNINGS
    // ========================================================================
    Q_PROPERTY(bool zoneWarningVisible READ zoneWarningVisible NOTIFY zoneWarningChanged)
    Q_PROPERTY(QString zoneWarningText READ zoneWarningText NOTIFY zoneWarningTextChanged)

    // ========================================================================
    // MESSAGES (Startup, Errors)
    // ========================================================================
    Q_PROPERTY(bool startupMessageVisible READ startupMessageVisible NOTIFY startupMessageVisibleChanged)
    Q_PROPERTY(QString startupMessageText READ startupMessageText NOTIFY startupMessageTextChanged)
    Q_PROPERTY(bool errorMessageVisible READ errorMessageVisible NOTIFY errorMessageVisibleChanged)
    Q_PROPERTY(QString errorMessageText READ errorMessageText NOTIFY errorMessageTextChanged)

    // ========================================================================
    // WEAPON STATUS
    // ========================================================================
    Q_PROPERTY(bool ammunitionLevel READ ammunitionLevel NOTIFY ammunitionLevelChanged)

    // ========================================================================
    // DEVICE HEALTH MONITORING
    // ========================================================================
    Q_PROPERTY(bool dayCameraConnected READ dayCameraConnected NOTIFY dayCameraConnectedChanged)
    Q_PROPERTY(bool dayCameraError READ dayCameraError NOTIFY dayCameraErrorChanged)
    Q_PROPERTY(bool nightCameraConnected READ nightCameraConnected NOTIFY nightCameraConnectedChanged)
    Q_PROPERTY(bool nightCameraError READ nightCameraError NOTIFY nightCameraErrorChanged)
    Q_PROPERTY(bool azServoConnected READ azServoConnected NOTIFY azServoConnectedChanged)
    Q_PROPERTY(bool azFault READ azFault NOTIFY azFaultChanged)
    Q_PROPERTY(bool elServoConnected READ elServoConnected NOTIFY elServoConnectedChanged)
    Q_PROPERTY(bool elFault READ elFault NOTIFY elFaultChanged)
    Q_PROPERTY(bool lrfConnected READ lrfConnected NOTIFY lrfConnectedChanged)
    Q_PROPERTY(bool lrfFault READ lrfFault NOTIFY lrfFaultChanged)
    Q_PROPERTY(bool lrfOverTemp READ lrfOverTemp NOTIFY lrfOverTempChanged)
    Q_PROPERTY(bool actuatorConnected READ actuatorConnected NOTIFY actuatorConnectedChanged)
    Q_PROPERTY(bool actuatorFault READ actuatorFault NOTIFY actuatorFaultChanged)
    Q_PROPERTY(bool plc21Connected READ plc21Connected NOTIFY plc21ConnectedChanged)
    Q_PROPERTY(bool plc42Connected READ plc42Connected NOTIFY plc42ConnectedChanged)
    Q_PROPERTY(bool joystickConnected READ joystickConnected NOTIFY joystickConnectedChanged)

public:
    explicit OsdViewModel(QObject *parent = nullptr);
    ~OsdViewModel() override = default;

    // Main update method called from MainWindow::handleFrameData
    void updateFromFrameData(const FrameData &data);

    // Setters for device health (called from MainWindow based on controller states)
    void setDayCameraConnected(bool connected);
    void setDayCameraError(bool error);
    void setNightCameraConnected(bool connected);
    void setNightCameraError(bool error);
    void setAzServoConnected(bool connected);
    void setAzFault(bool fault);
    void setElServoConnected(bool connected);
    void setElFault(bool fault);
    void setLrfConnected(bool connected);
    void setLrfFault(bool fault);
    void setLrfOverTemp(bool overTemp);
    void setActuatorConnected(bool connected);
    void setActuatorFault(bool fault);
    void setPlc21Connected(bool connected);
    void setPlc42Connected(bool connected);
    void setJoystickConnected(bool connected);

    // Setters for messages
    void setStartupMessage(const QString &message, bool visible);
    void setErrorMessage(const QString &message, bool visible);

    // Setter for vehicle heading (from IMU or external source)
    void setVehicleHeading(float heading);

    // Getters (Q_PROPERTY read functions)
    QColor accentColor() const { return m_accentColor; }
    QString modeText() const { return m_modeText; }
    QString motionText() const { return m_motionText; }
    QString rateText() const { return m_rateText; }
    QString stabText() const { return m_stabText; }
    QString cameraText() const { return m_cameraText; }
    QString fovText() const { return m_fovText; }
    QString lrfText() const { return m_lrfText; }
    QString statusText() const { return m_statusText; }
    QString speedText() const { return m_speedText; }

    bool zeroingVisible() const { return m_zeroingVisible; }
    QString zeroingText() const { return m_zeroingText; }
    bool windageVisible() const { return m_windageVisible; }
    QString windageText() const { return m_windageText; }
    bool environmentVisible() const { return m_environmentVisible; }
    QString environmentText() const { return m_environmentText; }

    bool detectionVisible() const { return m_detectionVisible; }
    QString detectionText() const { return m_detectionText; }
    QVariantList detectionBoxes() const { return m_detectionBoxes; }

    bool scanNameVisible() const { return m_scanNameVisible; }
    QString scanNameText() const { return m_scanNameText; }

    bool leadAngleVisible() const { return m_leadAngleVisible; }
    QString leadAngleText() const { return m_leadAngleText; }
    bool lacActive() const { return m_lacActive; }
    float leadAngleOffsetAz() const { return m_leadAngleOffsetAz; }
    float leadAngleOffsetEl() const { return m_leadAngleOffsetEl; }

    float azimuth() const { return m_azimuth; }
    float elevation() const { return m_elevation; }
    float vehicleHeading() const { return m_vehicleHeading; }

    bool imuConnected() const { return m_imuConnected; }
    double imuRollDeg() const { return m_imuRollDeg; }
    double imuPitchDeg() const { return m_imuPitchDeg; }
    double imuYawDeg() const { return m_imuYawDeg; }
    double imuTemp() const { return m_imuTemp; }
    double gyroX() const { return m_gyroX; }
    double gyroY() const { return m_gyroY; }
    double gyroZ() const { return m_gyroZ; }
    double accelX() const { return m_accelX; }
    double accelY() const { return m_accelY; }
    double accelZ() const { return m_accelZ; }

    int reticleType() const { return m_reticleType; }
    float reticleOffsetX() const { return m_reticleOffsetX; }
    float reticleOffsetY() const { return m_reticleOffsetY; }
    float currentFov() const { return m_currentFov; }

    bool ccipVisible() const { return m_ccipVisible; }
    float ccipX() const { return m_ccipX; }
    float ccipY() const { return m_ccipY; }
    QString ccipStatus() const { return m_ccipStatus; }

    bool trackingActive() const { return m_trackingActive; }
    bool isTrackingActive() const { return m_trackingActive; } // Alias
    bool trackingBoxVisible() const { return m_trackingBoxVisible; }
    QRectF trackingBox() const { return m_trackingBox; }
    QString trackingBoxColor() const { return m_trackingBoxColor; }
    bool trackingBoxDashed() const { return m_trackingBoxDashed; }
    float trackingConfidence() const { return m_trackingConfidence; }
    float confidenceLevel() const { return m_trackingConfidence; } // Alias

    bool acquisitionBoxVisible() const { return m_acquisitionBoxVisible; }
    QRectF acquisitionBox() const { return m_acquisitionBox; }

    float rangeMeters() const { return m_rangeMeters; }

    bool zoneWarningVisible() const { return m_zoneWarningVisible; }
    QString zoneWarningText() const { return m_zoneWarningText; }

    bool startupMessageVisible() const { return m_startupMessageVisible; }
    QString startupMessageText() const { return m_startupMessageText; }
    bool errorMessageVisible() const { return m_errorMessageVisible; }
    QString errorMessageText() const { return m_errorMessageText; }

    bool ammunitionLevel() const { return m_ammunitionLevel; }

    bool dayCameraConnected() const { return m_dayCameraConnected; }
    bool dayCameraError() const { return m_dayCameraError; }
    bool nightCameraConnected() const { return m_nightCameraConnected; }
    bool nightCameraError() const { return m_nightCameraError; }
    bool azServoConnected() const { return m_azServoConnected; }
    bool azFault() const { return m_azFault; }
    bool elServoConnected() const { return m_elServoConnected; }
    bool elFault() const { return m_elFault; }
    bool lrfConnected() const { return m_lrfConnected; }
    bool lrfFault() const { return m_lrfFault; }
    bool lrfOverTemp() const { return m_lrfOverTemp; }
    bool actuatorConnected() const { return m_actuatorConnected; }
    bool actuatorFault() const { return m_actuatorFault; }
    bool plc21Connected() const { return m_plc21Connected; }
    bool plc42Connected() const { return m_plc42Connected; }
    bool joystickConnected() const { return m_joystickConnected; }

signals:
    // Visual styling
    void accentColorChanged();

    // System status
    void modeTextChanged();
    void motionTextChanged();
    void rateTextChanged();
    void stabTextChanged();
    void cameraTextChanged();
    void fovTextChanged();
    void lrfTextChanged();
    void statusTextChanged();
    void speedTextChanged();

    // Procedures
    void zeroingVisibleChanged();
    void zeroingTextChanged();
    void windageVisibleChanged();
    void windageTextChanged();
    void environmentVisibleChanged();
    void environmentTextChanged();

    // Detection
    void detectionVisibleChanged();
    void detectionTextChanged();
    void detectionBoxesChanged();

    // Scan patterns
    void scanNameVisibleChanged();
    void scanNameTextChanged();

    // Lead angle
    void leadAngleVisibleChanged();
    void leadAngleTextChanged();
    void lacActiveChanged();
    void leadAngleOffsetsChanged();

    // Gimbal position
    void azimuthChanged();
    void elevationChanged();
    void vehicleHeadingChanged();

    // IMU data
    void imuConnectedChanged();
    void imuDataChanged();

    // Reticle
    void reticleTypeChanged();
    void reticlePositionChanged();
    void currentFovChanged();

    // CCIP
    void ccipVisibleChanged();
    void ccipPositionChanged();
    void ccipStatusChanged();

    // Tracking
    void trackingActiveChanged();
    void trackingBoxVisibleChanged();
    void trackingBoxChanged();
    void trackingBoxColorChanged();
    void trackingBoxDashedChanged();
    void trackingConfidenceChanged();

    // Acquisition box
    void acquisitionBoxVisibleChanged();
    void acquisitionBoxChanged();

    // Range
    void rangeMetersChanged();

    // Zone warnings
    void zoneWarningChanged();
    void zoneWarningTextChanged();

    // Messages
    void startupMessageVisibleChanged();
    void startupMessageTextChanged();
    void errorMessageVisibleChanged();
    void errorMessageTextChanged();

    // Weapon status
    void ammunitionLevelChanged();

    // Device health
    void dayCameraConnectedChanged();
    void dayCameraErrorChanged();
    void nightCameraConnectedChanged();
    void nightCameraErrorChanged();
    void azServoConnectedChanged();
    void azFaultChanged();
    void elServoConnectedChanged();
    void elFaultChanged();
    void lrfConnectedChanged();
    void lrfFaultChanged();
    void lrfOverTempChanged();
    void actuatorConnectedChanged();
    void actuatorFaultChanged();
    void plc21ConnectedChanged();
    void plc42ConnectedChanged();
    void joystickConnectedChanged();

private:
    // Helper methods
    QString formatMode(int mode) const;
    QString formatMotionMode(int motionMode) const;
    QString formatFireMode(int fireMode) const;
    QVariantMap createDetectionBox(const YoloDetection &detection) const;

    // Member variables - Visual Styling
    QColor m_accentColor;

    // System Status
    QString m_modeText;
    QString m_motionText;
    QString m_rateText;
    QString m_stabText;
    QString m_cameraText;
    QString m_fovText;
    QString m_lrfText;
    QString m_statusText;
    QString m_speedText;

    // Procedures
    bool m_zeroingVisible;
    QString m_zeroingText;
    bool m_windageVisible;
    QString m_windageText;
    bool m_environmentVisible;
    QString m_environmentText;

    // Detection
    bool m_detectionVisible;
    QString m_detectionText;
    QVariantList m_detectionBoxes;

    // Scan Patterns
    bool m_scanNameVisible;
    QString m_scanNameText;

    // Lead Angle Compensation
    bool m_leadAngleVisible;
    QString m_leadAngleText;
    bool m_lacActive;
    float m_leadAngleOffsetAz;
    float m_leadAngleOffsetEl;

    // Gimbal Position
    float m_azimuth;
    float m_elevation;
    float m_vehicleHeading;

    // IMU Sensor Data
    bool m_imuConnected;
    double m_imuRollDeg;
    double m_imuPitchDeg;
    double m_imuYawDeg;
    double m_imuTemp;
    double m_gyroX;
    double m_gyroY;
    double m_gyroZ;
    double m_accelX;
    double m_accelY;
    double m_accelZ;

    // Reticle (Gun Boresight)
    int m_reticleType;
    float m_reticleOffsetX;
    float m_reticleOffsetY;
    float m_currentFov;

    // CCIP (Bullet Impact Point)
    bool m_ccipVisible;
    float m_ccipX;
    float m_ccipY;
    QString m_ccipStatus;

    // Tracking
    bool m_trackingActive;
    bool m_trackingBoxVisible;
    QRectF m_trackingBox;
    QString m_trackingBoxColor;
    bool m_trackingBoxDashed;
    float m_trackingConfidence;

    // Acquisition Box
    bool m_acquisitionBoxVisible;
    QRectF m_acquisitionBox;

    // Range/LRF
    float m_rangeMeters;

    // Zone Warnings
    bool m_zoneWarningVisible;
    QString m_zoneWarningText;

    // Messages
    bool m_startupMessageVisible;
    QString m_startupMessageText;
    bool m_errorMessageVisible;
    QString m_errorMessageText;

    // Weapon Status
    bool m_ammunitionLevel;

    // Device Health Monitoring
    bool m_dayCameraConnected;
    bool m_dayCameraError;
    bool m_nightCameraConnected;
    bool m_nightCameraError;
    bool m_azServoConnected;
    bool m_azFault;
    bool m_elServoConnected;
    bool m_elFault;
    bool m_lrfConnected;
    bool m_lrfFault;
    bool m_lrfOverTemp;
    bool m_actuatorConnected;
    bool m_actuatorFault;
    bool m_plc21Connected;
    bool m_plc42Connected;
    bool m_joystickConnected;
};

#endif // OSDVIEWMODEL_H
