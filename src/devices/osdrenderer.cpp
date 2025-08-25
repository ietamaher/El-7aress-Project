#include "osdrenderer.h"
#include "outlinedtextitem.h" // Include the implementation dependency

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QDebug>
#include <cmath> // For M_PI, cos, sin
#include <QPainterPathStroker>

// === Constants ===
namespace {
// Z-Values for layering
constexpr qreal Z_ORDER_BACKGROUND = -1.0;
constexpr qreal Z_ORDER_OUTLINE = 9.0;
constexpr qreal Z_ORDER_MAIN = 10.0;
constexpr qreal Z_ORDER_TRACKING = 15.0;
constexpr qreal Z_ORDER_DETECTION = 12.0; // Ensure detections are visible but potentially behind tracking
constexpr qreal Z_ORDER_RETICLE_MAIN = 20.0; // Text should be on top of everything

// Default Colors (can be overridden by updateColorStyle)
const QColor DEFAULT_OSD_COLOR = QColor(70, 226, 165); // Green
const QColor DEFAULT_OUTLINE_COLOR = QColor(10, 10, 10);//(18, 67, 21); // Dark Green/Black
// Tracking Colors (Defined in original code, keep them)
//const QColor COLOR_TRACKING_DEFAULT = Qt::yellow;
//const QColor COLOR_TRACKING_ACQUIRING = Qt::cyan;
const QColor COLOR_TRACKING_ACQUIRED = Qt::green;

//const QColor COLOR_TRACKING_LOST = QColor(200,20,40);

// Default Font
const char* DEFAULT_FONT_FAMILY = "Archivo Narrow";
constexpr int DEFAULT_FONT_SIZE = 16;
constexpr QFont::Weight DEFAULT_FONT_WEIGHT = QFont::Bold;

// Default Line Width
constexpr int DEFAULT_LINE_WIDTH = 2;

// Element Positions & Sizes (Extracted from original code)
// Text Item Positions
const QPointF POS_MODE_TEXT(10, 25);
const QPointF POS_MOTION_TEXT(10, 55);
const QPointF POS_SPEED_TEXT(500, 25);
const QPointF POS_STAB_TEXT(100, 748);
const QPointF POS_CAMERA_TEXT(250, 748);
const QPointF POS_FOV_TEXT(425, 748);
const QPointF POS_ZOOM_TEXT(300, 748);
const QPointF POS_STATUS_TEXT(10, 120);
const QPointF POS_RATE_TEXT(10, 145);
const QPointF POS_LRF_TEXT(10, 170);
const QPointF POS_ZEROING_STATUS_TEXT(10, 195);  
const QPointF POS_WINDAGE_STATUS_TEXT(10, 220);
const QPointF POS_ZONE_WARNING_TEXT(1024 / 2.0 + 50, 768 / 2.0 + 50);
const QPointF POS_ZONE_LAC_TEXT(10, 245);
const QPointF POS_SCAN_NAME_TEXT(10, 270);

// Azimuth Indicator
constexpr qreal AZ_INDICATOR_X_OFFSET = 75; // Offset from right edge
constexpr qreal AZ_INDICATOR_Y = 75;
constexpr qreal AZ_RADIUS = 50;
constexpr qreal AZ_NEEDLE_LENGTH_FACTOR = 0.8;
constexpr qreal AZ_TICK_LENGTH_MAJOR = 8.0;
constexpr qreal AZ_TICK_LENGTH_MINOR = 4.0;
constexpr int AZ_TICK_STEP = 30; // Degrees
constexpr qreal AZ_LABEL_OFFSET = 12.0;
constexpr qreal AZ_TEXT_Y_OFFSET_FACTOR = 0.5; // Relative to radius
constexpr qreal AZ_TEXT_Y_EXTRA_OFFSET = 5.0;

// Elevation Scale
constexpr qreal EL_SCALE_X_OFFSET = 55; // Offset from right edge
constexpr qreal EL_SCALE_HEIGHT = 120;
constexpr qreal EL_SCALE_Y_OFFSET = 25; // Offset from bottom edge
constexpr qreal EL_RANGE =80.0;
constexpr qreal EL_MIN = -20.0;
constexpr qreal EL_TICK_LENGTH = 5.0;
constexpr qreal EL_MAJOR_TICK_LENGTH = 10.0;

constexpr qreal EL_LABEL_X_OFFSET = 13.0;
constexpr qreal EL_INDICATOR_WIDTH = 6.0; // Base of triangle is 8 (Y), point is 6 away (X)
constexpr qreal EL_INDICATOR_HEIGHT = 8.0;

// Tracking Corners
constexpr qreal TRACKING_CORNER_LENGTH = 15.0;

// Reticle Constants
constexpr double RETICLE_LINE_WIDTH = 2.0;
constexpr double RETICLE_OUTLINE_WIDTH_FACTOR = 2.0; // Multiplier for outline pen width
// Basic Reticle
constexpr qreal BASIC_RETICLE_SIZE = 20.0;
// Box Crosshair Reticle
constexpr qreal BOX_CROSSHAIR_LINE_LEN = 80.0;
constexpr qreal BOX_CROSSHAIR_BOX_SIZE = 50.0;
constexpr qreal BOX_CROSSHAIR_GAP = 2.0;
// Standard Crosshair Reticle
constexpr qreal STD_CROSSHAIR_SIZE = 60.0;
constexpr qreal STD_CROSSHAIR_GAP = 10.0;
// Precision Crosshair Reticle
constexpr qreal PRECISION_CROSSHAIR_SIZE = 100.0;
constexpr qreal PRECISION_CROSSHAIR_CENTER_DOT_RADIUS = 2.0;
constexpr qreal PRECISION_CROSSHAIR_TICK_LENGTH = 5.0;
constexpr int PRECISION_CROSSHAIR_NUM_TICKS = 5; // Per quadrant arm
constexpr qreal PRECISION_CROSSHAIR_TICK_SPACING = 15.0;
// Mil-Dot Reticle
constexpr qreal MILDOT_RETICLE_SIZE = 120.0;
constexpr qreal MILDOT_RETICLE_DOT_RADIUS = 1.5;
constexpr int MILDOT_RETICLE_NUM_DOTS = 4; // Per quadrant arm (excluding center)

// Detection Box
constexpr int DETECTION_TEXT_OFFSET_Y = -5; // Offset above the box

} // namespace

// === Constructor / Destructor ===

OsdRenderer::OsdRenderer(int width, int height, QObject *parent)
    : QObject(parent)
    , m_width(width)
    , m_height(height)
    , m_osdColor(DEFAULT_OSD_COLOR)
    , m_osdFont(QString::fromUtf8(DEFAULT_FONT_FAMILY), DEFAULT_FONT_SIZE, DEFAULT_FONT_WEIGHT)
    , m_lineWidth(DEFAULT_LINE_WIDTH)
    // Initialize state variables with defaults (as in original code or sensible defaults)
    , m_currentMode(OperationalMode::Idle)
    , m_motionMode(MotionMode::Manual)
    , m_stabEnabled(false)
    , m_cameraType("DAY")
    , m_lrfDistance(0.0f) // Initialize to 0 or a known invalid state
    , m_sysCharged(false)
    , m_sysArmed(false)
    , m_sysReady(false)
    , m_fireMode(FireMode::SingleShot)
    , m_fov(45.0f) // Example default
    , m_speed(0.0)
    , m_azimuth(0.0f)
    , m_elevation(0.0f)
    , m_trackingState(VPI_TRACKING_STATE_LOST)
    , m_reticleType(ReticleType::BoxCrosshair) // Default reticle
    , m_currentHfov(63.7) // Default HFOV
    // Null-initialize all graphics item pointers
    , m_backgroundItem(nullptr)
    , m_modeTextItem(nullptr)
    , m_motionTextItem(nullptr)
    , m_stabTextItem(nullptr)
    , m_cameraTextItem(nullptr)
    , m_lrfTextItem(nullptr)
    , m_statusTextItem(nullptr)
    , m_rateTextItem(nullptr)
    , m_fovTextItem(nullptr)
    , m_speedTextItem(nullptr)
    , m_azTextItem(nullptr)
    , m_elValueTextItem(nullptr)
    , m_zoomTextItem(nullptr)
    , m_zeroingDisplayItem(nullptr)   
    , m_windageDisplayItem(nullptr) 
    , m_azimuthCircleOutline(nullptr)
    , m_azimuthCircle(nullptr)
    , m_azimuthNeedleOutline(nullptr)
    , m_azimuthNeedle(nullptr)
    , m_elevationScaleOutline(nullptr)
    , m_elevationScale(nullptr)
    , m_elevationIndicatorOutline(nullptr)
    , m_elevationIndicator(nullptr)
    , m_trackingBox(nullptr)
    , m_reticleLeadOffsetXPx(0.0f)
    , m_reticleLeadOffsetYPx(0.0f)
    , m_isLacActiveForReticle(false)
{
    m_scene.setSceneRect(0, 0, width, height);

    // Configure the view (used internally for rendering)
    m_view.setScene(&m_scene);
    m_view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view.setRenderHint(QPainter::Antialiasing);
    m_view.setFixedSize(width, height);
    m_view.setViewportUpdateMode(QGraphicsView::NoViewportUpdate); // We control updates
    m_view.setBackgroundBrush(Qt::transparent); // Ensure transparency

    // Create the background item (initially empty and hidden)
    m_backgroundItem = m_scene.addPixmap(QPixmap());
    m_backgroundItem->setZValue(Z_ORDER_BACKGROUND);
    m_backgroundItem->setVisible(false); // Only visible during renderOsd

    // Initialize pens, brushes, and scene items
    setupPensAndBrushes();
    initializeScene();

    // Set initial reticle
    updateReticleType(m_reticleType);
}

OsdRenderer::~OsdRenderer()
{
    qDebug() << "OsdRenderer destructor";
    // Scene manages the items, so no explicit deletion needed here
    // unless items were allocated outside the scene's ownership.
    // Vectors of pointers owned by the scene are cleared automatically
    // when the scene is destroyed, but clearing them here is good practice.
    m_scene.clear(); // Removes and deletes all items
}

// === Public Methods ===

QImage OsdRenderer::renderOsd(const QImage &baseImage)
{
    // Update the background item
    m_backgroundItem->setPixmap(QPixmap::fromImage(baseImage));
    m_backgroundItem->setVisible(true);

    // Create the target image
    QImage resultImage(m_width, m_height, QImage::Format_ARGB32_Premultiplied);
    resultImage.fill(Qt::transparent);

    // Render the scene to the image
    QPainter painter(&resultImage);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene.render(&painter);
    painter.end(); // Ensure painting is finished

    // Hide the background item again for the next frame
    m_backgroundItem->setVisible(false);

    return resultImage;
}

