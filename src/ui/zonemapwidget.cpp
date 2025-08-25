#include "zonemapwidget.h"
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <QFont>
#include <cmath>

ZoneMapWidget::ZoneMapWidget(SystemStateModel* model, QWidget *parent)
    : QWidget(parent),
    m_stateModel(model),
    m_currentGimbalAz(0.0f),
    m_currentGimbalEl(0.0f),
    m_isWipZoneActive(false),
    m_isWipDefiningStart(false),
    m_isWipDefiningEnd(false),
    m_wipZoneType(ZoneType::None),
    m_highlightedZoneId(-1),
    m_highlightedZoneType(ZoneType::None)
{
    Q_ASSERT(m_stateModel != nullptr);
    setMinimumSize(400, 300);

    // Connect to model updates
    connect(m_stateModel, &SystemStateModel::zonesChanged,
            this, &ZoneMapWidget::onZonesChanged);
    // Connect gimbal position updates (can also be driven by controller calling updateGimbalPosition)
    // Choose one method: either connect here OR have controller call updateGimbalPosition.
    // Connecting here ensures map updates even if controller doesn't explicitly call it.
    connect(m_stateModel, &SystemStateModel::gimbalPositionChanged,
            this, &ZoneMapWidget::updateGimbalPosition);
}

// --- Public Update Methods ---

void ZoneMapWidget::updateGimbalPosition(float az, float el)
{
    float normalizedAz = normalizeAzimuthTo360(az);

    if (m_currentGimbalAz != normalizedAz || m_currentGimbalEl != el) {
        m_currentGimbalAz = normalizedAz;
        m_currentGimbalEl = el;
        update(); // Trigger repaint
    }
}

// New generic WIP update method
void ZoneMapWidget::updateWipZone(const QVariant& wipData, ZoneType type, bool isDefiningStart, bool isDefiningEnd)
{
    m_wipZoneData = wipData;
    m_wipZoneType = type;
    m_isWipZoneActive = true;
    m_isWipDefiningStart = isDefiningStart;
    m_isWipDefiningEnd = isDefiningEnd;
    update(); // Trigger repaint
}

void ZoneMapWidget::clearWipZone()
{
    if (m_isWipZoneActive) {
        m_isWipZoneActive = false;
        m_isWipDefiningStart = false;
        m_isWipDefiningEnd = false;
        m_wipZoneType = ZoneType::None;
        m_wipZoneData.clear();
        update(); // Trigger repaint
    }
}

void ZoneMapWidget::refreshDisplay()
{
    update(); // Trigger repaint
}

void ZoneMapWidget::highlightZone(int zoneId, ZoneType zoneType)
{
    if (m_highlightedZoneId != zoneId || m_highlightedZoneType != zoneType) {
        m_highlightedZoneId = zoneId;
        m_highlightedZoneType = zoneType; // Store type as well
        update(); // Trigger repaint
    }
}

// --- Slots ---

void ZoneMapWidget::onZonesChanged()
{
    // Model data changed, simply trigger a repaint
    update();
}

// --- Paint Event and Drawing Helpers ---

void ZoneMapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background and grid
    drawBackground(painter);
    drawGrid(painter);
    drawAxesLabels(painter);

    // Draw SAVED zones
    drawSavedAreaZones(painter);
    drawSavedSectorScanZones(painter);
    drawSavedTRPs(painter);

    // Draw WIP zone
    if (m_isWipZoneActive) {
        drawWipZone(painter);
    }

    // Draw gimbal position last (on top)
    drawGimbalIndicator(painter);
}

void ZoneMapWidget::drawBackground(QPainter &painter)
{
    painter.fillRect(rect(), QColor(40, 40, 40)); // Dark background
}

void ZoneMapWidget::drawGrid(QPainter &painter)
{
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    int w = width();
    int h = height();
    float azRange = AZ_MAX_DISPLAY - AZ_MIN_DISPLAY;
    float elRange = EL_MAX_DISPLAY - EL_MIN_DISPLAY;

    // Azimuth grid (0-360)
    float azStep = 30.0f;
    for (float az = AZ_MIN_DISPLAY; az <= AZ_MAX_DISPLAY; az += azStep) {
        float x = (az - AZ_MIN_DISPLAY) / azRange * w;
        painter.drawLine(QPointF(x, 0), QPointF(x, h));
    }
    // Elevation grid
    float elStep = 10.0f;
    for (float el = EL_MIN_DISPLAY; el <= EL_MAX_DISPLAY; el += elStep) {
        float y = h - ((el - EL_MIN_DISPLAY) / elRange * h);
        painter.drawLine(QPointF(0, y), QPointF(w, y));
    }
}

