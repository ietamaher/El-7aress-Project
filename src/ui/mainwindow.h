#ifndef MAINWINDOW_H


#define MAINWINDOW_H

// Qt Includes
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QTimer>
#include <QListWidget>
#include <QProcess>
#include <QWidget>
#include <QPointer>
#include <QImage>
#include <QSet>
#include <QSlider>
#include <QLabel>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlError>
#include <QUrl>
#include "videoimageprovider.h"
#include "../viewmodels/osdviewmodel.h"

// Forward Declarations (Controllers, Models, etc.)
class GimbalController;
class WeaponController;
class CameraController;
class JoystickController;
 
class SystemStateModel;
class CameraVideoStreamDevice;
class OsdRenderer;
class CustomMenuWidget;
class SystemStatusWidget;
class WindageWidget;
class ZeroingWidget;
class ZoneDefinitionWidget;
class VideoDisplayWidget;
class CameraContainerWidget;

// Project Includes
#include "../models/systemstatemodel.h" // Includes SystemStateData
#include "custommenudialog.h"
#include "systemstatuswidget.h"
#include "windagewidget.h"
#include "zeroingwidget.h"
#include "radartargetlistwidget.h"
#include "../ui/zonedefinitionwidget.h"
#include "../ui/videodisplaywidget.h"
#include "../ui/cameracontainerwidget.h"
#include "../devices/cameravideostreamdevice.h" // Includes FrameData
#include "../devices/osdrenderer.h"
#include "../utils/colorutils.h" // For color style conversions
// UI Namespace Forward Declaration
namespace Ui {
class MainWindow;
}

