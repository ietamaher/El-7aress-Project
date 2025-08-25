#ifndef RADARTARGETLISTWIDGET_H
#define RADARTARGETLISTWIDGET_H

#include "../ui/basestyledwidget.h" // <<< INHERIT FROM YOUR BASE CLASS
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
// Forward declarations
class SystemStateModel;
struct SystemStateData; // Assuming this is defined where BaseStyledWidget can see it

class RadarTargetListWidget : public BaseStyledWidget // <<< INHERIT
{
    Q_OBJECT
public:
    explicit RadarTargetListWidget(SystemStateModel* model, QWidget *parent = nullptr);

    // Public methods for MainWindow/JoystickController to call for navigation
    void moveSelectionUp();
    void moveSelectionDown();
    void selectCurrentItem(); // Corresponds to MENU/VALIDATE press

signals:
    void slewToTargetRequested(quint32 targetId);
    void widgetClosed(); // To notify MainWindow to hide/destroy this widget

protected:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override; // For keyboard testing

private slots:
    // Listens for changes in the model to update its display
    void onSystemStateChanged(const SystemStateData& data);

private:
    // UI Elements
    QVBoxLayout* m_mainLayout = nullptr;
    QLabel* m_titleLabel = nullptr;
    QListWidget* m_targetListWidget = nullptr;
    QLabel* m_navigationHintLabel = nullptr;

    // Local cache to avoid unnecessary list repopulation
    quint32 m_currentlyDisplayedSelectedId = 0;
    // We need a way to compare the radar plots vector. QVector supports operator== if its element does.
    // Let's assume SimpleRadarPlot has operator==.
    QVector<SimpleRadarPlot> m_currentlyDisplayedPlots;

    void initializeUI();
    void updateListDisplay(const QVector<SimpleRadarPlot>& plots, quint32 selectedId);
};

#endif // RADARTARGETLISTWIDGET_H
