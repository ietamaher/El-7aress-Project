#ifndef OSDRENDERER_OPTIMIZED_H
#define OSDRENDERER_OPTIMIZED_H

#include <QObject>
#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QFont>
#include <QColor>
#include <QPointF>
#include <QString>
#include <vector>
#include <QGraphicsItemGroup> 

// Forward declarations
class OutlinedTextItem;

// Include necessary enums/structs directly or via a dedicated header
#include "../models/systemstatemodel.h" // Contains OperationalMode, MotionMode, FireMode, ReticleType
#include <vpi/algo/DCFTracker.h>     // Contains VPITrackingState
#include "../utils/inference.h"         // Contains Detection struct

/**
 * @brief Renders On-Screen Display (OSD) elements over a base video image.
 *
 * Manages various graphical elements like status text, indicators (azimuth, elevation),
 * reticles, tracking boxes, and detection boxes within a QGraphicsScene.
 */
class OsdRenderer : public QObject
{
    Q_OBJECT

public:
    explicit OsdRenderer(int width, int height, QObject *parent = nullptr);
    ~OsdRenderer() override;

    /**
     * @brief Renders the current OSD state onto the provided base image.
     * @param baseImage The background image (e.g., video frame).
     * @return A new QImage with the OSD elements drawn on top.
     */
    QImage renderOsd(const QImage &baseImage);

public slots:
    // --- Update Slots for OSD Data ---
    void updateMode(OperationalMode mode);
    void updateMotionMode(MotionMode motionMode);
    void updateStabilization(bool enabled);
    void updateCameraType(const QString &cameraType);
    void updateLrfDistance(float distance);
    void updateSystemStatus(bool charged, bool armed, bool ready);
    void updateFiringMode(FireMode rate);
    void updateFov(float fov);
    void updateSpeed(double speed);
    void updateAzimuth(float azimuth);
    void updateElevation(float elevation);
    void updateTrackingState(VPITrackingState state);
    void updateTrackingBox(float x, float y, float width, float height);
    void updateTrackingPhaseDisplay(
        TrackingPhase phase,
        bool hasValidLock, // Equivalent to trackerHasValidTarget
        const QRectF& acquisitionBox, // The user-defined acquisition gate
        const QRectF& trackedBbox     // The box from the active tracker
        );

    void updateDetectionBoxes(const std::vector<YoloDetection> &detections);
    void updateReticleType(ReticleType type);
    void updateColorStyle(QColor style);
    void updateReticlePosition(float screenX_px, float screenY_px); // Receives final pixel coords
    void updateLeadStatusText(const QString& text); // Receives pre-formatted text
    //void updateZeroingStatusText(const QString& text);
    //void updateFovForDisplay(float hfov); // For FOV text and MilDot internal scaling
    /**
     * @brief Updates the OSD elements related to weapon zeroing.
     * @param zeroingModeActive True if the zeroing procedure UI is active.
     * @param zeroingApplied True if zeroing offsets are currently applied to ballistics.
     * @param azOffset Current azimuth zeroing offset (for potential display).
     * @param elOffset Current elevation zeroing offset (for potential display).
     */
    void updateZeroingDisplay(bool zeroingModeActive, bool zeroingApplied, float azOffset, float elOffset);

    /**
     * @brief Updates the OSD elements related to windage settings.
     * @param windageModeActive True if the windage setting UI is active.
     * @param windageApplied True if windage is currently applied to ballistics.
     * @param speedKnots Current wind speed in knots (for potential display).
     */
    void updateWindageDisplay(bool windageModeActive, bool windageApplied, float speedKnots);
    void updateAppliedZeroingOffsets(bool applied, float azOffset, float elOffset);
    void updateZoneWarning(bool inNoFireZone, bool inNoTraverseZoneAtLimit);
    void updateLeadAngleDisplay(bool active, LeadAngleStatus status, float offsetAz, float offsetEl);
    void updateCurrentScanNameDisplay(const QString& scanName);
    //void updateLeadAngleDisplay(bool active, LeadAngleStatus status, float offsetAzDegrees, float offsetElDegrees); // Now receives offsets
private:
    // --- Initialization and Setup ---
    void initializeScene();
    void setupPensAndBrushes(); // Helper to initialize/update pens based on m_osdColor
    void createReticle();       // Combined reticle creation logic

