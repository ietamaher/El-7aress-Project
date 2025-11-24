#ifndef WINDAGECONTROLLER_H
#define WINDAGECONTROLLER_H

#include <QObject>

class WindageViewModel;
class SystemStateModel;

class WindageController : public QObject
{
    Q_OBJECT

public:
    explicit WindageController(QObject *parent = nullptr);
    void initialize();
    void setViewModel(WindageViewModel* viewModel);
    void setStateModel(SystemStateModel* stateModel);

public slots:
    void show();
    void hide();
    void onSelectButtonPressed();
    void onBackButtonPressed();
    void onUpButtonPressed();
    void onDownButtonPressed();

signals:
    void windageFinished();
    void returnToMainMenu();

private slots:
    void onColorStyleChanged(const QColor& color);
        
private:
    enum class WindageState {
        Idle,
        Instruct_AlignToWind,
        Set_WindSpeed,
        Completed
    };

    void updateUI();
    void transitionToState(WindageState newState);

    WindageViewModel* m_viewModel;
    SystemStateModel* m_stateModel;
    WindageState m_currentState;
    float m_currentWindSpeedEdit;
};

#endif // WINDAGECONTROLLER_H
