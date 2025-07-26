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

#include <wx/collpane.h>

#include "settings.h"
#include "gui_ids.h"
#include "client_version.h"

#include <wx/confbase.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/statline.h>

#include <iostream>
#include <string>

#include "editor.h"

#include "gui.h"

#include "preferences.h"
#include "main_menubar.h"
#include "main_toolbar.h"
#include "dark_mode_manager.h"
#include "map_drawer.h"

BEGIN_EVENT_TABLE(PreferencesWindow, wxDialog)
EVT_BUTTON(wxID_OK, PreferencesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, PreferencesWindow::OnClickCancel)
EVT_BUTTON(wxID_APPLY, PreferencesWindow::OnClickApply)
EVT_BUTTON(wxID_ANY, PreferencesWindow::OnForceReloadRevScripts)
EVT_COLLAPSIBLEPANE_CHANGED(wxID_ANY, PreferencesWindow::OnCollapsiblePane)
END_EVENT_TABLE()

PreferencesWindow::PreferencesWindow(wxWindow* parent, bool clientVersionSelected = false) :
	wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxSize(400, 400), wxCAPTION | wxCLOSE_BOX) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	book = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_TOP);
	// book->SetPadding(4);

	book->AddPage(CreateGeneralPage(), "General", true);
	book->AddPage(CreateEditorPage(), "Editor");
	book->AddPage(CreateGraphicsPage(), "Graphics");
	book->AddPage(CreateUIPage(), "Interface");
	book->AddPage(CreateClientPage(), "Client Version", clientVersionSelected);
	book->AddPage(CreateLODPage(), "LOD");
	book->AddPage(CreateAutomagicPage(), "Automagic");
	book->AddPage(CreateTooltipInfoPage(), "Tooltip Info");
	book->AddPage(CreateInvisibleItemsPage(), "Invisible Items");

	sizer->Add(book, 1, wxEXPAND | wxALL, 10);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Border(wxALL, 5).Left().Center());
	subsizer->Add(newd wxButton(this, wxID_APPLY, "Apply"), wxSizerFlags(1).Center());
	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 10);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	// FindWindowById(PANE_ADVANCED_GRAPHICS, this)->GetParent()->Fit();
}

PreferencesWindow::~PreferencesWindow() {
	////
}

wxNotebookPage* PreferencesWindow::CreateGeneralPage() {
	wxNotebookPage* general_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	show_welcome_dialog_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 5);

	always_make_backup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 5);

	update_check_on_startup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Check for updates on startup");
	update_check_on_startup_chkbox->SetValue(g_settings.getInteger(Config::USE_UPDATER) == 1);
	sizer->Add(update_check_on_startup_chkbox, 0, wxLEFT | wxTOP, 5);

	only_one_instance_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.\nTo run multiple instances regardless of this setting, use the RunMultipleInstances.bat file or -force-multi-instance parameter.");
	sizer->Add(only_one_instance_chkbox, 0, wxLEFT | wxTOP, 5);

	enable_tileset_editing_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	sizer->Add(enable_tileset_editing_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_select_raw_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Auto-select RAW on right-click");
	auto_select_raw_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_SELECT_RAW_ON_RIGHTCLICK));
	auto_select_raw_chkbox->SetToolTip("Automatically selects RAW brush when right-clicking items while showing the context menu.");
	sizer->Add(auto_select_raw_chkbox, 0, wxLEFT | wxTOP, 5);

	show_map_warnings_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show map loader warnings");
	show_map_warnings_chkbox->SetValue(!g_settings.getBoolean(Config::SUPPRESS_MAP_WARNINGS));
	show_map_warnings_chkbox->SetToolTip("Show warnings dialog when loading maps (can be disabled with 'Don't show again' checkbox in the warnings dialog).");
	sizer->Add(show_map_warnings_chkbox, 0, wxLEFT | wxTOP, 5);

	// Add suppress map warnings checkbox (redundant, for explicit control)
	suppress_map_warnings_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Never show map loader warnings");
	suppress_map_warnings_chkbox->SetValue(g_settings.getBoolean(Config::SUPPRESS_MAP_WARNINGS));
	suppress_map_warnings_chkbox->SetToolTip("If checked, map loader warnings will never be shown, even if there are errors or warnings during map load.");
	sizer->Add(suppress_map_warnings_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	// Add autosave options
	wxBoxSizer* autosave_sizer = newd wxBoxSizer(wxHORIZONTAL); 
	autosave_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable autosave");
	autosave_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_SAVE_ENABLED));
	autosave_chkbox->SetToolTip("Automatically save a backup of your map periodically");
	autosave_sizer->Add(autosave_chkbox, 0, wxALL, 5);

	autosave_interval_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::AUTO_SAVE_INTERVAL)), 
		wxDefaultPosition, wxSize(120, -1), wxSP_ARROW_KEYS, 1, 7200, 60);
	autosave_interval_spin->SetToolTip("How often (in seconds) should autosave occur");
	autosave_sizer->Add(autosave_interval_spin, 0, wxALL, 5);
	autosave_sizer->Add(newd wxStaticText(general_page, wxID_ANY, "seconds"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

	sizer->Add(autosave_sizer);

	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo queue size: "), 0);
	undo_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x10000000);
	grid_sizer->Add(undo_size_spin, 0);
	SetWindowToolTip(tmptext, undo_size_spin, "How many action you can undo, be aware that a high value will increase memory usage.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo maximum memory size (MB): "), 0);
	undo_mem_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_MEM_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 4096);
	grid_sizer->Add(undo_mem_size_spin, 0);
	SetWindowToolTip(tmptext, undo_mem_size_spin, "The approximite limit for the memory usage of the undo queue.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Worker Threads: "), 0);
	worker_threads_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::WORKER_THREADS)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64);
	grid_sizer->Add(worker_threads_spin, 0);
	SetWindowToolTip(tmptext, worker_threads_spin, "How many threads the editor will use for intensive operations. This should be equivalent to the amount of logical processors in your system.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Replace count: "), 0);
	replace_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::REPLACE_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000);
	grid_sizer->Add(replace_size_spin, 0);
	SetWindowToolTip(tmptext, replace_size_spin, "How many items you can replace on the map using the Replace Item tool.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "OTS Data directory: "), 0);
	revscript_directory_picker = newd wxDirPickerCtrl(general_page, wxID_ANY);
	grid_sizer->Add(revscript_directory_picker, 1, wxEXPAND);
	wxString revscript_dir = wxstr(g_settings.getString(Config::OTS_DATA_DIRECTORY));
	revscript_directory_picker->SetPath(revscript_dir);
	SetWindowToolTip(tmptext, revscript_directory_picker, "Directory containing OTS data files (/data/ folder with scripts, actions, movements, etc.).");

	grid_sizer->Add(newd wxStaticText(general_page, wxID_ANY, ""), 0); // Empty cell for alignment
	force_reload_revscripts_btn = newd wxButton(general_page, wxID_ANY, "Force Reload Scripts");
	grid_sizer->Add(force_reload_revscripts_btn, 0);
	SetWindowToolTip(force_reload_revscripts_btn, "Force reload all script files from the selected OTS data directory.");


	sizer->Add(grid_sizer, 0, wxALL, 5);
	sizer->AddSpacer(10);

	wxString position_choices[] = { "  {x = 0, y = 0, z = 0}",
									R"(  {"x":0,"y":0,"z":0})",
									"  x, y, z",
									"  (x, y, z)",
									"  Position(x, y, z)" };
	int radio_choices = sizeof(position_choices) / sizeof(wxString);
	position_format = newd wxRadioBox(general_page, wxID_ANY, "Copy Position Format", wxDefaultPosition, wxDefaultSize, radio_choices, position_choices, 1, wxRA_SPECIFY_COLS);
	position_format->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	sizer->Add(position_format, 0, wxALL | wxEXPAND, 5);
	SetWindowToolTip(tmptext, position_format, "The position format when copying from the map.");

	general_page->SetSizerAndFit(sizer);

	return general_page;
}

