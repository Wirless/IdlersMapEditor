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

#include "materials.h"
#include "brush.h"
#include "editor.h"

#include "items.h"
#include "map.h"
#include "item.h"
#include "complexitem.h"
#include "raw_brush.h"

#include "palette_window.h"
#include "gui.h"
#include "application.h"
#include "common_windows.h"
#include "positionctrl.h"
#include "string_utils.h"


#ifdef _MSC_VER
	#pragma warning(disable : 4018) // signed/unsigned mismatch
#endif

// ============================================================================
// Map Properties Window

BEGIN_EVENT_TABLE(MapPropertiesWindow, wxDialog)
EVT_CHOICE(MAP_PROPERTIES_VERSION, MapPropertiesWindow::OnChangeVersion)
EVT_BUTTON(wxID_OK, MapPropertiesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, MapPropertiesWindow::OnClickCancel)
END_EVENT_TABLE()

MapPropertiesWindow::MapPropertiesWindow(wxWindow* parent, MapTab* view, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Map Properties", wxDefaultPosition, wxSize(300, 200), wxRESIZE_BORDER | wxCAPTION),
	view(view),
	editor(editor) {
	// Setup data variabels
	Map& map = editor.map;

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	// Description
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Description"));
	description_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getMapDescription()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	grid_sizer->Add(description_ctrl, wxSizerFlags(1).Expand());

	// Map version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Version"));
	version_choice = newd wxChoice(this, MAP_PROPERTIES_VERSION);
	version_choice->Append("OTServ 0.5.0");
	version_choice->Append("OTServ 0.6.0");
	version_choice->Append("OTServ 0.6.1");
	version_choice->Append("OTServ 0.7.0 (revscriptsys)");

	switch (map.getVersion().otbm) {
		case MAP_OTBM_1:
			version_choice->SetSelection(0);
			break;
		case MAP_OTBM_2:
			version_choice->SetSelection(1);
			break;
		case MAP_OTBM_3:
			version_choice->SetSelection(2);
			break;
		case MAP_OTBM_4:
			version_choice->SetSelection(3);
			break;
		default:
			version_choice->SetSelection(0);
	}

	grid_sizer->Add(version_choice, wxSizerFlags(1).Expand());

	// Version
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Client Version"));
	protocol_choice = newd wxChoice(this, wxID_ANY);

	protocol_choice->SetStringSelection(wxstr(g_gui.GetCurrentVersion().getName()));

	grid_sizer->Add(protocol_choice, wxSizerFlags(1).Expand());

	// Auto update checkbox
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Auto Update OTBM"));
	auto_update_checkbox = newd wxCheckBox(this, wxID_ANY, "");
	auto_update_checkbox->SetValue(true); // Default to enabled
	grid_sizer->Add(auto_update_checkbox, wxSizerFlags(0).Left());

	// Dimensions
	grid_sizer->Add(newd wxStaticText(this, wxID_ANY, "Map Dimensions"));
	{
		wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
		subsizer->Add(
			width_spin = newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getWidth())), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, MAP_MAX_WIDTH), wxSizerFlags(1).Expand()
		);
		subsizer->Add(
			height_spin = newd wxSpinCtrl(this, wxID_ANY, wxstr(i2s(map.getHeight())), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 256, MAP_MAX_HEIGHT), wxSizerFlags(1).Expand()
		);
		grid_sizer->Add(subsizer, 1, wxEXPAND);
	}

	// External files
	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Housefile")
	);

	grid_sizer->Add(
		house_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getHouseFilename())), 1, wxEXPAND
	);

	grid_sizer->Add(
		newd wxStaticText(this, wxID_ANY, "External Spawnfile")
	);

	grid_sizer->Add(
		spawn_filename_ctrl = newd wxTextCtrl(this, wxID_ANY, wxstr(map.getSpawnFilename())), 1, wxEXPAND
	);

	topsizer->Add(grid_sizer, wxSizerFlags(1).Expand().Border(wxALL, 20));

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	topsizer->Add(subsizer, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT | wxBOTTOM, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);
	UpdateProtocolList();

	ClientVersion* current_version = ClientVersion::get(map.getVersion().client);
	protocol_choice->SetStringSelection(wxstr(current_version->getName()));

	// Bind the protocol choice event
	protocol_choice->Bind(wxEVT_CHOICE, &MapPropertiesWindow::OnChangeVersion, this);
	//protocol_choice->Bind(wxEVT_CHOICE, &MapPropertiesWindow::OnClientVersionChange, this);
}

void MapPropertiesWindow::UpdateProtocolList() {
    wxString ver = version_choice->GetStringSelection();
    wxString client = protocol_choice->GetStringSelection();

    protocol_choice->Clear();

    // Always show all visible versions
    ClientVersionList versions = ClientVersion::getAllVisible();
    for(ClientVersionList::const_iterator p = versions.begin(); p != versions.end(); ++p) {
        protocol_choice->Append(wxstr((*p)->getName()));
    }

    protocol_choice->SetSelection(0);
    protocol_choice->SetStringSelection(client);
}

void MapPropertiesWindow::OnChangeVersion(wxCommandEvent& event) {
    // Get selected client version
    wxString client = protocol_choice->GetStringSelection();
    ClientVersion* version = ClientVersion::get(nstr(client));
    
    // If client version changed, update map version
    if(version) {
        // Set map version based on client version
        MapVersionID preferred_version = version->getPrefferedMapVersionID();
        switch(preferred_version) {
            case MAP_OTBM_1:
                version_choice->SetSelection(0); // OTBM 1 - 0.5.0
                break;
            case MAP_OTBM_2:
                version_choice->SetSelection(1); // OTBM 2 - 0.6.0
                break;
            case MAP_OTBM_3:
                version_choice->SetSelection(2); // OTBM 3 - 0.6.1
                break;
            case MAP_OTBM_4:
                version_choice->SetSelection(3); // OTBM 4 - 0.7.0
                break;
            default:
                version_choice->SetSelection(0); // Default to OTBM 1
                break;
        }
        version_choice->Refresh(); // Force visual refresh
    }
    
    // Get selected map version
    wxString map_ver = version_choice->GetStringSelection();
    
    // If map version changed, update client version list
    if(map_ver.Contains("0.5.0")) {
        // Filter client versions for OTBM 1
        UpdateProtocolList();
        protocol_choice->SetStringSelection(client);
    } else if(map_ver.Contains("0.6.0")) {
        // Filter client versions for OTBM 2
        UpdateProtocolList();
        protocol_choice->SetStringSelection(client);
    } else if(map_ver.Contains("0.6.1")) {
        // Filter client versions for OTBM 3
        UpdateProtocolList();
        protocol_choice->SetStringSelection(client);
    } else if(map_ver.Contains("0.7.0")) {
        // Filter client versions for OTBM 4
        UpdateProtocolList();
        protocol_choice->SetStringSelection(client);
    }
}

void MapPropertiesWindow::OnClientVersionChange(wxCommandEvent& event) {
    // Get selected client version
    wxString client = protocol_choice->GetStringSelection();
    ClientVersion* version = ClientVersion::get(nstr(client));
    
    // Only update OTBM version if auto-update is enabled
    if(version && auto_update_checkbox->GetValue()) {
        // Set map version based on client version
        MapVersionID preferred_version = version->getPrefferedMapVersionID();
        switch(preferred_version) {
            case MAP_OTBM_1:
                version_choice->SetSelection(0); // OTBM 1 - 0.5.0
                break;
            case MAP_OTBM_2:
                version_choice->SetSelection(1); // OTBM 2 - 0.6.0
                break;
            case MAP_OTBM_3:
                version_choice->SetSelection(2); // OTBM 3 - 0.6.1
                break;
            case MAP_OTBM_4:
                version_choice->SetSelection(3); // OTBM 4 - 0.7.0
                break;
            default:
                version_choice->SetSelection(0); // Default to OTBM 1
                break;
        }
        version_choice->Refresh();
    }
    
    event.Skip();
}

