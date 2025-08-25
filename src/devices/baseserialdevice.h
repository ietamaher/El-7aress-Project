#ifndef BASESERIALDEVICE_H
#define BASESERIALDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QMutex>

class BaseSerialDevice : public QObject
{
    Q_OBJECT

public:
    explicit BaseSerialDevice(QObject *parent = nullptr);
    virtual ~BaseSerialDevice();

    // Common interface
    virtual bool openSerialPort(const QString &portName);
    virtual void closeSerialPort();
    virtual void shutdown();
    virtual bool isConnected() const;

    // Connection state
    bool getConnectionState() const { return m_isConnected; }

protected:
    // Pure virtual methods that derived classes must implement
    virtual void configureSerialPort() = 0;  // Set baud rate, parity, etc.
    virtual void processIncomingData() = 0;   // Handle received data
    virtual void onConnectionEstablished() {} // Called after successful connection
    virtual void onConnectionLost() {}        // Called when connection is lost

    // Helper methods for derived classes
    void logMessage(const QString &message);
    void logError(const QString &message);
    void sendData(const QByteArray &data);
    bool waitForResponse(int timeoutMs = 1000);

    // Reconnection settings (can be overridden)
    virtual int getMaxReconnectAttempts() const { return 5; }
    virtual int getReconnectDelayMs(int attempt) const {
        return 1000 * (1 << attempt); // Exponential backoff
    }
    virtual bool shouldAttemptReconnection() const { return true; }

    // Data members accessible to derived classes
    QSerialPort *m_serialPort;
    QByteArray m_readBuffer;
    QString m_lastPortName;
    mutable QMutex m_mutex;

signals:
    void connectionStateChanged(bool connected);
    void logMessages(const QString &message);
    void errorOccurred(const QString &error);

private slots:
    void handleSerialError(QSerialPort::SerialPortError error);
    void attemptReconnection();
    void onSerialDataReady();

private:
    void setConnectionState(bool connected);

    bool m_isConnected;
    int m_reconnectAttempts;
    QTimer *m_reconnectTimer;

    static const int DEFAULT_MAX_RECONNECT_ATTEMPTS = 5;
};

#endif // BASESERIALDEVICE_H
