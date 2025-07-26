#include "main.h"
#include "monster_loot_dialog.h"
#include "find_item_window.h"
#include "gui.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/statbox.h>

wxBEGIN_EVENT_TABLE(MonsterLootDialog, wxDialog)
    EVT_RADIOBUTTON(ID_LOOT_ITEM_ID_RADIO, MonsterLootDialog::OnItemModeChanged)
    EVT_RADIOBUTTON(ID_LOOT_ITEM_NAME_RADIO, MonsterLootDialog::OnItemModeChanged)
    EVT_BUTTON(ID_LOOT_FETCH_CORPSE, MonsterLootDialog::OnFetchCorpse)
    EVT_BUTTON(ID_LOOT_ITEM_LOOKUP, MonsterLootDialog::OnItemLookup)
    EVT_BUTTON(ID_LOOT_ITEM_PALETTE, MonsterLootDialog::OnItemLookup)
    EVT_CHECKBOX(ID_LOOT_COUNTMAX_CHECK, MonsterLootDialog::OnCountMaxToggle)
    EVT_CHECKBOX(ID_LOOT_SUBTYPE_CHECK, MonsterLootDialog::OnSubTypeToggle)
    EVT_CHECKBOX(ID_LOOT_ACTIONID_CHECK, MonsterLootDialog::OnActionIdToggle)
    EVT_CHECKBOX(ID_LOOT_UNIQUEID_CHECK, MonsterLootDialog::OnUniqueIdToggle)
    EVT_BUTTON(ID_LOOT_OK, MonsterLootDialog::OnOK)
    EVT_BUTTON(ID_LOOT_CANCEL, MonsterLootDialog::OnCancel)
wxEND_EVENT_TABLE()

MonsterLootDialog::MonsterLootDialog(wxWindow* parent, const LootEntry* loot) :
    wxDialog(parent, wxID_ANY, "Loot Editor", wxDefaultPosition, wxSize(500, 450), 
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    CreateUI();
    PopulateSubTypes();
    
    if (loot) {
        SetLootEntry(*loot);
    } else {
        // Set default values
        m_useItemIdRadio->SetValue(true);
        m_itemIdCtrl->SetValue(1);
        m_chanceCtrl->SetValue(100000); // 100%
        m_countMaxCtrl->SetValue(1);
        m_countMaxCtrl->Enable(false);
        m_subTypeCombo->Enable(false);
        m_actionIdCtrl->Enable(false);
        m_uniqueIdCtrl->Enable(false);
    }
    
    UpdateItemModeControls();
}

MonsterLootDialog::~MonsterLootDialog() {
}