// Main Window Class Definition
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Constructor & Destructor
    explicit MainWindow(GimbalController *gimbal,
                        WeaponController *weapon,
                        CameraController *camera,
                         JoystickController *joystick,
                        SystemStateModel *stateModel,
                        CameraVideoStreamDevice *dayProcessor,
                        CameraVideoStreamDevice *nightProcessor,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
              // Slots for external connections or high-level actions if any

signals:
    // Signals emitted by MainWindow
    void trackSelectButtonPressed(); // Example signal

private slots:
    // UI Element Interactions (Buttons, etc.)
    void on_opmode_clicked();
    void on_fireOn_clicked();
    void on_fieOff_clicked(); // Typo in original? Should be fireOff?
    void on_mode_clicked();
    void on_track_clicked();
    void on_motion_clicked();
    void on_up_clicked();
    void on_down_clicked();
    void on_autotrack_clicked();
    void on_day_clicked();
    void on_night_clicked();
    void on_quit_clicked();
    void on_read_clicked(); // Alarm related
    void on_clear_clicked(); // Alarm related
    void on_toggleTrackingButton_clicked();
    void on_detection_clicked();
    void on_zoneDefinition_clicked();
    void on_dw_clicked(); // Assumed Down Switch related
    void on_val_clicked(); // Assumed Value/Select Switch related

    // System & Controller Event Handling
    void onCameraControllerStatus(const QString &message);
    void handleFrameData(const FrameData &data); // From VideoProcessors
    void onActiveCameraChanged(bool isDay);
    void onSystemStateChanged(const SystemStateData &newData);
    void onTrackSelectButtonPressed(); // Connected to signal?
    void onUpSwChanged();
    void onDownSwChanged();
    void onMenuValSwChanged();
    void onUpTrackChanged(bool state);
    void onDownTrackChanged(bool state);
    void onAlarmDetected(uint16_t alarmCode, const QString &description);
    void onAlarmCleared();
    void onAlarmHistoryRead(const QList<uint16_t> &alarmHistory);
    void onAlarmHistoryCleared();
    void onCameraStateChanged();
    void onSelectedTrackLost(int trackId);
    void onTrackedIdsUpdated(const QSet<int>& trackIds);
    void onTrackIdSelected(QListWidgetItem *current, QListWidgetItem *previous);

    // Menu & Widget Management
    void showIdleMenu();
    void handleMenuClosed();
    void showSystemStatus();
    void personalizeReticle();
    void personalizeColor();
    void adjustBrightness();
    void showZoneDefinitionScreen();
    void handleZoneDefinitionClosed();
    void showZeroingWidget();
    void handleZeroingClosed();
    void showWindageWidget();
    void handleWindageClosed();
    void showHelpAbout();
    void handleMenuOptionSelected(const QString &option);
    void handleClearAlarmsRequest();
    void showRadarTargetWidget();
    void onModalWidgetClosed();
    // Timer Callbacks
    void processPendingUpdates(); // For track list updates
    // Add slot for statusTimer if needed

    void on_LeadAngle_clicked();

    void on_pushButton_clicked();

private:
    // Helper Methods
    void initializeUI();
    void setupCameraDisplays();
    void updateUIForActiveCamera();
    void switchCameraWidget(); // Manages the QStackedWidget
    void closeAppAndHardware();
    int findItemIndexByData(QListWidget* listWidget, int data) const;
    ReticleType stringToReticleType(const QString& style);
    QString reticleTypeToString(ReticleType type);
    QString colorStyleToString(ColorStyle style);
    void setTracklistColorStyle(const QString &style);
    void testBothDisplays(); // Debug/Test method

    // Brightness Control Helpers
    void setBrightness(int percentage);
    void increaseBrightness();
    void decreaseBrightness();
    bool detectDisplays();

    // Member Variables
    // UI
    Ui::MainWindow *ui;
    QVBoxLayout *m_layout; // Main layout, if used directly
    QStackedWidget* m_displayStack;
    CameraContainerWidget* m_cameraContainer;
    QWidget *m_dayWidgetPlaceholder;
    QWidget *m_nightWidgetPlaceholder;
    VideoDisplayWidget *m_currentDisplayWidget; // Pointer to the active display in the stack

    // Controllers
    GimbalController *m_gimbalCtrl;
    WeaponController *m_weaponCtrl;
    CameraController *m_cameraCtrl;
    JoystickController *m_joystickCtrl;

    // State Management
 
    SystemStateModel *m_stateModel;
    SystemStateData m_oldState; // Store previous state

    // Video Processing & OSD
    QPointer<CameraVideoStreamDevice> m_dayProcessor;
    QPointer<CameraVideoStreamDevice> m_nightProcessor;
    QPointer<OsdRenderer> m_osdRenderer_day;
    QPointer<OsdRenderer> m_osdRenderer_night;

    // State Flags
    bool m_isDayCameraActive;
    int m_activeCameraIndex; // 0 for day, 1 for night?
    bool m_menuActive;
    bool m_reticleMenuActive;
    bool m_colorMenuActive;
    bool m_systemStatusActive;
    bool m_settingsMenuActive;
    bool m_aboutActive;
    bool m_trackingActive_cam1;
    bool m_trackingActive_cam2;
    bool m_detectionActive_cam1;
    bool m_detectionActive_cam2;
    bool m_brightnessControlActive;

    // Pointers to Managed Widgets (Menus, Dialogs, etc.)
    CustomMenuWidget *m_menuWidget;
    CustomMenuWidget *m_reticleMenuWidget;
    CustomMenuWidget *m_colorMenuWidget;
    SystemStatusWidget *m_systemStatusWidget;
    ZoneDefinitionWidget *m_zoneDefinitionControllerWidget;
    ZeroingWidget* m_zeroingWidget;
    WindageWidget* m_windageWidget;
    RadarTargetListWidget* m_radarWidget;
    CustomMenuWidget *m_settingsMenuWidget;
    CustomMenuWidget *m_aboutWidget;
    QWidget* m_brightnessMenuWidget; // Container for brightness controls
    QSlider* m_brightnessSlider;
    QLabel* m_brightnessLabel;

    // Timers & Update Handling
    QTimer *updateTimer; // For track list updates?
    QTimer *statusTimer; // Purpose?
    QSet<int> pendingTrackIds;
    bool updatePending;

    // Brightness Control State
    int m_currentBrightness;
    QString m_displayOutput; // Target display for brightness
    qint64 m_lastTrackButtonPressTime = 0; // For debouncing track button presses
    static constexpr int DOUBLE_CLICK_INTERVAL_MS = 1000; // 300ms for double-click

    // QML Integration
    QQuickView *m_qmlView;
    QWidget *m_qmlContainer;
    VideoImageProvider *m_videoImageProvider;
    OsdViewModel *m_osdViewModel;

protected:
    // Override to handle window resize for QML container
    void resizeEvent(QResizeEvent *event) override;
};

#endif // MAINWINDOW_H