// === Private Helper Functions ===

void OsdRenderer::setupPensAndBrushes()
{
    // Use a consistent outline color, or derive from m_osdColor if preferred
    // QColor currentOutlineColor = m_osdColor.darker(150);
    QColor currentOutlineColor = DEFAULT_OUTLINE_COLOR;

    m_fillBrush = QBrush(m_osdColor);

    // Text Outline Pen (thin)
    m_textOutlinePen = QPen(currentOutlineColor, 1);
    m_textOutlinePen.setJoinStyle(Qt::RoundJoin);

    // Main Pen (for primary colored elements)
    m_mainPen = QPen(m_osdColor, m_lineWidth);
    m_mainPen.setCosmetic(true); // Constant pixel width
    m_mainPen.setJoinStyle(Qt::RoundJoin);
    m_mainPen.setCapStyle(Qt::RoundCap);

    // Shape Outline Pen (slightly thicker)
    m_shapeOutlinePen = QPen(currentOutlineColor, m_lineWidth + 1);
    m_shapeOutlinePen.setCosmetic(true);
    m_shapeOutlinePen.setJoinStyle(Qt::RoundJoin);
    m_shapeOutlinePen.setCapStyle(Qt::RoundCap);

    // Needle Outline Pen (even thicker)
    m_needleOutlinePen = QPen(currentOutlineColor, m_lineWidth + 2);
    m_needleOutlinePen.setCosmetic(true);
    m_needleOutlinePen.setJoinStyle(Qt::RoundJoin);
    m_needleOutlinePen.setCapStyle(Qt::RoundCap);

    // Tick Mark Pens
    m_tickMarkMainPen = m_mainPen; // Use main pen for ticks
    m_tickMarkOutlinePen = m_shapeOutlinePen; // Use shape outline for tick outlines

    // Tracking Outline Pen
    m_trackingOutlinePen = QPen(currentOutlineColor, m_lineWidth + 2); // Thicker outline
    m_trackingOutlinePen.setCosmetic(true);

    // Reticle Outline Pen
    m_reticleOutlinePen = QPen(currentOutlineColor, RETICLE_LINE_WIDTH * RETICLE_OUTLINE_WIDTH_FACTOR);
    m_reticleOutlinePen.setCosmetic(true);
    m_reticleOutlinePen.setJoinStyle(Qt::RoundJoin);
    m_reticleOutlinePen.setCapStyle(Qt::RoundCap);
}

OutlinedTextItem* OsdRenderer::createTextItem(const QPointF &pos, qreal zValue)
{
    OutlinedTextItem *item = new OutlinedTextItem();
    item->setFont(m_osdFont);
    item->setOutlinePen(m_textOutlinePen);
    item->setFillBrush(m_fillBrush);
    item->setPos(pos);
    item->setZValue(zValue);
    m_scene.addItem(item);
    return item;
}

// Helper to add both the main reticle path and its outline
void OsdRenderer::addReticlePathWithOutline(const QPainterPath& path) {
    if (!m_reticleRootGroup) {
        qCritical() << "addReticlePathWithOutline: m_reticleRootGroup is NULL. Cannot add items.";
        return;
    }

    // Create items WITHOUT adding them to the scene directly first
    QGraphicsPathItem* outlineItem = new QGraphicsPathItem(path); // Path should be relative to (0,0)
    outlineItem->setPen(m_reticleOutlinePen);

    QGraphicsPathItem* mainItem = new QGraphicsPathItem(path);
    mainItem->setPen(m_mainPen);

    // Add to group first - this also adds them to the scene if the group is in the scene
    m_reticleRootGroup->addToGroup(outlineItem);
    m_reticleRootGroup->addToGroup(mainItem);

    // CRITICAL FIX: Set position to (0,0) AFTER adding to group
    // addToGroup() preserves scene position, so we override it afterwards
    outlineItem->setPos(0, 0);
    mainItem->setPos(0, 0);

    // Track them if you need to iterate over them specifically later,
    // but qDeleteAll(m_reticleRootGroup->childItems()) is good for clearing.
    m_currentReticleDrawingItems.push_back(outlineItem);
    m_currentReticleDrawingItems.push_back(mainItem);

    // qDebug() << "Added path items to group. Group now has:" << m_reticleRootGroup->childItems().count() << "children.";
}

void OsdRenderer::addReticleShapeWithOutline(const QPainterPath& path)
{
    if (!m_reticleRootGroup) {
        qCritical() << "addReticleShapeWithOutline: m_reticleRootGroup is NULL. Cannot add items.";
        return;
    }

    QGraphicsPathItem* outlineItem = new QGraphicsPathItem(path);
    outlineItem->setPen(m_reticleOutlinePen);
    outlineItem->setBrush(Qt::NoBrush);

    QGraphicsPathItem* mainItem = new QGraphicsPathItem(path);
    mainItem->setPen(m_mainPen);
    mainItem->setBrush(m_fillBrush);

    m_reticleRootGroup->addToGroup(outlineItem);
    m_reticleRootGroup->addToGroup(mainItem);

    // CRITICAL FIX:
    outlineItem->setPos(0, 0);
    mainItem->setPos(0, 0);

    m_currentReticleDrawingItems.push_back(outlineItem);
    m_currentReticleDrawingItems.push_back(mainItem);
}

// for debugging:
void OsdRenderer::debugReticlePositions() {
    if (!m_reticleRootGroup) {
        qDebug() << "Debug: m_reticleRootGroup is null.";
        return;
    }

    qDebug() << "Debug: m_reticleRootGroup scenePos():" << m_reticleRootGroup->scenePos();
    qDebug() << "Debug: m_reticleRootGroup pos() (relative to parent, which is scene root):" << m_reticleRootGroup->pos();
    qDebug() << "Debug: m_reticleRootGroup childItems count:" << m_reticleRootGroup->childItems().count();

    int i = 0;
    for (QGraphicsItem* childItem : m_reticleRootGroup->childItems()) {
        // QGraphicsPathItem* pathItem = qgraphicsitem_cast<QGraphicsPathItem*>(childItem);
        // if (pathItem) {
        qDebug() << "  Child" << i << "type:" << childItem->type();
        qDebug() << "    - pos() (relative to m_reticleRootGroup):" << childItem->pos();
        qDebug() << "    - scenePos():" << childItem->scenePos();
        qDebug() << "    - boundingRect() (in child's coords):" << childItem->boundingRect();
        qDebug() << "    - sceneBoundingRect():" << childItem->sceneBoundingRect();
        // }
        i++;
    }
}


// Helper to add tick marks (used for Azimuth and Elevation)
void OsdRenderer::addTickMarks(
    QGraphicsScene& scene, const QPointF& center, qreal radius,
    int startDeg, int endDeg, int stepDeg,
    qreal majorTickLen, qreal minorTickLen,
    const QPen& mainPen, const QPen& outlinePen,
    qreal zMain, qreal zOutline,
    std::vector<QGraphicsLineItem*>& mainTicks, std::vector<QGraphicsLineItem*>& outlineTicks,
    bool isAzimuth)
{
    // Clear existing ticks first
    for(auto item : mainTicks) { scene.removeItem(item); delete item; } mainTicks.clear();
    for(auto item : outlineTicks) { scene.removeItem(item); delete item; } outlineTicks.clear();

    for (int deg = startDeg; deg < endDeg; deg += stepDeg) {
        bool isMajor = (isAzimuth && (deg % 90 == 0)) || (!isAzimuth && (deg == 60 || deg == 30 || deg == 0 || deg == -20)); // Example major logic
        qreal currentTickLength = isMajor ? majorTickLen : minorTickLen;
        qreal outerRad = radius;
        qreal innerRad = radius - currentTickLength;

        QPointF startPt, endPt;
        if (isAzimuth) {
            double angleRad = (90.0 - deg) * M_PI / 180.0; // Convert degrees to radians for trig functions
            startPt = QPointF(center.x() + innerRad * cos(angleRad), center.y() - innerRad * sin(angleRad));
            endPt = QPointF(center.x() + outerRad * cos(angleRad), center.y() - outerRad * sin(angleRad));
        } else { // Elevation (Vertical ticks)
            qreal norm = (static_cast<qreal>(deg) - EL_MIN) / EL_RANGE;
            qreal yPos = center.y() - norm * radius; // Here radius acts as scaleHeight, center.y as base
            startPt = QPointF(center.x() + currentTickLength, yPos); // Ticks to the right
            endPt = QPointF(center.x(), yPos);
        }

        // Add Outline Tick
        QGraphicsLineItem* tickOutline = scene.addLine(QLineF(startPt, endPt), outlinePen);
        tickOutline->setZValue(zOutline);
        outlineTicks.push_back(tickOutline);

        // Add Main Tick
        QGraphicsLineItem* tickMain = scene.addLine(QLineF(startPt, endPt), mainPen);
        tickMain->setZValue(zMain);
        mainTicks.push_back(tickMain);
    }
}

// Helper to add cardinal direction labels (N, E, S, W)
void OsdRenderer::addCardinalLabels(const QPointF& center, qreal radius, qreal labelOffset)
{
    // Clear existing labels
    for(auto item : m_azimuthLabels) { m_scene.removeItem(item); delete item; } m_azimuthLabels.clear();

    const std::map<int, QString> labels = {{0, "N"}, {90, "E"}, {180, "S"}, {270, "W"}};
    for (const auto& pair : labels) {
        int deg = pair.first;
        const QString& labelText = pair.second;

        double angleRad = (90.0 - deg) * M_PI / 180.0;
        qreal labelRad = radius + labelOffset;
        qreal labelX = center.x() + labelRad * cos(angleRad);
        qreal labelY = center.y() - labelRad * sin(angleRad) + 25;

        OutlinedTextItem *lbl = createTextItem(QPointF(0, 0), Z_ORDER_MAIN); // Create with helper
        lbl->setText(labelText);
        // Center the label on its calculated position
        lbl->setPos(labelX - lbl->boundingRect().width() / 2.0, labelY - lbl->boundingRect().height() / 2.0);
        m_azimuthLabels.push_back(lbl);
    }
}

