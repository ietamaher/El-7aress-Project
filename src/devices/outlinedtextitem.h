#ifndef OUTLINEDTEXTITEM_OPTIMIZED_H
#define OUTLINEDTEXTITEM_OPTIMIZED_H

#include <QGraphicsSimpleTextItem> // Base class
#include <QPen>                  // For outline pen
#include <QBrush>                // For fill brush
#include <QString>               // For text constructor
#include <QPainter>              // For paint method
#include <QStyleOptionGraphicsItem> // For paint method
#include <QWidget>               // For paint method
#include <QPainterPath>          // For shape calculation
#include <QPainterPathStroker>   // For shape calculation

// Forward declaration if needed, but seems self-contained

/**
 * @brief A QGraphicsSimpleTextItem subclass that draws text with an outline.
 *
 * This class allows setting a separate pen for the outline and a brush for the fill,
 * providing a common visual effect for OSD elements.
 */
class OutlinedTextItem : public QGraphicsSimpleTextItem
{
public:
    // Use explicit constructors to avoid unintended conversions
    explicit OutlinedTextItem(QGraphicsItem *parent = nullptr);
    explicit OutlinedTextItem(const QString &text, QGraphicsItem *parent = nullptr);

    // Setters for appearance
    void setOutlinePen(const QPen &pen);
    void setFillBrush(const QBrush &brush);

    // --- QGraphicsItem overrides ---

    /**
     * @brief Reimplemented to draw the text outline and then the fill.
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    /**
     * @brief Reimplemented to account for the outline pen width.
     */
    QRectF boundingRect() const override;

    /**
     * @brief Reimplemented to provide a more accurate shape including the outline for collision detection.
     */
    QPainterPath shape() const override;

private:
    QPen m_outlinePen;  // Pen used for drawing the text outline
    QBrush m_fillBrush; // Brush used for filling the text
};

#endif // OUTLINEDTEXTITEM_OPTIMIZED_H

