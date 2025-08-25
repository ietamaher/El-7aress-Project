#include "outlinedtextitem.h"
#include <QPainter>              // For drawing operations
#include <QPainterPath>          // For creating text path
#include <QPainterPathStroker>   // For creating outline shape
#include <Qt>                    // For Qt::SolidLine, Qt::RoundCap, etc.

// --- Constructors ---

OutlinedTextItem::OutlinedTextItem(QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(parent),
      // Initialize with default values (black 1px outline, white fill)
      m_outlinePen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
      m_fillBrush(Qt::white)
{
    // Optional: Set flags like ItemUsesDeviceCoordinates if needed for specific behavior
    // setFlag(QGraphicsItem::ItemUsesDeviceCoordinates);
}

OutlinedTextItem::OutlinedTextItem(const QString &text, QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(text, parent),
      // Initialize with default values
      m_outlinePen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
      m_fillBrush(Qt::white)
{
    // Optional: Set flags
}

// --- Setters ---

void OutlinedTextItem::setOutlinePen(const QPen &pen) {
    // Only update if the pen has actually changed
    if (m_outlinePen != pen) {
        // Inform the graphics view system that the geometry might change
        // This is crucial if the pen width affects the bounding box
        prepareGeometryChange();
        m_outlinePen = pen;
        update(); // Schedule a repaint to reflect the change
    }
}

void OutlinedTextItem::setFillBrush(const QBrush &brush) {
    // Only update if the brush has actually changed
    if (m_fillBrush != brush) {
        m_fillBrush = brush;
        update(); // Schedule a repaint
    }
}

// --- QGraphicsItem Overrides ---

void OutlinedTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Avoid compiler warnings for unused parameters
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Enable antialiasing for smoother text rendering
    painter->setRenderHint(QPainter::Antialiasing);

    // Create a painter path from the text
    // Using (0, 0) as the origin because the item's position handles the placement
    QPainterPath path;
    path.addText(0, 0, font(), text());

    // 1. Draw the Outline:
    // Set the painter's pen to the outline pen
    painter->setPen(m_outlinePen);
    // Ensure the path is not filled when drawing the outline
    painter->setBrush(Qt::NoBrush);
    // Draw the outline
    painter->drawPath(path);

    // 2. Draw the Fill:
    // Ensure the path is not stroked when drawing the fill
    painter->setPen(Qt::NoPen);
    // Set the painter's brush to the fill brush
    painter->setBrush(m_fillBrush);
    // Draw the filled text
    painter->drawPath(path);
}

QRectF OutlinedTextItem::boundingRect() const {
    // Get the base bounding rectangle from the parent class
    QRectF baseRect = QGraphicsSimpleTextItem::boundingRect();

    // Calculate the pen width, handling NoPen case
    qreal penWidth = (m_outlinePen.style() == Qt::NoPen) ? 0.0 : m_outlinePen.widthF();

    // If pen width is negligible, return the base rectangle
    if (penWidth <= 0) {
        return baseRect;
    }

    // Adjust the bounding rectangle outwards by half the pen width on each side
    // This ensures the outline is fully contained within the bounding rectangle
    // Use widthF() for potentially non-integer pen widths
    qreal adjust = penWidth / 2.0;
    return baseRect.adjusted(-adjust, -adjust, adjust, adjust);
}

QPainterPath OutlinedTextItem::shape() const {
    // Get the basic shape of the text (the fill area) from the parent class
    QPainterPath fillPath = QGraphicsSimpleTextItem::shape();

    // Check if an outline should be drawn (pen exists and has width)
    if (m_outlinePen.style() != Qt::NoPen && m_outlinePen.widthF() > 0) {
        // Create a stroker to generate the outline shape
        QPainterPathStroker stroker;
        stroker.setWidth(m_outlinePen.widthF());
        stroker.setCapStyle(m_outlinePen.capStyle());
        stroker.setJoinStyle(m_outlinePen.joinStyle());
        stroker.setMiterLimit(m_outlinePen.miterLimit());

        // Create the stroke path from the fill path
        QPainterPath strokePath = stroker.createStroke(fillPath);

        // Combine the fill shape and the stroke shape for the final accurate shape
        // This is important for accurate collision detection
        return fillPath.united(strokePath);
    } else {
        // If there's no outline, the shape is just the fill path
        return fillPath;
    }
}
