#include "systemstatuswidget.h"
#include "../models/systemstatemodel.h" // For SystemStateData
#include <QFormLayout>
#include <QGroupBox>
#include <QKeyEvent>
#include <QDebug>

SystemStatusWidget::SystemStatusWidget(SystemStateModel *model, QWidget *parent)
    : BaseStyledWidget(model, parent), m_stateModel(model), m_currentFocusIndex(-1) // Init focus index
{
    setWindowTitle("System Status");
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // setAttribute(Qt::WA_DeleteOnClose); // Let parent handle deletion

    setupUi();

    if (m_stateModel) {
        connect(m_stateModel, &SystemStateModel::dataChanged,
                this, &SystemStatusWidget::onSystemStateChanged, Qt::QueuedConnection);
        //connect(m_stateModel, &SystemStateModel::colorStyleChanged, // Assuming this signal exists
        //        this, &SystemStatusWidget::onColorStyleChanged, Qt::QueuedConnection);

        // Populate with initial data & color
        populateData(m_stateModel->data());
        //setColorStyleChanged(m_stateModel->data().colorStyle); // Assuming helper in SystemStateData
    } else {
        qWarning() << "SystemStatusWidget created without a SystemStateModel!";
    }
}

SystemStatusWidget::~SystemStatusWidget() {
    qDebug() << "SystemStatusWidget destroyed";
}

