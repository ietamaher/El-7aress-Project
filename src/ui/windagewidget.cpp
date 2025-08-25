#include "windagewidget.h"
#include <QDebug>

WindageWidget::WindageWidget(SystemStateModel* model, QWidget *parent)
    : BaseStyledWidget(model, parent), m_stateModel(model), m_currentState(WindageState::Instruct_AlignToWind), m_currentWindSpeedEdit(0.0f)
{
    Q_ASSERT(m_stateModel != nullptr);
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setMinimumSize(400, 300);  // Add this
    initializeUI();
    connect(m_stateModel, &SystemStateModel::dataChanged, this, &WindageWidget::onModelStateChanged);
}

void WindageWidget::initializeUI() {
    // Set minimum size like SystemStatusWidget
    setMinimumSize(600, 400);

    // Outer layout with same margins and spacing
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(5, 5, 5, 5);
    outerLayout->setSpacing(10);

    // Configure QScrollArea (same as SystemStatusWidget)
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFocusPolicy(Qt::NoFocus);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    // Create content widget with minimum size
    m_contentWidget = new QWidget();
    m_contentWidget->setMinimumSize(580, 380);

    // Main content layout
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(5, 5, 5, 5);

    // Title Label
    m_titleLabel = new QLabel("Windage Setting", m_contentWidget);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    contentLayout->addWidget(m_titleLabel);

    // Instruction Label
    m_instructionLabel = new QLabel("", m_contentWidget);
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setMinimumHeight(150);
    contentLayout->addWidget(m_instructionLabel);

    // Wind Speed Display
    m_windSpeedDisplayLabel = new QLabel("Headwind: 0 knots", m_contentWidget);
    m_windSpeedDisplayLabel->setAlignment(Qt::AlignCenter);
    QFont windFont = m_windSpeedDisplayLabel->font();
    windFont.setBold(true);
    m_windSpeedDisplayLabel->setFont(windFont);
    contentLayout->addWidget(m_windSpeedDisplayLabel);

    // Add stretch to push content up
    contentLayout->addStretch(1);

    // Return button layout (following SystemStatusWidget pattern)
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);

    m_returnButton = new QPushButton("Return to Menu", m_contentWidget);
    m_returnButton->setMinimumWidth(150);
    m_returnButton->setMaximumWidth(200);
    connect(m_returnButton, &QPushButton::clicked, this, [this]() { close(); });

    buttonLayout->addWidget(m_returnButton);
    buttonLayout->addStretch(1);
    contentLayout->addLayout(buttonLayout);

    // Set layout on content widget
    m_contentWidget->setLayout(contentLayout);

    // Add content widget to scroll area
    m_scrollArea->setWidget(m_contentWidget);

    // Add scroll area to outer layout
    outerLayout->addWidget(m_scrollArea);

    // Set layout on main widget
    setLayout(outerLayout);

    // Position window (same as SystemStatusWidget)
    move(5, 100);
}

void WindageWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    m_stateModel->startWindageProcedure();
    m_currentWindSpeedEdit = m_stateModel->data().windageSpeedKnots; // Initialize with current model value
    m_currentState = WindageState::Instruct_AlignToWind;
    updateUI();
}

/*void WindageWidget::updateUI() {
    switch (m_currentState) {
        case WindageState::Instruct_AlignToWind:
            m_instructionLabel->setText("Align Weapon Station TOWARDS THE WIND using joystick.\n\nPress SELECT when aligned.");
            m_windSpeedDisplayLabel->setVisible(false);
            break;
        case WindageState::Set_WindSpeed:
            m_instructionLabel->setText("Set HEADWIND speed.\nUse UP/DOWN to adjust. Press SELECT to confirm.");
            m_windSpeedDisplayLabel->setText(QString("Headwind: %1 knots").arg(m_currentWindSpeedEdit, 0, 'f', 0));
            m_windSpeedDisplayLabel->setVisible(true);
            break;
    }
    // OSD Renderer shows "W" based on model state
}*/

void WindageWidget::onModelStateChanged(const SystemStateData &data) {
    if (!data.windageModeActive && m_currentState != WindageState::Instruct_AlignToWind /* or some initial state */) {
        // emit windageProcedureFinished();
    }
     // Update our editing value if model changes (e.g. from another source, though unlikely for this UI)
    // if (m_currentState == WindageState::Set_WindSpeed && m_currentWindSpeedEdit != data.windageSpeedKnots) {
    //     m_currentWindSpeedEdit = data.windageSpeedKnots;
    //     updateUI(); // Refresh display
    // }
}