wxNotebookPage* PreferencesWindow::CreateEditorPage() {
	wxNotebookPage* editor_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	group_actions_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Group same-type actions");
	group_actions_chkbox->SetValue(g_settings.getBoolean(Config::GROUP_ACTIONS));
	group_actions_chkbox->SetToolTip("This will group actions of the same type (drawing, selection..) when several take place in consecutive order.");
	sizer->Add(group_actions_chkbox, 0, wxLEFT | wxTOP, 5);

	duplicate_id_warn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Warn for duplicate IDs");
	duplicate_id_warn_chkbox->SetValue(g_settings.getBoolean(Config::WARN_FOR_DUPLICATE_ID));
	duplicate_id_warn_chkbox->SetToolTip("Warns for most kinds of duplicate IDs.");
	sizer->Add(duplicate_id_warn_chkbox, 0, wxLEFT | wxTOP, 5);

	house_remove_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "House brush removes items");
	house_remove_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_BRUSH_REMOVE_ITEMS));
	house_remove_chkbox->SetToolTip("When this option is checked, the house brush will automaticly remove items that will respawn every time the map is loaded.");
	sizer->Add(house_remove_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_assign_doors_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto-assign door ids");
	auto_assign_doors_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DOORID));
	auto_assign_doors_chkbox->SetToolTip("This will auto-assign unique door ids to all doors placed with the door brush (or doors painted over with the house brush).\nDoes NOT affect doors placed using the RAW palette.");
	sizer->Add(auto_assign_doors_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_assign_depot_to_closest_temple_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto-assign depot to closest temple");
	auto_assign_depot_to_closest_temple_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE));
	auto_assign_depot_to_closest_temple_chkbox->SetToolTip("Automatically assign depot town ID to the closest temple when placing depot items. This makes depot mapping more seamless by calculating the distance to all temples and setting the depot ID accordingly.");
	sizer->Add(auto_assign_depot_to_closest_temple_chkbox, 0, wxLEFT | wxTOP, 5);

	doodad_erase_same_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Doodad brush only erases same");
	doodad_erase_same_chkbox->SetValue(g_settings.getBoolean(Config::DOODAD_BRUSH_ERASE_LIKE));
	doodad_erase_same_chkbox->SetToolTip("The doodad brush will only erase items that belongs to the current brush.");
	sizer->Add(doodad_erase_same_chkbox, 0, wxLEFT | wxTOP, 5);

	eraser_leave_unique_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Eraser leaves unique items");
	eraser_leave_unique_chkbox->SetValue(g_settings.getBoolean(Config::ERASER_LEAVE_UNIQUE));
	eraser_leave_unique_chkbox->SetToolTip("The eraser will leave containers with items in them, items with unique or action id and items.");
	sizer->Add(eraser_leave_unique_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_create_spawn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto create spawn when placing creature");
	auto_create_spawn_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_CREATE_SPAWN));
	auto_create_spawn_chkbox->SetToolTip("When this option is checked, you can place creatures without placing a spawn manually, the spawn will be place automatically.");
	sizer->Add(auto_create_spawn_chkbox, 0, wxLEFT | wxTOP, 5);

	allow_multiple_orderitems_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Prevent toporder conflict");
	allow_multiple_orderitems_chkbox->SetValue(g_settings.getBoolean(Config::RAW_LIKE_SIMONE));
	allow_multiple_orderitems_chkbox->SetToolTip("When this option is checked, you can not place several items with the same toporder on one tile using a RAW Brush.");
	sizer->Add(allow_multiple_orderitems_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	merge_move_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge move");
	merge_move_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_MOVE));
	merge_move_chkbox->SetToolTip("Moved tiles won't replace already placed tiles.");
	sizer->Add(merge_move_chkbox, 0, wxLEFT | wxTOP, 5);

	merge_paste_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge paste");
	merge_paste_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_PASTE));
	merge_paste_chkbox->SetToolTip("Pasted tiles won't replace already placed tiles.");
	sizer->Add(merge_paste_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);
	
	// Add refresh radius control
	wxFlexGridSizer* refresh_sizer = newd wxFlexGridSizer(2, 10, 10);
	refresh_sizer->AddGrowableCol(1);
	
	wxStaticText* refresh_radius_label = newd wxStaticText(editor_page, wxID_ANY, "Multiplayer refresh radius: ");
	refresh_sizer->Add(refresh_radius_label, 0, wxALIGN_CENTER_VERTICAL);
	
	refresh_radius_spin = newd wxSpinCtrl(editor_page, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 50, g_settings.getInteger(Config::REFRESH_RADIUS));
	refresh_radius_spin->SetToolTip("The radius (in nodes) to refresh when using the 'Refresh Visible Area' feature in multiplayer mode.");
	refresh_sizer->Add(refresh_radius_spin, 0);
	
	sizer->Add(refresh_sizer, 0, wxLEFT | wxTOP | wxEXPAND, 5);

	editor_page->SetSizerAndFit(sizer);

	return editor_page;
}

