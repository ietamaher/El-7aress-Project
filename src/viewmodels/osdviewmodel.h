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
 * Property names match existing OsdViewModel interface.
 */
class OsdViewModel : public QObject
{
    Q_OBJECT

    // ========================================================================
    // CORE DISPLAY PROPERTIES
    // ========================================================================
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QString modeText READ modeText NOTIFY modeTextChanged)
    Q_PROPERTY(QString motionText READ motionText NOTIFY motionTextChanged)
    Q_PROPERTY(QString stabText READ stabText NOTIFY stabTextChanged)
    Q_PROPERTY(QString cameraText READ cameraText NOTIFY cameraTextChanged)
    Q_PROPERTY(QString speedText READ speedText NOTIFY speedTextChanged)

    // ========================================================================
    // GIMBAL POSITION
    // ========================================================================
    Q_PROPERTY(float azimuth READ azimuth NOTIFY azimuthChanged)
    Q_PROPERTY(float elevation READ elevation NOTIFY elevationChanged)

    Q_PROPERTY(bool imuConnected READ imuConnected NOTIFY imuConnectedChanged)
    Q_PROPERTY(double vehicleHeading READ vehicleHeading NOTIFY vehicleHeadingChanged)
    Q_PROPERTY(double vehicleRoll READ vehicleRoll NOTIFY vehicleRollChanged)
    Q_PROPERTY(double vehiclePitch READ vehiclePitch NOTIFY vehiclePitchChanged)
    Q_PROPERTY(double imuTemperature READ imuTemperature NOTIFY imuTemperatureChanged)

    // ========================================================================
    // SYSTEM STATUS
    // ========================================================================
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString rateText READ rateText NOTIFY rateTextChanged)
    Q_PROPERTY(QString lrfText READ lrfText NOTIFY lrfTextChanged)
    Q_PROPERTY(QString fovText READ fovText NOTIFY fovTextChanged)

    // ========================================================================
    // TRACKING
    // ========================================================================
    Q_PROPERTY(QRectF trackingBox READ trackingBox NOTIFY trackingBoxChanged)
    Q_PROPERTY(bool trackingBoxVisible READ trackingBoxVisible NOTIFY trackingBoxVisibleChanged)
    Q_PROPERTY(QColor trackingBoxColor READ trackingBoxColor NOTIFY trackingBoxColorChanged)
    Q_PROPERTY(bool trackingBoxDashed READ trackingBoxDashed NOTIFY trackingBoxDashedChanged)
    Q_PROPERTY(bool isTrackingActive READ isTrackingActive NOTIFY isTrackingActiveChanged)
    Q_PROPERTY(float trackingConfidence READ trackingConfidence NOTIFY trackingConfidenceChanged)
    Q_PROPERTY(bool trackingActive READ trackingActive NOTIFY trackingActiveChanged)

    // Acquisition box (for Tracking_Acquisition phase)
    Q_PROPERTY(QRectF acquisitionBox READ acquisitionBox NOTIFY acquisitionBoxChanged)
    Q_PROPERTY(bool acquisitionBoxVisible READ acquisitionBoxVisible NOTIFY acquisitionBoxVisibleChanged)

    // ========================================================================
    // RETICLE (Main aiming reticle - with zeroing only)
    // ========================================================================
    Q_PROPERTY(int reticleType READ reticleType NOTIFY reticleTypeChanged)
    Q_PROPERTY(float reticleOffsetX READ reticleOffsetX NOTIFY reticleOffsetChanged)
    Q_PROPERTY(float reticleOffsetY READ reticleOffsetY NOTIFY reticleOffsetChanged)
    Q_PROPERTY(float currentFov READ currentFov NOTIFY currentFovChanged)

    // ========================================================================
    // CCIP PIPPER (Impact prediction with lead angle)
    // ========================================================================
    Q_PROPERTY(float ccipX READ ccipX NOTIFY ccipPositionChanged)
    Q_PROPERTY(float ccipY READ ccipY NOTIFY ccipPositionChanged)
    Q_PROPERTY(bool ccipVisible READ ccipVisible NOTIFY ccipVisibleChanged)
    Q_PROPERTY(QString ccipStatus READ ccipStatus NOTIFY ccipStatusChanged)

    // ========================================================================
    // PROCEDURES (Zeroing, Environment)
    // ========================================================================
    Q_PROPERTY(QString zeroingText READ zeroingText NOTIFY zeroingTextChanged)
    Q_PROPERTY(bool zeroingVisible READ zeroingVisible NOTIFY zeroingVisibleChanged)

    Q_PROPERTY(QString environmentText READ environmentText NOTIFY environmentTextChanged)
    Q_PROPERTY(bool environmentVisible READ environmentVisible NOTIFY environmentVisibleChanged)

    Q_PROPERTY(QString windageText READ windageText NOTIFY windageTextChanged)
    Q_PROPERTY(bool windageVisible READ windageVisible NOTIFY windageVisibleChanged)

    Q_PROPERTY(QString detectionText READ detectionText NOTIFY detectionTextChanged)
    Q_PROPERTY(bool detectionVisible READ detectionVisible NOTIFY detectionVisibleChanged)
    Q_PROPERTY(QVariantList detectionBoxes READ detectionBoxes NOTIFY detectionBoxesChanged)

    // ========================================================================
    // ZONE WARNINGS
    // ========================================================================
    Q_PROPERTY(QString zoneWarningText READ zoneWarningText NOTIFY zoneWarningTextChanged)
    Q_PROPERTY(bool zoneWarningVisible READ zoneWarningVisible NOTIFY zoneWarningVisibleChanged)

    // ========================================================================
    // LEAD ANGLE & SCAN
    // ========================================================================
    Q_PROPERTY(QString leadAngleText READ leadAngleText NOTIFY leadAngleTextChanged)
    Q_PROPERTY(bool leadAngleVisible READ leadAngleVisible NOTIFY leadAngleVisibleChanged)

    Q_PROPERTY(QString scanNameText READ scanNameText NOTIFY scanNameTextChanged)
    Q_PROPERTY(bool scanNameVisible READ scanNameVisible NOTIFY scanNameVisibleChanged)

    Q_PROPERTY(bool lacActive READ lacActive NOTIFY lacActiveChanged)
    Q_PROPERTY(float rangeMeters READ rangeMeters NOTIFY rangeMetersChanged)
    Q_PROPERTY(float confidenceLevel READ confidenceLevel NOTIFY confidenceLevelChanged)

    // ========================================================================
    // STARTUP SEQUENCE & ERROR MESSAGES
    // ========================================================================
    Q_PROPERTY(QString startupMessageText READ startupMessageText NOTIFY startupMessageTextChanged)
    Q_PROPERTY(bool startupMessageVisible READ startupMessageVisible NOTIFY startupMessageVisibleChanged)

    Q_PROPERTY(QString errorMessageText READ errorMessageText NOTIFY errorMessageTextChanged)
    Q_PROPERTY(bool errorMessageVisible READ errorMessageVisible NOTIFY errorMessageVisibleChanged)

    // ========================================================================
    // DEVICE HEALTH STATUS (for warning displays)
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

    Q_PROPERTY(bool ammunitionLevel READ ammunitionLevel NOTIFY ammunitionLevelChanged)

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

    // Getters (Q_PROPERTY read functions)
    QColor accentColor() const { return m_accentColor; }
    QString modeText() const { return m_modeText; }
    QString motionText() const { return m_motionText; }
    QString stabText() const { return m_stabText; }
    QString cameraText() const { return m_cameraText; }
    QString speedText() const { return m_speedText; }

    float azimuth() const { return m_azimuth; }
    float elevation() const { return m_elevation; }

    bool imuConnected() const { return m_imuConnected; }
    double vehicleHeading() const { return m_vehicleHeading; }
    double vehicleRoll() const { return m_vehicleRoll; }
    double vehiclePitch() const { return m_vehiclePitch; }
    double imuTemperature() const { return m_imuTemperature; }

    QString statusText() const { return m_statusText; }
    QString rateText() const { return m_rateText; }
    QString lrfText() const { return m_lrfText; }
    QString fovText() const { return m_fovText; }

    QRectF trackingBox() const { return m_trackingBox; }
    bool trackingBoxVisible() const { return m_trackingBoxVisible; }
    QColor trackingBoxColor() const { return m_trackingBoxColor; }
    bool trackingBoxDashed() const { return m_trackingBoxDashed; }
    bool isTrackingActive() const { return m_trackingActive; }
    float trackingConfidence() const { return m_trackingConfidence; }
    bool trackingActive() const { return m_trackingActive; }

    QRectF acquisitionBox() const { return m_acquisitionBox; }
    bool acquisitionBoxVisible() const { return m_acquisitionBoxVisible; }

    int reticleType() const { return m_reticleType; }
    float reticleOffsetX() const { return m_reticleOffsetX; }
    float reticleOffsetY() const { return m_reticleOffsetY; }
    float currentFov() const { return m_currentFov; }

    float ccipX() const { return m_ccipX; }
    float ccipY() const { return m_ccipY; }
    bool ccipVisible() const { return m_ccipVisible; }
    QString ccipStatus() const { return m_ccipStatus; }

    QString zeroingText() const { return m_zeroingText; }
    bool zeroingVisible() const { return m_zeroingVisible; }

    QString environmentText() const { return m_environmentText; }
    bool environmentVisible() const { return m_environmentVisible; }

    QString windageText() const { return m_windageText; }
    bool windageVisible() const { return m_windageVisible; }

    QString detectionText() const { return m_detectionText; }
    bool detectionVisible() const { return m_detectionVisible; }
    QVariantList detectionBoxes() const { return m_detectionBoxes; }

    QString zoneWarningText() const { return m_zoneWarningText; }
    bool zoneWarningVisible() const { return m_zoneWarningVisible; }

    QString leadAngleText() const { return m_leadAngleText; }
    bool leadAngleVisible() const { return m_leadAngleVisible; }

    QString scanNameText() const { return m_scanNameText; }
    bool scanNameVisible() const { return m_scanNameVisible; }

    bool lacActive() const { return m_lacActive; }
    float rangeMeters() const { return m_rangeMeters; }
    float confidenceLevel() const { return m_trackingConfidence; }

    QString startupMessageText() const { return m_startupMessageText; }
    bool startupMessageVisible() const { return m_startupMessageVisible; }

    QString errorMessageText() const { return m_errorMessageText; }
    bool errorMessageVisible() const { return m_errorMessageVisible; }

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

    bool ammunitionLevel() const { return m_ammunitionLevel; }

