#ifndef TRPPARAMETERPANEL_H
#define TRPPARAMETERPANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include "../models/systemstatemodel.h"

class TRPParameterPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Field {
        LocationPage,
        TrpInPage,
        HaltTime,
        ValidateButton,
        CancelButton,
        None
    };

    enum class EditMode {
        Navigation,    // Navigation entre les champs avec UP/DOWN
        ValueEdit      // Édition de valeur avec UP/DOWN
    };

    explicit TRPParameterPanel(QWidget *parent = nullptr);

    void setTRPData(const TargetReferencePoint& trp);
    TargetReferencePoint getTRPData(const TargetReferencePoint& baseTRP) const;

    Field getActiveField() const { return m_activeField; }
    void setActiveField(Field field);
    
    EditMode getEditMode() const { return m_editMode; }
    
    // Méthodes pour l'interaction UP/DOWN/SELECT
    void handleUpInput();
    void handleDownInput();
    void handleSelectInput();
    
    void updateFieldHighlighting();
    void updateColorTheme(const QString& theme);

signals:
    void parametersChanged(); 

private slots:
    void onValidateClicked();
    void onCancelClicked();

private:
    void setupUI();
    void updateLocationPageDisplay();
    void updateTrpInPageDisplay();
    void updateHaltTimeDisplay();
    void enterValueEditMode();
    void exitValueEditMode();
    void adjustLocationPage(bool increase);
    void adjustTrpInPage(bool increase);
    void adjustHaltTime(bool increase);

    // UI Elements
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    
    // Location Page field
    QHBoxLayout* m_locationPageLayout;
    QLabel* m_locationPageLabel;
    QLabel* m_locationPageValueLabel;
    QLabel* m_locationPageEditIndicator;
    
    // TRP In Page field
    QHBoxLayout* m_trpInPageLayout;
    QLabel* m_trpInPageLabel;
    QLabel* m_trpInPageValueLabel;
    QLabel* m_trpInPageEditIndicator;
    
    // Halt Time field
    QHBoxLayout* m_haltTimeLayout;
    QLabel* m_haltTimeLabel;
    QLabel* m_haltTimeValueLabel;
    QLabel* m_haltTimeEditIndicator;
    
    // Buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_validateButton;
    QPushButton* m_cancelButton;

    // State
    Field m_activeField;
    EditMode m_editMode;
    int m_locationPageValue;  // 1-200
    int m_trpInPageValue;     // 1-50
    int m_haltTimeValue;      // 10-600 (1.0-60.0 secondes en incréments de 1.0)
    
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

#endif // TRPPARAMETERPANEL_H

