#include "radartargetlistwidget.h"
#include <QDebug>
#include <QKeyEvent>

RadarTargetListWidget::RadarTargetListWidget(SystemStateModel* model, QWidget *parent)
    : BaseStyledWidget(model, parent) // <<< CALL BASE CONSTRUCTOR
{
    // The BaseStyledWidget constructor already handles the m_stateModel pointer
    Q_ASSERT(m_stateModel != nullptr);
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    initializeUI();

    // Connect to the model to receive updates
    connect(m_stateModel, &SystemStateModel::dataChanged, this, &RadarTargetListWidget::onSystemStateChanged);
}

void RadarTargetListWidget::initializeUI() {
    // This looks very similar to CustomMenuWidget::setupUi
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);
    setLayout(m_mainLayout);

    m_titleLabel = new QLabel("Radar Targets", this);
    m_titleLabel->setObjectName("menuTitle"); // Use same object name for consistent styling
    m_mainLayout->addWidget(m_titleLabel);

    QFrame* separatorLine = new QFrame(this);
    separatorLine->setFrameShape(QFrame::HLine);
    separatorLine->setObjectName("menuSeparator");
    m_mainLayout->addWidget(separatorLine);

    m_targetListWidget = new QListWidget(this);
    m_targetListWidget->setObjectName("menuListWidget"); // Use same object name for consistent styling
    m_mainLayout->addWidget(m_targetListWidget, 1); // Stretch factor

    m_navigationHintLabel = new QLabel("UP/DOWN: Select Target | MENU: Slew to Target", this);
    m_navigationHintLabel->setObjectName("navigationHints");
    m_mainLayout->addWidget(m_navigationHintLabel);
}

void RadarTargetListWidget::showEvent(QShowEvent* event) {
    BaseStyledWidget::showEvent(event); // Call base class method
    // When shown, immediately populate with the latest data
    onSystemStateChanged(m_stateModel->data());
    setFocus();
}

void RadarTargetListWidget::onSystemStateChanged(const SystemStateData& data) {
    // Only update if the relevant parts of the state have changed
    if (data.radarPlots != m_currentlyDisplayedPlots || data.selectedRadarTrackId != m_currentlyDisplayedSelectedId) {
        updateListDisplay(data.radarPlots, data.selectedRadarTrackId);
    }
}

void RadarTargetListWidget::updateListDisplay(const QVector<SimpleRadarPlot>& plots, quint32 selectedId) {
    m_targetListWidget->blockSignals(true);
    m_targetListWidget->clear();

    if (plots.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("No Radar Targets Detected");
        item->setFlags(Qt::NoItemFlags); // Make it non-selectable
        item->setTextAlignment(Qt::AlignCenter);
        m_targetListWidget->addItem(item);
    } else {
        for (int i = 0; i < plots.size(); ++i) {
            const auto& plot = plots.at(i);
            QString itemText = QString("ID: %1 | Az: %2° | Rng: %3 m")
                                   .arg(plot.id)
                                   .arg(plot.azimuth, 0, 'f', 1)
                                   .arg(plot.range, 0, 'f', 0);
            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, plot.id); // Store the ID in the item for easy retrieval
            m_targetListWidget->addItem(item);

            if (plot.id == selectedId) {
                m_targetListWidget->setCurrentRow(i);
            }
        }
    }
    m_targetListWidget->blockSignals(false);

    m_currentlyDisplayedPlots = plots;
    m_currentlyDisplayedSelectedId = selectedId;
}

void RadarTargetListWidget::moveSelectionUp() {
    if(m_stateModel) m_stateModel->selectPreviousRadarTrack();
}

void RadarTargetListWidget::moveSelectionDown() {
    if(m_stateModel) m_stateModel->selectNextRadarTrack();
}

void RadarTargetListWidget::selectCurrentItem() {
    if (!m_stateModel || m_targetListWidget->currentRow() < 0) return;

    QListWidgetItem* currentItem = m_targetListWidget->currentItem();
    if (currentItem && (currentItem->flags() & Qt::ItemIsSelectable)) {
        quint32 targetId = currentItem->data(Qt::UserRole).toUInt();
        if (targetId != 0) {
            qDebug() << "RadarTargetListWidget: Requesting slew to target ID" << targetId;
            m_stateModel->commandSlewToSelectedRadarTrack();
        }
    }
}

void RadarTargetListWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Up:    moveSelectionUp(); break;
    case Qt::Key_Down:  moveSelectionDown(); break;
    case Qt::Key_Return:
    case Qt::Key_Enter: selectCurrentItem(); break;
    case Qt::Key_Escape:
    case Qt::Key_Backspace: emit widgetClosed(); break; // Emit signal to be closed by MainWindow
    default: BaseStyledWidget::keyPressEvent(event);
    }
}
