#include "custommenudialog.h"
#include "../models/systemstatemodel.h"
#include <QCloseEvent>
#include <QShowEvent>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QDebug>
#include <QApplication>

CustomMenuWidget::CustomMenuWidget(const QStringList &options,
                                   SystemStateModel *model,
                                   QWidget *parent,
                                   const QString &title,
                                   const QString &description)
    : BaseStyledWidget(model, parent)
    , m_stateModel(model)
    , m_options(options)
    , m_listWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_headerWidget(nullptr)
    , m_footerWidget(nullptr)
    , m_titleLabel(nullptr)
    , m_descriptionLabel(nullptr)
    , m_iconLabel(nullptr)
    , m_navigationHintLabel(nullptr)
    , m_separatorLine(nullptr)
    , m_menuTitle(title)
    , m_menuDescription(description)
    , m_navigationHintsEnabled(true)
    , m_menuEnabled(true)
    , m_autoCloseTimer(nullptr)
    , m_autoCloseTimeout(0)
{
    // Window configuration
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setStyleSheet("background-color: rgba(0, 0, 0, 200);");

    initializeDefaults();
    validateConfiguration();
    setupUi();
}

void CustomMenuWidget::initializeDefaults()
{
    if (m_menuTitle.isEmpty()) {
        m_menuTitle = "Menu Options";
    }

    if (m_menuDescription.isEmpty() && !m_options.isEmpty()) {
        m_menuDescription = QString("Select from %1 available options").arg(m_options.size());
    }
}

void CustomMenuWidget::validateConfiguration()
{
    if (!validateMenuOptions()) {
        qWarning() << "CustomMenuWidget: Invalid menu configuration detected";
        handleInvalidState();
        return;
    }

    if (!m_stateModel) {
        qWarning() << "CustomMenuWidget: StateModel is null";
    }
}

bool CustomMenuWidget::validateMenuOptions() const
{
    if (m_options.isEmpty()) {
        qDebug() << "CustomMenuWidget: No menu options provided";
        return false;
    }

    // Check for duplicate options
    QStringList uniqueOptions = m_options;
    uniqueOptions.removeDuplicates();
    if (uniqueOptions.size() != m_options.size()) {
        qWarning() << "CustomMenuWidget: Duplicate menu options detected";
    }

    return true;
}

void CustomMenuWidget::handleInvalidState()
{
    m_menuEnabled = false;
    // Could emit an error signal here if needed
}

void CustomMenuWidget::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 0, 0, 5);
    m_mainLayout->setSpacing(0);

    setupHeader();
    setupListWidget();
    setupFooter();
    //setupAnimations();

    // Set default size and position
    resize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    move(5, 180);
}

void CustomMenuWidget::setupHeader()
{
    m_headerWidget = new QWidget(this);
    m_headerWidget->setFixedHeight(HEADER_HEIGHT);

    QHBoxLayout *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(5, 5, 5, 5);

    // Icon (if provided)
    m_iconLabel = new QLabel(m_headerWidget);
    m_iconLabel->setFixedSize(24, 24);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setVisible(!m_menuIcon.isNull());

    // Title and description
    QVBoxLayout *textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    m_titleLabel = new QLabel(m_menuTitle, m_headerWidget);
    m_titleLabel->setObjectName("menuTitle");
     m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_descriptionLabel = new QLabel(m_menuDescription, m_headerWidget);
    m_descriptionLabel->setObjectName("menuDescription");
    m_descriptionLabel->setVisible(!m_menuDescription.isEmpty());
    m_descriptionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_descriptionLabel);

    headerLayout->addWidget(m_iconLabel);
    headerLayout->addLayout(textLayout);
    headerLayout->addLayout(textLayout, 1); 

    // Separator line
    m_separatorLine = new QFrame(this);
    m_separatorLine->setFrameShape(QFrame::HLine);
    m_separatorLine->setObjectName("menuSeparator");

    m_mainLayout->addWidget(m_headerWidget);
    m_mainLayout->addWidget(m_separatorLine);
}

