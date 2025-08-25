// areazoneparameterpanel.h

#ifndef AREAZONEPARAMETERPANEL_H
#define AREAZONEPARAMETERPANEL_H

#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../models/systemstatemodel.h"

class AreaZoneParameterPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Field {
        Enabled,
        Overridable,
        ValidateButton,
        CancelButton,
        None
    };

    // EditMode is kept for consistency, but only Navigation is used.
    enum class EditMode {
        Navigation,
        ValueEdit
    };

    explicit AreaZoneParameterPanel(QWidget *parent = nullptr);

    void setZoneData(const AreaZone& zone);
    AreaZone getZoneData(const AreaZone& currentZoneData) const;
    void setEditable(bool editable);

    Field getActiveField() const { return m_activeField; }
    void setActiveField(Field field);
    
    EditMode getEditMode() const { return m_editMode; }
    
    // Méthodes pour l'interaction UP/DOWN/SELECT
    void handleUpInput();
    void handleDownInput();
    void handleSelectInput();
    
    void updateFieldHighlighting();
    void updateColorTheme(const QString& colorStyle);

signals:
    void parametersChanged();
    void validateRequested();
    void cancelRequested();

private slots:
    void onValidateClicked();
    void onCancelClicked();

private:
    void setupUI();
    void toggleActiveCheckbox();

    // UI Elements
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;

    // Enabled field
    QHBoxLayout* m_enabledLayout;
    QLabel* m_enabledLabel;
    QCheckBox* m_enabledCheckBox;

    // Overridable field
    QHBoxLayout* m_overridableLayout;
    QLabel* m_overridableLabel;
    QCheckBox* m_overridableCheckBox;

    // Buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_validateButton;
    QPushButton* m_cancelButton;

    // State
    Field m_activeField;
    EditMode m_editMode;
    bool m_isEnabled;
    bool m_isOverridable;
    
    // Style management
    QString m_currentColorStyle;
    QString m_baseStyle;
    QString m_buttonStyle;
    QString m_labelStyle;
};

#endif // AREAZONEPARAMETERPANEL_H