struct MapConversionContext {
	struct CreatureInfo {
		std::string name;
		bool is_npc;
		Outfit outfit;
	};
	typedef std::map<std::string, CreatureInfo> CreatureMap;
	CreatureMap creature_types;

	void operator()(Map& map, Tile* tile, long long done) {
		if (tile->creature) {
			CreatureMap::iterator f = creature_types.find(tile->creature->getName());
			if (f == creature_types.end()) {
				CreatureInfo info = {
					tile->creature->getName(),
					tile->creature->isNpc(),
					tile->creature->getLookType()
				};
				creature_types[tile->creature->getName()] = info;
			}
		}
	}
};

void MapPropertiesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
    Map& map = editor.map;

    MapVersion old_ver = map.getVersion();
    MapVersion new_ver;

    wxString ver = version_choice->GetStringSelection();

    new_ver.client = ClientVersion::get(nstr(protocol_choice->GetStringSelection()))->getID();
    if (ver.Contains("0.5.0")) {
        new_ver.otbm = MAP_OTBM_1;
    } else if (ver.Contains("0.6.0")) {
        new_ver.otbm = MAP_OTBM_2;
    } else if (ver.Contains("0.6.1")) {
        new_ver.otbm = MAP_OTBM_3;
    } else if (ver.Contains("0.7.0")) {
        new_ver.otbm = MAP_OTBM_4;
    }

    if (new_ver.client != old_ver.client) {
        if (g_gui.GetOpenMapCount() > 1) {
            g_gui.PopupDialog(this, "Error", "You can not change editor version with multiple maps open", wxOK);
            return;
        }
        wxString error;
        wxArrayString warnings;

        // Switch version
        g_gui.GetCurrentEditor()->selection.clear();
        g_gui.GetCurrentEditor()->actionQueue->clear();

        if (!g_gui.LoadVersion(new_ver.client, error, warnings)) {
            g_gui.PopupDialog(this, "Error", error, wxOK);
            g_gui.ListDialog(this, "Warnings", warnings);
            return;
        }

        if (!warnings.empty()) {
            g_gui.ListDialog(this, "Warnings", warnings);
        }
    }

    map.convert(new_ver, true);
    map.setMapDescription(nstr(description_ctrl->GetValue()));
    map.setHouseFilename(nstr(house_filename_ctrl->GetValue()));
    map.setSpawnFilename(nstr(spawn_filename_ctrl->GetValue()));

    // Only resize if we have to
    int new_map_width = width_spin->GetValue();
    int new_map_height = height_spin->GetValue();
    if(new_map_width != map.getWidth() || new_map_height != map.getHeight()) {
        map.setWidth(new_map_width);
        map.setHeight(new_map_height);
        g_gui.FitViewToMap(view);
    }
    g_gui.RefreshPalettes();

    EndModal(1);
}

void MapPropertiesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(1);
}

MapPropertiesWindow::~MapPropertiesWindow() = default;

// ============================================================================
// Map Import Window

BEGIN_EVENT_TABLE(ImportMapWindow, wxDialog)
EVT_BUTTON(MAP_WINDOW_FILE_BUTTON, ImportMapWindow::OnClickBrowse)
EVT_BUTTON(wxID_OK, ImportMapWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, ImportMapWindow::OnClickCancel)
END_EVENT_TABLE()

ImportMapWindow::ImportMapWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Import Map", wxDefaultPosition, wxSize(350, 315)),
	editor(editor) {
	wxBoxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticBoxSizer* tmpsizer;

	// File
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Map File"), wxHORIZONTAL);
	file_text_field = newd wxTextCtrl(tmpsizer->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxSize(230, 23));
	tmpsizer->Add(file_text_field, 0, wxALL, 5);
	wxButton* browse_button = newd wxButton(tmpsizer->GetStaticBox(), MAP_WINDOW_FILE_BUTTON, "Browse...", wxDefaultPosition, wxSize(80, 23));
	tmpsizer->Add(browse_button, 0, wxALL, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

	// Import offset
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Import Offset"), wxHORIZONTAL);
	tmpsizer->Add(newd wxStaticText(tmpsizer->GetStaticBox(), wxID_ANY, "Offset X:"), 0, wxALL | wxEXPAND, 5);
	x_offset_ctrl = newd wxSpinCtrl(tmpsizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, 23), wxSP_ARROW_KEYS, -MAP_MAX_HEIGHT, MAP_MAX_HEIGHT);
	tmpsizer->Add(x_offset_ctrl, 0, wxALL, 5);
	tmpsizer->Add(newd wxStaticText(tmpsizer->GetStaticBox(), wxID_ANY, "Offset Y:"), 0, wxALL, 5);
	y_offset_ctrl = newd wxSpinCtrl(tmpsizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, 23), wxSP_ARROW_KEYS, -MAP_MAX_HEIGHT, MAP_MAX_HEIGHT);
	tmpsizer->Add(y_offset_ctrl, 0, wxALL, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// Import options
	wxArrayString house_choices;
	house_choices.Add("Smart Merge");
	house_choices.Add("Insert");
	house_choices.Add("Merge");
	house_choices.Add("Don't Import");

	// House options
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "House Import Behaviour"), wxVERTICAL);
	house_options = newd wxChoice(tmpsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, house_choices);
	house_options->SetSelection(0);
	tmpsizer->Add(house_options, 0, wxALL | wxEXPAND, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// Import options
	wxArrayString spawn_choices;
	spawn_choices.Add("Merge");
	spawn_choices.Add("Don't Import");

	// Spawn options
	tmpsizer = newd wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Spawn Import Behaviour"), wxVERTICAL);
	spawn_options = newd wxChoice(tmpsizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, spawn_choices);
	spawn_options->SetSelection(0);
	tmpsizer->Add(spawn_options, 0, wxALL | wxEXPAND, 5);
	sizer->Add(tmpsizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 5);

	// OK/Cancel buttons
	wxBoxSizer* buttons = newd wxBoxSizer(wxHORIZONTAL);
	buttons->Add(newd wxButton(this, wxID_OK, "Ok"), 0, wxALL, 5);
	buttons->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
	sizer->Add(buttons, wxSizerFlags(1).Center());

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);
}

ImportMapWindow::~ImportMapWindow() = default;

void ImportMapWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dialog(this, "Import...", "", "", "*.otbm", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	int ok = dialog.ShowModal();

	if (ok == wxID_OK) {
		file_text_field->ChangeValue(dialog.GetPath());
	}
}

void ImportMapWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (Validate() && TransferDataFromWindow()) {
		wxFileName fn = file_text_field->GetValue();
		if (!fn.FileExists()) {
			g_gui.PopupDialog(this, "Error", "The specified map file doesn't exist", wxOK);
			return;
		}

		ImportType spawn_import_type = IMPORT_DONT;
		ImportType house_import_type = IMPORT_DONT;

		switch (spawn_options->GetSelection()) {
			case 0:
				spawn_import_type = IMPORT_MERGE;
				break;
			case 1:
				spawn_import_type = IMPORT_DONT;
				break;
		}

		switch (house_options->GetSelection()) {
			case 0:
				house_import_type = IMPORT_SMART_MERGE;
				break;
			case 1:
				house_import_type = IMPORT_MERGE;
				break;
			case 2:
				house_import_type = IMPORT_INSERT;
				break;
			case 3:
				house_import_type = IMPORT_DONT;
				break;
		}

		EndModal(1);

		editor.importMap(fn, x_offset_ctrl->GetValue(), y_offset_ctrl->GetValue(), house_import_type, spawn_import_type);
	}
}

void ImportMapWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

// ============================================================================
// Export Minimap window

BEGIN_EVENT_TABLE(ExportMiniMapWindow, wxDialog)
EVT_BUTTON(MAP_WINDOW_FILE_BUTTON, ExportMiniMapWindow::OnClickBrowse)
EVT_BUTTON(wxID_OK, ExportMiniMapWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, ExportMiniMapWindow::OnClickCancel)
EVT_CHOICE(wxID_ANY, ExportMiniMapWindow::OnExportTypeChange)
END_EVENT_TABLE()

