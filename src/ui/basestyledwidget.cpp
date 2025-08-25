#include "basestyledwidget.h"

BaseStyledWidget::BaseStyledWidget(SystemStateModel* model, QWidget *parent) :
    QWidget(parent),
    m_stateModel(model)
{
    connect(m_stateModel, &SystemStateModel::colorStyleChanged, // Assuming this signal exists
            this, &BaseStyledWidget::onColorStyleChanged, Qt::QueuedConnection);

}

void BaseStyledWidget::onColorStyleChanged(const QColor &style)
{
    setColorStyleChanged(style);
}

void BaseStyledWidget::setColorStyleChanged(const QColor &style)
{
    // Same logic as your original method but generalized
    if (style == QColor(70, 226, 165)) {
        m_currentColorStyle = "Green";
    }
    else if (style == QColor(200,20,40)) {
        m_currentColorStyle = "Red";
    }
    else if (style == Qt::white) {
        m_currentColorStyle = "White";
    }

    m_baseStyle = "background-color: rgba(0,0,0,150); font: 600 14pt 'Archivo Narrow';";


    if (m_currentColorStyle == "Red") {
        m_baseStyle += "color: rgba(200,20,40,255);";
        m_buttonStyle = "QPushButton {" + m_baseStyle + "border: 1px solid rgba(200,20,40,255);}"
                                                    "QPushButton:focus {background-color: rgba(200,20,40,255); color: white; border: 1px solid white;}";
        m_listStyle = "QListWidget {" + m_baseStyle + "}"
                                                  "QListWidget::item:selected {color: white; background: rgba(200,20,40,255); border: 1px solid white;}";
        m_labelStyle = "QLabel {" + m_baseStyle + "}";
        m_groupBoxStyle = "QGroupBox {" + m_baseStyle + "border: 1px solid rgba(200,20,40,255); margin-top: 1ex;}"
                                                    "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else if (m_currentColorStyle == "Green") {
        m_baseStyle += "color: rgba(70, 226, 165,255);";
        m_buttonStyle = "QPushButton {" + m_baseStyle + "border: 1px solid rgba(70, 226, 165,255);}"
                                                    "QPushButton:focus {background-color: rgba(70, 226, 165,255); color: white; border: 1px solid white;}";
        m_listStyle = "QListWidget {" + m_baseStyle + " }"
                                                  "QListWidget::item:selected {color: white; background: rgba(70, 226, 165,255); border: 1px solid white;}";
        m_labelStyle = "QLabel {" + m_baseStyle + "}"
                                                  "QLabel#menuTitle {" + m_baseStyle + "}"
                                       "QLabel#menuDescription {" + m_baseStyle + "}"
                                       "QLabel#navigationHints {" + m_baseStyle + "}";;
        m_groupBoxStyle = "QGroupBox {" + m_baseStyle + "border: 1px solid rgba(70, 226, 165,255); margin-top: 1ex;}"
                                                    "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else if (m_currentColorStyle == "White") {
        m_baseStyle += "color: rgba(255,255,255,255);";
        m_buttonStyle = "QPushButton {" + m_baseStyle + "border: 1px solid rgba(255,255,255,255);}"
                                                    "QPushButton:focus {background-color: rgba(255,255,255,255); color: rgba(0,0,0,255); border: 1px solid white;}";
        m_listStyle = "QListWidget {" + m_baseStyle + "}"
                                                  "QListWidget::item:selected {background: rgba(255,255,255,255); color: rgba(0,0,0,255); border: 1px solid white;}";
        m_labelStyle = "QLabel {" + m_baseStyle + "}";
        m_groupBoxStyle = "QGroupBox {" + m_baseStyle + "border: 1px solid rgba(255,255,255,255); margin-top: 1ex;}"
                                                    "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
    }
    else {
        // Default - Green
       /* m_baseStyle += "color: rgba(70, 226, 165,255);";
        m_buttonStyle = "QPushButton {" + m_baseStyle + "border: 1px solid rgba(70, 226, 165,255);}"
                                                    "QPushButton:focus {background-color: rgba(70, 226, 165,255); color: white; border: 1px solid white;}";
        m_listStyle = "QListWidget {" + m_baseStyle + "}"
                                                  "QListWidget::item:selected {color: white; background: rgba(70, 226, 165,255); border: 1px solid white;}";
        m_labelStyle = "QLabel {" + m_baseStyle + "}";
        m_groupBoxStyle = "QGroupBox {" + m_baseStyle + "border: 1px solid rgba(70, 226, 165,255); margin-top: 1ex;}"
                                                    "QGroupBox::title {subcontrol-origin: margin; subcontrol-position: top center;}";
   */ }

    // Apply styles to the content widget if it exists
    if (m_contentWidget) {
        applyStylesToWidget(m_contentWidget);
    } else {
        // Fallback: apply to this widget
        applyStylesToWidget(this);
    }

    // Ensure scroll area transparency
    if (m_scrollArea) {
        m_scrollArea->setStyleSheet("QScrollArea {background-color: transparent; border: none;}");
    }
}

void BaseStyledWidget::applyStylesToWidget(QWidget *widget)
{
    // Apply base style to the widget
    widget->setStyleSheet(m_baseStyle);

    // Apply styles to all child widgets by type
    for (QPushButton* button : widget->findChildren<QPushButton*>()) {
        button->setStyleSheet(m_buttonStyle);
    }

    for (QListWidget* list : widget->findChildren<QListWidget*>()) {
        list->setStyleSheet(m_listStyle);
    }

    for (QLabel* label : widget->findChildren<QLabel*>()) {
        label->setStyleSheet(m_labelStyle);
    }

    for (QGroupBox* groupBox : widget->findChildren<QGroupBox*>()) {
        groupBox->setStyleSheet(m_groupBoxStyle);
    }
}