void CustomMenuWidget::setupListWidget()
{
    m_listWidget = new QListWidget(this);

    // Populate with items and handle separators
    for (const QString& option : m_options) {
        QListWidgetItem* item = new QListWidgetItem(option);

        // Check if this is a separator
        if (option.startsWith("---") && option.endsWith("---")) {
            // Style separator items
            item->setFlags(Qt::NoItemFlags); // Make non-selectable
            item->setTextAlignment(Qt::AlignCenter);

            // Separator styling
            QFont font = item->font();
            font.setBold(true);
            font.setItalic(true);
            font.setPointSize(font.pointSize() - 1);
            item->setFont(font);

            // Use a subtle color for separators
            item->setForeground(QColor(150, 150, 150));
            item->setBackground(QColor(40, 40, 40, 100)); // Subtle background

            // Clean up the separator text (remove --- ---)
            QString cleanText = option.mid(4, option.length() - 8).trimmed();
            item->setText(cleanText);
        }

        m_listWidget->addItem(item);
    }

    // Set initial selection to first selectable item
    setInitialSelection();

    // Connect signals
    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &CustomMenuWidget::onListItemChanged);

    m_mainLayout->addWidget(m_listWidget, 1); // Give it stretch factor
}

void CustomMenuWidget::setInitialSelection()
{
    // Find first selectable item
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QListWidgetItem* item = m_listWidget->item(i);
        if (item && (item->flags() & Qt::ItemIsSelectable)) {
            m_listWidget->setCurrentRow(i);
            break;
        }
    }
}

void CustomMenuWidget::setCurrentSelection(int index)
{
    if (m_listWidget && index >= 0 && index < m_listWidget->count()) {
        // Make sure the item at this index is selectable
        QListWidgetItem* item = m_listWidget->item(index);
        if (item && (item->flags() & Qt::ItemIsSelectable)) {
            m_listWidget->setCurrentRow(index);
            highlightCurrentItem();
        }
    }
}

void CustomMenuWidget::setupFooter()
{
    m_footerWidget = new QWidget(this);
    m_footerWidget->setFixedHeight(FOOTER_HEIGHT);

    QHBoxLayout *footerLayout = new QHBoxLayout(m_footerWidget);
    footerLayout->setContentsMargins(5, 5, 5, 5);

    m_navigationHintLabel = new QLabel("- UP/DOWN: Navigate <br> - MENU: Select", m_footerWidget);
    m_navigationHintLabel->setObjectName("navigationHints");
    m_navigationHintLabel->setVisible(m_navigationHintsEnabled);
    m_navigationHintLabel->setWordWrap(true); // Enable wrapping for multi-line
    m_navigationHintLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred); // Expand horizontally

    footerLayout->addWidget(m_navigationHintLabel);

    m_mainLayout->addWidget(m_footerWidget);
}

// Keep existing method names and functionality
void CustomMenuWidget::moveSelectionUp()
{
    if (!m_menuEnabled || !m_listWidget) return;

    int currentRow = m_listWidget->currentRow();
    int newRow = currentRow;

    // Find previous selectable item
    do {
        newRow--;
        if (newRow < 0) {
            // Wrap to last selectable item
            newRow = m_listWidget->count() - 1;
            while (newRow >= 0 && !(m_listWidget->item(newRow)->flags() & Qt::ItemIsSelectable)) {
                newRow--;
            }
            break;
        }
    } while (newRow >= 0 && !(m_listWidget->item(newRow)->flags() & Qt::ItemIsSelectable));

    if (newRow >= 0 && newRow != currentRow) {
        m_listWidget->setCurrentRow(newRow);

        QListWidgetItem *item = m_listWidget->currentItem();
        if (item) {
            emit currentItemChanged(item->text());
            emit selectionChanged(m_listWidget->currentRow(), item->text());
        }
    }
}

void CustomMenuWidget::moveSelectionDown()
{
    if (!m_menuEnabled || !m_listWidget) return;

    int currentRow = m_listWidget->currentRow();
    int newRow = currentRow;

    // Find next selectable item
    do {
        newRow++;
        if (newRow >= m_listWidget->count()) {
            // Wrap to first selectable item
            newRow = 0;
            while (newRow < m_listWidget->count() && !(m_listWidget->item(newRow)->flags() & Qt::ItemIsSelectable)) {
                newRow++;
            }
            break;
        }
    } while (newRow < m_listWidget->count() && !(m_listWidget->item(newRow)->flags() & Qt::ItemIsSelectable));

    if (newRow < m_listWidget->count() && newRow != currentRow) {
        m_listWidget->setCurrentRow(newRow);

        QListWidgetItem *item = m_listWidget->currentItem();
        if (item) {
            emit currentItemChanged(item->text());
            emit selectionChanged(m_listWidget->currentRow(), item->text());
        }
    }
}

