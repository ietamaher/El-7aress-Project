#ifndef ZONEMAPWIDGET_H
#define ZONEMAPWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QVariant> // For generic WIP data
#include "../models/systemstatemodel.h"

class ZoneMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZoneMapWidget(SystemStateModel* model, QWidget *parent = nullptr);

    // Public methods to update the map state
    void updateGimbalPosition(float az, float el); // Called by controller or model signal
    // New generic WIP update method
    void updateWipZone(const QVariant& wipData, ZoneType type, bool isDefiningStart, bool isDefiningEnd);

    void clearWipZone();
    void refreshDisplay(); // Forces a repaint
    void highlightZone(int zoneId, ZoneType zoneType = ZoneType::None); // Added type hint for potential future use

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Drawing helpers
    void drawBackground(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawAxesLabels(QPainter &painter);
    void drawGimbalIndicator(QPainter &painter);

    // Drawing functions for SAVED zones (called within paintEvent)
    void drawSavedAreaZones(QPainter &painter);
    void drawSavedSectorScanZones(QPainter &painter); // New
    void drawSavedTRPs(QPainter &painter);          // New

    // Drawing functions for WIP zones (called within paintEvent)
    void drawWipZone(QPainter &painter); // Main WIP drawing dispatcher
    void drawWipAreaZoneInternal(QPainter &painter, const AreaZone& zone, bool definingStart, bool definingEnd);
    void drawWipSectorScanZoneInternal(QPainter &painter, const AutoSectorScanZone& zone, bool definingStart, bool definingEnd); // New
    void drawWipTRPInternal(QPainter &painter, const TargetReferencePoint& zone, bool definingStart, bool definingEnd);          // New

    // Specific drawing primitives
    void drawZoneRect(QPainter& painter, const AreaZone& zone, bool isHighlighted, bool isWip);
    void drawWrappedZoneRect(QPainter& painter, const AreaZone& zone, bool isHighlighted, bool isWip); // Handles 0/360 wrap for AreaZone
    void drawSectorScanLine(QPainter& painter, const AutoSectorScanZone& zone, bool isHighlighted, bool isWip); // New
    void drawTRPMarker(QPainter& painter, const TargetReferencePoint& zone, bool isHighlighted, bool isWip);    // New

    // Coordinate conversion helpers
    QPointF azElToPixel(float az, float el) const;
    QRectF areaZoneToRect(const AreaZone& zone) const; // Renamed for clarity

    // Helper to normalize azimuth
    float normalizeAzimuthTo360(float az) const;

private slots:
    // Slot connected to model's zonesChanged signal
    void onZonesChanged();
    // Slot connected to model's gimbalPositionChanged signal (NEW CONNECTION NEEDED)
    // Note: updateGimbalPosition can be called directly by controller OR connected to model signal

private:
    SystemStateModel* m_stateModel;
    float m_currentGimbalAz;
    float m_currentGimbalEl;

    // WIP zone state - Generic
    bool m_isWipZoneActive;
    bool m_isWipDefiningStart;
    bool m_isWipDefiningEnd;
    ZoneType m_wipZoneType;
    QVariant m_wipZoneData; // Store WIP data generically

    // Display parameters
    static constexpr float AZ_MIN_DISPLAY = 0.0f;
    static constexpr float AZ_MAX_DISPLAY = 360.0f;
    static constexpr float EL_MIN_DISPLAY = -20.0f;
    static constexpr float EL_MAX_DISPLAY = 60.0f;

    // Highlighting
    int m_highlightedZoneId;
    ZoneType m_highlightedZoneType; // Store type for potential specific highlighting
};

#endif // ZONEMAPWIDGET_H