// Helper to add elevation scale labels
void OsdRenderer::addElevationLabels(qreal scaleX, qreal scaleYBase, qreal scaleHeight, qreal elMin, qreal elRange)
{
    // Clear existing labels
    for(auto item : m_elevationLabels) { m_scene.removeItem(item); delete item; } m_elevationLabels.clear();

    const std::vector<qreal> degrees = {60.0, 0.0, -20.0};
    for (qreal degree : degrees) {
        qreal norm = (degree - elMin) / elRange;
        qreal yPos = scaleYBase - norm * scaleHeight + 15;
        QString labelText = QString::number(static_cast<int>(degree));

        OutlinedTextItem *lbl = createTextItem(QPointF(0, 0), Z_ORDER_MAIN);
        lbl->setText(labelText);
        // Position label to the right of the tick
        lbl->setPos(scaleX + EL_LABEL_X_OFFSET, yPos - lbl->boundingRect().height() / 2.0);
        m_elevationLabels.push_back(lbl);
    }
}

void OsdRenderer::initializeScene()
{
    // --- Text Items ---
    m_modeTextItem = createTextItem(POS_MODE_TEXT, Z_ORDER_MAIN);
    m_motionTextItem = createTextItem(POS_MOTION_TEXT, Z_ORDER_MAIN);
    m_speedTextItem = createTextItem(POS_SPEED_TEXT, Z_ORDER_MAIN);
    m_stabTextItem = createTextItem(POS_STAB_TEXT, Z_ORDER_MAIN);
    m_cameraTextItem = createTextItem(POS_CAMERA_TEXT, Z_ORDER_MAIN);
    m_fovTextItem = createTextItem(POS_FOV_TEXT, Z_ORDER_MAIN);
    m_zoomTextItem = createTextItem(POS_ZOOM_TEXT, Z_ORDER_MAIN);
    m_statusTextItem = createTextItem(POS_STATUS_TEXT, Z_ORDER_MAIN);
    m_rateTextItem = createTextItem(POS_RATE_TEXT, Z_ORDER_MAIN);
    m_lrfTextItem = createTextItem(POS_LRF_TEXT, Z_ORDER_MAIN);
    m_azTextItem = createTextItem(QPointF(0, 0), Z_ORDER_MAIN); // Position updated in updateAzimuth
    m_elValueTextItem = createTextItem(QPointF(0, 0), Z_ORDER_MAIN); // Position updated in updateElevation
    m_zeroingDisplayItem = createTextItem(POS_ZEROING_STATUS_TEXT, Z_ORDER_MAIN);
    m_zeroingDisplayItem->setText("Z: N/A"); // Initial placeholder
    m_zeroingDisplayItem->setVisible(false); // Hide initially

    m_windageDisplayItem = createTextItem(POS_WINDAGE_STATUS_TEXT, Z_ORDER_MAIN);
    m_windageDisplayItem->setText("W: N/A"); // Initial placeholder
    m_windageDisplayItem->setVisible(false); // Hide initially

    m_zoneWarningItem = createTextItem(POS_ZONE_WARNING_TEXT, Z_ORDER_MAIN + 5); // High Z-order
    m_zoneWarningItem->setBrush(QBrush(QColor(200,20,40))); // Make warnings stand out
    // m_zoneWarningItem->setOutlinePen(...); // Ensure good contrast for outline
    m_zoneWarningItem->setText(""); // Initially empty
    m_zoneWarningItem->setVisible(false);
    // Center the text item
    // m_zoneWarningItem->setPos(POS_ZONE_WARNING_TEXT.x() - m_zoneWarningItem->boundingRect().width() / 2.0, POS_ZONE_WARNING_TEXT.y());*
    m_leadAngleStatusTextItem = createTextItem(POS_ZONE_LAC_TEXT, Z_ORDER_MAIN + 5); // High Z-order
    m_leadAngleStatusTextItem->setText(""); // Initially empty
    m_leadAngleStatusTextItem->setVisible(false);

    m_currentScanNameTextItem = createTextItem(POS_SCAN_NAME_TEXT, Z_ORDER_MAIN);
    m_currentScanNameTextItem->setText(""); // Initially empty
    m_currentScanNameTextItem->setVisible(false);


        // --- Create Fixed LOB Marker ---
    QPainterPath lobPath;
    qreal lob_cross_size = 5.0; // Small, simple cross
    lobPath.moveTo(-lob_cross_size, 0); lobPath.lineTo(lob_cross_size, 0);
    lobPath.moveTo(0, -lob_cross_size); lobPath.lineTo(0, lob_cross_size);

    // Outline for LOB marker (optional, for better visibility)
    if (m_fixedLobMarkerOutlineItem) { m_scene.removeItem(m_fixedLobMarkerOutlineItem); delete m_fixedLobMarkerOutlineItem; }
    m_fixedLobMarkerOutlineItem = new QGraphicsPathItem(lobPath);
    // Use a distinct outline pen, perhaps thinner or a standard outline color
    QPen lobOutlinePen = m_reticleOutlinePen; // Or a specific pen for LOB marker outline
    //lobOutlinePen.setWidthF(lobOutlinePen.widthF() > 1 ? lobOutlinePen.widthF() -1 : 1); // Slightly thinner maybe
    m_fixedLobMarkerOutlineItem->setPen(lobOutlinePen);
    m_fixedLobMarkerOutlineItem->setPos(m_width / 2.0, m_height / 2.0); // Position at screen center
    m_fixedLobMarkerOutlineItem->setZValue(Z_ORDER_MAIN - 1); // Just behind main OSD elements, but above background
    m_scene.addItem(m_fixedLobMarkerOutlineItem);

    // Main LOB marker
    if (m_fixedLobMarkerItem) { m_scene.removeItem(m_fixedLobMarkerItem); delete m_fixedLobMarkerItem; }
    m_fixedLobMarkerItem = new QGraphicsPathItem(lobPath);
    // Use a distinct but unobtrusive color for the LOB marker itself
    // It should not compete with the main aiming reticle.
    //QPen lobMainPen(m_osdColor);//.lighter(120), 1.0); // Lighter shade of OSD color, thin
    //lobMainPen.setCosmetic(true);

    m_fixedLobMarkerItem->setPen(m_mainPen);
    m_fixedLobMarkerItem->setPos(m_width / 2.0, m_height / 2.0); // Position at screen center
    m_fixedLobMarkerItem->setZValue(Z_ORDER_MAIN); // Same level as other main OSD elements
    m_scene.addItem(m_fixedLobMarkerItem);


    // --- Azimuth Indicator ---
    qreal azIndicatorX = m_width - AZ_INDICATOR_X_OFFSET;
    QPointF azCenter(azIndicatorX, AZ_INDICATOR_Y);

    // Outline Circle
    m_azimuthCircleOutline = m_scene.addEllipse(azCenter.x() - AZ_RADIUS, azCenter.y() - AZ_RADIUS, AZ_RADIUS * 2, AZ_RADIUS * 2, m_shapeOutlinePen, Qt::NoBrush);
    m_azimuthCircleOutline->setZValue(Z_ORDER_OUTLINE);
    // Main Circle
    m_azimuthCircle = m_scene.addEllipse(azCenter.x() - AZ_RADIUS, azCenter.y() - AZ_RADIUS, AZ_RADIUS * 2, AZ_RADIUS * 2, m_mainPen, Qt::NoBrush);
    m_azimuthCircle->setZValue(Z_ORDER_MAIN);

    // Ticks
    addTickMarks(m_scene, azCenter, AZ_RADIUS, 0, 360, AZ_TICK_STEP,
                 AZ_TICK_LENGTH_MAJOR, AZ_TICK_LENGTH_MINOR,
                 m_tickMarkMainPen, m_tickMarkOutlinePen,
                 Z_ORDER_MAIN, Z_ORDER_OUTLINE,
                 m_azimuthTicks, m_azimuthTicksOutline, true);

    // Cardinal Labels
    addCardinalLabels(azCenter, AZ_RADIUS, AZ_LABEL_OFFSET);

    // Needle (Initial position pointing North)
    QLineF needleLine(azCenter, QPointF(azCenter.x(), azCenter.y() - AZ_RADIUS * AZ_NEEDLE_LENGTH_FACTOR));
    m_azimuthNeedleOutline = m_scene.addLine(needleLine, m_needleOutlinePen);
    m_azimuthNeedleOutline->setZValue(Z_ORDER_OUTLINE);
    m_azimuthNeedle = m_scene.addLine(needleLine, m_mainPen);
    m_azimuthNeedle->setZValue(Z_ORDER_MAIN);
    // Set transform origin for rotation
    m_azimuthNeedleOutline->setTransformOriginPoint(azCenter);
    m_azimuthNeedle->setTransformOriginPoint(azCenter);

    // --- Elevation Scale ---
    qreal elScaleX = m_width - EL_SCALE_X_OFFSET;
    qreal elScaleYBase = m_height - EL_SCALE_Y_OFFSET;
    qreal elScaleYTop = elScaleYBase - EL_SCALE_HEIGHT;
    QPointF elScaleTopPt(elScaleX, elScaleYTop);
    QPointF elScaleBasePt(elScaleX, elScaleYBase);
    QPointF elScaleCenterForTicks(elScaleX, elScaleYBase); // Use base as reference for tick calculation

    // Main Vertical Scale Line
    m_elevationScaleOutline = m_scene.addLine(QLineF(elScaleTopPt, elScaleBasePt), m_shapeOutlinePen);
    m_elevationScaleOutline->setZValue(Z_ORDER_OUTLINE);
    m_elevationScale = m_scene.addLine(QLineF(elScaleTopPt, elScaleBasePt), m_mainPen);
    m_elevationScale->setZValue(Z_ORDER_MAIN);

    // Ticks (Note: Using scaleHeight as 'radius' for addTickMarks)
    addTickMarks(m_scene, elScaleCenterForTicks, EL_SCALE_HEIGHT, static_cast<int>(EL_MIN), static_cast<int>(EL_MIN + EL_RANGE + 1), 10, // Tick every 10 degrees
                 EL_MAJOR_TICK_LENGTH, EL_TICK_LENGTH  , // Same length for major/minor here
                 m_tickMarkMainPen, m_tickMarkOutlinePen,
                 Z_ORDER_MAIN, Z_ORDER_OUTLINE,
                 m_elevationTicks, m_elevationTicksOutline, false);

    // Labels
    addElevationLabels(elScaleX, elScaleYBase, EL_SCALE_HEIGHT, EL_MIN, EL_RANGE);

    // Elevation Indicator Triangle
    QPainterPath trianglePath;
    qreal indicatorY = elScaleYBase  ; // Initial position at 0 degrees
    qreal indicatorX = elScaleX - 12;
    trianglePath.moveTo(indicatorX + EL_INDICATOR_WIDTH, indicatorY); // Point left
    trianglePath.lineTo(indicatorX, indicatorY - EL_INDICATOR_HEIGHT / 2.0);
    trianglePath.lineTo(indicatorX, indicatorY + EL_INDICATOR_HEIGHT / 2.0);
    trianglePath.closeSubpath();

    m_elevationIndicatorOutline = m_scene.addPath(trianglePath, m_shapeOutlinePen, Qt::NoBrush);
    m_elevationIndicatorOutline->setZValue(Z_ORDER_OUTLINE);
    QPen triMainPen(m_osdColor, 1.0); // Thin border for filled shape
    triMainPen.setCosmetic(true);
    m_elevationIndicator = m_scene.addPath(trianglePath, triMainPen, m_fillBrush);
    m_elevationIndicator->setZValue(Z_ORDER_MAIN);

    // --- Tracking Box / Corners ---
    // Hidden reference box
    m_trackingBox = m_scene.addRect(0, 0, 0, 0, Qt::NoPen); // No pen needed, it's invisible
    m_trackingBox->setVisible(false);
    m_trackingBox->setZValue(Z_ORDER_TRACKING); // High Z-order

    // Create vectors for corners (size 8)
    m_trackingCorners.resize(8);
    m_trackingCornersOutline.resize(8);

    // Create Outline Corners (Black/Dark, thicker, behind)
    for (int i = 0; i < 8; ++i) {
        m_trackingCornersOutline[i] = m_scene.addLine(0, 0, 0, 0, m_trackingOutlinePen);
        m_trackingCornersOutline[i]->setVisible(false);
        m_trackingCornersOutline[i]->setZValue(Z_ORDER_OUTLINE); // Behind main corners
    }
    // Create Main Corners (Colored, standard width, on top)
    QPen initialTrackingPen(COLOR_TRACKING_DEFAULT, m_lineWidth);
    initialTrackingPen.setCosmetic(true);
    for (int i = 0; i < 8; ++i) {
        m_trackingCorners[i] = m_scene.addLine(0, 0, 0, 0, initialTrackingPen);
        m_trackingCorners[i]->setVisible(false);
        m_trackingCorners[i]->setZValue(Z_ORDER_MAIN); // Above outline
    }

    if (!m_reticleRootGroup) { // Ensure it's created only once
        m_reticleRootGroup = new QGraphicsItemGroup();
        m_scene.addItem(m_reticleRootGroup); // Add to scene
        m_reticleRootGroup->setPos(m_width / 2.0, m_height / 2.0); // Center it
        qDebug() << "m_reticleRootGroup CREATED and centered at" << m_reticleRootGroup->pos();
    } else {
        qDebug() << "m_reticleRootGroup already exists, pos:" << m_reticleRootGroup->pos();
    }
    // ...
    //m_currentHfov = model->data().cameraFOV; // Get initial FOV
    // --- Initial Updates --- (Call public slots to ensure consistency)
    updateMode(m_currentMode);
    updateMotionMode(m_motionMode);
    updateStabilization(m_stabEnabled);
    updateCameraType(m_cameraType);
    updateLrfDistance(m_lrfDistance);
    updateSystemStatus(m_sysCharged, m_sysArmed, m_sysReady);
    updateFiringMode(m_fireMode);
    updateFov(m_fov);
    updateSpeed(m_speed);
    updateAzimuth(m_azimuth);
    updateElevation(m_elevation);
    updateTrackingState(m_trackingState); // Updates corner color
    updateTrackingBox(0, 0, 0, 0); // Updates corner positions & visibility
    updateDetectionBoxes({}); // Clear any initial detections
}

