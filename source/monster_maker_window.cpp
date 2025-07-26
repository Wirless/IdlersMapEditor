#include "main.h"
#include "monster_maker_window.h"
#include "monster_attack_dialog.h"
#include "monster_defense_dialog.h"
#include "monster_loot_dialog.h"
#include "gui.h"
#include "creature_sprite_manager.h"

#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/menu.h>

// ContextMenuListCtrl implementation
wxBEGIN_EVENT_TABLE(ContextMenuListCtrl, wxListCtrl)
    EVT_CONTEXT_MENU(ContextMenuListCtrl::OnContextMenu)
    EVT_MENU(ID_CONTEXT_ADD, ContextMenuListCtrl::OnMenuAdd)
    EVT_MENU(ID_CONTEXT_EDIT, ContextMenuListCtrl::OnMenuEdit)
    EVT_MENU(ID_CONTEXT_DELETE, ContextMenuListCtrl::OnMenuDelete)
wxEND_EVENT_TABLE()

ContextMenuListCtrl::ContextMenuListCtrl(wxWindow* parent, wxWindowID id, MonsterMakerWindow* owner, const wxString& listType)
    : wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL), 
      m_owner(owner), m_listType(listType) {
    
    // Create context menu
    m_contextMenu = new wxMenu();
    m_contextMenu->Append(ID_CONTEXT_ADD, wxString::Format("Add %s", m_listType));
    m_contextMenu->Append(ID_CONTEXT_EDIT, wxString::Format("Edit %s", m_listType));
    m_contextMenu->Append(ID_CONTEXT_DELETE, wxString::Format("Delete %s", m_listType));
}

void ContextMenuListCtrl::OnContextMenu(wxContextMenuEvent& event) {
    if (!m_contextMenu) return;
    
    // Check if we have a selection to enable/disable Edit and Delete
    long selectedIndex = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    bool hasSelection = (selectedIndex != -1);
    
    m_contextMenu->Enable(ID_CONTEXT_EDIT, hasSelection);
    m_contextMenu->Enable(ID_CONTEXT_DELETE, hasSelection);
    
    // Get mouse position
    wxPoint position = event.GetPosition();
    if (position == wxPoint(-1, -1)) {
        // Keyboard event - use first selected item position
        if (hasSelection) {
            wxRect rect;
            GetItemRect(selectedIndex, rect);
            position = ClientToScreen(rect.GetPosition());
        } else {
            position = ClientToScreen(wxPoint(0, 0));
        }
    }
    
    // Show context menu
    PopupMenu(m_contextMenu, ScreenToClient(position));
}

void ContextMenuListCtrl::OnMenuAdd(wxCommandEvent& event) {
    if (m_listType == "Attack") {
        m_owner->AddAttack();
    } else if (m_listType == "Loot") {
        m_owner->AddLoot();
    } else if (m_listType == "Defense") {
        m_owner->AddDefense();
    } else if (m_listType == "Summon") {
        m_owner->AddSummon();
    } else if (m_listType == "Voice") {
        m_owner->AddVoice();
    }
}

void ContextMenuListCtrl::OnMenuEdit(wxCommandEvent& event) {
    long selectedIndex = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedIndex != -1) {
        if (m_listType == "Attack") {
            m_owner->EditAttack(selectedIndex);
        } else if (m_listType == "Loot") {
            m_owner->EditLoot(selectedIndex);
        } else if (m_listType == "Defense") {
            m_owner->EditDefense(selectedIndex);
        } else if (m_listType == "Summon") {
            m_owner->EditSummon(selectedIndex);
        } else if (m_listType == "Voice") {
            m_owner->EditVoice(selectedIndex);
        }
    }
}

void ContextMenuListCtrl::OnMenuDelete(wxCommandEvent& event) {
    long selectedIndex = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedIndex != -1) {
        if (wxMessageBox(wxString::Format("Delete selected %s?", m_listType), "Confirm Delete", 
                        wxYES_NO | wxICON_QUESTION) == wxYES) {
            if (m_listType == "Attack") {
                m_owner->DeleteAttack(selectedIndex);
            } else if (m_listType == "Loot") {
                m_owner->DeleteLoot(selectedIndex);
            } else if (m_listType == "Defense") {
                m_owner->DeleteDefense(selectedIndex);
            } else if (m_listType == "Summon") {
                m_owner->DeleteSummon(selectedIndex);
            } else if (m_listType == "Voice") {
                m_owner->DeleteVoice(selectedIndex);
            }
        }
    }
}

// MonsterPreviewPanel implementation
wxBEGIN_EVENT_TABLE(MonsterPreviewPanel, wxPanel)
    EVT_PAINT(MonsterPreviewPanel::OnPaint)
wxEND_EVENT_TABLE()

MonsterPreviewPanel::MonsterPreviewPanel(wxWindow* parent, MonsterMakerWindow* owner)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(80, 80)), m_owner(owner) {
    SetBackgroundColour(*wxLIGHT_GREY);
}

void MonsterPreviewPanel::OnPaint(wxPaintEvent& event) {
    if (m_owner) {
        m_owner->UpdatePreview();
    }
    event.Skip();
}

wxBEGIN_EVENT_TABLE(MonsterMakerWindow, wxFrame)
    EVT_CLOSE(MonsterMakerWindow::OnClose)
    EVT_BUTTON(ID_CLOSE_BUTTON, MonsterMakerWindow::OnCloseButton)
    EVT_BUTTON(ID_CREATE_MONSTER, MonsterMakerWindow::OnCreateMonster)
    EVT_BUTTON(ID_LOAD_MONSTER, MonsterMakerWindow::OnLoadMonster)
    EVT_BUTTON(ID_ADD_SUMMON, MonsterMakerWindow::OnAddSummon)
    EVT_BUTTON(ID_EDIT_SUMMON, MonsterMakerWindow::OnEditSummon)
    EVT_BUTTON(ID_DELETE_SUMMON, MonsterMakerWindow::OnDeleteSummon)
    EVT_BUTTON(ID_ADD_VOICE, MonsterMakerWindow::OnAddVoice)
    EVT_BUTTON(ID_EDIT_VOICE, MonsterMakerWindow::OnEditVoice)
    EVT_BUTTON(ID_DELETE_VOICE, MonsterMakerWindow::OnDeleteVoice)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, MonsterMakerWindow::OnTabChange)
    EVT_TIMER(ID_PREVIEW_TIMER, MonsterMakerWindow::OnPreviewTimer)
    EVT_COMBOBOX(ID_SKULL_COMBO, MonsterMakerWindow::OnSkullChange)
    EVT_COMMAND_SCROLL(ID_STRATEGY_SLIDER, MonsterMakerWindow::OnStrategyChange)
    EVT_COMMAND(ID_LIGHT_COLOR_SPIN, wxEVT_COMMAND_SPINCTRL_UPDATED, MonsterMakerWindow::OnLightColorChange)
    EVT_COMMAND(ID_HEAD_PALETTE, wxEVT_COLOR_PALETTE_SELECTION, MonsterMakerWindow::OnColorPaletteChange)
    EVT_COMMAND(ID_BODY_PALETTE, wxEVT_COLOR_PALETTE_SELECTION, MonsterMakerWindow::OnColorPaletteChange)
    EVT_COMMAND(ID_LEGS_PALETTE, wxEVT_COLOR_PALETTE_SELECTION, MonsterMakerWindow::OnColorPaletteChange)
    EVT_COMMAND(ID_FEET_PALETTE, wxEVT_COLOR_PALETTE_SELECTION, MonsterMakerWindow::OnColorPaletteChange)
    EVT_CHECKBOX(ID_SHOW_NUMBERS_TOGGLE, MonsterMakerWindow::OnShowNumbersToggle)
wxEND_EVENT_TABLE()

