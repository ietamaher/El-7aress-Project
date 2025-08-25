#include "zonedefinitionwidget.h"
#include <QDebug>
#include <QMessageBox> // Consider replacing with custom message display

// Constructor and Destructor remain largely the same...
ZoneDefinitionWidget::ZoneDefinitionWidget(SystemStateModel* model, QWidget *parent)
    : QWidget(parent),
    m_currentState(ControllerState::Idle_MainMenu),
    m_stateModel(model),
    m_wipZoneType(ZoneType::None),
    m_editingZoneId(-1),
    m_corner1Defined(false),
    m_currentMenuIndex(0),
    m_currentGimbalAz(0.0f),
    m_currentGimbalEl(0.0f),
    m_currentColorStyle("Green") // Default
{
    Q_ASSERT(m_stateModel != nullptr);
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
     //setAttribute(Qt::WA_DeleteOnClose);
    // Transparent BG
    setStyleSheet("background-color: rgba(0, 0, 0, 150);");
    initializeUI();
    connectModelSignals();
    resetWipDataAndState(); // Initialize WIP data and UI state

    // Populate with initial data & color from model
    if (m_stateModel) {
        // populateData(m_stateModel->data()); // Removed, gimbal handled by signal

        connect(m_stateModel, &SystemStateModel::colorStyleChanged, // Assuming this signal exists
                this, &ZoneDefinitionWidget::onColorStyleChanged, Qt::QueuedConnection);

        onGimbalPositionChanged(m_stateModel->data().gimbalAz, m_stateModel->data().gimbalEl); // Initial pos
        onColorStyleChanged(m_stateModel->data().colorStyle);
        onZonesChanged(); // Populate lists initially if needed by Idle state
    } else {
        qWarning() << "ZoneDefinitionWidget created without a SystemStateModel!";
    }
}

ZoneDefinitionWidget::~ZoneDefinitionWidget()
{
    qDebug() << "ZoneDefinitionWidget destroyed";
}

// initializeUI remains the same (assuming AreaZoneParameterPanel is correctly instantiated)
void ZoneDefinitionWidget::initializeUI()
{
    // Définir une taille minimale pour le widget principal
    setMinimumSize(600, 550);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(5, 5, 5, 5);
    outerLayout->setSpacing(10);

    // Configurer le QScrollArea
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFocusPolicy(Qt::NoFocus);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    // Créer le widget de contenu avec une taille minimale
    m_contentWidget = new QWidget();
    m_contentWidget->setMinimumSize(580, 530); // Définir une taille minimale

    // Layout principal pour le contenu
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);

    // Title Section
    m_titleLabel = new QLabel("Zone Definition", m_contentWidget);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = font(); titleFont.setPointSize(16); titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_mainLayout->addWidget(m_titleLabel);

    m_instructionLabel = new QLabel("Instructions...", m_contentWidget);
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setWordWrap(true);
    m_mainLayout->addWidget(m_instructionLabel);

    // Live Gimbal Data Display
    QHBoxLayout* gimbalDisplayLayout = new QHBoxLayout();
    m_liveGimbalAzLabel = new QLabel("Az: ---.-", m_contentWidget);
    m_liveGimbalElLabel = new QLabel("El: ---.-", m_contentWidget);
    gimbalDisplayLayout->addWidget(new QLabel("WS Pos:", m_contentWidget));
    gimbalDisplayLayout->addWidget(m_liveGimbalAzLabel);
    gimbalDisplayLayout->addStretch();
    gimbalDisplayLayout->addWidget(m_liveGimbalElLabel);
    m_mainLayout->addLayout(gimbalDisplayLayout);

    // Menu List (for main menu, type selection, zone selection, confirm)
    m_mainMenuListWidget = new QListWidget(m_contentWidget);
    m_mainLayout->addWidget(m_mainMenuListWidget);

    m_zoneSelectionListWidget = new QListWidget(m_contentWidget); // For modify/delete
    m_mainLayout->addWidget(m_zoneSelectionListWidget);
    m_zoneSelectionListWidget->setVisible(false); // Hidden initially

    // Zone Map
    m_zoneMapWidget = new ZoneMapWidget(m_stateModel, m_contentWidget);
    m_zoneMapWidget->setMinimumHeight(200); // Give it some space
    m_mainLayout->addWidget(m_zoneMapWidget);

    // Parameter Panels Stack
    m_parameterPanelStack = new QStackedWidget(m_contentWidget);
    m_parameterPanelStack->setStyleSheet("QStackedWidget { background-color: rgba(30, 30, 30, 150); }");
    m_parameterPanelStack->setMaximumHeight(180); // Limit height for parameter panels
    m_areaZoneParameterPanel = new AreaZoneParameterPanel(m_contentWidget); // Assumes constructor is updated
    m_parameterPanelStack->addWidget(m_areaZoneParameterPanel);

    m_sectorScanParameterPanel = new SectorScanParameterPanel(m_contentWidget);
    m_parameterPanelStack->addWidget(m_sectorScanParameterPanel);

    m_trpParameterPanel = new TRPParameterPanel(m_contentWidget);
    m_parameterPanelStack->addWidget(m_trpParameterPanel);

    m_mainLayout->addWidget(m_parameterPanelStack);
    m_parameterPanelStack->setVisible(false); // Hidden initially

    // ZoneDefinitionWidget (if used)
    // m_zoneDefinitionWidget = new ZoneDefinitionWidget(m_stateModel, this);
    // m_mainLayout->addWidget(m_zoneDefinitionWidget);
    // m_zoneDefinitionWidget->setVisible(false);

    // Définir le layout sur le widget de contenu
    m_contentWidget->setLayout(m_mainLayout);

    // Ajouter le widget de contenu au QScrollArea
    m_scrollArea->setWidget(m_contentWidget);

    // Ajouter le QScrollArea au layout principal
    outerLayout->addWidget(m_scrollArea);

    // Définir le layout sur le widget principal
    setLayout(outerLayout);

    move(5, 180);
}

// connectModelSignals remains the same
void ZoneDefinitionWidget::connectModelSignals() {
    if (!m_stateModel) return;

    // Connect mandatory signals
    connect(m_stateModel, &SystemStateModel::gimbalPositionChanged,
            this, &ZoneDefinitionWidget::onGimbalPositionChanged);
    connect(m_stateModel, &SystemStateModel::zonesChanged,
            this, &ZoneDefinitionWidget::onZonesChanged);
    connect(m_stateModel, &SystemStateModel::colorStyleChanged,
            this, &ZoneDefinitionWidget::onColorStyleChanged);

    // Optional: Connect dataChanged if needed for other state updates
    // connect(m_stateModel, &SystemStateModel::dataChanged,
    //         this, &ZoneDefinitionWidget::onSystemStateChanged);

    // Connect signals from parameter panels if they exist and emit changes
    if (m_areaZoneParameterPanel) {
        connect(m_areaZoneParameterPanel, &AreaZoneParameterPanel::parametersChanged,
                this, &ZoneDefinitionWidget::onAreaZonePanelChanged);
    }

    if (m_sectorScanParameterPanel) {
        connect(m_sectorScanParameterPanel, &SectorScanParameterPanel::parametersChanged,
                this, &ZoneDefinitionWidget::onSectorScanPanelChanged);
    }
    if (m_trpParameterPanel) {
        connect(m_trpParameterPanel, &TRPParameterPanel::parametersChanged,
                this, &ZoneDefinitionWidget::onTRPPanelChanged);
    }
}

// showEvent remains the same
void ZoneDefinitionWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_currentState = ControllerState::Idle_MainMenu;
    resetWipDataAndState();
    updateUI();
    this->setFocus(); // Set focus to the controller widget itself initially
}

// resetWipDataAndState needs to reset corner flags
void ZoneDefinitionWidget::resetWipDataAndState() {
    m_wipZoneType = ZoneType::None;
    m_wipAreaZone = AreaZone{}; // Default construct
    m_wipSectorScanZone = AutoSectorScanZone{};
    m_wipTRP = TargetReferencePoint{};
    m_editingZoneId = -1;
    m_corner1Defined = false; // Reset corner flag
    m_wipAz1 = m_wipEl1 = m_wipAz2 = m_wipEl2 = 0.0f; // Reset corner coords
    m_currentMenuIndex = 0;
    if (m_zoneMapWidget) {
        m_zoneMapWidget->clearWipZone();
        m_zoneMapWidget->highlightZone(-1); // Clear highlighting
    }
    // Reset parameter panels if they exist
    if (m_areaZoneParameterPanel) m_areaZoneParameterPanel->setActiveField(AreaZoneParameterPanel::Field::None);
    if (m_sectorScanParameterPanel) m_sectorScanParameterPanel->setActiveField(SectorScanParameterPanel::Field::None);
    if (m_trpParameterPanel) m_trpParameterPanel->setActiveField(TRPParameterPanel::Field::None);
}

// onGimbalPositionChanged needs to update WIP map for AreaZone aiming states
void ZoneDefinitionWidget::onGimbalPositionChanged(float az, float el)
{
    m_currentGimbalAz = az;
    m_currentGimbalEl = el;
    m_liveGimbalAzLabel->setText(QString("Az: %1 deg").arg(az, 0, 'f', 1));
    m_liveGimbalElLabel->setText(QString("El: %1 deg").arg(el, 0, 'f', 1));

    // Update map's gimbal indicator directly
    if (m_zoneMapWidget) {
        m_zoneMapWidget->updateGimbalPosition(az, el);
    }

    // If in an aiming state, update the WIP zone on the map
    switch (m_currentState) {
        case ControllerState::AreaZone_Aim_Corner1: // Update WIP based on current gimbal
        case ControllerState::AreaZone_Aim_Corner2: // Update WIP based on corner1 and current gimbal
        case ControllerState::SectorScan_Aim_Point1:
        case ControllerState::SectorScan_Aim_Point2:
        case ControllerState::TRP_Aim_Point:
            updateMapWipZone(); // Update visual feedback during aiming
            break;
        default:
            break;
    }
}

// onSystemStateChanged, onColorStyleChanged, onZonesChanged remain the same
void ZoneDefinitionWidget::onSystemStateChanged(const SystemStateData &data)
{
    // Likely not needed if specific signals cover updates
}

