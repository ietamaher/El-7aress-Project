// areazoneparameterpanel.cpp

#include "areazoneparameterpanel.h"
#include <QDebug>
#include <QFrame>

AreaZoneParameterPanel::AreaZoneParameterPanel(QWidget *parent)
    : QWidget(parent)
    , m_activeField(Field::Enabled)
    , m_editMode(EditMode::Navigation)
    , m_isEnabled(true)
    , m_isOverridable(false)
    , m_currentColorStyle("Green")
{
    setupUI();
    updateColorTheme(m_currentColorStyle); // This will call updateFieldHighlighting
}

void AreaZoneParameterPanel::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    
    m_mainLayout = new QVBoxLayout(this);
    // Remove custom margins and spacing to match SectorScanParameterPanel
    setLayout(m_mainLayout);

    // Title
    m_titleLabel = new QLabel("Area Zone Parameters", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;"); // Add styling like SectorScan
    m_mainLayout->addWidget(m_titleLabel);
    
    // Enabled field - match SectorScan checkbox positioning
    m_enabledLayout = new QHBoxLayout();
    m_enabledLabel = new QLabel("Enabled:", this);
    m_enabledCheckBox = new QCheckBox(this);
    m_enabledCheckBox->setChecked(m_isEnabled);
    m_enabledLayout->addWidget(m_enabledLabel);        // Label first
    m_enabledLayout->addWidget(m_enabledCheckBox);     // Then checkbox
    m_enabledLayout->addStretch();                     // Then stretch
    m_mainLayout->addLayout(m_enabledLayout);

    // Overridable field - match SectorScan checkbox positioning
    m_overridableLayout = new QHBoxLayout();
    m_overridableLabel = new QLabel("Overridable:", this);
    m_overridableCheckBox = new QCheckBox(this);
    m_overridableCheckBox->setChecked(m_isOverridable);
    m_overridableLayout->addWidget(m_overridableLabel);  // Label first
    m_overridableLayout->addWidget(m_overridableCheckBox); // Then checkbox
    m_overridableLayout->addStretch();                   // Then stretch
    m_mainLayout->addLayout(m_overridableLayout);
 
    // Button layout - match SectorScan button order and positioning
    m_buttonLayout = new QHBoxLayout();
    m_validateButton = new QPushButton("Validate", this);   
    m_cancelButton = new QPushButton("Cancel", this);

    // Remove stretches around buttons to match SectorScan
    m_buttonLayout->addWidget(m_validateButton);    // Validate first
    m_buttonLayout->addWidget(m_cancelButton);      // Cancel second

    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_validateButton, &QPushButton::clicked, this, &AreaZoneParameterPanel::onValidateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &AreaZoneParameterPanel::onCancelClicked);
    
    // Connect checkbox state changes to internal state
    connect(m_enabledCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        m_isEnabled = checked;
        emit parametersChanged();
    });
    connect(m_overridableCheckBox, &QCheckBox::toggled, this, [this](bool checked){
        m_isOverridable = checked;
        emit parametersChanged();
    });
}

void AreaZoneParameterPanel::setZoneData(const AreaZone& zone)
{
    // Block signals to prevent emitting parametersChanged during setup
    m_enabledCheckBox->blockSignals(true);
    m_overridableCheckBox->blockSignals(true);

    m_isEnabled = zone.isEnabled;
    m_isOverridable = zone.isOverridable;
    
    m_enabledCheckBox->setChecked(m_isEnabled);
    m_overridableCheckBox->setChecked(m_isOverridable);

    m_enabledCheckBox->blockSignals(false);
    m_overridableCheckBox->blockSignals(false);
}

AreaZone AreaZoneParameterPanel::getZoneData(const AreaZone& currentZoneData) const
{
    AreaZone data = currentZoneData;
    data.isEnabled = m_isEnabled;
    data.isOverridable = m_isOverridable;
    return data;
}

void AreaZoneParameterPanel::setEditable(bool editable)
{
    m_enabledCheckBox->setEnabled(editable);
    m_overridableCheckBox->setEnabled(editable);
    m_validateButton->setEnabled(editable);
    m_cancelButton->setEnabled(editable);
}

void AreaZoneParameterPanel::setActiveField(Field field)
{
    if (m_activeField == field) return;
    m_activeField = field;
    m_editMode = EditMode::Navigation;
    updateFieldHighlighting();
}

void AreaZoneParameterPanel::handleUpInput()
{
    switch (m_activeField) {
        case Field::Enabled:       setActiveField(Field::CancelButton); break;
        case Field::Overridable:   setActiveField(Field::Enabled); break;
        case Field::ValidateButton:setActiveField(Field::Overridable); break;
        case Field::CancelButton:  setActiveField(Field::ValidateButton); break;
    }
}

