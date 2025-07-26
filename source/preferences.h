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

#ifndef RME_PREFERENCES_WINDOW_H_
#define RME_PREFERENCES_WINDOW_H_

#include "main.h"
#include <wx/listbook.h>
#include <wx/collpane.h>
#include <wx/clrpicker.h>
#include "gui.h"

class PreferencesWindow : public wxDialog {
public:
	explicit PreferencesWindow(wxWindow* parent) :
		PreferencesWindow(parent, false) {};
	PreferencesWindow(wxWindow* parent, bool clientVersionSelected);
	virtual ~PreferencesWindow();

	void OnClickDefaults(wxCommandEvent&);
	void OnClickApply(wxCommandEvent&);
	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);

	void OnForceReloadRevScripts(wxCommandEvent&);

	void OnCollapsiblePane(wxCollapsiblePaneEvent&);

	void UpdateClientOverride();

protected:
	void SetDefaults();
	void Apply();
	void SaveValues();

	wxBookCtrl* book;

	// General
	wxCheckBox* always_make_backup_chkbox;
	wxCheckBox* create_on_startup_chkbox;
	wxCheckBox* update_check_on_startup_chkbox;
	wxCheckBox* only_one_instance_chkbox;
	wxCheckBox* show_welcome_dialog_chkbox;
	wxCheckBox* enable_tileset_editing_chkbox;
	wxSpinCtrl* undo_size_spin;
	wxSpinCtrl* undo_mem_size_spin;
	wxSpinCtrl* worker_threads_spin;
	wxSpinCtrl* replace_size_spin;
	wxRadioBox* position_format;

	// Editor
	wxCheckBox* group_actions_chkbox;
	wxCheckBox* duplicate_id_warn_chkbox;
	wxCheckBox* house_remove_chkbox;
	wxCheckBox* auto_assign_doors_chkbox;
	wxCheckBox* auto_assign_depot_to_closest_temple_chkbox;
	wxCheckBox* eraser_leave_unique_chkbox;
	wxCheckBox* doodad_erase_same_chkbox;
	wxCheckBox* auto_create_spawn_chkbox;
	wxCheckBox* allow_multiple_orderitems_chkbox;
	wxCheckBox* merge_move_chkbox;
	wxCheckBox* merge_paste_chkbox;
	wxSpinCtrl* refresh_radius_spin;

	// Graphics
	wxCheckBox* icon_selection_shadow_chkbox;
	wxChoice* icon_background_choice;
	wxCheckBox* use_memcached_chkbox;
	wxDirPickerCtrl* screenshot_directory_picker;
	wxDirPickerCtrl* revscript_directory_picker;
	wxButton* force_reload_revscripts_btn;
	wxChoice* screenshot_format_choice;
	wxCheckBox* hide_items_when_zoomed_chkbox;
	wxColourPickerCtrl* cursor_color_pick;
	wxColourPickerCtrl* cursor_alt_color_pick;
	wxCheckBox* dark_mode_chkbox;
	wxCheckBox* dark_mode_color_enabled_chkbox;
	wxColourPickerCtrl* dark_mode_color_pick;
	/*
	wxCheckBox* texture_managment_chkbox;
	wxSpinCtrl* clean_interval_spin;
	wxSpinCtrl* texture_longevity_spin;
	wxSpinCtrl* texture_threshold_spin;
	wxSpinCtrl* software_threshold_spin;
	wxSpinCtrl* software_clean_amount_spin;
	*/

	// Automagic
	wxCheckBox* automagic_enabled_chkbox;
	wxCheckBox* same_ground_type_chkbox;
	wxCheckBox* walls_repel_borders_chkbox;
	wxCheckBox* layer_carpets_chkbox;
	wxCheckBox* borderize_delete_chkbox;
	wxCheckBox* borderize_paste_chkbox;
	wxCheckBox* borderize_drag_chkbox;
	wxSpinCtrl* borderize_drag_threshold_spin;
	wxSpinCtrl* borderize_paste_threshold_spin;

	wxCheckBox* custom_border_checkbox;
	wxStaticText* custom_border_id_label;
	wxSpinCtrl* custom_border_id_spin;

	// Interface
	wxChoice* terrain_palette_style_choice;
	wxChoice* collection_palette_style_choice;
	wxChoice* doodad_palette_style_choice;
	wxChoice* item_palette_style_choice;
	wxChoice* raw_palette_style_choice;

	wxCheckBox* large_terrain_tools_chkbox;
	wxCheckBox* large_collection_tools_chkbox;
	wxCheckBox* large_doodad_sizebar_chkbox;
	wxCheckBox* large_item_sizebar_chkbox;
	wxCheckBox* large_house_sizebar_chkbox;
	wxCheckBox* large_raw_sizebar_chkbox;
	wxCheckBox* large_container_icons_chkbox;
	wxCheckBox* large_pick_item_icons_chkbox;

	wxCheckBox* switch_mousebtn_chkbox;
	wxCheckBox* doubleclick_properties_chkbox;
	wxCheckBox* inversed_scroll_chkbox;
	wxSlider* scroll_speed_slider;
	wxSlider* zoom_speed_slider;



	// Client info
	wxChoice* default_version_choice;
	std::vector<wxDirPickerCtrl*> version_dir_pickers;
	wxCheckBox* check_sigs_chkbox;
	wxCheckBox* force_client_otb_chkbox;

	// Create controls
	wxChoice* AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting);
	void SetPaletteStyleChoice(wxChoice* ctrl, int key);

	// Create windows
	wxNotebookPage* CreateGeneralPage();
	wxNotebookPage* CreateGraphicsPage();
	wxNotebookPage* CreateUIPage();
	wxNotebookPage* CreateEditorPage();
	wxNotebookPage* CreateClientPage();
	wxNotebookPage* CreateLODPage();
	wxNotebookPage* CreateAutomagicPage();
	wxNotebookPage* CreateTooltipInfoPage();
	wxNotebookPage* CreateInvisibleItemsPage();

	// Helper method to update the UI state of dark mode controls
	void UpdateDarkModeUI();
	
	// Helper method to update the UI state of invisible items controls
	void UpdateInvisibleItemsUI();

	// Helper method to update the UI state of client box controls
	void UpdateClientBoxUI();

	// Add with other checkbox declarations
	wxCheckBox* auto_select_raw_chkbox;
	wxCheckBox* show_map_warnings_chkbox;
	wxCheckBox* autosave_chkbox;
	wxSpinCtrl* autosave_interval_spin;
	
	// Invisible Items Color controls
	wxCheckBox* invisible_items_custom_chkbox;
	wxColourPickerCtrl* invisible_invalid_color_pick;
	wxColourPickerCtrl* invisible_stairs_color_pick;
	wxColourPickerCtrl* invisible_walkable_color_pick;
	wxColourPickerCtrl* invisible_wall_color_pick;
	wxTextCtrl* invisible_custom_ids_textctrl;
	
	// Client Box Size controls
	wxCheckBox* ingame_box_custom_size_chkbox;
	wxSpinCtrl* ingame_box_width_spin;
	wxSpinCtrl* ingame_box_height_spin;
	wxSpinCtrl* ingame_box_offset_x_spin;
	wxSpinCtrl* ingame_box_offset_y_spin;
	
	// LOD settings
	wxSpinCtrl* tooltip_max_zoom_spin;
	wxSpinCtrl* ground_only_threshold_spin;
	wxSpinCtrl* item_display_threshold_spin;
	wxSpinCtrl* special_features_threshold_spin;
	wxSpinCtrl* animation_threshold_spin;
	wxSpinCtrl* effects_threshold_spin;
	wxSpinCtrl* light_threshold_spin;
	wxSpinCtrl* shade_threshold_spin;
	wxSpinCtrl* town_zone_threshold_spin;
	wxSpinCtrl* grid_threshold_spin;

	// Palette grid settings
	wxSpinCtrl* chunk_size_spin;
	wxSpinCtrl* visible_rows_margin_spin;

	// Tooltip Info
	wxTextCtrl* tooltip_ignore_ids_textctrl;
	wxCheckBox* tooltip_enable_chkbox;
	wxCheckBox* tooltip_show_hasscript_chkbox;
	wxCheckBox* tooltip_show_text_chkbox;
	wxCheckBox* tooltip_show_itemid_chkbox;
	wxCheckBox* tooltip_show_aid_chkbox;
	wxCheckBox* tooltip_show_uid_chkbox;
	wxCheckBox* tooltip_show_doorid_chkbox;
	wxCheckBox* tooltip_show_destination_chkbox;
	wxCheckBox* tooltip_show_houseid_chkbox;
	wxCheckBox* house_custom_colors_chkbox;

	// Map warning suppression
	wxCheckBox* suppress_map_warnings_chkbox;

	DECLARE_EVENT_TABLE()
};

#endif
