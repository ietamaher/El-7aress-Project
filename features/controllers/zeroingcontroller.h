#ifndef ZEROINGCONTROLLER_H
#define ZEROINGCONTROLLER_H

#include <QObject>

class ZeroingViewModel;
class SystemStateModel;

class ZeroingController : public QObject
{
    Q_OBJECT

public:
    explicit ZeroingController(QObject *parent = nullptr);
    void initialize();
    void setViewModel(ZeroingViewModel* viewModel);
    void setStateModel(SystemStateModel* stateModel);

public slots:
    void show();
    void hide();
    void onSelectButtonPressed();
    void onBackButtonPressed();
    void onUpButtonPressed();
    void onDownButtonPressed();

signals:
    void zeroingFinished();
    void returnToMainMenu();

private slots:
    void onColorStyleChanged(const QColor& color);
    
private:
    enum class ZeroingState {
        Idle,
        Instruct_MoveReticleToImpact,
        Completed
    };

    void updateUI();
    void transitionToState(ZeroingState newState);

    ZeroingViewModel* m_viewModel;
    SystemStateModel* m_stateModel;
    ZeroingState m_currentState;

};

#endif // ZEROINGCONTROLLER_H