signals:
    // Core display
    void accentColorChanged();
    void modeTextChanged();
    void motionTextChanged();
    void stabTextChanged();
    void cameraTextChanged();
    void speedTextChanged();

    // Gimbal position
    void azimuthChanged();
    void elevationChanged();

    // IMU
    void imuConnectedChanged();
    void vehicleHeadingChanged();
    void vehicleRollChanged();
    void vehiclePitchChanged();
    void imuTemperatureChanged();

    // System status
    void statusTextChanged();
    void rateTextChanged();
    void lrfTextChanged();
    void fovTextChanged();

    // Tracking
    void trackingBoxChanged();
    void trackingBoxVisibleChanged();
    void trackingBoxColorChanged();
    void trackingBoxDashedChanged();
    void isTrackingActiveChanged();
    void trackingConfidenceChanged();
    void trackingActiveChanged();

    void acquisitionBoxChanged();
    void acquisitionBoxVisibleChanged();

    // Reticle
    void reticleTypeChanged();
    void reticleOffsetChanged();
    void currentFovChanged();

    // CCIP
    void ccipPositionChanged();
    void ccipVisibleChanged();
    void ccipStatusChanged();

    // Procedures
    void zeroingTextChanged();
    void zeroingVisibleChanged();

    void environmentTextChanged();
    void environmentVisibleChanged();

    void windageTextChanged();
    void windageVisibleChanged();

    void detectionTextChanged();
    void detectionVisibleChanged();
    void detectionBoxesChanged();

    // Zone warnings
    void zoneWarningTextChanged();
    void zoneWarningVisibleChanged();

    // Lead angle & scan
    void leadAngleTextChanged();
    void leadAngleVisibleChanged();

    void scanNameTextChanged();
    void scanNameVisibleChanged();

    void lacActiveChanged();
    void rangeMetersChanged();
    void confidenceLevelChanged();

    // Messages
    void startupMessageTextChanged();
    void startupMessageVisibleChanged();

    void errorMessageTextChanged();
    void errorMessageVisibleChanged();

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

    void ammunitionLevelChanged();