void ZoneMapWidget::drawAxesLabels(QPainter &painter)
{
    painter.setPen(QPen(Qt::white, 1));
    QFont font = painter.font(); font.setPointSize(8);
    painter.setFont(font);
    int w = width(); int h = height();
    float azRange = AZ_MAX_DISPLAY - AZ_MIN_DISPLAY;
    float elRange = EL_MAX_DISPLAY - EL_MIN_DISPLAY;

    // Azimuth labels (0-360)
    float azStep = 60.0f;
    for (float az = AZ_MIN_DISPLAY; az <= AZ_MAX_DISPLAY; az += azStep) {
        float x = (az - AZ_MIN_DISPLAY) / azRange * w;
        QString label = QString("%1°").arg(static_cast<int>(az));
        QFontMetrics fm(font); int textWidth = fm.horizontalAdvance(label);
        painter.drawText(QPointF(x - textWidth/2, h - 5), label);
    }
    // Elevation labels
    float elStep = 20.0f;
    for (float el = EL_MIN_DISPLAY; el <= EL_MAX_DISPLAY; el += elStep) {
        float y = h - ((el - EL_MIN_DISPLAY) / elRange * h);
        painter.drawText(QPointF(5, y + 5), QString("%1°").arg(static_cast<int>(el)));
    }
    // Axis titles
    painter.drawText(QPointF(w/2 - 20, h - 20), "Azimuth (0-360°)");
    painter.save(); painter.translate(15, h/2); painter.rotate(-90);
    painter.drawText(QPointF(-25, 0), "Elevation"); painter.restore();
}

void ZoneMapWidget::drawGimbalIndicator(QPainter &painter)
{
    QPointF gimbalPixel = azElToPixel(m_currentGimbalAz, m_currentGimbalEl);
    if (gimbalPixel.x() >= 0 && gimbalPixel.x() <= width() &&
        gimbalPixel.y() >= 0 && gimbalPixel.y() <= height()) {
        painter.setPen(QPen(Qt::yellow, 2));
        painter.setBrush(Qt::yellow);
        painter.drawLine(gimbalPixel.x() - 10, gimbalPixel.y(), gimbalPixel.x() + 10, gimbalPixel.y());
        painter.drawLine(gimbalPixel.x(), gimbalPixel.y() - 10, gimbalPixel.x(), gimbalPixel.y() + 10);
        painter.drawEllipse(gimbalPixel, 3, 3);
    }
}

// --- Saved Zone Drawing ---

void ZoneMapWidget::drawSavedAreaZones(QPainter &painter)
{
    if (!m_stateModel) return;
    for (const auto& zone : m_stateModel->getAreaZones()) {
        if (!zone.isEnabled) continue;
        bool isHighlighted = (zone.id == m_highlightedZoneId && m_highlightedZoneType == zone.type);
        drawWrappedZoneRect(painter, zone, isHighlighted, false);
    }
}

void ZoneMapWidget::drawSavedSectorScanZones(QPainter &painter)
{
    if (!m_stateModel) return;
    for (const auto& zone : m_stateModel->getSectorScanZones()) {
        if (!zone.isEnabled) continue;
        bool isHighlighted = (zone.id == m_highlightedZoneId && m_highlightedZoneType == ZoneType::AutoSectorScan);
        drawSectorScanLine(painter, zone, isHighlighted, false);
    }
}

void ZoneMapWidget::drawSavedTRPs(QPainter &painter)
{
    if (!m_stateModel) return;
    for (const auto& zone : m_stateModel->getTargetReferencePoints()) {
        // TRPs are always considered 'enabled' unless explicitly filtered
        bool isHighlighted = (zone.id == m_highlightedZoneId && m_highlightedZoneType == ZoneType::TargetReferencePoint);
        drawTRPMarker(painter, zone, isHighlighted, false);
    }
}

