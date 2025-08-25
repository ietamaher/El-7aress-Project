#include "trpparameterpanel.h"
#include <QDebug>

/*const QString TRPParameterPanel::ACTIVE_STYLE = "QLabel { background-color: #4CAF50; color: white; padding: 2px; }";
const QString TRPParameterPanel::INACTIVE_STYLE = "QLabel { background-color: transparent; color: black; }";
const QString TRPParameterPanel::EDIT_MODE_STYLE = "QLabel { background-color: #FF9800; color: white; padding: 2px; }";

TRPParameterPanel::TRPParameterPanel(QWidget *parent)
    : QWidget(parent)
    , m_activeField(Field::LocationPage)
    , m_editMode(EditMode::Navigation)
    , m_locationPageValue(1)
    , m_trpInPageValue(1)
    , m_haltTimeValue(10)  // 1.0 seconde par défaut
{
    setupUI();
    updateFieldHighlighting();
}

void TRPParameterPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("TRP Parameters", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_mainLayout->addWidget(m_titleLabel);
    
    // Location Page field
    m_locationPageLayout = new QHBoxLayout();
    m_locationPageLabel = new QLabel("Location Page:", this);
    m_locationPageValueLabel = new QLabel(QString::number(m_locationPageValue), this);
    m_locationPageEditIndicator = new QLabel("[EDIT]", this);
    m_locationPageEditIndicator->setVisible(false);
    m_locationPageLayout->addWidget(m_locationPageLabel);
    m_locationPageLayout->addWidget(m_locationPageValueLabel);
    m_locationPageLayout->addWidget(m_locationPageEditIndicator);
    m_locationPageLayout->addStretch();
    m_mainLayout->addLayout(m_locationPageLayout);
    
    // TRP In Page field
    m_trpInPageLayout = new QHBoxLayout();
    m_trpInPageLabel = new QLabel("TRP Index:", this);
    m_trpInPageValueLabel = new QLabel(QString::number(m_trpInPageValue), this);
    m_trpInPageEditIndicator = new QLabel("[EDIT]", this);
    m_trpInPageEditIndicator->setVisible(false);
    m_trpInPageLayout->addWidget(m_trpInPageLabel);
    m_trpInPageLayout->addWidget(m_trpInPageValueLabel);
    m_trpInPageLayout->addWidget(m_trpInPageEditIndicator);
    m_trpInPageLayout->addStretch();
    m_mainLayout->addLayout(m_trpInPageLayout);
    
    // Halt Time field
    m_haltTimeLayout = new QHBoxLayout();
    m_haltTimeLabel = new QLabel("Halt Time:", this);
    m_haltTimeValueLabel = new QLabel(QString("%1 sec").arg(m_haltTimeValue / 10.0, 0, 'f', 1), this);
    m_haltTimeEditIndicator = new QLabel("[EDIT]", this);
    m_haltTimeEditIndicator->setVisible(false);
    m_haltTimeLayout->addWidget(m_haltTimeLabel);
    m_haltTimeLayout->addWidget(m_haltTimeValueLabel);
    m_haltTimeLayout->addWidget(m_haltTimeEditIndicator);
    m_haltTimeLayout->addStretch();
    m_mainLayout->addLayout(m_haltTimeLayout);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_validateButton = new QPushButton("Validate", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_buttonLayout->addWidget(m_validateButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_validateButton, &QPushButton::clicked, this, &TRPParameterPanel::onValidateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &TRPParameterPanel::onCancelClicked);
}

void TRPParameterPanel::setTRPData(const TargetReferencePoint& trp)
{
    m_locationPageValue = trp.locationPage;
    m_trpInPageValue = trp.trpInPage;
    m_haltTimeValue = static_cast<int>(trp.haltTime * 10);  // Convertir secondes en dixièmes
    
    updateLocationPageDisplay();
    updateTrpInPageDisplay();
    updateHaltTimeDisplay();
}

TargetReferencePoint TRPParameterPanel::getTRPData(const TargetReferencePoint& baseTRP) const
{
    TargetReferencePoint result = baseTRP;
    result.locationPage = m_locationPageValue;
    result.trpInPage = m_trpInPageValue;
    result.haltTime = m_haltTimeValue / 10.0;  // Convertir dixièmes en secondes
    return result;
}

void TRPParameterPanel::setActiveField(Field field)
{
    m_activeField = field;
    m_editMode = EditMode::Navigation;  // Retour en mode navigation
    updateFieldHighlighting();
}

void TRPParameterPanel::handleUpInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        switch (m_activeField) {
            case Field::LocationPage:
                adjustLocationPage(true);
                break;
            case Field::TrpInPage:
                adjustTrpInPage(true);
                break;
            case Field::HaltTime:
                adjustHaltTime(true);
                break;
            default:
                break;
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
                setActiveField(Field::CancelButton);
                break;
            case Field::TrpInPage:
                setActiveField(Field::LocationPage);
                break;
            case Field::HaltTime:
                setActiveField(Field::TrpInPage);
                break;
            case Field::ValidateButton:
                setActiveField(Field::HaltTime);
                break;
            case Field::CancelButton:
                setActiveField(Field::ValidateButton);
                break;
        }
    }
}

void TRPParameterPanel::handleDownInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        switch (m_activeField) {
            case Field::LocationPage:
                adjustLocationPage(false);
                break;
            case Field::TrpInPage:
                adjustTrpInPage(false);
                break;
            case Field::HaltTime:
                adjustHaltTime(false);
                break;
            default:
                break;
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
                setActiveField(Field::TrpInPage);
                break;
            case Field::TrpInPage:
                setActiveField(Field::HaltTime);
                break;
            case Field::HaltTime:
                setActiveField(Field::ValidateButton);
                break;
            case Field::ValidateButton:
                setActiveField(Field::CancelButton);
                break;
            case Field::CancelButton:
                setActiveField(Field::LocationPage);
                break;
        }
    }
}

void TRPParameterPanel::handleSelectInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // Sortir du mode édition
        exitValueEditMode();
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
            case Field::TrpInPage:
            case Field::HaltTime:
                enterValueEditMode();
                break;
            case Field::ValidateButton:
                onValidateClicked();
                break;
            case Field::CancelButton:
                onCancelClicked();
                break;
        }
    }
}

void TRPParameterPanel::enterValueEditMode()
{
    m_editMode = EditMode::ValueEdit;
    
    // Afficher l'indicateur d'édition pour le champ actif
    switch (m_activeField) {
        case Field::LocationPage:
            m_locationPageEditIndicator->setVisible(true);
            break;
        case Field::TrpInPage:
            m_trpInPageEditIndicator->setVisible(true);
            break;
        case Field::HaltTime:
            m_haltTimeEditIndicator->setVisible(true);
            break;
        default:
            break;
    }
    
    updateFieldHighlighting();
}

void TRPParameterPanel::exitValueEditMode()
{
    m_editMode = EditMode::Navigation;
    
    // Masquer tous les indicateurs d'édition
    m_locationPageEditIndicator->setVisible(false);
    m_trpInPageEditIndicator->setVisible(false);
    m_haltTimeEditIndicator->setVisible(false);
    
    updateFieldHighlighting();
}

void TRPParameterPanel::adjustLocationPage(bool increase)
{
    if (increase) {
        if (m_locationPageValue < 200) {
            m_locationPageValue++;
        }
    } else {
        if (m_locationPageValue > 1) {
            m_locationPageValue--;
        }
    }
    updateLocationPageDisplay();
}

void TRPParameterPanel::adjustTrpInPage(bool increase)
{
    if (increase) {
        if (m_trpInPageValue < 50) {
            m_trpInPageValue++;
        }
    } else {
        if (m_trpInPageValue > 1) {
            m_trpInPageValue--;
        }
    }
    updateTrpInPageDisplay();
}

void TRPParameterPanel::adjustHaltTime(bool increase)
{
    if (increase) {
        if (m_haltTimeValue < 600) {  // Maximum 60.0 secondes
            m_haltTimeValue += 10;    // Incrément de 1.0 seconde
        }
    } else {
        if (m_haltTimeValue > 10) {   // Minimum 1.0 seconde
            m_haltTimeValue -= 10;    // Décrément de 1.0 seconde
        }
    }
    updateHaltTimeDisplay();
}

void TRPParameterPanel::updateLocationPageDisplay()
{
    m_locationPageValueLabel->setText(QString::number(m_locationPageValue));
}

void TRPParameterPanel::updateTrpInPageDisplay()
{
    m_trpInPageValueLabel->setText(QString::number(m_trpInPageValue));
}

void TRPParameterPanel::updateHaltTimeDisplay()
{
    m_haltTimeValueLabel->setText(QString("%1 sec").arg(m_haltTimeValue / 10.0, 0, 'f', 1));
}

void TRPParameterPanel::updateFieldHighlighting()
{
    // Reset all styles
    m_locationPageLabel->setStyleSheet(INACTIVE_STYLE);
    m_trpInPageLabel->setStyleSheet(INACTIVE_STYLE);
    m_haltTimeLabel->setStyleSheet(INACTIVE_STYLE);
    m_validateButton->setStyleSheet("");
    m_cancelButton->setStyleSheet("");
    
    // Highlight active field
    QString activeStyle = (m_editMode == EditMode::ValueEdit) ? EDIT_MODE_STYLE : ACTIVE_STYLE;
    
    switch (m_activeField) {
        case Field::LocationPage:
            m_locationPageLabel->setStyleSheet(activeStyle);
            break;
        case Field::TrpInPage:
            m_trpInPageLabel->setStyleSheet(activeStyle);
            break;
        case Field::HaltTime:
            m_haltTimeLabel->setStyleSheet(activeStyle);
            break;
        case Field::ValidateButton:
            m_validateButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; }");
            break;
        case Field::CancelButton:
            m_cancelButton->setStyleSheet("QPushButton { background-color: #f44336; color: white; }");
            break;
    }
}

void TRPParameterPanel::onValidateClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "TRPParameterPanel: Validate clicked";
}

void TRPParameterPanel::onCancelClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "TRPParameterPanel: Cancel clicked";
}
*/