private:
    // Helper methods
    QString formatMode(int mode) const;
    QString formatMotionMode(int motionMode) const;
    QString formatFireMode(int fireMode) const;
    QVariantMap createDetectionBox(const YoloDetection &detection) const;

    // Member variables - Core Display
    QColor m_accentColor;
    QString m_modeText;
    QString m_motionText;
    QString m_stabText;
    QString m_cameraText;
    QString m_speedText;

    // Gimbal Position
    float m_azimuth;
    float m_elevation;

    // IMU
    bool m_imuConnected;
    double m_vehicleHeading;
    double m_vehicleRoll;
    double m_vehiclePitch;
    double m_imuTemperature;

    // System Status
    QString m_statusText;
    QString m_rateText;
    QString m_lrfText;
    QString m_fovText;

    // Tracking
    QRectF m_trackingBox;
    bool m_trackingBoxVisible;
    QColor m_trackingBoxColor;
    bool m_trackingBoxDashed;
    bool m_trackingActive;
    float m_trackingConfidence;

    QRectF m_acquisitionBox;
    bool m_acquisitionBoxVisible;

    // Reticle
    int m_reticleType;
    float m_reticleOffsetX;
    float m_reticleOffsetY;
    float m_currentFov;

    // CCIP
    float m_ccipX;
    float m_ccipY;
    bool m_ccipVisible;
    QString m_ccipStatus;

    // Procedures
    QString m_zeroingText;
    bool m_zeroingVisible;

    QString m_environmentText;
    bool m_environmentVisible;

    QString m_windageText;
    bool m_windageVisible;

    QString m_detectionText;
    bool m_detectionVisible;
    QVariantList m_detectionBoxes;

    // Zone Warnings
    QString m_zoneWarningText;
    bool m_zoneWarningVisible;

    // Lead Angle & Scan
    QString m_leadAngleText;
    bool m_leadAngleVisible;

    QString m_scanNameText;
    bool m_scanNameVisible;

    bool m_lacActive;
    float m_rangeMeters;

    // Messages
    QString m_startupMessageText;
    bool m_startupMessageVisible;

    QString m_errorMessageText;
    bool m_errorMessageVisible;

    // Device Health
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

    bool m_ammunitionLevel;
};

#endif // OSDVIEWMODEL_H