void ZoneDefinitionWidget::onColorStyleChanged(const QColor &style)
{
     // Determine color style string
    QString colorStyleString;
    if (style == QColor(70, 226, 165)) {
        m_currentColorStyle = "Green";
        colorStyleString = "Green";
    } else if (style == QColor(200,20,40)) {
        m_currentColorStyle = "Red";
        colorStyleString = "Red";
    } else if (style == Qt::white) {
        m_currentColorStyle = "White";
        colorStyleString = "White";
    } else {
        m_currentColorStyle = "Green"; // Default
        colorStyleString = "Green";
    }


    QString baseStyle = "background-color: rgba(0,0,0,150); font: 600 14pt 'Archivo Narrow';";
    QString buttonStyle;
    QString listStyle;
    QString labelStyle;
    QString groupBoxStyle;
    QString spinBoxStyle; // Add style for spinboxes if needed
    QString checkBoxStyle; // Add style for checkboxes if needed

    if (m_currentColorStyle == "Red") {
        baseStyle += "color: rgba(200,20,40,255);";
        buttonStyle = "QPushButton {" + baseStyle + "border: 1px solid rgba(200,20,40,255);}"
                      "QPushButton:focus {background-color: rgba(200,20,40,255); color: white; border: 1px solid white;}";

        listStyle = "QListWidget {" + baseStyle + "}"
                    "QListWidget::item:selected {color: white; background: rgba(200,20,40,255); border: 1px solid white;}";
        labelStyle = "QLabel {" + baseStyle + "}";
        groupBoxStyle = "QGroupBox {" + baseStyle + "border: 1px solid rgba(200,20,40,255); margin-top: 1ex;}"
                        "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else if (m_currentColorStyle == "Green") {
        baseStyle += "color: rgba(70, 226, 165,255);";
        buttonStyle = "QPushButton {" + baseStyle + "border: 1px solid rgba(70, 226, 165,255);}"
                      "QPushButton:focus {background-color: rgba(70, 226, 165,255); color: white; border: 1px solid white;}";
        listStyle = "QListWidget {" + baseStyle + "}"
                    "QListWidget::item:selected {color: white; background: rgba(70, 226, 165,255); border: 1px solid white;}";
        labelStyle = "QLabel {" + baseStyle + "}";
        groupBoxStyle = "QGroupBox {" + baseStyle + "border: 1px solid rgba(70, 226, 165,255); margin-top: 1ex;}"
                        "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else if (m_currentColorStyle == "White") {
        baseStyle += "color: rgba(255,255,255,255);";
        buttonStyle = "QPushButton {" + baseStyle + "border: 1px solid rgba(255,255,255,255);}"
                      "QPushButton:focus {background-color: rgba(255,255,255,255); color: rgba(0,0,0,255); border: 1px solid white;}";
        listStyle = "QListWidget {" + baseStyle + "}"
                    "QListWidget::item:selected {background: rgba(255,255,255,255); color: rgba(0,0,0,255); border: 1px solid white;}";
        labelStyle = "QLabel {" + baseStyle + "}";
        groupBoxStyle = "QGroupBox {" + baseStyle + "border: 1px solid rgba(255,255,255,255); margin-top: 1ex;}"
                        "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else {
        // default - Green
        baseStyle += "color: rgba(70, 226, 165,255);";
        buttonStyle = "QPushButton {" + baseStyle + "border: 1px solid rgba(70, 226, 165,255);}"
                      "QPushButton:focus {background-color: rgba(70, 226, 165,255); color: white; border: 1px solid white;}";
        listStyle = "QListWidget {" + baseStyle + "}"
                    "QListWidget::item:selected {color: white; background: rgba(70, 226, 165,255); border: 1px solid white;}";
        labelStyle = "QLabel {" + baseStyle + "}";
        groupBoxStyle = "QGroupBox {" + baseStyle + "border: 1px solid rgba(70, 226, 165,255); margin-top: 1ex;}"
                        "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }

    // Common styles independent of color (or customize per color)
    labelStyle = "QLabel { border: none; " + baseStyle + "}";
    groupBoxStyle = "QGroupBox { border: 1px solid " + baseStyle.split(";")[1] + "; margin-top: 1ex;}" // Use color from baseStyle
                                                                                 "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    // Add styles for SpinBox, CheckBox if needed, potentially inheriting from baseStyle
    spinBoxStyle = "QDoubleSpinBox { border: 1px solid gray; " + baseStyle + "}";
    checkBoxStyle = "QCheckBox { border: none; " + baseStyle + "}";

    // Apply styles
     m_contentWidget->setStyleSheet(baseStyle);
    this->setStyleSheet(labelStyle + listStyle + groupBoxStyle + spinBoxStyle + checkBoxStyle + buttonStyle );
    // Apply specific styles to list widgets if needed beyond the global QListWidget style
    // m_mainMenuListWidget->setStyleSheet(listStyle);
    // m_zoneSelectionListWidget->setStyleSheet(listStyle);

    // Apply styles to parameter panels (or let them inherit)
    if(m_areaZoneParameterPanel) {
        m_areaZoneParameterPanel->updateColorTheme(colorStyleString);
    }

    if(m_sectorScanParameterPanel) {
        m_sectorScanParameterPanel->updateColorTheme(colorStyleString);
    }
    if(m_trpParameterPanel) {
        m_trpParameterPanel->updateColorTheme(colorStyleString);
    }
}

void ZoneDefinitionWidget::onZonesChanged()
{
    qDebug() << "ZoneDefinitionWidget: Received zonesChanged() signal.";
    // If currently in a state that shows a list of zones, refresh it
    switch (m_currentState) {
        case ControllerState::Select_AreaZone_ToModify:
            setupSelectExistingZoneUI(ZoneType::Safety, "Modify Area Zone"); // Assuming Safety covers all AreaZone types for selection
            break;
        case ControllerState::Select_SectorScan_ToModify:
            setupSelectExistingZoneUI(ZoneType::AutoSectorScan, "Modify Sector Scan Zone");
            break;
        case ControllerState::Select_TRP_ToModify:
            setupSelectExistingZoneUI(ZoneType::TargetReferencePoint, "Modify TRP");
            break;
        case ControllerState::Select_AreaZone_ToDelete:
            setupSelectExistingZoneUI(ZoneType::Safety, "Delete Area Zone"); // Assuming Safety covers all AreaZone types for selection
            break;
        case ControllerState::Select_SectorScan_ToDelete:
            setupSelectExistingZoneUI(ZoneType::AutoSectorScan, "Delete Sector Scan Zone");
            break;
        case ControllerState::Select_TRP_ToDelete:
            setupSelectExistingZoneUI(ZoneType::TargetReferencePoint, "Delete TRP");
            break;
        default:
            // In other states, no immediate action needed on zonesChanged,
            // but the map widget will update itself via its own connection.
            break;
    }
    // Refresh map explicitly if needed, though it should have its own connection
    if (m_zoneMapWidget) m_zoneMapWidget->refreshDisplay();
}

// onAreaZonePanelChanged updates WIP data from the modified panel
void ZoneDefinitionWidget::onAreaZonePanelChanged()
{
    if (m_currentState == ControllerState::AreaZone_Edit_Parameters && m_areaZoneParameterPanel) {
        // Update the WIP AreaZone with the current panel values (checkboxes)
        m_wipAreaZone = m_areaZoneParameterPanel->getZoneData(m_wipAreaZone);
        // No need to update map here, as only non-geometric parameters changed
        qDebug() << "AreaZone parameters updated in WIP: Enabled=" << m_wipAreaZone.isEnabled << "Overridable=" << m_wipAreaZone.isOverridable;
    }
}

void ZoneDefinitionWidget::onSectorScanPanelChanged()
{
    if (m_sectorScanParameterPanel) {
        m_wipSectorScanZone = m_sectorScanParameterPanel->getZoneData(m_wipSectorScanZone);
        updateMapWipZone();
    }
}

void ZoneDefinitionWidget::onTRPPanelChanged()
{
    if (m_trpParameterPanel) {
       // m_wipTRP = m_trpParameterPanel->getZoneData(m_wipTRP);
        updateMapWipZone();
    }
}

// --- UI Update Helpers ---

// setupIdleMainMenuUI, setupSelectZoneTypeUI, setupSelectExistingZoneUI remain the same
void ZoneDefinitionWidget::setupIdleMainMenuUI()
{
    m_titleLabel->setText("Zone Definition Menu");
    m_instructionLabel->setText("Select option using UP/DOWN, confirm with SELECT.");
    m_currentMenuItems = {"New Zone", "Modify Zone", "Delete Zone", "Return"};
    m_mainMenuListWidget->clear();
    m_mainMenuListWidget->addItems(m_currentMenuItems);
    m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
    m_mainMenuListWidget->setVisible(true);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    m_mainMenuListWidget->setFocus();
}

void ZoneDefinitionWidget::setupSelectZoneTypeUI()
{
    m_titleLabel->setText("Select Zone Type");
    m_instructionLabel->setText("Choose type to create.");
    // Assuming ZoneType enum order matches this list
    m_currentMenuItems = {"Safety Zone", "No-Traverse Zone", "No-Fire Zone", "Sector Scan", "Target Ref Point", "Back"};
    m_mainMenuListWidget->clear();
    m_mainMenuListWidget->addItems(m_currentMenuItems);
    m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
    m_mainMenuListWidget->setVisible(true);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    m_mainMenuListWidget->setFocus();
}

void ZoneDefinitionWidget::setupSelectExistingZoneUI(ZoneType typeToSelect, const QString& title)
{
    m_titleLabel->setText(title);
    m_instructionLabel->setText("Select zone using UP/DOWN, confirm with SELECT.");
    m_currentMenuItems.clear();
    m_zoneSelectionListWidget->clear();

    // Populate list based on typeToSelect
    if (typeToSelect == ZoneType::Safety || typeToSelect == ZoneType::NoFire || typeToSelect == ZoneType::NoTraverse) {
        const auto& zones = m_stateModel->getAreaZones();
        qDebug() << "setupSelectExistingZoneUI: Found" << zones.size() << "area zones";
        for (const auto& zone : zones) {
            // Optionally filter by specific type if needed, otherwise list all AreaZones
            QString itemText = QString("ID: %1 (%2) %3").arg(zone.id).arg(zoneTypeToString(zone.type)).arg(zone.isEnabled ? "Enabled" : "Disabled");
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id)); // Store ID for retrieval
        }
    } else if (typeToSelect == ZoneType::AutoSectorScan) {
        const auto& zones = m_stateModel->getSectorScanZones();
        qDebug() << "setupSelectExistingZoneUI: Found" << zones.size() << "sector scan zones";
        for (const auto& zone : zones) {
            QString itemText = QString("ID: %1 (Sector Scan) %2").arg(zone.id).arg(zone.isEnabled ? "Enabled" : "Disabled");
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id));
        }
    } else if (typeToSelect == ZoneType::TargetReferencePoint) {
        const auto& zones = m_stateModel->getTargetReferencePoints();
        qDebug() << "setupSelectExistingZoneUI: Found" << zones.size() << "TRP zones";
        for (const auto& zone : zones) {
            QString itemText = QString("ID: %1 (TRP) Page:%2 Idx:%3").arg(zone.id).arg(zone.locationPage).arg(zone.trpInPage);
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id));
        }
    }

    // Toujours ajouter "Back" même si aucune zone n'est trouvée
    m_zoneSelectionListWidget->addItem("Back");
    m_currentMenuItems.append("Back");

    // Si aucune zone n'est trouvée, afficher un message informatif
    if (m_currentMenuItems.size() == 1) { // Seulement "Back"
        QString noZoneMessage;
        if (typeToSelect == ZoneType::AutoSectorScan) {
            noZoneMessage = "No Sector Scan zones defined";
        } else if (typeToSelect == ZoneType::TargetReferencePoint) {
            noZoneMessage = "No TRP zones defined";
        } else {
            noZoneMessage = "No Area zones defined";
        }
        m_zoneSelectionListWidget->insertItem(0, noZoneMessage);
        m_currentMenuItems.insert(0, "NoZones");
    }

    m_zoneSelectionListWidget->setCurrentRow(m_currentMenuIndex);
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(true);
    m_parameterPanelStack->setVisible(false);
    m_zoneSelectionListWidget->setFocus();
}