const QString TRPParameterPanel::ACTIVE_STYLE = "QLabel { background-color: #4CAF50; color: white; padding: 2px; }";
const QString TRPParameterPanel::INACTIVE_STYLE = "QLabel { background-color: transparent; color: black; }";
const QString TRPParameterPanel::EDIT_MODE_STYLE = "QLabel { background-color: #FF9800; color: white; padding: 2px; }";

TRPParameterPanel::TRPParameterPanel(QWidget *parent)
    : QWidget(parent)
    , m_activeField(Field::LocationPage)
    , m_editMode(EditMode::Navigation)
    , m_locationPageValue(1)
    , m_trpInPageValue(1)
    , m_haltTimeValue(10)  // 1.0 seconde par défaut
{
    setupUI();
    updateFieldHighlighting();
}

void TRPParameterPanel::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    
    // Title
    m_titleLabel = new QLabel("TRP Parameters", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_mainLayout->addWidget(m_titleLabel);
    
    // Location Page field
    m_locationPageLayout = new QHBoxLayout();
    m_locationPageLabel = new QLabel("Location Page:", this);
    m_locationPageValueLabel = new QLabel(QString::number(m_locationPageValue), this);
    m_locationPageEditIndicator = new QLabel("[EDIT]", this);
    m_locationPageEditIndicator->setVisible(false);
    m_locationPageLayout->addWidget(m_locationPageLabel);
    m_locationPageLayout->addWidget(m_locationPageValueLabel);
    m_locationPageLayout->addWidget(m_locationPageEditIndicator);
    m_locationPageLayout->addStretch();
    m_mainLayout->addLayout(m_locationPageLayout);
    
    // TRP In Page field
    m_trpInPageLayout = new QHBoxLayout();
    m_trpInPageLabel = new QLabel("TRP Index:", this);
    m_trpInPageValueLabel = new QLabel(QString::number(m_trpInPageValue), this);
    m_trpInPageEditIndicator = new QLabel("[EDIT]", this);
    m_trpInPageEditIndicator->setVisible(false);
    m_trpInPageLayout->addWidget(m_trpInPageLabel);
    m_trpInPageLayout->addWidget(m_trpInPageValueLabel);
    m_trpInPageLayout->addWidget(m_trpInPageEditIndicator);
    m_trpInPageLayout->addStretch();
    m_mainLayout->addLayout(m_trpInPageLayout);
    
    // Halt Time field
    m_haltTimeLayout = new QHBoxLayout();
    m_haltTimeLabel = new QLabel("Halt Time:", this);
    m_haltTimeValueLabel = new QLabel(QString("%1 sec").arg(m_haltTimeValue / 10.0, 0, 'f', 1), this);
    m_haltTimeEditIndicator = new QLabel("[EDIT]", this);
    m_haltTimeEditIndicator->setVisible(false);
    m_haltTimeLayout->addWidget(m_haltTimeLabel);
    m_haltTimeLayout->addWidget(m_haltTimeValueLabel);
    m_haltTimeLayout->addWidget(m_haltTimeEditIndicator);
    m_haltTimeLayout->addStretch();
    m_mainLayout->addLayout(m_haltTimeLayout);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_validateButton = new QPushButton("Validate", this);
    m_cancelButton = new QPushButton("Cancel", this);
    m_buttonLayout->addWidget(m_validateButton);
    m_buttonLayout->addWidget(m_cancelButton);
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Connect signals
    connect(m_validateButton, &QPushButton::clicked, this, &TRPParameterPanel::onValidateClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &TRPParameterPanel::onCancelClicked);
}

