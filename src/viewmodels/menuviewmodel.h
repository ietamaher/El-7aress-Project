#ifndef MENUVIEWMODEL_H
#define MENUVIEWMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QColor>

// Forward declaration
class SystemStateModel;

/**
 * @brief MenuViewModel - Manages menu state and navigation for QML
 *
 * This ViewModel handles all menu interactions for the 3-button navigation system:
 * - UP button: Navigate up through menu options
 * - DOWN button: Navigate down through menu options
 * - MENU/VAL button: Select current option or show main menu from IDLE
 */
class MenuViewModel : public QObject
{
    Q_OBJECT

    // ========================================================================
    // MENU STATE
    // ========================================================================
    Q_PROPERTY(MenuState currentMenuState READ currentMenuState NOTIFY currentMenuStateChanged)
    Q_PROPERTY(bool mainMenuVisible READ mainMenuVisible NOTIFY mainMenuVisibleChanged)
    Q_PROPERTY(bool reticleMenuVisible READ reticleMenuVisible NOTIFY reticleMenuVisibleChanged)
    Q_PROPERTY(bool colorMenuVisible READ colorMenuVisible NOTIFY colorMenuVisibleChanged)
    Q_PROPERTY(bool brightnessMenuVisible READ brightnessMenuVisible NOTIFY brightnessMenuVisibleChanged)
    Q_PROPERTY(bool zeroingOverlayVisible READ zeroingOverlayVisible NOTIFY zeroingOverlayVisibleChanged)
    Q_PROPERTY(bool windageOverlayVisible READ windageOverlayVisible NOTIFY windageOverlayVisibleChanged)
    Q_PROPERTY(bool zoneDefinitionOverlayVisible READ zoneDefinitionOverlayVisible NOTIFY zoneDefinitionOverlayVisibleChanged)
    Q_PROPERTY(bool systemStatusOverlayVisible READ systemStatusOverlayVisible NOTIFY systemStatusOverlayVisibleChanged)
    Q_PROPERTY(bool aboutDialogVisible READ aboutDialogVisible NOTIFY aboutDialogVisibleChanged)

    // ========================================================================
    // MENU CONTENT
    // ========================================================================
    Q_PROPERTY(QStringList menuOptions READ menuOptions NOTIFY menuOptionsChanged)
    Q_PROPERTY(int currentSelection READ currentSelection NOTIFY currentSelectionChanged)
    Q_PROPERTY(QString menuTitle READ menuTitle NOTIFY menuTitleChanged)
    Q_PROPERTY(QString menuDescription READ menuDescription NOTIFY menuDescriptionChanged)

    // ========================================================================
    // VISUAL STYLING
    // ========================================================================
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)

public:
    enum MenuState {
        Idle,
        MainMenu,
        ReticleMenu,
        ColorMenu,
        BrightnessMenu,
        ZeroingOverlay,
        WindageOverlay,
        ZoneDefinitionOverlay,
        SystemStatusOverlay,
        AboutDialog
    };
    Q_ENUM(MenuState)

    explicit MenuViewModel(SystemStateModel *stateModel, QObject *parent = nullptr);

    // Getters
    MenuState currentMenuState() const { return m_currentMenuState; }
    bool mainMenuVisible() const { return m_currentMenuState == MainMenu; }
    bool reticleMenuVisible() const { return m_currentMenuState == ReticleMenu; }
    bool colorMenuVisible() const { return m_currentMenuState == ColorMenu; }
    bool brightnessMenuVisible() const { return m_currentMenuState == BrightnessMenu; }
    bool zeroingOverlayVisible() const { return m_currentMenuState == ZeroingOverlay; }
    bool windageOverlayVisible() const { return m_currentMenuState == WindageOverlay; }
    bool zoneDefinitionOverlayVisible() const { return m_currentMenuState == ZoneDefinitionOverlay; }
    bool systemStatusOverlayVisible() const { return m_currentMenuState == SystemStatusOverlay; }
    bool aboutDialogVisible() const { return m_currentMenuState == AboutDialog; }

    QStringList menuOptions() const { return m_menuOptions; }
    int currentSelection() const { return m_currentSelection; }
    QString menuTitle() const { return m_menuTitle; }
    QString menuDescription() const { return m_menuDescription; }
    QColor accentColor() const { return m_accentColor; }

public slots:
    // Navigation methods (called from C++ button handlers)
    void moveUp();
    void moveDown();
    void selectCurrent();
    void showMainMenu();
    void closeCurrentMenu();

    // Direct menu show methods (can be called from QML or C++)
    void showReticleMenu();
    void showColorMenu();
    void showBrightnessMenu();
    void showZeroingOverlay();
    void showWindageOverlay();
    void showZoneDefinitionOverlay();
    void showSystemStatusOverlay();
    void showAboutDialog();

    // Menu actions
    void clearActiveZero();
    void clearActiveWindage();

    // Color update
    void setAccentColor(const QColor &color);

signals:
    // State change signals
    void currentMenuStateChanged();
    void mainMenuVisibleChanged();
    void reticleMenuVisibleChanged();
    void colorMenuVisibleChanged();
    void brightnessMenuVisibleChanged();
    void zeroingOverlayVisibleChanged();
    void windageOverlayVisibleChanged();
    void zoneDefinitionOverlayVisibleChanged();
    void systemStatusOverlayVisibleChanged();
    void aboutDialogVisibleChanged();

    // Content signals
    void menuOptionsChanged();
    void currentSelectionChanged();
    void menuTitleChanged();
    void menuDescriptionChanged();
    void accentColorChanged();

    // Action signals (for MainWindow to handle hardware actions)
    void requestClearAlarms();
    void requestIncreaseBrightness();
    void requestDecreaseBrightness();

private:
    void setMenuState(MenuState newState);
    void updateMenuOptions();
    void setCurrentSelection(int index);
    void handleMainMenuSelection(const QString &option);
    void handleReticleMenuSelection(const QString &option);
    void handleColorMenuSelection(const QString &option);

    // State
    SystemStateModel *m_stateModel;
    MenuState m_currentMenuState;
    QStringList m_menuOptions;
    int m_currentSelection;
    QString m_menuTitle;
    QString m_menuDescription;
    QColor m_accentColor;

    // Previous menu stack (for back navigation)
    MenuState m_previousMenuState;
};

#endif // MENUVIEWMODEL_H