wxNotebookPage* PreferencesWindow::CreateGraphicsPage() {
	wxWindow* tmp;
	wxNotebookPage* graphics_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	hide_items_when_zoomed_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Hide items when zoomed out");
	hide_items_when_zoomed_chkbox->SetValue(g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED));
	hide_items_when_zoomed_chkbox->SetToolTip("Hides items when zooming out too far.");
	sizer->Add(hide_items_when_zoomed_chkbox, 0, wxLEFT | wxTOP, 5);

	icon_selection_shadow_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use icon selection shadow");
	icon_selection_shadow_chkbox->SetValue(g_settings.getBoolean(Config::USE_GUI_SELECTION_SHADOW));
	icon_selection_shadow_chkbox->SetToolTip("When this option is enabled, a darker shadow will be used for selection highlights (for icon-based palettes).");
	sizer->Add(icon_selection_shadow_chkbox, 0, wxLEFT | wxTOP, 5);

	use_memcached_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Cache sprites in memory");
	use_memcached_chkbox->SetValue(g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES_TO_SAVE));
	use_memcached_chkbox->SetToolTip("Uncheck this to conserve memory.");
	sizer->Add(use_memcached_chkbox, 0, wxLEFT | wxTOP, 5);

	dark_mode_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use dark mode");
	dark_mode_chkbox->SetValue(g_settings.getBoolean(Config::DARK_MODE));
	dark_mode_chkbox->SetToolTip("Enable dark mode for the application interface.");
	sizer->Add(dark_mode_chkbox, 0, wxLEFT | wxTOP, 5);

	// Add dark mode color picker
	wxBoxSizer* dark_mode_color_sizer = newd wxBoxSizer(wxHORIZONTAL);
	dark_mode_color_sizer->Add(newd wxStaticText(graphics_page, wxID_ANY, "Custom dark mode color: "), 0, wxALIGN_CENTER_VERTICAL);
	dark_mode_color_enabled_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Enable");
	dark_mode_color_enabled_chkbox->SetValue(g_settings.getBoolean(Config::DARK_MODE_CUSTOM_COLOR));
	dark_mode_color_enabled_chkbox->SetToolTip("Use a custom color for dark mode instead of the default dark color.");
	dark_mode_color_sizer->Add(dark_mode_color_enabled_chkbox, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	
	wxColor current_dark_color = wxColor(
		g_settings.getInteger(Config::DARK_MODE_RED),
		g_settings.getInteger(Config::DARK_MODE_GREEN),
		g_settings.getInteger(Config::DARK_MODE_BLUE)
	);
	dark_mode_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, current_dark_color);
	dark_mode_color_pick->SetToolTip("Select custom color for dark mode. This will be used when both dark mode and custom color are enabled.");
	dark_mode_color_sizer->Add(dark_mode_color_pick, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
	sizer->Add(dark_mode_color_sizer, 0, wxLEFT | wxTOP, 5);

	// Connect events to update UI state
	dark_mode_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		UpdateDarkModeUI();
	});
	
	dark_mode_color_enabled_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		UpdateDarkModeUI();
	});
	
	// Initial update of UI state
	UpdateDarkModeUI();

	sizer->AddSpacer(10);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	// Icon background color
	icon_background_choice = newd wxChoice(graphics_page, wxID_ANY);
	icon_background_choice->Append("Black background");
	icon_background_choice->Append("Gray background");
	icon_background_choice->Append("White background");
	if (g_settings.getInteger(Config::ICON_BACKGROUND) == 255) {
		icon_background_choice->SetSelection(2);
	} else if (g_settings.getInteger(Config::ICON_BACKGROUND) == 88) {
		icon_background_choice->SetSelection(1);
	} else {
		icon_background_choice->SetSelection(0);
	}

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Icon background color: "), 0);
	subsizer->Add(icon_background_choice, 0);
	SetWindowToolTip(icon_background_choice, tmp, "This will change the background color on icons in all windows.");

	// Cursor colors
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Cursor color: "), 0);
	subsizer->Add(cursor_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE), g_settings.getInteger(Config::CURSOR_ALPHA))), 0);
	SetWindowToolTip(icon_background_choice, tmp, "The color of the main cursor on the map (while in drawing mode).");

	// Alternate cursor color
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Secondary cursor color: "), 0);
	subsizer->Add(cursor_alt_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE), g_settings.getInteger(Config::CURSOR_ALT_ALPHA))), 0);
	SetWindowToolTip(icon_background_choice, tmp, "The color of the secondary cursor on the map (for houses and flags).");

	// Screenshot dir
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot directory: "), 0);
	screenshot_directory_picker = newd wxDirPickerCtrl(graphics_page, wxID_ANY);
	subsizer->Add(screenshot_directory_picker, 1, wxEXPAND);
	wxString ss = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	screenshot_directory_picker->SetPath(ss);
	SetWindowToolTip(screenshot_directory_picker, "Screenshot taken in the editor will be saved to this directory.");

	// Screenshot format
	screenshot_format_choice = newd wxChoice(graphics_page, wxID_ANY);
	screenshot_format_choice->Append("PNG");
	screenshot_format_choice->Append("JPG");
	screenshot_format_choice->Append("TGA");
	screenshot_format_choice->Append("BMP");
	if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "png") {
		screenshot_format_choice->SetSelection(0);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "jpg") {
		screenshot_format_choice->SetSelection(1);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "tga") {
		screenshot_format_choice->SetSelection(2);
	} else if (g_settings.getString(Config::SCREENSHOT_FORMAT) == "bmp") {
		screenshot_format_choice->SetSelection(3);
	} else {
		screenshot_format_choice->SetSelection(0);
	}
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot format: "), 0);
	subsizer->Add(screenshot_format_choice, 0);
	SetWindowToolTip(screenshot_format_choice, tmp, "This will affect the screenshot format used by the editor.\nTo take a screenshot, press F11.");

	sizer->Add(subsizer, 1, wxEXPAND | wxALL, 5);

	// Advanced g_settings
	/*
	wxCollapsiblePane* pane = newd wxCollapsiblePane(graphics_page, PANE_ADVANCED_GRAPHICS, "Advanced g_settings");
	{
		wxSizer* pane_sizer = newd wxBoxSizer(wxVERTICAL);

		pane_sizer->Add(texture_managment_chkbox = newd wxCheckBox(pane->GetPane(), wxID_ANY, "Use texture managment"));
		if(g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
			texture_managment_chkbox->SetValue(true);
		}
		pane_sizer->AddSpacer(8);

		wxFlexGridSizer* pane_grid_sizer = newd wxFlexGridSizer(2, 10, 10);
		pane_grid_sizer->AddGrowableCol(1);

		pane_grid_sizer->Add(tmp = newd wxStaticText(pane->GetPane(), wxID_ANY, "Texture clean interval: "), 0);
		clean_interval_spin = newd wxSpinCtrl(pane->GetPane(), wxID_ANY, i2ws(g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x1000000);
		pane_grid_sizer->Add(clean_interval_spin, 0);
		SetWindowToolTip(clean_interval_spin, tmp, "This controls how often the editor tries to free hardware texture resources.");

		pane_grid_sizer->Add(tmp = newd wxStaticText(pane->GetPane(), wxID_ANY, "Texture longevity: "), 0);
		texture_longevity_spin = newd wxSpinCtrl(pane->GetPane(), wxID_ANY, i2ws(g_settings.getInteger(Config::TEXTURE_LONGEVITY)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x1000000);
		pane_grid_sizer->Add(texture_longevity_spin, 0);
		SetWindowToolTip(texture_longevity_spin, tmp, "This controls for how long (in seconds) that the editor will keep textures in memory before it cleans them up.");

		pane_grid_sizer->Add(tmp = newd wxStaticText(pane->GetPane(), wxID_ANY, "Texture clean threshold: "), 0);
		texture_threshold_spin = newd wxSpinCtrl(pane->GetPane(), wxID_ANY, i2ws(g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 0x1000000);
		pane_grid_sizer->Add(texture_threshold_spin, 0);
		SetWindowToolTip(texture_threshold_spin, tmp, "This controls how many textures the editor will hold in memory before it attempts to clean up old textures. However, an infinite amount MIGHT be loaded.");

		pane_grid_sizer->Add(tmp = newd wxStaticText(pane->GetPane(), wxID_ANY, "Software clean threshold: "), 0);
		software_threshold_spin = newd wxSpinCtrl(pane->GetPane(), wxID_ANY, i2ws(g_settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 0x1000000);
		pane_grid_sizer->Add(software_threshold_spin, 0);
		SetWindowToolTip(software_threshold_spin, tmp, "This controls how many GUI sprites (icons) the editor will hold in memory at the same time.");

		pane_grid_sizer->Add(tmp = newd wxStaticText(pane->GetPane(), wxID_ANY, "Software clean amount: "), 0);
		software_clean_amount_spin = newd wxSpinCtrl(pane->GetPane(), wxID_ANY, i2ws(g_settings.getInteger(Config::SOFTWARE_CLEAN_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 0x1000000);
		pane_grid_sizer->Add(software_clean_amount_spin, 0);
		SetWindowToolTip(software_clean_amount_spin, tmp, "How many sprites the editor will free at once when the limit is exceeded.");

		pane_sizer->Add(pane_grid_sizer, 0, wxEXPAND);

		pane->GetPane()->SetSizerAndFit(pane_sizer);

		pane->Collapse();
	}

	sizer->Add(pane, 0);
	*/

	sizer->AddSpacer(15);

	// Client Box Size Settings
	sizer->Add(newd wxStaticText(graphics_page, wxID_ANY, "Client Box Customization:"), 0, wxLEFT | wxTOP, 5);
	
	ingame_box_custom_size_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Enable custom client box size");
	ingame_box_custom_size_chkbox->SetValue(g_settings.getBoolean(Config::INGAME_BOX_CUSTOM_SIZE_ENABLED));
	ingame_box_custom_size_chkbox->SetToolTip("When enabled, use custom size and offsets for the client box instead of the default 17x13 area.");
	sizer->Add(ingame_box_custom_size_chkbox, 0, wxLEFT | wxTOP, 5);

	// Grid for client box controls
	auto* client_box_grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	client_box_grid_sizer->AddGrowableCol(1);

	// Box Width
	wxStaticText* box_width_label = newd wxStaticText(graphics_page, wxID_ANY, "Client box width:");
	client_box_grid_sizer->Add(box_width_label, 0, wxALIGN_CENTER_VERTICAL);
	
	ingame_box_width_spin = newd wxSpinCtrl(graphics_page, wxID_ANY, 
		i2ws(g_settings.getInteger(Config::INGAME_BOX_WIDTH)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 50);
	ingame_box_width_spin->SetToolTip("Width of the client box in tiles (default: 17)");
	client_box_grid_sizer->Add(ingame_box_width_spin, 0);

	// Box Height
	wxStaticText* box_height_label = newd wxStaticText(graphics_page, wxID_ANY, "Client box height:");
	client_box_grid_sizer->Add(box_height_label, 0, wxALIGN_CENTER_VERTICAL);
	
	ingame_box_height_spin = newd wxSpinCtrl(graphics_page, wxID_ANY, 
		i2ws(g_settings.getInteger(Config::INGAME_BOX_HEIGHT)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 50);
	ingame_box_height_spin->SetToolTip("Height of the client box in tiles (default: 13)");
	client_box_grid_sizer->Add(ingame_box_height_spin, 0);

	// Offset X
	wxStaticText* offset_x_label = newd wxStaticText(graphics_page, wxID_ANY, "X border offset:");
	client_box_grid_sizer->Add(offset_x_label, 0, wxALIGN_CENTER_VERTICAL);
	
	ingame_box_offset_x_spin = newd wxSpinCtrl(graphics_page, wxID_ANY, 
		i2ws(g_settings.getInteger(Config::INGAME_BOX_OFFSET_X)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10);
	ingame_box_offset_x_spin->SetToolTip("Horizontal offset for the client box position");
	client_box_grid_sizer->Add(ingame_box_offset_x_spin, 0);

	// Offset Y
	wxStaticText* offset_y_label = newd wxStaticText(graphics_page, wxID_ANY, "Y border offset:");
	client_box_grid_sizer->Add(offset_y_label, 0, wxALIGN_CENTER_VERTICAL);
	
	ingame_box_offset_y_spin = newd wxSpinCtrl(graphics_page, wxID_ANY, 
		i2ws(g_settings.getInteger(Config::INGAME_BOX_OFFSET_Y)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -10, 10);
	ingame_box_offset_y_spin->SetToolTip("Vertical offset for the client box position (default: 2)");
	client_box_grid_sizer->Add(ingame_box_offset_y_spin, 0);

	sizer->Add(client_box_grid_sizer, 0, wxEXPAND | wxALL, 6);

	// Connect events to update UI state
	ingame_box_custom_size_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		UpdateClientBoxUI();
	});
	
	// Initial update of UI state
	UpdateClientBoxUI();

	graphics_page->SetSizerAndFit(sizer);

	return graphics_page;
}

wxChoice* PreferencesWindow::AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting) {
	wxStaticText* text;
	sizer->Add(text = newd wxStaticText(parent, wxID_ANY, short_description), 0);

	wxChoice* choice = newd wxChoice(parent, wxID_ANY);
	sizer->Add(choice, 0);

	choice->Append("Large Icons");
	choice->Append("Small Icons");
	choice->Append("Listbox with Icons");
	
	// Add Direct Draw option for RAW palette only
	if (short_description.Contains("RAW")) {
		choice->Append("Direct Draw");
	}

	text->SetToolTip(description);
	choice->SetToolTip(description);

	if (setting == "large icons") {
		choice->SetSelection(0);
	} else if (setting == "small icons") {
		choice->SetSelection(1);
	} else if (setting == "listbox") {
		choice->SetSelection(2);
	} else if (setting == "direct draw") {
		choice->SetSelection(3);
	}

	return choice;
}

void PreferencesWindow::SetPaletteStyleChoice(wxChoice* ctrl, int key) {
	if (ctrl->GetSelection() == 0) {
		g_settings.setString(key, "large icons");
	} else if (ctrl->GetSelection() == 1) {
		g_settings.setString(key, "small icons");
	} else if (ctrl->GetSelection() == 2) {
		g_settings.setString(key, "listbox");
	} else if (ctrl->GetSelection() == 3 && ctrl->GetString(3) == "Direct Draw") {
		g_settings.setString(key, "direct draw");
	}
}