void TRPParameterPanel::setTRPData(const TargetReferencePoint& trp)
{
    m_locationPageValue = trp.locationPage;
    m_trpInPageValue = trp.trpInPage;
    m_haltTimeValue = static_cast<int>(trp.haltTime * 10);  // Convertir secondes en dixièmes
    
    updateLocationPageDisplay();
    updateTrpInPageDisplay();
    updateHaltTimeDisplay();
}

TargetReferencePoint TRPParameterPanel::getTRPData(const TargetReferencePoint& baseTRP) const
{
    TargetReferencePoint result = baseTRP;
    result.locationPage = m_locationPageValue;
    result.trpInPage = m_trpInPageValue;
    result.haltTime = m_haltTimeValue / 10.0;  // Convertir dixièmes en secondes
    return result;
}

void TRPParameterPanel::setActiveField(Field field)
{
    m_activeField = field;
    m_editMode = EditMode::Navigation;  // Retour en mode navigation
    updateFieldHighlighting();
}

void TRPParameterPanel::handleUpInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        switch (m_activeField) {
            case Field::LocationPage:
                adjustLocationPage(true);
                break;
            case Field::TrpInPage:
                adjustTrpInPage(true);
                break;
            case Field::HaltTime:
                adjustHaltTime(true);
                break;
            default:
                break;
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
                setActiveField(Field::CancelButton);
                break;
            case Field::TrpInPage:
                setActiveField(Field::LocationPage);
                break;
            case Field::HaltTime:
                setActiveField(Field::TrpInPage);
                break;
            case Field::ValidateButton:
                setActiveField(Field::HaltTime);
                break;
            case Field::CancelButton:
                setActiveField(Field::ValidateButton);
                break;
        }
    }
}

