#ifndef WINDAGEWIDGET_H
#define WINDAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "../models/systemstatemodel.h"
#include "basestyledwidget.h"

class WindageWidget : public BaseStyledWidget
{
    Q_OBJECT
public:
    explicit WindageWidget(SystemStateModel* model, QWidget *parent = nullptr);

    void handleUpAction();    // Increase wind speed
    void handleDownAction();  // Decrease wind speed
    void handleSelectAction();
    void handleBackAction();

signals:
    void windageProcedureFinished();

protected:
    void showEvent(QShowEvent *event) override;

private:
    enum class WindageState {
        Instruct_AlignToWind,
        Set_WindSpeed,
        Completed  
    };

    SystemStateModel* m_stateModel;
    WindageState m_currentState;

    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QLabel* m_instructionLabel;
    QLabel* m_windSpeedDisplayLabel; // Shows current knots value being edited
    QPushButton *m_returnButton = nullptr;
    float m_currentWindSpeedEdit; // Temporary value for editing

    void initializeUI();
    void updateUI();

private slots:
    void onModelStateChanged(const SystemStateData &data);
};
#endif // WINDAGEWIDGET_H