wxNotebookPage* PreferencesWindow::CreateUIPage() {
	wxNotebookPage* ui_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);
	terrain_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Terrain Palette Style:",
		"Configures the look of the terrain palette.",
		g_settings.getString(Config::PALETTE_TERRAIN_STYLE)
	);
	collection_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Collections Palette Style:",
		"Configures the look of the collections palette.",
		g_settings.getString(Config::PALETTE_COLLECTION_STYLE)
	);
	doodad_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Doodad Palette Style:",
		"Configures the look of the doodad palette.",
		g_settings.getString(Config::PALETTE_DOODAD_STYLE)
	);
	item_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Item Palette Style:",
		"Configures the look of the item palette.",
		g_settings.getString(Config::PALETTE_ITEM_STYLE)
	);
	raw_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"RAW Palette Style:",
		"Configures the look of the raw palette.",
		g_settings.getString(Config::PALETTE_RAW_STYLE)
	);

	sizer->Add(subsizer, 0, wxALL, 6);

	sizer->AddSpacer(10);

	large_terrain_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large terrain palette tool && size icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	sizer->Add(large_terrain_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_collection_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large collections palette tool && size icons");
	large_collection_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_COLLECTION_TOOLBAR));
	sizer->Add(large_collection_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	large_doodad_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large doodad size palette icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	sizer->Add(large_doodad_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_item_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item size palette icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	sizer->Add(large_item_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_house_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large house palette size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	sizer->Add(large_house_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_raw_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large raw palette size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	sizer->Add(large_raw_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_container_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	sizer->Add(large_container_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	large_pick_item_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	sizer->Add(large_pick_item_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	switch_mousebtn_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 5);

	doubleclick_properties_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 5);

	inversed_scroll_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When this checkbox is checked, dragging the map using the center mouse button will be inversed (default RTS behaviour).");
	sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Scroll speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_scrollspeed = int(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 10);
	scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_scrollspeed, 1, max(true_scrollspeed, 100));
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Zoom speed: "), 0, wxLEFT | wxTOP, 5);

	auto true_zoomspeed = int(g_settings.getFloat(Config::ZOOM_SPEED) * 10);
	zoom_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_zoomspeed, 1, max(true_zoomspeed, 100));
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	ui_page->SetSizerAndFit(sizer);

	return ui_page;
}

wxNotebookPage* PreferencesWindow::CreateClientPage() {
	wxNotebookPage* client_page = newd wxPanel(book, wxID_ANY);

	// Refresh g_settings
	ClientVersion::saveVersions();
	ClientVersionList versions = ClientVersion::getAllVisible();

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	auto* options_sizer = newd wxFlexGridSizer(2, 10, 10);
	options_sizer->AddGrowableCol(1);

	// Default client version choice control
	default_version_choice = newd wxChoice(client_page, wxID_ANY);
	wxStaticText* default_client_tooltip = newd wxStaticText(client_page, wxID_ANY, "Default client version:");
	options_sizer->Add(default_client_tooltip, 0, wxLEFT | wxTOP, 5);
	options_sizer->Add(default_version_choice, 0, wxTOP, 5);
	SetWindowToolTip(default_client_tooltip, default_version_choice, "This will decide what client version will be used when new maps are created.");

	// Check file sigs checkbox
	check_sigs_chkbox = newd wxCheckBox(client_page, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getBoolean(Config::CHECK_SIGNATURES));
	check_sigs_chkbox->SetToolTip("When this option is not checked, the editor will load any OTB/DAT/SPR combination without complaints. This may cause graphics bugs.");
	options_sizer->Add(check_sigs_chkbox, 0, wxLEFT | wxRIGHT | wxTOP, 5);

	// Force client items.otb checkbox
	force_client_otb_chkbox = newd wxCheckBox(client_page, wxID_ANY, "Force load items.otb from client folder");
	force_client_otb_chkbox->SetValue(g_settings.getBoolean(Config::FORCE_CLIENT_ITEMS_OTB));
	force_client_otb_chkbox->SetToolTip("When enabled, items.otb and items.xml will be loaded from the same folder as the client files instead of the data directory.");
	options_sizer->Add(force_client_otb_chkbox, 0, wxLEFT | wxRIGHT | wxTOP, 5);

	// Add the grid sizer
	topsizer->Add(options_sizer, wxSizerFlags(0).Expand());
	topsizer->AddSpacer(10);

	wxScrolledWindow* client_list_window = newd wxScrolledWindow(client_page, wxID_ANY, wxDefaultPosition, wxDefaultSize);
	client_list_window->SetMinSize(FROM_DIP(this, wxSize(450, 450)));
	auto* client_list_sizer = newd wxFlexGridSizer(2, 10, 10);
	client_list_sizer->AddGrowableCol(1);

	int version_counter = 0;
	for (auto version : versions) {
		if (!version->isVisible()) {
			continue;
		}

		default_version_choice->Append(wxstr(version->getName()));

		wxStaticText* tmp_text = newd wxStaticText(client_list_window, wxID_ANY, wxString(version->getName()));
		client_list_sizer->Add(tmp_text, wxSizerFlags(0).Expand());

		wxDirPickerCtrl* dir_picker = newd wxDirPickerCtrl(client_list_window, wxID_ANY, version->getClientPath().GetFullPath());
		version_dir_pickers.push_back(dir_picker);
		client_list_sizer->Add(dir_picker, wxSizerFlags(0).Border(wxRIGHT, 10).Expand());

		wxString tooltip;
		tooltip << "The editor will look for " << wxstr(version->getName()) << " DAT & SPR here.";
		tmp_text->SetToolTip(tooltip);
		dir_picker->SetToolTip(tooltip);

		if (version->getID() == g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION)) {
			default_version_choice->SetSelection(version_counter);
		}

		version_counter++;
	}

	// Set the sizers
	client_list_window->SetSizer(client_list_sizer);
	client_list_window->FitInside();
	client_list_window->SetScrollRate(5, 5);
	topsizer->Add(client_list_window, 0, wxALL, 5);
	client_page->SetSizerAndFit(topsizer);

	return client_page;
}

wxNotebookPage* PreferencesWindow::CreateLODPage() {
	wxNotebookPage* lod_page = newd wxPanel(book, wxID_ANY);
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext;

	// Add tooltip max zoom control
	wxFlexGridSizer* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	// Tooltip settings
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Tooltip max zoom: "), 0);
	tooltip_max_zoom_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::TOOLTIP_MAX_ZOOM)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 10);
	grid_sizer->Add(tooltip_max_zoom_spin, 0);
	SetWindowToolTip(tmptext, tooltip_max_zoom_spin, "When zoomed out beyond this level, tooltips will not be generated to improve performance.");

	// Ground only rendering threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Ground-only zoom threshold: "), 0);
	ground_only_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::GROUND_ONLY_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, 8);
	grid_sizer->Add(ground_only_threshold_spin, 0);
	SetWindowToolTip(tmptext, ground_only_threshold_spin, "When zoomed out beyond this level, only ground tiles will be rendered for better performance.");

	// Item display threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Item display zoom threshold: "), 0);
	item_display_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::ITEM_DISPLAY_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, 10);
	grid_sizer->Add(item_display_threshold_spin, 0);
	SetWindowToolTip(tmptext, item_display_threshold_spin, "When zoomed out beyond this level, items on tiles won't be displayed (unless hide items when zoomed is disabled).");

	// Special features threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Special features zoom threshold: "), 0);
	special_features_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::SPECIAL_FEATURES_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, 10);
	grid_sizer->Add(special_features_threshold_spin, 0);
	SetWindowToolTip(tmptext, special_features_threshold_spin, "When zoomed out beyond this level, special features like waypoints and house exits won't be shown.");

	// Animation threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Animation zoom threshold: "), 0);
	animation_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::ANIMATION_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 20, 2);
	grid_sizer->Add(animation_threshold_spin, 0);
	SetWindowToolTip(tmptext, animation_threshold_spin, "When zoomed out beyond this level, item animations won't be processed for better performance.");

	// Effects threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Effects zoom threshold: "), 0);
	effects_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::EFFECTS_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 20, 6);
	grid_sizer->Add(effects_threshold_spin, 0);
	SetWindowToolTip(tmptext, effects_threshold_spin, "When zoomed out beyond this level, visual effects like house highlighting won't be rendered.");

	// Light threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Light zoom threshold: "), 0);
	light_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::LIGHT_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 20, 4);
	grid_sizer->Add(light_threshold_spin, 0);
	SetWindowToolTip(tmptext, light_threshold_spin, "When zoomed out beyond this level, light effects won't be rendered.");

	// Shade threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Shade zoom threshold: "), 0);
	shade_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::SHADE_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 30, 8);
	grid_sizer->Add(shade_threshold_spin, 0);
	SetWindowToolTip(tmptext, shade_threshold_spin, "When zoomed out beyond this level, tile shading won't be shown.");

	// Town/Zone threshold 
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Town/Zone zoom threshold: "), 0);
	town_zone_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::TOWN_ZONE_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 20, 6);
	grid_sizer->Add(town_zone_threshold_spin, 0);
	SetWindowToolTip(tmptext, town_zone_threshold_spin, "When zoomed out beyond this level, towns and zone markers won't be displayed.");

	// Grid threshold
	grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Grid zoom threshold: "), 0);
	grid_threshold_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::GRID_ZOOM_THRESHOLD)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 50, 12);
	grid_sizer->Add(grid_threshold_spin, 0);
	SetWindowToolTip(tmptext, grid_threshold_spin, "When zoomed out beyond this level, the grid won't be displayed.");

	sizer->Add(grid_sizer, 0, wxALL, 5);
	
	// Add a separator line
	wxStaticLine* separator = new wxStaticLine(lod_page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL);
	sizer->Add(separator, 0, wxEXPAND | wxALL, 5);
	
	// Add a section title for palette grid settings
	sizer->Add(newd wxStaticText(lod_page, wxID_ANY, "Palette Grid Settings"), 0, wxALL, 5);
	
	// Create a new grid sizer for palette grid settings
	wxFlexGridSizer* palette_grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	palette_grid_sizer->AddGrowableCol(1);
	
	// Chunk size setting
	palette_grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Grid chunk size: "), 0);
	chunk_size_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::GRID_CHUNK_SIZE)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 500, 10000, 3000);
	palette_grid_sizer->Add(chunk_size_spin, 0);
	SetWindowToolTip(tmptext, chunk_size_spin, "Number of items per chunk in large tilesets. Lower values improve performance but require more navigation.");
	
	// Visible rows margin setting
	palette_grid_sizer->Add(tmptext = newd wxStaticText(lod_page, wxID_ANY, "Visible rows margin: "), 0);
	visible_rows_margin_spin = newd wxSpinCtrl(lod_page, wxID_ANY, i2ws(g_settings.getInteger(Config::GRID_VISIBLE_ROWS_MARGIN)), 
		wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 5, 100, 30);
	palette_grid_sizer->Add(visible_rows_margin_spin, 0);
	SetWindowToolTip(tmptext, visible_rows_margin_spin, "Number of extra rows to load above/below the visible area. Higher values use more memory but reduce flickering.");
	
	sizer->Add(palette_grid_sizer, 0, wxALL, 5);
	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(lod_page, wxID_ANY, "Higher values = better performance, less detail."), 0, wxLEFT | wxBOTTOM, 5);

	lod_page->SetSizerAndFit(sizer);
	return lod_page;
}

