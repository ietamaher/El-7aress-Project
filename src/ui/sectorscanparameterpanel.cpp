#include "sectorscanparameterpanel.h"
#include <QDebug>

const QString SectorScanParameterPanel::ACTIVE_STYLE = "QLabel { background-color: #4CAF50; color: white; padding: 2px; }";
const QString SectorScanParameterPanel::INACTIVE_STYLE = "QLabel { background-color: transparent; color: black; }";
const QString SectorScanParameterPanel::EDIT_MODE_STYLE = "QLabel { background-color: #FF9800; color: white; padding: 2px; }";

SectorScanParameterPanel::SectorScanParameterPanel(QWidget *parent)
    : QWidget(parent)
    , m_activeField(Field::Enabled)
    , m_editMode(EditMode::Navigation)
    , m_isEnabled(true)
    , m_scanSpeedValue(5)  // Valeur par défaut : 5 degrés/seconde
{
    setupUI();
    updateFieldHighlighting();
}

void SectorScanParameterPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("Sector Scan Parameters", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_mainLayout->addWidget(m_titleLabel);
    
    // Enabled field
    m_enabledLayout = new QHBoxLayout();
    m_enabledLabel = new QLabel("Enabled:", this);
    m_enabledCheckBox = new QCheckBox(this);
    m_enabledCheckBox->setChecked(m_isEnabled);
    m_enabledLayout->addWidget(m_enabledLabel);
    m_enabledLayout->addWidget(m_enabledCheckBox);
    m_enabledLayout->addStretch();
    m_mainLayout->addLayout(m_enabledLayout);
    
    // Scan Speed field
    m_scanSpeedLayout = new QHBoxLayout();
    m_scanSpeedLabel = new QLabel("Scan Speed:", this);
    m_scanSpeedValueLabel = new QLabel(QString("%1 deg/s").arg(m_scanSpeedValue), this);
    m_scanSpeedEditIndicator = new QLabel("[EDIT]", this);
    m_scanSpeedEditIndicator->setVisible(false);
    m_scanSpeedLayout->addWidget(m_scanSpeedLabel);
    m_scanSpeedLayout->addWidget(m_scanSpeedValueLabel);
    m_scanSpeedLayout->addWidget(m_scanSpeedEditIndicator);
    m_scanSpeedLayout->addStretch();
    m_mainLayout->addLayout(m_scanSpeedLayout);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_validateButton = new QPushButton("Validate", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_buttonLayout->addWidget(m_validateButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_validateButton, &QPushButton::clicked, this, &SectorScanParameterPanel::onValidateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SectorScanParameterPanel::onCancelClicked);
}

void SectorScanParameterPanel::setZoneData(const AutoSectorScanZone& zone)
{
    m_isEnabled = zone.isEnabled;
    m_scanSpeedValue = zone.scanSpeed;  // Supposant que scanSpeed est en degrés/seconde
    
    m_enabledCheckBox->setChecked(m_isEnabled);
    updateScanSpeedDisplay();
}

AutoSectorScanZone SectorScanParameterPanel::getZoneData(const AutoSectorScanZone& baseZone) const
{
    AutoSectorScanZone result = baseZone;
    result.isEnabled = m_isEnabled;
    result.scanSpeed = m_scanSpeedValue;
    return result;
}

void SectorScanParameterPanel::setActiveField(Field field)
{
    m_activeField = field;
    m_editMode = EditMode::Navigation;  // Retour en mode navigation
    updateFieldHighlighting();
}

void SectorScanParameterPanel::handleUpInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        if (m_activeField == Field::ScanSpeed) {
            adjustScanSpeed(true);  // Augmenter
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::Enabled:
                setActiveField(Field::CancelButton);
                break;
            case Field::ScanSpeed:
                setActiveField(Field::Enabled);
                break;
            case Field::ValidateButton:
                setActiveField(Field::ScanSpeed);
                break;
            case Field::CancelButton:
                setActiveField(Field::ValidateButton);
                break;
        }
    }
}

void SectorScanParameterPanel::handleDownInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        if (m_activeField == Field::ScanSpeed) {
            adjustScanSpeed(false);  // Diminuer
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::Enabled:
                setActiveField(Field::ScanSpeed);
                break;
            case Field::ScanSpeed:
                setActiveField(Field::ValidateButton);
                break;
            case Field::ValidateButton:
                setActiveField(Field::CancelButton);
                break;
            case Field::CancelButton:
                setActiveField(Field::Enabled);
                break;
        }
    }
}

void SectorScanParameterPanel::handleSelectInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // Sortir du mode édition
        exitValueEditMode();
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::Enabled:
                toggleActiveCheckbox();
                break;
            case Field::ScanSpeed:
                enterValueEditMode();
                break;
            case Field::ValidateButton:
                onValidateClicked();
                emit parametersChanged(); // Émettre le signal ici
                break;
            case Field::CancelButton:
                onCancelClicked();
                emit parametersChanged(); // Émettre le signal ici
                break;
        }
    }
}

void SectorScanParameterPanel::toggleActiveCheckbox()
{
    if (m_activeField == Field::Enabled) {
        m_isEnabled = !m_isEnabled;
        m_enabledCheckBox->setChecked(m_isEnabled);
    }
}

void SectorScanParameterPanel::enterValueEditMode()
{
    if (m_activeField == Field::ScanSpeed) {
        m_editMode = EditMode::ValueEdit;
        m_scanSpeedEditIndicator->setVisible(true);
        updateFieldHighlighting();
    }
}

void SectorScanParameterPanel::exitValueEditMode()
{
    m_editMode = EditMode::Navigation;
    m_scanSpeedEditIndicator->setVisible(false);
    updateFieldHighlighting();
}

void SectorScanParameterPanel::adjustScanSpeed(bool increase)
{
    if (increase) {
        if (m_scanSpeedValue < 10) {
            m_scanSpeedValue++;
        }
    } else {
        if (m_scanSpeedValue > 1) {
            m_scanSpeedValue--;
        }
    }
    updateScanSpeedDisplay();
}

void SectorScanParameterPanel::updateScanSpeedDisplay()
{
    m_scanSpeedValueLabel->setText(QString("%1 deg/s").arg(m_scanSpeedValue));
}

void SectorScanParameterPanel::updateColorTheme(const QString& colorStyle)
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

    updateFieldHighlighting();
}

void SectorScanParameterPanel::updateFieldHighlighting()
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
    m_scanSpeedLabel->setStyleSheet(m_labelStyle);
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
    //QString activeStyle = (m_editMode == EditMode::ValueEdit) ? EDIT_MODE_STYLE : ACTIVE_STYLE;
    
    switch (m_activeField) {
        case Field::Enabled:
            m_enabledLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::ScanSpeed:
            m_scanSpeedLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::ValidateButton:
            m_validateButton->setStyleSheet(highlightButtonStyle);
            break;
        case Field::CancelButton:
            m_cancelButton->setStyleSheet(highlightButtonStyle);
            break;
    }
}

void SectorScanParameterPanel::onValidateClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "SectorScanParameterPanel: Validate clicked";
}

void SectorScanParameterPanel::onCancelClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "SectorScanParameterPanel: Cancel clicked";
}