ExportMiniMapWindow::ExportMiniMapWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export Minimap", wxDefaultPosition, wxSize(400, 300)),
	editor(editor) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	// Error field
	error_field = newd wxStaticText(this, wxID_VIEW_DETAILS, "", wxDefaultPosition, wxDefaultSize);
	error_field->SetForegroundColour(*wxRED);
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(error_field, 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// Output folder
	directory_text_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
	directory_text_field->Bind(wxEVT_KEY_UP, &ExportMiniMapWindow::OnDirectoryChanged, this);
	directory_text_field->SetValue(wxString(g_settings.getString(Config::MINIMAP_EXPORT_DIR)));
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Output Folder");
	tmpsizer->Add(directory_text_field, 1, wxALL, 5);
	tmpsizer->Add(newd wxButton(this, MAP_WINDOW_FILE_BUTTON, "Browse"), 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxALL | wxEXPAND, 5);

	// File name
	wxString mapName(editor.map.getName().c_str(), wxConvUTF8);
	file_name_text_field = newd wxTextCtrl(this, wxID_ANY, mapName.BeforeLast('.'), wxDefaultPosition, wxDefaultSize);
	file_name_text_field->Bind(wxEVT_KEY_UP, &ExportMiniMapWindow::OnFileNameChanged, this);
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "File Name");
	tmpsizer->Add(file_name_text_field, 1, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// Export options
	wxArrayString choices;
	choices.Add("All Floors");
	choices.Add("Ground Floor");
	choices.Add("Specific Floor");

	if (editor.hasSelection()) {
		choices.Add("Selected Area");
	}

	// Area options
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Area Options");
	floor_options = newd wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, choices);
	floor_number = newd wxSpinCtrl(this, wxID_ANY, i2ws(GROUND_LAYER), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, MAP_MAX_LAYER, GROUND_LAYER);
	floor_number->Enable(false);
	floor_options->SetSelection(0);
	tmpsizer->Add(floor_options, 1, wxALL, 5);
	tmpsizer->Add(floor_number, 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(ok_button = newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER, 10);

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);
	CheckValues();
}

ExportMiniMapWindow::~ExportMiniMapWindow() = default;

void ExportMiniMapWindow::OnExportTypeChange(wxCommandEvent& event) {
	floor_number->Enable(event.GetSelection() == 2);
}

void ExportMiniMapWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event)) {
	wxDirDialog dialog(NULL, "Select the output folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dialog.ShowModal() == wxID_OK) {
		const wxString& directory = dialog.GetPath();
		directory_text_field->ChangeValue(directory);
	}
	CheckValues();
}

void ExportMiniMapWindow::OnDirectoryChanged(wxKeyEvent& event) {
	CheckValues();
	event.Skip();
}

void ExportMiniMapWindow::OnFileNameChanged(wxKeyEvent& event) {
	CheckValues();
	event.Skip();
}

void ExportMiniMapWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	g_gui.CreateLoadBar("Exporting minimap");

	try {
		FileName directory(directory_text_field->GetValue());
		g_settings.setString(Config::MINIMAP_EXPORT_DIR, directory_text_field->GetValue().ToStdString());

		switch (floor_options->GetSelection()) {
			case 0: { // All floors
				for (int floor = 0; floor < MAP_LAYERS; ++floor) {
					g_gui.SetLoadScale(int(floor * (100.f / 16.f)), int((floor + 1) * (100.f / 16.f)));
					FileName file(file_name_text_field->GetValue() + "_" + i2ws(floor) + ".bmp");
					file.Normalize(wxPATH_NORM_ALL, directory.GetFullPath());
					editor.exportMiniMap(file, floor, true);
				}
				break;
			}

			case 1: { // Ground floor
				FileName file(file_name_text_field->GetValue() + "_" + i2ws(GROUND_LAYER) + ".bmp");
				file.Normalize(wxPATH_NORM_ALL, directory.GetFullPath());
				editor.exportMiniMap(file, GROUND_LAYER, true);
				break;
			}

			case 2: { // Specific floors
				int floor = floor_number->GetValue();
				FileName file(file_name_text_field->GetValue() + "_" + i2ws(floor) + ".bmp");
				file.Normalize(wxPATH_NORM_ALL, directory.GetFullPath());
				editor.exportMiniMap(file, floor, true);
				break;
			}

			case 3: { // Selected area
				editor.exportSelectionAsMiniMap(directory, file_name_text_field->GetValue());
				break;
			}
		}
	} catch (std::bad_alloc&) {
		g_gui.PopupDialog("Error", "There is not enough memory available to complete the operation.", wxOK);
	}

	g_gui.DestroyLoadBar();
	EndModal(1);
}

void ExportMiniMapWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

void ExportMiniMapWindow::CheckValues() {
	if (directory_text_field->IsEmpty()) {
		error_field->SetLabel("Type or select an output folder.");
		ok_button->Enable(false);
		return;
	}

	if (file_name_text_field->IsEmpty()) {
		error_field->SetLabel("Type a name for the file.");
		ok_button->Enable(false);
		return;
	}

	FileName directory(directory_text_field->GetValue());

	if (!directory.Exists()) {
		error_field->SetLabel("Output folder not found.");
		ok_button->Enable(false);
		return;
	}

	if (!directory.IsDirWritable()) {
		error_field->SetLabel("Output folder is not writable.");
		ok_button->Enable(false);
		return;
	}

	error_field->SetLabel(wxEmptyString);
	ok_button->Enable(true);
}

// ============================================================================
// Export Tilesets window

BEGIN_EVENT_TABLE(ExportTilesetsWindow, wxDialog)
EVT_BUTTON(TILESET_FILE_BUTTON, ExportTilesetsWindow::OnClickBrowse)
EVT_BUTTON(wxID_OK, ExportTilesetsWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, ExportTilesetsWindow::OnClickCancel)
END_EVENT_TABLE()

ExportTilesetsWindow::ExportTilesetsWindow(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Export Tilesets", wxDefaultPosition, wxSize(400, 230)),
	editor(editor) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	// Error field
	error_field = newd wxStaticText(this, wxID_VIEW_DETAILS, "", wxDefaultPosition, wxDefaultSize);
	error_field->SetForegroundColour(*wxRED);
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(error_field, 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// Output folder
	directory_text_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
	directory_text_field->Bind(wxEVT_KEY_UP, &ExportTilesetsWindow::OnDirectoryChanged, this);
	directory_text_field->SetValue(wxString(g_settings.getString(Config::TILESET_EXPORT_DIR)));
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Output Folder");
	tmpsizer->Add(directory_text_field, 1, wxALL, 5);
	tmpsizer->Add(newd wxButton(this, TILESET_FILE_BUTTON, "Browse"), 0, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxALL | wxEXPAND, 5);

	// File name
	file_name_text_field = newd wxTextCtrl(this, wxID_ANY, "tilesets", wxDefaultPosition, wxDefaultSize);
	file_name_text_field->Bind(wxEVT_KEY_UP, &ExportTilesetsWindow::OnFileNameChanged, this);
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "File Name");
	tmpsizer->Add(file_name_text_field, 1, wxALL, 5);
	sizer->Add(tmpsizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 5);

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(ok_button = newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER, 10);

	SetSizer(sizer);
	Layout();
	Centre(wxBOTH);
	CheckValues();
}

ExportTilesetsWindow::~ExportTilesetsWindow() = default;

void ExportTilesetsWindow::OnClickBrowse(wxCommandEvent& WXUNUSED(event)) {
	wxDirDialog dialog(NULL, "Select the output folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dialog.ShowModal() == wxID_OK) {
		const wxString& directory = dialog.GetPath();
		directory_text_field->ChangeValue(directory);
	}
	CheckValues();
}

void ExportTilesetsWindow::OnDirectoryChanged(wxKeyEvent& event) {
	CheckValues();
	event.Skip();
}