wxNotebookPage* PreferencesWindow::CreateAutomagicPage() {
	wxNotebookPage* automagic_page = newd wxPanel(book, wxID_ANY);
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Create main checkbox for enabling/disabling automagic
	automagic_enabled_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Enable Automagic");
	automagic_enabled_chkbox->SetValue(g_settings.getBoolean(Config::USE_AUTOMAGIC));
	automagic_enabled_chkbox->SetToolTip("Automatically apply borders and wall connections when editing (Toggle with 'A' key)");
	sizer->Add(automagic_enabled_chkbox, 0, wxLEFT | wxTOP, 5);

	// Create settings group for detailed options
	wxStaticBoxSizer* settings_sizer = newd wxStaticBoxSizer(wxVERTICAL, automagic_page, "Border Settings");
	
	same_ground_type_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Same Ground Type Border");
	same_ground_type_chkbox->SetValue(g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER));
	same_ground_type_chkbox->SetToolTip("Preserve existing borders and only apply borders for the current ground type");
	settings_sizer->Add(same_ground_type_chkbox, 0, wxALL, 5);
	
	walls_repel_borders_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Walls Repel Borders");
	walls_repel_borders_chkbox->SetValue(g_settings.getBoolean(Config::WALLS_REPEL_BORDERS));
	walls_repel_borders_chkbox->SetToolTip("When enabled, walls will block border generation, preventing borders from crossing through walls");
	settings_sizer->Add(walls_repel_borders_chkbox, 0, wxALL, 5);
	
	layer_carpets_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Layer Carpets");
	layer_carpets_chkbox->SetValue(g_settings.getBoolean(Config::LAYER_CARPETS));
	layer_carpets_chkbox->SetToolTip("When enabled, carpet brushes will be placed on top of existing carpets instead of replacing them");
	settings_sizer->Add(layer_carpets_chkbox, 0, wxALL, 5);
	
	borderize_delete_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Borderize on Delete");
	borderize_delete_chkbox->SetValue(g_settings.getBoolean(Config::BORDERIZE_DELETE));
	borderize_delete_chkbox->SetToolTip("When enabled, deleting items will trigger automatic bordering of surrounding tiles");
	settings_sizer->Add(borderize_delete_chkbox, 0, wxALL, 5);
	
	// Paste/Drag borderize settings
	borderize_paste_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Borderize on Paste");
	borderize_paste_chkbox->SetValue(g_settings.getBoolean(Config::BORDERIZE_PASTE));
	borderize_paste_chkbox->SetToolTip("When enabled, pasting will trigger automatic bordering");
	settings_sizer->Add(borderize_paste_chkbox, 0, wxALL, 5);
	
	wxBoxSizer* paste_threshold_sizer = newd wxBoxSizer(wxHORIZONTAL);
	paste_threshold_sizer->Add(newd wxStaticText(automagic_page, wxID_ANY, "Paste Borderize Threshold:  "), 0, wxLEFT | wxTOP, 5);
	borderize_paste_threshold_spin = newd wxSpinCtrl(automagic_page, wxID_ANY, i2ws(g_settings.getInteger(Config::BORDERIZE_PASTE_THRESHOLD)), wxDefaultPosition, wxDefaultSize);
	paste_threshold_sizer->Add(borderize_paste_threshold_spin, 0, wxLEFT | wxTOP, 5);
	settings_sizer->Add(paste_threshold_sizer, 0, wxALL, 0);
	
	borderize_drag_chkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Borderize on Drag");
	borderize_drag_chkbox->SetValue(g_settings.getBoolean(Config::BORDERIZE_DRAG));
	borderize_drag_chkbox->SetToolTip("When enabled, dragging will trigger automatic bordering");
	settings_sizer->Add(borderize_drag_chkbox, 0, wxALL, 5);
	
	wxBoxSizer* drag_threshold_sizer = newd wxBoxSizer(wxHORIZONTAL);
	drag_threshold_sizer->Add(newd wxStaticText(automagic_page, wxID_ANY, "Drag Borderize Threshold:  "), 0, wxLEFT | wxTOP, 5);
	borderize_drag_threshold_spin = newd wxSpinCtrl(automagic_page, wxID_ANY, i2ws(g_settings.getInteger(Config::BORDERIZE_DRAG_THRESHOLD)), wxDefaultPosition, wxDefaultSize);
	drag_threshold_sizer->Add(borderize_drag_threshold_spin, 0, wxLEFT | wxTOP, 5);
	settings_sizer->Add(drag_threshold_sizer, 0, wxALL, 0);

	custom_border_checkbox = newd wxCheckBox(automagic_page, wxID_ANY, "Use Custom Border");
	custom_border_checkbox->SetValue(g_settings.getBoolean(Config::CUSTOM_BORDER_ENABLED));
	custom_border_checkbox->SetToolTip("Override automatic border selection with a specific border ID");
	settings_sizer->Add(custom_border_checkbox, 0, wxALL, 5);

	// Create custom border ID input field in a horizontal layout
	wxBoxSizer* custom_border_sizer = newd wxBoxSizer(wxHORIZONTAL);
	custom_border_id_label = newd wxStaticText(automagic_page, wxID_ANY, "Custom Border ID:  ");
	custom_border_sizer->Add(custom_border_id_label, 0, wxALIGN_CENTER_VERTICAL);
	
	custom_border_id_spin = newd wxSpinCtrl(automagic_page, wxID_ANY, i2ws(g_settings.getInteger(Config::CUSTOM_BORDER_ID)), 
		wxDefaultPosition, wxDefaultSize);
	custom_border_id_spin->SetRange(1, 65535);
	custom_border_id_spin->SetToolTip("The ID of the border to apply during automagic operations");
	custom_border_sizer->Add(custom_border_id_spin, 0, wxLEFT, 5);
	
	settings_sizer->Add(custom_border_sizer, 0, wxALL, 5);

	sizer->Add(settings_sizer, 0, wxEXPAND | wxALL, 5);
	
	// Add description text
	wxStaticText* description = newd wxStaticText(automagic_page, wxID_ANY, 
		"The Automagic system automatically applies borders and wall connections.\n\n"
		"When 'Same Ground Type Border' is enabled, the editor will:\n"
		"- Preserve existing borders on tiles\n"
		"- Only apply borders for the current ground type\n"
		"- Respect Z-axis positioning of existing borders\n\n"
		"When 'Walls Repel Borders' is enabled, the editor will:\n"
		"- Prevent borders from crossing through walls\n"
		"- Treat walls as barriers for border generation\n"
		"The threshold values control the maximum selection size for\n"
		"auto-borderizing during paste and drag operations.");
	sizer->Add(description, 0, wxALL, 5);
	
	// Make dependent controls dynamically enable/disable based on master checkbox
	automagic_enabled_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		bool enabled = automagic_enabled_chkbox->GetValue();
		same_ground_type_chkbox->Enable(enabled);
		walls_repel_borders_chkbox->Enable(enabled);
		layer_carpets_chkbox->Enable(enabled);
		borderize_delete_chkbox->Enable(enabled);
		borderize_paste_chkbox->Enable(enabled);
		borderize_drag_chkbox->Enable(enabled);
		borderize_paste_threshold_spin->Enable(enabled && borderize_paste_chkbox->GetValue());
		borderize_drag_threshold_spin->Enable(enabled && borderize_drag_chkbox->GetValue());
		custom_border_checkbox->Enable(enabled);
		custom_border_id_spin->Enable(enabled && custom_border_checkbox->GetValue());
		custom_border_id_label->Enable(enabled && custom_border_checkbox->GetValue());
	});
	
	borderize_paste_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		borderize_paste_threshold_spin->Enable(automagic_enabled_chkbox->GetValue() && borderize_paste_chkbox->GetValue());
	});
	
	borderize_drag_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		borderize_drag_threshold_spin->Enable(automagic_enabled_chkbox->GetValue() && borderize_drag_chkbox->GetValue());
	});
	
	custom_border_checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		custom_border_id_spin->Enable(automagic_enabled_chkbox->GetValue() && custom_border_checkbox->GetValue());
		custom_border_id_label->Enable(automagic_enabled_chkbox->GetValue() && custom_border_checkbox->GetValue());
	});
	
	// Initial state setup
	bool enabled = automagic_enabled_chkbox->GetValue();
	same_ground_type_chkbox->Enable(enabled);
	walls_repel_borders_chkbox->Enable(enabled);
	layer_carpets_chkbox->Enable(enabled);
	borderize_delete_chkbox->Enable(enabled);
	borderize_paste_chkbox->Enable(enabled);
	borderize_drag_chkbox->Enable(enabled);
	borderize_paste_threshold_spin->Enable(enabled && borderize_paste_chkbox->GetValue());
	borderize_drag_threshold_spin->Enable(enabled && borderize_drag_chkbox->GetValue());
	custom_border_checkbox->Enable(enabled);
	custom_border_id_spin->Enable(enabled && custom_border_checkbox->GetValue());
	custom_border_id_label->Enable(enabled && custom_border_checkbox->GetValue());
	
	automagic_page->SetSizerAndFit(sizer);
	return automagic_page;
}