// --- WIP Zone Drawing ---

void ZoneMapWidget::drawWipZone(QPainter &painter)
{
    if (!m_isWipZoneActive || !m_wipZoneData.isValid()) return;

    switch (m_wipZoneType) {
        case ZoneType::Safety:
        case ZoneType::NoTraverse:
        case ZoneType::NoFire:
            if (m_wipZoneData.canConvert<AreaZone>()) {
                drawWipAreaZoneInternal(painter, m_wipZoneData.value<AreaZone>(), m_isWipDefiningStart, m_isWipDefiningEnd);
            }
            break;
        case ZoneType::AutoSectorScan:
             if (m_wipZoneData.canConvert<AutoSectorScanZone>()) {
                drawWipSectorScanZoneInternal(painter, m_wipZoneData.value<AutoSectorScanZone>(), m_isWipDefiningStart, m_isWipDefiningEnd);
            }
            break;
        case ZoneType::TargetReferencePoint:
             if (m_wipZoneData.canConvert<TargetReferencePoint>()) {
                drawWipTRPInternal(painter, m_wipZoneData.value<TargetReferencePoint>(), m_isWipDefiningStart, m_isWipDefiningEnd);
            }
            break;
        default:
            break;
    }
}

void ZoneMapWidget::drawWipAreaZoneInternal(QPainter &painter, const AreaZone& zone, bool definingStart, bool definingEnd)
{
    if (definingStart && !definingEnd) {
        // Draw vertical line at startAz and line to current gimbal
        float normalizedStartAz = normalizeAzimuthTo360(zone.startAzimuth);
        painter.setPen(QPen(Qt::green, 2, Qt::DotLine));
        float x = (normalizedStartAz - AZ_MIN_DISPLAY) / (AZ_MAX_DISPLAY - AZ_MIN_DISPLAY) * width();
        painter.drawLine(QPointF(x, 0), QPointF(x, height()));

        QPointF gimbalPixel = azElToPixel(m_currentGimbalAz, m_currentGimbalEl);
        painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
        painter.drawLine(QPointF(x, height()/2), gimbalPixel); // Line from center of start line to gimbal
    }
    else if (definingStart && definingEnd) {
        // Draw the complete WIP rectangle
        drawWrappedZoneRect(painter, zone, false, true);
    }
}

void ZoneMapWidget::drawWipSectorScanZoneInternal(QPainter &painter, const AutoSectorScanZone& zone, bool definingStart, bool definingEnd)
{
     if (definingStart && !definingEnd) {
        // Draw marker at point 1 and line to current gimbal
        QPointF p1 = azElToPixel(normalizeAzimuthTo360(zone.az1), zone.el1);
        painter.setPen(QPen(Qt::green, 2));
        painter.setBrush(Qt::green);
        painter.drawEllipse(p1, 4, 4); // Mark point 1

        QPointF gimbalPixel = azElToPixel(m_currentGimbalAz, m_currentGimbalEl);
        painter.setPen(QPen(Qt::green, 1, Qt::DashLine));
        painter.drawLine(p1, gimbalPixel);
    }
    else if (definingStart && definingEnd) {
        // Draw the complete WIP line
        drawSectorScanLine(painter, zone, false, true);
    }
}