void ExportTilesetsWindow::OnFileNameChanged(wxKeyEvent& event) {
	CheckValues();
	event.Skip();
}

void ExportTilesetsWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	g_gui.CreateLoadBar("Exporting Tilesets");

	try {
		FileName directory(directory_text_field->GetValue());
		g_settings.setString(Config::TILESET_EXPORT_DIR, directory_text_field->GetValue().ToStdString());

		FileName file(file_name_text_field->GetValue() + ".xml");
		file.Normalize(wxPATH_NORM_ALL, directory.GetFullPath());

		pugi::xml_document doc;
		pugi::xml_node node = doc.append_child("materials");

		std::map<std::string, TilesetCategoryType> palettes {
			{ "Terrain", TILESET_TERRAIN },
			{ "Doodad", TILESET_DOODAD },
			{ "Items", TILESET_ITEM },
			{ "Collection", TILESET_COLLECTION },
			{ "Raw", TILESET_RAW }
		};
		for (TilesetContainer::iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
			std::string _data = iter->second->name;
			std::transform(_data.begin(), _data.end(), _data.begin(), [](unsigned char c) { return std::tolower(c); });
			if (_data == "others") {
				bool blocked = 1;

				for (const auto& kv : palettes) {
					TilesetCategory* tilesetCategory = iter->second->getCategory(kv.second);

					if (kv.second != TILESET_RAW && tilesetCategory->brushlist.size() > 0) {
						blocked = 0;
					}
				}

				if (blocked) {
					continue;
				}
			}

			pugi::xml_node tileset = node.append_child("tileset");
			tileset.append_attribute("name") = iter->second->name.c_str();

			for (const auto& kv : palettes) {
				TilesetCategory* tilesetCategory = iter->second->getCategory(kv.second);

				if (tilesetCategory->brushlist.size() > 0) {
					std::string data = kv.first;
					std::transform(data.begin(), data.end(), data.begin(), [](unsigned char c) { return std::tolower(c); });

					pugi::xml_node palette = tileset.append_child(data.c_str());
					for (BrushVector::const_iterator _iter = tilesetCategory->brushlist.begin(); _iter != tilesetCategory->brushlist.end(); ++_iter) {
						if (!(*_iter)->isRaw()) {
							pugi::xml_node brush = palette.append_child("brush");
							brush.append_attribute("name") = (*_iter)->getName().c_str();
						} else {
							ItemType& it = g_items[(*_iter)->asRaw()->getItemID()];
							if (it.id != 0) {
								pugi::xml_node item = palette.append_child("item");
								item.append_attribute("id") = it.id;
							}
						}
					}
				}
			}

			size_t n = std::distance(tileset.begin(), tileset.end());
			if (n <= 0) {
				node.remove_child(tileset);
			}
		}

		doc.save_file(file.GetFullPath().mb_str());
		g_gui.PopupDialog("Successfully saved Tilesets", "Saved tilesets to '" + std::string(file.GetFullPath().mb_str()) + "'", wxOK);
		g_materials.modify(false);
	} catch (std::bad_alloc&) {
		g_gui.PopupDialog("Error", "There is not enough memory available to complete the operation.", wxOK);
	}

	g_gui.DestroyLoadBar();
	EndModal(1);
}

void ExportTilesetsWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

void ExportTilesetsWindow::CheckValues() {
	if (directory_text_field->IsEmpty()) {
		error_field->SetLabel("Type or select an output folder.");
		ok_button->Enable(false);
		return;
	}

	if (file_name_text_field->IsEmpty()) {
		error_field->SetLabel("Type a name for the file.");
		ok_button->Enable(false);
		return;
	}

	FileName directory(directory_text_field->GetValue());

	if (!directory.Exists()) {
		error_field->SetLabel("Output folder not found.");
		ok_button->Enable(false);
		return;
	}

	if (!directory.IsDirWritable()) {
		error_field->SetLabel("Output folder is not writable.");
		ok_button->Enable(false);
		return;
	}

	error_field->SetLabel(wxEmptyString);
	ok_button->Enable(true);
}

// ============================================================================
// Numkey forwarding text control

BEGIN_EVENT_TABLE(KeyForwardingTextCtrl, wxTextCtrl)
EVT_KEY_DOWN(KeyForwardingTextCtrl::OnKeyDown)
END_EVENT_TABLE()

void KeyForwardingTextCtrl::OnKeyDown(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_PAGEDOWN || event.GetKeyCode() == WXK_PAGEUP) {
		GetParent()->GetEventHandler()->AddPendingEvent(event);
	} else {
		event.Skip();
	}
}

// ============================================================================
// Find Item Dialog (Jump to item)

BEGIN_EVENT_TABLE(FindDialog, wxDialog)
EVT_TIMER(wxID_ANY, FindDialog::OnTextIdle)
EVT_TEXT(JUMP_DIALOG_TEXT, FindDialog::OnTextChange)
EVT_KEY_DOWN(FindDialog::OnKeyDown)
EVT_TEXT_ENTER(JUMP_DIALOG_TEXT, FindDialog::OnClickOK)
EVT_LISTBOX_DCLICK(JUMP_DIALOG_LIST, FindDialog::OnClickList)
EVT_BUTTON(wxID_OK, FindDialog::OnClickOK)
EVT_BUTTON(wxID_CANCEL, FindDialog::OnClickCancel)
END_EVENT_TABLE()

FindDialog::FindDialog(wxWindow* parent, wxString title) :
	wxDialog(g_gui.root, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX),
	idle_input_timer(this),
	result_brush(nullptr),
	result_id(0) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	search_field = newd KeyForwardingTextCtrl(this, JUMP_DIALOG_TEXT, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	search_field->SetFocus();
	sizer->Add(search_field, 0, wxEXPAND);

	item_list = newd FindDialogListBox(this, JUMP_DIALOG_LIST);
	item_list->SetMinSize(wxSize(470, 400));
	sizer->Add(item_list, wxSizerFlags(1).Expand().Border());

	wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
	stdsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	stdsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(stdsizer, wxSizerFlags(0).Center().Border());

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	// We can't call it here since it calls an abstract function, call in child constructors instead.
	// RefreshContents();
}

FindDialog::~FindDialog() = default;

void FindDialog::OnKeyDown(wxKeyEvent& event) {
	int w, h;
	item_list->GetSize(&w, &h);
	size_t amount = 1;

	switch (event.GetKeyCode()) {
		case WXK_PAGEUP:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_UP: {
			if (item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				if (n == wxNOT_FOUND) {
					n = 0;
				} else if (n != amount && n - amount < n) { // latter is needed for unsigned overflow
					n -= amount;
				} else {
					n = 0;
				}
				item_list->SetSelection(n);
			}
			break;
		}

		case WXK_PAGEDOWN:
			amount = h / 32 + 1;
			[[fallthrough]];
		case WXK_DOWN: {
			if (item_list->GetItemCount() > 0) {
				ssize_t n = item_list->GetSelection();
				size_t itemcount = item_list->GetItemCount();
				if (n == wxNOT_FOUND) {
					n = 0;
				} else if (static_cast<uint32_t>(n) < itemcount - amount && itemcount - amount < itemcount) {
					n += amount;
				} else {
					n = item_list->GetItemCount() - 1;
				}

				item_list->SetSelection(n);
			}
			break;
		}
		default:
			event.Skip();
			break;
	}
}

void FindDialog::OnTextIdle(wxTimerEvent& WXUNUSED(event)) {
	RefreshContents();
}

void FindDialog::OnTextChange(wxCommandEvent& WXUNUSED(event)) {
	idle_input_timer.Start(800, true);
}

void FindDialog::OnClickList(wxCommandEvent& event) {
	OnClickListInternal(event);
}

void FindDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	// This is to get virtual callback
	OnClickOKInternal();
}

void FindDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void FindDialog::RefreshContents() {
	// This is to get virtual callback
	RefreshContentsInternal();
}

// ============================================================================
// Find Brush Dialog (Jump to brush)

FindBrushDialog::FindBrushDialog(wxWindow* parent, wxString title) :
	FindDialog(parent, title) {
	RefreshContents();
}