// --- Update Functions for Specific OSD Elements ---

void OsdRenderer::updateStatusText() {
    if (!m_statusTextItem || !m_rateTextItem) return;

    QString status = QString("SYS: %1 %2 %3")
                         .arg(m_sysCharged ? "CHG" : "---")
                         .arg(m_sysArmed ? "ARM" : "SAF")
                         .arg(m_sysReady ? "RDY" : "NRD");
    m_statusTextItem->setText(status);

    QString rate;
    switch (m_fireMode) {
    case FireMode::SingleShot: rate = "RATE: SINGLE SHOT"; break;
    case FireMode::ShortBurst: rate = "RATE: SHORT BURST"; break;
    case FireMode::LongBurst: rate = "RATE: LONG BURST"; break;
    default: rate = "RATE: UNKNOWN"; break;
    }
    m_rateTextItem->setText(rate);
}

void OsdRenderer::updateAzimuthIndicator() {
    if (!m_azimuthNeedle || !m_azimuthNeedleOutline || !m_azTextItem) return;

    // Rotate needle (0 degrees = North/Up)
    qreal rotation = m_azimuth;
    m_azimuthNeedle->setRotation(rotation);
    m_azimuthNeedleOutline->setRotation(rotation);

    // Update text
    m_azTextItem->setText(QString::number(m_azimuth, 'f', 1) + QChar(0xB0)); // Degree symbol
    // Center text below the indicator
    qreal azIndicatorX = m_width - AZ_INDICATOR_X_OFFSET;
    qreal azIndicatorY = AZ_INDICATOR_Y;
    qreal textX = azIndicatorX - m_azTextItem->boundingRect().width() / 2.0;
    qreal textY = azIndicatorY + AZ_RADIUS * AZ_TEXT_Y_OFFSET_FACTOR + AZ_TEXT_Y_EXTRA_OFFSET;
    m_azTextItem->setPos(textX, textY);
}

void OsdRenderer::updateElevationScale() {
    if (!m_elevationIndicator || !m_elevationIndicatorOutline || !m_elValueTextItem) return;

    // Calculate indicator position
    qreal elScaleYBase = m_height - EL_SCALE_Y_OFFSET;
    qreal elScaleHeight = EL_SCALE_HEIGHT;
    qreal normElevation = (m_elevation - EL_MIN) / EL_RANGE;
    // Clamp normalized value between 0 and 1
    normElevation = std::max(0.0, std::min(1.0, static_cast<double>(normElevation)));
    qreal indicatorY = elScaleYBase - normElevation * elScaleHeight;

    // Update indicator position (move the existing path items)
    //qreal currentIndicatorY = m_elevationIndicator->pos().y(); // Assuming initial Y pos is 0
    //qreal dy = indicatorY - (elScaleYBase - (0.0 - EL_MIN) / EL_RANGE * elScaleHeight); // Calculate delta from initial 0-degree position

    // Get the original path definition point (center of the triangle base Y)
    qreal initialY = elScaleYBase - (0.0 - EL_MIN) / EL_RANGE * elScaleHeight;
    qreal newYPos = indicatorY;

    // We need to move the item itself, not just change its internal path Y coordinates easily.
    // The easiest is to set the item's Y position relative to its initial creation position.
    m_elevationIndicator->setY(newYPos - elScaleYBase);
    m_elevationIndicatorOutline->setY(newYPos - elScaleYBase);


    // Update text value
    m_elValueTextItem->setText(QString::number(m_elevation, 'f', 1) + QChar(0xB0));
    // Position text next to the indicator
    qreal elScaleX = m_width - EL_SCALE_X_OFFSET;
    qreal textX = elScaleX - EL_INDICATOR_WIDTH - m_elValueTextItem->boundingRect().width() - 5; // Position left of indicator
    qreal textY = elScaleYBase - (m_elevation - EL_MIN) / EL_RANGE * elScaleHeight;
    m_elValueTextItem->setPos(textX, textY);
}

void OsdRenderer::updateTrackingCorners(float x, float y, float width, float height) {
    bool visible = (width > 0 && height > 0);

    if (!visible) {
        for (auto corner : m_trackingCorners) if (corner) corner->setVisible(false);
        for (auto outline : m_trackingCornersOutline) if (outline) outline->setVisible(false);
        return;
    }

    // Calculate corner points
    QPointF tl(x, y);
    QPointF tr(x + width, y);
    QPointF bl(x, y + height);
    QPointF br(x + width, y + height);
    qreal len = TRACKING_CORNER_LENGTH;

    // Define the 8 lines for the corners
    QLineF lines[8] = {
        QLineF(tl, tl + QPointF(len, 0)), // Top-left H
        QLineF(tl, tl + QPointF(0, len)), // Top-left V
        QLineF(tr, tr + QPointF(-len, 0)),// Top-right H
        QLineF(tr, tr + QPointF(0, len)), // Top-right V
        QLineF(bl, bl + QPointF(len, 0)), // Bottom-left H
        QLineF(bl, bl + QPointF(0, -len)),// Bottom-left V
        QLineF(br, br + QPointF(-len, 0)),// Bottom-right H
        QLineF(br, br + QPointF(0, -len)) // Bottom-right V
    };

    // Update the lines
    for (int i = 0; i < 8; ++i) {
        if (m_trackingCorners[i]) {
            m_trackingCorners[i]->setLine(lines[i]);
            m_trackingCorners[i]->setVisible(true);
        }
        if (m_trackingCornersOutline[i]) {
            m_trackingCornersOutline[i]->setLine(lines[i]);
            m_trackingCornersOutline[i]->setVisible(true);
        }
    }
}

void OsdRenderer::clearReticleDrawingItems() {
    if (!m_reticleRootGroup) {
        qDebug() << "clearReticleDrawingItems: m_reticleRootGroup is null, nothing to clear.";
        return;
    }

    // Get children directly from the group and delete them
    QList<QGraphicsItem*> children = m_reticleRootGroup->childItems();
    if (!children.isEmpty()) {
        qDebug() << "clearReticleDrawingItems: Clearing" << children.count() << "children from m_reticleRootGroup.";
        for (QGraphicsItem* item : children) {
            // It's important that removeFromGroup is called before delete if the group
            // doesn't automatically handle this on child deletion.
            // However, deleting a child QGraphicsItem that has a parent should
            // also notify the parent to remove it.
            // qDeleteAll is often the simplest for children of a QGraphicsItem.
        }
        qDeleteAll(children); // This removes them from the group and deletes them
    } else {
        qDebug() << "clearReticleDrawingItems: m_reticleRootGroup had no children to clear.";
    }
    m_currentReticleDrawingItems.clear(); // Also clear your tracking vector
}

void OsdRenderer::clearDetectionGraphics() {
    for (auto item : m_detectionRectItems) { m_scene.removeItem(item); delete item; }
    m_detectionRectItems.clear();
    for (auto item : m_detectionTextItems) { m_scene.removeItem(item); delete item; }
    m_detectionTextItems.clear();
    for (auto item : m_detectionRectOutlines) { m_scene.removeItem(item); delete item; }
    m_detectionRectOutlines.clear();
}