void ZoneDefinitionWidget::setupSelectZoneTypeForModifyDeleteUI(const QString& action)
{
    m_titleLabel->setText(QString("%1 Zone - Select Type").arg(action));
    m_instructionLabel->setText("Select zone type using UP/DOWN, confirm with SELECT.");
    m_currentMenuItems.clear();
    m_mainMenuListWidget->clear();

    // Options de types de zones
    m_currentMenuItems << "Area Zone" << "Sector Scan" << "TRP" << "Back";

    for (const QString& item : m_currentMenuItems) {
        m_mainMenuListWidget->addItem(item);
    }

    m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
    m_mainMenuListWidget->setVisible(true);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    m_mainMenuListWidget->setFocus();
}

/*void ZoneDefinitionWidget::setupSelectExistingZoneUI(ZoneType typeToSelect, const QString& title)
{
    m_titleLabel->setText(title);
    m_instructionLabel->setText("Select zone using UP/DOWN, confirm with SELECT.");
    m_currentMenuItems.clear();
    m_zoneSelectionListWidget->clear();

    // Populate list based on typeToSelect
    if (typeToSelect == ZoneType::Safety || typeToSelect == ZoneType::NoFire || typeToSelect == ZoneType::NoTraverse) {
        const auto& zones = m_stateModel->getAreaZones();
        for (const auto& zone : zones) {
            // Optionally filter by specific type if needed, otherwise list all AreaZones
            QString itemText = QString("ID: %1 (%2) %3").arg(zone.id).arg(zoneTypeToString(zone.type)).arg(zone.isEnabled ? "Enabled" : "Disabled");
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id)); // Store ID for retrieval
        }
    } else if (typeToSelect == ZoneType::AutoSectorScan) {
        const auto& zones = m_stateModel->getSectorScanZones();
        for (const auto& zone : zones) {
            QString itemText = QString("ID: %1 (Sector Scan) %2").arg(zone.id).arg(zone.isEnabled ? "Enabled" : "Disabled");
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id));
        }
    } else if (typeToSelect == ZoneType::TargetReferencePoint) {
        const auto& zones = m_stateModel->getTargetReferencePoints();
        for (const auto& zone : zones) {
            QString itemText = QString("ID: %1 (TRP) Page:%2 Idx:%3").arg(zone.id).arg(zone.locationPage).arg(zone.trpInPage);
            m_zoneSelectionListWidget->addItem(itemText);
            m_currentMenuItems.append(QString::number(zone.id));
        }
    }
    m_zoneSelectionListWidget->addItem("Back");
    m_currentMenuItems.append("Back");

    m_zoneSelectionListWidget->setCurrentRow(m_currentMenuIndex);
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(true);
    m_parameterPanelStack->setVisible(false);
    m_zoneSelectionListWidget->setFocus();
}*/

// setupAimPointUI remains the same
void ZoneDefinitionWidget::setupAimPointUI(const QString& instructionText)
{
    m_titleLabel->setText("Define Zone Geometry");
    m_instructionLabel->setText(instructionText);
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    // Focus should remain on the controller to capture SELECT
    this->setFocus();
}

// setupAreaZoneParametersUI needs to update instruction text
void ZoneDefinitionWidget::setupAreaZoneParametersUI(bool isNewZone)
{
    qDebug() << "Setting up AreaZone parameters UI";
    m_titleLabel->setText(isNewZone ? "Set New Area Zone Parameters" : "Modify Area Zone Parameters");
    m_instructionLabel->setText("Configure area zone parameters using UP/DOWN to navigate, SELECT to toggle/confirm");
    
    // Set up the parameter panel with current zone data
    if (m_areaZoneParameterPanel) {
        m_areaZoneParameterPanel->setZoneData(m_wipAreaZone);
        m_areaZoneParameterPanel->setActiveField(AreaZoneParameterPanel::Field::Enabled);
    }

     // Afficher le bon panneau
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(true);
    m_parameterPanelStack->setCurrentWidget(m_areaZoneParameterPanel);

    if (m_areaZoneParameterPanel) {
        m_areaZoneParameterPanel->setFocus();
    }
    updateMapWipZone();

}
// setupSectorScanParametersUI, setupTRPParametersUI remain placeholders
void ZoneDefinitionWidget::setupSectorScanParametersUI(bool isNewZone)
{
    QString title = isNewZone ? "New Sector Scan Zone - Parameters" : "Modify Sector Scan Zone - Parameters";
    m_titleLabel->setText(title);
    m_instructionLabel->setText("Configure parameters using UP/DOWN, SELECT to edit values, BACK to confirm.");

    // Configurer le panneau avec les données WIP
    if (m_sectorScanParameterPanel) {
        m_sectorScanParameterPanel->setZoneData(m_wipSectorScanZone);
        m_sectorScanParameterPanel->setActiveField(SectorScanParameterPanel::Field::Enabled);
    }

    // Afficher le bon panneau
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(true);
    m_parameterPanelStack->setCurrentWidget(m_sectorScanParameterPanel);

    if (m_sectorScanParameterPanel) {
        m_sectorScanParameterPanel->setFocus();
    }
    updateMapWipZone();
}

void ZoneDefinitionWidget::setupTRPParametersUI(bool isNewZone)
{
    QString title = isNewZone ? "New TRP - Parameters" : "Modify TRP - Parameters";
    m_titleLabel->setText(title);
    m_instructionLabel->setText("Configure parameters using UP/DOWN, SELECT to edit values, BACK to confirm.");

    // Configurer le panneau avec les données WIP
    if (m_trpParameterPanel) {
        m_trpParameterPanel->setTRPData(m_wipTRP);
        m_trpParameterPanel->setActiveField(TRPParameterPanel::Field::LocationPage);
    }

    // Afficher le bon panneau
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(true);
    m_parameterPanelStack->setCurrentWidget(m_trpParameterPanel);

    if (m_trpParameterPanel) {
        m_trpParameterPanel->setFocus();
    }

    updateMapWipZone();
}


// setupConfirmUI, setupShowMessageUI remain the same
void ZoneDefinitionWidget::setupConfirmUI(const QString& title, const QString& question)
{
    m_titleLabel->setText(title);
    m_instructionLabel->setText(question);
    m_currentMenuItems = {"Yes", "No"};
    m_mainMenuListWidget->clear();
    m_mainMenuListWidget->addItems(m_currentMenuItems);
    m_mainMenuListWidget->setCurrentRow(0); // Default to Yes
    m_mainMenuListWidget->setVisible(true);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    m_mainMenuListWidget->setFocus();
}

void ZoneDefinitionWidget::setupShowMessageUI(const QString& message)
{
    m_titleLabel->setText("Information");
    m_instructionLabel->setText(message + "\nPress SELECT or BACK to return.");
    m_mainMenuListWidget->setVisible(false);
    m_zoneSelectionListWidget->setVisible(false);
    m_parameterPanelStack->setVisible(false);
    this->setFocus();
}

// --- Action Processors ---

// processMainMenuSelect, processSelectZoneTypeSelect, processSelectExistingZoneSelect need minor adjustments for state names
// !!!TODO add SCAN and TRP selection logic for modify/delete flows
void ZoneDefinitionWidget::processMainMenuSelect()
{
    QString selectedOption = m_currentMenuItems.value(m_currentMenuIndex);
    if (selectedOption == "New Zone") {
        m_currentState = ControllerState::Select_ZoneType_ForNew;
        m_currentMenuIndex = 0;
    } else if (selectedOption == "Modify Zone") {
        // CORRECTION : Aller d'abord à la sélection du type de zone à modifier
        m_currentState = ControllerState::Select_ZoneType_ForModify;
        m_currentMenuIndex = 0;
        qDebug() << "Modify Zone selected - going to zone type selection.";
    } else if (selectedOption == "Delete Zone") {
        // CORRECTION : Aller d'abord à la sélection du type de zone à supprimer
        m_currentState = ControllerState::Select_ZoneType_ForDelete;
        m_currentMenuIndex = 0;
        qDebug() << "Delete Zone selected - going to zone type selection.";
    } else if (selectedOption == "Return") {
        this->close();
        emit widgetClosed();
        return; // Don't updateUI
    }
    updateUI();
}