FindBrushDialog::~FindBrushDialog() = default;

void FindBrushDialog::OnClickListInternal(wxCommandEvent& event) {
	Brush* brush = item_list->GetSelectedBrush();
	if (brush) {
		result_brush = brush;
		EndModal(1);
	}
}

void FindBrushDialog::OnClickOKInternal() {
	// This is kind of stupid as it would fail unless the "Please enter a search string" wasn't there
	if (item_list->GetItemCount() > 0) {
		if (item_list->GetSelection() == wxNOT_FOUND) {
			item_list->SetSelection(0);
		}
		Brush* brush = item_list->GetSelectedBrush();
		if (!brush) {
			// It's either "Please enter a search string" or "No matches"
			// Perhaps we can refresh now?
			std::string search_string = as_lower_str(nstr(search_field->GetValue()));
			bool do_search = (search_string.size() >= 2);

			if (do_search) {
				const BrushMap& map = g_brushes.getMap();
				for (BrushMap::const_iterator iter = map.begin(); iter != map.end(); ++iter) {
					const Brush* brush = iter->second;
					if (as_lower_str(brush->getName()).find(search_string) == std::string::npos) {
						continue;
					}

					// Don't match RAWs now.
					if (brush->isRaw()) {
						continue;
					}

					// Found one!
					result_brush = brush;
					break;
				}

				// Did we not find a matching brush?
				if (!result_brush) {
					// Then let's search the RAWs
					for (int id = 0; id <= g_items.getMaxID(); ++id) {
						ItemType& it = g_items[id];
						if (it.id == 0) {
							continue;
						}

						RAWBrush* raw_brush = it.raw_brush;
						if (!raw_brush) {
							continue;
						}

						if (as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos) {
							continue;
						}

						// Found one!
						result_brush = raw_brush;
						break;
					}
				}
				// Done!
			}
		} else {
			result_brush = brush;
		}
	}
	EndModal(1);
}

void FindBrushDialog::RefreshContentsInternal() {
	item_list->Clear();

	// We store the raws so they display last of all results
	bool found_search_results = false;

	std::string search_string = as_lower_str(nstr(search_field->GetValue()));
	std::vector<std::string> parts = splitString(search_string, '-');
	if (parts.size() == 2 && isInteger(parts[0]) && isInteger(parts[1])) {
		uint16_t id_from = std::stoi(parts[0]);
		uint16_t id_to = std::stoi(parts[1]);
		for (int id = 0; id <= g_items.getMaxID(); ++id) {
			ItemType& it = g_items[id];
			if (it.id == 0) {
				continue;
			}

			if (it.id < id_from || it.id > id_to) {
				continue;
			}

			RAWBrush* raw_brush = it.raw_brush;
			if (!raw_brush) {
				continue;
			}

			found_search_results = true;
			item_list->AddBrush(raw_brush);
		}
	} else if (!parts.empty()) {
		for (auto& search_text : parts) {
			const BrushMap& brushes_map = g_brushes.getMap();
			for (BrushMap::const_iterator iter = brushes_map.begin(); iter != brushes_map.end(); ++iter) {
				const Brush* brush = iter->second;
				if (!brush || as_lower_str(brush->getName()).find(search_text) == std::string::npos) {
					continue;
				}

				if (brush->isRaw()) {
					continue;
				}

				found_search_results = true;
				item_list->AddBrush(const_cast<Brush*>(brush));
			}

			for (int id = 0; id <= g_items.getMaxID(); ++id) {
				ItemType& it = g_items[id];
				if (it.id == 0) {
					continue;
				}

				RAWBrush* raw_brush = it.raw_brush;
				if (!raw_brush) {
					continue;
				}

				if (as_lower_str(raw_brush->getName()).find(search_text) == std::string::npos) {
					continue;
				}

				found_search_results = true;
				item_list->AddBrush(raw_brush);
			}
		}
	} else if (search_string.size() >= 2) {
		const BrushMap& brushes_map = g_brushes.getMap();
		for (BrushMap::const_iterator iter = brushes_map.begin(); iter != brushes_map.end(); ++iter) {
			const Brush* brush = iter->second;
			if (!brush || as_lower_str(brush->getName()).find(search_string) == std::string::npos) {
				continue;
			}

			if (brush->isRaw()) {
				continue;
			}

			found_search_results = true;
			item_list->AddBrush(const_cast<Brush*>(brush));
		}

		for (int id = 0; id <= g_items.getMaxID(); ++id) {
			ItemType& it = g_items[id];
			if (it.id == 0) {
				continue;
			}

			RAWBrush* raw_brush = it.raw_brush;
			if (!raw_brush) {
				continue;
			}

			if (as_lower_str(raw_brush->getName()).find(search_string) == std::string::npos) {
				continue;
			}

			found_search_results = true;
			item_list->AddBrush(raw_brush);
		}
	}

	if (found_search_results) {
		item_list->SetSelection(0);
	} else {
		item_list->SetNoMatches();
	}
	item_list->Refresh();
}

// ============================================================================
// Listbox in find item / brush stuff

FindDialogListBox::FindDialogListBox(wxWindow* parent, wxWindowID id) :
	wxVListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	cleared(false),
	no_matches(false) {
	Clear();
}

FindDialogListBox::~FindDialogListBox() {
	////
}

void FindDialogListBox::Clear() {
	cleared = true;
	no_matches = false;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::SetNoMatches() {
	cleared = false;
	no_matches = true;
	brushlist.clear();
	SetItemCount(1);
}

void FindDialogListBox::AddBrush(Brush* brush) {
	if (cleared || no_matches) {
		SetItemCount(0);
	}

	cleared = false;
	no_matches = false;

	SetItemCount(GetItemCount() + 1);
	brushlist.push_back(brush);
}

Brush* FindDialogListBox::GetSelectedBrush() {
	ssize_t n = GetSelection();
	if (n == wxNOT_FOUND || no_matches || cleared) {
		return nullptr;
	}
	return brushlist[n];
}

void FindDialogListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	if (no_matches) {
		dc.DrawText("No matches for your search.", rect.GetX() + 40, rect.GetY() + 6);
	} else if (cleared) {
		dc.DrawText("Please enter your search string.", rect.GetX() + 40, rect.GetY() + 6);
	} else {
		ASSERT(n < brushlist.size());
		Sprite* spr = g_gui.gfx.getSprite(brushlist[n]->getLookID());
		if (spr) {
			spr->DrawTo(&dc, SPRITE_SIZE_32x32, rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
		}

		if (IsSelected(n)) {
			if (HasFocus()) {
				dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
			} else {
				dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
			}
		} else {
			dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
		}

		dc.DrawText(wxstr(brushlist[n]->getName()), rect.GetX() + 40, rect.GetY() + 6);
	}
}

wxCoord FindDialogListBox::OnMeasureItem(size_t n) const {
	return 32;
}

// ============================================================================
// wxListBox that can be sorted

SortableListBox::SortableListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size) :
	wxListBox(parent, id, pos, size, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB) { }

SortableListBox::SortableListBox(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, int n, const wxString choices[], long style) :
	wxListBox(parent, id, pos, size, n, choices, style) { }

SortableListBox::~SortableListBox() { }

void SortableListBox::Sort() {

	if (GetCount() == 0) {
		return;
	}

	wxASSERT_MSG(GetClientDataType() != wxClientData_Object, "Sorting a list with data of type wxClientData_Object is currently not implemented");

	DoSort();
}

