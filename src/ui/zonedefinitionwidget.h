#ifndef ZONEDEFINITIONCONTROLLERWIDGET_H
#define ZONEDEFINITIONCONTROLLERWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QKeyEvent>
#include <QVariant> // For passing generic zone data
#include <cmath> // For std::min/max
#include <algorithm> // For std::min/max
#include <QScrollArea>
#include "../models/systemstatemodel.h" // Includes systemstatedata.h
#include "../ui/zonemapwidget.h" // Include ZoneMapWidget header
#include "../ui/areazoneparameterpanel.h" // Include AreaZoneParameterPanel header
#include "../ui/sectorscanparameterpanel.h"
#include "../ui/trpparameterpanel.h"
// Forward declare other panels if needed
// class SectorScanParameterPanel;
// class TRPParameterPanel;

// Forward declaration of ZoneDefinitionWidget if used
// class ZoneDefinitionWidget;

class ZoneDefinitionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ZoneDefinitionWidget(SystemStateModel* model, QWidget *parent = nullptr);
    ~ZoneDefinitionWidget() override;

    // Public methods for MainWindow/Input Handling
    void handleUpNavigation();
    void handleDownNavigation();
    void handleSelectAction();
    void handleBackAction();

signals:
    void widgetClosed(); // To notify MainWindow
    // Signal to potentially delegate detailed steps to ZoneDefinitionWidget
    // void requestZoneDefinitionStep(ZoneType type, const QVariant& wipData, QString instruction);

protected:
    void keyPressEvent(QKeyEvent *event) override; // Keep for testing
    void showEvent(QShowEvent *event) override;

