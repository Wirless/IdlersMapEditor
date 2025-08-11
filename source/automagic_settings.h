#ifndef RME_AUTOMAGIC_SETTINGS_H_
#define RME_AUTOMAGIC_SETTINGS_H_

#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>

class AutomagicSettingsDialog : public wxDialog {
public:
    AutomagicSettingsDialog(wxWindow* parent);
    virtual ~AutomagicSettingsDialog();

    // Get the current settings from the dialog
    bool IsAutomagicEnabled() const;
    bool IsSameGroundTypeBorderEnabled() const;
    bool IsWallsRepelBordersEnabled() const;
    bool IsLayerCarpetsEnabled() const;
    bool IsForceRoofOuterBordersEnabled() const;
    bool IsBorderizeDeleteEnabled() const;
    // New custom border methods
    bool IsCustomBorderEnabled() const;
    int GetCustomBorderId() const;

protected:
    // Event handlers
    void OnClickOK(wxCommandEvent& event);
    void OnClickCancel(wxCommandEvent& event);
    void OnAutomagicCheck(wxCommandEvent& event);
    void OnSameGroundTypeCheck(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnCustomBorderCheck(wxCommandEvent& event); // New event handler

    // UI elements
    wxCheckBox* automagic_enabled_checkbox;
    wxCheckBox* same_ground_type_checkbox;
    wxCheckBox* walls_repel_borders_checkbox;
    wxCheckBox* layer_carpets_checkbox;
    wxCheckBox* force_roof_outer_borders_checkbox;
    wxCheckBox* borderize_delete_checkbox;
    // New UI elements for custom border
    wxCheckBox* custom_border_checkbox;
    wxSpinCtrl* custom_border_id_field;
    wxStaticText* custom_border_id_label;
    wxStdDialogButtonSizer* buttons_sizer;
    wxButton* ok_button;
    wxButton* cancel_button;

    DECLARE_EVENT_TABLE()
};

#endif // RME_AUTOMAGIC_SETTINGS_H_