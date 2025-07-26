#ifndef RME_MONSTER_MAKER_WINDOW_H_
#define RME_MONSTER_MAKER_WINDOW_H_

#include "main.h"
#include "monster_manager.h"
#include "color_palette_ctrl.h"

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/listctrl.h>
#include <wx/timer.h>
#include <wx/checkbox.h>
#include <wx/slider.h>

// Forward declarations
class MonsterMakerWindow;
class MonsterAttackDialog;
class MonsterLootDialog;

// Custom list control with right-click context menu support
class ContextMenuListCtrl : public wxListCtrl {
public:
    ContextMenuListCtrl(wxWindow* parent, wxWindowID id, MonsterMakerWindow* owner, const wxString& listType);
    
    void OnContextMenu(wxContextMenuEvent& event);
    void OnMenuAdd(wxCommandEvent& event);
    void OnMenuEdit(wxCommandEvent& event);
    void OnMenuDelete(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()

private:
    MonsterMakerWindow* m_owner;
    wxString m_listType;
    wxMenu* m_contextMenu;
};

// Custom preview panel that handles its own painting
class MonsterPreviewPanel : public wxPanel {
public:
    MonsterPreviewPanel(wxWindow* parent, MonsterMakerWindow* owner);
    
    void OnPaint(wxPaintEvent& event);
    
    DECLARE_EVENT_TABLE()

private:
    MonsterMakerWindow* m_owner;
};

class MonsterMakerWindow : public wxFrame {
public:
    MonsterMakerWindow(wxWindow* parent);
    ~MonsterMakerWindow();

    // Set monster data from existing monster
    void LoadMonster(const MonsterEntry& monster);
    
    // Clear all fields
    void ClearAll();
    
    // Refresh the monsters list
    void RefreshMonsterList();
    
    // Preview methods
    void UpdatePreview();
    void DrawFallbackPreview(wxDC& dc, const wxSize& size, int lookType, int headColor, int bodyColor, int legsColor);
    
    // List management methods
    void AddAttack();
    void EditAttack(int index);
    void DeleteAttack(int index);
    void AddLoot();
    void EditLoot(int index);
    void DeleteLoot(int index);
    void AddDefense();
    void EditDefense(int index);
    void DeleteDefense(int index);
    void AddSummon();
    void EditSummon(int index);
    void DeleteSummon(int index);
    void AddVoice();
    void EditVoice(int index);
    void DeleteVoice(int index);
    
    // Get current monster entry from UI
    MonsterEntry GetCurrentMonsterEntry() const;
    
    // Update list displays
    void UpdateAttacksList();
    void UpdateLootList();
    void UpdateDefensesList();
    void UpdateSummonsList();
    void UpdateVoicesList();
    
    // XML Preview
    void UpdateXMLPreview();

private:
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnCloseButton(wxCommandEvent& event);
    void OnCreateMonster(wxCommandEvent& event);
    void OnLoadMonster(wxCommandEvent& event);
    void OnAddSummon(wxCommandEvent& event);
    void OnEditSummon(wxCommandEvent& event);
    void OnDeleteSummon(wxCommandEvent& event);
    void OnAddVoice(wxCommandEvent& event);
    void OnEditVoice(wxCommandEvent& event);
    void OnDeleteVoice(wxCommandEvent& event);
    void OnPreviewUpdate(wxCommandEvent& event);
    void OnTabChange(wxNotebookEvent& event);
    void OnLookTypeChange(wxCommandEvent& event);
    void OnColorChange(wxCommandEvent& event);
    void OnPreviewTimer(wxTimerEvent& event);
    void OnSkullChange(wxCommandEvent& event);
    void OnStrategyChange(wxScrollEvent& event);
    void OnLightColorChange(wxCommandEvent& event);
    void OnColorPaletteChange(wxCommandEvent& event);
    void OnShowNumbersToggle(wxCommandEvent& event);
    
    // UI Creation
    void CreateMonsterTab(wxNotebook* notebook);
    void CreateFlagsTab(wxNotebook* notebook);
    void CreateAttacksTab(wxNotebook* notebook);
    void CreateDefensesTab(wxNotebook* notebook);
    void CreateElementsTab(wxNotebook* notebook);
    void CreateImmunitiesTab(wxNotebook* notebook);
    void CreateSummonsTab(wxNotebook* notebook);
    void CreateVoicesTab(wxNotebook* notebook);
    void CreateLootTab(wxNotebook* notebook);
    void CreateIOTab(wxNotebook* notebook);
    void CreateXMLPreviewTab(wxNotebook* notebook);
    
    void SaveToFile(const wxString& filename);
    bool LoadFromFile(const wxString& filename);
    
    // UI Components
    wxNotebook* m_notebook;
    
    // Monster Tab - Enhanced
    wxTextCtrl* m_name;
    wxTextCtrl* m_nameDescription;
    wxComboBox* m_race;
    wxComboBox* m_skull;
    wxSpinCtrl* m_experience;
    wxSpinCtrl* m_speed;
    wxSpinCtrl* m_manacost;
    wxSpinCtrl* m_healthNow;
    wxSpinCtrl* m_healthMax;
    