void ZoneMapWidget::drawWipTRPInternal(QPainter &painter, const TargetReferencePoint& zone, bool definingStart, bool definingEnd)
{
     if (definingStart && !definingEnd) {
        // Draw marker at current gimbal position (where user is aiming)
        QPointF gimbalPixel = azElToPixel(m_currentGimbalAz, m_currentGimbalEl);
        painter.setPen(QPen(Qt::green, 2, Qt::DotLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(gimbalPixel.x() - 8, gimbalPixel.y(), gimbalPixel.x() + 8, gimbalPixel.y());
        painter.drawLine(gimbalPixel.x(), gimbalPixel.y() - 8, gimbalPixel.x(), gimbalPixel.y() + 8);
    }
    else if (definingStart && definingEnd) {
        // Draw the defined WIP TRP marker
        drawTRPMarker(painter, zone, false, true);
    }
}

// --- Specific Drawing Primitives ---

void ZoneMapWidget::drawZoneRect(QPainter& painter, const AreaZone& zone, bool isHighlighted, bool isWip)
{
    QColor zoneColor = Qt::gray;
    if (zone.type == ZoneType::Safety) zoneColor = Qt::cyan;
    else if (zone.type == ZoneType::NoTraverse) zoneColor = QColor(200,20,40);
    else if (zone.type == ZoneType::NoFire) zoneColor = Qt::magenta;

    if (isWip) zoneColor = Qt::green;
    else if (zone.isFactorySet) zoneColor = zoneColor.darker(120);
    if (isHighlighted) zoneColor = zoneColor.lighter(150);

    Qt::PenStyle penStyle = zone.isOverridable ? Qt::DashLine : Qt::SolidLine;
    if (isWip) penStyle = Qt::DotLine;
    int penWidth = isHighlighted ? 3 : 2;
    painter.setPen(QPen(zoneColor, penWidth, penStyle));

    QColor fillColor = zoneColor; fillColor.setAlpha(30);
    painter.setBrush(QBrush(fillColor));

    QRectF rect = areaZoneToRect(zone);
    painter.drawRect(rect);

    // Draw ID label inside if space permits
    if (!isWip && rect.width() > 30 && rect.height() > 15) {
        painter.setPen(Qt::white);
        QFont font = painter.font(); font.setPointSize(7);
        painter.setFont(font);
        painter.drawText(rect.topLeft() + QPointF(3, 10), QString("ID:%1").arg(zone.id));
    }
}

void ZoneMapWidget::drawWrappedZoneRect(QPainter& painter, const AreaZone& zone, bool isHighlighted, bool isWip)
{
    float startAz = normalizeAzimuthTo360(zone.startAzimuth);
    float endAz = normalizeAzimuthTo360(zone.endAzimuth);
    bool wrapsAround = (startAz > endAz);

    if (!wrapsAround) {
        AreaZone normalizedZone = zone; // Create copy to modify az values for drawing
        normalizedZone.startAzimuth = startAz;
        normalizedZone.endAzimuth = endAz;
        drawZoneRect(painter, normalizedZone, isHighlighted, isWip);
    } else {
        // Draw part 1: startAz to 360
        AreaZone leftPart = zone;
        leftPart.startAzimuth = startAz;
        leftPart.endAzimuth = 359.9f;
        drawZoneRect(painter, leftPart, isHighlighted, isWip);
        // Draw part 2: 0 to endAz
        AreaZone rightPart = zone;
        rightPart.startAzimuth = 0.0f;
        rightPart.endAzimuth = endAz;
        drawZoneRect(painter, rightPart, isHighlighted, isWip);
    }
}

void ZoneMapWidget::drawSectorScanLine(QPainter& painter, const AutoSectorScanZone& zone, bool isHighlighted, bool isWip)
{
    float az1 = normalizeAzimuthTo360(zone.az1);
    float az2 = normalizeAzimuthTo360(zone.az2);
    float el1 = zone.el1;
    float el2 = zone.el2;

    QColor color = Qt::blue;
    if (isWip) color = Qt::green;
    if (isHighlighted) color = color.lighter(150);

    Qt::PenStyle penStyle = isWip ? Qt::DotLine : Qt::SolidLine;
    int penWidth = isHighlighted ? 3 : 2;
    painter.setPen(QPen(color, penWidth, penStyle));
    painter.setBrush(Qt::NoBrush);

    QPointF p1 = azElToPixel(az1, el1);
    QPointF p2 = azElToPixel(az2, el2);

    // Vérifier si on traverse la limite 0/360°
    // Cela arrive quand az1 > az2 et la différence est significative
    bool crossesZero = (az1 > az2) && ((az1 - az2) > 180.0f);
    
    if (crossesZero) {
        // Calculer l'élévation interpolée au point de croisement
        float totalAzSpan = (360.0f - az1) + az2;  // Span total en traversant 0°
        float elAtZero = el1 + (el2 - el1) * (360.0f - az1) / totalAzSpan;
        
        // Points aux limites 0° et 360°
        QPointF pZero = azElToPixel(0.0f, elAtZero);
        QPointF p360 = azElToPixel(359.9f, elAtZero);
        
        // Dessiner deux segments
        painter.drawLine(p1, p360);  // De az1 à 360°
        painter.drawLine(pZero, p2); // De 0° à az2
        
        // Marqueurs aux extrémités principales
        painter.setBrush(color);
        painter.drawEllipse(p1, 3, 3);
        painter.drawEllipse(p2, 3, 3);
        
        // Petits marqueurs aux points de transition (optionnel)
        painter.setBrush(color.lighter(130));
        painter.drawEllipse(pZero, 2, 2);
        painter.drawEllipse(p360, 2, 2);
        
        // ID près du premier segment
        if (!isWip) {
            QPointF midPoint = (p1 + p360) / 2;
            painter.setPen(Qt::white);
            QFont font = painter.font(); 
            font.setPointSize(7);
            painter.setFont(font);
            painter.drawText(midPoint + QPointF(5, -5), QString("ID:%1").arg(zone.id));
        }
    } else {
        // Cas normal : une seule ligne
        painter.drawLine(p1, p2);
        
        // Marqueurs aux extrémités
        painter.setBrush(color);
        painter.drawEllipse(p1, 3, 3);
        painter.drawEllipse(p2, 3, 3);
        
        // ID au milieu
        if (!isWip) {
            QPointF midPoint = (p1 + p2) / 2;
            painter.setPen(Qt::white);
            QFont font = painter.font(); 
            font.setPointSize(7);
            painter.setFont(font);
            painter.drawText(midPoint + QPointF(5, -5), QString("ID:%1").arg(zone.id));
        }
    }
}

void ZoneMapWidget::drawTRPMarker(QPainter& painter, const TargetReferencePoint& zone, bool isHighlighted, bool isWip)
{
    QPointF p = azElToPixel(normalizeAzimuthTo360(zone.azimuth), zone.elevation);

    QColor color = Qt::yellow;
    if (isWip) color = Qt::green;
    if (isHighlighted) color = color.lighter(150);

    Qt::PenStyle penStyle = isWip ? Qt::DotLine : Qt::SolidLine;
    int penWidth = isHighlighted ? 3 : 2;
    painter.setPen(QPen(color, penWidth, penStyle));
    painter.setBrush(Qt::NoBrush);

    // Draw cross marker
    painter.drawLine(p.x() - 6, p.y(), p.x() + 6, p.y());
    painter.drawLine(p.x(), p.y() - 6, p.x(), p.y() + 6);

    // Draw ID label nearby if space permits
    if (!isWip) {
        painter.setPen(Qt::white);
        QFont font = painter.font(); 
        font.setPointSize(7);
        painter.setFont(font);
        painter.drawText(p + QPointF(8, -8), QString("ID:%1").arg(zone.id));
    }
}

// --- Coordinate Conversion ---

QPointF ZoneMapWidget::azElToPixel(float az, float el) const
{
    float azRange = AZ_MAX_DISPLAY - AZ_MIN_DISPLAY;
    float elRange = EL_MAX_DISPLAY - EL_MIN_DISPLAY;
    float x = (normalizeAzimuthTo360(az) - AZ_MIN_DISPLAY) / azRange * width();
    float y = height() - ((el - EL_MIN_DISPLAY) / elRange * height());
    return QPointF(x, y);
}

QRectF ZoneMapWidget::areaZoneToRect(const AreaZone& zone) const
{
    // Note: This assumes startAz and endAz are already normalized for drawing within this function call
    QPointF topLeft = azElToPixel(zone.startAzimuth, zone.maxElevation);
    QPointF bottomRight = azElToPixel(zone.endAzimuth, zone.minElevation);

    // Handle potential coordinate inversion if start/end points are swapped visually
    qreal left = qMin(topLeft.x(), bottomRight.x());
    qreal top = qMin(topLeft.y(), bottomRight.y());
    qreal right = qMax(topLeft.x(), bottomRight.x());
    qreal bottom = qMax(topLeft.y(), bottomRight.y());

    return QRectF(QPointF(left, top), QPointF(right, bottom));
}

// --- Normalization ---

float ZoneMapWidget::normalizeAzimuthTo360(float az) const
{
    // Ensures azimuth is in the [0, 360) range
    while (az < 0.0f) az += 360.0f;
    while (az >= 360.0f) az -= 360.0f;
    return az;
}