wxNotebookPage* PreferencesWindow::CreateTooltipInfoPage() {
	wxNotebookPage* tooltip_page = newd wxPanel(book, wxID_ANY);
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Master enable checkbox
	tooltip_enable_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Enable tooltips");
	tooltip_enable_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW));
	tooltip_enable_chkbox->SetToolTip("Enable or disable all tooltips in the map editor.");
	sizer->Add(tooltip_enable_chkbox, 0, wxALL, 5);

	// Show hasScript checkbox
	tooltip_show_hasscript_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show 'hasScript' in tooltips");
	tooltip_show_hasscript_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_HASSCRIPT));
	tooltip_show_hasscript_chkbox->SetToolTip("Show the 'hasScript' property in tooltips (if present).");
	sizer->Add(tooltip_show_hasscript_chkbox, 0, wxALL, 5);

	// Show text checkbox
	tooltip_show_text_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show text in tooltips");
	tooltip_show_text_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_TEXT));
	tooltip_show_text_chkbox->SetToolTip("Show item text in tooltips (if present).");
	sizer->Add(tooltip_show_text_chkbox, 0, wxALL, 5);

	// Show item ID checkbox
	tooltip_show_itemid_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show item ID in tooltips");
	tooltip_show_itemid_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_ITEMID));
	tooltip_show_itemid_chkbox->SetToolTip("Show the item ID in tooltips (id: ####).");
	sizer->Add(tooltip_show_itemid_chkbox, 0, wxALL, 5);

	// Ignore IDs input
	sizer->Add(newd wxStaticText(tooltip_page, wxID_ANY, "Ignore Item IDs/Ranges:"), 0, wxLEFT | wxTOP, 5);
	tooltip_ignore_ids_textctrl = newd wxTextCtrl(tooltip_page, wxID_ANY, wxstr(g_settings.getString(Config::TOOLTIP_IGNORE_IDS)), wxDefaultPosition, wxSize(300, -1), 0);
	tooltip_ignore_ids_textctrl->SetToolTip("Enter item IDs or ranges to ignore in tooltips, separated by commas. Example: 658-660,6891,136,3159,333-444");
	sizer->Add(tooltip_ignore_ids_textctrl, 0, wxALL | wxEXPAND, 5);

	// Show action ID checkbox
	tooltip_show_aid_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show action ID (aid) in tooltips");
	tooltip_show_aid_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_AID));
	tooltip_show_aid_chkbox->SetToolTip("Show the action ID (aid: ####) in tooltips.");
	sizer->Add(tooltip_show_aid_chkbox, 0, wxALL, 5);

	// Show unique ID checkbox
	tooltip_show_uid_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show unique ID (uid) in tooltips");
	tooltip_show_uid_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_UID));
	tooltip_show_uid_chkbox->SetToolTip("Show the unique ID (uid: ####) in tooltips.");
	sizer->Add(tooltip_show_uid_chkbox, 0, wxALL, 5);

	// Show door ID checkbox
	tooltip_show_doorid_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show door ID in tooltips");
	tooltip_show_doorid_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_DOORID));
	tooltip_show_doorid_chkbox->SetToolTip("Show the door ID (door id: #) in tooltips.");
	sizer->Add(tooltip_show_doorid_chkbox, 0, wxALL, 5);

	// Show destination checkbox
	tooltip_show_destination_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show destination in tooltips");
	tooltip_show_destination_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_DESTINATION));
	tooltip_show_destination_chkbox->SetToolTip("Show the teleport destination (destination: x, y, z) in tooltips.");
	sizer->Add(tooltip_show_destination_chkbox, 0, wxALL, 5);

	// Show house ID checkbox
	tooltip_show_houseid_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Show house ID in tooltips");
	tooltip_show_houseid_chkbox->SetValue(g_settings.getBoolean(Config::TOOLTIP_SHOW_HOUSEID));
	tooltip_show_houseid_chkbox->SetToolTip("Show the house ID on house tiles.");
	sizer->Add(tooltip_show_houseid_chkbox, 0, wxALL, 5);
	
	// Custom house colors
	house_custom_colors_chkbox = newd wxCheckBox(tooltip_page, wxID_ANY, "Use custom house colors");
	house_custom_colors_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_CUSTOM_COLORS));
	house_custom_colors_chkbox->SetToolTip("Color house tiles differently based on their house ID. Works with all items on house tiles.");
	sizer->Add(house_custom_colors_chkbox, 0, wxALL, 5);

	tooltip_page->SetSizerAndFit(sizer);
	return tooltip_page;
}

// Event handlers!

void PreferencesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	Apply();
	EndModal(0);
}

void PreferencesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void PreferencesWindow::OnClickApply(wxCommandEvent& WXUNUSED(event)) {
	Apply();
}

void PreferencesWindow::OnForceReloadRevScripts(wxCommandEvent& WXUNUSED(event)) {
//this function can only be called by button so no if statement.
		// Save the current RevScript directory setting first
		g_settings.setString(Config::OTS_DATA_DIRECTORY, nstr(revscript_directory_picker->GetPath()));
		
		// Call the GUI's reload method
		g_gui.ReloadRevScripts();
		
		// Show statistics for debugging - include RevScripts, Monsters, and NPCs
		std::string revscript_stats = g_gui.revscript_manager.getScanStatistics();
		std::string monster_stats = g_gui.monster_manager.getScanStatistics();
		std::string npc_stats = g_gui.npc_manager.getScanStatistics();
		
		wxString message = "Scripts reloaded!\n\n" + wxstr(revscript_stats) + "\n\n" + wxstr(monster_stats) + "\n\n" + wxstr(npc_stats);
		wxMessageBox(message, "Script Reload Complete", wxOK | wxICON_INFORMATION, this);
	
}

void PreferencesWindow::OnCollapsiblePane(wxCollapsiblePaneEvent& event) {
	auto* win = (wxWindow*)event.GetEventObject();
	win->GetParent()->Fit();
}

// Stuff

