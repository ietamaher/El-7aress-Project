#ifndef ZEROINGWIDGET_H
#define ZEROINGWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <src/models/systemstatemodel.h> // For SystemStateData
#include "basestyledwidget.h"

// Forward declare
class SystemStateModel;

class ZeroingWidget : public BaseStyledWidget
{
    Q_OBJECT
public:
    explicit ZeroingWidget(SystemStateModel* model, QWidget *parent = nullptr);

    // Public methods for MainWindow to call
    void handleUpAction();    // Might be used for +/- 3 degree fine adjustment steps
    void handleDownAction();  // Might be used for +/- 3 degree fine adjustment steps
    void handleSelectAction();
    void handleBackAction();  // To exit Zeroing mode

signals:
    void zeroingProcedureFinished(); // To tell MainWindow to hide this widget

protected:
    void showEvent(QShowEvent *event) override;

private:
    enum class ZeroingState {
        Idle,
        Instruct_MoveReticleToImpact,
        // FineTune_Adjustments, // If you were to add this
        Completed // <<< ENSURE THIS STATE IS DEFINED
    };

    SystemStateModel* m_stateModel;
    ZeroingState m_currentState;

    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_instructionLabel;
    QLabel* m_statusLabel; // To show current offset values if needed, or "ZEROING"
    QPushButton *m_returnButton = nullptr;
    // If implementing +/- 3 degree discrete adjustments
    // float m_currentAdjustmentStepAz = 0.1f; // Degrees or mils per UP/DOWN press
    // float m_currentAdjustmentStepEl = 0.1f;

    void initializeUI();
    void updateUI();

private slots:
    void onModelStateChanged(const SystemStateData &data); // To react to zeroing flags
};

#endif // ZEROINGWIDGET_H
