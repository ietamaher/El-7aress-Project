// TimestampLogger.h
#ifndef TIMESTAMPLOGGER_H
#define TIMESTAMPLOGGER_H

#include <QDebug>
#include <QDateTime>
#include <QElapsedTimer>
#include <QString>
#include <QMap>
#include <QMutex>

class TimestampLogger {
public:
    // Singleton pattern
    static TimestampLogger& instance() {
        static TimestampLogger instance;
        return instance;
    }

    // Start timer for a specific component
    void startTimer(const QString& componentName) {
        QMutexLocker locker(&m_mutex);
        if (!m_timers.contains(componentName)) {
            m_timers[componentName] = QElapsedTimer();
        }
        m_timers[componentName].start();
        
        qDebug() << "[" << QDateTime::currentMSecsSinceEpoch() << "] " 
                 << componentName << ": Timer started";
    }

    // Log timestamp for a specific component with operation
    void logTimestamp(const QString& componentName, const QString& operation) {
        qDebug() << "[" << QDateTime::currentMSecsSinceEpoch() << "] " 
                 << componentName << " - " << operation;
    }

    // Log elapsed time since timer start for a component
    void logElapsed(const QString& componentName, const QString& operation) {
        QMutexLocker locker(&m_mutex);
        if (m_timers.contains(componentName)) {
            qDebug() << "[" << QDateTime::currentMSecsSinceEpoch() << "] " 
                     << componentName << " - " << operation 
                     << " - Elapsed: " << m_timers[componentName].elapsed() << "ms";
        } else {
            qDebug() << "[" << QDateTime::currentMSecsSinceEpoch() << "] " 
                     << componentName << " - " << operation 
                     << " - No timer started";
        }
    }

    // Track transition between components
    void logTransition(const QString& fromComponent, const QString& toComponent, const QString& operation = "data transfer") {
        qDebug() << "[" << QDateTime::currentMSecsSinceEpoch() << "] " 
                 << "Transition: " << fromComponent << " -> " << toComponent 
                 << " - " << operation;
        
        QMutexLocker locker(&m_mutex);
        if (m_timers.contains(fromComponent)) {
            qDebug() << "    Elapsed in " << fromComponent << ": " 
                     << m_timers[fromComponent].elapsed() << "ms";
        }
    }

private:
    TimestampLogger() {} // Private constructor for singleton
    TimestampLogger(const TimestampLogger&) = delete;
    TimestampLogger& operator=(const TimestampLogger&) = delete;
    
    QMap<QString, QElapsedTimer> m_timers;
    QMutex m_mutex;
};

// Convenience macro for logging timestamps
#define LOG_TS(component, operation) \
    TimestampLogger::instance().logTimestamp(component, operation)

// Convenience macro for starting timer
#define START_TS_TIMER(component) \
    TimestampLogger::instance().startTimer(component)

// Convenience macro for logging elapsed time
#define LOG_TS_ELAPSED(component, operation) \
    TimestampLogger::instance().logElapsed(component, operation)

// Convenience macro for logging transitions
#define LOG_TS_TRANSITION(from, to, operation) \
    TimestampLogger::instance().logTransition(from, to, operation)

#endif // TIMESTAMPLOGGER_H