bool CustomMenuWidget::isValidSelection() const
{
    if (!m_listWidget || !m_listWidget->currentItem()) {
        return false;
    }

    QListWidgetItem* currentItem = m_listWidget->currentItem();
    return currentItem &&
           (currentItem->flags() & Qt::ItemIsSelectable) &&
           m_listWidget->currentRow() >= 0 &&
           m_listWidget->currentRow() < m_listWidget->count();
}

void CustomMenuWidget::selectCurrentItem()
{
    if (!m_menuEnabled || !isValidSelection()) return;

    QListWidgetItem *item = m_listWidget->currentItem();
    if (item) {
        emit optionSelected(item->text());

        // Emit about to close signal for cleanup
        emit menuAboutToClose();

        close();
        emit menuClosed();
    }
}

QString CustomMenuWidget::currentItemText() const
{
    if (!m_listWidget) return QString();

    QListWidgetItem *item = m_listWidget->currentItem();
    return item ? item->text() : QString();
}

// Professional enhancement methods
void CustomMenuWidget::setMenuTitle(const QString &title)
{
    m_menuTitle = title;
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void CustomMenuWidget::setMenuDescription(const QString &description)
{
    m_menuDescription = description;
    if (m_descriptionLabel) {
        m_descriptionLabel->setText(description);
        m_descriptionLabel->setVisible(!description.isEmpty());
    }
}

void CustomMenuWidget::setMenuIcon(const QPixmap &icon)
{
    m_menuIcon = icon;
    if (m_iconLabel) {
        m_iconLabel->setPixmap(icon);
        m_iconLabel->setVisible(!icon.isNull());
    }
}

void CustomMenuWidget::setAutoCloseTimeout(int milliseconds)
{
    m_autoCloseTimeout = milliseconds;

    if (milliseconds > 0) {
        if (!m_autoCloseTimer) {
            m_autoCloseTimer = new QTimer(this);
            connect(m_autoCloseTimer, &QTimer::timeout,
                    this, &CustomMenuWidget::onAutoCloseTimeout);
        }
        m_autoCloseTimer->start(milliseconds);
    } else if (m_autoCloseTimer) {
        m_autoCloseTimer->stop();
    }
}

void CustomMenuWidget::setNavigationHints(bool enabled)
{
    m_navigationHintsEnabled = enabled;
    if (m_navigationHintLabel) {
        m_navigationHintLabel->setVisible(enabled);
    }
}

int CustomMenuWidget::getCurrentIndex() const
{
    return m_listWidget ? m_listWidget->currentRow() : -1;
}

int CustomMenuWidget::getMenuItemCount() const
{
    return m_listWidget ? m_listWidget->count() : 0;
}

void CustomMenuWidget::setMenuEnabled(bool enabled)
{
    m_menuEnabled = enabled;
    if (m_listWidget) {
        m_listWidget->setEnabled(enabled);
    }
}

void CustomMenuWidget::highlightCurrentItem()
{
    if (!m_listWidget || !m_listWidget->currentItem()) return;

    // Ensure current item is visible
    m_listWidget->scrollToItem(m_listWidget->currentItem());
}

void CustomMenuWidget::onAutoCloseTimeout()
{
    qDebug() << "CustomMenuWidget: Auto-close timeout reached";
    close();
}

void CustomMenuWidget::onListItemChanged()
{
    highlightCurrentItem();
}


void CustomMenuWidget::cleanupResources()
{
    if (m_autoCloseTimer) {
        m_autoCloseTimer->stop();
    }
}

void CustomMenuWidget::closeEvent(QCloseEvent *event)
{
    cleanupResources();
    emit menuAboutToClose();
    BaseStyledWidget::closeEvent(event);
    emit menuClosed();
}

void CustomMenuWidget::showEvent(QShowEvent *event)
{
    BaseStyledWidget::showEvent(event);

    if (m_autoCloseTimeout > 0 && m_autoCloseTimer) {
        m_autoCloseTimer->start(m_autoCloseTimeout);
    }

    // Ensure first item is properly selected
    if (m_listWidget && m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
        highlightCurrentItem();
    }
}

void CustomMenuWidget::keyPressEvent(QKeyEvent *event)
{
    // Handle keyboard input for testing purposes
    // Your physical buttons will call the move/select methods directly
    switch (event->key()) {
    case Qt::Key_Up:
        moveSelectionUp();
        break;
    case Qt::Key_Down:
        moveSelectionDown();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        selectCurrentItem();
        break;
    case Qt::Key_Escape:
        close();
        break;
    default:
        BaseStyledWidget::keyPressEvent(event);
    }
}