void SortableListBox::DoSort() {
	size_t count = GetCount();
	int selection = GetSelection();
	wxClientDataType dataType = GetClientDataType();

	wxArrayString stringList;
	wxArrayPtrVoid dataList;

	for (size_t i = 0; i < count; ++i) {
		stringList.Add(GetString(i));
		if (dataType == wxClientData_Void) {
			dataList.Add(GetClientData(i));
		}
	}

	// Insertion sort
	for (size_t i = 0; i < count; ++i) {
		size_t j = i;
		while (j > 0 && stringList[j].CmpNoCase(stringList[j - 1]) < 0) {

			wxString tmpString = stringList[j];
			stringList[j] = stringList[j - 1];
			stringList[j - 1] = tmpString;

			if (dataType == wxClientData_Void) {
				void* tmpData = dataList[j];
				dataList[j] = dataList[j - 1];
				dataList[j - 1] = tmpData;
			}

			if (selection == j - 1) {
				selection++;
			} else if (selection == j) {
				selection--;
			}

			j--;
		}
	}

	Freeze();
	Clear();
	for (size_t i = 0; i < count; ++i) {
		if (dataType == wxClientData_Void) {
			Append(stringList[i], dataList[i]);
		} else {
			Append(stringList[i]);
		}
	}
	Thaw();

	SetSelection(selection);
}

// ============================================================================
// Object properties base

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Item* item, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, title, position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(item),
	edit_creature(nullptr),
	edit_spawn(nullptr) {
	////
}

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Creature* creature, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, title, position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(nullptr),
	edit_creature(creature),
	edit_spawn(nullptr) {
	////
}

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, const Map* map, const Tile* tile, Spawn* spawn, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, title, position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER),
	edit_map(map),
	edit_tile(tile),
	edit_item(nullptr),
	edit_creature(nullptr),
	edit_spawn(spawn) {
	////
}

ObjectPropertiesWindowBase::ObjectPropertiesWindowBase(wxWindow* parent, wxString title, wxPoint position /* = wxDefaultPosition */) :
	wxDialog(parent, wxID_ANY, title, position, wxSize(600, 400), wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	////
}

Item* ObjectPropertiesWindowBase::getItemBeingEdited() {
	return edit_item;
}

// ============================================================================
// Edit Towns Dialog

BEGIN_EVENT_TABLE(EditTownsDialog, wxDialog)
EVT_LISTBOX(EDIT_TOWNS_LISTBOX, EditTownsDialog::OnListBoxChange)

EVT_BUTTON(EDIT_TOWNS_SELECT_TEMPLE, EditTownsDialog::OnClickSelectTemplePosition)
EVT_BUTTON(EDIT_TOWNS_ADD, EditTownsDialog::OnClickAdd)
EVT_BUTTON(EDIT_TOWNS_REMOVE, EditTownsDialog::OnClickRemove)
EVT_BUTTON(wxID_OK, EditTownsDialog::OnClickOK)
EVT_BUTTON(wxID_CANCEL, EditTownsDialog::OnClickCancel)
EVT_BUTTON(EDIT_TOWNS_EXPORT, EditTownsDialog::OnClickExport)
EVT_BUTTON(EDIT_TOWNS_IMPORT, EditTownsDialog::OnClickImport)
END_EVENT_TABLE()

EditTownsDialog::EditTownsDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Towns", wxDefaultPosition, wxSize(280, 380)),
	editor(editor)
{
	Map& map = editor.map;

	// Create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* tmpsizer;

	for (TownMap::const_iterator town_iter = map.towns.begin(); town_iter != map.towns.end(); ++town_iter) {
		Town* town = town_iter->second;
		town_list.push_back(newd Town(*town));
		if (max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	// Town list
	town_listbox = newd wxListBox(this, EDIT_TOWNS_LISTBOX, wxDefaultPosition, wxSize(240, 100));
	sizer->Add(town_listbox, 1, wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 10);

	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(newd wxButton(this, EDIT_TOWNS_ADD, "Add"), 0, wxTOP, 5);
	tmpsizer->Add(remove_button = newd wxButton(this, EDIT_TOWNS_REMOVE, "Remove"), 0, wxRIGHT | wxTOP, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// House options - town name and ID
	tmpsizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Name / ID");
	name_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(190, 20), 0, wxTextValidator(wxFILTER_ASCII, &town_name));
	tmpsizer->Add(name_field, 2, wxEXPAND | wxLEFT | wxBOTTOM, 5);

	id_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(40, 20), wxTE_READONLY, wxTextValidator(wxFILTER_NUMERIC, &town_id));
	id_field->Enable(false);
	tmpsizer->Add(id_field, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(tmpsizer, 0, wxEXPAND | wxALL, 10);

	// Temple position section
	// Create the PositionCtrl for temple position entry.
	// Note: PositionCtrl is a sizer so we cannot bind events directly to it.
	temple_position = newd PositionCtrl(this, "Temple Position", 0, 0, 0, map.getWidth(), map.getHeight());
	select_position_button = newd wxButton(this, EDIT_TOWNS_SELECT_TEMPLE, "Go To");
	temple_position->Add(select_position_button, 0, wxLEFT | wxRIGHT | wxBOTTOM, 5);
	sizer->Add(temple_position, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	// --- New: Extra text control for seamless temple position pasting ---
	// This control accepts a full position string (in any supported format) and updates the temple position fields.
	paste_temple_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
	paste_temple_field->SetHint("Paste temple position (e.g., {x = 0, y = 0, z = 0})");
	paste_temple_field->Bind(wxEVT_TEXT, &EditTownsDialog::OnPasteTempleText, this);
	sizer->Add(paste_temple_field, 0, wxEXPAND | wxALL, 5);
	// --- End new section ---

	// OK/Cancel buttons
	tmpsizer = newd wxBoxSizer(wxHORIZONTAL);
	tmpsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	tmpsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(tmpsizer, 0, wxCENTER | wxALL, 10);

	// Add Import/Export buttons
	wxBoxSizer* importExportSizer = new wxBoxSizer(wxHORIZONTAL);
	importExportSizer->Add(new wxButton(this, EDIT_TOWNS_IMPORT, "Import XML"), 0, wxRIGHT, 5);
	importExportSizer->Add(new wxButton(this, EDIT_TOWNS_EXPORT, "Export XML"), 0, wxLEFT, 5);
	sizer->Add(importExportSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	BuildListBox(true);
}

EditTownsDialog::~EditTownsDialog() {
	for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
		delete *town_iter;
	}
}

void EditTownsDialog::SynchronizeWithMap() {
	// Clear our local list and rebuild from the map
	for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
		delete *town_iter;
	}
	town_list.clear();
	
	// Rebuild from the actual map data
	for (TownMap::const_iterator town_iter = editor.map.towns.begin(); town_iter != editor.map.towns.end(); ++town_iter) {
		Town* town = town_iter->second;
		town_list.push_back(newd Town(*town));
		if (max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}
}

void EditTownsDialog::BuildListBox(bool doselect) {
	long tmplong = 0;
	max_town_id = 0;
	wxArrayString town_name_list;
	uint32_t selection_before = 0;

	// First synchronize with the map to ensure we have current data
	SynchronizeWithMap();

	if (doselect && id_field->GetValue().ToLong(&tmplong)) {
		uint32_t old_town_id = tmplong;

		for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			if (old_town_id == (*town_iter)->getID()) {
				selection_before = (*town_iter)->getID();
				break;
			}
		}
	}

	for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
		Town* town = *town_iter;
		town_name_list.Add(wxstr(town->getName()));
		if (max_town_id < town->getID()) {
			max_town_id = town->getID();
		}
	}

	town_listbox->Set(town_name_list);
	remove_button->Enable(town_listbox->GetCount() != 0);
	select_position_button->Enable(false);

	if (doselect) {
		if (selection_before) {
			int i = 0;
			for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
				if (selection_before == (*town_iter)->getID()) {
					town_listbox->SetSelection(i);
					return;
				}
				++i;
			}
		}
		UpdateSelection(0);
	}
}