void AreaZoneParameterPanel::handleDownInput()
{
    switch (m_activeField) {
        case Field::Enabled:       setActiveField(Field::Overridable); break;
        case Field::Overridable:   setActiveField(Field::ValidateButton); break;
        case Field::ValidateButton:setActiveField(Field::CancelButton); break;
        case Field::CancelButton:  setActiveField(Field::Enabled); break;
    }
}

void AreaZoneParameterPanel::handleSelectInput()
{
    switch (m_activeField) {
        case Field::Enabled:
        case Field::Overridable:
            toggleActiveCheckbox();
            break;
        case Field::ValidateButton:
            onValidateClicked();
            break;
        case Field::CancelButton:
            onCancelClicked();
            break;
    }
}

void AreaZoneParameterPanel::toggleActiveCheckbox()
{
    switch (m_activeField) {
        case Field::Enabled:
            m_enabledCheckBox->toggle(); // Signal will update m_isEnabled
            break;
        case Field::Overridable:
            m_overridableCheckBox->toggle(); // Signal will update m_isOverridable
            break;
        default:
            break;
    }
}

void AreaZoneParameterPanel::updateColorTheme(const QString& colorStyle)
{
    m_currentColorStyle = colorStyle;
    QString mainColor, titleFontColor, panelBgColor;

    if (m_currentColorStyle == "Red") {
        mainColor = "rgba(200,20,40,255)";
        titleFontColor = mainColor;
        panelBgColor = "rgba(40, 0, 0, 200)";
    } else if (m_currentColorStyle == "White") {
        mainColor = "rgba(255,255,255,255)";
        titleFontColor = mainColor;
        panelBgColor = "rgba(50, 50, 50, 200)";
    } else { // Default to Green
        mainColor = "rgba(70, 226, 165,255)";
        titleFontColor = mainColor;
        panelBgColor = "rgba(0, 40, 30, 200)";
    }

    this->setStyleSheet("background-color: " + panelBgColor + ";");
    
    m_titleLabel->setStyleSheet("font: 700 16pt 'Archivo Narrow'; color: " + titleFontColor + "; background-color: transparent;");

    m_baseStyle = "font: 600 14pt 'Archivo Narrow'; background-color: transparent;";

    m_labelStyle = "QLabel { " + m_baseStyle + " color: " + mainColor + "; padding: 5px; border: 1px solid transparent; }";
    
    m_buttonStyle = "QPushButton { " + m_baseStyle + " color: " + mainColor + 
                    "; border: 1px solid " + mainColor + "; padding: 5px 15px; }";

    // Style for checkboxes to match theme
    QString checkBoxStyle = "QCheckBox { spacing: 5px; " + m_baseStyle + " color: " + mainColor + "; }"
                            "QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid " + mainColor + "; }"
                            "QCheckBox::indicator:checked { background-color: " + mainColor + "; }";
    m_enabledCheckBox->setStyleSheet(checkBoxStyle);
    m_overridableCheckBox->setStyleSheet(checkBoxStyle);
    
    updateFieldHighlighting();
}

void AreaZoneParameterPanel::updateFieldHighlighting()
{
    QString highlightColor, highlightTextColor;

    if (m_currentColorStyle == "Red") {
        highlightColor = "rgba(200,20,40,255)";
        highlightTextColor = "white";
    } else if (m_currentColorStyle == "White") {
        highlightColor = "rgba(255,255,255,255)";
        highlightTextColor = "black";
    } else { // Default Green
        highlightColor = "rgba(70, 226, 165,255)";
        highlightTextColor = "black";
    }

    // Reset all elements to base style
    m_enabledLabel->setStyleSheet(m_labelStyle);
    m_overridableLabel->setStyleSheet(m_labelStyle);
    m_validateButton->setStyleSheet(m_buttonStyle);
    m_cancelButton->setStyleSheet(m_buttonStyle);

    // Create highlight styles
    QString highlightLabelStyle = "QLabel { " + m_baseStyle + 
                                  "background-color: " + highlightColor + "; " +
                                  "color: " + highlightTextColor + "; " +
                                  "border: 1px solid " + highlightColor + "; padding: 5px; }";
    
    QString highlightButtonStyle = "QPushButton { " + m_baseStyle + 
                                   "background-color: " + highlightColor + "; " +
                                   "color: " + highlightTextColor + "; " +
                                   "border: 1px solid " + highlightColor + "; padding: 5px 15px; }";

    // Apply highlighting based on active field
    switch (m_activeField) {
        case Field::Enabled:
            m_enabledLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::Overridable:
            m_overridableLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::ValidateButton:
            m_validateButton->setStyleSheet(highlightButtonStyle);
            break;
        case Field::CancelButton:
            m_cancelButton->setStyleSheet(highlightButtonStyle);
            break;
    }
}

void AreaZoneParameterPanel::onValidateClicked()
{
    qDebug() << "AreaZoneParameterPanel: Validate clicked";
    emit validateRequested();
}

void AreaZoneParameterPanel::onCancelClicked()
{
    qDebug() << "AreaZoneParameterPanel: Cancel clicked";
    emit cancelRequested();
}