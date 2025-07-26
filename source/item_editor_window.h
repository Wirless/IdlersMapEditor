#ifndef RME_ITEM_EDITOR_WINDOW_H_
#define RME_ITEM_EDITOR_WINDOW_H_

#include "main.h"
#include "items.h"
#include <wx/grid.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>

class ItemType;

// Item Editor Dialog for editing items.otb files
class ItemEditorWindow : public wxDialog
{
public:
    ItemEditorWindow(wxWindow* parent);
    virtual ~ItemEditorWindow();

    // Show the dialog and handle user interaction
    int ShowModal() override;

private:
    // Event handlers
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnItemSelected(wxGridEvent& event);
    void OnSaveOTB(wxCommandEvent& event);
    void OnReloadOTB(wxCommandEvent& event);
    void OnGenerateMissing(wxCommandEvent& event);
    void OnCreateItem(wxCommandEvent& event);
    void OnDuplicateItem(wxCommandEvent& event);
    void OnDeleteItem(wxCommandEvent& event);
    void OnFindItem(wxCommandEvent& event);
    
    // Property change handlers
    void OnPropertyChanged(wxCommandEvent& event);
    void OnGroupChanged(wxCommandEvent& event);
    void OnTypeChanged(wxCommandEvent& event);
    
    // UI creation methods
    void CreateControls();
    void CreateItemGrid(wxPanel* parent);
    void CreatePropertiesPanel(wxPanel* parent);
    void CreateButtonPanel();
    
    // Data management
    void LoadItemList();
    void LoadItemProperties(uint16_t itemId);
    void SaveItemProperties();
    void ApplyChangesToItem(ItemType* item);
    void RefreshItemDisplay();
    
    // Helper methods
    wxString GetItemDisplayText(const ItemType& item);
    void EnablePropertyControls(bool enable);
    bool SaveOTBFile(const wxString& filename = "");
    bool GenerateMissingItems();
    
    // Member variables
    wxGrid* m_itemGrid;
    
    // Properties panel
    wxScrolledWindow* m_propertiesPanel;
    wxTextCtrl* m_nameCtrl;
    wxTextCtrl* m_descCtrl;
    wxSpinCtrl* m_clientIdCtrl;
    wxSpinCtrl* m_serverIdCtrl;
    wxChoice* m_groupChoice;
    wxChoice* m_typeChoice;
    wxSpinCtrl* m_speedCtrl;
    wxTextCtrl* m_weightCtrl;
    
    // Flags
    wxCheckBox* m_unpassableFlag;
    wxCheckBox* m_blockMissilesFlag;
    wxCheckBox* m_blockPathfinderFlag;
    wxCheckBox* m_hasElevationFlag;
    wxCheckBox* m_pickupableFlag;
    wxCheckBox* m_moveableFlag;
    wxCheckBox* m_stackableFlag;
    wxCheckBox* m_readableFlag;
    wxCheckBox* m_rotableFlag;
    wxCheckBox* m_hangableFlag;
    wxCheckBox* m_hookEastFlag;
    wxCheckBox* m_hookSouthFlag;
    wxCheckBox* m_allowDistReadFlag;
    wxCheckBox* m_floorChangeDownFlag;
    wxCheckBox* m_floorChangeNorthFlag;
    wxCheckBox* m_floorChangeSouthFlag;
    wxCheckBox* m_floorChangeEastFlag;
    wxCheckBox* m_floorChangeWestFlag;
    
    // Attributes
    wxSpinCtrl* m_lightLevelCtrl;
    wxSpinCtrl* m_lightColorCtrl;
    wxSpinCtrl* m_minimapColorCtrl;
    wxSpinCtrl* m_maxTextLenCtrl;
    wxSpinCtrl* m_rotateToCtrl;
    wxSpinCtrl* m_volumeCtrl;
    
    // Button panel
    wxPanel* m_buttonPanel;
    wxButton* m_saveButton;
    wxButton* m_reloadButton;
    wxButton* m_generateButton;
    wxButton* m_createButton;
    wxButton* m_duplicateButton;
    wxButton* m_deleteButton;
    wxButton* m_findButton;
    
    // Current state
    uint16_t m_currentItemId;
    bool m_itemsModified;
    
    wxTimer m_refreshTimer;
    uint16_t m_pendingClientId;
    void OnSpinCtrlChanged(wxSpinEvent& event);
    void OnRefreshTimer(wxTimerEvent& event);
    
    DECLARE_EVENT_TABLE()
};

enum
{
    ID_ITEM_GRID = 1000,
    ID_SAVE_OTB,
    ID_RELOAD_OTB,
    ID_GENERATE_MISSING,
    ID_CREATE_ITEM,
    ID_DUPLICATE_ITEM,
    ID_DELETE_ITEM,
    ID_FIND_ITEM,
    ID_GROUP_CHOICE,
    ID_TYPE_CHOICE
};

#endif // RME_ITEM_EDITOR_WINDOW_H_ 