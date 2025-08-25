#ifndef BASESTYLEDWIDGET_H
#define BASESTYLEDWIDGET_H

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout> // Added for two-column layout
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>   // Added for sectioning
#include <QString>
#include "../models/systemstatemodel.h" // Includes systemstatedata.h


class BaseStyledWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseStyledWidget(SystemStateModel* model, QWidget *parent = nullptr);

    void setColorStyleChanged(const QColor &style);

public slots:
    virtual void onColorStyleChanged(const QColor &style);

protected:

    SystemStateModel* m_stateModel; // Pointer to the central model

    // Protected members that derived classes can access
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_contentWidget = nullptr;
    QString m_currentColorStyle = "Green";
    QString m_baseStyle;
    QString m_buttonStyle;
    QString m_listStyle;
    QString m_labelStyle;
    QString m_groupBoxStyle;
    // Common styling method
    void applyStylesToWidget(QWidget *widget);
};

#endif // BASESTYLEDWIDGET_H