void OsdRenderer::drawDetectionBox(const YoloDetection& detection) {
    QRectF boxRect(detection.box.x, detection.box.y, detection.box.width, detection.box.height);

    // Outline
    QGraphicsRectItem* outline = m_scene.addRect(boxRect, m_shapeOutlinePen);
    outline->setZValue(Z_ORDER_OUTLINE);
    m_detectionRectOutlines.push_back(outline);

    // Main Box (using a distinct color, maybe based on class or confidence? Using main OSD color for now)
    QPen detectionPen(m_osdColor, 2); // Thinner pen for detection boxes
    detectionPen.setCosmetic(true);
    QGraphicsRectItem* mainBox = m_scene.addRect(boxRect, detectionPen);
    mainBox->setZValue(Z_ORDER_DETECTION);
    m_detectionRectItems.push_back(mainBox);

    // Text Label (Class + Confidence)
    QString label = QString("%1 %2%")
                        .arg(QString::fromStdString(detection.className))
                        .arg(static_cast<int>(detection.confidence * 100));
    OutlinedTextItem* text = createTextItem(QPointF(0,0), Z_ORDER_DETECTION + 1); // Above box
    text->setText(label);
    // Position above the box
    //text->setPos(boxRect.left(), boxRect.top() - text->boundingRect().height() + DETECTION_TEXT_OFFSET_Y);
    text->setPos(boxRect.left(), boxRect.top()+ DETECTION_TEXT_OFFSET_Y);
    m_detectionTextItems.push_back(text);
}

void OsdRenderer::createBasicReticle() { // Assuming pens are member variables updated by setupPensAndBrushes
    // clearReticleDrawingItems(); // This should be called by updateReticleType before calling this

    qreal size = BASIC_RETICLE_SIZE; // Define this constant (e.g., 20.0)

    QPainterPath path;
    // Paths are defined relative to (0,0) which is the center of m_reticleRootGroup
    path.moveTo(-size, 0);
    path.lineTo(size, 0);
    path.moveTo(0, -size);
    path.lineTo(0, size);

    // Use member pens setup by setupPensAndBrushes()
    // Assuming m_mainPen is for reticle lines and m_reticleOutlinePen for their outline
    addReticlePathWithOutline(path);
}

void OsdRenderer::createBoxCrosshairReticle() {
    // clearReticleDrawingItems(); // This is called by the calling function: updateReticleType

    // All coordinates are relative to the center of the reticle group, which is (0,0)
    qreal lineLen = BOX_CROSSHAIR_LINE_LEN;
    qreal boxSize = BOX_CROSSHAIR_BOX_SIZE;
    qreal halfBox = boxSize / 2.0;
    qreal gap = BOX_CROSSHAIR_GAP;

    // --- Cross lines with gap ---
    QPainterPath linesPath;
    // Horizontal line parts
    linesPath.moveTo(-lineLen, 0);              // Leftmost point of left segment
    linesPath.lineTo(-halfBox - gap, 0);        // To the left of the box gap

    linesPath.moveTo(halfBox + gap, 0);         // From the right of the box gap
    linesPath.lineTo(lineLen, 0);               // To the rightmost point of right segment

    // Vertical line parts
    linesPath.moveTo(0, -lineLen);              // Topmost point of top segment
    linesPath.lineTo(0, -halfBox - gap);        // To the top of the box gap

    linesPath.moveTo(0, halfBox + gap);         // From the bottom of the box gap
    linesPath.lineTo(0, lineLen);               // To the bottommost point of bottom segment

    // Add the lines path with its outline
    addReticlePathWithOutline(linesPath);


    // --- Box ---
    QPainterPath boxPath;
    // Rectangle centered at (0,0)
    boxPath.addRect(-halfBox, -halfBox, boxSize, boxSize);

    // Add the box path with its outline
    addReticlePathWithOutline(boxPath);
}

void OsdRenderer::createStandardCrosshairReticle() {
    // clearReticleDrawingItems(); // Called by updateReticleType
    qreal size = STD_CROSSHAIR_SIZE;
    qreal gap = STD_CROSSHAIR_GAP;

    QPainterPath path;
    // Define path around (0,0)
    path.moveTo(-size, 0); path.lineTo(-gap, 0);
    path.moveTo(gap, 0); path.lineTo(size, 0);
    path.moveTo(0, -size); path.lineTo(0, -gap);
    path.moveTo(0, gap); path.lineTo(0, size);

    addReticlePathWithOutline(path);
}

void OsdRenderer::createPrecisionCrosshairReticle() {
    // clearReticleDrawingItems(); // Called by updateReticleType

    qreal size = PRECISION_CROSSHAIR_SIZE;
    qreal dotRadius = PRECISION_CROSSHAIR_CENTER_DOT_RADIUS;
    qreal tickLen = PRECISION_CROSSHAIR_TICK_LENGTH;
    int numTicks = PRECISION_CROSSHAIR_NUM_TICKS;
    qreal tickSpacing = PRECISION_CROSSHAIR_TICK_SPACING;

    QPainterPath path;
    // Center dot (relative to group's 0,0)
    path.addEllipse(0 - dotRadius, 0 - dotRadius, dotRadius * 2, dotRadius * 2);

    // Main lines (relative to group's 0,0)
    path.moveTo(-size, 0); path.lineTo(size, 0);
    path.moveTo(0, -size); path.lineTo(0, size);

    // Ticks (relative to group's 0,0)
    for (int i = 1; i <= numTicks; ++i) {
        qreal dist = i * tickSpacing;
        // Horizontal ticks
        path.moveTo(-dist, -tickLen); path.lineTo(-dist, tickLen);
        path.moveTo( dist, -tickLen); path.lineTo( dist, tickLen);
        // Vertical ticks
        path.moveTo(-tickLen, -dist); path.lineTo(tickLen, -dist);
        path.moveTo(-tickLen,  dist); path.lineTo(tickLen,  dist);
    }
    addReticlePathWithOutline(path);
}

double OsdRenderer::calculatePixelsPerMil(double horizontalFovDegrees, double screenWidthPixels) {
    if (horizontalFovDegrees <= 0 || screenWidthPixels <= 0) {
        return 0.0; // Avoid division by zero or invalid input
    }
    // Convert FOV from degrees to radians
    double horizontalFovRadians = horizontalFovDegrees * M_PI / 180.0;
    // Calculate the horizontal size visible at 1000 units distance
    double visibleWidthAt1000 = 2.0 * 1000.0 * tan(horizontalFovRadians / 2.0);
    // Calculate how many mils fit into this visible width (1 mil subtends 1 unit at 1000 units)
    double milsAcrossScreen = visibleWidthAt1000;
    // Calculate pixels per mil
    return screenWidthPixels / milsAcrossScreen;
}

void OsdRenderer::createMilDotReticle() { // Assuming pens are member variables
    // clearReticleDrawingItems(); // Called by updateReticleType

    qreal lineSize = MILDOT_RETICLE_SIZE;     // e.g., 150.0
    qreal dotRadius = MILDOT_RETICLE_DOT_RADIUS; // e.g., 1.5 (total diameter 3.0)
    int numDotsPerArm = MILDOT_RETICLE_NUM_DOTS; // e.g., 4

    double pixelsPerMil = calculatePixelsPerMil(m_currentHfov, m_width);
    if (pixelsPerMil <= 0.1) {
        qWarning() << "Cannot create MilDot reticle: Invalid FOV/width or pixelsPerMil too small (" << pixelsPerMil << "). Falling back.";
        createStandardCrosshairReticle(); // Fallback, ensure it also uses member pens
        return;
    }
    qreal dotSpacing = pixelsPerMil; // 1 Mil spacing

    // --- Main Cross Lines (relative to 0,0) ---
    QPainterPath linesPath;
    linesPath.moveTo(-lineSize, 0); linesPath.lineTo(lineSize, 0);
    linesPath.moveTo(0, -lineSize); linesPath.lineTo(0, lineSize);
    addReticlePathWithOutline(linesPath);

    // --- Dots ---
    // Pens/Brushes for dots
    QPen dotMainPen = m_mainPen; // Or Qt::NoPen if fillBrush defines the color entirely
    QBrush dotFillBrush = m_fillBrush; // This uses m_osdColor
    QPen dotOutlinePen = m_reticleOutlinePen;

    for (int i = 1; i <= numDotsPerArm; ++i) {
        qreal distFromCenter = i * dotSpacing;
        if (distFromCenter > lineSize + dotRadius) break; // Optional: Don't draw dots way past lines

        QPainterPath dotPath; // Path for one set of 4 dots at this mil distance

        // Horizontal dots (left and right of center)
        dotPath.addEllipse(-distFromCenter - dotRadius, -dotRadius, dotRadius * 2, dotRadius * 2); // Left
        dotPath.addEllipse( distFromCenter - dotRadius, -dotRadius, dotRadius * 2, dotRadius * 2); // Right

        // Vertical dots (top and bottom of center)
        dotPath.addEllipse(-dotRadius, -distFromCenter - dotRadius, dotRadius * 2, dotRadius * 2); // Top
        dotPath.addEllipse(-dotRadius,  distFromCenter - dotRadius, dotRadius * 2, dotRadius * 2); // Bottom

        // Add these 4 dots (as one path) with outline and fill
        addReticleShapeWithOutline(dotPath);
    }
}

// === Public Slots Implementation ===

void OsdRenderer::updateMode(OperationalMode mode) {
    if (m_currentMode == mode && m_modeTextItem && !m_modeTextItem->text().isEmpty()) return;
    m_currentMode = mode;
    if (!m_modeTextItem) return;
    QString modeStr;
    switch (mode) {
    case OperationalMode::Idle: modeStr = "MODE: IDLE"; break;
    case OperationalMode::Surveillance: modeStr = "MODE: OBS"; break;
    case OperationalMode::Tracking: modeStr = "MODE: TRACKING"; break;
 
    case OperationalMode::Engagement: modeStr = "MODE: ENGAGE"; break;
    case OperationalMode::EmergencyStop: modeStr = "MODE: EMERGENCY STOP"; break;
            //case OperationalMode::: modeStr = "MODE: EMERGENCY"; break;
    
    //case OperationalMode::eme: modeStr = "MODE: CALIB"; break;
    default: modeStr = "MODE: UNKNOWN"; break;
    }
    m_modeTextItem->setText(modeStr);

    // Make the text highly visible
    if (mode == OperationalMode::EmergencyStop) {
        m_modeTextItem->setFillBrush(Qt::red);
        // You could even make it blink using a QTimer in OsdRenderer
    } else {
        m_modeTextItem->setFillBrush(m_fillBrush); // Revert to normal color
    }
}

