#ifndef RME_ADD_CREATURE_DIALOG_H_
#define RME_ADD_CREATURE_DIALOG_H_

#include <wx/dialog.h>
#include "creatures.h"

class wxSpinCtrl;
class wxTextCtrl;
class wxRadioBox;
class wxPanel;

class AddCreatureDialog : public wxDialog {
public:
    AddCreatureDialog(wxWindow* parent, const wxString& name);
    virtual ~AddCreatureDialog();

    CreatureType* GetCreatureType() const { return creature_type; }

protected:
    void OnClickOK(wxCommandEvent& event);
    void OnClickCancel(wxCommandEvent& event);
    void OnOutfitChange(wxCommandEvent& event);
    void OnPreviewPaint(wxPaintEvent& event);

private:
    wxTextCtrl* name_field;
    wxSpinCtrl* looktype_field;
    wxSpinCtrl* lookhead_field;
    wxSpinCtrl* lookbody_field;
    wxSpinCtrl* looklegs_field;
    wxSpinCtrl* lookfeet_field;
    wxRadioBox* type_radio;
    wxPanel* preview_panel;

    CreatureType* creature_type;
    
    // Helper functions
    void UpdatePreview();

    DECLARE_EVENT_TABLE()
};

#endif // RME_ADD_CREATURE_DIALOG_H_ 