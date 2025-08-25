#include "zeroingwidget.h"
#include "../models/systemstatemodel.h" // For SystemStateData
#include <QDebug>

ZeroingWidget::ZeroingWidget(SystemStateModel* model, QWidget *parent)
    : BaseStyledWidget(model, parent), m_stateModel(model), m_currentState(ZeroingState::Idle)
{
    Q_ASSERT(m_stateModel != nullptr);
    
    // Set window properties
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    
    // Set size constraints
    setMinimumSize(450, 350);
    setMaximumSize(600, 500);
    resize(500, 400);
    
    // Make sure it's modal-like behavior
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setAttribute(Qt::WA_DeleteOnClose);
    
    initializeUI();
    
    // Connect to model changes
    connect(m_stateModel, &SystemStateModel::dataChanged, 
            this, &ZeroingWidget::onModelStateChanged);
}

void ZeroingWidget::initializeUI() {
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
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(20, 20, 20, 20);

    // Title Label
    m_titleLabel = new QLabel("Weapon Zeroing", m_contentWidget);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    contentLayout->addWidget(m_titleLabel, 0);

    // Instruction Label
    m_instructionLabel = new QLabel("", m_contentWidget);
    m_instructionLabel->setAlignment(Qt::AlignCenter);
    m_instructionLabel->setWordWrap(true);
    m_instructionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_instructionLabel->setMinimumHeight(150);
    contentLayout->addWidget(m_instructionLabel, 1);

    // Status Label
    m_statusLabel = new QLabel("", m_contentWidget);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont statusFont = m_statusLabel->font();
    statusFont.setBold(true);
    m_statusLabel->setFont(statusFont);
    contentLayout->addWidget(m_statusLabel, 0);

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

void ZeroingWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    m_stateModel->startZeroingProcedure(); // Inform model we are starting
    m_currentState = ZeroingState::Instruct_MoveReticleToImpact; // Jump to key instruction
    updateUI();
    // MainWindow should ensure joystick controls gimbal for aiming
}

 

void ZeroingWidget::updateUI() {
    switch (m_currentState) {
        case ZeroingState::Instruct_MoveReticleToImpact:
            m_titleLabel->setText("Weapon Zeroing: Adjust");
            m_instructionLabel->setText("ZEROING\n\n1. (Fire weapon at a fixed target)\n"
                                        "2. Observe impact point.\n"
                                        "3. Use JOYSTICK to move main RETICLE to the ACTUAL IMPACT POINT.\n\n"
                                        "Press SELECT to set this as the new zero.");
            m_statusLabel->setText("ADJUSTING RETICLE TO IMPACT");
            break;

        case ZeroingState::Completed: // <<< UI FOR COMPLETED STATE
            m_titleLabel->setText("Zeroing Applied");
            m_instructionLabel->setText("Zeroing Adjustment Applied!\n'Z' will display on OSD when active.\n"
                                        "You may need to repeat the process if further adjustment is required.\n"
                                        "\nPress SELECT or BACK to return to Settings Menu.");
            // Display current offsets to confirm what was applied
            if (m_stateModel) { // Check if model is valid
                m_statusLabel->setText(QString("FINAL OFFSETS: Az %1, El %2")
                                       .arg(m_stateModel->data().zeroingAzimuthOffset, 0, 'f', 2)
                                       .arg(m_stateModel->data().zeroingElevationOffset, 0, 'f', 2));
            } else {
                m_statusLabel->setText("ZEROING APPLIED (Model invalid)");
            }
            break;

        case ZeroingState::Idle: // Fallback or initial state before showEvent kicks in
        default:
             m_titleLabel->setText("Weapon Zeroing");
             m_instructionLabel->setText("Zeroing Standby. Press BACK to exit this screen if entered unintentionally.");
             m_statusLabel->setText("");
            break;
    }
    // OSD Renderer will show "Z" or "ZEROING" based on
    // m_stateModel->data().zeroingModeActive / m_stateModel->data().zeroingAppliedToBallistics
}

void ZeroingWidget::onModelStateChanged(const SystemStateData &data) {
    // If zeroing is externally cancelled, or to update displayed offsets if in a fine-tuning state
    if (!data.zeroingModeActive && m_currentState != ZeroingState::Idle && m_currentState != ZeroingState::Completed) {
        qDebug() << "Zeroing mode became inactive externally, finishing widget.";
        this->close();
        emit zeroingProcedureFinished();
    }

    // If you had a FineTune_Adjustments state, you would update m_statusLabel here
    // with data.zeroingAzimuthOffset and data.zeroingElevationOffset.
    // For the "Completed" state, updateUI already fetches the final offsets.
}

void ZeroingWidget::handleSelectAction() {
    switch (m_currentState) {
        case ZeroingState::Instruct_MoveReticleToImpact:
            // User has aimed the reticle at the impact point.
            // This SELECT confirms that the current reticle position *is* the new zero.
            // The SystemStateModel::finalizeZeroing() should ideally take the current
            // gimbal/reticle state and calculate the necessary offsets if it doesn't
            // implicitly assume the weapon controller does this.
            // For our model, finalizeZeroing mainly sets the 'zeroingAppliedToBallistics' flag.
            // The actual offsets should have been accumulated if we had a fine-tuning step,
            // or the underlying system understands "current reticle position IS the zero".

            m_stateModel->finalizeZeroing(); // This will set zeroingAppliedToBallistics = true
                                          // and zeroingModeActive = false in the model.

            qDebug() << "Zeroing finalized. Offsets Az:" << m_stateModel->data().zeroingAzimuthOffset
                     << "El:" << m_stateModel->data().zeroingElevationOffset;

            m_currentState = ZeroingState::Completed; // Transition to the "Completed" screen
            updateUI(); // Update UI to show completion message
            break;

        case ZeroingState::Completed:
            // If SELECT is pressed on the "Completed" screen, it means "OK, I'm done reading this".
            this->close(); // Close the widget
            emit zeroingProcedureFinished(); // Signal MainWindow to close this widget
            break;

        // If you had FineTune_Adjustments state:
        // case ZeroingState::FineTune_Adjustments:
        //     m_stateModel->finalizeZeroing();
        //     m_currentState = ZeroingState::Completed;
        //     updateUI();
        //     break;

        default:
            qWarning() << "ZeroingWidget::handleSelectAction() unhandled for state:" << static_cast<int>(m_currentState);
            break;
    }
}

// handleBackAction() remains the same as the corrected version from the previous response:
void ZeroingWidget::handleBackAction() {
    SystemStateData currentData = m_stateModel->data();

    if (currentData.zeroingModeActive) {
        if (!currentData.zeroingAppliedToBallistics && m_currentState != ZeroingState::Completed) {
            m_stateModel->clearZeroing();
        } else {
            // If zeroing was applied or we are on "Completed" screen, just update UI mode flag in model
            SystemStateData updatedData = currentData;
            updatedData.zeroingModeActive = false;
            m_stateModel->updateData(updatedData);
            qDebug() << "ZeroingWidget: Exiting UI, applied zeroing (if any) remains.";
        }
    }
    this->close(); // Close the widget
    emit zeroingProcedureFinished();
}

// handleUpAction / handleDownAction could be used if you implement
// discrete +/- 3 degree fine-tuning steps AFTER initial reticle placement.
// For that, you'd need a new state like ZeroingState::FineTuneAdjustments.
void ZeroingWidget::handleUpAction() { /* TODO if fine-tuning needed */ }
void ZeroingWidget::handleDownAction() { /* TODO if fine-tuning needed */ }

 