void ZoneDefinitionWidget::processSelectZoneTypeSelect()
{
    QString selectedType = m_currentMenuItems.value(m_currentMenuIndex);
    resetWipDataAndState(); // Clear previous WIP
    m_editingZoneId = -1; // Ensure it's a new zone

    if (selectedType == "Safety Zone") {
        m_wipZoneType = ZoneType::Safety;
        m_currentState = ControllerState::AreaZone_Aim_Corner1;
    } else if (selectedType == "No-Traverse Zone") {
        m_wipZoneType = ZoneType::NoTraverse;
        m_currentState = ControllerState::AreaZone_Aim_Corner1;
    } else if (selectedType == "No-Fire Zone") {
        m_wipZoneType = ZoneType::NoFire;
        m_currentState = ControllerState::AreaZone_Aim_Corner1;
    } else if (selectedType == "Sector Scan") {
        m_wipZoneType = ZoneType::AutoSectorScan;
        m_currentState = ControllerState::SectorScan_Aim_Point1;
    } else if (selectedType == "Target Ref Point") {
        m_wipZoneType = ZoneType::TargetReferencePoint;
        m_currentState = ControllerState::TRP_Aim_Point;
    } else if (selectedType == "Back") {
        m_currentState = ControllerState::Idle_MainMenu;
        m_currentMenuIndex = 0; // Reset index for main menu
    }
    m_wipAreaZone.type = m_wipZoneType; // Set type in WIP data
    updateUI();
}

void ZoneDefinitionWidget::processSelectZoneTypeForModifyDeleteSelect()
{
    if (m_currentMenuIndex >= 0 && m_currentMenuIndex < m_currentMenuItems.size()) {
        QString selectedType = m_currentMenuItems[m_currentMenuIndex];

        if (selectedType == "Back") {
            m_currentState = ControllerState::Idle_MainMenu;
            m_currentMenuIndex = 0;
        } else {
            // Déterminer l'état suivant basé sur l'état actuel et le type sélectionné
            if (m_currentState == ControllerState::Select_ZoneType_ForModify) {
                if (selectedType == "Area Zone") {
                    m_currentState = ControllerState::Select_AreaZone_ToModify;
                } else if (selectedType == "Sector Scan") {
                    m_currentState = ControllerState::Select_SectorScan_ToModify;
                } else if (selectedType == "TRP") {
                    m_currentState = ControllerState::Select_TRP_ToModify;
                }
            } else if (m_currentState == ControllerState::Select_ZoneType_ForDelete) {
                if (selectedType == "Area Zone") {
                    m_currentState = ControllerState::Select_AreaZone_ToDelete;
                } else if (selectedType == "Sector Scan") {
                    m_currentState = ControllerState::Select_SectorScan_ToDelete;
                } else if (selectedType == "TRP") {
                    m_currentState = ControllerState::Select_TRP_ToDelete;
                }
            }
            m_currentMenuIndex = 0;
        }
    }
    updateUI();
}

/*void ZoneDefinitionWidget::processSelectExistingZoneSelect()
{
    QString selectedIdStr = m_currentMenuItems.value(m_currentMenuIndex);
    if (selectedIdStr == "Back") {
        m_currentState = ControllerState::Idle_MainMenu; // Or go back to type selection?
        m_currentMenuIndex = 0;
        updateUI();
        return;
    }

    bool ok;
    int selectedId = selectedIdStr.toInt(&ok);
    if (!ok || !m_stateModel) {
        m_currentState = ControllerState::Show_Message;
        setupShowMessageUI("Error: Invalid zone selection.");
        return;
    }

    m_editingZoneId = selectedId;
    bool zoneFound = false;

    // Determine next state based on current state (Modify vs Delete)
    ControllerState nextState = ControllerState::Show_Message; // Default to error
    QString errorMessage = "Error: Could not load selected zone data.";

    if (m_currentState == ControllerState::Select_AreaZone_ToModify ||
        m_currentState == ControllerState::Select_SectorScan_ToModify ||
        m_currentState == ControllerState::Select_TRP_ToModify) {
        // --- MODIFY FLOW ---
        if (m_currentState == ControllerState::Select_AreaZone_ToModify) {
            AreaZone zoneData;
            if (m_stateModel->getAreaZoneById(selectedId)) {
                m_wipAreaZone = zoneData; // Load data into WIP
                m_wipZoneType = zoneData.type;
                nextState = ControllerState::AreaZone_Aim_Corner1; // Start re-definition
                zoneFound = true;
            }
        } else if (m_currentState == ControllerState::Select_SectorScan_ToModify) {
            AutoSectorScanZone zoneData;
            if (m_stateModel->getSectorScanZoneById(selectedId)) {
                m_wipSectorScanZone = zoneData;
                m_wipZoneType = ZoneType::AutoSectorScan;
                nextState = ControllerState::SectorScan_Aim_Point1;
                zoneFound = true;
            }
        } else { // TRP
            TargetReferencePoint zoneData;
            if (m_stateModel->getTRPById(selectedId)) {
                m_wipTRP = zoneData;
                m_wipZoneType = ZoneType::TargetReferencePoint;
                nextState = ControllerState::TRP_Aim_Point;
                zoneFound = true;
            }
        }
    } else if (m_currentState == ControllerState::Select_AreaZone_ToDelete ||
               m_currentState == ControllerState::Select_SectorScan_ToDelete ||
               m_currentState == ControllerState::Select_TRP_ToDelete) {
        // --- DELETE FLOW ---
        // Need to know the type to confirm deletion message
        if (m_currentState == ControllerState::Select_AreaZone_ToDelete) m_wipZoneType = ZoneType::Safety; // Type doesn't strictly matter here, just need ID
        else if (m_currentState == ControllerState::Select_SectorScan_ToDelete) m_wipZoneType = ZoneType::AutoSectorScan;
        else m_wipZoneType = ZoneType::TargetReferencePoint;
        nextState = ControllerState::Confirm_Delete;
        zoneFound = true; // Assume ID is valid for confirmation
    }

    if (zoneFound) {
        m_currentState = nextState;
    } else {
        m_currentState = ControllerState::Show_Message;
        setupShowMessageUI(errorMessage);
        return; // Don't updateUI normally
    }
    updateUI();
}*/
void ZoneDefinitionWidget::processSelectExistingZoneSelect()
{
    if (m_currentMenuIndex >= 0 && m_currentMenuIndex < m_currentMenuItems.size()) {
        QString selectedItem = m_currentMenuItems[m_currentMenuIndex];

        qDebug() << "processSelectExistingZoneSelect: Selected item:" << selectedItem
                 << "Current state:" << static_cast<int>(m_currentState);

        if (selectedItem == "Back") {
            // Retourner au menu de sélection du type de zone selon l'état actuel
            switch (m_currentState) {
                case ControllerState::Select_AreaZone_ToModify:
                case ControllerState::Select_SectorScan_ToModify:
                case ControllerState::Select_TRP_ToModify:
                    m_currentState = ControllerState::Select_ZoneType_ForModify;
                    break;
                case ControllerState::Select_AreaZone_ToDelete:
                case ControllerState::Select_SectorScan_ToDelete:
                case ControllerState::Select_TRP_ToDelete:
                    m_currentState = ControllerState::Select_ZoneType_ForDelete;
                    break;
                default:
                    qWarning() << "processSelectExistingZoneSelect: Unexpected state for Back navigation:"
                               << static_cast<int>(m_currentState);
                    m_currentState = ControllerState::Idle_MainMenu;
                    break;
            }
            m_currentMenuIndex = 0;

        } else if (selectedItem == "NoZones") {
            qDebug() << "processSelectExistingZoneSelect: NoZones selected, ignoring";
            return;

        } else {
            // Zone ID selected
            m_editingZoneId = selectedItem.toInt();

            qDebug() << "processSelectExistingZoneSelect: Zone ID selected:" << m_editingZoneId;

            // Determine next state based on current state
            switch (m_currentState) {
                case ControllerState::Select_AreaZone_ToModify:
                    if (auto* zone = m_stateModel->getAreaZoneById(m_editingZoneId)) {
                        m_wipAreaZone = *zone;
                        m_wipZoneType = zone->type;
                        m_currentState = ControllerState::AreaZone_Edit_Parameters;
                        qDebug() << "processSelectExistingZoneSelect: Loaded AreaZone for editing, ID:" << m_editingZoneId;
                    } else {
                        qWarning() << "processSelectExistingZoneSelect: AreaZone not found, ID:" << m_editingZoneId;
                        m_currentState = ControllerState::Show_Message;
                        setupShowMessageUI("Zone not found!");
                    }
                    break;

                case ControllerState::Select_SectorScan_ToModify:
                    if (auto* zone = m_stateModel->getSectorScanZoneById(m_editingZoneId)) {
                        m_wipSectorScanZone = *zone;
                        m_wipZoneType = ZoneType::AutoSectorScan;
                        m_currentState = ControllerState::SectorScan_Edit_Parameters;
                        qDebug() << "processSelectExistingZoneSelect: Loaded SectorScanZone for editing, ID:" << m_editingZoneId;
                    } else {
                        qWarning() << "processSelectExistingZoneSelect: SectorScanZone not found, ID:" << m_editingZoneId;
                        m_currentState = ControllerState::Show_Message;
                        setupShowMessageUI("Zone not found!");
                    }
                    break;

                case ControllerState::Select_TRP_ToModify:
                    if (auto* trp = m_stateModel->getTRPById(m_editingZoneId)) {
                        m_wipTRP = *trp;
                        m_wipZoneType = ZoneType::TargetReferencePoint;
                        m_currentState = ControllerState::TRP_Edit_Parameters;
                        qDebug() << "processSelectExistingZoneSelect: Loaded TRP for editing, ID:" << m_editingZoneId;
                    } else {
                        qWarning() << "processSelectExistingZoneSelect: TRP not found, ID:" << m_editingZoneId;
                        m_currentState = ControllerState::Show_Message;
                        setupShowMessageUI("Zone not found!");
                    }
                    break;

                case ControllerState::Select_AreaZone_ToDelete:
                    // NOUVEAU : Stocker le type de zone à supprimer
                    m_deleteZoneType = ZoneType::Safety; // Ou le type approprié d'Area Zone
                    m_currentState = ControllerState::Confirm_Delete;
                    qDebug() << "processSelectExistingZoneSelect: Going to delete Area Zone ID:" << m_editingZoneId;
                    break;

                case ControllerState::Select_SectorScan_ToDelete:
                    // NOUVEAU : Stocker le type de zone à supprimer
                    m_deleteZoneType = ZoneType::AutoSectorScan;
                    m_currentState = ControllerState::Confirm_Delete;
                    qDebug() << "processSelectExistingZoneSelect: Going to delete Sector Scan Zone ID:" << m_editingZoneId;
                    break;

                case ControllerState::Select_TRP_ToDelete:
                    // NOUVEAU : Stocker le type de zone à supprimer
                    m_deleteZoneType = ZoneType::TargetReferencePoint;
                    m_currentState = ControllerState::Confirm_Delete;
                    qDebug() << "processSelectExistingZoneSelect: Going to delete TRP ID:" << m_editingZoneId;
                    break;

                default:
                    qWarning() << "processSelectExistingZoneSelect: Unexpected state for zone selection:"
                               << static_cast<int>(m_currentState);
                    m_currentState = ControllerState::Idle_MainMenu;
                    break;
            }
            m_currentMenuIndex = 0;
        }
    }

    qDebug() << "processSelectExistingZoneSelect: New state:" << static_cast<int>(m_currentState);
    updateUI();
}


