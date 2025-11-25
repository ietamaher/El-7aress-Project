#include "videoimageprovider.h"
#include <QDebug>
#include <QMutexLocker>

VideoImageProvider::VideoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Create a placeholder with a test pattern (RED for visibility)
    m_currentFrame = QImage(1024, 768, QImage::Format_RGB888);
    m_currentFrame.fill(Qt::red); // RED = test pattern visible, waiting for video

    qDebug() << "VideoImageProvider initialized with TEST PATTERN (RED) - size:" << m_currentFrame.size();
}

QImage VideoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);

    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_currentFrame.size();
    }

    // Return using implicit sharing (copy-on-write) - no deep copy unless modified
    // This saves ~2.3 MB per request × 30 fps = ~70 MB/sec
    return m_currentFrame;
}

void VideoImageProvider::updateFrame(const QImage &frame)
{
    if (frame.isNull()) {
        qWarning() << "VideoImageProvider::updateFrame - Received null frame";
        return;
    }

    // DEBUG: Log frame format to detect conversions
    static int frameCount = 0;
    static QImage::Format lastFormat = QImage::Format_Invalid;
    if (frame.format() != lastFormat || frameCount % 300 == 0) {
        qDebug() << "[VideoImageProvider] Frame format:" << frame.format()
                 << "Size:" << frame.size() << "Count:" << frameCount;
        lastFormat = frame.format();
    }
    frameCount++;

    QMutexLocker locker(&m_mutex);

    // Ensure the frame is in a QML-compatible format
    if (frame.format() == QImage::Format_RGB888 ||
        frame.format() == QImage::Format_ARGB32 ||
        frame.format() == QImage::Format_ARGB32_Premultiplied) {
        // Use implicit sharing (no deep copy unless frame is modified elsewhere)
        m_currentFrame = frame;
    } else {
        // ⚠️ PERFORMANCE: This creates a NEW QImage (potential leak if called frequently)
        qWarning() << "[VideoImageProvider] Converting frame from format" << frame.format() << "to RGB888";
        m_currentFrame = frame.convertToFormat(QImage::Format_RGB888);
    }
}