void TRPParameterPanel::handleDownInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // En mode édition de valeur
        switch (m_activeField) {
            case Field::LocationPage:
                adjustLocationPage(false);
                break;
            case Field::TrpInPage:
                adjustTrpInPage(false);
                break;
            case Field::HaltTime:
                adjustHaltTime(false);
                break;
            default:
                break;
        }
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
                setActiveField(Field::TrpInPage);
                break;
            case Field::TrpInPage:
                setActiveField(Field::HaltTime);
                break;
            case Field::HaltTime:
                setActiveField(Field::ValidateButton);
                break;
            case Field::ValidateButton:
                setActiveField(Field::CancelButton);
                break;
            case Field::CancelButton:
                setActiveField(Field::LocationPage);
                break;
        }
    }
}

void TRPParameterPanel::handleSelectInput()
{
    if (m_editMode == EditMode::ValueEdit) {
        // Sortir du mode édition
        exitValueEditMode();
    } else {
        // En mode navigation
        switch (m_activeField) {
            case Field::LocationPage:
            case Field::TrpInPage:
            case Field::HaltTime:
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

void TRPParameterPanel::enterValueEditMode()
{
    m_editMode = EditMode::ValueEdit;
    
    // Afficher l'indicateur d'édition pour le champ actif
    switch (m_activeField) {
        case Field::LocationPage:
            m_locationPageEditIndicator->setVisible(true);
            break;
        case Field::TrpInPage:
            m_trpInPageEditIndicator->setVisible(true);
            break;
        case Field::HaltTime:
            m_haltTimeEditIndicator->setVisible(true);
            break;
        default:
            break;
    }
    
    updateFieldHighlighting();
}

void TRPParameterPanel::exitValueEditMode()
{
    m_editMode = EditMode::Navigation;
    
    // Masquer tous les indicateurs d'édition
    m_locationPageEditIndicator->setVisible(false);
    m_trpInPageEditIndicator->setVisible(false);
    m_haltTimeEditIndicator->setVisible(false);
    
    updateFieldHighlighting();
}

void TRPParameterPanel::adjustLocationPage(bool increase)
{
    if (increase) {
        if (m_locationPageValue < 200) {
            m_locationPageValue++;
        }
    } else {
        if (m_locationPageValue > 1) {
            m_locationPageValue--;
        }
    }
    updateLocationPageDisplay();
}

void TRPParameterPanel::adjustTrpInPage(bool increase)
{
    if (increase) {
        if (m_trpInPageValue < 50) {
            m_trpInPageValue++;
        }
    } else {
        if (m_trpInPageValue > 1) {
            m_trpInPageValue--;
        }
    }
    updateTrpInPageDisplay();
}

void TRPParameterPanel::adjustHaltTime(bool increase)
{
    if (increase) {
        if (m_haltTimeValue < 600) {  // Maximum 60.0 secondes
            m_haltTimeValue += 10;    // Incrément de 1.0 seconde
        }
    } else {
        if (m_haltTimeValue > 10) {   // Minimum 1.0 seconde
            m_haltTimeValue -= 10;    // Décrément de 1.0 seconde
        }
    }
    updateHaltTimeDisplay();
}

void TRPParameterPanel::updateLocationPageDisplay()
{
    m_locationPageValueLabel->setText(QString::number(m_locationPageValue));
}

void TRPParameterPanel::updateTrpInPageDisplay()
{
    m_trpInPageValueLabel->setText(QString::number(m_trpInPageValue));
}

void TRPParameterPanel::updateHaltTimeDisplay()
{
    m_haltTimeValueLabel->setText(QString("%1 sec").arg(m_haltTimeValue / 10.0, 0, 'f', 1));
}

void TRPParameterPanel::updateColorTheme(const QString& colorStyle)
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

    updateFieldHighlighting();
}

void TRPParameterPanel::updateFieldHighlighting()
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
    m_trpInPageLabel->setStyleSheet(m_labelStyle);
    m_locationPageLabel->setStyleSheet(m_labelStyle);
    m_haltTimeLabel->setStyleSheet(m_labelStyle);
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

    // Highlight active field
    //QString activeStyle = (m_editMode == EditMode::ValueEdit) ? EDIT_MODE_STYLE : ACTIVE_STYLE;
    
    switch (m_activeField) {
        case Field::LocationPage:
            m_locationPageLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::TrpInPage:
            m_trpInPageLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::HaltTime:
            m_haltTimeLabel->setStyleSheet(highlightLabelStyle);
            break;
        case Field::ValidateButton:
            m_validateButton->setStyleSheet(highlightButtonStyle);
            break;
        case Field::CancelButton:
            m_cancelButton->setStyleSheet(highlightButtonStyle);
            break;
    }
}

void TRPParameterPanel::onValidateClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "TRPParameterPanel: Validate clicked";
}

void TRPParameterPanel::onCancelClicked()
{
    // Signal sera géré par ZoneDefinitionWidget
    qDebug() << "TRPParameterPanel: Cancel clicked";
}


