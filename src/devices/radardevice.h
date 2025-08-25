#ifndef RADARDEVICE_H
#define RADARDEVICE_H

#include "baseserialdevice.h"
#include <QObject>
#include <QVector>

struct RadarData {
    quint32 id = 0;           ///< Unique identifier for the tracked target.
    float azimuthDegrees = 0.0f; ///< Target's bearing from the vessel in degrees.
    float rangeMeters = 0.0f;    ///< Distance to the target in meters.
    float relativeCourseDegrees = 0.0f; ///< The target's course relative to the vessel in degrees.
    float relativeSpeedMPS = 0.0f; ///< The target's speed relative to the vessel in meters per second.

    bool operator==(const RadarData &other) const {
        return id == other.id &&
               qFuzzyCompare(azimuthDegrees, other.azimuthDegrees) &&
               qFuzzyCompare(rangeMeters, other.rangeMeters) &&
               qFuzzyCompare(relativeCourseDegrees, other.relativeCourseDegrees) &&
               qFuzzyCompare(relativeSpeedMPS, other.relativeSpeedMPS);
    }

    bool operator!=(const RadarData &other) const {
        return !(*this == other);
    }
};

class RadarDevice : public BaseSerialDevice
{
    Q_OBJECT

public:
    explicit RadarDevice(QObject *parent = nullptr);

signals:
    void radarPlotsUpdated(const QVector<RadarData> &plots);

protected:
    void configureSerialPort() override;
    void processIncomingData() override;

private:
    bool validateChecksum(const QByteArray &sentence);
    RadarData parseRATTM(const QByteArray &sentence);

    QVector<RadarData> m_trackedTargets;
};

#endif // RADARDEVICE_H