// processAimPointConfirm needs significant changes for AreaZone
void ZoneDefinitionWidget::processAimPointConfirm()
{
    switch (m_currentState) {
        // --- AreaZone: Capture Corner 1 or Corner 2 ---
        case ControllerState::AreaZone_Aim_Corner1:
            m_wipAz1 = m_currentGimbalAz;
            m_wipEl1 = m_currentGimbalEl;
            m_corner1Defined = true;
            m_currentState = ControllerState::AreaZone_Aim_Corner2;
            qDebug() << "AreaZone Corner 1 captured: Az=" << m_wipAz1 << "El=" << m_wipEl1;
            break;
        case ControllerState::AreaZone_Aim_Corner2:
            if (!m_corner1Defined) { // Should not happen in normal flow
                 m_currentState = ControllerState::Show_Message;
                 setupShowMessageUI("Error: Corner 1 not defined.");
                 return;
            }
            m_wipAz2 = m_currentGimbalAz;
            m_wipEl2 = m_currentGimbalEl;
            qDebug() << "AreaZone Corner 2 captured: Az=" << m_wipAz2 << "El=" << m_wipEl2;
            // Calculate geometry and store in m_wipAreaZone
            calculateAreaZoneGeometry();
            m_currentState = ControllerState::AreaZone_Edit_Parameters;
            break;

        // --- SectorScan: Capture Point 1 or Point 2 ---
        case ControllerState::SectorScan_Aim_Point1:
            m_wipSectorScanZone.az1 = m_currentGimbalAz;
            m_wipSectorScanZone.el1 = m_currentGimbalEl;
            m_currentState = ControllerState::SectorScan_Aim_Point2;
            break;
        case ControllerState::SectorScan_Aim_Point2:
            m_wipSectorScanZone.az2 = m_currentGimbalAz;
            m_wipSectorScanZone.el2 = m_currentGimbalEl;
            m_currentState = ControllerState::SectorScan_Edit_Parameters;
            break;

        // --- TRP: Capture Point ---
        case ControllerState::TRP_Aim_Point:
            m_wipTRP.azimuth = m_currentGimbalAz;
            m_wipTRP.elevation = m_currentGimbalEl;
            m_currentState = ControllerState::TRP_Edit_Parameters;
            break;

        default:
            qWarning() << "processAimPointConfirm called in unexpected state:" << static_cast<int>(m_currentState);
            return;
    }
    updateUI();
}

// processEditParametersConfirm is now only called when Validate button is activated
void ZoneDefinitionWidget::processEditParametersConfirm()
{
     switch (m_currentState) {
        case ControllerState::AreaZone_Edit_Parameters:
            if (m_areaZoneParameterPanel) {
                m_wipAreaZone = m_areaZoneParameterPanel->getZoneData(m_wipAreaZone);
            }
            m_currentState = ControllerState::Confirm_Save;
            break;
        case ControllerState::SectorScan_Edit_Parameters:
            if (m_sectorScanParameterPanel) {
                m_wipSectorScanZone = m_sectorScanParameterPanel->getZoneData(m_wipSectorScanZone);
            }
            m_currentState = ControllerState::Confirm_Save;
            break;
        case ControllerState::TRP_Edit_Parameters:
            if (m_trpParameterPanel) {
               // m_wipTRP = m_trpParameterPanel->getZoneData(m_wipTRP);
            }
            m_currentState = ControllerState::Confirm_Save;
            break;
        default:
            qWarning() << "processEditParametersConfirm called in unexpected state:" << static_cast<int>(m_currentState);
            return;
    }
    updateUI();
}

// processConfirmSaveSelect needs to handle all zone types
void ZoneDefinitionWidget::processConfirmSaveSelect()
{
    QString selectedOption = m_currentMenuItems.value(m_currentMenuIndex);
    if (selectedOption == "Yes") {
        bool success = false;
        QString successMsg = "Zone saved successfully.";
        QString errorMsg = "Error: Failed to save zone.";

        if (!m_stateModel) {
            success = false;
            errorMsg = "Error: SystemStateModel is not available.";
        } else {
            if (m_editingZoneId == -1) { // ADD new zone
                switch (m_wipZoneType) {
                    case ZoneType::Safety:
                    case ZoneType::NoTraverse:
                    case ZoneType::NoFire:
                        success = m_stateModel->addAreaZone(m_wipAreaZone);
                        break;
                    case ZoneType::AutoSectorScan:
                        success = m_stateModel->addSectorScanZone(m_wipSectorScanZone);
                        break;
                    case ZoneType::TargetReferencePoint:
                        success = m_stateModel->addTRP(m_wipTRP);
                        break;
                    default: errorMsg = "Error: Unknown zone type to add."; break;
                }
            } else { // MODIFY existing zone
                 switch (m_wipZoneType) {
                    case ZoneType::Safety:
                    case ZoneType::NoTraverse:
                    case ZoneType::NoFire:
                        success = m_stateModel->modifyAreaZone(m_editingZoneId, m_wipAreaZone);
                        break;
                    case ZoneType::AutoSectorScan:
                        success = m_stateModel->modifySectorScanZone(m_editingZoneId, m_wipSectorScanZone);
                        break;
                    case ZoneType::TargetReferencePoint:
                        success = m_stateModel->modifyTRP(m_editingZoneId, m_wipTRP);
                        break;
                    default: errorMsg = "Error: Unknown zone type to modify."; break;
                }
            }
        }

        if (success) {
            if (m_stateModel->saveZonesToFile("zones.json")) {
                qDebug() << "Zones successfully saved to zones.json";
            } else {
                qWarning() << "Failed to save zones to zones.json!";
                // Vous pourriez envisager d'informer l'utilisateur ici si la sauvegarde échoue,
                // même si l'ajout/modification en mémoire a réussi.
            }
            m_currentState = ControllerState::Idle_MainMenu;
            resetWipDataAndState();
            m_currentMenuIndex = 0;
            // Optionally show success message briefly?
        } else {
            m_currentState = ControllerState::Show_Message;
            setupShowMessageUI(errorMsg);
            return; // Don't updateUI normally
        }

    } else { // No
        m_currentState = ControllerState::Idle_MainMenu;
        resetWipDataAndState();
        m_currentMenuIndex = 0;
    }
    updateUI();
}

// processConfirmDeleteSelect needs to handle all zone types
void ZoneDefinitionWidget::processConfirmDeleteSelect()
{
    if (m_currentMenuIndex >= 0 && m_currentMenuIndex < m_currentMenuItems.size()) {
        QString selectedItem = m_currentMenuItems[m_currentMenuIndex];

        qDebug() << "processConfirmDeleteSelect: Selected:" << selectedItem
                 << "Zone ID:" << m_editingZoneId
                 << "Previous state before Confirm_Delete:" << static_cast<int>(m_deleteZoneType);

        if (selectedItem == "Yes") {
            bool success = false;
            QString zoneTypeName;

            // CORRECTION : Utiliser l'état précédent pour déterminer le type de zone à supprimer
            // On doit stocker l'état précédent avant d'entrer dans Confirm_Delete

            // Déterminer le type de zone basé sur l'état d'où on vient
            if (m_deleteZoneType == ZoneType::Safety ||
                m_deleteZoneType == ZoneType::NoTraverse ||
                m_deleteZoneType == ZoneType::NoFire) {
                // Supprimer une Area Zone
                success = m_stateModel->deleteAreaZone(m_editingZoneId);
                zoneTypeName = "Area Zone";
                qDebug() << "processConfirmDeleteSelect: Attempting to delete Area Zone ID:" << m_editingZoneId;

            } else if (m_deleteZoneType == ZoneType::AutoSectorScan) {
                // Supprimer une Sector Scan Zone
                success = m_stateModel->deleteSectorScanZone(m_editingZoneId);
                zoneTypeName = "Sector Scan Zone";
                qDebug() << "processConfirmDeleteSelect: Attempting to delete Sector Scan Zone ID:" << m_editingZoneId;

            } else if (m_deleteZoneType == ZoneType::TargetReferencePoint) {
                // Supprimer un TRP
                success = m_stateModel->deleteTRP(m_editingZoneId);
                zoneTypeName = "TRP";
                qDebug() << "processConfirmDeleteSelect: Attempting to delete TRP ID:" << m_editingZoneId;

            } else {
                qWarning() << "processConfirmDeleteSelect: Unknown zone type for deletion:" << static_cast<int>(m_deleteZoneType);
                success = false;
                zoneTypeName = "Unknown";
            }

            if (success) {
                // AJOUT : Sauvegarder les zones dans le fichier JSON après suppression
                QString jsonFilePath = "zones_data.json"; // Ou le chemin approprié
                bool saveSuccess = m_stateModel->saveZonesToFile(jsonFilePath);

                if (saveSuccess) {
                    m_currentState = ControllerState::Show_Message;
                    setupShowMessageUI(QString("%1 deleted and saved successfully!").arg(zoneTypeName));
                    qDebug() << "processConfirmDeleteSelect: Successfully deleted and saved" << zoneTypeName << "ID:" << m_editingZoneId;
                } else {
                    m_currentState = ControllerState::Show_Message;
                    setupShowMessageUI(QString("%1 deleted but failed to save to file!").arg(zoneTypeName));
                    qWarning() << "processConfirmDeleteSelect: Deleted" << zoneTypeName << "but failed to save to JSON";
                }

                // Auto-return to main menu after a brief delay
                QTimer::singleShot(2000, [this]() {
                    m_currentState = ControllerState::Idle_MainMenu;
                    updateUI();
                });
            } else {
                m_currentState = ControllerState::Show_Message;
                setupShowMessageUI(QString("Failed to delete %1!").arg(zoneTypeName));
                qWarning() << "processConfirmDeleteSelect: Failed to delete" << zoneTypeName << "ID:" << m_editingZoneId;
            }
        } else if (selectedItem == "No") {
            // Cancel delete, return to main menu
            m_currentState = ControllerState::Idle_MainMenu;
            qDebug() << "processConfirmDeleteSelect: Delete cancelled";
        }

        m_currentMenuIndex = 0;
        resetWipDataAndState();
    }
    updateUI();
}


