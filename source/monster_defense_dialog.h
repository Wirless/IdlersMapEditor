#ifndef RME_MONSTER_DEFENSE_DIALOG_H_
#define RME_MONSTER_DEFENSE_DIALOG_H_

#include "main.h"
#include "monster_manager.h"

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>

class MonsterDefenseDialog : public wxDialog {
public:
    MonsterDefenseDialog(wxWindow* parent, const DefenseEntry* defense = nullptr);
    ~MonsterDefenseDialog();
    
    // Get the configured defense entry
    DefenseEntry GetDefenseEntry() const;
    
    // Set the defense entry for editing
    void SetDefenseEntry(const DefenseEntry& defense);

private:
    void CreateUI();
    void PopulateDefenseNames();
    void PopulateEffects();
    void UpdateControlStates();
    void OnDiceRollDefense();
    
    // Event handlers
    void OnDefenseNameChanged(wxCommandEvent& event);
    void OnEnableRadiusToggle(wxCommandEvent& event);
    void OnEnableStatusToggle(wxCommandEvent& event);
    void OnEnableVisualToggle(wxCommandEvent& event);
    void OnDiceRoll(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    // Basic Controls
    wxComboBox* m_nameCombo;
    wxComboBox* m_typeCombo;
    wxSpinCtrl* m_intervalCtrl;
    wxSpinCtrl* m_chanceCtrl;
    wxSpinCtrl* m_minValueCtrl;
    wxSpinCtrl* m_maxValueCtrl;
    
    // Advanced Properties
    wxCheckBox* m_enableRadius;
    wxSpinCtrl* m_radiusCtrl;
    
    // Status Effects (for speed and invisible defenses)
    wxCheckBox* m_enableStatusEffect;
    wxSpinCtrl* m_speedChangeCtrl;
    wxSpinCtrl* m_durationCtrl;
    
    // Visual Effects
    wxCheckBox* m_enableVisualEffects;
    wxComboBox* m_areaEffectCombo;
    
    // Dice roll button
    wxButton* m_diceRollBtn;
    
    DECLARE_EVENT_TABLE()
};

enum {
    ID_DEFENSE_NAME_COMBO = wxID_HIGHEST + 100,
    ID_DEFENSE_TYPE_COMBO,
    ID_DEFENSE_ENABLE_RADIUS,
    ID_DEFENSE_ENABLE_STATUS,
    ID_DEFENSE_ENABLE_VISUAL,
    ID_DEFENSE_DICE_ROLL,
    ID_DEFENSE_OK,
    ID_DEFENSE_CANCEL
};

#endif // RME_MONSTER_DEFENSE_DIALOG_H_ 