void SystemStatusWidget::setupUi() {
    // Définir une taille minimale pour le widget principal
    setMinimumSize(600, 400);
    
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
    m_contentWidget->setMinimumSize(580, 380); // Définir une taille minimale

    // Layout principal pour le contenu
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(5, 5, 5, 5);

    // Layout pour les deux colonnes
    QHBoxLayout *columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(10);

    // --- Colonne gauche ---
    QVBoxLayout *leftColumnLayout = new QVBoxLayout();
    leftColumnLayout->setSpacing(10);

    // Groupe Gimbal
    QGroupBox *gimbalGroup = new QGroupBox("Gimbal Status", m_contentWidget);
    QFormLayout *gimbalLayout = new QFormLayout(gimbalGroup);
    m_azimuthLabelValue = new QLabel("N/A", gimbalGroup);
    m_elevationLabelValue = new QLabel("N/A", gimbalGroup);
    m_azDriverTempLabelValue = new QLabel("N/A", gimbalGroup);
    m_azMotorTempLabelValue = new QLabel("N/A", gimbalGroup);
    m_elDriverTempLabelValue = new QLabel("N/A", gimbalGroup);
    m_elMotorTempLabelValue = new QLabel("N/A", gimbalGroup);
    gimbalLayout->addRow("Azimuth:", m_azimuthLabelValue);
    gimbalLayout->addRow("Elevation:", m_elevationLabelValue);
    gimbalLayout->addRow("Az Drv Temp:", m_azDriverTempLabelValue);
    gimbalLayout->addRow("Az Mtr Temp:", m_azMotorTempLabelValue);
    gimbalLayout->addRow("El Drv Temp:", m_elDriverTempLabelValue);
    gimbalLayout->addRow("El Mtr Temp:", m_elMotorTempLabelValue);
    leftColumnLayout->addWidget(gimbalGroup);

    // Groupe LRF
    QGroupBox *lrfGroup = new QGroupBox("LRF Status", m_contentWidget);
    QFormLayout *lrfLayout = new QFormLayout(lrfGroup);
    m_lrfDistanceLabelValue = new QLabel("N/A", lrfGroup);
    m_lrfStatusLabelValue = new QLabel("N/A", lrfGroup);
    lrfLayout->addRow("Distance:", m_lrfDistanceLabelValue);
    lrfLayout->addRow("Status:", m_lrfStatusLabelValue);
    leftColumnLayout->addWidget(lrfGroup);

    // Ajouter un stretch pour pousser les groupes vers le haut
    leftColumnLayout->addStretch(1);
    columnsLayout->addLayout(leftColumnLayout, 1);

    // --- Colonne droite ---
    QVBoxLayout *rightColumnLayout = new QVBoxLayout();
    rightColumnLayout->setSpacing(10);

    // Groupe Camera
    QGroupBox *cameraGroup = new QGroupBox("Camera Status", m_contentWidget);
    QFormLayout *cameraLayout = new QFormLayout(cameraGroup);
    m_activeCameraLabelValue = new QLabel("N/A", cameraGroup);
    m_dayCamFovLabelValue = new QLabel("N/A", cameraGroup);
    m_nightCamFovLabelValue = new QLabel("N/A", cameraGroup);
    m_dayCamConnectedLabelValue = new QLabel("N/A", cameraGroup);
    m_nightCamConnectedLabelValue = new QLabel("N/A", cameraGroup);
    m_dayCamStatusLabelValue = new QLabel("N/A", cameraGroup);
    m_nightCamStatusLabelValue = new QLabel("N/A", cameraGroup);
    cameraLayout->addRow("Active Cam:", m_activeCameraLabelValue);
    cameraLayout->addRow("Day FOV:", m_dayCamFovLabelValue);
    cameraLayout->addRow("Night FOV:", m_nightCamFovLabelValue);
    cameraLayout->addRow("Day Cam:", m_dayCamConnectedLabelValue);
    cameraLayout->addRow("Night Cam:", m_nightCamConnectedLabelValue);
    cameraLayout->addRow("Day Status:", m_dayCamStatusLabelValue);
    cameraLayout->addRow("Night Status:", m_nightCamStatusLabelValue);
    rightColumnLayout->addWidget(cameraGroup);

    // Groupe PLC
    QGroupBox *plcGroup = new QGroupBox("PLC Status", m_contentWidget);
    QFormLayout *plcLayout = new QFormLayout(plcGroup);
    m_plcStationEnabledLabelValue = new QLabel("N/A", plcGroup);
    m_plcGunArmedLabelValue = new QLabel("N/A", plcGroup);
    plcLayout->addRow("Station Enabled:", m_plcStationEnabledLabelValue);
    plcLayout->addRow("Gun Armed:", m_plcGunArmedLabelValue);
    rightColumnLayout->addWidget(plcGroup);

    // Ajouter un stretch pour pousser les groupes vers le haut
    rightColumnLayout->addStretch(1);
    columnsLayout->addLayout(rightColumnLayout, 1);

    // Ajouter le layout des colonnes au layout principal
    contentLayout->addLayout(columnsLayout);

    // --- Section Alarmes ---
    QGroupBox *alarmsGroup = new QGroupBox("Alarms", m_contentWidget);
    QVBoxLayout *alarmsLayout = new QVBoxLayout(alarmsGroup);
    alarmsLayout->setSpacing(5);
    
    // Définir une taille fixe pour la liste d'alarmes
    m_alarmListWidget = new QListWidget(alarmsGroup);
    m_alarmListWidget->setFocusPolicy(Qt::NoFocus);
    m_alarmListWidget->setMinimumHeight(80);
    m_alarmListWidget->setMaximumHeight(120);
    
    // Ajouter quelques éléments de test pour vérifier l'affichage
    m_alarmListWidget->addItem("Test Alarm 1");
    m_alarmListWidget->addItem("Test Alarm 2");
    
    alarmsLayout->addWidget(m_alarmListWidget);
    
    m_clearAlarmsButton = new QPushButton("Clear Alarms", alarmsGroup);
    connect(m_clearAlarmsButton, &QPushButton::clicked, this, &SystemStatusWidget::onClearAlarmsClicked);
    alarmsLayout->addWidget(m_clearAlarmsButton);
    m_focusableItems.append(m_clearAlarmsButton);
    
    // Ajouter le groupe d'alarmes au layout principal
    contentLayout->addWidget(alarmsGroup);

    // --- Bouton de retour ---
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);
    
    m_returnButton = new QPushButton("Return to Menu", m_contentWidget);
    m_returnButton->setMinimumWidth(150);
    m_returnButton->setMaximumWidth(200);
    connect(m_returnButton, &QPushButton::clicked, this, [this]() { close(); });
    
    buttonLayout->addWidget(m_returnButton);
    buttonLayout->addStretch(1);
    
    contentLayout->addLayout(buttonLayout);
    m_focusableItems.append(m_returnButton);

    // Définir le layout sur le widget de contenu
    m_contentWidget->setLayout(contentLayout);
    
    // Ajouter le widget de contenu au QScrollArea
    m_scrollArea->setWidget(m_contentWidget);
    
    // Ajouter le QScrollArea au layout principal
    outerLayout->addWidget(m_scrollArea);
    
    // Définir le layout sur le widget principal
    setLayout(outerLayout);

    // Définir le focus initial
    if (!m_focusableItems.isEmpty()) {
        m_currentFocusIndex = 0;
        m_focusableItems.at(m_currentFocusIndex)->setFocus();
    }

    move(5, 100);
     // Debugging output to check sizes
    qDebug() << "Content widget size:" << m_contentWidget->size();
    qDebug() << "Alarm list size:" << m_alarmListWidget->size();
    qDebug() << "Return button size:" << m_returnButton->size();

}

void SystemStatusWidget::onSystemStateChanged(const SystemStateData &data) {
    populateData(data);
}