void OsdRenderer::updateMotionMode(MotionMode motionMode) {
    // Original code commented out the m_motionTextItem initialization.
    // If it's needed, uncomment the initialization and this update logic.

    if (m_motionMode == motionMode && m_motionTextItem && !m_motionTextItem->text().isEmpty()) return;
    m_motionMode = motionMode;
    if (!m_motionTextItem) return;
    QString motionStr;
    switch (motionMode) {
        case MotionMode::Manual: motionStr = "MOTION: MAN"; break;
        case MotionMode::AutoSectorScan: motionStr = "MOTION: SCAN"; break;
        case MotionMode::TRPScan: motionStr = "MOTION: TRP"; break;
        case MotionMode::ManualTrack: motionStr = "MOTION: TRACK"; break;
        case MotionMode::AutoTrack: motionStr = "MOTION: AUTO TRACK"; break;
        case MotionMode::RadarSlew: motionStr = "MOTION: RADAR"; break;
 
        default: motionStr = "MOTION: N/A"; break;
    }
    m_motionTextItem->setText(motionStr);

}

void OsdRenderer::updateStabilization(bool enabled) {
    if (m_stabEnabled == enabled && m_stabTextItem && !m_stabTextItem->text().isEmpty()) return;
    m_stabEnabled = enabled;
    if (!m_stabTextItem) return;
    m_stabTextItem->setText(enabled ? "STAB: ON" : "STAB: OFF");
}

void OsdRenderer::updateCameraType(const QString &cameraType) {
    if (m_cameraType == cameraType && m_cameraTextItem && !m_cameraTextItem->text().isEmpty()) return;
    m_cameraType = cameraType;
    if (!m_cameraTextItem) return;
    // Assuming cameraType is like "DAY", "IR", "FUSED"
    m_cameraTextItem->setText(QString("CAM: %1").arg(cameraType.toUpper()));
    // Potentially update m_zoomTextItem based on camera type if needed
}

void OsdRenderer::updateLrfDistance(float distance) {
    // Use a tolerance for float comparison or update always if frequent changes expected
    // if (qFuzzyCompare(m_lrfDistance, distance)) return;
    m_lrfDistance = distance;
    if (!m_lrfTextItem) return;
    QString distStr = (distance > 0.1f) ? QString::number(distance, 'f', 1) + " m" : "LRF: --- m";
    m_lrfTextItem->setText(distStr);
}

void OsdRenderer::updateSystemStatus(bool charged, bool armed, bool ready) {
    if (m_sysCharged == charged && m_sysArmed == armed && m_sysReady == ready && m_statusTextItem && !m_statusTextItem->text().isEmpty()) return;
    m_sysCharged = charged;
    m_sysArmed = armed;
    m_sysReady = ready;
    updateStatusText(); // Combined update logic
}

void OsdRenderer::updateFiringMode(FireMode rate) {
    if (m_fireMode == rate && m_rateTextItem && !m_rateTextItem->text().isEmpty()) return;
    m_fireMode = rate;
    updateStatusText(); // Combined update logic
}

void OsdRenderer::updateFov(float hfovDegrees) {
    if (!qFuzzyCompare(m_currentHfov, hfovDegrees)) {
        m_currentHfov = hfovDegrees;
        // If Zeroing or LAC is active, their pixel offsets depend on FOV, so recalculate and apply
        if (m_isZeroingCurrentlyApplied || m_isLacCurrentlyActive) {
            // Re-calculate pixel offsets based on new FOV
            if (m_isZeroingCurrentlyApplied) {
                QPointF zeroPx = convertAngularToPixelOffset(m_currentZeroingAzOffset, m_currentZeroingElOffset); // Assuming you store angular offsets
                m_zeroingOffsetXPx = zeroPx.x();
                m_zeroingOffsetYPx = zeroPx.y();
            }
            if (m_isLacCurrentlyActive) {
                 QPointF leadPx = convertAngularToPixelOffset(m_currentLeadAzOffsetDegrees, m_currentLeadElOffsetDegrees); // Need to store angular lead offsets
                m_leadOffsetXPx = leadPx.x();
                m_leadOffsetYPx = leadPx.y();
            }
            applyReticlePosition();
        }
        // Update FOV text item
        if (m_fovTextItem) m_fovTextItem->setText(QString("FOV: %1°").arg(m_currentHfov, 0, 'f', 1));

        // Mil-dot reticle needs recreation if FOV changes as dot spacing is FOV dependent
        if (m_reticleType == ReticleType::MilDot || m_reticleType == ReticleType::PrecisionCrosshair) {
             m_forceReticleRecreation = true; // Flag to force recreation on next updateReticleType
             updateReticleType(m_reticleType); // Recreate with new FOV
        }
    }
}

void OsdRenderer::updateSpeed(double speed) {
    m_speed = speed;
    if (!m_speedTextItem) return;
    m_speedTextItem->setText(QString("SPD: %1 %").arg(speed, 0, 'f', 1));
}

void OsdRenderer::updateAzimuth(float azimuth) {
    // Normalize azimuth to 0-360 range if necessary
    while (azimuth < 0.0f) azimuth += 360.0f;
    while (azimuth >= 360.0f) azimuth -= 360.0f;
    m_azimuth = azimuth;
    updateAzimuthIndicator();
}

void OsdRenderer::updateElevation(float elevation) {
    // Clamp elevation to visual range if necessary
    // elevation = std::max(EL_MIN, std::min(EL_MIN + EL_RANGE, elevation));
    m_elevation = elevation;
    updateElevationScale();
}

void OsdRenderer::updateTrackingState(VPITrackingState state) {
    if (m_trackingState == state) return;
    m_trackingState = state;

    QColor trackingColor;
    switch (state) {
    //case VPI_TRACKING_STATE_ACQUIRING: trackingColor = COLOR_TRACKING_ACQUIRING; break;
    case VPI_TRACKING_STATE_TRACKED:  trackingColor = COLOR_TRACKING_ACQUIRED; break;
    case VPI_TRACKING_STATE_LOST:      trackingColor = COLOR_TRACKING_LOST; break;
    default:                           trackingColor = COLOR_TRACKING_DEFAULT; break;
    }

    // Update the pen color for the main tracking corners
    QPen trackingPen(trackingColor, m_lineWidth);
    trackingPen.setCosmetic(true);
    for (QGraphicsLineItem* corner : m_trackingCorners) {
        if (corner) {
            corner->setPen(trackingPen);
        }
    }
}

void OsdRenderer::updateTrackingPhaseDisplay(
    TrackingPhase phase,
    bool hasValidLock,
    const QRectF& acquisitionBox,
    const QRectF& trackedBbox)
{
    // 1. Determine Visibility, Position, and Size of the box
    bool boxIsVisible = true;
    QRectF boxToDraw;

    switch (phase) {
    case TrackingPhase::Acquisition:
        // In acquisition phase, we always draw the user-sizable acquisition box
        boxToDraw = acquisitionBox;
        break;

    case TrackingPhase::Tracking_LockPending:
    case TrackingPhase::Tracking_ActiveLock:
    case TrackingPhase::Tracking_Coast:
    case TrackingPhase::Tracking_Firing:
        // For all active tracking phases, we draw the box reported by the tracker
        if (hasValidLock) {
            boxToDraw = trackedBbox;
        } else {
            // In Coast or a failed LockPending, we have no valid box from the tracker. Hide it.
            boxIsVisible = false;
        }
        break;

    case TrackingPhase::Off:
    default:
        boxIsVisible = false;
        break;
    }

    // 2. Determine Color and Style of the box based on the phase
    QColor boxColor = COLOR_TRACKING_DEFAULT;
    Qt::PenStyle boxStyle = Qt::SolidLine;

    switch (phase) {
    case TrackingPhase::Acquisition:
    case TrackingPhase::Tracking_LockPending:
        boxColor = COLOR_TRACKING_ACQUIRING; // Yellow
        boxStyle = Qt::SolidLine;
        break;
    case TrackingPhase::Tracking_ActiveLock:
        boxColor = COLOR_TRACKING_ACQUIRED;  // Red
        boxStyle = Qt::DashLine;
        break;
    case TrackingPhase::Tracking_Coast:
        boxColor = COLOR_TRACKING_LOST;      // Yellow
        boxStyle = Qt::DashLine;
        break;
    case TrackingPhase::Tracking_Firing:
        boxColor = COLOR_TRACKING_FIRING;    // Green
        boxStyle = Qt::DashLine;
        break;
    case TrackingPhase::Off:
    default:
        // Handled by boxIsVisible = false
        break;
    }


    // 3. Apply Style and Position to the Corner Graphics
    for (int i = 0; i < 8; ++i) { // Assuming 8 corner items
        if (m_trackingCorners[i]) m_trackingCorners[i]->setVisible(boxIsVisible);
        if (m_trackingCornersOutline[i]) m_trackingCornersOutline[i]->setVisible(boxIsVisible);
    }

    if (boxIsVisible) {
        // Update Pen Style
        QPen trackingPen(boxColor, m_lineWidth);
        trackingPen.setCosmetic(true);
        trackingPen.setStyle(boxStyle);
        for (QGraphicsLineItem* corner : m_trackingCorners) {
            if (corner) corner->setPen(trackingPen);
        }

        // Update Outline Pen Style
        QPen outlinePen = m_trackingOutlinePen; // Get base outline pen
        outlinePen.setStyle(boxStyle);
        for (QGraphicsLineItem* outline : m_trackingCornersOutline) {
            if (outline) outline->setPen(outlinePen);
        }

        // Update Position using the existing updateTrackingCorners logic
        updateTrackingCorners(boxToDraw.x(), boxToDraw.y(), boxToDraw.width(), boxToDraw.height());
    }
}

void OsdRenderer::updateTrackingBox(float x, float y, float width, float height) {
    // Update the hidden reference box (optional, but can be useful for debugging)
    if (m_trackingBox) {
        m_trackingBox->setRect(x, y, width, height);
        m_trackingBox->setVisible(width > 0 && height > 0);
    }
    // Update the visible corner lines
    updateTrackingCorners(x, y, width, height);
}