// --- Parameter Panel Navigation ---

// activateNext/PreviousParameterField needs update for AreaZone panel with buttons
void ZoneDefinitionWidget::activateNextParameterField()
{
    if (m_currentState == ControllerState::AreaZone_Edit_Parameters && m_areaZoneParameterPanel) {
        AreaZoneParameterPanel::Field currentField = m_areaZoneParameterPanel->getActiveField();
        AreaZoneParameterPanel::Field nextField = AreaZoneParameterPanel::Field::None;
        switch (currentField) {
            case AreaZoneParameterPanel::Field::None:
            case AreaZoneParameterPanel::Field::CancelButton: // Loop back from last to first
                nextField = AreaZoneParameterPanel::Field::Enabled;
                break;
            case AreaZoneParameterPanel::Field::Enabled:
                nextField = AreaZoneParameterPanel::Field::Overridable;
                break;
            case AreaZoneParameterPanel::Field::Overridable:
                nextField = AreaZoneParameterPanel::Field::ValidateButton;
                break;
            case AreaZoneParameterPanel::Field::ValidateButton:
                nextField = AreaZoneParameterPanel::Field::CancelButton;
                break;
        }
        m_areaZoneParameterPanel->setActiveField(nextField);
    }
    else if (m_currentState == ControllerState::SectorScan_Edit_Parameters && m_sectorScanParameterPanel) {
        SectorScanParameterPanel::Field currentField = m_sectorScanParameterPanel->getActiveField();
        SectorScanParameterPanel::Field nextField = SectorScanParameterPanel::Field::None;
        switch (currentField) {
            case SectorScanParameterPanel::Field::None:
            case SectorScanParameterPanel::Field::CancelButton: // Loop back from last to first
                nextField = SectorScanParameterPanel::Field::Enabled;
                break;
            case SectorScanParameterPanel::Field::Enabled:
                nextField = SectorScanParameterPanel::Field::ValidateButton;
                break;
            case SectorScanParameterPanel::Field::ValidateButton:
                nextField = SectorScanParameterPanel::Field::CancelButton;
                break;
        }
        m_sectorScanParameterPanel->setActiveField(nextField);
    }
    else if (m_currentState == ControllerState::TRP_Edit_Parameters && m_trpParameterPanel) {
        TRPParameterPanel::Field currentField = m_trpParameterPanel->getActiveField();
        TRPParameterPanel::Field nextField = TRPParameterPanel::Field::None;
        switch (currentField) {
            case TRPParameterPanel::Field::None:
            case TRPParameterPanel::Field::CancelButton: // Loop back from last to first
                nextField = TRPParameterPanel::Field::LocationPage;
                break;
            case TRPParameterPanel::Field::LocationPage:
                nextField = TRPParameterPanel::Field::ValidateButton;
                break;
            case TRPParameterPanel::Field::ValidateButton:
                nextField = TRPParameterPanel::Field::CancelButton;
                break;
        }
        m_trpParameterPanel->setActiveField(nextField);
    }
    else if (m_currentState == ControllerState::Idle_MainMenu) {
        // Cycle through main menu items
        m_currentMenuIndex = (m_currentMenuIndex + 1) % m_currentMenuItems.size();
        m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
    }
    // Add logic for other panels...
}

void ZoneDefinitionWidget::activatePreviousParameterField()
{
     if (m_currentState == ControllerState::AreaZone_Edit_Parameters && m_areaZoneParameterPanel) {
        AreaZoneParameterPanel::Field currentField = m_areaZoneParameterPanel->getActiveField();
        AreaZoneParameterPanel::Field prevField = AreaZoneParameterPanel::Field::None;
        switch (currentField) {
            case AreaZoneParameterPanel::Field::None:
            case AreaZoneParameterPanel::Field::Enabled: // Loop back from first to last
                prevField = AreaZoneParameterPanel::Field::CancelButton;
                break;
            case AreaZoneParameterPanel::Field::Overridable:
                prevField = AreaZoneParameterPanel::Field::Enabled;
                break;
            case AreaZoneParameterPanel::Field::ValidateButton:
                prevField = AreaZoneParameterPanel::Field::Overridable;
                break;
            case AreaZoneParameterPanel::Field::CancelButton:
                prevField = AreaZoneParameterPanel::Field::ValidateButton;
                break;
        }
        m_areaZoneParameterPanel->setActiveField(prevField);
    }
    else if (m_currentState == ControllerState::SectorScan_Edit_Parameters && m_sectorScanParameterPanel) {
        SectorScanParameterPanel::Field currentField = m_sectorScanParameterPanel->getActiveField();
        SectorScanParameterPanel::Field prevField = SectorScanParameterPanel::Field::None;
        switch (currentField) {
            case SectorScanParameterPanel::Field::None:
            case SectorScanParameterPanel::Field::Enabled: // Loop back from first to last
                prevField = SectorScanParameterPanel::Field::CancelButton;
                break;
            case SectorScanParameterPanel::Field::ValidateButton:
                prevField = SectorScanParameterPanel::Field::Enabled;
                break;
            case SectorScanParameterPanel::Field::CancelButton:
                prevField = SectorScanParameterPanel::Field::ValidateButton;
                break;
        }
        m_sectorScanParameterPanel->setActiveField(prevField);
    }
    else if (m_currentState == ControllerState::TRP_Edit_Parameters && m_trpParameterPanel) {
        TRPParameterPanel::Field currentField = m_trpParameterPanel->getActiveField();
        TRPParameterPanel::Field prevField = TRPParameterPanel::Field::None;
        switch (currentField) {
            case TRPParameterPanel::Field::None:
            case TRPParameterPanel::Field::LocationPage: // Loop back from first to last
                prevField = TRPParameterPanel::Field::CancelButton;
                break;
            case TRPParameterPanel::Field::ValidateButton:
                prevField = TRPParameterPanel::Field::LocationPage;
                break;
            case TRPParameterPanel::Field::CancelButton:
                prevField = TRPParameterPanel::Field::ValidateButton;
                break;
        }
        m_trpParameterPanel->setActiveField(prevField);
    }
    else if (m_currentState == ControllerState::Idle_MainMenu) {
        // Cycle through main menu items backwards
        m_currentMenuIndex = (m_currentMenuIndex - 1 + m_currentMenuItems.size()) % m_currentMenuItems.size();
        m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
    }
    // Add logic for other panels...
}

// handleParameterPanelInput needs update for AreaZone panel
void ZoneDefinitionWidget::handleParameterPanelInput(bool isUp)
{
    // UP/DOWN navigates fields/buttons
    if (isUp) {
        activatePreviousParameterField();
    } else {
        activateNextParameterField();
    }
}

// --- Other Helpers ---

// updateMapWipZone needs update for AreaZone aiming
void ZoneDefinitionWidget::updateMapWipZone()
{
    if (!m_zoneMapWidget) return;

    QVariant wipData;
    bool isDefiningStart = false;
    bool isDefiningEnd = false;

    switch (m_currentState) {
        case ControllerState::AreaZone_Aim_Corner1:
            // Show gimbal position as potential corner 1
            m_wipAreaZone.startAzimuth = m_currentGimbalAz; // Use temp fields for drawing
            m_wipAreaZone.minElevation = m_currentGimbalEl;
            m_wipAreaZone.endAzimuth = m_currentGimbalAz;   // Make it a point for now
            m_wipAreaZone.maxElevation = m_currentGimbalEl;
            wipData.setValue(m_wipAreaZone);
            isDefiningStart = true;
            isDefiningEnd = false;
            break;
        case ControllerState::AreaZone_Aim_Corner2:
            // Show rectangle defined by fixed corner 1 and current gimbal as corner 2
            calculateAreaZoneGeometry(); // Calculate geometry based on m_wipAz1/El1 and current gimbal
            wipData.setValue(m_wipAreaZone);
            isDefiningStart = true;
            isDefiningEnd = true; // Show the full potential rectangle
            break;
        case ControllerState::AreaZone_Edit_Parameters:
             // Show the final defined rectangle
            wipData.setValue(m_wipAreaZone);
            isDefiningStart = true;
            isDefiningEnd = true;
            break;

        case ControllerState::SectorScan_Aim_Point1:
            m_wipSectorScanZone.az1 = m_currentGimbalAz;
            m_wipSectorScanZone.el1 = m_currentGimbalEl;
            m_wipSectorScanZone.az2 = m_currentGimbalAz; // Temp end point
            m_wipSectorScanZone.el2 = m_currentGimbalEl;
            wipData.setValue(m_wipSectorScanZone);
            isDefiningStart = true;
            isDefiningEnd = false;
            break;
        case ControllerState::SectorScan_Aim_Point2:
            // Keep az1/el1 fixed, update az2/el2 to current gimbal
            m_wipSectorScanZone.az2 = m_currentGimbalAz;
            m_wipSectorScanZone.el2 = m_currentGimbalEl;
            wipData.setValue(m_wipSectorScanZone);
            isDefiningStart = true;
            isDefiningEnd = true;
            break;
         case ControllerState::SectorScan_Edit_Parameters:
            wipData.setValue(m_wipSectorScanZone);
            isDefiningStart = true;
            isDefiningEnd = true;
            break;

        case ControllerState::TRP_Aim_Point:
            m_wipTRP.azimuth = m_currentGimbalAz;
            m_wipTRP.elevation = m_currentGimbalEl;
            wipData.setValue(m_wipTRP);
            isDefiningStart = true;
            isDefiningEnd = false;
            break;
        case ControllerState::TRP_Edit_Parameters:
             wipData.setValue(m_wipTRP);
             isDefiningStart = true;
             isDefiningEnd = true;
             break;

        default:
            // Not in an aiming or editing state where WIP map update is needed
            m_zoneMapWidget->clearWipZone();
            return;
    }

    // Call the map's generic update function
    m_zoneMapWidget->updateWipZone(wipData, m_wipZoneType, isDefiningStart, isDefiningEnd);

}

