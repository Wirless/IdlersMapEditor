#include "main.h"
#include "monster_defense_dialog.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <random>

wxBEGIN_EVENT_TABLE(MonsterDefenseDialog, wxDialog)
    EVT_COMBOBOX(ID_DEFENSE_NAME_COMBO, MonsterDefenseDialog::OnDefenseNameChanged)
    EVT_CHECKBOX(ID_DEFENSE_ENABLE_RADIUS, MonsterDefenseDialog::OnEnableRadiusToggle)
    EVT_CHECKBOX(ID_DEFENSE_ENABLE_STATUS, MonsterDefenseDialog::OnEnableStatusToggle)
    EVT_CHECKBOX(ID_DEFENSE_ENABLE_VISUAL, MonsterDefenseDialog::OnEnableVisualToggle)
    EVT_BUTTON(ID_DEFENSE_DICE_ROLL, MonsterDefenseDialog::OnDiceRoll)
    EVT_BUTTON(ID_DEFENSE_OK, MonsterDefenseDialog::OnOK)
    EVT_BUTTON(ID_DEFENSE_CANCEL, MonsterDefenseDialog::OnCancel)
wxEND_EVENT_TABLE()

MonsterDefenseDialog::MonsterDefenseDialog(wxWindow* parent, const DefenseEntry* defense) :
    wxDialog(parent, wxID_ANY, "Advanced Defense Editor", wxDefaultPosition, wxSize(600, 650), 
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateUI();
    PopulateDefenseNames();
    PopulateEffects();
    
    if (defense) {
        SetDefenseEntry(*defense);
    } else {
        // Set default values
        m_nameCombo->SetValue("healing");
        m_typeCombo->SetSelection(0);
        m_intervalCtrl->SetValue(2000);
        m_chanceCtrl->SetValue(100);
        m_minValueCtrl->SetValue(100);
        m_maxValueCtrl->SetValue(200);
        m_radiusCtrl->SetValue(1);
        m_speedChangeCtrl->SetValue(300);
        m_durationCtrl->SetValue(10000);
    }
    
    UpdateControlStates();
}

MonsterDefenseDialog::~MonsterDefenseDialog() {
}