void WindageWidget::handleUpAction() {
    if (m_currentState == WindageState::Set_WindSpeed) {
        m_currentWindSpeedEdit += 1.0f; // Increment by 1 knot
        if (m_currentWindSpeedEdit > 50) m_currentWindSpeedEdit = 50; // Max example
        updateUI(); // Update display label
    }
}

void WindageWidget::handleDownAction() {
    if (m_currentState == WindageState::Set_WindSpeed) {
        m_currentWindSpeedEdit -= 1.0f;
        if (m_currentWindSpeedEdit < 0) m_currentWindSpeedEdit = 0;
        updateUI();
    }
}
 

void WindageWidget::handleSelectAction() {
    switch (m_currentState) {
        case WindageState::Instruct_AlignToWind:
            m_currentWindSpeedEdit = m_stateModel->data().windageSpeedKnots;
            m_currentState = WindageState::Set_WindSpeed;
            updateUI();
            break;

        case WindageState::Set_WindSpeed:
            m_stateModel->setWindageSpeed(m_currentWindSpeedEdit);
            m_stateModel->finalizeWindage();
            //m_instructionLabel->setText(QString("Windage set to %1 knots.\n'W' displayed on OSD when active.\n\nPress BACK or MENU ON/OFF to exit.").arg(m_currentWindSpeedEdit));
            qDebug() << "Windage finalized with speed:" << m_currentWindSpeedEdit;

            // Change state to a "Completed" message state
            m_currentState = WindageState::Completed; // << NEW OR REPURPOSED STATE
            updateUI(); // Update to show the completion message

            // Optional: If you want it to auto-close after showing the message
            // QTimer::singleShot(2000, this, &WindageWidget::windageProcedureFinished);
            // If no auto-close, the user presses BACK to exit the "Completed" screen.
            break;

        // Add a Completed state if you want a persistent message screen
        case WindageState::Completed:
            // If SELECT is pressed while on the "Completed" screen,
            // it could also trigger the finish/close.
             this->close();
            emit windageProcedureFinished();
            break;
    }
}

void WindageWidget::handleBackAction() {
    SystemStateData currentData = m_stateModel->data(); // Get a copy

    if (currentData.windageModeActive) {
        if (!currentData.windageAppliedToBallistics && m_currentState != WindageState::Completed) {
            // If user backs out before finalizing windage, and not on "Completed" screen, clear it.
            m_stateModel->clearWindage(); // This sets windageModeActive = false and resets speed.
        } else {
            // If windage was already applied or we are on "Completed" screen,
            // just signal that the UI interaction for windage is done.
            SystemStateData updatedData = currentData;
            updatedData.windageModeActive = false;    // Only change the UI mode flag
            m_stateModel->updateData(updatedData);       // Set the modified data back
            qDebug() << "WindageWidget: Exiting UI, applied windage (if any) remains.";
        }
    }
    // Always emit finished when Back is pressed
     this->close();
    emit windageProcedureFinished();
}

void WindageWidget::updateUI() {
    switch (m_currentState) {
        case WindageState::Instruct_AlignToWind:
            m_titleLabel->setText("Windage (1/2): Alignment");
            m_instructionLabel->setText("Align Weapon Station TOWARDS THE WIND using joystick.\n\nPress SELECT when aligned.");
            m_windSpeedDisplayLabel->setVisible(false);
            break;
        case WindageState::Set_WindSpeed:
            m_titleLabel->setText("Windage (2/2): Speed");
            m_instructionLabel->setText("Set HEADWIND speed.\nUse UP/DOWN to adjust. Press SELECT to confirm.");
            m_windSpeedDisplayLabel->setText(QString("Headwind: %1 knots").arg(m_currentWindSpeedEdit, 0, 'f', 0));
            m_windSpeedDisplayLabel->setVisible(true);
            break;
        case WindageState::Completed: // << UI FOR COMPLETED STATE
            m_titleLabel->setText("Windage Set");
            m_instructionLabel->setText(QString("Windage set to %1 knots and applied.\n'W' will display on OSD.\n\nPress BACK or SELECT to return.").arg(m_stateModel->data().windageSpeedKnots)); // Show actual model value
            m_windSpeedDisplayLabel->setText(QString("Headwind: %1 knots (APPLIED)").arg(m_stateModel->data().windageSpeedKnots, 0, 'f', 0));
            m_windSpeedDisplayLabel->setVisible(true);
            break;
    }
}

 