void SystemStatusWidget::populateData(const SystemStateData &data) {
    // Gimbal
    m_azimuthLabelValue->setText(QString::number(data.gimbalAz, 'f', 2) + "°");
    m_elevationLabelValue->setText(QString::number(data.gimbalEl, 'f', 2) + "°");
    m_azDriverTempLabelValue->setText(QString::number(data.azDriverTemp, 'f', 1) + "°C"); // Assuming fields exist
    m_azMotorTempLabelValue->setText(QString::number(data.azMotorTemp, 'f', 1) + "°C");
    m_elDriverTempLabelValue->setText(QString::number(data.elDriverTemp, 'f', 1) + "°C");
    m_elMotorTempLabelValue->setText(QString::number(data.elMotorTemp, 'f', 1) + "°C");

    // LRF
    m_lrfDistanceLabelValue->setText(QString::number(data.lrfDistance, 'f', 1) + " m");
    m_lrfStatusLabelValue->setText(QString::number(data.lrfSystemStatus)); // Assuming a status string

    // Camera
    m_activeCameraLabelValue->setText(data.activeCameraIsDay ? "Day" : "Night");
    m_dayCamFovLabelValue->setText(QString::number(data.dayCurrentHFOV, 'f', 1) + "°");
    m_nightCamFovLabelValue->setText(QString::number(data.nightCurrentHFOV, 'f', 1) + "°");
    m_dayCamConnectedLabelValue->setText(data.dayCameraConnected ? "Connected" : "Disconnected");

    m_nightCamConnectedLabelValue->setText(data.nightCameraConnected ? "Connected" : "Disconnected");
    m_dayCamStatusLabelValue->setText(QString::number(data.dayCameraStatus));
    m_nightCamStatusLabelValue->setText(QString::number(data.nightCameraStatus));


    // PLC
    m_plcStationEnabledLabelValue->setText(data.stationEnabled ? "Enabled" : "Disabled");
    m_plcGunArmedLabelValue->setText(data.gunArmed ? "ARMED" : "SAFE");

    // Alarms (Example - adapt to your actual alarm flags/list in SystemStateData)
    m_alarmListWidget->clear();
    if (data.emergencyStopActive) m_alarmListWidget->addItem("EMERGENCY STOP ACTIVE");
    //if (data.azimuthFault) m_alarmListWidget->addItem("Azimuth Drive Fault"); // Assuming such fields
    //if (data.elevationFault) m_alarmListWidget->addItem("Elevation Drive Fault");
    // Add more alarms based on SystemStateData fields
    if (m_alarmListWidget->count() == 0) {
        m_alarmListWidget->addItem("No Active Alarms");
    }
}


void SystemStatusWidget::focusNextItem(bool forward) {
    if (m_focusableItems.isEmpty()) return;

    // Remove focus style from the previously focused item (if any)
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_focusableItems.size()) {
        QPushButton *oldButton = qobject_cast<QPushButton*>(m_focusableItems.at(m_currentFocusIndex));
        if (oldButton) {
            // oldButton->setStyleSheet(... a style WITHOUT the focus highlight ...);
            // Or, if your stylesheet handles :focus well, just ensure it loses focus.
            // Qt usually handles this automatically when focus changes.
        }
    }

    if (forward) { // Move Down
        m_currentFocusIndex++;
        if (m_currentFocusIndex >= m_focusableItems.size()) {
            m_currentFocusIndex = 0; // Wrap around
        }
    } else { // Move Up
        m_currentFocusIndex--;
        if (m_currentFocusIndex < 0) {
            m_currentFocusIndex = m_focusableItems.size() - 1; // Wrap around
        }
    }

    // Apply focus to the new item
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_focusableItems.size()) {
        QWidget *newItem = m_focusableItems.at(m_currentFocusIndex);
        newItem->setFocus(Qt::OtherFocusReason); // Give it keyboard focus

        // Scroll to ensure the focused item is visible
        if (m_scrollArea) { // Check if scrollArea exists
            m_scrollArea->ensureWidgetVisible(newItem, 50, 50); // 50px margins
        }

        // The stylesheet should handle the :focus state automatically.
        // If you need more explicit styling:
        // QPushButton *newButton = qobject_cast<QPushButton*>(newItem);
        // if (newButton) {
        //    // newButton->setStyleSheet(... a style WITH the focus highlight ...);
        // }
    }
}

void SystemStatusWidget::moveSelectionUp() {
    focusNextItem(false);
}

void SystemStatusWidget::moveSelectionDown() {
    focusNextItem(true); 
}

void SystemStatusWidget::selectCurrentItem() {
    if (m_currentFocusIndex >= 0 && m_currentFocusIndex < m_focusableItems.size()) {
        QPushButton *button = qobject_cast<QPushButton*>(m_focusableItems.at(m_currentFocusIndex));
        if (button) {
            button->click(); // Simulate a click on the focused button
        }
    }
}

void SystemStatusWidget::onClearAlarmsClicked() {
    qDebug() << "SystemStatusWidget: Clear Alarms button clicked! Emitting request.";
    emit clearAlarmsRequested(); // Emit the signal
}

void SystemStatusWidget::closeEvent(QCloseEvent *event) {
    emit menuClosed();
    QWidget::closeEvent(event);
}

void SystemStatusWidget::keyPressEvent(QKeyEvent *event) {
    // Fallback for keyboard testing, but joystick methods are primary
    switch (event->key()) {
        case Qt::Key_Up: moveSelectionUp(); break;
        case Qt::Key_Down: moveSelectionDown(); break;
        case Qt::Key_Return:
        case Qt::Key_Enter: selectCurrentItem(); break;
        case Qt::Key_Escape: emit menuClosed(); break;
        default: QWidget::keyPressEvent(event);
    }
}