    // --- Helper Functions for Creating Graphics Items ---
    OutlinedTextItem* createTextItem(const QPointF &pos, qreal zValue); // Helper for text items
    void addReticlePathWithOutline(const QPainterPath& path); // Helper for reticle parts
    void addReticleShapeWithOutline(const QPainterPath &path);
    void addTickMarks(QGraphicsScene& scene, const QPointF& center, qreal radius, int startDeg, int endDeg, int stepDeg, qreal majorTickLen, qreal minorTickLen, const QPen& mainPen, const QPen& outlinePen, qreal zMain, qreal zOutline, std::vector<QGraphicsLineItem*>& mainTicks, std::vector<QGraphicsLineItem*>& outlineTicks, bool isAzimuth = true);
    void addCardinalLabels(const QPointF& center, qreal radius, qreal labelOffset);
    void addElevationLabels(qreal scaleX, qreal scaleYBase, qreal scaleHeight, qreal elMin, qreal elRange);

    // --- Update Functions for Specific OSD Elements ---
    void updateStatusText();
    void updateAzimuthIndicator();
    void updateElevationScale();
    void updateTrackingCorners(float x, float y, float width, float height); // Combined update for corners
    void clearReticleComponents();
    void clearDetectionGraphics();
    void drawDetectionBox(const YoloDetection& detection);

    // --- Reticle Creation Functions ---
    void createBasicReticle();
    void createBoxCrosshairReticle();
    void createStandardCrosshairReticle();
    void createPrecisionCrosshairReticle();
    void createMilDotReticle();

    // --- Utility Functions ---
    double calculatePixelsPerMil(double horizontalFovDegrees, double screenWidthPixels);
    QPointF convertAngularOffsetToPixelOffset(float offsetAzDegrees, float offsetElDegrees);
    void applyReticleTranslation(); // Applies the pixel offsets to reticle QGraphicsItems
    void clearReticleDrawingItems(); // Clears children from m_reticleRootGroup
    void applyReticlePosition();     // Calculates total offset and moves m_reticleRootGroup
    QPointF convertAngularToPixelOffset(float offsetAzDegrees, float offsetElDegrees); // Helper

    // === CORE RENDERING COMPONENTS ===
    QGraphicsScene m_scene; // The scene holding all OSD items
    QGraphicsView m_view;   // The view used for rendering (could be internal)
    QGraphicsPixmapItem *m_backgroundItem = nullptr; // For the base image
    int m_width;  // Scene/View width
    int m_height; // Scene/View height

    // === STYLING AND APPEARANCE ===
    QColor m_osdColor; // Current primary OSD color
    QFont m_osdFont;   // Font used for text items
    int m_lineWidth;   // Base line width for OSD elements

    // Pens and Brushes (managed by setupPensAndBrushes)
    QPen m_mainPen;             // Pen for primary colored elements
    QPen m_shapeOutlinePen;     // Pen for outlines of shapes (circles, scales)
    QPen m_needleOutlinePen;    // Pen for thicker needle outlines
    QPen m_tickMarkMainPen;     // Pen for main tick marks
    QPen m_tickMarkOutlinePen;  // Pen for tick mark outlines
    QPen m_textOutlinePen;      // Pen for text outlines
    QPen m_trackingOutlinePen;  // Pen for tracking corner outlines
    QPen m_reticleOutlinePen;   // Pen for reticle outlines
    QBrush m_fillBrush;         // Brush for text fill and filled shapes

    // === SYSTEM STATE DATA ===
    OperationalMode m_currentMode = OperationalMode::Idle;
    MotionMode m_motionMode = MotionMode::Manual;
    bool m_stabEnabled = false;
    QString m_cameraType = "DAY";
    float m_lrfDistance = 0.0f;
    bool m_sysCharged = false;
    bool m_sysArmed = false;
    bool m_sysReady = false;
    FireMode m_fireMode = FireMode::SingleShot; // Default based on common usage
    float m_fov = 0.0f;
    double m_speed = 0.0;
    float m_azimuth = 0.0f;
    float m_elevation = 0.0f;
    VPITrackingState m_trackingState = VPI_TRACKING_STATE_LOST;

    // === RETICLE STATE AND POSITIONING ===
    ReticleType m_reticleType = ReticleType::BoxCrosshair; // Default reticle
    double m_currentHfov = 5; //63.7; // Default horizontal FOV in degrees
    QGraphicsItemGroup *m_reticleRootGroup = nullptr;   // Root for all reticle parts
    std::vector<QGraphicsItem*> m_currentReticleDrawingItems; // To keep track of items added to root group

    // Zeroing offsets
    float m_currentZeroingAzOffset = 0.0f;
    float m_currentZeroingElOffset = 0.0f;
    float m_currentZeroingAzOffsetDegrees = 0.0f;
    float m_currentZeroingElOffsetDegrees = 0.0f;
    float m_zeroingOffsetXPx = 0.0f;
    float m_zeroingOffsetYPx = 0.0f;
    bool m_isZeroingApplied = false;
    bool m_isZeroingCurrentlyApplied = false;