void PreferencesWindow::Apply() {
	bool must_restart = false;
	bool palette_update_needed = false;
	bool dark_mode_changed = false;

	// General
	g_settings.setInteger(Config::WELCOME_DIALOG, show_welcome_dialog_chkbox->GetValue());
	g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, always_make_backup_chkbox->GetValue());
	g_settings.setInteger(Config::USE_UPDATER, update_check_on_startup_chkbox->GetValue());
	g_settings.setInteger(Config::ONLY_ONE_INSTANCE, only_one_instance_chkbox->GetValue());
	
	// Check if tileset editor setting changed
	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR) != enable_tileset_editing_chkbox->GetValue()) {
		palette_update_needed = true;
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());
	
	g_settings.setInteger(Config::AUTO_SELECT_RAW_ON_RIGHTCLICK, auto_select_raw_chkbox->GetValue());
	g_settings.setInteger(Config::SUPPRESS_MAP_WARNINGS, !show_map_warnings_chkbox->GetValue());
	g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	g_settings.setInteger(Config::WORKER_THREADS, worker_threads_spin->GetValue());
	g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_format->GetSelection());
	g_settings.setInteger(Config::AUTO_SAVE_ENABLED, autosave_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_SAVE_INTERVAL, autosave_interval_spin->GetValue());

	// RevScript directory
	g_settings.setString(Config::OTS_DATA_DIRECTORY, nstr(revscript_directory_picker->GetPath()));

	// LOD Settings
	g_settings.setInteger(Config::TOOLTIP_MAX_ZOOM, tooltip_max_zoom_spin->GetValue());
	g_settings.setInteger(Config::GROUND_ONLY_ZOOM_THRESHOLD, ground_only_threshold_spin->GetValue());
	g_settings.setInteger(Config::ITEM_DISPLAY_ZOOM_THRESHOLD, item_display_threshold_spin->GetValue());
	g_settings.setInteger(Config::SPECIAL_FEATURES_ZOOM_THRESHOLD, special_features_threshold_spin->GetValue());
	g_settings.setInteger(Config::ANIMATION_ZOOM_THRESHOLD, animation_threshold_spin->GetValue());
	g_settings.setInteger(Config::EFFECTS_ZOOM_THRESHOLD, effects_threshold_spin->GetValue());
	g_settings.setInteger(Config::LIGHT_ZOOM_THRESHOLD, light_threshold_spin->GetValue());
	g_settings.setInteger(Config::SHADE_ZOOM_THRESHOLD, shade_threshold_spin->GetValue());
	g_settings.setInteger(Config::TOWN_ZONE_ZOOM_THRESHOLD, town_zone_threshold_spin->GetValue());
	g_settings.setInteger(Config::GRID_ZOOM_THRESHOLD, grid_threshold_spin->GetValue());
	
	// Palette grid settings
	g_settings.setInteger(Config::GRID_CHUNK_SIZE, chunk_size_spin->GetValue());
	g_settings.setInteger(Config::GRID_VISIBLE_ROWS_MARGIN, visible_rows_margin_spin->GetValue());
	
	// Editor
	g_settings.setInteger(Config::GROUP_ACTIONS, group_actions_chkbox->GetValue());
	g_settings.setInteger(Config::WARN_FOR_DUPLICATE_ID, duplicate_id_warn_chkbox->GetValue());
	g_settings.setInteger(Config::HOUSE_BRUSH_REMOVE_ITEMS, house_remove_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_ASSIGN_DOORID, auto_assign_doors_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE, auto_assign_depot_to_closest_temple_chkbox->GetValue());
	g_settings.setInteger(Config::ERASER_LEAVE_UNIQUE, eraser_leave_unique_chkbox->GetValue());
	g_settings.setInteger(Config::DOODAD_BRUSH_ERASE_LIKE, doodad_erase_same_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_CREATE_SPAWN, auto_create_spawn_chkbox->GetValue());
	g_settings.setInteger(Config::RAW_LIKE_SIMONE, allow_multiple_orderitems_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_MOVE, merge_move_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_PASTE, merge_paste_chkbox->GetValue());
	g_settings.setInteger(Config::REFRESH_RADIUS, refresh_radius_spin->GetValue());


	// Graphics
	g_settings.setInteger(Config::USE_GUI_SELECTION_SHADOW, icon_selection_shadow_chkbox->GetValue());
	if (g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES) != use_memcached_chkbox->GetValue()) {
		must_restart = true;
	}
	g_settings.setInteger(Config::USE_MEMCACHED_SPRITES_TO_SAVE, use_memcached_chkbox->GetValue());
	if (icon_background_choice->GetSelection() == 0) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 0) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 0);
	} else if (icon_background_choice->GetSelection() == 1) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 88) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 88);
	} else if (icon_background_choice->GetSelection() == 2) {
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != 255) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, 255);
	}

	// Dark mode setting
	bool new_dark_mode_value = dark_mode_chkbox->GetValue();
	if (g_settings.getBoolean(Config::DARK_MODE) != new_dark_mode_value) {
		g_settings.setInteger(Config::DARK_MODE, new_dark_mode_value ? 1 : 0);
		dark_mode_changed = true;
	}

	// Dark mode custom color settings
	bool new_custom_color_value = dark_mode_color_enabled_chkbox->GetValue();
	if (g_settings.getBoolean(Config::DARK_MODE_CUSTOM_COLOR) != new_custom_color_value) {
		g_settings.setInteger(Config::DARK_MODE_CUSTOM_COLOR, new_custom_color_value ? 1 : 0);
		dark_mode_changed = true;
	}

	wxColor dark_mode_clr = dark_mode_color_pick->GetColour();
	if (g_settings.getInteger(Config::DARK_MODE_RED) != dark_mode_clr.Red() ||
		g_settings.getInteger(Config::DARK_MODE_GREEN) != dark_mode_clr.Green() ||
		g_settings.getInteger(Config::DARK_MODE_BLUE) != dark_mode_clr.Blue()) {
		
		g_settings.setInteger(Config::DARK_MODE_RED, dark_mode_clr.Red());
		g_settings.setInteger(Config::DARK_MODE_GREEN, dark_mode_clr.Green());
		g_settings.setInteger(Config::DARK_MODE_BLUE, dark_mode_clr.Blue());
		dark_mode_changed = true;
	}

	// Screenshots
	g_settings.setString(Config::SCREENSHOT_DIRECTORY, nstr(screenshot_directory_picker->GetPath()));

	std::string new_format = nstr(screenshot_format_choice->GetStringSelection());
	if (new_format == "PNG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "png");
	} else if (new_format == "TGA") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "tga");
	} else if (new_format == "JPG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "jpg");
	} else if (new_format == "BMP") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "bmp");
	}

	wxColor clr = cursor_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_BLUE, clr.Blue());
	// g_settings.setInteger(Config::CURSOR_ALPHA, clr.Alpha());

	clr = cursor_alt_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());
	// g_settings.setInteger(Config::CURSOR_ALT_ALPHA, clr.Alpha());

	g_settings.setInteger(Config::HIDE_ITEMS_WHEN_ZOOMED, hide_items_when_zoomed_chkbox->GetValue());
	/*
	g_settings.setInteger(Config::TEXTURE_MANAGEMENT, texture_managment_chkbox->GetValue());
	g_settings.setInteger(Config::TEXTURE_CLEAN_PULSE, clean_interval_spin->GetValue());
	g_settings.setInteger(Config::TEXTURE_LONGEVITY, texture_longevity_spin->GetValue());
	g_settings.setInteger(Config::TEXTURE_CLEAN_THRESHOLD, texture_threshold_spin->GetValue());
	g_settings.setInteger(Config::SOFTWARE_CLEAN_THRESHOLD, software_threshold_spin->GetValue());
	g_settings.setInteger(Config::SOFTWARE_CLEAN_SIZE, software_clean_amount_spin->GetValue());
	*/

	// Interface
	SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE);
	SetPaletteStyleChoice(collection_palette_style_choice, Config::PALETTE_COLLECTION_STYLE);
	SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE);
	SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE);
	SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE);
	g_settings.setInteger(Config::USE_LARGE_TERRAIN_TOOLBAR, large_terrain_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_COLLECTION_TOOLBAR, large_collection_tools_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_DOODAD_SIZEBAR, large_doodad_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_ITEM_SIZEBAR, large_item_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_HOUSE_SIZEBAR, large_house_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_RAW_SIZEBAR, large_raw_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CONTAINER_ICONS, large_container_icons_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CHOOSE_ITEM_ICONS, large_pick_item_icons_chkbox->GetValue());

	g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	float scroll_mul = 1.0;
	if (inversed_scroll_chkbox->GetValue()) {
		scroll_mul = -1.0;
	}
	g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 10.f);
	g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 10.f);

	// Client
	ClientVersionList versions = ClientVersion::getAllVisible();
	int version_counter = 0;
	for (auto version : versions) {
		wxString dir = version_dir_pickers[version_counter]->GetPath();
		if (dir.Length() > 0 && dir.Last() != '/' && dir.Last() != '\\') {
			dir.Append("/");
		}
		version->setClientPath(FileName(dir));

		if (version->getName() == default_version_choice->GetStringSelection()) {
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, version->getID());
		}

		version_counter++;
	}
	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());
	g_settings.setInteger(Config::FORCE_CLIENT_ITEMS_OTB, force_client_otb_chkbox->GetValue());

	// Make sure to reload client paths
	ClientVersion::saveVersions();
	ClientVersion::loadVersions();

	g_settings.save();

	if (must_restart) {
		g_gui.PopupDialog(this, "Notice", "You must restart the editor for the changes to take effect.", wxOK);
	}

	if (dark_mode_changed) {
		g_darkMode.ToggleDarkMode();
		g_darkMode.ApplyTheme(g_gui.root);
		g_gui.PopupDialog(this, "Dark Mode Changed", "The application theme has been changed. Some elements may require a restart to display correctly.", wxOK);
	}

	if (palette_update_needed) {
		// Rebuild the palettes completely
		g_gui.RebuildPalettes();
	} else {
		// Just refresh the palettes with updated settings
		g_gui.RefreshPalettes();
	}

	g_settings.setInteger(Config::AUTO_SAVE_ENABLED, autosave_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_SAVE_INTERVAL, autosave_interval_spin->GetValue());
	g_settings.setInteger(Config::USE_AUTOMAGIC, automagic_enabled_chkbox->GetValue());
	g_settings.setInteger(Config::SAME_GROUND_TYPE_BORDER, same_ground_type_chkbox->GetValue());
	g_settings.setInteger(Config::WALLS_REPEL_BORDERS, walls_repel_borders_chkbox->GetValue());
	g_settings.setInteger(Config::LAYER_CARPETS, layer_carpets_chkbox->GetValue());
	g_settings.setInteger(Config::BORDERIZE_DELETE, borderize_delete_chkbox->GetValue());
	g_settings.setInteger(Config::BORDERIZE_PASTE, borderize_paste_chkbox->GetValue());
	g_settings.setInteger(Config::BORDERIZE_PASTE_THRESHOLD, borderize_paste_threshold_spin->GetValue());
	g_settings.setInteger(Config::BORDERIZE_DRAG, borderize_drag_chkbox->GetValue());
	g_settings.setInteger(Config::BORDERIZE_DRAG_THRESHOLD, borderize_drag_threshold_spin->GetValue());
	g_settings.setInteger(Config::CUSTOM_BORDER_ENABLED, custom_border_checkbox->GetValue());
	g_settings.setInteger(Config::CUSTOM_BORDER_ID, custom_border_id_spin->GetValue());

	// Tooltip Info
	g_settings.setInteger(Config::TOOLTIP_SHOW, tooltip_enable_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_HASSCRIPT, tooltip_show_hasscript_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_TEXT, tooltip_show_text_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_ITEMID, tooltip_show_itemid_chkbox->GetValue());
	g_settings.setString(Config::TOOLTIP_IGNORE_IDS, nstr(tooltip_ignore_ids_textctrl->GetValue()));
	g_settings.setInteger(Config::TOOLTIP_SHOW_AID, tooltip_show_aid_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_UID, tooltip_show_uid_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_DOORID, tooltip_show_doorid_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_DESTINATION, tooltip_show_destination_chkbox->GetValue());
	g_settings.setInteger(Config::TOOLTIP_SHOW_HOUSEID, tooltip_show_houseid_chkbox->GetValue());
	g_settings.setInteger(Config::HOUSE_CUSTOM_COLORS, house_custom_colors_chkbox->GetValue());

	// Invisible Items Colors
	g_settings.setInteger(Config::INVISIBLE_ITEMS_ENABLE_CUSTOM, invisible_items_custom_chkbox->GetValue());
	
	wxColor invalid_color = invisible_invalid_color_pick->GetColour();
	g_settings.setInteger(Config::INVISIBLE_INVALID_RED, invalid_color.Red());
	g_settings.setInteger(Config::INVISIBLE_INVALID_GREEN, invalid_color.Green());
	g_settings.setInteger(Config::INVISIBLE_INVALID_BLUE, invalid_color.Blue());
	
	wxColor stairs_color = invisible_stairs_color_pick->GetColour();
	g_settings.setInteger(Config::INVISIBLE_STAIRS_RED, stairs_color.Red());
	g_settings.setInteger(Config::INVISIBLE_STAIRS_GREEN, stairs_color.Green());
	g_settings.setInteger(Config::INVISIBLE_STAIRS_BLUE, stairs_color.Blue());
	
	wxColor walkable_color = invisible_walkable_color_pick->GetColour();
	g_settings.setInteger(Config::INVISIBLE_WALKABLE_RED, walkable_color.Red());
	g_settings.setInteger(Config::INVISIBLE_WALKABLE_GREEN, walkable_color.Green());
	g_settings.setInteger(Config::INVISIBLE_WALKABLE_BLUE, walkable_color.Blue());
	
	wxColor wall_color = invisible_wall_color_pick->GetColour();
	g_settings.setInteger(Config::INVISIBLE_WALL_RED, wall_color.Red());
	g_settings.setInteger(Config::INVISIBLE_WALL_GREEN, wall_color.Green());
	g_settings.setInteger(Config::INVISIBLE_WALL_BLUE, wall_color.Blue());
	
	g_settings.setString(Config::INVISIBLE_CUSTOM_IDS, nstr(invisible_custom_ids_textctrl->GetValue()));

	// Reload invisible items color settings
	InvisibleItemsColorManager::ReloadFromSettings();

	// Client Box Settings
	g_settings.setInteger(Config::INGAME_BOX_CUSTOM_SIZE_ENABLED, ingame_box_custom_size_chkbox->GetValue());
	g_settings.setInteger(Config::INGAME_BOX_WIDTH, ingame_box_width_spin->GetValue());
	g_settings.setInteger(Config::INGAME_BOX_HEIGHT, ingame_box_height_spin->GetValue());
	g_settings.setInteger(Config::INGAME_BOX_OFFSET_X, ingame_box_offset_x_spin->GetValue());
	g_settings.setInteger(Config::INGAME_BOX_OFFSET_Y, ingame_box_offset_y_spin->GetValue());
}

