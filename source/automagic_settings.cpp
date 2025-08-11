//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "automagic_settings.h"
#include "settings.h"
#include "gui.h"

BEGIN_EVENT_TABLE(AutomagicSettingsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AutomagicSettingsDialog::OnClickOK)
    EVT_BUTTON(wxID_CANCEL, AutomagicSettingsDialog::OnClickCancel)
    EVT_CHECKBOX(wxID_ANY, AutomagicSettingsDialog::OnAutomagicCheck)
    EVT_CLOSE(AutomagicSettingsDialog::OnClose)
END_EVENT_TABLE()

AutomagicSettingsDialog::AutomagicSettingsDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Automagic Settings", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    // Create main sizer
    wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);
    
    // Create a static box for the settings
    wxStaticBoxSizer* settings_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Border Settings");
    
    // Create checkboxes
    automagic_enabled_checkbox = newd wxCheckBox(this, wxID_ANY, "Enable Automagic");
    automagic_enabled_checkbox->SetValue(g_settings.getBoolean(Config::USE_AUTOMAGIC));
    automagic_enabled_checkbox->SetToolTip("Automatically apply borders and wall connections when editing");
    settings_sizer->Add(automagic_enabled_checkbox, 0, wxALL, 5);
    
    same_ground_type_checkbox = newd wxCheckBox(this, wxID_ANY, "Same Ground Type Border");
    same_ground_type_checkbox->SetValue(g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER));
    same_ground_type_checkbox->SetToolTip("Preserve existing borders and only apply borders for the current ground type");
    same_ground_type_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    settings_sizer->Add(same_ground_type_checkbox, 0, wxALL, 5);
    
    walls_repel_borders_checkbox = newd wxCheckBox(this, wxID_ANY, "Walls Repel Borders");
    walls_repel_borders_checkbox->SetValue(g_settings.getBoolean(Config::WALLS_REPEL_BORDERS));
    walls_repel_borders_checkbox->SetToolTip("When enabled, walls will block border generation, preventing borders from crossing through walls");
    walls_repel_borders_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    settings_sizer->Add(walls_repel_borders_checkbox, 0, wxALL, 5);
    
    layer_carpets_checkbox = newd wxCheckBox(this, wxID_ANY, "Layer Carpets");
    layer_carpets_checkbox->SetValue(g_settings.getBoolean(Config::LAYER_CARPETS));
    layer_carpets_checkbox->SetToolTip("When enabled, carpet brushes will be placed on top of existing carpets instead of replacing them");
    layer_carpets_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    settings_sizer->Add(layer_carpets_checkbox, 0, wxALL, 5);
    
    force_roof_outer_borders_checkbox = newd wxCheckBox(this, wxID_ANY, "Force Roof Outer Borders");
    force_roof_outer_borders_checkbox->SetValue(g_settings.getBoolean(Config::FORCE_ROOF_OUTER_BORDERS));
    force_roof_outer_borders_checkbox->SetToolTip("When enabled, autoborders will force place outer borders for roofs in empty spaces, solving issues where roofs don't get proper borders when working with their groundID");
    force_roof_outer_borders_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    settings_sizer->Add(force_roof_outer_borders_checkbox, 0, wxALL, 5);
    
    borderize_delete_checkbox = newd wxCheckBox(this, wxID_ANY, "Borderize on Delete");
    borderize_delete_checkbox->SetValue(g_settings.getBoolean(Config::BORDERIZE_DELETE));
    borderize_delete_checkbox->SetToolTip("When enabled, deleting items will trigger automatic bordering of surrounding tiles");
    borderize_delete_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    settings_sizer->Add(borderize_delete_checkbox, 0, wxALL, 5);
    
    // Create custom border controls
    wxBoxSizer* custom_border_sizer = newd wxBoxSizer(wxHORIZONTAL);
    
    custom_border_checkbox = newd wxCheckBox(this, wxID_ANY, "Use Custom Border");
    custom_border_checkbox->SetValue(g_settings.getBoolean(Config::CUSTOM_BORDER_ENABLED));
    custom_border_checkbox->SetToolTip("Override automatic border selection with a specific border ID");
    custom_border_checkbox->Enable(automagic_enabled_checkbox->GetValue());
    custom_border_sizer->Add(custom_border_checkbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    custom_border_id_label = newd wxStaticText(this, wxID_ANY, "Border ID:");
    custom_border_id_label->Enable(custom_border_checkbox->GetValue() && automagic_enabled_checkbox->GetValue());
    custom_border_sizer->Add(custom_border_id_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    custom_border_id_field = newd wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    custom_border_id_field->SetRange(1, 65535);
    custom_border_id_field->SetValue(g_settings.getInteger(Config::CUSTOM_BORDER_ID) > 0 ? 
                                     g_settings.getInteger(Config::CUSTOM_BORDER_ID) : 1);
    custom_border_id_field->Enable(custom_border_checkbox->GetValue() && automagic_enabled_checkbox->GetValue());
    custom_border_id_field->SetToolTip("The ID of the border to use when drawing any tile");
    custom_border_sizer->Add(custom_border_id_field, 0, wxALIGN_CENTER_VERTICAL);
    
    settings_sizer->Add(custom_border_sizer, 0, wxALL, 5);
    
    // Add description text
    wxStaticText* description = newd wxStaticText(this, wxID_ANY, 
        "When 'Same Ground Type Border' is enabled, the editor will:\n"
        "- Preserve existing borders on tiles\n"
        "- Only apply borders for the current ground type\n"
        "- Respect Z-axis positioning of existing borders\n"
        "- Allow multiple border layering\n\n"
        "When 'Walls Repel Borders' is enabled, the editor will:\n"
        "- Prevent borders from crossing through walls\n"
        "- Treat walls as barriers for border generation\n"
        "- Preserve the structure of buildings and houses\n\n"
        "When 'Layer Carpets' is enabled, the editor will:\n"
        "- Place new carpets on top of existing carpets\n"
        "- Allow creating multi-layered carpet designs\n\n"
        "When 'Force Roof Outer Borders' is enabled, the editor will:\n"
        "- Force place outer borders for roofs in empty spaces\n"
        "- Solve issues where roofs don't get proper borders\n"
        "- Ensure consistent roof border placement\n\n"
        "When 'Use Custom Border' is enabled, the editor will:\n"
        "- Override automatic border selection with the specified border ID\n"
        "- Apply the same border pattern around any tile you draw\n"
        "- Ignore terrain type border transitions");
    settings_sizer->Add(description, 0, wxALL, 5);
    
    main_sizer->Add(settings_sizer, 0, wxEXPAND | wxALL, 10);
    
    // Create buttons
    buttons_sizer = newd wxStdDialogButtonSizer();
    ok_button = newd wxButton(this, wxID_OK, "OK");
    cancel_button = newd wxButton(this, wxID_CANCEL, "Cancel");
    
    buttons_sizer->AddButton(ok_button);
    buttons_sizer->AddButton(cancel_button);
    buttons_sizer->Realize();
    
    main_sizer->Add(buttons_sizer, 0, wxALIGN_CENTER | wxALL, 10);
    
    // Set sizer and fit
    SetSizer(main_sizer);
    Fit();
    Centre();
}

AutomagicSettingsDialog::~AutomagicSettingsDialog() {
    // Nothing to clean up
}

bool AutomagicSettingsDialog::IsAutomagicEnabled() const {
    return automagic_enabled_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsSameGroundTypeBorderEnabled() const {
    return same_ground_type_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsWallsRepelBordersEnabled() const {
    return walls_repel_borders_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsLayerCarpetsEnabled() const {
    return layer_carpets_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsForceRoofOuterBordersEnabled() const {
    return force_roof_outer_borders_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsBorderizeDeleteEnabled() const {
    return borderize_delete_checkbox->GetValue();
}

bool AutomagicSettingsDialog::IsCustomBorderEnabled() const {
    return custom_border_checkbox->GetValue();
}

int AutomagicSettingsDialog::GetCustomBorderId() const {
    return custom_border_id_field->GetValue();
}

void AutomagicSettingsDialog::OnClickOK(wxCommandEvent& event) {
    // Save settings
    g_settings.setInteger(Config::USE_AUTOMAGIC, IsAutomagicEnabled() ? 1 : 0);
    g_settings.setInteger(Config::BORDER_IS_GROUND, IsAutomagicEnabled() ? 1 : 0);
    g_settings.setInteger(Config::SAME_GROUND_TYPE_BORDER, IsSameGroundTypeBorderEnabled() ? 1 : 0);
    g_settings.setInteger(Config::WALLS_REPEL_BORDERS, IsWallsRepelBordersEnabled() ? 1 : 0);
    g_settings.setInteger(Config::LAYER_CARPETS, IsLayerCarpetsEnabled() ? 1 : 0);
    g_settings.setInteger(Config::FORCE_ROOF_OUTER_BORDERS, IsForceRoofOuterBordersEnabled() ? 1 : 0);
    g_settings.setInteger(Config::BORDERIZE_DELETE, IsBorderizeDeleteEnabled() ? 1 : 0);

	
    
    // Save custom border settings
    g_settings.setInteger(Config::CUSTOM_BORDER_ENABLED, IsCustomBorderEnabled() ? 1 : 0);
    g_settings.setInteger(Config::CUSTOM_BORDER_ID, GetCustomBorderId());
    
    // Update status text
    if (IsAutomagicEnabled()) {
        g_gui.SetStatusText("Automagic enabled.");
    } else {
        g_gui.SetStatusText("Automagic disabled.");
    }
    
    EndModal(wxID_OK);
}

void AutomagicSettingsDialog::OnClickCancel(wxCommandEvent& event) {
    // Don't save settings
    EndModal(wxID_CANCEL);
}

void AutomagicSettingsDialog::OnAutomagicCheck(wxCommandEvent& event) {
    // Get the checkbox that triggered the event
    wxCheckBox* checkbox = dynamic_cast<wxCheckBox*>(event.GetEventObject());
    
    // Update all checkbox states based on automagic checkbox
    bool automagic_enabled = automagic_enabled_checkbox->GetValue();
    
    // If this is the automagic checkbox, update all dependent checkboxes
    if (checkbox == automagic_enabled_checkbox) {
        same_ground_type_checkbox->Enable(automagic_enabled);
        walls_repel_borders_checkbox->Enable(automagic_enabled);
        layer_carpets_checkbox->Enable(automagic_enabled);
        force_roof_outer_borders_checkbox->Enable(automagic_enabled);
        borderize_delete_checkbox->Enable(automagic_enabled);
        custom_border_checkbox->Enable(automagic_enabled);
    }
    
    // Always update the custom border fields based on both checkboxes
    bool custom_border_enabled = custom_border_checkbox->GetValue() && automagic_enabled;
    custom_border_id_label->Enable(custom_border_enabled);
    custom_border_id_field->Enable(custom_border_enabled);
}

void AutomagicSettingsDialog::OnClose(wxCloseEvent& event) {
    // Same as cancel
    EndModal(wxID_CANCEL);
} 
