#ifndef SYSTEMSTATUSWIDGET_H
#define SYSTEMSTATUSWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout> // Added for two-column layout
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>   // Added for sectioning
#include "basestyledwidget.h"

class SystemStateModel;
struct SystemStateData;

class SystemStatusWidget : public BaseStyledWidget
{
    Q_OBJECT
public:
    explicit SystemStatusWidget(SystemStateModel *stateModel, QWidget *parent = nullptr);
    ~SystemStatusWidget() override;

    void moveSelectionUp();
    void moveSelectionDown();
    void selectCurrentItem();
    //void setColorStyleChanged(const QColor &style); // Keep this

signals:
    void menuClosed();
    void clearAlarmsRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onSystemStateChanged(const SystemStateData &data);
    void onClearAlarmsClicked();
    //void onColorStyleChanged(const QColor &style); // Keep this

private:
    void populateData(const SystemStateData &data);
    void setupUi();
    void focusNextItem(bool forward);

    SystemStateModel *m_stateModel = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    // QHBoxLayout *m_mainColumnsLayout = nullptr; // This will be the layout of m_contentWidget

    // --- Left Column UI Elements ---
    // Gimbal Group
    QLabel *m_azimuthLabelValue = nullptr;
    QLabel *m_elevationLabelValue = nullptr;
    QLabel *m_azDriverTempLabelValue = nullptr;
    QLabel *m_azMotorTempLabelValue = nullptr; // From your code
    QLabel *m_elDriverTempLabelValue = nullptr;
    QLabel *m_elMotorTempLabelValue = nullptr; // From your code
    // LRF Group
    QLabel *m_lrfDistanceLabelValue = nullptr; // Example for LRF
    QLabel *m_lrfStatusLabelValue = nullptr;   // Example for LRF
    // Alarms Group
    QListWidget *m_alarmListWidget = nullptr;
    QPushButton *m_clearAlarmsButton = nullptr;

    // --- Right Column UI Elements ---
    // Camera Group
    QLabel *m_activeCameraLabelValue = nullptr;
    QLabel *m_dayCamFovLabelValue = nullptr;
    QLabel *m_nightCamFovLabelValue = nullptr;
    QLabel *m_nightCamConnectedLabelValue = nullptr;
    QLabel *m_dayCamConnectedLabelValue = nullptr;
    QLabel *m_dayCamStatusLabelValue = nullptr;
    QLabel *m_nightCamStatusLabelValue = nullptr;
    // PLC Group
    QLabel *m_plcStationEnabledLabelValue = nullptr;
    QLabel *m_plcGunArmedLabelValue = nullptr;
    // ... add more labels for PLC and other right-column items

    // --- Bottom Buttons ---
    QPushButton *m_returnButton = nullptr;

    // Navigation
    QList<QWidget*> m_focusableItems;
    int m_currentFocusIndex = -1;
    QString m_currentColorStyle = "Green"; // Default or get from model
};

#endif // SYSTEMSTATUSWIDGET_H