MonsterMakerWindow::MonsterMakerWindow(wxWindow* parent) :
    wxFrame(parent, wxID_ANY, "Monster Maker", wxDefaultPosition, wxSize(650, 600), 
            wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT),
    m_previewTimer(nullptr), m_previewUpdatePending(false)
{
    SetIcon(wxNullIcon);
    
    // Initialize timer for live preview updates (150ms delay)
    m_previewTimer = new wxTimer(this, ID_PREVIEW_TIMER);
    
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create notebook for tabs
    m_notebook = new wxNotebook(this, ID_NOTEBOOK);
    
    // Create tabs
    CreateMonsterTab(m_notebook);
    CreateFlagsTab(m_notebook);
    CreateAttacksTab(m_notebook);
    CreateDefensesTab(m_notebook);
    CreateElementsTab(m_notebook);
    CreateImmunitiesTab(m_notebook);
    CreateSummonsTab(m_notebook);
    CreateVoicesTab(m_notebook);
    CreateLootTab(m_notebook);
    CreateIOTab(m_notebook);
    CreateXMLPreviewTab(m_notebook);
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    
    // Create buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_createButton = new wxButton(this, ID_CREATE_MONSTER, "Create Monster");
    m_saveButton = new wxButton(this, ID_SAVE_MONSTER, "Save to File");
    wxButton* closeButton = new wxButton(this, ID_CLOSE_BUTTON, "Close");
    
    buttonSizer->Add(m_createButton, 0, wxRIGHT, 5);
    buttonSizer->Add(m_saveButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeButton, 0, 0, 0);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Bind spin control events for live preview updates
    if (m_lookType) {
        m_lookType->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnLookTypeChange, this);
        m_lookType->Bind(wxEVT_TEXT, &MonsterMakerWindow::OnLookTypeChange, this); // Real-time as you type
    }
    
    // Bind events for XML preview updates
    if (m_name) m_name->Bind(wxEVT_TEXT, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_nameDescription) m_nameDescription->Bind(wxEVT_TEXT, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_race) m_race->Bind(wxEVT_TEXT, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_experience) m_experience->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_speed) m_speed->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_manacost) m_manacost->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_healthMax) m_healthMax->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    
    // Bind flag checkboxes
    if (m_summonable) m_summonable->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_attackable) m_attackable->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_hostile) m_hostile->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_illusionable) m_illusionable->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_convinceable) m_convinceable->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_pushable) m_pushable->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_canpushitems) m_canpushitems->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_canpushcreatures) m_canpushcreatures->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_hidehealth) m_hidehealth->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    
    // Bind element controls
    if (m_physicalPercent) m_physicalPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_firePercent) m_firePercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_energyPercent) m_energyPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_earthPercent) m_earthPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_icePercent) m_icePercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_holyPercent) m_holyPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_deathPercent) m_deathPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_drownPercent) m_drownPercent->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    
    // Bind immunity controls
    if (m_immunityFire) m_immunityFire->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityEnergy) m_immunityEnergy->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityEarth) m_immunityEarth->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityIce) m_immunityIce->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityHoly) m_immunityHoly->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityDeath) m_immunityDeath->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityPhysical) m_immunityPhysical->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityDrown) m_immunityDrown->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityParalyze) m_immunityParalyze->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityInvisible) m_immunityInvisible->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityLifedrain) m_immunityLifedrain->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_immunityDrunk) m_immunityDrunk->Bind(wxEVT_CHECKBOX, &MonsterMakerWindow::OnPreviewUpdate, this);
    
    // Bind summon and voice controls
    if (m_maxSummons) m_maxSummons->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_voiceInterval) m_voiceInterval->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    if (m_voiceChance) m_voiceChance->Bind(wxEVT_SPINCTRL, &MonsterMakerWindow::OnPreviewUpdate, this);
    
    // Set default values
    ClearAll();
    
    // Trigger initial preview update after a short delay
    if (m_previewTimer) {
        m_previewUpdatePending = true;
        m_previewTimer->Start(300, wxTIMER_ONE_SHOT);
    }
}

MonsterMakerWindow::~MonsterMakerWindow() {
    if (m_previewTimer) {
        m_previewTimer->Stop();
        delete m_previewTimer;
        m_previewTimer = nullptr;
    }
}

