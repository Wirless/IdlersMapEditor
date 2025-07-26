#ifndef RME_MONSTER_LOOT_DIALOG_H_
#define RME_MONSTER_LOOT_DIALOG_H_

#include "main.h"
#include "monster_manager.h"

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/button.h>

// Forward declarations
class FindItemDialog;

class MonsterLootDialog : public wxDialog {
public:
    MonsterLootDialog(wxWindow* parent, const LootEntry* loot = nullptr);
    ~MonsterLootDialog();
    
    // Get the configured loot entry
    LootEntry GetLootEntry() const;
    
    // Set the loot entry for editing
    void SetLootEntry(const LootEntry& loot);

private:
    void CreateUI();
    void UpdateItemModeControls();
    void PopulateSubTypes();
    void FetchCorpseItems();
    void LookupItemByName();
    void OpenItemPalette();
    void UpdateCorpseInfo();
    
    // Event handlers
    void OnItemModeChanged(wxCommandEvent& event);
    void OnFetchCorpse(wxCommandEvent& event);
    void OnItemLookup(wxCommandEvent& event);
    void OnCountMaxToggle(wxCommandEvent& event);
    void OnSubTypeToggle(wxCommandEvent& event);
    void OnActionIdToggle(wxCommandEvent& event);
    void OnUniqueIdToggle(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    // UI Controls
    wxRadioButton* m_useItemIdRadio;
    wxRadioButton* m_useItemNameRadio;
    
    wxSpinCtrl* m_itemIdCtrl;
    wxTextCtrl* m_itemNameCtrl;
    wxButton* m_fetchCorpseBtn;
    wxButton* m_lookupBtn;
    
    wxSpinCtrl* m_chanceCtrl;
    
    wxCheckBox* m_countMaxCheck;
    wxSpinCtrl* m_countMaxCtrl;
    
    wxCheckBox* m_subTypeCheck;
    wxComboBox* m_subTypeCombo;
    
    wxCheckBox* m_actionIdCheck;
    wxSpinCtrl* m_actionIdCtrl;
    
    wxCheckBox* m_uniqueIdCheck;
    wxSpinCtrl* m_uniqueIdCtrl;
    
    // Preview and corpse info
    wxStaticText* m_previewText;
    wxStaticText* m_corpseSizeText;
    wxStaticText* m_corpseFreeSlots;
    
    DECLARE_EVENT_TABLE()
};

enum {
    ID_LOOT_ITEM_ID_RADIO = wxID_HIGHEST + 1,
    ID_LOOT_ITEM_NAME_RADIO,
    ID_LOOT_FETCH_CORPSE,
    ID_LOOT_ITEM_LOOKUP,
    ID_LOOT_ITEM_PALETTE,
    ID_LOOT_COUNTMAX_CHECK,
    ID_LOOT_SUBTYPE_CHECK,
    ID_LOOT_ACTIONID_CHECK,
    ID_LOOT_UNIQUEID_CHECK,
    ID_LOOT_OK,
    ID_LOOT_CANCEL
};

#endif // RME_MONSTER_LOOT_DIALOG_H_ 