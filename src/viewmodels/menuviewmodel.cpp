#include "menuviewmodel.h"
#include "../models/systemstatemodel.h"
#include "../utils/colorutils.h"
#include <QDebug>

MenuViewModel::MenuViewModel(SystemStateModel *stateModel, QObject *parent)
    : QObject(parent)
    , m_stateModel(stateModel)
    , m_currentMenuState(Idle)
    , m_currentSelection(0)
    , m_menuTitle("")
    , m_menuDescription("")
    , m_accentColor("#46E2A5")
    , m_previousMenuState(Idle)
{
    // Connect to state model for color updates
    if (m_stateModel) {
        connect(m_stateModel, &SystemStateModel::dataChanged, this, [this]() {
            QColor newColor = Qt::red ; //ColorUtils::toQColor(m_stateModel->data().colorStyle);
            if (newColor != m_accentColor) {
                m_accentColor = newColor;
                emit accentColorChanged();
            }
        });
    }
}

// ============================================================================
// NAVIGATION METHODS
// ============================================================================

void MenuViewModel::moveUp()
{
    if (m_menuOptions.isEmpty()) return;

    // Handle brightness menu specially (increase brightness)
    if (m_currentMenuState == BrightnessMenu) {
        emit requestIncreaseBrightness();
        return;
    }

    // Standard menu navigation
    int newIndex = m_currentSelection - 1;
    if (newIndex < 0) {
        newIndex = m_menuOptions.count() - 1; // Wrap to bottom
    }
    setCurrentSelection(newIndex);
}

void MenuViewModel::moveDown()
{
    if (m_menuOptions.isEmpty()) return;

    // Handle brightness menu specially (decrease brightness)
    if (m_currentMenuState == BrightnessMenu) {
        emit requestDecreaseBrightness();
        return;
    }

    // Standard menu navigation
    int newIndex = m_currentSelection + 1;
    if (newIndex >= m_menuOptions.count()) {
        newIndex = 0; // Wrap to top
    }
    setCurrentSelection(newIndex);
}

void MenuViewModel::selectCurrent()
{
    if (m_menuOptions.isEmpty()) return;

    QString selectedOption = m_menuOptions.at(m_currentSelection);
    qDebug() << "[MenuViewModel] Selected option:" << selectedOption << "in state:" << m_currentMenuState;

    switch (m_currentMenuState) {
        case Idle:
            // In idle, MENU/VAL opens main menu
            showMainMenu();
            break;
        case MainMenu:
            handleMainMenuSelection(selectedOption);
            break;
        case ReticleMenu:
            handleReticleMenuSelection(selectedOption);
            break;
        case ColorMenu:
            handleColorMenuSelection(selectedOption);
            break;
        case BrightnessMenu:
            // Close brightness menu
            setMenuState(Idle);
            break;
        case ZeroingOverlay:
        case WindageOverlay:
        case ZoneDefinitionOverlay:
        case SystemStatusOverlay:
        case AboutDialog:
            // These overlays handle their own selection internally
            // Just close them on MENU/VAL
            setMenuState(Idle);
            break;
    }
}

// ============================================================================
// MENU DISPLAY METHODS
// ============================================================================

void MenuViewModel::showMainMenu()
{
    if (m_currentMenuState == MainMenu) return; // Already showing

    m_previousMenuState = m_currentMenuState;
    setMenuState(MainMenu);
}

void MenuViewModel::showReticleMenu()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(ReticleMenu);
}

void MenuViewModel::showColorMenu()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(ColorMenu);
}

void MenuViewModel::showBrightnessMenu()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(BrightnessMenu);
}

void MenuViewModel::showZeroingOverlay()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(ZeroingOverlay);
}

void MenuViewModel::showWindageOverlay()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(WindageOverlay);
}

void MenuViewModel::showZoneDefinitionOverlay()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(ZoneDefinitionOverlay);
}

void MenuViewModel::showSystemStatusOverlay()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(SystemStatusOverlay);
}

