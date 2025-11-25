#ifndef SYSTEMSTATUSCONTROLLER_H
#define SYSTEMSTATUSCONTROLLER_H

#include <QObject>
#include <QElapsedTimer>

class SystemStatusViewModel;
class SystemStateModel;
class SystemStateData;

class SystemStatusController : public QObject
{
    Q_OBJECT

public:
    explicit SystemStatusController(QObject *parent = nullptr);

    void setViewModel(SystemStatusViewModel* viewModel);
    void setStateModel(SystemStateModel* stateModel);
    void initialize();

    void show();
    void hide();

public slots:
    // Button handlers
    void onSelectButtonPressed();
    void onBackButtonPressed();
    void onUpButtonPressed();
    void onDownButtonPressed();

signals:
    void returnToMainMenu();
    void menuFinished();
    void clearAlarmsSignal();

private slots:
    void onSystemStateChanged(const SystemStateData& data);
    void onClearAlarmsRequested();
    void onColorStyleChanged(const QColor& color);

private:
    QStringList buildAlarmsList(const SystemStateData& data);
    void updateUI();

    SystemStatusViewModel* m_viewModel;
    SystemStateModel* m_stateModel;

    // Update throttling (0.5 second intervals to prevent memory leak)
    QElapsedTimer m_updateThrottleTimer;
    static constexpr qint64 UPDATE_INTERVAL_MS = 500;  // 0.5 seconds = 2 Hz
};

#endif // SYSTEMSTATUSCONTROLLER_H