void MonsterDefenseDialog::CreateUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Advanced Defense Configuration");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    mainSizer->Add(title, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Basic properties section
    wxStaticBoxSizer* basicSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Basic Properties");
    
    wxFlexGridSizer* basicGridSizer = new wxFlexGridSizer(4, 2, 5, 5);
    basicGridSizer->AddGrowableCol(1);
    
    // Defense Name
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Defense Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameCombo = new wxComboBox(this, ID_DEFENSE_NAME_COMBO, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    basicGridSizer->Add(m_nameCombo, 1, wxEXPAND);
    
    // Defense Type
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Defense Type:"), 0, wxALIGN_CENTER_VERTICAL);
    m_typeCombo = new wxComboBox(this, ID_DEFENSE_TYPE_COMBO, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
    m_typeCombo->Append("healing");
    m_typeCombo->Append("speed");
    m_typeCombo->Append("invisible");
    m_typeCombo->SetSelection(0);
    basicGridSizer->Add(m_typeCombo, 1, wxEXPAND);
    
    // Interval
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Interval (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    m_intervalCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 60000, 2000);
    basicGridSizer->Add(m_intervalCtrl, 1, wxEXPAND);
    
    // Chance
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Chance (%):"), 0, wxALIGN_CENTER_VERTICAL);
    m_chanceCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 100);
    basicGridSizer->Add(m_chanceCtrl, 1, wxEXPAND);
    
    basicSizer->Add(basicGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 5);
    
    // Healing values section
    wxStaticBoxSizer* healingSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Healing Values (for healing defense)");
    
    wxFlexGridSizer* healingGridSizer = new wxFlexGridSizer(2, 2, 5, 5);
    healingGridSizer->AddGrowableCol(1);
    
    // Min Value
    healingGridSizer->Add(new wxStaticText(this, wxID_ANY, "Min Value:"), 0, wxALIGN_CENTER_VERTICAL);
    m_minValueCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 100);
    healingGridSizer->Add(m_minValueCtrl, 1, wxEXPAND);
    
    // Max Value
    healingGridSizer->Add(new wxStaticText(this, wxID_ANY, "Max Value:"), 0, wxALIGN_CENTER_VERTICAL);
    m_maxValueCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 200);
    healingGridSizer->Add(m_maxValueCtrl, 1, wxEXPAND);
    
    healingSizer->Add(healingGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(healingSizer, 0, wxEXPAND | wxALL, 5);
    
    // Advanced properties section
    wxStaticBoxSizer* advancedSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Advanced Properties");
    
    // Radius
    wxFlexGridSizer* radiusGridSizer = new wxFlexGridSizer(1, 3, 5, 5);
    radiusGridSizer->AddGrowableCol(1);
    
    m_enableRadius = new wxCheckBox(this, ID_DEFENSE_ENABLE_RADIUS, "Radius:");
    radiusGridSizer->Add(m_enableRadius, 0, wxALIGN_CENTER_VERTICAL);
    m_radiusCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 20, 1);
    radiusGridSizer->Add(m_radiusCtrl, 1, wxEXPAND);
    radiusGridSizer->Add(new wxStaticText(this, wxID_ANY, "tiles"), 0, wxALIGN_CENTER_VERTICAL);
    
    advancedSizer->Add(radiusGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(advancedSizer, 0, wxEXPAND | wxALL, 5);
    
    // Status effects section
    wxStaticBoxSizer* statusSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Status Effects (for speed/invisible defenses)");
    
    wxFlexGridSizer* statusGridSizer = new wxFlexGridSizer(2, 3, 5, 5);
    statusGridSizer->AddGrowableCol(1);
    
    m_enableStatusEffect = new wxCheckBox(this, ID_DEFENSE_ENABLE_STATUS, "Speed Change:");
    statusGridSizer->Add(m_enableStatusEffect, 0, wxALIGN_CENTER_VERTICAL);
    m_speedChangeCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1000, 1000, 300);
    statusGridSizer->Add(m_speedChangeCtrl, 1, wxEXPAND);
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "points"), 0, wxALIGN_CENTER_VERTICAL);
    
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "Duration:"), 0, wxALIGN_CENTER_VERTICAL);
    m_durationCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 300000, 10000);
    statusGridSizer->Add(m_durationCtrl, 1, wxEXPAND);
    statusGridSizer->Add(new wxStaticText(this, wxID_ANY, "ms"), 0, wxALIGN_CENTER_VERTICAL);
    
    statusSizer->Add(statusGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(statusSizer, 0, wxEXPAND | wxALL, 5);
    
    // Visual effects section
    wxStaticBoxSizer* visualSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Visual Effects");
    
    wxFlexGridSizer* visualGridSizer = new wxFlexGridSizer(1, 2, 5, 5);
    visualGridSizer->AddGrowableCol(1);
    
    m_enableVisualEffects = new wxCheckBox(this, ID_DEFENSE_ENABLE_VISUAL, "Area Effect:");
    visualGridSizer->Add(m_enableVisualEffects, 0, wxALIGN_CENTER_VERTICAL);
    m_areaEffectCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    visualGridSizer->Add(m_areaEffectCombo, 1, wxEXPAND);
    
    visualSizer->Add(visualGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(visualSizer, 0, wxEXPAND | wxALL, 5);
    
    // Dice roll section
    wxBoxSizer* diceSizer = new wxBoxSizer(wxHORIZONTAL);
    m_diceRollBtn = new wxButton(this, ID_DEFENSE_DICE_ROLL, "🛡️ Random Defense Generator");
    m_diceRollBtn->SetToolTip("Generate a random defense with realistic values based on monster examples");
    m_diceRollBtn->SetBackgroundColour(wxColour(240, 255, 240));
    diceSizer->Add(m_diceRollBtn, 0, wxALL, 5);
    diceSizer->AddStretchSpacer();
    mainSizer->Add(diceSizer, 0, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, ID_DEFENSE_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, ID_DEFENSE_CANCEL, "Cancel"), 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
}

void MonsterDefenseDialog::PopulateDefenseNames() {
    // Common defense names from the OTClient mod examples
    m_nameCombo->Append("healing");
    m_nameCombo->Append("speed");
    m_nameCombo->Append("invisible");
    m_nameCombo->Append("outfit");
    m_nameCombo->Append("drunk");
    m_nameCombo->Append("armor");
    m_nameCombo->Append("defense");
}

void MonsterDefenseDialog::PopulateEffects() {
    // Area effects from OTClient mod examples
    m_areaEffectCombo->Append("redspark");
    m_areaEffectCombo->Append("bluebubble");
    m_areaEffectCombo->Append("poff");
    m_areaEffectCombo->Append("yellowspark");
    m_areaEffectCombo->Append("explosionarea");
    m_areaEffectCombo->Append("explosion");
    m_areaEffectCombo->Append("firearea");
    m_areaEffectCombo->Append("yellowbubble");
    m_areaEffectCombo->Append("greenbubble");
    m_areaEffectCombo->Append("blackspark");
    m_areaEffectCombo->Append("teleport");
    m_areaEffectCombo->Append("energy");
    m_areaEffectCombo->Append("blueshimmer");
    m_areaEffectCombo->Append("redshimmer");
    m_areaEffectCombo->Append("greenshimmer");
    m_areaEffectCombo->Append("mortarea");
    m_areaEffectCombo->Append("blockhit");
}

void MonsterDefenseDialog::UpdateControlStates() {
    wxString defenseType = m_nameCombo->GetValue().Lower();
    
    // Enable/disable controls based on defense type
    bool isHealing = (defenseType == "healing");
    bool isStatusEffect = (defenseType == "speed" || defenseType == "invisible");
    
    m_minValueCtrl->Enable(isHealing);
    m_maxValueCtrl->Enable(isHealing);
    
    m_radiusCtrl->Enable(m_enableRadius->GetValue() && isHealing);
    
    m_speedChangeCtrl->Enable(m_enableStatusEffect->GetValue() && isStatusEffect);
    m_durationCtrl->Enable(m_enableStatusEffect->GetValue() && isStatusEffect);
    
    m_areaEffectCombo->Enable(m_enableVisualEffects->GetValue());
}

void MonsterDefenseDialog::OnDiceRollDefense() {
    // Advanced random defense generator based on real monster examples
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Defense types weighted by frequency from actual monsters
    std::vector<std::string> defenseTypes = {
        "healing", "healing", "healing",  // Most common
        "speed",   // Speed boost defenses
        "invisible"  // Invisibility defenses
    };
    
    std::uniform_int_distribution<> typeDist(0, defenseTypes.size() - 1);
    std::string selectedType = defenseTypes[typeDist(gen)];
    
    m_nameCombo->SetValue(selectedType);
    m_typeCombo->SetStringSelection(selectedType);
    
    // Clear all checkboxes first
    m_enableRadius->SetValue(false);
    m_enableStatusEffect->SetValue(false);
    m_enableVisualEffects->SetValue(false);
    
    // Generate realistic properties based on defense type
    std::uniform_int_distribution<> chanceDist(10, 30);
    std::uniform_int_distribution<> intervalDist(1500, 4000);
    
    m_intervalCtrl->SetValue(intervalDist(gen));
    m_chanceCtrl->SetValue(chanceDist(gen));
    
    if (selectedType == "healing") {
        // Healing defenses: 50-500 HP healing
        std::uniform_int_distribution<> healDist(50, 500);
        int minHeal = healDist(gen);
        int maxHeal = minHeal + healDist(gen) / 2;
        
        m_minValueCtrl->SetValue(minHeal);
        m_maxValueCtrl->SetValue(maxHeal);
        
        // 30% chance for radius healing
        std::uniform_int_distribution<> boolDist(0, 2);
        if (boolDist(gen) == 0) {
            m_enableRadius->SetValue(true);
            m_radiusCtrl->SetValue(std::uniform_int_distribution<>(1, 3)(gen));
        }
        
        // Visual effects (80% chance)
        if (std::uniform_int_distribution<>(0, 4)(gen) != 0) {
            m_enableVisualEffects->SetValue(true);
            m_areaEffectCombo->SetValue("bluebubble");
        }
        
    } else if (selectedType == "speed") {
        // Speed defenses: +200 to +600 speed boost
        std::uniform_int_distribution<> speedDist(200, 600);
        std::uniform_int_distribution<> durationDist(5000, 15000);
        
        m_enableStatusEffect->SetValue(true);
        m_speedChangeCtrl->SetValue(speedDist(gen));
        m_durationCtrl->SetValue(durationDist(gen));
        
        m_enableVisualEffects->SetValue(true);
        m_areaEffectCombo->SetValue("redshimmer");
        
    } else if (selectedType == "invisible") {
        // Invisibility defenses: 3-10 second duration
        std::uniform_int_distribution<> durationDist(3000, 10000);
        
        m_enableStatusEffect->SetValue(true);
        m_durationCtrl->SetValue(durationDist(gen));
        
        m_enableVisualEffects->SetValue(true);
        m_areaEffectCombo->SetValue("poff");
    }
    
    UpdateControlStates();
    
    wxMessageBox(wxString::Format("🛡️ Random %s defense generated!\n\nInterval: %dms\nChance: %d%%", 
                                 selectedType, m_intervalCtrl->GetValue(), m_chanceCtrl->GetValue()), 
                "Dice Roll Complete", wxOK | wxICON_INFORMATION);
}

DefenseEntry MonsterDefenseDialog::GetDefenseEntry() const {
    DefenseEntry defense;
    
    defense.name = nstr(m_nameCombo->GetValue());
    defense.type = nstr(m_typeCombo->GetStringSelection());
    defense.interval = m_intervalCtrl->GetValue();
    defense.chance = m_chanceCtrl->GetValue();
    defense.minValue = m_minValueCtrl->GetValue();
    defense.maxValue = m_maxValueCtrl->GetValue();
    
    // Store additional properties in the type field as XML attributes
    wxString additionalProps;
    
    if (m_enableRadius->GetValue() && m_radiusCtrl->GetValue() > 0) {
        additionalProps += wxString::Format(" radius=\"%d\"", m_radiusCtrl->GetValue());
    }
    
    if (m_enableStatusEffect->GetValue()) {
        if (m_speedChangeCtrl->GetValue() != 0) {
            additionalProps += wxString::Format(" speedchange=\"%d\"", m_speedChangeCtrl->GetValue());
        }
        if (m_durationCtrl->GetValue() > 0) {
            additionalProps += wxString::Format(" duration=\"%d\"", m_durationCtrl->GetValue());
        }
    }
    
    if (m_enableVisualEffects->GetValue() && !m_areaEffectCombo->GetValue().IsEmpty()) {
        additionalProps += wxString::Format(" areaEffect=\"%s\"", nstr(m_areaEffectCombo->GetValue()));
    }
    
    defense.type += nstr(additionalProps);
    
    return defense;
}

void MonsterDefenseDialog::SetDefenseEntry(const DefenseEntry& defense) {
    m_nameCombo->SetValue(wxstr(defense.name));
    
    // Parse type and additional properties
    wxString typeStr = wxstr(defense.type);
    wxString baseType = typeStr.BeforeFirst(' ');
    m_typeCombo->SetStringSelection(baseType);
    
    m_intervalCtrl->SetValue(defense.interval);
    m_chanceCtrl->SetValue(defense.chance);
    m_minValueCtrl->SetValue(defense.minValue);
    m_maxValueCtrl->SetValue(defense.maxValue);
    
    // Parse additional properties from type string
    if (typeStr.Contains("radius=")) {
        m_enableRadius->SetValue(true);
        // Extract radius value
        int start = typeStr.Find("radius=\"") + 8;
        int end = typeStr.find("\"", start);
        if (end != wxNOT_FOUND) {
            long radius;
            typeStr.Mid(start, end - start).ToLong(&radius);
            m_radiusCtrl->SetValue(radius);
        }
    }
    
    if (typeStr.Contains("speedchange=") || typeStr.Contains("duration=")) {
        m_enableStatusEffect->SetValue(true);
        // Parse speedchange and duration values
        // ... (similar parsing logic)
    }
    
    if (typeStr.Contains("areaEffect=")) {
        m_enableVisualEffects->SetValue(true);
        // Extract area effect value
        // ... (similar parsing logic)
    }
    
    UpdateControlStates();
}

// Event handlers
void MonsterDefenseDialog::OnDefenseNameChanged(wxCommandEvent& event) {
    // Auto-suggest properties based on defense name
    wxString name = m_nameCombo->GetValue().Lower();
    
    if (name == "healing") {
        m_typeCombo->SetStringSelection("healing");
        m_areaEffectCombo->SetValue("bluebubble");
        m_enableVisualEffects->SetValue(true);
    } else if (name == "speed") {
        m_typeCombo->SetStringSelection("speed");
        m_areaEffectCombo->SetValue("redshimmer");
        m_enableVisualEffects->SetValue(true);
        m_enableStatusEffect->SetValue(true);
    } else if (name == "invisible") {
        m_typeCombo->SetStringSelection("invisible");
        m_areaEffectCombo->SetValue("poff");
        m_enableVisualEffects->SetValue(true);
        m_enableStatusEffect->SetValue(true);
    }
    
    UpdateControlStates();
}

void MonsterDefenseDialog::OnEnableRadiusToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterDefenseDialog::OnEnableStatusToggle(wxCommandEvent& event) { UpdateControlStates(); }
void MonsterDefenseDialog::OnEnableVisualToggle(wxCommandEvent& event) { UpdateControlStates(); }

void MonsterDefenseDialog::OnDiceRoll(wxCommandEvent& event) {
    OnDiceRollDefense();
}

void MonsterDefenseDialog::OnOK(wxCommandEvent& event) {
    if (m_nameCombo->GetValue().IsEmpty()) {
        wxMessageBox("Please enter a defense name.", "Validation Error", wxOK | wxICON_WARNING);
        return;
    }
    
    EndModal(wxID_OK);
}

void MonsterDefenseDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
} 