#ifndef SECTORSCANPARAMETERPANEL_H
#define SECTORSCANPARAMETERPANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>
#include "../models/systemstatemodel.h"

class SectorScanParameterPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Field {
        Enabled,
        ScanSpeed,
        ValidateButton,
        CancelButton,
        None
    };

    enum class EditMode {
        Navigation,    // Navigation entre les champs avec UP/DOWN
        ValueEdit      // Édition de valeur avec UP/DOWN
    };

    explicit SectorScanParameterPanel(QWidget *parent = nullptr);

    void setZoneData(const AutoSectorScanZone& zone);
    AutoSectorScanZone getZoneData(const AutoSectorScanZone& baseZone) const;

    Field getActiveField() const { return m_activeField; }
    void setActiveField(Field field);
    
    EditMode getEditMode() const { return m_editMode; }
    
    // Méthodes pour l'interaction UP/DOWN/SELECT
    void handleUpInput();
    void handleDownInput();
    void handleSelectInput();
    
    void toggleActiveCheckbox();
    void updateFieldHighlighting();
    void updateColorTheme(const QString& theme);

signals:
    void parametersChanged(); 

private slots:
    void onValidateClicked();
    void onCancelClicked();

private:
    void setupUI();
    void updateScanSpeedDisplay();
    void enterValueEditMode();
    void exitValueEditMode();
    void adjustScanSpeed(bool increase);

    // UI Elements
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    
    // Enabled field
    QHBoxLayout* m_enabledLayout;
    QLabel* m_enabledLabel;
    QCheckBox* m_enabledCheckBox;
    
    // Scan Speed field
    QHBoxLayout* m_scanSpeedLayout;
    QLabel* m_scanSpeedLabel;
    QLabel* m_scanSpeedValueLabel;
    QLabel* m_scanSpeedEditIndicator;  // Indicateur mode édition
    
    // Buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_validateButton;
    QPushButton* m_cancelButton;

    // State
    Field m_activeField;
    EditMode m_editMode;
    bool m_isEnabled;
    int m_scanSpeedValue;  // En degrés/seconde (1-10)
    
    // Style constants
    static const QString ACTIVE_STYLE;
    static const QString INACTIVE_STYLE;
    static const QString EDIT_MODE_STYLE;

        // Style management
    QString m_currentColorStyle;
    QString m_baseStyle;
    QString m_buttonStyle;
    QString m_labelStyle;
    
};

#endif // SECTORSCANPARAMETERPANEL_H

// 2. Implémentation améliorée pour SectorScanParameterPanel