void MonsterMakerWindow::CreateMonsterTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Monster");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Basic info section
    wxStaticBoxSizer* basicSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Basic Information");
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(4, 2, 5, 5);
    gridSizer->AddGrowableCol(1);
    
    // Name
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
    m_name = new wxTextCtrl(panel, wxID_ANY);
    gridSizer->Add(m_name, 1, wxEXPAND);
    
    // Name Description
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Description:"), 0, wxALIGN_CENTER_VERTICAL);
    m_nameDescription = new wxTextCtrl(panel, wxID_ANY);
    gridSizer->Add(m_nameDescription, 1, wxEXPAND);
    
    // Race
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Race:"), 0, wxALIGN_CENTER_VERTICAL);
    m_race = new wxComboBox(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN);
    m_race->Append("blood");
    m_race->Append("venom");
    m_race->Append("undead");
    m_race->Append("fire");
    m_race->Append("energy");
    gridSizer->Add(m_race, 1, wxEXPAND);
    
    // Skull
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Skull:"), 0, wxALIGN_CENTER_VERTICAL);
    m_skull = new wxComboBox(panel, ID_SKULL_COMBO, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
    m_skull->Append("none");
    m_skull->Append("yellow");
    m_skull->Append("black");
    m_skull->Append("red");
    m_skull->Append("white");
    m_skull->Append("orange");
    m_skull->Append("green");
    m_skull->SetSelection(0);
    gridSizer->Add(m_skull, 1, wxEXPAND);
    
    // Experience
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Experience:"), 0, wxALIGN_CENTER_VERTICAL);
    m_experience = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999, 1000);
    gridSizer->Add(m_experience, 1, wxEXPAND);
    
    basicSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(basicSizer, 0, wxEXPAND | wxALL, 5);
    
    // Health and Speed section
    wxStaticBoxSizer* statsSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Stats");
    
    wxFlexGridSizer* statGridSizer = new wxFlexGridSizer(3, 2, 5, 5);
    statGridSizer->AddGrowableCol(1);
    
    // Speed
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Speed:"), 0, wxALIGN_CENTER_VERTICAL);
    m_speed = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, 100);
    statGridSizer->Add(m_speed, 1, wxEXPAND);
    
    // Health Max
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Health Max:"), 0, wxALIGN_CENTER_VERTICAL);
    m_healthMax = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999, 500);
    statGridSizer->Add(m_healthMax, 1, wxEXPAND);
    
    // Mana Cost
    statGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Mana Cost:"), 0, wxALIGN_CENTER_VERTICAL);
    m_manacost = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 9999, 0);
    statGridSizer->Add(m_manacost, 1, wxEXPAND);
    
    statsSizer->Add(statGridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(statsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Look section
    wxStaticBoxSizer* lookSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Appearance");
    
    // Look Type
    wxBoxSizer* lookTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    lookTypeSizer->Add(new wxStaticText(panel, wxID_ANY, "Look Type:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_lookType = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999, 1);
    lookTypeSizer->Add(m_lookType, 0, wxEXPAND);
    lookSizer->Add(lookTypeSizer, 0, wxEXPAND | wxALL, 5);
    
    // Color palettes in a notebook for better organization
    wxNotebook* colorNotebook = new wxNotebook(panel, wxID_ANY);
    
    // Add checkbox to control number visibility
    wxCheckBox* showNumbersCheck = new wxCheckBox(panel, ID_SHOW_NUMBERS_TOGGLE, "Show color numbers");
    showNumbersCheck->SetValue(true);
    lookSizer->Add(showNumbersCheck, 0, wxALL, 5);
    
    // Head Color Tab
    wxPanel* headPanel = new wxPanel(colorNotebook);
    wxBoxSizer* headSizer = new wxBoxSizer(wxVERTICAL);
    headSizer->Add(new wxStaticText(headPanel, wxID_ANY, "Head Color (click to select):"), 0, wxALL, 2);
    m_headPalette = new ColorPaletteCtrl(headPanel, ID_HEAD_PALETTE, wxDefaultPosition, wxSize(320, 120));
    headSizer->Add(m_headPalette, 0, wxALL, 2);
    headPanel->SetSizer(headSizer);
    colorNotebook->AddPage(headPanel, "Head");
    
    // Body Color Tab
    wxPanel* bodyPanel = new wxPanel(colorNotebook);
    wxBoxSizer* bodySizer = new wxBoxSizer(wxVERTICAL);
    bodySizer->Add(new wxStaticText(bodyPanel, wxID_ANY, "Body Color (click to select):"), 0, wxALL, 2);
    m_bodyPalette = new ColorPaletteCtrl(bodyPanel, ID_BODY_PALETTE, wxDefaultPosition, wxSize(320, 120));
    bodySizer->Add(m_bodyPalette, 0, wxALL, 2);
    bodyPanel->SetSizer(bodySizer);
    colorNotebook->AddPage(bodyPanel, "Body");
    
    // Legs Color Tab
    wxPanel* legsPanel = new wxPanel(colorNotebook);
    wxBoxSizer* legsSizer = new wxBoxSizer(wxVERTICAL);
    legsSizer->Add(new wxStaticText(legsPanel, wxID_ANY, "Legs Color (click to select):"), 0, wxALL, 2);
    m_legsPalette = new ColorPaletteCtrl(legsPanel, ID_LEGS_PALETTE, wxDefaultPosition, wxSize(320, 120));
    legsSizer->Add(m_legsPalette, 0, wxALL, 2);
    legsPanel->SetSizer(legsSizer);
    colorNotebook->AddPage(legsPanel, "Legs");
    
    // Feet Color Tab
    wxPanel* feetPanel = new wxPanel(colorNotebook);
    wxBoxSizer* feetSizer = new wxBoxSizer(wxVERTICAL);
    feetSizer->Add(new wxStaticText(feetPanel, wxID_ANY, "Feet Color (click to select):"), 0, wxALL, 2);
    m_feetPalette = new ColorPaletteCtrl(feetPanel, ID_FEET_PALETTE, wxDefaultPosition, wxSize(320, 120));
    feetSizer->Add(m_feetPalette, 0, wxALL, 2);
    feetPanel->SetSizer(feetSizer);
    colorNotebook->AddPage(feetPanel, "Feet");
    
    lookSizer->Add(colorNotebook, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(lookSizer, 1, wxEXPAND | wxALL, 5);
    
    // Preview section
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Preview");
    m_previewPanel = new MonsterPreviewPanel(panel, this);
    
    previewSizer->Add(m_previewPanel, 0, wxALIGN_CENTER | wxALL, 5);
    mainSizer->Add(previewSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::CreateFlagsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Flags");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Behavior flags
    wxStaticBoxSizer* behaviorSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Behavior");
    
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(3, 3, 5, 5);
    
    m_summonable = new wxCheckBox(panel, wxID_ANY, "Summonable");
    m_attackable = new wxCheckBox(panel, wxID_ANY, "Attackable");
    m_hostile = new wxCheckBox(panel, wxID_ANY, "Hostile");
    m_illusionable = new wxCheckBox(panel, wxID_ANY, "Illusionable");
    m_convinceable = new wxCheckBox(panel, wxID_ANY, "Convinceable");
    m_pushable = new wxCheckBox(panel, wxID_ANY, "Pushable");
    m_canpushitems = new wxCheckBox(panel, wxID_ANY, "Can Push Items");
    m_canpushcreatures = new wxCheckBox(panel, wxID_ANY, "Can Push Creatures");
    m_hidehealth = new wxCheckBox(panel, wxID_ANY, "Hide Health");
    
    gridSizer->Add(m_summonable, 0, 0, 0);
    gridSizer->Add(m_attackable, 0, 0, 0);
    gridSizer->Add(m_hostile, 0, 0, 0);
    gridSizer->Add(m_illusionable, 0, 0, 0);
    gridSizer->Add(m_convinceable, 0, 0, 0);
    gridSizer->Add(m_pushable, 0, 0, 0);
    gridSizer->Add(m_canpushitems, 0, 0, 0);
    gridSizer->Add(m_canpushcreatures, 0, 0, 0);
    gridSizer->Add(m_hidehealth, 0, 0, 0);
    
    behaviorSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(behaviorSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::CreateAttacksTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Attacks");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Right-click to add, edit, or delete attacks"), 0, wxALL, 5);
    
    m_attacksList = new ContextMenuListCtrl(panel, ID_ATTACKS_LIST, this, "Attack");
    m_attacksList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 100);
    m_attacksList->AppendColumn("Damage", wxLIST_FORMAT_LEFT, 80);
    m_attacksList->AppendColumn("Interval", wxLIST_FORMAT_LEFT, 60);
    m_attacksList->AppendColumn("Chance %", wxLIST_FORMAT_LEFT, 60);
    m_attacksList->AppendColumn("Range", wxLIST_FORMAT_LEFT, 50);
    m_attacksList->AppendColumn("Radius", wxLIST_FORMAT_LEFT, 50);
    m_attacksList->AppendColumn("Properties", wxLIST_FORMAT_LEFT, 150);
    
    sizer->Add(m_attacksList, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateDefensesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Defenses");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Right-click to add, edit, or delete defenses"), 0, wxALL, 5);
    
    m_defensesList = new ContextMenuListCtrl(panel, ID_DEFENSES_LIST, this, "Defense");
    m_defensesList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 80);
    m_defensesList->AppendColumn("Type", wxLIST_FORMAT_LEFT, 80);
    m_defensesList->AppendColumn("Interval", wxLIST_FORMAT_LEFT, 60);
    m_defensesList->AppendColumn("Chance %", wxLIST_FORMAT_LEFT, 60);
    m_defensesList->AppendColumn("Value", wxLIST_FORMAT_LEFT, 80);
    m_defensesList->AppendColumn("Properties", wxLIST_FORMAT_LEFT, 150);
    
    sizer->Add(m_defensesList, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateElementsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Elements");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Element Damage Modifiers (% - negative values = weakness, positive = resistance)"), 0, wxALL, 5);
    
    // Create grid for element controls
    wxFlexGridSizer* elementGridSizer = new wxFlexGridSizer(4, 4, 10, 10);
    elementGridSizer->AddGrowableCol(1);
    elementGridSizer->AddGrowableCol(3);
    
    // Physical
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Physical:"), 0, wxALIGN_CENTER_VERTICAL);
    m_physicalPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_physicalPercent, 1, wxEXPAND);
    
    // Fire  
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Fire:"), 0, wxALIGN_CENTER_VERTICAL);
    m_firePercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_firePercent, 1, wxEXPAND);
    
    // Energy
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Energy:"), 0, wxALIGN_CENTER_VERTICAL);
    m_energyPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_energyPercent, 1, wxEXPAND);
    
    // Earth
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Earth:"), 0, wxALIGN_CENTER_VERTICAL);
    m_earthPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_earthPercent, 1, wxEXPAND);
    
    // Ice
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Ice:"), 0, wxALIGN_CENTER_VERTICAL);
    m_icePercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_icePercent, 1, wxEXPAND);
    
    // Holy
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Holy:"), 0, wxALIGN_CENTER_VERTICAL);
    m_holyPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_holyPercent, 1, wxEXPAND);
    
    // Death
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Death:"), 0, wxALIGN_CENTER_VERTICAL);
    m_deathPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_deathPercent, 1, wxEXPAND);
    
    // Drown
    elementGridSizer->Add(new wxStaticText(panel, wxID_ANY, "Drown:"), 0, wxALIGN_CENTER_VERTICAL);
    m_drownPercent = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100, 100, 0);
    elementGridSizer->Add(m_drownPercent, 1, wxEXPAND);
    
    sizer->Add(elementGridSizer, 1, wxEXPAND | wxALL, 10);
    
    // Help text
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Examples: -25% = 25% more damage, +50% = 50% damage reduction"), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateImmunitiesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Immunities");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Monster Immunities - Check to make the monster immune to specific effects"), 0, wxALL, 5);
    
    // Create grid for immunity checkboxes
    wxFlexGridSizer* immunityGridSizer = new wxFlexGridSizer(3, 3, 10, 10);
    
    // Damage type immunities
    m_immunityFire = new wxCheckBox(panel, wxID_ANY, "Fire Damage");
    immunityGridSizer->Add(m_immunityFire, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityEnergy = new wxCheckBox(panel, wxID_ANY, "Energy Damage");
    immunityGridSizer->Add(m_immunityEnergy, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityEarth = new wxCheckBox(panel, wxID_ANY, "Earth/Poison Damage");
    immunityGridSizer->Add(m_immunityEarth, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityIce = new wxCheckBox(panel, wxID_ANY, "Ice Damage");
    immunityGridSizer->Add(m_immunityIce, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityHoly = new wxCheckBox(panel, wxID_ANY, "Holy Damage");
    immunityGridSizer->Add(m_immunityHoly, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityDeath = new wxCheckBox(panel, wxID_ANY, "Death Damage");
    immunityGridSizer->Add(m_immunityDeath, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityPhysical = new wxCheckBox(panel, wxID_ANY, "Physical Damage");
    immunityGridSizer->Add(m_immunityPhysical, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityDrown = new wxCheckBox(panel, wxID_ANY, "Drowning");
    immunityGridSizer->Add(m_immunityDrown, 0, wxALIGN_CENTER_VERTICAL);
    
    // Status effect immunities
    m_immunityParalyze = new wxCheckBox(panel, wxID_ANY, "Paralyze");
    immunityGridSizer->Add(m_immunityParalyze, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityInvisible = new wxCheckBox(panel, wxID_ANY, "Invisibility");
    immunityGridSizer->Add(m_immunityInvisible, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityLifedrain = new wxCheckBox(panel, wxID_ANY, "Life Drain");
    immunityGridSizer->Add(m_immunityLifedrain, 0, wxALIGN_CENTER_VERTICAL);
    
    m_immunityDrunk = new wxCheckBox(panel, wxID_ANY, "Drunk");
    immunityGridSizer->Add(m_immunityDrunk, 0, wxALIGN_CENTER_VERTICAL);
    
    sizer->Add(immunityGridSizer, 1, wxEXPAND | wxALL, 10);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateSummonsTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Summons");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Max summons control
    wxBoxSizer* maxSummonsSizer = new wxBoxSizer(wxHORIZONTAL);
    maxSummonsSizer->Add(new wxStaticText(panel, wxID_ANY, "Max Summons:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_maxSummons = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 20, 0);
    maxSummonsSizer->Add(m_maxSummons, 0, wxRIGHT, 10);
    sizer->Add(maxSummonsSizer, 0, wxALL, 5);
    
    // Summons list
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Summons List:"), 0, wxALL, 5);
    m_summonsList = new ContextMenuListCtrl(panel, ID_SUMMONS_LIST, this, "Summon");
    m_summonsList->AppendColumn("Creature Name", wxLIST_FORMAT_LEFT, 200);
    m_summonsList->AppendColumn("Interval (ms)", wxLIST_FORMAT_LEFT, 100);
    m_summonsList->AppendColumn("Chance (%)", wxLIST_FORMAT_LEFT, 100);
    sizer->Add(m_summonsList, 1, wxEXPAND | wxALL, 5);
    
    // Add/Edit/Delete buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(panel, ID_ADD_SUMMON, "Add Summon"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(panel, ID_EDIT_SUMMON, "Edit Summon"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(panel, ID_DELETE_SUMMON, "Delete Summon"), 0);
    sizer->Add(buttonSizer, 0, wxALL, 5);
    
    // Instructions
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Right-click on list for context menu. Double-click to edit."), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateVoicesTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Voices");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Voice settings controls
    wxFlexGridSizer* voiceSettingsSizer = new wxFlexGridSizer(2, 2, 5, 10);
    voiceSettingsSizer->AddGrowableCol(1);
    
    voiceSettingsSizer->Add(new wxStaticText(panel, wxID_ANY, "Voice Interval (ms):"), 0, wxALIGN_CENTER_VERTICAL);
    m_voiceInterval = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1000, 60000, 5000);
    voiceSettingsSizer->Add(m_voiceInterval, 1, wxEXPAND);
    
    voiceSettingsSizer->Add(new wxStaticText(panel, wxID_ANY, "Voice Chance (%):"), 0, wxALIGN_CENTER_VERTICAL);
    m_voiceChance = new wxSpinCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 10);
    voiceSettingsSizer->Add(m_voiceChance, 1, wxEXPAND);
    
    sizer->Add(voiceSettingsSizer, 0, wxEXPAND | wxALL, 5);
    
    // Voices list
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Voice Messages:"), 0, wxALL, 5);
    m_voicesList = new ContextMenuListCtrl(panel, ID_VOICES_LIST, this, "Voice");
    m_voicesList->AppendColumn("Message", wxLIST_FORMAT_LEFT, 400);
    m_voicesList->AppendColumn("Yell", wxLIST_FORMAT_LEFT, 60);
    sizer->Add(m_voicesList, 1, wxEXPAND | wxALL, 5);
    
    // Add/Edit/Delete buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(panel, ID_ADD_VOICE, "Add Voice"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(panel, ID_EDIT_VOICE, "Edit Voice"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(panel, ID_DELETE_VOICE, "Delete Voice"), 0);
    sizer->Add(buttonSizer, 0, wxALL, 5);
    
    // Instructions
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Right-click on list for context menu. Double-click to edit."), 0, wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateLootTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Loot");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Right-click to add, edit, or delete loot items"), 0, wxALL, 5);
    
    m_lootList = new ContextMenuListCtrl(panel, ID_LOOT_LIST, this, "Loot");
    m_lootList->AppendColumn("Item", wxLIST_FORMAT_LEFT, 120);
    m_lootList->AppendColumn("ID/Name", wxLIST_FORMAT_LEFT, 100);
    m_lootList->AppendColumn("Chance %", wxLIST_FORMAT_LEFT, 80);
    m_lootList->AppendColumn("Count Max", wxLIST_FORMAT_LEFT, 80);
    m_lootList->AppendColumn("Properties", wxLIST_FORMAT_LEFT, 150);
    
    sizer->Add(m_lootList, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::CreateIOTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "Load/Save");
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Monsters list
    wxStaticBoxSizer* listSizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Available Monsters");
    
    m_monstersList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
    m_monstersList->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
    m_monstersList->AppendColumn("File", wxLIST_FORMAT_LEFT, 250);
    
    // Populate with loaded monsters
    if (g_gui.monster_manager.isLoaded()) {
        const auto& monsters = g_gui.monster_manager.getAllMonsters();
        int index = 0;
        for (const auto& entry : monsters) {
            long itemIndex = m_monstersList->InsertItem(index, wxstr(entry.second.name));
            m_monstersList->SetItem(itemIndex, 1, wxstr(entry.second.filename));
            index++;
        }
    }
    
    listSizer->Add(m_monstersList, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(listSizer, 1, wxEXPAND | wxALL, 5);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_loadButton = new wxButton(panel, ID_LOAD_MONSTER, "Load Selected");
    buttonSizer->Add(m_loadButton, 0, wxRIGHT, 5);
    
    buttonSizer->Add(new wxButton(panel, wxID_ANY, "Refresh List"), 0, wxRIGHT, 5);
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(mainSizer);
}

void MonsterMakerWindow::RefreshMonsterList() {
    if (m_monstersList && g_gui.monster_manager.isLoaded()) {
        m_monstersList->DeleteAllItems();
        const auto& monsters = g_gui.monster_manager.getAllMonsters();
        int index = 0;
        for (const auto& entry : monsters) {
            long itemIndex = m_monstersList->InsertItem(index, wxstr(entry.second.name));
            m_monstersList->SetItem(itemIndex, 1, wxstr(entry.second.filename));
            index++;
        }
    }
}

void MonsterMakerWindow::LoadMonster(const MonsterEntry& monster) {
    m_name->SetValue(wxstr(monster.name));
    m_race->SetValue(wxstr(monster.race));
    m_experience->SetValue(monster.experience);
    m_speed->SetValue(monster.speed);
    m_manacost->SetValue(monster.manacost);
    m_healthMax->SetValue(monster.health_max);
    m_lookType->SetValue(monster.look_type);
    m_headPalette->SetSelectedColor(monster.look_head);
    m_bodyPalette->SetSelectedColor(monster.look_body);
    m_legsPalette->SetSelectedColor(monster.look_legs);
    m_feetPalette->SetSelectedColor(monster.look_feet);
    
    UpdatePreview();
}

void MonsterMakerWindow::ClearAll() {
    m_name->SetValue("");
    m_nameDescription->SetValue("");
    m_race->SetValue("blood");
    m_experience->SetValue(1000);
    m_speed->SetValue(100);
    m_manacost->SetValue(0);
    m_healthMax->SetValue(500);
    m_lookType->SetValue(1);
    m_headPalette->SetSelectedColor(0);
    m_bodyPalette->SetSelectedColor(0);
    m_legsPalette->SetSelectedColor(0);
    m_feetPalette->SetSelectedColor(0);
    
    // Clear flags
    m_summonable->SetValue(false);
    m_attackable->SetValue(true);
    m_hostile->SetValue(true);
    m_illusionable->SetValue(false);
    m_convinceable->SetValue(false);
    m_pushable->SetValue(false);
    m_canpushitems->SetValue(false);
    m_canpushcreatures->SetValue(false);
    m_hidehealth->SetValue(false);
    
    UpdatePreview();
}

void MonsterMakerWindow::UpdatePreview() {
    if (!m_previewPanel) return;
    
    // Get current look values
    int lookType = m_lookType->GetValue();
    int headColor = m_headPalette->GetSelectedColor();
    int bodyColor = m_bodyPalette->GetSelectedColor();
    int legsColor = m_legsPalette->GetSelectedColor();
    int feetColor = m_feetPalette->GetSelectedColor();
    
    // Validate look type and use fallback if needed
    if (lookType <= 0) {
        lookType = 5; // Fallback to look type 5
        if (m_lookType->GetValue() != lookType) {
            m_lookType->SetValue(lookType);
        }
    }
    
    // Create a client device context for drawing
    wxClientDC dc(m_previewPanel);
    
    // Clear the panel with gray background
    wxSize panelSize = m_previewPanel->GetSize();
    dc.SetBrush(wxBrush(wxColour(192, 192, 192))); // Light gray
    dc.SetPen(wxPen(wxColour(128, 128, 128))); // Gray border
    dc.DrawRectangle(0, 0, panelSize.x, panelSize.y);
    
    // Try to render the monster sprite using the creature sprite manager
    wxBitmap* spriteBitmap = nullptr;
    
    // Use creature sprite manager to get the rendered sprite
    if (headColor > 0 || bodyColor > 0 || legsColor > 0 || feetColor > 0) {
        spriteBitmap = g_creature_sprites.getSpriteBitmap(
            lookType, headColor, bodyColor, legsColor, feetColor, 64, 64);
    } else {
        spriteBitmap = g_creature_sprites.getSpriteBitmap(lookType, 64, 64);
    }
    
    if (spriteBitmap) {
        // Calculate center position for the sprite
        int x = (panelSize.x - spriteBitmap->GetWidth()) / 2;
        int y = (panelSize.y - spriteBitmap->GetHeight()) / 2;
        
        // Draw the sprite
        dc.DrawBitmap(*spriteBitmap, x, y, true);
    } else {
        // Fallback: draw a simple colored representation
        DrawFallbackPreview(dc, panelSize, lookType, headColor, bodyColor, legsColor);
    }
}

void MonsterMakerWindow::DrawFallbackPreview(wxDC& dc, const wxSize& size, int lookType, int headColor, int bodyColor, int legsColor) {
    // Draw a simple colored representation
    int centerX = size.x / 2;
    int centerY = size.y / 2;
    
    // Draw a simple creature representation with color zones
    dc.SetBrush(wxBrush(wxColour(150 + (headColor * 2), 100, 100)));
    dc.SetPen(wxPen(wxColour(50, 50, 50)));
    dc.DrawCircle(centerX, centerY - 8, 8); // Head
    
    dc.SetBrush(wxBrush(wxColour(100, 150 + (bodyColor * 2), 100)));
    dc.DrawRectangle(centerX - 6, centerY - 2, 12, 10); // Body
    
    dc.SetBrush(wxBrush(wxColour(100, 100, 150 + (legsColor * 2))));
    dc.DrawRectangle(centerX - 4, centerY + 6, 8, 6); // Legs
    
    // Draw look type number
    dc.SetTextForeground(wxColour(0, 0, 0));
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    wxString lookText = wxString::Format("Type: %d", lookType);
    wxSize textSize = dc.GetTextExtent(lookText);
    dc.DrawText(lookText, (size.x - textSize.x) / 2, size.y - textSize.y - 2);
}

void MonsterMakerWindow::OnClose(wxCloseEvent& event) {
    Hide();
}

void MonsterMakerWindow::OnCloseButton(wxCommandEvent& event) {
    Hide();
}

void MonsterMakerWindow::OnCreateMonster(wxCommandEvent& event) {
    // Create monster entry from current UI values
    MonsterEntry entry;
    entry.name = nstr(m_name->GetValue());
    entry.race = nstr(m_race->GetValue());
    entry.experience = m_experience->GetValue();
    entry.speed = m_speed->GetValue();
    entry.manacost = m_manacost->GetValue();
    entry.health_max = m_healthMax->GetValue();
    entry.look_type = m_lookType->GetValue();
    entry.look_head = m_headPalette->GetSelectedColor();
    entry.look_body = m_bodyPalette->GetSelectedColor();
    entry.look_legs = m_legsPalette->GetSelectedColor();
    entry.look_feet = m_feetPalette->GetSelectedColor();
    
    // Ask for save location
    wxFileDialog saveDialog(this, "Save Monster XML", "", entry.name + ".xml", 
                           "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (saveDialog.ShowModal() == wxID_OK) {
        wxString path = saveDialog.GetPath();
        if (g_gui.monster_manager.createMonsterXML(entry, nstr(path))) {
            wxMessageBox("Monster created successfully!", "Success", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to create monster file!", "Error", wxOK | wxICON_ERROR);
        }
    }
}

void MonsterMakerWindow::OnLoadMonster(wxCommandEvent& event) {
    long selectedIndex = m_monstersList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedIndex == -1) {
        wxMessageBox("Please select a monster to load.", "No Selection", wxOK | wxICON_WARNING);
        return;
    }
    
    wxString monsterName = m_monstersList->GetItemText(selectedIndex, 0);
    MonsterEntry* entry = g_gui.monster_manager.findByName(nstr(monsterName));
    
    if (entry) {
        // Load details if not already loaded
        if (!entry->loaded) {
            g_gui.monster_manager.loadMonsterDetails(*entry);
        }
        LoadMonster(*entry);
        m_notebook->SetSelection(0); // Switch to Monster tab
    } else {
        wxMessageBox("Monster not found!", "Error", wxOK | wxICON_ERROR);
    }
}

void MonsterMakerWindow::OnPreviewUpdate(wxCommandEvent& event) {
    UpdatePreview();
    UpdateXMLPreview();
}

void MonsterMakerWindow::OnTabChange(wxNotebookEvent& event) {
    // Handle tab changes if needed
    event.Skip();
}

void MonsterMakerWindow::OnLookTypeChange(wxCommandEvent& event) {
    // For text events (typing), use shorter delay for more responsive updates
    int delay = (event.GetEventType() == wxEVT_TEXT) ? 300 : 150;
    
    // Restart timer for delayed preview update
    if (m_previewTimer) {
        if (m_previewTimer->IsRunning()) {
            m_previewTimer->Stop();
        }
        m_previewUpdatePending = true;
        m_previewTimer->Start(delay, wxTIMER_ONE_SHOT);
    }
    event.Skip();
}

void MonsterMakerWindow::OnColorChange(wxCommandEvent& event) {
    // Start timer for delayed preview update (150ms delay)
    if (m_previewTimer && !m_previewTimer->IsRunning()) {
        m_previewUpdatePending = true;
        m_previewTimer->Start(150, wxTIMER_ONE_SHOT);
    }
    event.Skip();
}

void MonsterMakerWindow::OnPreviewTimer(wxTimerEvent& event) {
    if (m_previewUpdatePending) {
        UpdatePreview();
        m_previewUpdatePending = false;
    }
}

void MonsterMakerWindow::OnSkullChange(wxCommandEvent& event) {
    // Handle skull selection change
    event.Skip();
}

void MonsterMakerWindow::OnStrategyChange(wxScrollEvent& event) {
    // Handle strategy slider change
    event.Skip();
}

void MonsterMakerWindow::OnLightColorChange(wxCommandEvent& event) {
    // Handle light color change
    event.Skip();
}

void MonsterMakerWindow::OnColorPaletteChange(wxCommandEvent& event) {
    // Handle color palette selection change - update preview immediately for color changes
    if (m_previewTimer) {
        if (m_previewTimer->IsRunning()) {
            m_previewTimer->Stop();
        }
        m_previewUpdatePending = true;
        m_previewTimer->Start(50, wxTIMER_ONE_SHOT); // Very fast response for color changes
    }
    
    // Also update XML preview
    UpdateXMLPreview();
    
    event.Skip();
}

void MonsterMakerWindow::OnShowNumbersToggle(wxCommandEvent& event) {
    bool showNumbers = event.IsChecked();
    
    // Update all color palettes
    if (m_headPalette) m_headPalette->SetShowNumbers(showNumbers);
    if (m_bodyPalette) m_bodyPalette->SetShowNumbers(showNumbers);
    if (m_legsPalette) m_legsPalette->SetShowNumbers(showNumbers);
    if (m_feetPalette) m_feetPalette->SetShowNumbers(showNumbers);
}

// List management methods
void MonsterMakerWindow::AddAttack() {
    MonsterAttackDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        AttackEntry attack = dialog.GetAttackEntry();
        m_currentMonster.attacks.push_back(attack);
        UpdateAttacksList();
    }
}

void MonsterMakerWindow::EditAttack(int index) {
    if (index >= 0 && index < (int)m_currentMonster.attacks.size()) {
        MonsterAttackDialog dialog(this, &m_currentMonster.attacks[index]);
        if (dialog.ShowModal() == wxID_OK) {
            m_currentMonster.attacks[index] = dialog.GetAttackEntry();
            UpdateAttacksList();
        }
    }
}

void MonsterMakerWindow::DeleteAttack(int index) {
    if (index >= 0 && index < (int)m_currentMonster.attacks.size()) {
        m_currentMonster.attacks.erase(m_currentMonster.attacks.begin() + index);
        UpdateAttacksList();
    }
}

void MonsterMakerWindow::AddLoot() {
    MonsterLootDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        LootEntry loot = dialog.GetLootEntry();
        m_currentMonster.loot.push_back(loot);
        UpdateLootList();
    }
}

void MonsterMakerWindow::EditLoot(int index) {
    if (index >= 0 && index < (int)m_currentMonster.loot.size()) {
        MonsterLootDialog dialog(this, &m_currentMonster.loot[index]);
        if (dialog.ShowModal() == wxID_OK) {
            m_currentMonster.loot[index] = dialog.GetLootEntry();
            UpdateLootList();
        }
    }
}

void MonsterMakerWindow::DeleteLoot(int index) {
    if (index >= 0 && index < (int)m_currentMonster.loot.size()) {
        m_currentMonster.loot.erase(m_currentMonster.loot.begin() + index);
        UpdateLootList();
    }
}

void MonsterMakerWindow::AddDefense() {
    MonsterDefenseDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        DefenseEntry defense = dialog.GetDefenseEntry();
        m_currentMonster.defenses.push_back(defense);
        UpdateDefensesList();
    }
}

void MonsterMakerWindow::EditDefense(int index) {
    if (index >= 0 && index < (int)m_currentMonster.defenses.size()) {
        MonsterDefenseDialog dialog(this, &m_currentMonster.defenses[index]);
        if (dialog.ShowModal() == wxID_OK) {
            m_currentMonster.defenses[index] = dialog.GetDefenseEntry();
            UpdateDefensesList();
        }
    }
}

void MonsterMakerWindow::DeleteDefense(int index) {
    if (index >= 0 && index < (int)m_currentMonster.defenses.size()) {
        m_currentMonster.defenses.erase(m_currentMonster.defenses.begin() + index);
        UpdateDefensesList();
    }
}

MonsterEntry MonsterMakerWindow::GetCurrentMonsterEntry() const {
    MonsterEntry entry = m_currentMonster;
    
    // Update basic properties from UI
    entry.name = nstr(m_name->GetValue());
    entry.nameDescription = nstr(m_nameDescription->GetValue());
    entry.race = nstr(m_race->GetValue());
    entry.skull = nstr(m_skull->GetStringSelection());
    entry.experience = m_experience->GetValue();
    entry.speed = m_speed->GetValue();
    entry.manacost = m_manacost->GetValue();
    entry.health_max = m_healthMax->GetValue();
    entry.health_now = m_healthMax->GetValue(); // Default to max
    entry.look_type = m_lookType->GetValue();
    entry.look_head = m_headPalette->GetSelectedColor();
    entry.look_body = m_bodyPalette->GetSelectedColor();
    entry.look_legs = m_legsPalette->GetSelectedColor();
    entry.look_feet = m_feetPalette->GetSelectedColor();
    
    // Update flags from UI
    entry.summonable = m_summonable->GetValue();
    entry.attackable = m_attackable->GetValue();
    entry.hostile = m_hostile->GetValue();
    entry.illusionable = m_illusionable->GetValue();
    entry.convinceable = m_convinceable->GetValue();
    entry.pushable = m_pushable->GetValue();
    entry.canPushItems = m_canpushitems->GetValue();
    entry.canPushCreatures = m_canpushcreatures->GetValue();
    entry.hideHealth = m_hidehealth->GetValue();
    
    return entry;
}

// Update list displays
void MonsterMakerWindow::UpdateAttacksList() {
    if (!m_attacksList) return;
    
    m_attacksList->DeleteAllItems();
    for (size_t i = 0; i < m_currentMonster.attacks.size(); ++i) {
        const AttackEntry& attack = m_currentMonster.attacks[i];
        
        long itemIndex = m_attacksList->InsertItem(i, wxstr(attack.name));
        
        // Damage range
        wxString damageStr;
        if (attack.minDamage == attack.maxDamage) {
            damageStr = wxString::Format("%d", attack.maxDamage);
        } else {
            damageStr = wxString::Format("%d to %d", attack.minDamage, attack.maxDamage);
        }
        m_attacksList->SetItem(itemIndex, 1, damageStr);
        
        m_attacksList->SetItem(itemIndex, 2, wxString::Format("%d", attack.interval));
        m_attacksList->SetItem(itemIndex, 3, wxString::Format("%d", attack.chance));
        m_attacksList->SetItem(itemIndex, 4, attack.range > 0 ? wxString::Format("%d", attack.range) : "-");
        m_attacksList->SetItem(itemIndex, 5, attack.radius > 0 ? wxString::Format("%d", attack.radius) : "-");
        
        // Properties summary
        wxString properties;
        if (attack.length > 0) properties += wxString::Format("Length:%d ", attack.length);
        if (attack.spread > 0) properties += wxString::Format("Spread:%d ", attack.spread);
        if (attack.speedChange != 0) properties += wxString::Format("Speed:%+d ", attack.speedChange);
        if (attack.duration > 0) properties += wxString::Format("Duration:%ds ", attack.duration/1000);
        if (!attack.shootEffect.empty()) properties += wxString::Format("Shoot:%s ", wxstr(attack.shootEffect));
        if (!attack.areaEffect.empty()) properties += wxString::Format("Area:%s ", wxstr(attack.areaEffect));
        if (!attack.conditionType.empty()) properties += wxString::Format("Condition:%s ", wxstr(attack.conditionType));
        
        m_attacksList->SetItem(itemIndex, 6, properties.Trim());
    }
    
    UpdateXMLPreview();
}

void MonsterMakerWindow::UpdateLootList() {
    if (!m_lootList) return;
    
    m_lootList->DeleteAllItems();
    for (size_t i = 0; i < m_currentMonster.loot.size(); ++i) {
        const LootEntry& loot = m_currentMonster.loot[i];
        
        wxString itemName = loot.useItemName ? wxstr(loot.itemName) : wxString::Format("ID: %d", loot.itemId);
        long itemIndex = m_lootList->InsertItem(i, itemName);
        m_lootList->SetItem(itemIndex, 1, loot.useItemName ? wxstr(loot.itemName) : wxString::Format("%d", loot.itemId));
        m_lootList->SetItem(itemIndex, 2, wxString::Format("%.3f", loot.chance / 1000.0));
        m_lootList->SetItem(itemIndex, 3, wxString::Format("%d", loot.countMax));
        
        wxString properties;
        if (loot.subType > 0) properties += wxString::Format("SubType:%d ", loot.subType);
        if (loot.actionId > 0) properties += wxString::Format("ActionID:%d ", loot.actionId);
        if (loot.uniqueId > 0) properties += wxString::Format("UniqueID:%d ", loot.uniqueId);
        m_lootList->SetItem(itemIndex, 4, properties);
    }
    
    UpdateXMLPreview();
}

void MonsterMakerWindow::UpdateDefensesList() {
    if (!m_defensesList) return;
    
    m_defensesList->DeleteAllItems();
    for (size_t i = 0; i < m_currentMonster.defenses.size(); ++i) {
        const DefenseEntry& defense = m_currentMonster.defenses[i];
        
        long itemIndex = m_defensesList->InsertItem(i, wxstr(defense.name));
        m_defensesList->SetItem(itemIndex, 1, wxstr(defense.type));
        m_defensesList->SetItem(itemIndex, 2, wxString::Format("%d", defense.interval));
        m_defensesList->SetItem(itemIndex, 3, wxString::Format("%d", defense.chance));
        
        // Value range for healing defenses
        wxString valueStr;
        if (defense.name == "healing") {
            if (defense.minValue == defense.maxValue) {
                valueStr = wxString::Format("%d", defense.maxValue);
            } else {
                valueStr = wxString::Format("%d-%d", defense.minValue, defense.maxValue);
            }
        } else {
            valueStr = "-";
        }
        m_defensesList->SetItem(itemIndex, 4, valueStr);
        
        // Properties summary
        wxString properties;
        if (defense.type.find("radius=") != std::string::npos) {
            // Extract radius value
            size_t start = defense.type.find("radius=\"") + 8;
            size_t end = defense.type.find("\"", start);
            if (end != std::string::npos) {
                properties += wxString::Format("Radius:%s ", wxstr(defense.type.substr(start, end - start)));
            }
        }
        if (defense.type.find("speedchange=") != std::string::npos) {
            // Extract speedchange value
            size_t start = defense.type.find("speedchange=\"") + 13;
            size_t end = defense.type.find("\"", start);
            if (end != std::string::npos) {
                properties += wxString::Format("Speed:%s ", wxstr(defense.type.substr(start, end - start)));
            }
        }
        if (defense.type.find("duration=") != std::string::npos) {
            // Extract duration value
            size_t start = defense.type.find("duration=\"") + 10;
            size_t end = defense.type.find("\"", start);
            if (end != std::string::npos) {
                properties += wxString::Format("Duration:%sms ", wxstr(defense.type.substr(start, end - start)));
            }
        }
        if (defense.type.find("areaEffect=") != std::string::npos) {
            // Extract area effect value
            size_t start = defense.type.find("areaEffect=\"") + 12;
            size_t end = defense.type.find("\"", start);
            if (end != std::string::npos) {
                properties += wxString::Format("Effect:%s ", wxstr(defense.type.substr(start, end - start)));
            }
        }
        
        m_defensesList->SetItem(itemIndex, 5, properties);
    }
    
    UpdateXMLPreview();
}



void MonsterMakerWindow::UpdateSummonsList() {
    if (!m_summonsList) return;
    
    m_summonsList->DeleteAllItems();
    for (size_t i = 0; i < m_currentMonster.summons.size(); ++i) {
        const SummonEntry& summon = m_currentMonster.summons[i];
        
        long itemIndex = m_summonsList->InsertItem(i, wxstr(summon.name));
        m_summonsList->SetItem(itemIndex, 1, wxString::Format("%d", summon.interval));
        m_summonsList->SetItem(itemIndex, 2, wxString::Format("%d", summon.chance));
    }
}

void MonsterMakerWindow::UpdateVoicesList() {
    if (!m_voicesList) return;
    
    m_voicesList->DeleteAllItems();
    for (size_t i = 0; i < m_currentMonster.voices.size(); ++i) {
        const VoiceEntry& voice = m_currentMonster.voices[i];
        
        long itemIndex = m_voicesList->InsertItem(i, wxstr(voice.text));
        m_voicesList->SetItem(itemIndex, 1, voice.yell ? "Yes" : "No");
    }
}

void MonsterMakerWindow::AddSummon() {
    wxTextEntryDialog dialog(this, "Enter creature name:", "Add Summon", "");
    if (dialog.ShowModal() == wxID_OK) {
        wxString creatureName = dialog.GetValue();
        if (!creatureName.IsEmpty()) {
            SummonEntry summon;
            summon.name = creatureName.ToStdString();
            summon.interval = 2000;
            summon.chance = 10;
            m_currentMonster.summons.push_back(summon);
            UpdateSummonsList();
            UpdateXMLPreview();
        }
    }
}

void MonsterMakerWindow::EditSummon(int index) {
    if (index >= 0 && index < (int)m_currentMonster.summons.size()) {
        SummonEntry& summon = m_currentMonster.summons[index];
        
        wxTextEntryDialog nameDialog(this, "Enter creature name:", "Edit Summon", wxstr(summon.name));
        if (nameDialog.ShowModal() == wxID_OK) {
            summon.name = nameDialog.GetValue().ToStdString();
            
            // Get interval
            wxString intervalStr = wxGetTextFromUser("Enter interval (ms):", "Edit Summon", wxString::Format("%d", summon.interval), this);
            if (!intervalStr.IsEmpty()) {
                long intervalValue;
                if (intervalStr.ToLong(&intervalValue) && intervalValue >= 100 && intervalValue <= 60000) {
                    summon.interval = intervalValue;
                    
                    // Get chance
                    wxString chanceStr = wxGetTextFromUser("Enter chance (%):", "Edit Summon", wxString::Format("%d", summon.chance), this);
                    if (!chanceStr.IsEmpty()) {
                        long chanceValue;
                        if (chanceStr.ToLong(&chanceValue) && chanceValue >= 1 && chanceValue <= 100) {
                            summon.chance = chanceValue;
                            UpdateSummonsList();
                            UpdateXMLPreview();
                        }
                    }
                }
            }
        }
    }
}

void MonsterMakerWindow::DeleteSummon(int index) {
    if (index >= 0 && index < (int)m_currentMonster.summons.size()) {
        m_currentMonster.summons.erase(m_currentMonster.summons.begin() + index);
        UpdateSummonsList();
        UpdateXMLPreview();
    }
}

void MonsterMakerWindow::AddVoice() {
    wxTextEntryDialog dialog(this, "Enter voice message:", "Add Voice", "");
    if (dialog.ShowModal() == wxID_OK) {
        wxString message = dialog.GetValue();
        if (!message.IsEmpty()) {
            VoiceEntry voice;
            voice.text = message.ToStdString();
            voice.yell = false;
            m_currentMonster.voices.push_back(voice);
            UpdateVoicesList();
            UpdateXMLPreview();
        }
    }
}

void MonsterMakerWindow::EditVoice(int index) {
    if (index >= 0 && index < (int)m_currentMonster.voices.size()) {
        VoiceEntry& voice = m_currentMonster.voices[index];
        
        wxTextEntryDialog messageDialog(this, "Enter voice message:", "Edit Voice", wxstr(voice.text));
        if (messageDialog.ShowModal() == wxID_OK) {
            voice.text = messageDialog.GetValue().ToStdString();
            
            int result = wxMessageBox("Should this voice be a yell?", "Voice Type", wxYES_NO | wxICON_QUESTION);
            voice.yell = (result == wxYES);
            
            UpdateVoicesList();
            UpdateXMLPreview();
        }
    }
}

void MonsterMakerWindow::DeleteVoice(int index) {
    if (index >= 0 && index < (int)m_currentMonster.voices.size()) {
        m_currentMonster.voices.erase(m_currentMonster.voices.begin() + index);
        UpdateVoicesList();
        UpdateXMLPreview();
    }
}

void MonsterMakerWindow::CreateXMLPreviewTab(wxNotebook* notebook) {
    wxPanel* panel = new wxPanel(notebook);
    notebook->AddPage(panel, "XML Preview");
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Live XML Preview - Updates automatically as you edit"), 0, wxALL, 5);
    
    m_xmlPreview = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 
                                  wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    m_xmlPreview->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    
    sizer->Add(m_xmlPreview, 1, wxEXPAND | wxALL, 5);
    
    panel->SetSizer(sizer);
}

void MonsterMakerWindow::UpdateXMLPreview() {
    if (!m_xmlPreview) return;
    
    MonsterEntry entry = GetCurrentMonsterEntry();
    
    wxString xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += wxString::Format("<monster name=\"%s\"", wxstr(entry.name));
    
    if (!entry.nameDescription.empty()) {
        xml += wxString::Format(" nameDescription=\"%s\"", wxstr(entry.nameDescription));
    }
    if (!entry.race.empty()) {
        xml += wxString::Format(" race=\"%s\"", wxstr(entry.race));
    }
    if (entry.experience > 0) {
        xml += wxString::Format(" experience=\"%d\"", entry.experience);
    }
    if (entry.speed > 0) {
        xml += wxString::Format(" speed=\"%d\"", entry.speed);
    }
    if (entry.manacost > 0) {
        xml += wxString::Format(" manacost=\"%d\"", entry.manacost);
    }
    
    xml += ">\n";
    
    // Health
    if (entry.health_max > 0) {
        xml += wxString::Format("\t<health now=\"%d\" max=\"%d\"/>\n", 
                               entry.health_now > 0 ? entry.health_now : entry.health_max, 
                               entry.health_max);
    }
    
    // Look
    if (entry.look_type > 0) {
        xml += wxString::Format("\t<look type=\"%d\"", entry.look_type);
        if (entry.look_head > 0) xml += wxString::Format(" head=\"%d\"", entry.look_head);
        if (entry.look_body > 0) xml += wxString::Format(" body=\"%d\"", entry.look_body);
        if (entry.look_legs > 0) xml += wxString::Format(" legs=\"%d\"", entry.look_legs);
        if (entry.look_feet > 0) xml += wxString::Format(" feet=\"%d\"", entry.look_feet);
        if (entry.look_addons > 0) xml += wxString::Format(" addons=\"%d\"", entry.look_addons);
        if (entry.look_mount > 0) xml += wxString::Format(" mount=\"%d\"", entry.look_mount);
        if (entry.corpse > 0) xml += wxString::Format(" corpse=\"%d\"", entry.corpse);
        xml += "/>\n";
    }
    
    // Target change
    if (entry.interval > 0 && entry.chance > 0) {
        xml += wxString::Format("\t<targetchange interval=\"%d\" chance=\"%d\"/>\n", entry.interval, entry.chance);
    }
    
    // Flags
    if (entry.summonable || entry.attackable || entry.hostile || entry.illusionable || 
        entry.convinceable || entry.pushable || entry.canPushItems || entry.canPushCreatures ||
        entry.hideHealth || entry.targetDistance > 0 || entry.staticAttack != 90 || entry.runOnHealth > 0) {
        
        xml += "\t<flags>\n";
        
        if (entry.summonable) xml += "\t\t<flag summonable=\"1\"/>\n";
        else xml += "\t\t<flag summonable=\"0\"/>\n";
        
        if (entry.attackable) xml += "\t\t<flag attackable=\"1\"/>\n";
        else xml += "\t\t<flag attackable=\"0\"/>\n";
        
        if (entry.hostile) xml += "\t\t<flag hostile=\"1\"/>\n";
        else xml += "\t\t<flag hostile=\"0\"/>\n";
        
        if (entry.illusionable) xml += "\t\t<flag illusionable=\"1\"/>\n";
        else xml += "\t\t<flag illusionable=\"0\"/>\n";
        
        if (entry.convinceable) xml += "\t\t<flag convinceable=\"1\"/>\n";
        else xml += "\t\t<flag convinceable=\"0\"/>\n";
        
        if (entry.pushable) xml += "\t\t<flag pushable=\"1\"/>\n";
        else xml += "\t\t<flag pushable=\"0\"/>\n";
        
        if (entry.canPushItems) xml += "\t\t<flag canpushitems=\"1\"/>\n";
        if (entry.canPushCreatures) xml += "\t\t<flag canpushcreatures=\"1\"/>\n";
        if (entry.hideHealth) xml += "\t\t<flag hidehealth=\"1\"/>\n";
        
        if (entry.targetDistance > 0) {
            xml += wxString::Format("\t\t<flag targetdistance=\"%d\"/>\n", entry.targetDistance);
        }
        if (entry.staticAttack != 90) {
            xml += wxString::Format("\t\t<flag staticattack=\"%d\"/>\n", entry.staticAttack);
        }
        if (entry.runOnHealth > 0) {
            xml += wxString::Format("\t\t<flag runonhealth=\"%d\"/>\n", entry.runOnHealth);
        }
        
        xml += "\t</flags>\n";
    }
    
    // Attacks
    if (!entry.attacks.empty()) {
        xml += "\t<attacks>\n";
        for (const auto& attack : entry.attacks) {
            xml += wxString::Format("\t\t<attack name=\"%s\"", wxstr(attack.name));
            if (attack.interval > 0) xml += wxString::Format(" interval=\"%d\"", attack.interval);
            if (attack.chance < 100) xml += wxString::Format(" chance=\"%d\"", attack.chance);
            if (attack.range > 0) xml += wxString::Format(" range=\"%d\"", attack.range);
            if (attack.radius > 0) xml += wxString::Format(" radius=\"%d\"", attack.radius);
            if (attack.target > 0) xml += wxString::Format(" target=\"%d\"", attack.target);
            if (attack.length > 0) xml += wxString::Format(" length=\"%d\"", attack.length);
            if (attack.spread > 0) xml += wxString::Format(" spread=\"%d\"", attack.spread);
            if (attack.minDamage != 0) xml += wxString::Format(" min=\"%d\"", attack.minDamage);
            if (attack.maxDamage != 0) xml += wxString::Format(" max=\"%d\"", attack.maxDamage);
            if (attack.speedChange != 0) xml += wxString::Format(" speedchange=\"%d\"", attack.speedChange);
            if (attack.duration > 0) xml += wxString::Format(" duration=\"%d\"", attack.duration);
            
            // Check if we need attributes section
            bool hasAttributes = !attack.shootEffect.empty() || !attack.areaEffect.empty();
            
            if (hasAttributes) {
                xml += ">\n";
                if (!attack.shootEffect.empty()) {
                    xml += wxString::Format("\t\t\t<attribute key=\"shootEffect\" value=\"%s\"/>\n", wxstr(attack.shootEffect));
                }
                if (!attack.areaEffect.empty()) {
                    xml += wxString::Format("\t\t\t<attribute key=\"areaEffect\" value=\"%s\"/>\n", wxstr(attack.areaEffect));
                }
                xml += "\t\t</attack>\n";
            } else {
                xml += "/>\n";
            }
        }
        xml += "\t</attacks>\n";
    }
    
    // Defenses
    if (!entry.defenses.empty()) {
        xml += "\t<defenses>\n";
        for (const auto& defense : entry.defenses) {
            xml += wxString::Format("\t\t<defense name=\"%s\"", wxstr(defense.name));
            if (defense.interval > 0) xml += wxString::Format(" interval=\"%d\"", defense.interval);
            if (defense.chance < 100) xml += wxString::Format(" chance=\"%d\"", defense.chance);
            
            // Add healing values for healing defenses
            if (defense.name == "healing") {
                if (defense.minValue > 0) xml += wxString::Format(" min=\"%d\"", defense.minValue);
                if (defense.maxValue > 0) xml += wxString::Format(" max=\"%d\"", defense.maxValue);
            }
            
            // Parse additional properties from type field
            wxString typeStr = wxstr(defense.type);
            if (typeStr.Contains("radius=")) {
                // Extract radius value
                int start = typeStr.Find("radius=\"") + 8;
                int end = typeStr.find("\"", start);
                if (end != wxNOT_FOUND) {
                    xml += " radius=\"" + typeStr.Mid(start, end - start) + "\"";
                }
            }
            if (typeStr.Contains("speedchange=")) {
                // Extract speedchange value
                int start = typeStr.Find("speedchange=\"") + 13;
                int end = typeStr.find("\"", start);
                if (end != wxNOT_FOUND) {
                    xml += " speedchange=\"" + typeStr.Mid(start, end - start) + "\"";
                }
            }
            if (typeStr.Contains("duration=")) {
                // Extract duration value
                int start = typeStr.Find("duration=\"") + 10;
                int end = typeStr.find("\"", start);
                if (end != wxNOT_FOUND) {
                    xml += " duration=\"" + typeStr.Mid(start, end - start) + "\"";
                }
            }
            
            // Check if we need attributes section for area effect
            bool hasAreaEffect = typeStr.Contains("areaEffect=");
            
            if (hasAreaEffect) {
                xml += ">\n";
                // Extract area effect value
                int start = typeStr.Find("areaEffect=\"") + 12;
                int end = typeStr.find("\"", start);
                if (end != wxNOT_FOUND) {
                    xml += wxString::Format("\t\t\t<attribute key=\"areaEffect\" value=\"%s\"/>\n", 
                                          typeStr.Mid(start, end - start));
                }
                xml += "\t\t</defense>\n";
            } else {
                xml += "/>\n";
            }
        }
        xml += "\t</defenses>\n";
    }
    
    // Loot
    if (!entry.loot.empty()) {
        xml += "\t<loot>\n";
        for (const auto& loot : entry.loot) {
            xml += "\t\t<item";
            if (loot.useItemName && !loot.itemName.empty()) {
                xml += wxString::Format(" name=\"%s\"", wxstr(loot.itemName));
            } else if (loot.itemId > 0) {
                xml += wxString::Format(" id=\"%d\"", loot.itemId);
            }
            if (loot.chance < 100000) {
                xml += wxString::Format(" chance=\"%d\"", loot.chance);
            }
            if (loot.countMax > 1) {
                xml += wxString::Format(" countmax=\"%d\"", loot.countMax);
            }
            if (loot.subType > 0) {
                xml += wxString::Format(" subtype=\"%d\"", loot.subType);
            }
            if (loot.actionId > 0) {
                xml += wxString::Format(" actionid=\"%d\"", loot.actionId);
            }
            if (loot.uniqueId > 0) {
                xml += wxString::Format(" uniqueid=\"%d\"", loot.uniqueId);
            }
            xml += "/>\n";
        }
        xml += "\t</loot>\n";
    }
    
    // Elements - Get values from UI controls
    bool hasElements = false;
    wxString elementsXML = "";
    
    if (m_physicalPercent && m_physicalPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element physicalPercent=\"%d\"/>\n", m_physicalPercent->GetValue());
        hasElements = true;
    }
    if (m_firePercent && m_firePercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element firePercent=\"%d\"/>\n", m_firePercent->GetValue());
        hasElements = true;
    }
    if (m_energyPercent && m_energyPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element energyPercent=\"%d\"/>\n", m_energyPercent->GetValue());
        hasElements = true;
    }
    if (m_earthPercent && m_earthPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element earthPercent=\"%d\"/>\n", m_earthPercent->GetValue());
        hasElements = true;
    }
    if (m_icePercent && m_icePercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element icePercent=\"%d\"/>\n", m_icePercent->GetValue());
        hasElements = true;
    }
    if (m_holyPercent && m_holyPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element holyPercent=\"%d\"/>\n", m_holyPercent->GetValue());
        hasElements = true;
    }
    if (m_deathPercent && m_deathPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element deathPercent=\"%d\"/>\n", m_deathPercent->GetValue());
        hasElements = true;
    }
    if (m_drownPercent && m_drownPercent->GetValue() != 0) {
        elementsXML += wxString::Format("\t\t<element drownPercent=\"%d\"/>\n", m_drownPercent->GetValue());
        hasElements = true;
    }
    
    if (hasElements) {
        xml += "\t<elements>\n";
        xml += elementsXML;
        xml += "\t</elements>\n";
    }
    
    // Immunities - Get values from UI controls
    bool hasImmunities = false;
    wxString immunitiesXML = "";
    
    if (m_immunityFire && m_immunityFire->GetValue()) {
        immunitiesXML += "\t\t<immunity fire=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityEnergy && m_immunityEnergy->GetValue()) {
        immunitiesXML += "\t\t<immunity energy=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityEarth && m_immunityEarth->GetValue()) {
        immunitiesXML += "\t\t<immunity earth=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityIce && m_immunityIce->GetValue()) {
        immunitiesXML += "\t\t<immunity ice=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityHoly && m_immunityHoly->GetValue()) {
        immunitiesXML += "\t\t<immunity holy=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityDeath && m_immunityDeath->GetValue()) {
        immunitiesXML += "\t\t<immunity death=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityPhysical && m_immunityPhysical->GetValue()) {
        immunitiesXML += "\t\t<immunity physical=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityDrown && m_immunityDrown->GetValue()) {
        immunitiesXML += "\t\t<immunity drown=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityParalyze && m_immunityParalyze->GetValue()) {
        immunitiesXML += "\t\t<immunity paralyze=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityInvisible && m_immunityInvisible->GetValue()) {
        immunitiesXML += "\t\t<immunity invisible=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityLifedrain && m_immunityLifedrain->GetValue()) {
        immunitiesXML += "\t\t<immunity lifedrain=\"1\"/>\n";
        hasImmunities = true;
    }
    if (m_immunityDrunk && m_immunityDrunk->GetValue()) {
        immunitiesXML += "\t\t<immunity drunk=\"1\"/>\n";
        hasImmunities = true;
    }
    
    if (hasImmunities) {
        xml += "\t<immunities>\n";
        xml += immunitiesXML;
        xml += "\t</immunities>\n";
    }
    
    // Summons
    if (!entry.summons.empty()) {
        int maxSummons = m_maxSummons ? m_maxSummons->GetValue() : 0;
        xml += wxString::Format("\t<summons maxSummons=\"%d\">\n", maxSummons > 0 ? maxSummons : (int)entry.summons.size());
        for (const auto& summon : entry.summons) {
            xml += wxString::Format("\t\t<summon name=\"%s\" interval=\"%d\" chance=\"%d\"/>\n",
                                   wxstr(summon.name), summon.interval, summon.chance);
        }
        xml += "\t</summons>\n";
    }
    
    // Voices
    if (!entry.voices.empty()) {
        int voiceInterval = m_voiceInterval ? m_voiceInterval->GetValue() : 5000;
        int voiceChance = m_voiceChance ? m_voiceChance->GetValue() : 10;
        xml += wxString::Format("\t<voices interval=\"%d\" chance=\"%d\">\n", voiceInterval, voiceChance);
        for (const auto& voice : entry.voices) {
            xml += wxString::Format("\t\t<voice sentence=\"%s\"", wxstr(voice.text));
            if (voice.yell) {
                xml += " yell=\"1\"";
            }
            xml += "/>\n";
        }
        xml += "\t</voices>\n";
    }
    
    xml += "</monster>\n";
    
    m_xmlPreview->SetValue(xml);
}

// New event handlers for summons and voices
void MonsterMakerWindow::OnAddSummon(wxCommandEvent& event) {
    AddSummon();
}

void MonsterMakerWindow::OnEditSummon(wxCommandEvent& event) {
    if (m_summonsList) {
        long selected = m_summonsList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected != -1) {
            EditSummon(selected);
        }
    }
}

void MonsterMakerWindow::OnDeleteSummon(wxCommandEvent& event) {
    if (m_summonsList) {
        long selected = m_summonsList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected != -1) {
            DeleteSummon(selected);
        }
    }
}

void MonsterMakerWindow::OnAddVoice(wxCommandEvent& event) {
    AddVoice();
}

void MonsterMakerWindow::OnEditVoice(wxCommandEvent& event) {
    if (m_voicesList) {
        long selected = m_voicesList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected != -1) {
            EditVoice(selected);
        }
    }
}

void MonsterMakerWindow::OnDeleteVoice(wxCommandEvent& event) {
    if (m_voicesList) {
        long selected = m_voicesList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected != -1) {
            DeleteVoice(selected);
        }
    }
} 