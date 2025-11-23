#ifndef VIDEOIMAGEPROVIDER_H
#define VIDEOIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

/**
 * @brief Provides video frames to QML Image elements
 *
 * This class acts as a bridge between C++ video processing
 * and QML rendering. It implements QQuickImageProvider to
 * supply frames on-demand to QML Image components.
 *
 * Usage in QML:
 *   Image {
 *       source: "image://video/camera?" + Date.now()
 *       cache: false
 *   }
 */
class VideoImageProvider : public QQuickImageProvider
{
public:
    VideoImageProvider();
    ~VideoImageProvider() override = default;

    /**
     * @brief Called by QML when an image is requested
     * @param id The identifier from the source URL (e.g., "camera")
     * @param size Output parameter for the image size
     * @param requestedSize Requested size (usually ignored for video)
     * @return The current video frame
     */
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    /**
     * @brief Updates the current video frame
     * @param frame The new frame to display
     *
     * Thread-safe: Can be called from video processing threads
     */
    void updateFrame(const QImage &frame);

private:
    QMutex m_mutex;          ///< Protects m_currentFrame
    QImage m_currentFrame;   ///< The current video frame
};

#endif // VIDEOIMAGEPROVIDER_H