void MonsterLootDialog::CreateUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Item selection section
    wxStaticBoxSizer* itemSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Item Selection");
    
    // Radio buttons for item mode
    wxBoxSizer* modeSizer = new wxBoxSizer(wxHORIZONTAL);
    m_useItemIdRadio = new wxRadioButton(this, ID_LOOT_ITEM_ID_RADIO, "Use Item ID", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_useItemNameRadio = new wxRadioButton(this, ID_LOOT_ITEM_NAME_RADIO, "Use Item Name");
    modeSizer->Add(m_useItemIdRadio, 0, wxRIGHT, 10);
    modeSizer->Add(m_useItemNameRadio, 0, 0, 0);
    itemSizer->Add(modeSizer, 0, wxALL, 5);
    
    // Item ID/Name controls
    wxFlexGridSizer* itemGridSizer = new wxFlexGridSizer(2, 2, 5, 5);
    itemGridSizer->AddGrowableCol(1);
    
    itemGridSizer->Add(new wxStaticText(this, wxID_ANY, "Item ID:"), 0, wxALIGN_CENTER_VERTICAL);
    m_itemIdCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1);
    itemGridSizer->Add(m_itemIdCtrl, 1, wxEXPAND);
    
    itemGridSizer->Add(new wxStaticText(this, wxID_ANY, "Item Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_itemNameCtrl = new wxTextCtrl(this, wxID_ANY);
    itemGridSizer->Add(m_itemNameCtrl, 1, wxEXPAND);
    
    itemSizer->Add(itemGridSizer, 0, wxEXPAND | wxALL, 5);
    
    // Action buttons
    wxBoxSizer* actionSizer = new wxBoxSizer(wxHORIZONTAL);
    m_fetchCorpseBtn = new wxButton(this, ID_LOOT_FETCH_CORPSE, "Fetch from Corpse");
    m_lookupBtn = new wxButton(this, ID_LOOT_ITEM_LOOKUP, "Find Item");
    wxButton* paletteBtn = new wxButton(this, ID_LOOT_ITEM_PALETTE, "Item Palette");
    actionSizer->Add(m_fetchCorpseBtn, 0, wxRIGHT, 5);
    actionSizer->Add(m_lookupBtn, 0, wxRIGHT, 5);
    actionSizer->Add(paletteBtn, 0, 0, 0);
    itemSizer->Add(actionSizer, 0, wxALL, 5);
    
    mainSizer->Add(itemSizer, 0, wxEXPAND | wxALL, 5);
    
    // Basic properties section
    wxStaticBoxSizer* basicSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Basic Properties");
    
    wxFlexGridSizer* basicGridSizer = new wxFlexGridSizer(1, 2, 5, 5);
    basicGridSizer->AddGrowableCol(1);
    
    // Chance
    basicGridSizer->Add(new wxStaticText(this, wxID_ANY, "Chance (1-100000):"), 0, wxALIGN_CENTER_VERTICAL);
    m_chanceCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 100000);
    basicGridSizer->Add(m_chanceCtrl, 1, wxEXPAND);
    
    basicSizer->Add(basicGridSizer, 0, wxEXPAND | wxALL, 5);
    
    // Add help text for chance
    wxStaticText* chanceHelp = new wxStaticText(this, wxID_ANY, 
        "Chance scale: 1=0.001%, 1000=1%, 10000=10%, 100000=100%");
    chanceHelp->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL));
    basicSizer->Add(chanceHelp, 0, wxALL, 5);
    
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 5);
    
    // Optional properties section
    wxStaticBoxSizer* optionalSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Optional Properties");
    
    wxFlexGridSizer* optionalGridSizer = new wxFlexGridSizer(4, 2, 5, 5);
    optionalGridSizer->AddGrowableCol(1);
    
    // Count Max
    m_countMaxCheck = new wxCheckBox(this, ID_LOOT_COUNTMAX_CHECK, "Count Max:");
    m_countMaxCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000, 1);
    optionalGridSizer->Add(m_countMaxCheck, 0, wxALIGN_CENTER_VERTICAL);
    optionalGridSizer->Add(m_countMaxCtrl, 1, wxEXPAND);
    
    // Subtype
    m_subTypeCheck = new wxCheckBox(this, ID_LOOT_SUBTYPE_CHECK, "Subtype:");
    m_subTypeCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
    optionalGridSizer->Add(m_subTypeCheck, 0, wxALIGN_CENTER_VERTICAL);
    optionalGridSizer->Add(m_subTypeCombo, 1, wxEXPAND);
    
    // Action ID
    m_actionIdCheck = new wxCheckBox(this, ID_LOOT_ACTIONID_CHECK, "Action ID:");
    m_actionIdCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000000, 1);
    optionalGridSizer->Add(m_actionIdCheck, 0, wxALIGN_CENTER_VERTICAL);
    optionalGridSizer->Add(m_actionIdCtrl, 1, wxEXPAND);
    
    // Unique ID
    m_uniqueIdCheck = new wxCheckBox(this, ID_LOOT_UNIQUEID_CHECK, "Unique ID:");
    m_uniqueIdCtrl = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10000000, 1);
    optionalGridSizer->Add(m_uniqueIdCheck, 0, wxALIGN_CENTER_VERTICAL);
    optionalGridSizer->Add(m_uniqueIdCtrl, 1, wxEXPAND);
    
    optionalSizer->Add(optionalGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(optionalSizer, 0, wxEXPAND | wxALL, 5);
    
    // Corpse information section
    wxStaticBoxSizer* corpseSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Corpse Information");
    
    wxFlexGridSizer* corpseGridSizer = new wxFlexGridSizer(2, 2, 5, 5);
    corpseGridSizer->AddGrowableCol(1);
    
    corpseGridSizer->Add(new wxStaticText(this, wxID_ANY, "Corpse Size:"), 0, wxALIGN_CENTER_VERTICAL);
    m_corpseSizeText = new wxStaticText(this, wxID_ANY, "Not selected");
    corpseGridSizer->Add(m_corpseSizeText, 1, wxEXPAND);
    
    corpseGridSizer->Add(new wxStaticText(this, wxID_ANY, "Free Slots:"), 0, wxALIGN_CENTER_VERTICAL);
    m_corpseFreeSlots = new wxStaticText(this, wxID_ANY, "Unknown");
    corpseGridSizer->Add(m_corpseFreeSlots, 1, wxEXPAND);
    
    corpseSizer->Add(corpseGridSizer, 0, wxEXPAND | wxALL, 5);
    
    // Add help text about corpse size
    wxStaticText* corpseHelp = new wxStaticText(this, wxID_ANY, 
        "Note: Corpse can be ignored for loot generation due to low item chances");
    corpseHelp->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL));
    corpseSizer->Add(corpseHelp, 0, wxALL, 5);
    
    mainSizer->Add(corpseSizer, 0, wxEXPAND | wxALL, 5);
    
    // Preview section
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, "XML Preview");
    m_previewText = new wxStaticText(this, wxID_ANY, "Loot XML preview will appear here");
    m_previewText->SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    previewSizer->Add(m_previewText, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(previewSizer, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(new wxButton(this, ID_LOOT_OK, "OK"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, ID_LOOT_CANCEL, "Cancel"), 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Initialize UI state
    UpdateItemModeControls();
    UpdateCorpseInfo();
}

void MonsterLootDialog::PopulateSubTypes() {
    // Fluid types for containers
    m_subTypeCombo->Append("None");
    m_subTypeCombo->Append("Water");
    m_subTypeCombo->Append("Blood");
    m_subTypeCombo->Append("Beer");
    m_subTypeCombo->Append("Slime");
    m_subTypeCombo->Append("Lemonade");
    m_subTypeCombo->Append("Milk");
    m_subTypeCombo->Append("Mana");
    m_subTypeCombo->Append("Life");
    m_subTypeCombo->Append("Oil");
    m_subTypeCombo->Append("Wine");
    m_subTypeCombo->Append("Mud");
    m_subTypeCombo->Append("Fruit Juice");
    m_subTypeCombo->Append("Coconut Milk");
    m_subTypeCombo->Append("Tea");
    m_subTypeCombo->SetSelection(0);
}

void MonsterLootDialog::UpdateItemModeControls() {
    bool useItemId = m_useItemIdRadio->GetValue();
    
    m_itemIdCtrl->Enable(useItemId);
    m_itemNameCtrl->Enable(!useItemId);
    m_fetchCorpseBtn->Enable(!useItemId);
    m_lookupBtn->Enable(!useItemId);
}

void MonsterLootDialog::FetchCorpseItems() {
    // TODO: Integrate with map editor's corpse system
    // This should:
    // 1. Get selected corpse from map editor
    // 2. Read corpse size (slots available)
    // 3. Count current items in corpse
    // 4. Calculate free slots
    // 5. Update UI with corpse information
    
    // Placeholder implementation - update corpse info
    UpdateCorpseInfo();
    
    wxMessageBox("Corpse fetching will read items from the selected corpse on the map.\n"
                "It will show corpse size and available slots for loot generation.", 
                "Corpse Fetching", wxOK | wxICON_INFORMATION);
}

void MonsterLootDialog::LookupItemByName() {
    // Use existing Find Item window functionality
    OpenItemPalette();
}

void MonsterLootDialog::OpenItemPalette() {
    // Use the existing FindItemDialog to allow item selection
    FindItemDialog* dialog = new FindItemDialog(this, "Find Item for Loot", false);
    
    // Pre-populate search if we have an item name
    wxString itemName = m_itemNameCtrl->GetValue();
    if (!itemName.IsEmpty()) {
        dialog->setSearchMode(FindItemDialog::Names);
        // Note: The FindItemDialog would need a method to set the search text
        // For now, we'll open it empty and the user can search
    }
    
    if (dialog->ShowModal() == wxID_OK) {
        uint16_t selectedItemId = dialog->getResultID();
        
        if (selectedItemId > 0) {
            if (m_useItemIdRadio->GetValue()) {
                // Using item ID mode - set the ID directly
                m_itemIdCtrl->SetValue(selectedItemId);
            } else {
                // Using item name mode - we'd need to get the item name from the database
                // For now, switch to ID mode and set the ID
                m_useItemIdRadio->SetValue(true);
                m_useItemNameRadio->SetValue(false);
                m_itemIdCtrl->SetValue(selectedItemId);
                UpdateItemModeControls();
            }
            
            // Show success message
            wxMessageBox(wxString::Format("Selected item ID: %d", selectedItemId), 
                        "Item Selected", wxOK | wxICON_INFORMATION);
        }
    }
    
    dialog->Destroy();
}

void MonsterLootDialog::UpdateCorpseInfo() {
    // TODO: Get actual corpse information from map editor
    // For now, show placeholder data
    
    // Example corpse sizes for different corpse types:
    // Small corpse: 4 slots
    // Medium corpse: 8 slots  
    // Large corpse: 12-20 slots
    // Special corpses: varies
    
    m_corpseSizeText->SetLabel("20 slots (Large Corpse)");
    m_corpseFreeSlots->SetLabel("18 free (2 items present)");
    
    // Update layout after text change
    GetSizer()->Layout();
}

LootEntry MonsterLootDialog::GetLootEntry() const {
    LootEntry loot;
    
    loot.useItemName = m_useItemNameRadio->GetValue();
    if (loot.useItemName) {
        loot.itemName = nstr(m_itemNameCtrl->GetValue());
        loot.itemId = 0; // Will be resolved later
    } else {
        loot.itemId = m_itemIdCtrl->GetValue();
        loot.itemName = "";
    }
    
    loot.chance = m_chanceCtrl->GetValue();
    loot.countMax = m_countMaxCheck->GetValue() ? m_countMaxCtrl->GetValue() : 1;
    loot.subType = m_subTypeCheck->GetValue() ? m_subTypeCombo->GetSelection() : 0;
    loot.actionId = m_actionIdCheck->GetValue() ? m_actionIdCtrl->GetValue() : 0;
    loot.uniqueId = m_uniqueIdCheck->GetValue() ? m_uniqueIdCtrl->GetValue() : 0;
    
    return loot;
}

void MonsterLootDialog::SetLootEntry(const LootEntry& loot) {
    if (loot.useItemName) {
        m_useItemNameRadio->SetValue(true);
        m_itemNameCtrl->SetValue(wxstr(loot.itemName));
    } else {
        m_useItemIdRadio->SetValue(true);
        m_itemIdCtrl->SetValue(loot.itemId);
    }
    
    m_chanceCtrl->SetValue(loot.chance);
    
    m_countMaxCheck->SetValue(loot.countMax > 1);
    m_countMaxCtrl->SetValue(loot.countMax);
    m_countMaxCtrl->Enable(loot.countMax > 1);
    
    m_subTypeCheck->SetValue(loot.subType > 0);
    m_subTypeCombo->SetSelection(loot.subType);
    m_subTypeCombo->Enable(loot.subType > 0);
    
    m_actionIdCheck->SetValue(loot.actionId > 0);
    m_actionIdCtrl->SetValue(loot.actionId);
    m_actionIdCtrl->Enable(loot.actionId > 0);
    
    m_uniqueIdCheck->SetValue(loot.uniqueId > 0);
    m_uniqueIdCtrl->SetValue(loot.uniqueId);
    m_uniqueIdCtrl->Enable(loot.uniqueId > 0);
    
    UpdateItemModeControls();
}

// Event handlers
void MonsterLootDialog::OnItemModeChanged(wxCommandEvent& event) {
    UpdateItemModeControls();
}

void MonsterLootDialog::OnFetchCorpse(wxCommandEvent& event) {
    FetchCorpseItems();
}

void MonsterLootDialog::OnItemLookup(wxCommandEvent& event) {
    LookupItemByName();
}

void MonsterLootDialog::OnCountMaxToggle(wxCommandEvent& event) {
    m_countMaxCtrl->Enable(m_countMaxCheck->GetValue());
}

void MonsterLootDialog::OnSubTypeToggle(wxCommandEvent& event) {
    m_subTypeCombo->Enable(m_subTypeCheck->GetValue());
}

void MonsterLootDialog::OnActionIdToggle(wxCommandEvent& event) {
    m_actionIdCtrl->Enable(m_actionIdCheck->GetValue());
}

void MonsterLootDialog::OnUniqueIdToggle(wxCommandEvent& event) {
    m_uniqueIdCtrl->Enable(m_uniqueIdCheck->GetValue());
}

void MonsterLootDialog::OnOK(wxCommandEvent& event) {
    // Validate input
    if (m_useItemNameRadio->GetValue() && m_itemNameCtrl->GetValue().IsEmpty()) {
        wxMessageBox("Item name cannot be empty!", "Validation Error", wxOK | wxICON_ERROR);
        return;
    }
    
    if (m_useItemIdRadio->GetValue() && m_itemIdCtrl->GetValue() <= 0) {
        wxMessageBox("Item ID must be greater than 0!", "Validation Error", wxOK | wxICON_ERROR);
        return;
    }
    
    EndModal(wxID_OK);
}

void MonsterLootDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
} 