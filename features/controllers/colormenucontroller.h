#ifndef COLORMENUCONTROLLER_H
#define COLORMENUCONTROLLER_H

#include <QObject>
#include <QColor>
#include "../models/menuviewmodel.h"
#include "models/systemstatedata.h"

class OsdViewModel;
class SystemStateModel;

class ColorMenuController : public QObject
{
    Q_OBJECT
public:
    explicit ColorMenuController(QObject *parent = nullptr);
    void initialize();
    void setViewModel(MenuViewModel* viewModel);
    void setOsdViewModel(OsdViewModel* osdViewModel);
    void setStateModel(SystemStateModel* stateModel);

public slots:
    void show();
    void hide();
    void onUpButtonPressed();
    void onDownButtonPressed();
    void onSelectButtonPressed();
    void onBackButtonPressed();

signals:
    void menuFinished();
    void returnToMainMenu();

private slots:
    void handleMenuOptionSelected(const QString& option);
    void handleCurrentItemChanged(int index);
    void onColorStyleChanged(const QColor& color);

private:
    MenuViewModel* m_viewModel;
    OsdViewModel* m_osdViewModel;
    SystemStateModel* m_stateModel;

    QStringList buildColorOptions() const;
    QString colorStyleToString(ColorStyle style) const;
    ColorStyle stringToColorStyle(const QString& str) const;
    QColor colorStyleToQColor(ColorStyle style) const;

    ColorStyle m_originalColorStyle;

};

#endif // COLORMENUCONTROLLER_H