void OsdRenderer::updateDetectionBoxes(const std::vector<YoloDetection> &detections) {
    clearDetectionGraphics(); // Clear previous boxes
    for (const auto& det : detections) {
        drawDetectionBox(det);
    }
}
 void OsdRenderer::updateReticleType(ReticleType type) {
    // Check if recreation is truly needed (type changed OR forced e.g. by color/FOV change)
    if (m_reticleType == type && !m_currentReticleDrawingItems.empty() && !m_forceReticleRecreation) {  
        //qDebug() << "Reticle type" << static_cast<int>(type) << "is same and no forced recreation. Skipping.";
        return;
    }

    m_reticleType = type;
    m_forceReticleRecreation = false; // Reset flag after use

    qDebug() << "Reticle type updating to:" << static_cast<int>(type) << "Recreating drawing items.";

    clearReticleDrawingItems(); // Clear previous items from m_reticleRootGroup

    switch (type) {
        case ReticleType::Basic:            createBasicReticle(); break;
        case ReticleType::BoxCrosshair:     createBoxCrosshairReticle(); break;
        case ReticleType::StandardCrosshair:createStandardCrosshairReticle(); break;
        case ReticleType::PrecisionCrosshair:createPrecisionCrosshairReticle(); break;
        case ReticleType::MilDot:           createMilDotReticle(); break;
        default:
            qWarning() << "Unknown reticle type:" << static_cast<int>(type);
            createStandardCrosshairReticle(); // Fallback
            break;
    }

    applyReticlePosition(); // Apply current zeroing/lead offsets to the new reticle group's position
    m_view.update();        // Request viewport update
    debugReticlePositions(); // <<< CALL DEBUG METHOD

}

void OsdRenderer::updateColorStyle(QColor style) {
    if (m_osdColor == style) return; // No change

    qDebug() << "Updating OSD Color Style from" << m_osdColor.name() << "to" << style.name();
    m_osdColor = style;

    // Re-initialize all pens and brushes based on the new color
    setupPensAndBrushes();

    // --- Update Existing Graphics Items --- Apply new pens/brushes

    // Text Items (using the helper ensures correct pens/brushes are set initially)
    OutlinedTextItem* textItems[] = {
        m_modeTextItem, m_motionTextItem, m_speedTextItem, m_stabTextItem, m_cameraTextItem,
        m_fovTextItem, m_zoomTextItem, m_statusTextItem, m_rateTextItem,
        m_lrfTextItem, m_azTextItem, m_elValueTextItem,     m_zeroingDisplayItem,  
        m_windageDisplayItem , m_zoneWarningItem, m_leadAngleStatusTextItem, m_currentScanNameTextItem   
    };
    for (OutlinedTextItem* item : textItems) {
        if (item) {
            item->setOutlinePen(m_textOutlinePen);
            item->setFillBrush(m_fillBrush);
        }
    }

    // Azimuth/Elevation Labels (stored in vectors)
    for (OutlinedTextItem* item : m_azimuthLabels) {
        if (item) {
            item->setOutlinePen(m_textOutlinePen);
            item->setFillBrush(m_fillBrush);
        }
    }
    for (OutlinedTextItem* item : m_elevationLabels) {
        if (item) {
            item->setOutlinePen(m_textOutlinePen);
            item->setFillBrush(m_fillBrush);
        }
    }

    // Azimuth Indicator Shapes
    if (m_azimuthCircleOutline) m_azimuthCircleOutline->setPen(m_shapeOutlinePen);
    if (m_azimuthCircle) m_azimuthCircle->setPen(m_mainPen);
    if (m_azimuthNeedleOutline) m_azimuthNeedleOutline->setPen(m_needleOutlinePen);
    if (m_azimuthNeedle) m_azimuthNeedle->setPen(m_mainPen);

    // Azimuth Ticks
    for (QGraphicsLineItem* tick : m_azimuthTicks) { if (tick) tick->setPen(m_tickMarkMainPen); }
    for (QGraphicsLineItem* tickOutline : m_azimuthTicksOutline) { if (tickOutline) tickOutline->setPen(m_tickMarkOutlinePen); }

    // Elevation Scale Shapes
    if (m_elevationScaleOutline) m_elevationScaleOutline->setPen(m_shapeOutlinePen);
    if (m_elevationScale) m_elevationScale->setPen(m_mainPen);
    if (m_elevationIndicatorOutline) m_elevationIndicatorOutline->setPen(m_shapeOutlinePen);
    if (m_elevationIndicator) {
        QPen triMainPen(m_osdColor, 1.0); triMainPen.setCosmetic(true);
        m_elevationIndicator->setPen(triMainPen); // Update border
        m_elevationIndicator->setBrush(m_fillBrush); // Update fill
    }

    // Elevation Ticks
    for (QGraphicsLineItem* tick : m_elevationTicks) { if (tick) tick->setPen(m_tickMarkMainPen); }
    for (QGraphicsLineItem* tickOutline : m_elevationTicksOutline) { if (tickOutline) tickOutline->setPen(m_tickMarkOutlinePen); }

    // Tracking Corners (Outline color is handled by setupPensAndBrushes, main color by updateTrackingState)
    for (QGraphicsLineItem* outline : m_trackingCornersOutline) { 
        if (outline) outline->setPen(m_trackingOutlinePen); 
    }
    // Re-apply current tracking state color to main corners
    updateTrackingState(m_trackingState);

        //  LOB Marker Color Update --- // <<< ADD THIS SECTION
    if (m_fixedLobMarkerItem) {
        // Create a new pen for the LOB marker based on the new m_osdColor
        QPen lobMainPen(m_osdColor); // Or some other derivation
        lobMainPen.setCosmetic(true);
        m_fixedLobMarkerItem->setPen(lobMainPen);
    }
    if (m_fixedLobMarkerOutlineItem) {
        // The m_reticleOutlinePen (or a similar dark outline pen) should have been updated by setupPensAndBrushes()
        // If you use a specific LOB outline pen, update it here too.
        // For now, assume m_reticleOutlinePen is suitable or a general dark outline.
        QPen lobOutlinePen = m_reticleOutlinePen; // Use the (now updated) member pen
        // lobOutlinePen.setWidthF(m_reticleOutlinePen.widthF() > 1 ? m_reticleOutlinePen.widthF() -1 : 1); // Adjust width if needed
        m_fixedLobMarkerOutlineItem->setPen(lobOutlinePen);
    }



    // Reticle (Recreate to ensure correct colors/pens)
    // This is simpler than trying to update pens on potentially complex path items
    m_forceReticleRecreation = true;
    updateReticleType(m_reticleType);

    // Detection Boxes (Outline color handled by setupPensAndBrushes, main pen created in drawDetectionBox)
    // Re-drawing them ensures correct colors if they depend on m_osdColor
    // Need the last known detections to redraw. This might be complex.
    // Alternative: Iterate and update pens if possible.
    for (auto item : m_detectionRectOutlines) { if(item) item->setPen(m_shapeOutlinePen); }
    QPen detectionPen(m_osdColor, 1); detectionPen.setCosmetic(true);
    for (auto item : m_detectionRectItems) { if(item) item->setPen(detectionPen); }
    for (auto item : m_detectionTextItems) {
        if (item) {
            item->setOutlinePen(m_textOutlinePen);
            item->setFillBrush(m_fillBrush);
        }
    }


    m_view.update(); // Request redraw
}

void OsdRenderer::updateZeroingDisplay(bool zeroingModeActive, bool zeroingApplied,
                                       float azOffset, float elOffset)
{
    if (!m_zeroingDisplayItem) return;

    if (zeroingModeActive) {
        // PDF: "When Zeroing is started, ZEROING displays in the status field"
        m_zeroingDisplayItem->setText("ZEROING");
        m_zeroingDisplayItem->setVisible(true);
        // Optionally display current offsets being adjusted if you have m_zeroingValuesItem
        // if (m_zeroingValuesItem) {
        //     m_zeroingValuesItem->setText(QString("Off: Az %1 El %2").arg(azOffset, 0, 'f', 2).arg(elOffset, 0, 'f', 2));
        //     m_zeroingValuesItem->setVisible(true);
        // }
    } else if (zeroingApplied) {
        // PDF: "A 'Z' will display on the engagement screen."
        // PDF: "Z displays in the zeroing field on screen."
        m_zeroingDisplayItem->setText("Z"); // Concise indicator that zeroing is active
        m_zeroingDisplayItem->setVisible(true);
        // if (m_zeroingValuesItem) m_zeroingValuesItem->setVisible(false);
    } else {
        m_zeroingDisplayItem->setVisible(false);
        // if (m_zeroingValuesItem) m_zeroingValuesItem->setVisible(false);
    }
}

void OsdRenderer::updateWindageDisplay(bool windageModeActive, bool windageApplied,
                                       float speedKnots)
{
    if (!m_windageDisplayItem) return;

    if (windageModeActive) {
        // Displaying that windage setting UI is active and current speed being set
        m_windageDisplayItem->setText(QString("WINDAGE: %1 kt").arg(speedKnots, 0, 'f', 0));
        m_windageDisplayItem->setVisible(true);
    } else if (windageApplied) {
        // PDF: "A 'W' will display on the engagement screen."
        // PDF: "A W appears in the Zeroing, Windage field (1) on screen"
        m_windageDisplayItem->setText(QString("W: %1 kt").arg(speedKnots, 0, 'f', 0)); // Show "W" and the applied speed
        m_windageDisplayItem->setVisible(true);
    } else {
        m_windageDisplayItem->setVisible(false);
    }
}

void OsdRenderer::updateCurrentScanNameDisplay(const QString& scanName) {
    if (m_currentScanNameTextItem) {
        m_currentScanNameTextItem->setText(scanName);
        m_currentScanNameTextItem->setVisible(!scanName.isEmpty());
    }
}

void OsdRenderer::updateZoneWarning(bool inNoFireZone, bool inNoTraverseZoneAtLimit) {
    if (!m_zoneWarningItem) return;

    QString warningText = "";
    bool showWarning = false;

    if (inNoFireZone) {
        warningText = "NO FIRE ZONE";
        showWarning = true;
    } else if (inNoTraverseZoneAtLimit) {
        // This implies the gimbal *stopped* at the edge of a No Traverse Zone
        warningText = "NO TRAVERSE LIMIT";
        showWarning = true;
    }

    if (showWarning) {
        m_zoneWarningItem->setText(warningText);
        // Dynamically center the text item based on its new bounding rect
        qreal textWidth = m_zoneWarningItem->boundingRect().width();
        m_zoneWarningItem->setPos(m_width  / 2.0 + 50, m_height / 2.0 + 50); // Adjust Y as needed
        m_zoneWarningItem->setVisible(true);
    } else {
        m_zoneWarningItem->setVisible(false);
    }
}

