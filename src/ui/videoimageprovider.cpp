#include "videoimageprovider.h"
#include <QDebug>
#include <QMutexLocker>

VideoImageProvider::VideoImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Create a placeholder black frame
    m_currentFrame = QImage(1024, 768, QImage::Format_RGB888);
    m_currentFrame.fill(Qt::black);

    qDebug() << "VideoImageProvider initialized with" << m_currentFrame.size();
}

QImage VideoImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize);

    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_currentFrame.size();
    }

    // Return a copy to avoid threading issues
    return m_currentFrame.copy();
}

void VideoImageProvider::updateFrame(const QImage &frame)
{
    if (frame.isNull()) {
        qWarning() << "VideoImageProvider::updateFrame - Received null frame";
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_currentFrame = frame.copy();
}