void EditTownsDialog::UpdateSelection(int new_selection) {
	long tmplong;

	// Save old values
	if (town_list.size() > 0) {
		if (id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

		Town* old_town = nullptr;

		for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			if (old_town_id == (*town_iter)->getID()) {
				old_town = *town_iter;
				break;
			}
		}

		if (old_town) {
			editor.map.getOrCreateTile(old_town->getTemplePosition())->getLocation()->decreaseTownCount();

			Position templePos = temple_position->GetPosition();

			editor.map.getOrCreateTile(templePos)->getLocation()->increaseTownCount();

				old_town->setTemplePosition(templePos);

			wxString new_name = name_field->GetValue();
			wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
			
				if (new_name != old_name) {
					// Name has changed, update list
					BuildListBox(false);
				}
			}
		}
	}

	// Clear fields
	town_name.Clear();
	town_id.Clear();

	if (town_list.size() > size_t(new_selection)) {
		name_field->Enable(true);
		temple_position->Enable(true);
		select_position_button->Enable(true);

		// Change the values to reflect the newd selection
		Town* town = town_list[new_selection];
		ASSERT(town);

		// printf("Selected %d:%s\n", new_selection, town->getName().c_str());
		town_name << wxstr(town->getName());
		name_field->SetValue(town_name);
		town_id << long(town->getID());
		id_field->SetValue(town_id);
		temple_position->SetPosition(town->getTemplePosition());
		town_listbox->SetSelection(new_selection);
	} else {
		name_field->Enable(false);
		temple_position->Enable(false);
		select_position_button->Enable(false);
	}
	Refresh();
}

void EditTownsDialog::OnListBoxChange(wxCommandEvent& event) {
	UpdateSelection(event.GetSelection());
}

void EditTownsDialog::OnClickSelectTemplePosition(wxCommandEvent& WXUNUSED(event)) {
	Position templepos = temple_position->GetPosition();
	g_gui.SetScreenCenterPosition(templepos);
}

void EditTownsDialog::OnClickAdd(wxCommandEvent& WXUNUSED(event)) {
	// Find the next available ID
	uint32_t new_id = 1;
	while (editor.map.towns.getTown(new_id) != nullptr) {
		new_id++;
	}
	
	Town* new_town = newd Town(new_id);
	new_town->setName("Unnamed Town");
	new_town->setTemplePosition(Position(0, 0, 0));
	town_list.push_back(new_town);

	editor.map.getOrCreateTile(Position(0, 0, 0))->getLocation()->increaseTownCount();

	// Update max_town_id if necessary
	if (new_id > max_town_id) {
		max_town_id = new_id;
	}

	BuildListBox(false);
	UpdateSelection(town_list.size() - 1);
	town_listbox->SetSelection(town_list.size() - 1);
}

void EditTownsDialog::OnClickRemove(wxCommandEvent& WXUNUSED(event)) {
    int current_selection = town_listbox->GetSelection();
    
    if (current_selection == wxNOT_FOUND) {
        return;
    }

    // Clear the current selection to prevent interference
    town_listbox->SetSelection(wxNOT_FOUND);
    name_field->Clear();
    id_field->Clear();
    temple_position->SetPosition(Position(0, 0, 0));

    // Create and populate choices for the dialog
    wxArrayString choices;
    for (size_t i = 0; i < town_list.size(); i++) {
        wxString choice = wxString::Format("%s (ID: %d)", 
            wxstr(town_list[i]->getName()).c_str(), 
            town_list[i]->getID());
        choices.Add(choice);
    }

    // Create custom dialog with dropdown
    wxDialog* dialog = new wxDialog(this, wxID_ANY, "Remove Town", 
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(new wxStaticText(dialog, wxID_ANY, "Select town to remove:"), 
        0, wxALL, 5);
    
    wxChoice* townChoice = new wxChoice(dialog, wxID_ANY, 
        wxDefaultPosition, wxDefaultSize, choices);
    townChoice->SetSelection(current_selection);
    sizer->Add(townChoice, 0, wxEXPAND | wxALL, 5);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(dialog, wxID_OK, "OK"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(dialog, wxID_CANCEL, "Cancel"), 0, wxALL, 5);
    sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    dialog->SetSizer(sizer);
    dialog->Fit();

    if (dialog->ShowModal() == wxID_OK) {
        int selected_town_index = townChoice->GetSelection();
        if (selected_town_index != wxNOT_FOUND) {
            Town* town_to_remove = town_list[selected_town_index];
            uint32_t town_id_to_remove = town_to_remove->getID();
            
            // Count houses in this town
            int house_count = 0;
            for (HouseMap::iterator house_iter = editor.map.houses.begin(); 
                 house_iter != editor.map.houses.end(); ++house_iter) {
                if (house_iter->second->townid == town_id_to_remove) {
                    house_count++;
                }
            }

            if (house_count > 0) {
                wxString msg;
                msg << "This town has " << house_count << " house" << (house_count > 1 ? "s" : "") 
                    << " that will be removed.\n\nDo you want to remove the houses?";
                
                if (g_gui.PopupDialog(this, "Remove Houses", msg, wxYES | wxNO) == wxID_NO) {
                    dialog->Destroy();
                    UpdateSelection(current_selection);
                    return;
                }

                // Remove houses
                HouseMap::iterator house_iter = editor.map.houses.begin();
                while (house_iter != editor.map.houses.end()) {
                    if (house_iter->second->townid == town_id_to_remove) {
                        House* house = house_iter->second;
                        house->clean();
                        editor.map.houses.removeHouse(house);
                        house_iter = editor.map.houses.begin();
                    } else {
                        ++house_iter;
                    }
                }
            }

            // Remove town flag from tile
            editor.map.getOrCreateTile(town_to_remove->getTemplePosition())
                ->getLocation()->decreaseTownCount();

            // Remove the town from the list and delete it
            town_list.erase(town_list.begin() + selected_town_index);
            delete town_to_remove;
            BuildListBox(false);
            
            if (selected_town_index >= int(town_list.size())) {
                selected_town_index = town_list.size() - 1;
            }
            UpdateSelection(selected_town_index);
        }
    } else {
        UpdateSelection(current_selection);
    }
    
    dialog->Destroy();
}

void EditTownsDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	long tmplong = 0;

	if (Validate() && TransferDataFromWindow()) {
		// Save old values
		if (town_list.size() > 0 && id_field->GetValue().ToLong(&tmplong)) {
			uint32_t old_town_id = tmplong;

			Town* old_town = nullptr;

			for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
				if (old_town_id == (*town_iter)->getID()) {
					old_town = *town_iter;
					break;
				}
			}

			if (old_town) {
				editor.map.getOrCreateTile(old_town->getTemplePosition())->getLocation()->decreaseTownCount();

				Position templePos = temple_position->GetPosition();

				editor.map.getOrCreateTile(templePos)->getLocation()->increaseTownCount();

				old_town->setTemplePosition(templePos);

				wxString new_name = name_field->GetValue();
				wxString old_name = wxstr(old_town->getName());

				old_town->setName(nstr(new_name));
				
				if (new_name != old_name) {
					// Name has changed, update list
					BuildListBox(true);
				}
			}
		}

		Towns& towns = editor.map.towns;

		// Verify the newd information
		for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			Town* town = *town_iter;
			if (town->getName() == "") {
				g_gui.PopupDialog(this, "Error", "You can't have a town with an empty name.", wxOK);
				return;
			}
			if (!town->getTemplePosition().isValid() || town->getTemplePosition().x > editor.map.getWidth() || town->getTemplePosition().y > editor.map.getHeight()) {
				wxString msg;
				msg << "The town " << wxstr(town->getName()) << " has an invalid temple position.";
				g_gui.PopupDialog(this, "Error", msg, wxOK);
				return;
			}
		}

		// Clear old towns
		towns.clear();

		// Build the newd town map
		for (std::vector<Town*>::iterator town_iter = town_list.begin(); town_iter != town_list.end(); ++town_iter) {
			towns.addTown(*town_iter);
		}
		town_list.clear();
		editor.map.doChange();

		EndModal(1);
		g_gui.RefreshPalettes();
	}
}

void EditTownsDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

void EditTownsDialog::OnPasteTempleText(wxCommandEvent& event) {
	Position pos;
	// Retrieve pasted text from the additional paste field.
	std::string input = paste_temple_field->GetValue().ToStdString();
	// Attempt to parse input using supported position formats.
	if(posFromClipboard(pos, editor.map.getWidth(), editor.map.getHeight(), input)) {
		// Successfully parsed position; update the temple position fields.
		temple_position->SetPosition(pos);
		// Optionally, clear the paste field after successful parsing.
		paste_temple_field->Clear();
	}
	event.Skip();
}