    // Look settings - Enhanced
    wxRadioButton* m_lookTypeCheck;
    wxRadioButton* m_lookTypeExCheck;
    wxSpinCtrl* m_lookType;
    ColorPaletteCtrl* m_headPalette;
    ColorPaletteCtrl* m_bodyPalette;
    ColorPaletteCtrl* m_legsPalette;
    ColorPaletteCtrl* m_feetPalette;
    wxSpinCtrl* m_addons;
    wxSpinCtrl* m_mount;
    wxSpinCtrl* m_corpse;
    
    // Combat settings - Enhanced
    wxSpinCtrl* m_interval;
    wxSpinCtrl* m_chance;
    wxCheckBox* m_strategyCheck;
    wxSlider* m_strategy;
    wxSpinCtrl* m_targetDistance;
    wxSlider* m_staticAttack;
    wxSpinCtrl* m_lightColor;
    wxSpinCtrl* m_lightLevel;
    wxSlider* m_runOnHealth;
    
    // Preview
    MonsterPreviewPanel* m_previewPanel;
    wxTimer* m_previewTimer;
    bool m_previewUpdatePending;
    
    // Flags Tab - Complete implementation
    wxCheckBox* m_summonable;
    wxCheckBox* m_attackable;
    wxCheckBox* m_hostile;
    wxCheckBox* m_illusionable;
    wxCheckBox* m_convinceable;
    wxCheckBox* m_pushable;
    wxCheckBox* m_canpushitems;
    wxCheckBox* m_canpushcreatures;
    wxCheckBox* m_hidehealth;
    
    // Enhanced list controls with context menus
    ContextMenuListCtrl* m_attacksList;
    ContextMenuListCtrl* m_defensesList;
    ContextMenuListCtrl* m_lootList;
    
    // Elements Tab - Simple percentage controls
    wxSpinCtrl* m_physicalPercent;
    wxSpinCtrl* m_firePercent;
    wxSpinCtrl* m_energyPercent;
    wxSpinCtrl* m_earthPercent;
    wxSpinCtrl* m_icePercent;
    wxSpinCtrl* m_holyPercent;
    wxSpinCtrl* m_deathPercent;
    wxSpinCtrl* m_drownPercent;
    
    // Immunities Tab - Simple checkboxes
    wxCheckBox* m_immunityFire;
    wxCheckBox* m_immunityEnergy;
    wxCheckBox* m_immunityEarth;
    wxCheckBox* m_immunityIce;
    wxCheckBox* m_immunityHoly;
    wxCheckBox* m_immunityDeath;
    wxCheckBox* m_immunityPhysical;
    wxCheckBox* m_immunityDrown;
    wxCheckBox* m_immunityParalyze;
    wxCheckBox* m_immunityInvisible;
    wxCheckBox* m_immunityLifedrain;
    wxCheckBox* m_immunityDrunk;
    
    // Summons Tab - List and controls
    wxSpinCtrl* m_maxSummons;
    ContextMenuListCtrl* m_summonsList;
    
    // Voices Tab - List and controls
    wxSpinCtrl* m_voiceInterval;
    wxSpinCtrl* m_voiceChance;
    ContextMenuListCtrl* m_voicesList;
    
    // IO Tab
    wxListCtrl* m_monstersList;
    wxButton* m_loadButton;
    wxButton* m_saveButton;
    wxButton* m_createButton;
    
    // XML Preview Tab
    wxTextCtrl* m_xmlPreview;
    
    // Current monster data
    MonsterEntry m_currentMonster;
    
    DECLARE_EVENT_TABLE()
};

enum {
    ID_NOTEBOOK = wxID_HIGHEST + 1,
    ID_CREATE_MONSTER,
    ID_LOAD_MONSTER,
    ID_SAVE_MONSTER,
    ID_CLOSE_BUTTON,
    ID_PREVIEW_TIMER,
    ID_SKULL_COMBO,
    ID_STRATEGY_SLIDER,
    ID_LIGHT_COLOR_SPIN,
    
    // Color palette IDs
    ID_HEAD_PALETTE,
    ID_BODY_PALETTE,
    ID_LEGS_PALETTE,
    ID_FEET_PALETTE,
    ID_SHOW_NUMBERS_TOGGLE,
    
    // Context menu IDs
    ID_CONTEXT_ADD = wxID_HIGHEST + 100,
    ID_CONTEXT_EDIT,
    ID_CONTEXT_DELETE,
    
    // List control IDs
    ID_ATTACKS_LIST,
    ID_DEFENSES_LIST,
    ID_SUMMONS_LIST,
    ID_VOICES_LIST,
    ID_LOOT_LIST,
    
    // Button IDs for new systems
    ID_ADD_SUMMON,
    ID_EDIT_SUMMON,
    ID_DELETE_SUMMON,
    ID_ADD_VOICE,
    ID_EDIT_VOICE,
    ID_DELETE_VOICE
};

#endif // RME_MONSTER_MAKER_WINDOW_H_ 