private:
    // --- Controller States ---
    enum class ControllerState {
        Idle_MainMenu,              // "New Zone", "Modify", "Delete", "Return"
        Select_ZoneType_ForNew,     // "Safety", "No-Traverse", "No-Fire", "Sector Scan", "TRP"
        
        // NOUVEAUX ÉTATS pour sélection du type avant modification/suppression
        Select_ZoneType_ForModify,  // "Area Zone", "Sector Scan", "TRP", "Back"
        Select_ZoneType_ForDelete,  // "Area Zone", "Sector Scan", "TRP", "Back"
        
        Select_AreaZone_ToModify,
        Select_SectorScan_ToModify,
        Select_TRP_ToModify,

        Select_AreaZone_ToDelete,
        Select_SectorScan_ToDelete,
        Select_TRP_ToDelete,

        // Defining/Modifying AreaZone (Option B: Two Corners)
        AreaZone_Aim_Corner1,       // User aims for Corner 1 (Az1, El1)
        AreaZone_Aim_Corner2,       // User aims for Corner 2 (Az2, El2)
        AreaZone_Edit_Parameters,   // User edits Enabled, Overridable via AreaZoneParameterPanel

        // Defining/Modifying AutoSectorScanZone
        SectorScan_Aim_Point1,      // User aims for Point 1 (Az1, El1)
        SectorScan_Aim_Point2,      // User aims for Point 2 (Az2, El2)
        SectorScan_Edit_Parameters, // User edits Scan Speed, Enabled via SectorScanParameterPanel

        // Defining/Modifying TargetReferencePoint
        TRP_Aim_Point,              // User aims for TRP position (Az, El)
        TRP_Edit_Parameters,        // User edits Page, Index, Halt Time via TRPParameterPanel

        Confirm_Save,               // "Save Changes? Yes/No"
        Confirm_Delete,             // "Delete Zone? Yes/No"
        Show_Message                // For errors or info
    };

    ControllerState m_currentState;
    SystemStateModel* m_stateModel; // Pointer to the central model

    // --- Work-In-Progress (WIP) Data ---
    ZoneType m_wipZoneType;
    AreaZone m_wipAreaZone;
    AutoSectorScanZone m_wipSectorScanZone;
    TargetReferencePoint m_wipTRP;
    int m_editingZoneId;    // -1 for new zone, or ID of zone being modified/deleted
    float m_currentGimbalAz; // Store locally for capture
    float m_currentGimbalEl; // Store locally for capture

    // Temporary storage for AreaZone corner definition
    float m_wipAz1, m_wipEl1, m_wipAz2, m_wipEl2;
    bool m_corner1Defined;

    // --- UI Components ---
    QScrollArea *m_scrollArea;
    QWidget *m_contentWidget;
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_instructionLabel;
    QLabel* m_liveGimbalAzLabel;
    QLabel* m_liveGimbalElLabel;

    QListWidget* m_mainMenuListWidget;    // For main menu, type selection, confirm yes/no
    QListWidget* m_zoneSelectionListWidget; // For selecting existing zone to modify/delete

    ZoneMapWidget* m_zoneMapWidget;         // Visual display of zones

    QStackedWidget* m_parameterPanelStack;         // Holds different parameter panels
    AreaZoneParameterPanel* m_areaZoneParameterPanel; // Panel for AreaZone properties
    SectorScanParameterPanel* m_sectorScanParameterPanel; // Placeholder
    TRPParameterPanel* m_trpParameterPanel;             // Placeholder

    // ZoneDefinitionWidget* m_zoneDefinitionWidget; // If used for detailed steps

    // --- Helper Members ---
    QStringList m_currentMenuItems; // Content for the active list widget
    int m_currentMenuIndex;
    QString m_currentColorStyle; // Keep track of current style

    // --- Private Methods ---
    void initializeUI();
    void connectModelSignals();
    void updateUI(); // Main UI refresh function
    void resetWipDataAndState();

    // UI Update Helpers
    void setupIdleMainMenuUI();
    void setupSelectZoneTypeUI();
    void setupSelectExistingZoneUI(ZoneType typeToSelect, const QString& title);
    void setupAimPointUI(const QString& instructionText);
    void setupAreaZoneParametersUI(bool isNew);
    void setupSectorScanParametersUI(bool isNew); // Placeholder
    void setupTRPParametersUI(bool isNew);       // Placeholder
    void setupConfirmUI(const QString& title, const QString& question);
    void setupShowMessageUI(const QString& message);
    void setupSelectZoneTypeForModifyDeleteUI(const QString& action);
    void processSelectZoneTypeForModifyDeleteSelect();

    // Action Processors (called by handleSelectAction)
    void processMainMenuSelect();
    void processSelectZoneTypeSelect();
    void processSelectExistingZoneSelect();
    void processAimPointConfirm(); // Generic handler for point capture
    void processEditParametersConfirm(); // Generic handler for panel confirmation
    void processConfirmSaveSelect();
    void processConfirmDeleteSelect();

    // Parameter Panel Navigation Helpers
    void activateNextParameterField();
    void activatePreviousParameterField();
    void handleParameterPanelInput(bool isUp); // Handles UP/DOWN for active field

    // WIP Zone Update for Map
    void updateMapWipZone();

    // Helper to get string representation of ZoneType
    QString zoneTypeToString(ZoneType type);

    // Helper to calculate AreaZone geometry from corners
    void calculateAreaZoneGeometry();
    // Helper to normalize azimuth
    float normalizeAzimuthTo360(float az) const;
    ZoneType m_deleteZoneType;
    

private slots:
    // Slots connected to SystemStateModel signals
    void onGimbalPositionChanged(float az, float el);
    void onSystemStateChanged(const SystemStateData &data); // For general updates if needed
    void onColorStyleChanged(const QColor &style);
    void onZonesChanged(); // To update zone selection lists

    // Slots connected to Parameter Panels (example)
    void onAreaZonePanelChanged();
    void onSectorScanPanelChanged();
    void onTRPPanelChanged();

    // Slots connected to ZoneDefinitionWidget (if used)
    // void onZoneDefinitionStepComplete(...);
};

#endif // ZONEDEFINITIONCONTROLLERWIDGET_H