void EditTownsDialog::OnClickExport(wxCommandEvent& WXUNUSED(event)) {
    int selection = town_listbox->GetSelection();
    if (selection == wxNOT_FOUND) {
        g_gui.PopupDialog(this, "Error", "Please select a town to export.", wxOK);
        return;
    }

    Town* town = town_list[selection];
    wxString defaultFileName = wxString::Format("%s.xml", wxstr(town->getName()));
    
    wxFileDialog dialog(this, "Export Town to XML", "", defaultFileName,
        "XML files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK) {
        ExportTownToXML(dialog.GetPath(), town);
    }
}

void EditTownsDialog::ExportTownToXML(const wxString& path, Town* town) {
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("town");
    
    // Export basic town information.
    root.append_attribute("id") = town->getID();
    root.append_attribute("name") = town->getName().c_str();
    
    Position temple_pos = town->getTemplePosition();
    pugi::xml_node temple = root.append_child("temple");
    temple.append_attribute("x") = temple_pos.x;
    temple.append_attribute("y") = temple_pos.y;
    temple.append_attribute("z") = temple_pos.z;
    
    // Export houses that belong to the selected town.
    pugi::xml_node houses = root.append_child("houses");
    for (HouseMap::iterator house_iter = editor.map.houses.begin(); 
         house_iter != editor.map.houses.end(); ++house_iter) {
        House* house = house_iter->second;
        if (house->townid == town->getID()) {
            pugi::xml_node house_node = houses.append_child("house");
            house_node.append_attribute("id") = house->getID();
            house_node.append_attribute("name") = house->name.c_str();
            
            Position exit = house->getExit();
            pugi::xml_node exit_node = house_node.append_child("exit");
            exit_node.append_attribute("x") = exit.x;
            exit_node.append_attribute("y") = exit.y;
            exit_node.append_attribute("z") = exit.z;
            
            // Export all tile positions associated with this house.
            pugi::xml_node tiles = house_node.append_child("tiles");
            const auto& tilePositions = house->getTilePositions();
            for (auto it = tilePositions.begin(); it != tilePositions.end(); ++it) {
                const Position& pos = *it;
                pugi::xml_node tile = tiles.append_child("tile");
                tile.append_attribute("x") = pos.x;
                tile.append_attribute("y") = pos.y;
                tile.append_attribute("z") = pos.z;
            }
        }
    }
    
    if (!doc.save_file(path.mb_str())) {
        g_gui.PopupDialog(this, "Error", "Failed to save town XML file.", wxOK);
    }
}

void EditTownsDialog::OnClickImport(wxCommandEvent& WXUNUSED(event)) {
    wxFileDialog dialog(this, "Import Town from XML", "", "",
        "XML files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dialog.ShowModal() == wxID_OK) {
        ImportTownFromXML(dialog.GetPath());
    }
}

void EditTownsDialog::ImportTownFromXML(const wxString& path) {
    pugi::xml_document doc;
    if (!doc.load_file(path.mb_str())) {
        g_gui.PopupDialog(this, "Error", "Failed to load town XML file.", wxOK);
        return;
    }
    
    pugi::xml_node root = doc.child("town");
    if (!root) {
        g_gui.PopupDialog(this, "Error", "Invalid town XML format.", wxOK);
        return;
    }
    
    // Create new town with next available ID
    uint32_t new_town_id = editor.map.towns.getEmptyID();
    Town* new_town = new Town(new_town_id);
    
    new_town->setName(root.attribute("name").as_string());
    
    pugi::xml_node temple = root.child("temple");
    if (temple) {
        Position temple_pos(
            temple.attribute("x").as_int(),
            temple.attribute("y").as_int(),
            temple.attribute("z").as_int()
        );
        new_town->setTemplePosition(temple_pos);
    }
    
    // Import houses
    pugi::xml_node houses = root.child("houses");
    if (houses) {
        for (pugi::xml_node house_node = houses.child("house"); 
             house_node; house_node = house_node.next_sibling("house")) {
            
            uint32_t new_house_id = editor.map.houses.getEmptyID();
            House* new_house = newd House(editor.map);
            new_house->setID(new_house_id);
            new_house->townid = new_town_id;
            new_house->name = house_node.attribute("name").as_string();
            
            pugi::xml_node exit = house_node.child("exit");
            if (exit) {
                Position exit_pos(
                    exit.attribute("x").as_int(),
                    exit.attribute("y").as_int(),
                    exit.attribute("z").as_int()
                );
                new_house->setExit(exit_pos);
            }
            
            // Import house tiles
            pugi::xml_node tiles = house_node.child("tiles");
            for (pugi::xml_node tile = tiles.child("tile"); 
                 tile; tile = tile.next_sibling("tile")) {
                Position pos(
                    tile.attribute("x").as_int(),
                    tile.attribute("y").as_int(),
                    tile.attribute("z").as_int()
                );
                Tile* map_tile = editor.map.getOrCreateTile(pos);
                if (map_tile) {
                    new_house->addTile(map_tile);
                }
            }
            
            editor.map.houses.addHouse(new_house);
        }
    }
    
    // Add town to map and update UI
    town_list.push_back(new_town);
    editor.map.towns.addTown(new_town);
    max_town_id = std::max(max_town_id, new_town_id);
    
    BuildListBox(true);
    UpdateSelection(town_list.size() - 1);
}



// ============================================================================
// Go To Position Dialog
// Jump to a position on the map by entering XYZ coordinates

BEGIN_EVENT_TABLE(GotoPositionDialog, wxDialog)
EVT_BUTTON(wxID_OK, GotoPositionDialog::OnClickOK)
EVT_BUTTON(wxID_CANCEL, GotoPositionDialog::OnClickCancel)
END_EVENT_TABLE()

GotoPositionDialog::GotoPositionDialog(wxWindow* parent, Editor& editor) :
	wxDialog(parent, wxID_ANY, "Go To Position", wxDefaultPosition, wxDefaultSize),
	editor(editor) {
	// Create topsizer
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	// Create a static box with instructions
	wxStaticBoxSizer* input_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Enter Position");
	
	// Add some example formats as static text
	wxString hint = "Supported formats:\n"
				   "{x = 0, y = 0, z = 0}\n"
				   "{\"x\":0,\"y\":0,\"z\":0}\n"
				   "x, y, z\n"
				   "(x, y, z)\n"
				   "Position(x, y, z)";
	input_sizer->Add(newd wxStaticText(this, wxID_ANY, hint), 0, wxALL, 5);

	// Create the input field
	position_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, 
								   wxSize(200, -1), wxTE_PROCESS_ENTER);
	position_field->SetHint("Enter position...");
	input_sizer->Add(position_field, 0, wxEXPAND | wxALL, 5);
	
	sizer->Add(input_sizer, 0, wxEXPAND | wxALL, 5);

	// OK/Cancel buttons
	wxSizer* button_sizer = newd wxBoxSizer(wxHORIZONTAL);
	button_sizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	button_sizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center());
	sizer->Add(button_sizer, 0, wxALL | wxCENTER, 5);

	// Bind events
	position_field->Bind(wxEVT_TEXT_ENTER, &GotoPositionDialog::OnClickOK, this);

	SetSizerAndFit(sizer);
	Centre(wxBOTH);
	
	// Set focus to the input field
	position_field->SetFocus();
}

void GotoPositionDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	Position pos;
	std::string input = position_field->GetValue().ToStdString();
	if (posFromClipboard(pos, editor.map.getWidth(), editor.map.getHeight(), input)) {
		g_gui.SetScreenCenterPosition(pos);
		EndModal(1);
	} else {
		g_gui.PopupDialog(this, "Error", "Invalid position format.", wxOK);
	}
}

void GotoPositionDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}