void OsdRenderer::updateAppliedZeroingOffsets(bool applied, float azOffsetDegrees, float elOffsetDegrees) {
    bool changed = (m_isZeroingCurrentlyApplied != applied ||
                    !qFuzzyCompare(m_currentZeroingAzOffsetDegrees, azOffsetDegrees) || // Store and compare angular
                    !qFuzzyCompare(m_currentZeroingElOffsetDegrees, elOffsetDegrees));

    m_isZeroingCurrentlyApplied = applied;
    if (m_isZeroingCurrentlyApplied) {
        m_currentZeroingAzOffsetDegrees = azOffsetDegrees; // Store angular
        m_currentZeroingElOffsetDegrees = elOffsetDegrees; // Store angular
        QPointF pixelOffsets = convertAngularToPixelOffset(azOffsetDegrees, elOffsetDegrees);
        m_zeroingOffsetXPx = pixelOffsets.x();
        m_zeroingOffsetYPx = pixelOffsets.y();
    } else {
        m_currentZeroingAzOffsetDegrees = 0.0f;
        m_currentZeroingElOffsetDegrees = 0.0f;
        m_zeroingOffsetXPx = 0.0f;
        m_zeroingOffsetYPx = 0.0f;
    }

    if (changed) {
        applyReticlePosition();
        // m_view.update(); // applyReticlePosition should be enough if it triggers scene update
    }
}

void OsdRenderer::updateReticlePosition(float screenX_px, float screenY_px) {
    if (!m_reticleRootGroup) return;
    if (!qFuzzyCompare(static_cast<float>(m_reticleRootGroup->pos().x()), screenX_px) ||
        !qFuzzyCompare(static_cast<float>(m_reticleRootGroup->pos().y()), screenY_px)) {
        m_reticleRootGroup->setPos(screenX_px, screenY_px);
        qDebug() << "OSD::updateReticlePosition: Group Pos set to" << screenX_px << "," << screenY_px;
        // m_view.update(); // Scene changes might trigger this, or renderOsd handles repaint
    }
}

void OsdRenderer::updateLeadStatusText(const QString& text) {
    if (m_leadAngleStatusTextItem) {
        m_leadAngleStatusTextItem->setText(text);
        m_leadAngleStatusTextItem->setVisible(!text.isEmpty());
        // Update color based on content if desired (e.g. LAG is yellow)
        if (text.contains("LAG")) m_leadAngleStatusTextItem->setFillBrush(Qt::yellow);
        else if (text.contains("ZOOM")) m_leadAngleStatusTextItem->setFillBrush(QColor(200,20,40));
        else m_leadAngleStatusTextItem->setFillBrush(m_fillBrush); // Default OSD color
    }
}

void OsdRenderer::updateLeadAngleDisplay(bool active, LeadAngleStatus status,
                                       float offsetAzDegrees, float offsetElDegrees) {
    // ... (update m_leadAngleStatusTextItem as before) ...
    if (!m_leadAngleStatusTextItem) return;

    if (active) {
        switch (status) {
        case LeadAngleStatus::On:
            m_leadAngleStatusTextItem->setText("LEAD ANGLE ON");
            m_leadAngleStatusTextItem->setFillBrush(QBrush(m_osdColor)); // Normal color
            break;
        case LeadAngleStatus::Lag:
            m_leadAngleStatusTextItem->setText("LEAD ANGLE LAG");
            m_leadAngleStatusTextItem->setFillBrush(QBrush(Qt::yellow)); // Warning color
            break;
        case LeadAngleStatus::ZoomOut:
            m_leadAngleStatusTextItem->setText("ZOOM OUT"); // Over reticle, as per PDF
                // This might need special placement logic
                // Or be a separate, larger text item.
            m_leadAngleStatusTextItem->setFillBrush(QBrush(QColor(200,20,40))); // Error/Alert color
            break;
        case LeadAngleStatus::Off: // Should not happen if 'active' is true, but handle defensively
            m_leadAngleStatusTextItem->setVisible(false);
            return;
        }
        m_leadAngleStatusTextItem->setVisible(true);
    } else {
        m_leadAngleStatusTextItem->setVisible(false);
    }

    m_isLacCurrentlyActive = active && (status == LeadAngleStatus::On || status == LeadAngleStatus::Lag);

    bool offsetsChanged = !qFuzzyCompare(m_currentLeadAzOffsetDegrees, offsetAzDegrees) ||
                          !qFuzzyCompare(m_currentLeadElOffsetDegrees, offsetElDegrees);


    if (m_isLacCurrentlyActive) {
        m_currentLeadAzOffsetDegrees = offsetAzDegrees; // Store angular
        m_currentLeadElOffsetDegrees = offsetElDegrees; // Store angular
        QPointF pixelOffsets = convertAngularToPixelOffset(offsetAzDegrees, offsetElDegrees);
        m_leadOffsetXPx = pixelOffsets.x();
        m_leadOffsetYPx = pixelOffsets.y();
    } else {
        m_currentLeadAzOffsetDegrees = 0.0f;
        m_currentLeadElOffsetDegrees = 0.0f;
        m_leadOffsetXPx = 0.0f;
        m_leadOffsetYPx = 0.0f;
    }

    if (m_isLacActiveForReticle != m_isLacCurrentlyActive || offsetsChanged || m_leadAngleStatusTextItem->isVisible() != active) { // Check if visibility or offsets changed
        applyReticlePosition();
    }
     // Update text status visibility
    if (m_leadAngleStatusTextItem) { // Ensure item exists
        if (active) {
            // Set text based on status as before...
            m_leadAngleStatusTextItem->setVisible(true);
        } else {
            m_leadAngleStatusTextItem->setVisible(false);
        }
    }
}


QPointF OsdRenderer::convertAngularToPixelOffset(float gun_offsetAzDegrees, float gun_offsetElDegrees) {
    if (m_currentHfov <= 0.001f || m_width <= 0 || m_height <= 0) {
        return QPointF(0,0);
    }

    double ppdAz = static_cast<double>(m_width) / m_currentHfov;
    double vfov_rad = 2.0 * std::atan( (static_cast<double>(m_height) / static_cast<double>(m_width)) * std::tan((m_currentHfov * M_PI / 180.0) / 2.0) );
    double vfov_deg = vfov_rad * 180.0 / M_PI;
    double ppdEl = (vfov_deg > 0.001f) ? (static_cast<double>(m_height) / vfov_deg) : ppdAz;

    // Calculate reticle's pixel displacement on screen.
    // If gun needs to aim right (gun_offsetAzDegrees > 0), reticle moves left.
    qreal reticle_shift_x_px = static_cast<qreal>(-gun_offsetAzDegrees * ppdAz);

    // If gun needs to aim up (gun_offsetElDegrees > 0), reticle moves down on screen (positive Y in Qt).
    qreal reticle_shift_y_px = static_cast<qreal>(gun_offsetElDegrees * ppdEl);

    qDebug() << "convertAngularToPixelOffset: GunOffsetAzD" << gun_offsetAzDegrees << "GunOffsetElD" << gun_offsetElDegrees
             << "-> ReticleShiftPxX" << reticle_shift_x_px << "ReticleShiftPxY" << reticle_shift_y_px;

    return QPointF(reticle_shift_x_px, reticle_shift_y_px);
}


void OsdRenderer::applyReticlePosition() {
    if (!m_reticleRootGroup) return;

    qreal totalOffsetXpx = 0.0;
    qreal totalOffsetYpx = 0.0;

    if (m_isZeroingCurrentlyApplied) {
        totalOffsetXpx += m_zeroingOffsetXPx;
        totalOffsetYpx += m_zeroingOffsetYPx;
    }
    if (m_isLacCurrentlyActive) {
        totalOffsetXpx += m_leadOffsetXPx;
        totalOffsetYpx += m_leadOffsetYPx;
    }

    qreal screenCenterX = m_width / 2.0;
    qreal screenCenterY = m_height / 2.0;

    m_reticleRootGroup->setPos(screenCenterX + totalOffsetXpx, screenCenterY + totalOffsetYpx);
    // qDebug() << "Reticle Group new pos:" << m_reticleRootGroup->pos() << "Total Offset X:" << totalOffsetXpx << "Y:" << totalOffsetYpx;
        /*qDebug() << "applyReticlePosition: ScreenCenter(" << screenCenterX << "," << screenCenterY
             << "), TotalOffset(" << totalOffsetXpx << "," << totalOffsetYpx
             << "), ZeroingActive:" << m_isZeroingCurrentlyApplied << "Zx:" << m_zeroingOffsetXPx << "Zy:" << m_zeroingOffsetYPx
             << ", LACActive:" << m_isLacCurrentlyActive << "Lx:" << m_leadOffsetXPx << "Ly:" << m_leadOffsetYPx;
        */
    m_reticleRootGroup->setPos(screenCenterX + totalOffsetXpx, screenCenterY + totalOffsetYpx);
    qDebug() << "applyReticlePosition: m_reticleRootGroup new effective pos:" << m_reticleRootGroup->pos();
}



QPointF OsdRenderer::convertAngularOffsetToPixelOffset(float offsetAzDegrees, float offsetElDegrees)
{
    // This needs the current camera's Field of View (FOV)
    // Assuming m_fov stores the horizontal FOV in degrees (or get it from SystemStateModel)
    // And m_width, m_height are the OSD/screen dimensions.

    if (m_fov <= 0) return QPointF(0,0); // Cannot calculate

    // Pixels per degree for horizontal (Azimuth)
    double pixelsPerDegreeAz = static_cast<double>(m_width) / m_fov;

    // For Elevation, we need vertical FOV. If screen is 4:3 and HFOV is given:
    // VFOV = HFOV * (screenHeight / screenWidth) if aspect ratio of sensor matches screen
    // Or, more simply, assume square pixels and calculate pixelsPerDegreeEl similarly
    // if you have vertical FOV.
    // For a quick approximation, if pixel aspect ratio is 1:
    double vfov_approx = m_fov * (static_cast<double>(m_height) / static_cast<double>(m_width));
    double pixelsPerDegreeEl = static_cast<double>(m_height) / vfov_approx;
    // A better way is to have both HFOV and VFOV available.

    // Calculate pixel offsets
    // Azimuth offset: positive offsetAzDegrees means target is to the right, so reticle moves left.
    // Elevation offset: positive offsetElDegrees means target is up, so reticle moves down.
    // The sign depends on how your lead offsets are defined (e.g., where the reticle *should go*
    // or where the *gun should point relative to reticle*).
    // Let's assume leadOffsetAz > 0 means reticle shifts RIGHT on screen.
    // Let's assume leadOffsetEl > 0 means reticle shifts UP on screen.
    // (Adjust signs based on your coordinate system and lead definition)
    qreal xOffsetPx = static_cast<qreal>(offsetAzDegrees * pixelsPerDegreeAz);
    qreal yOffsetPx = static_cast<qreal>(-offsetElDegrees * pixelsPerDegreeEl); // Negative because QPoint Y is down

    return QPointF(xOffsetPx, yOffsetPx);
}