// zoneTypeToString remains the same
QString ZoneDefinitionWidget::zoneTypeToString(ZoneType type)
{
    switch (type) {
        case ZoneType::Safety: return "Safety";
        case ZoneType::NoTraverse: return "No-Traverse";
        case ZoneType::NoFire: return "No-Fire";
        case ZoneType::AutoSectorScan: return "Sector Scan";
        case ZoneType::TargetReferencePoint: return "TRP";
        default: return "Unknown";
    }
}

// --- New Helper Methods ---

float ZoneDefinitionWidget::normalizeAzimuthTo360(float az) const {
    // Normalize azimuth to be within [0, 360)
    float normalized = fmod(az, 360.0f);
    if (normalized < 0) {
        normalized += 360.0f;
    }
    return normalized;
}

void ZoneDefinitionWidget::calculateAreaZoneGeometry() {
    // Calculate geometry based on m_wipAz1/El1 and m_wipAz2/El2 (or current gimbal if called during Aim_Corner2)
    float az1_norm = normalizeAzimuthTo360(m_wipAz1);
    float az2_norm = normalizeAzimuthTo360( (m_currentState == ControllerState::AreaZone_Aim_Corner2) ? m_currentGimbalAz : m_wipAz2 );
    float el1 = m_wipEl1;
    float el2 = (m_currentState == ControllerState::AreaZone_Aim_Corner2) ? m_currentGimbalEl : m_wipEl2;

    // Determine min/max elevation
    m_wipAreaZone.minElevation = std::min(el1, el2);
    m_wipAreaZone.maxElevation = std::max(el1, el2);

    // Determine start/end azimuth, handling wrap-around
    // If the distance clockwise is shorter, az1 is start
    // If the distance counter-clockwise is shorter, az2 is start
    float diff = az2_norm - az1_norm;
    if (diff >= 0) { // az2 is clockwise or same as az1
        if (diff <= 180.0f) { // Clockwise is shorter or equal
            m_wipAreaZone.startAzimuth = az1_norm;
            m_wipAreaZone.endAzimuth = az2_norm;
        } else { // Counter-clockwise is shorter
            m_wipAreaZone.startAzimuth = az2_norm;
            m_wipAreaZone.endAzimuth = az1_norm;
        }
    } else { // az2 is counter-clockwise from az1 (diff is negative)
        if (diff >= -180.0f) { // Counter-clockwise is shorter or equal
            m_wipAreaZone.startAzimuth = az2_norm;
            m_wipAreaZone.endAzimuth = az1_norm;
        } else { // Clockwise is shorter
            m_wipAreaZone.startAzimuth = az1_norm;
            m_wipAreaZone.endAzimuth = az2_norm;
        }
    }

    // Ensure start/end are distinct if corners were identical
    if (m_wipAreaZone.startAzimuth == m_wipAreaZone.endAzimuth && m_wipAreaZone.minElevation == m_wipAreaZone.maxElevation) {
        // Make it a minimal zone, e.g., 1 degree wide/high? Or handle as error?
        // For now, let it be a point/line zone.
        qWarning() << "AreaZone defined with identical corners.";
    }

    qDebug() << "Calculated AreaZone Geometry: StartAz=" << m_wipAreaZone.startAzimuth
             << "EndAz=" << m_wipAreaZone.endAzimuth
             << "MinEl=" << m_wipAreaZone.minElevation
             << "MaxEl=" << m_wipAreaZone.maxElevation;
}


// --- Event Handlers ---