void MenuViewModel::showAboutDialog()
{
    m_previousMenuState = m_currentMenuState;
    setMenuState(AboutDialog);
}

void MenuViewModel::closeCurrentMenu()
{
    setMenuState(Idle);
}

// ============================================================================
// MENU ACTIONS
// ============================================================================

void MenuViewModel::clearActiveZero()
{
    if (m_stateModel) {
        m_stateModel->clearZeroing();
        qDebug() << "[MenuViewModel] Active zero cleared";
    }
    setMenuState(Idle);
}

void MenuViewModel::clearActiveWindage()
{
    if (m_stateModel) {
        m_stateModel->clearWindage();
        qDebug() << "[MenuViewModel] Active windage cleared";
    }
    setMenuState(Idle);
}

void MenuViewModel::setAccentColor(const QColor &color)
{
    if (m_accentColor != color) {
        m_accentColor = color;
        emit accentColorChanged();
    }
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void MenuViewModel::setMenuState(MenuState newState)
{
    if (m_currentMenuState == newState) return;

    qDebug() << "[MenuViewModel] State change:" << m_currentMenuState << "->" << newState;

    m_currentMenuState = newState;
    updateMenuOptions();

    // Emit all visibility signals
    emit currentMenuStateChanged();
    emit mainMenuVisibleChanged();
    emit reticleMenuVisibleChanged();
    emit colorMenuVisibleChanged();
    emit brightnessMenuVisibleChanged();
    emit zeroingOverlayVisibleChanged();
    emit windageOverlayVisibleChanged();
    emit zoneDefinitionOverlayVisibleChanged();
    emit systemStatusOverlayVisibleChanged();
    emit aboutDialogVisibleChanged();
}

void MenuViewModel::updateMenuOptions()
{
    QStringList newOptions;
    QString newTitle;
    QString newDescription;

    switch (m_currentMenuState) {
        case Idle:
            // No menu visible
            break;

        case MainMenu:
            newTitle = "Main Menu";
            newDescription = "Navigate Through Options";
            newOptions << "--- RETICLE & DISPLAY ---"
                      << "Personalize Reticle"
                      << "Personalize Colors"
                      << "Adjust Brightness"
                      << "--- BALLISTICS ---"
                      << "Zeroing"
                      << "Clear Active Zero"
                      << "Windage"
                      << "Clear Active Windage"
                      << "--- SYSTEM ---"
                      << "Zone Definitions"
                      << "System Status"
                      << "--- INFO ---"
                      << "Help/About"
                      << "Return ...";
            break;

        case ReticleMenu: {
            newTitle = "Personalize Reticle";
            newDescription = "Select Reticle Style";

            // Build reticle options from SystemStateModel
            newOptions << "Basic"
                      << "Box Crosshair"
                      << "Standard Crosshair"
                      << "Precision Crosshair"
                      << "Mil-Dot"
                      << "Return ...";

            // Set current selection to active reticle
            if (m_stateModel) {
                int currentReticle = static_cast<int>(m_stateModel->data().reticleType);
                setCurrentSelection(currentReticle); // Will be set after options update
            }
            break;
        }

        case ColorMenu: {
            newTitle = "Personalize Color";
            newDescription = "Select HUD Color";

            // Build color options
            for (int i = 0; i < static_cast<int>(ColorStyle::COUNT); ++i) {
                newOptions << ColorUtils::toString(static_cast<ColorStyle>(i));
            }
            newOptions << "Return ...";

            // Set current selection to active color
            if (m_stateModel) {
               // int currentColor = static_cast<int>(m_stateModel->data().colorStyle);
               // setCurrentSelection(currentColor);
            }
            break;
        }

        case BrightnessMenu:
            newTitle = "Adjust Brightness";
            newDescription = "Use UP/DOWN to adjust";
            // No options needed - handled by UP/DOWN directly
            break;

        case ZeroingOverlay:
            newTitle = "Zeroing Procedure";
            newDescription = "Follow instructions";
            break;

        case WindageOverlay:
            newTitle = "Windage Adjustment";
            newDescription = "Compensate for wind";
            break;

        case ZoneDefinitionOverlay:
            newTitle = "Zone Definitions";
            newDescription = "Define no-fire zones";
            break;

        case SystemStatusOverlay:
            newTitle = "System Status";
            newDescription = "View system health";
            break;

        case AboutDialog:
            newTitle = "Help / About";
            newDescription = "System Information";
            break;
    }

    // Update properties
    if (m_menuOptions != newOptions) {
        m_menuOptions = newOptions;
        emit menuOptionsChanged();
    }

    if (m_menuTitle != newTitle) {
        m_menuTitle = newTitle;
        emit menuTitleChanged();
    }

    if (m_menuDescription != newDescription) {
        m_menuDescription = newDescription;
        emit menuDescriptionChanged();
    }

    // Reset selection when changing menus
    if (m_currentMenuState != ReticleMenu && m_currentMenuState != ColorMenu) {
        setCurrentSelection(0);
    }
}

void MenuViewModel::setCurrentSelection(int index)
{
    if (index < 0 || index >= m_menuOptions.count()) return;
    if (m_currentSelection != index) {
        m_currentSelection = index;
        emit currentSelectionChanged();
        qDebug() << "[MenuViewModel] Selection changed to index:" << index
                 << "(" << (index < m_menuOptions.count() ? m_menuOptions.at(index) : "invalid") << ")";
    }
}

void MenuViewModel::handleMainMenuSelection(const QString &option)
{
    qDebug() << "[MenuViewModel] Main menu selection:" << option;

    if (option == "Return ...") {
        setMenuState(Idle);
    } else if (option == "System Status") {
        showSystemStatusOverlay();
    } else if (option == "Personalize Reticle") {
        showReticleMenu();
    } else if (option == "Personalize Colors") {
        showColorMenu();
    } else if (option == "Zeroing") {
        showZeroingOverlay();
    } else if (option == "Clear Active Zero") {
        clearActiveZero();
    } else if (option == "Windage") {
        showWindageOverlay();
    } else if (option == "Clear Active Windage") {
        clearActiveWindage();
    } else if (option == "Zone Definitions") {
        showZoneDefinitionOverlay();
    } else if (option == "Adjust Brightness") {
        showBrightnessMenu();
    } else if (option == "Help/About") {
        showAboutDialog();
    }
    // Ignore section headers (--- XXX ---)
}

void MenuViewModel::handleReticleMenuSelection(const QString &option)
{
    qDebug() << "[MenuViewModel] Reticle menu selection:" << option;

    if (option == "Return ...") {
        showMainMenu();
        return;
    }

    // Convert string to ReticleType and update state model
    if (!m_stateModel) return;

    ReticleType newType = ReticleType::Basic;
    if (option == "Basic") newType = ReticleType::Basic;
    else if (option == "Box Crosshair") newType = ReticleType::BoxCrosshair;
    else if (option == "Standard Crosshair") newType = ReticleType::StandardCrosshair;
    else if (option == "Precision Crosshair") newType = ReticleType::PrecisionCrosshair;
    else if (option == "Mil-Dot") newType = ReticleType::MilDot;

    m_stateModel->setReticleStyle(newType);
    qDebug() << "[MenuViewModel] Reticle changed to:" << option;

    // Return to main menu
    showMainMenu();
}

void MenuViewModel::handleColorMenuSelection(const QString &option)
{
    qDebug() << "[MenuViewModel] Color menu selection:" << option;

    if (option == "Return ...") {
        showMainMenu();
        return;
    }

    // Convert string to ColorStyle and update state model
    if (!m_stateModel) return;

    ColorStyle newStyle = ColorUtils::fromString(option);
    QColor newColor = ColorUtils::toQColor(newStyle);
    m_stateModel->setColorStyle(newColor);

    qDebug() << "[MenuViewModel] Color changed to:" << option;

    // Return to main menu
    showMainMenu();
}