    // Lead angle offsets
    float m_reticleLeadOffsetXPx; // Current lead offset in X pixels
    float m_reticleLeadOffsetYPx; // Current lead offset in Y pixels
    float m_currentLeadAzOffsetDegrees = 0.0f;
    float m_currentLeadElOffsetDegrees = 0.0f;
    float m_leadOffsetXPx = 0.0f;
    float m_leadOffsetYPx = 0.0f;
    bool m_isLacActiveForReticle;
    bool m_isLacCurrentlyActive = false; // This was m_isLacActiveForReticle

    // State flags
    bool m_forceReticleRecreation = false;

    // === TEXT DISPLAY ITEMS ===
    OutlinedTextItem *m_modeTextItem = nullptr;
    OutlinedTextItem *m_motionTextItem = nullptr; // Kept for potential future use
    OutlinedTextItem *m_stabTextItem = nullptr;
    OutlinedTextItem *m_cameraTextItem = nullptr;
    OutlinedTextItem *m_lrfTextItem = nullptr;
    OutlinedTextItem *m_statusTextItem = nullptr;
    OutlinedTextItem *m_rateTextItem = nullptr;
    OutlinedTextItem *m_fovTextItem = nullptr;
    OutlinedTextItem *m_speedTextItem = nullptr;
    OutlinedTextItem *m_azTextItem = nullptr;
    OutlinedTextItem *m_elValueTextItem = nullptr;
    OutlinedTextItem *m_zoomTextItem = nullptr; // Assuming this relates to FOV/Camera
    OutlinedTextItem *m_zeroingDisplayItem = nullptr; // To show "ZEROING" or "Z"
    OutlinedTextItem *m_windageDisplayItem = nullptr; // To show "WINDAGE ACTIVE" or "W"
    OutlinedTextItem *m_zoneWarningItem = nullptr; // For "NO FIRE ZONE", "NO TRAVERSE ZONE"
    OutlinedTextItem *m_leadAngleStatusTextItem = nullptr;
    OutlinedTextItem *m_currentScanNameTextItem = nullptr; // For sector scan status

    // === AZIMUTH INDICATOR GRAPHICS ===
    QGraphicsEllipseItem* m_azimuthCircleOutline = nullptr;
    QGraphicsEllipseItem* m_azimuthCircle = nullptr;
    QGraphicsLineItem* m_azimuthNeedleOutline = nullptr;
    QGraphicsLineItem* m_azimuthNeedle = nullptr;
    std::vector<QGraphicsLineItem*> m_azimuthTicks;
    std::vector<QGraphicsLineItem*> m_azimuthTicksOutline;
    std::vector<OutlinedTextItem*> m_azimuthLabels; // Store labels if needed for updates

    // === ELEVATION SCALE GRAPHICS ===
    QGraphicsLineItem *m_elevationScaleOutline = nullptr;
    QGraphicsLineItem *m_elevationScale = nullptr;
    QGraphicsPathItem *m_elevationIndicatorOutline = nullptr;
    QGraphicsPathItem *m_elevationIndicator = nullptr;
    std::vector<QGraphicsLineItem*> m_elevationTicks;
    std::vector<QGraphicsLineItem*> m_elevationTicksOutline;
    std::vector<OutlinedTextItem*> m_elevationLabels; // Store labels if needed for updates

    // === RETICLE GRAPHICS ===
    std::vector<QGraphicsItem*> m_reticleItems; // Generic container for all reticle parts
    QGraphicsPathItem *m_fixedLobMarkerItem = nullptr; // For the static LOB mark at screen center
    QGraphicsPathItem *m_fixedLobMarkerOutlineItem = nullptr; // Optional outline for LOB mark

    // === TRACKING VISUALIZATION ===
    QGraphicsRectItem *m_trackingBox = nullptr; // The hidden bounding box for reference
    std::vector<QGraphicsLineItem*> m_trackingCorners;      // Main colored corners (size 8)
    std::vector<QGraphicsLineItem*> m_trackingCornersOutline; // Outline corners (size 8)

    // === DETECTION VISUALIZATION ===
    std::vector<QGraphicsRectItem*> m_detectionRectItems;
    std::vector<OutlinedTextItem*> m_detectionTextItems;
    std::vector<QGraphicsRectItem*> m_detectionRectOutlines;

    void debugReticlePositions();
};

#endif // OSDRENDERER_OPTIMIZED_H