void PreferencesWindow::UpdateDarkModeUI() {
	bool enabled = dark_mode_chkbox->GetValue();
	dark_mode_color_enabled_chkbox->Enable(enabled);
	dark_mode_color_pick->Enable(enabled && dark_mode_color_enabled_chkbox->GetValue());
}

wxNotebookPage* PreferencesWindow::CreateInvisibleItemsPage() {
	wxNotebookPage* invisible_page = newd wxPanel(book, wxID_ANY);
	
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Enable custom colors checkbox
	invisible_items_custom_chkbox = newd wxCheckBox(invisible_page, wxID_ANY, "Enable custom invisible item colors");
	invisible_items_custom_chkbox->SetValue(g_settings.getBoolean(Config::INVISIBLE_ITEMS_ENABLE_CUSTOM));
	invisible_items_custom_chkbox->SetToolTip("When enabled, use custom colors for invisible items instead of the default hardcoded colors.");
	sizer->Add(invisible_items_custom_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	// Color pickers for predefined invisible items
	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	// Invalid items color (ID = 0)
	wxStaticText* invalid_label = newd wxStaticText(invisible_page, wxID_ANY, "Invalid items (ID=0):");
	grid_sizer->Add(invalid_label, 0, wxALIGN_CENTER_VERTICAL);
	
	wxColor current_invalid_color(
		g_settings.getInteger(Config::INVISIBLE_INVALID_RED),
		g_settings.getInteger(Config::INVISIBLE_INVALID_GREEN),
		g_settings.getInteger(Config::INVISIBLE_INVALID_BLUE)
	);
	invisible_invalid_color_pick = newd wxColourPickerCtrl(invisible_page, wxID_ANY, current_invalid_color);
	invisible_invalid_color_pick->SetToolTip("Color for invalid items (clientID = 0)");
	grid_sizer->Add(invisible_invalid_color_pick, 0);

	// Invisible stairs color (469)
	wxStaticText* stairs_label = newd wxStaticText(invisible_page, wxID_ANY, "Invisible stairs (469):");
	grid_sizer->Add(stairs_label, 0, wxALIGN_CENTER_VERTICAL);
	
	wxColor current_stairs_color(
		g_settings.getInteger(Config::INVISIBLE_STAIRS_RED),
		g_settings.getInteger(Config::INVISIBLE_STAIRS_GREEN),
		g_settings.getInteger(Config::INVISIBLE_STAIRS_BLUE)
	);
	invisible_stairs_color_pick = newd wxColourPickerCtrl(invisible_page, wxID_ANY, current_stairs_color);
	invisible_stairs_color_pick->SetToolTip("Color for invisible stairs tiles (clientID = 469)");
	grid_sizer->Add(invisible_stairs_color_pick, 0);

	// Invisible walkable tiles color (470, etc.)
	wxStaticText* walkable_label = newd wxStaticText(invisible_page, wxID_ANY, "Invisible walkable (470, 17970, 20028, 34168):");
	grid_sizer->Add(walkable_label, 0, wxALIGN_CENTER_VERTICAL);
	
	wxColor current_walkable_color(
		g_settings.getInteger(Config::INVISIBLE_WALKABLE_RED),
		g_settings.getInteger(Config::INVISIBLE_WALKABLE_GREEN),
		g_settings.getInteger(Config::INVISIBLE_WALKABLE_BLUE)
	);
	invisible_walkable_color_pick = newd wxColourPickerCtrl(invisible_page, wxID_ANY, current_walkable_color);
	invisible_walkable_color_pick->SetToolTip("Color for invisible walkable tiles (clientID = 470, 17970, 20028, 34168)");
	grid_sizer->Add(invisible_walkable_color_pick, 0);

	// Invisible wall color (2187)
	wxStaticText* wall_label = newd wxStaticText(invisible_page, wxID_ANY, "Invisible wall (2187):");
	grid_sizer->Add(wall_label, 0, wxALIGN_CENTER_VERTICAL);
	
	wxColor current_wall_color(
		g_settings.getInteger(Config::INVISIBLE_WALL_RED),
		g_settings.getInteger(Config::INVISIBLE_WALL_GREEN),
		g_settings.getInteger(Config::INVISIBLE_WALL_BLUE)
	);
	invisible_wall_color_pick = newd wxColourPickerCtrl(invisible_page, wxID_ANY, current_wall_color);
	invisible_wall_color_pick->SetToolTip("Color for invisible wall tiles (clientID = 2187)");
	grid_sizer->Add(invisible_wall_color_pick, 0);

	sizer->Add(grid_sizer, 0, wxEXPAND | wxALL, 6);

	sizer->AddSpacer(15);

	// Custom IDs section
	wxStaticText* custom_label = newd wxStaticText(invisible_page, wxID_ANY, "Custom invisible item IDs:");
	sizer->Add(custom_label, 0, wxLEFT | wxTOP, 5);

	wxStaticText* format_label = newd wxStaticText(invisible_page, wxID_ANY, "Format: ID:R,G,B;ID:R,G,B (e.g., 1234:255,0,0;5678:0,255,0)");
	format_label->SetFont(format_label->GetFont().Smaller());
	sizer->Add(format_label, 0, wxLEFT | wxTOP, 5);

	invisible_custom_ids_textctrl = newd wxTextCtrl(invisible_page, wxID_ANY, 
		wxstr(g_settings.getString(Config::INVISIBLE_CUSTOM_IDS)),
		wxDefaultPosition, wxSize(400, 60), wxTE_MULTILINE);
	invisible_custom_ids_textctrl->SetToolTip("Add custom invisible item IDs with their colors. Each entry should be in format ID:R,G,B separated by semicolons.");
	sizer->Add(invisible_custom_ids_textctrl, 0, wxEXPAND | wxALL, 5);

	// Connect events to update UI state
	invisible_items_custom_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		UpdateInvisibleItemsUI();
	});
	
	// Initial update of UI state
	UpdateInvisibleItemsUI();

	invisible_page->SetSizerAndFit(sizer);
	return invisible_page;
}

void PreferencesWindow::UpdateInvisibleItemsUI() {
	bool enabled = invisible_items_custom_chkbox->GetValue();
	invisible_invalid_color_pick->Enable(enabled);
	invisible_stairs_color_pick->Enable(enabled);
	invisible_walkable_color_pick->Enable(enabled);
	invisible_wall_color_pick->Enable(enabled);
	invisible_custom_ids_textctrl->Enable(enabled);
}

void PreferencesWindow::UpdateClientBoxUI() {
	bool enabled = ingame_box_custom_size_chkbox->GetValue();
	ingame_box_width_spin->Enable(enabled);
	ingame_box_height_spin->Enable(enabled);
	ingame_box_offset_x_spin->Enable(enabled);
	ingame_box_offset_y_spin->Enable(enabled);
}