// keyPressEvent needs update for panel navigation
void ZoneDefinitionWidget::keyPressEvent(QKeyEvent *event)
{
    // This is mainly for testing without the actual hardware buttons
    switch (event->key()) {
    case Qt::Key_Up:
        handleUpNavigation();
        break;
    case Qt::Key_Down:
        handleDownNavigation();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        handleSelectAction();
        break;
    case Qt::Key_Backspace:
        handleBackAction();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

// handleUpNavigation updated for panel navigation
void ZoneDefinitionWidget::handleUpNavigation()
{
    switch (m_currentState) {
        case ControllerState::Idle_MainMenu:
        case ControllerState::Select_ZoneType_ForNew:
        case ControllerState::Select_ZoneType_ForModify:
        case ControllerState::Select_ZoneType_ForDelete:
        case ControllerState::Select_AreaZone_ToModify:
        case ControllerState::Select_SectorScan_ToModify:
        case ControllerState::Select_TRP_ToModify:
        case ControllerState::Select_AreaZone_ToDelete:
        case ControllerState::Select_SectorScan_ToDelete:
        case ControllerState::Select_TRP_ToDelete:
        case ControllerState::Confirm_Save:
        case ControllerState::Confirm_Delete:
            // Navigation dans les listes
            if (m_currentMenuIndex > 0) {
                m_currentMenuIndex--;
                if (m_mainMenuListWidget->isVisible()) {
                    m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
                } else if (m_zoneSelectionListWidget->isVisible()) {
                    m_zoneSelectionListWidget->setCurrentRow(m_currentMenuIndex);
                }
            }
            break;

        case ControllerState::AreaZone_Edit_Parameters:
            if (m_areaZoneParameterPanel) {
                m_areaZoneParameterPanel->handleUpInput();
            }
            break;

        case ControllerState::SectorScan_Edit_Parameters:
            if (m_sectorScanParameterPanel) {
                m_sectorScanParameterPanel->handleUpInput();
            }
            break;

        case ControllerState::TRP_Edit_Parameters:
            if (m_trpParameterPanel) {
                m_trpParameterPanel->handleUpInput();
            }
            break;

        default:
            break;
    }
}

// handleDownNavigation updated for panel navigation
void ZoneDefinitionWidget::handleDownNavigation()
{
    switch (m_currentState) {
        case ControllerState::Idle_MainMenu:
        case ControllerState::Select_ZoneType_ForNew:
        case ControllerState::Select_ZoneType_ForModify:
        case ControllerState::Select_ZoneType_ForDelete:
        case ControllerState::Select_AreaZone_ToModify:
        case ControllerState::Select_SectorScan_ToModify:
        case ControllerState::Select_TRP_ToModify:
        case ControllerState::Select_AreaZone_ToDelete:
        case ControllerState::Select_SectorScan_ToDelete:
        case ControllerState::Select_TRP_ToDelete:
        case ControllerState::Confirm_Save:
        case ControllerState::Confirm_Delete:
            // Navigation dans les listes
            if (m_currentMenuIndex < m_currentMenuItems.size() - 1) {
                m_currentMenuIndex++;
                if (m_mainMenuListWidget->isVisible()) {
                    m_mainMenuListWidget->setCurrentRow(m_currentMenuIndex);
                } else if (m_zoneSelectionListWidget->isVisible()) {
                    m_zoneSelectionListWidget->setCurrentRow(m_currentMenuIndex);
                }
            }
            break;

        case ControllerState::AreaZone_Edit_Parameters:
            if (m_areaZoneParameterPanel) {
                m_areaZoneParameterPanel->handleDownInput();
            }
            break;

        case ControllerState::SectorScan_Edit_Parameters:
            if (m_sectorScanParameterPanel) {
                m_sectorScanParameterPanel->handleDownInput();
            }
            break;

        case ControllerState::TRP_Edit_Parameters:
            if (m_trpParameterPanel) {
                m_trpParameterPanel->handleDownInput();
            }
            break;

        default:
            break;
    }
}

// handleSelectAction needs update for panel interaction with buttons
void ZoneDefinitionWidget::handleSelectAction()
{
    qDebug() << "processSelectAction called with state:" << static_cast<int>(m_currentState) 
             << "Menu index:" << m_currentMenuIndex;
    
    switch (m_currentState) {
        case ControllerState::Idle_MainMenu:
            processMainMenuSelect();
            break;
        case ControllerState::Select_ZoneType_ForNew:
            processSelectZoneTypeSelect();
            break;
            
        case ControllerState::Select_ZoneType_ForModify:
        case ControllerState::Select_ZoneType_ForDelete:
            processSelectZoneTypeForModifyDeleteSelect();
            break;

        case ControllerState::Select_AreaZone_ToModify:
        case ControllerState::Select_SectorScan_ToModify:
        case ControllerState::Select_TRP_ToModify:
        case ControllerState::Select_AreaZone_ToDelete:
        case ControllerState::Select_SectorScan_ToDelete:
        case ControllerState::Select_TRP_ToDelete:
            processSelectExistingZoneSelect();
            break;
            
        case ControllerState::AreaZone_Aim_Corner1:
        case ControllerState::AreaZone_Aim_Corner2:
        case ControllerState::SectorScan_Aim_Point1:
        case ControllerState::SectorScan_Aim_Point2:
        case ControllerState::TRP_Aim_Point:
            processAimPointConfirm();
            break;
            
        case ControllerState::AreaZone_Edit_Parameters:
            if (m_areaZoneParameterPanel) {
                m_areaZoneParameterPanel->handleSelectInput();
                
                auto activeField = m_areaZoneParameterPanel->getActiveField();
                if (activeField == AreaZoneParameterPanel::Field::ValidateButton && 
                    m_areaZoneParameterPanel->getEditMode() == AreaZoneParameterPanel::EditMode::Navigation) {
                    m_wipAreaZone = m_areaZoneParameterPanel->getZoneData(m_wipAreaZone);
                    qDebug() << "AreaZone Validate button activated.";
                    processEditParametersConfirm();
                    return;
                } else if (activeField == AreaZoneParameterPanel::Field::CancelButton && 
                          m_areaZoneParameterPanel->getEditMode() == AreaZoneParameterPanel::EditMode::Navigation) {
                    qDebug() << "AreaZone Cancel button activated.";
                    m_currentState = ControllerState::AreaZone_Aim_Corner2;
                    if (m_editingZoneId != -1) {
                        if (auto* zone = m_stateModel->getAreaZoneById(m_editingZoneId)) {
                            m_wipAreaZone = *zone;
                        } else {
                            qWarning() << "Could not reload original AreaZone data on cancel.";
                            resetWipDataAndState();
                            m_currentState = ControllerState::Idle_MainMenu;
                        }
                    } else {
                        m_corner1Defined = true;
                    }
                    updateUI();
                    return;
                }
            }
            break;
            
        case ControllerState::SectorScan_Edit_Parameters:
            if (m_sectorScanParameterPanel) {
                m_sectorScanParameterPanel->handleSelectInput();
                
                auto activeField = m_sectorScanParameterPanel->getActiveField();
                if (activeField == SectorScanParameterPanel::Field::ValidateButton && 
                    m_sectorScanParameterPanel->getEditMode() == SectorScanParameterPanel::EditMode::Navigation) {
                    m_wipSectorScanZone = m_sectorScanParameterPanel->getZoneData(m_wipSectorScanZone);
                    qDebug() << "SectorScan Validate button activated.";
                    processEditParametersConfirm();
                    return;
                } else if (activeField == SectorScanParameterPanel::Field::CancelButton && 
                          m_sectorScanParameterPanel->getEditMode() == SectorScanParameterPanel::EditMode::Navigation) {
                    qDebug() << "SectorScan Cancel button activated.";
                    m_currentState = ControllerState::SectorScan_Aim_Point2;
                    if (m_editingZoneId != -1) {
                        if (auto* zone = m_stateModel->getSectorScanZoneById(m_editingZoneId)) {
                            m_wipSectorScanZone = *zone;
                        }
                        else {
                            qWarning() << "Could not reload original SectorScan zone data on cancel.";
                            resetWipDataAndState();
                            m_currentState = ControllerState::Idle_MainMenu;
                        }
                    }
                    updateUI();
                    return;
                }
            }
            break;
            
        case ControllerState::TRP_Edit_Parameters:
            if (m_trpParameterPanel) {
                m_trpParameterPanel->handleSelectInput();
                
                auto activeField = m_trpParameterPanel->getActiveField();
                if (activeField == TRPParameterPanel::Field::ValidateButton && 
                    m_trpParameterPanel->getEditMode() == TRPParameterPanel::EditMode::Navigation) {
                    // CORRECTION ICI : Utiliser getTRPData au lieu de getZoneData
                    m_wipTRP = m_trpParameterPanel->getTRPData(m_wipTRP);
                    qDebug() << "TRP Validate button activated.";
                    processEditParametersConfirm();
                    return;
                } else if (activeField == TRPParameterPanel::Field::CancelButton && 
                          m_trpParameterPanel->getEditMode() == TRPParameterPanel::EditMode::Navigation) {
                    qDebug() << "TRP Cancel button activated.";
                    m_currentState = ControllerState::TRP_Aim_Point;
                    if (m_editingZoneId != -1) {
                        if (auto* trp = m_stateModel->getTRPById(m_editingZoneId)) {
                            m_wipTRP = *trp;
                        }
                        else {
                            qWarning() << "Could not reload original TRP data on cancel.";
                            resetWipDataAndState();
                            m_currentState = ControllerState::Idle_MainMenu;
                        }
                    }
                    updateUI();
                    return;
                }
            }
            break;
            
        case ControllerState::Confirm_Save:
            processConfirmSaveSelect();
            break;
        case ControllerState::Confirm_Delete:
            processConfirmDeleteSelect();
            break;
            
        case ControllerState::Show_Message:
            m_currentState = ControllerState::Idle_MainMenu;
            resetWipDataAndState();
            updateUI();
            break;
            
        default:
            qWarning() << "processSelectAction: Unhandled state:" << static_cast<int>(m_currentState);
            break;
    }
}

// handleBackAction needs update for panel interaction
void ZoneDefinitionWidget::handleBackAction()
{
    switch (m_currentState) {
        case ControllerState::Idle_MainMenu:
            this->close();
            emit widgetClosed(); // Back from main menu closes widget
            break;
        case ControllerState::Select_ZoneType_ForNew:
        case ControllerState::Select_AreaZone_ToModify:
        case ControllerState::Select_SectorScan_ToModify:
        case ControllerState::Select_TRP_ToModify:
        case ControllerState::Select_AreaZone_ToDelete:
        case ControllerState::Select_SectorScan_ToDelete:
        case ControllerState::Select_TRP_ToDelete:
            m_currentState = ControllerState::Idle_MainMenu;
            resetWipDataAndState();
            m_currentMenuIndex = 0;
            break;
        case ControllerState::AreaZone_Aim_Corner1:
        case ControllerState::SectorScan_Aim_Point1:
        case ControllerState::TRP_Aim_Point:
            // Back from first aiming point goes back to type selection (if new) or zone selection (if modify)
            if (m_editingZoneId == -1) m_currentState = ControllerState::Select_ZoneType_ForNew;
            else {
                 // Need to know which modify state we came from
                 if (m_wipZoneType == ZoneType::Safety || m_wipZoneType == ZoneType::NoFire || m_wipZoneType == ZoneType::NoTraverse) m_currentState = ControllerState::Select_AreaZone_ToModify;
                 else if (m_wipZoneType == ZoneType::AutoSectorScan) m_currentState = ControllerState::Select_SectorScan_ToModify;
                 else m_currentState = ControllerState::Select_TRP_ToModify;
            }
            resetWipDataAndState();
            m_currentMenuIndex = 0;
            break;
        case ControllerState::AreaZone_Aim_Corner2:
            m_currentState = ControllerState::AreaZone_Aim_Corner1;
            m_corner1Defined = false; // Reset corner 1 capture
            break;
        case ControllerState::SectorScan_Aim_Point2:
             m_currentState = ControllerState::SectorScan_Aim_Point1;
             break;
        case ControllerState::AreaZone_Edit_Parameters:
            // BACK in parameter editing now acts like the Cancel button
            qDebug() << "BACK treated as Cancel in AreaZone parameters.";
            // Simulate activating and selecting the Cancel button
            if (m_areaZoneParameterPanel) {
                m_areaZoneParameterPanel->setActiveField(AreaZoneParameterPanel::Field::CancelButton);
                handleSelectAction(); // Trigger the cancel logic
                return; // updateUI called by handleSelectAction
            }
            // Fallback if panel doesn't exist
            m_currentState = ControllerState::AreaZone_Aim_Corner2;
            break;
        case ControllerState::SectorScan_Edit_Parameters:
        case ControllerState::TRP_Edit_Parameters:
             // Back from these panels goes back to the last aiming step
             if (m_wipZoneType == ZoneType::AutoSectorScan) m_currentState = ControllerState::SectorScan_Aim_Point2;
             else m_currentState = ControllerState::TRP_Aim_Point;
             break;
        case ControllerState::Confirm_Save:
        case ControllerState::Confirm_Delete:
            // Back from confirmation goes back to the previous step (editing or aiming)
            // This requires more complex state tracking or logic.
            // Simplification: Go back to main menu for now.
            m_currentState = ControllerState::Idle_MainMenu;
            resetWipDataAndState();
            m_currentMenuIndex = 0;
            break;
        case ControllerState::Show_Message:
            m_currentState = ControllerState::Idle_MainMenu;
            resetWipDataAndState();
            m_currentMenuIndex = 0;
            break;
        default:
             qWarning() << "handleBackAction called in unhandled state:" << static_cast<int>(m_currentState);
             m_currentState = ControllerState::Idle_MainMenu;
             resetWipDataAndState();
             m_currentMenuIndex = 0;
             break;
    }
    updateUI();
}

// updateUI needs to handle new states
void ZoneDefinitionWidget::updateUI()
{
    switch (m_currentState) {
        case ControllerState::Idle_MainMenu:
            setupIdleMainMenuUI();
            break;
        case ControllerState::Select_ZoneType_ForNew:
            setupSelectZoneTypeUI();
            break;
        case ControllerState::Select_ZoneType_ForModify:
            setupSelectZoneTypeForModifyDeleteUI("Modify");
            break;
        case ControllerState::Select_ZoneType_ForDelete:
            setupSelectZoneTypeForModifyDeleteUI("Delete");
            break;
        case ControllerState::Select_AreaZone_ToModify:
            setupSelectExistingZoneUI(ZoneType::Safety, "Modify Area Zone");
            break;
        case ControllerState::Select_SectorScan_ToModify:
            setupSelectExistingZoneUI(ZoneType::AutoSectorScan, "Modify Sector Scan Zone");
            break;
        case ControllerState::Select_TRP_ToModify:
            setupSelectExistingZoneUI(ZoneType::TargetReferencePoint, "Modify TRP");
            break;
        case ControllerState::Select_AreaZone_ToDelete:
            setupSelectExistingZoneUI(ZoneType::Safety, "Delete Area Zone");
            break;
        case ControllerState::Select_SectorScan_ToDelete:
            setupSelectExistingZoneUI(ZoneType::AutoSectorScan, "Delete Sector Scan Zone");
            break;
        case ControllerState::Select_TRP_ToDelete:
            setupSelectExistingZoneUI(ZoneType::TargetReferencePoint, "Delete TRP");
            break;
        case ControllerState::AreaZone_Aim_Corner1:
            setupAimPointUI("Aim at FIRST corner (Az/El) and press SELECT.");
            break;
        case ControllerState::AreaZone_Aim_Corner2:
            setupAimPointUI("Aim at SECOND corner (Az/El) and press SELECT.");
            break;
        case ControllerState::AreaZone_Edit_Parameters:
            setupAreaZoneParametersUI(m_editingZoneId == -1);
            break;
        case ControllerState::SectorScan_Aim_Point1:
            setupAimPointUI("Aim at Sector Scan START point (Az/El) and press SELECT.");
            break;
        case ControllerState::SectorScan_Aim_Point2:
            setupAimPointUI("Aim at Sector Scan END point (Az/El) and press SELECT.");
            break;
        case ControllerState::SectorScan_Edit_Parameters:
            setupSectorScanParametersUI(m_editingZoneId == -1);
            break;
        case ControllerState::TRP_Aim_Point:
            setupAimPointUI("Aim at Target Reference Point (Az/El) and press SELECT.");
            break;
        case ControllerState::TRP_Edit_Parameters:
            setupTRPParametersUI(m_editingZoneId == -1);
            break;
        case ControllerState::Confirm_Save:
            setupConfirmUI("Confirm Save", "Save Zone Definition?");
            break;
        case ControllerState::Confirm_Delete:
            setupConfirmUI("Confirm Delete", QString("Delete Zone ID %1?").arg(m_editingZoneId));
            break;
        case ControllerState::Show_Message:
            // UI is set by setupShowMessageUI directly
            break;
    }
    // Ensure map reflects current WIP state
    updateMapWipZone();
}

