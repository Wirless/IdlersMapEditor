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
#include "doodads_filling_dialog.h"
#include "gui.h"
#include "items.h"
#include "string_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>
#include <string>

enum {
	ID_ADD_DOODAD = wxID_HIGHEST + 1,
	ID_REMOVE_DOODAD,
	ID_DOODADS_LIST,
	ID_DOODAD_ID_SPIN,
	ID_DOODAD_CHANCE_SLIDER,
	ID_CLUMPING_SLIDER,
	ID_CLUMPING_FACTOR_SLIDER
};

BEGIN_EVENT_TABLE(DoodadsFillingDialog, wxDialog)
	EVT_BUTTON(ID_ADD_DOODAD, DoodadsFillingDialog::OnAddDoodad)
	EVT_BUTTON(ID_REMOVE_DOODAD, DoodadsFillingDialog::OnRemoveDoodad)
	EVT_LIST_ITEM_SELECTED(ID_DOODADS_LIST, DoodadsFillingDialog::OnDoodadItemSelected)
	EVT_LIST_ITEM_ACTIVATED(ID_DOODADS_LIST, DoodadsFillingDialog::OnDoodadItemActivated)
	EVT_BUTTON(wxID_OK, DoodadsFillingDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, DoodadsFillingDialog::OnClickCancel)
	EVT_CLOSE(DoodadsFillingDialog::OnClose)
END_EVENT_TABLE()

DoodadsFillingDialog::DoodadsFillingDialog(wxWindow* parent, const wxString& title) :
	wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(700, 600), wxDEFAULT_DIALOG_STYLE),
	selected_doodad_index(-1) {
	
	SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Tile selection section
	wxStaticBoxSizer* tile_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Target Tiles"), wxVERTICAL);
	
	wxStaticText* tile_label = new wxStaticText(tile_sizer->GetStaticBox(), wxID_ANY, "Tile ID Ranges:");
	tile_sizer->Add(tile_label, 0, wxALL, 5);
	
	tile_ranges_input = new wxTextCtrl(tile_sizer->GetStaticBox(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0);
	tile_ranges_input->SetToolTip("Enter tile IDs or ranges separated by commas (e.g., 100-105,120,150-155)");
	tile_sizer->Add(tile_ranges_input, 0, wxALL | wxEXPAND, 5);
	
	main_sizer->Add(tile_sizer, 0, wxALL | wxEXPAND, 5);

	// Doodad items section
	wxStaticBoxSizer* doodads_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Doodad Items"), wxVERTICAL);
	
	// List of doodads
	doodads_list = new wxListCtrl(doodads_sizer->GetStaticBox(), ID_DOODADS_LIST, wxDefaultPosition, wxSize(-1, 150), wxLC_REPORT | wxLC_SINGLE_SEL);
	doodads_list->AppendColumn("ID", wxLIST_FORMAT_LEFT, 60);
	doodads_list->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
	doodads_list->AppendColumn("Chance (%)", wxLIST_FORMAT_RIGHT, 80);
	doodads_sizer->Add(doodads_list, 1, wxALL | wxEXPAND, 5);
	
	// Doodad controls
	wxBoxSizer* doodad_controls_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	wxStaticText* id_label = new wxStaticText(doodads_sizer->GetStaticBox(), wxID_ANY, "Item ID:");
	doodad_controls_sizer->Add(id_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	doodad_id_spin = new wxSpinCtrl(doodads_sizer->GetStaticBox(), ID_DOODAD_ID_SPIN, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, g_items.getMaxID(), 100);
	doodad_controls_sizer->Add(doodad_id_spin, 0, wxALL, 5);
	
	wxStaticText* chance_label_static = new wxStaticText(doodads_sizer->GetStaticBox(), wxID_ANY, "Chance:");
	doodad_controls_sizer->Add(chance_label_static, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	doodad_chance_slider = new wxSlider(doodads_sizer->GetStaticBox(), ID_DOODAD_CHANCE_SLIDER, 100, 1, 100, wxDefaultPosition, wxSize(100, -1));
	doodad_controls_sizer->Add(doodad_chance_slider, 0, wxALL, 5);
	
	doodad_chance_label = new wxStaticText(doodads_sizer->GetStaticBox(), wxID_ANY, "100%");
	doodad_controls_sizer->Add(doodad_chance_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	doodads_sizer->Add(doodad_controls_sizer, 0, wxALL | wxEXPAND, 5);
	
	// Add/Remove buttons
	wxBoxSizer* doodad_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	add_doodad_button = new wxButton(doodads_sizer->GetStaticBox(), ID_ADD_DOODAD, "Add Doodad");
	doodad_buttons_sizer->Add(add_doodad_button, 0, wxALL, 5);
	
	remove_doodad_button = new wxButton(doodads_sizer->GetStaticBox(), ID_REMOVE_DOODAD, "Remove Selected");
	remove_doodad_button->Enable(false);
	doodad_buttons_sizer->Add(remove_doodad_button, 0, wxALL, 5);
	
	doodads_sizer->Add(doodad_buttons_sizer, 0, wxALL, 5);
	
	main_sizer->Add(doodads_sizer, 1, wxALL | wxEXPAND, 5);

	// Placement parameters section
	wxStaticBoxSizer* params_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Placement Parameters"), wxVERTICAL);
	
	// Clumping
	wxBoxSizer* clumping_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* clumping_label_static = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "Clumping:");
	clumping_sizer->Add(clumping_label_static, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	clumping_slider = new wxSlider(params_sizer->GetStaticBox(), ID_CLUMPING_SLIDER, 50, 0, 100, wxDefaultPosition, wxSize(150, -1));
	clumping_sizer->Add(clumping_slider, 0, wxALL, 5);
	
	clumping_label = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "50%");
	clumping_sizer->Add(clumping_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	params_sizer->Add(clumping_sizer, 0, wxALL | wxEXPAND, 5);
	
	// Clumping Factor
	wxBoxSizer* clumping_factor_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* clumping_factor_label_static = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "Clumping Factor:");
	clumping_factor_sizer->Add(clumping_factor_label_static, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	clumping_factor_slider = new wxSlider(params_sizer->GetStaticBox(), ID_CLUMPING_FACTOR_SLIDER, 150, 100, 300, wxDefaultPosition, wxSize(150, -1));
	clumping_factor_sizer->Add(clumping_factor_slider, 0, wxALL, 5);
	
	clumping_factor_label = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "1.50");
	clumping_factor_sizer->Add(clumping_factor_label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	params_sizer->Add(clumping_factor_sizer, 0, wxALL | wxEXPAND, 5);
	
	// Spacing
	wxBoxSizer* spacing_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText* spacing_label_static = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "Spacing:");
	spacing_sizer->Add(spacing_label_static, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	spacing_spin = new wxSpinCtrl(params_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 2);
	spacing_sizer->Add(spacing_spin, 0, wxALL, 5);
	
	wxStaticText* spacing_help = new wxStaticText(params_sizer->GetStaticBox(), wxID_ANY, "(minimum tiles between placements)");
	spacing_sizer->Add(spacing_help, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
	
	params_sizer->Add(spacing_sizer, 0, wxALL | wxEXPAND, 5);
	
	main_sizer->Add(params_sizer, 0, wxALL | wxEXPAND, 5);

	// Options section
	wxStaticBoxSizer* options_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Options"), wxVERTICAL);
	
	place_on_selection = new wxCheckBox(options_sizer->GetStaticBox(), wxID_ANY, "Place only within current selection");
	place_on_selection->SetToolTip("If checked, only places doodads on matching tiles within the current selection. Otherwise, places on all matching tiles in the map.");
	options_sizer->Add(place_on_selection, 0, wxALL, 5);
	
	main_sizer->Add(options_sizer, 0, wxALL | wxEXPAND, 5);

	// Buttons
	buttons_sizer = new wxStdDialogButtonSizer();
	ok_button = new wxButton(this, wxID_OK);
	buttons_sizer->AddButton(ok_button);
	cancel_button = new wxButton(this, wxID_CANCEL);
	buttons_sizer->AddButton(cancel_button);
	buttons_sizer->Realize();
	
	main_sizer->Add(buttons_sizer, 0, wxALIGN_CENTER | wxALL, 5);

	SetSizer(main_sizer);
	Layout();

	// Bind slider events to update labels
	Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		doodad_chance_label->SetLabel(wxString::Format("%d%%", doodad_chance_slider->GetValue()));
	}, ID_DOODAD_CHANCE_SLIDER);
	
	Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		clumping_label->SetLabel(wxString::Format("%d%%", clumping_slider->GetValue()));
	}, ID_CLUMPING_SLIDER);
	
	Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
		clumping_factor_label->SetLabel(wxString::Format("%.2f", clumping_factor_slider->GetValue() / 100.0));
	}, ID_CLUMPING_FACTOR_SLIDER);

	Center();
}

DoodadsFillingDialog::~DoodadsFillingDialog() {
	// Destructor
}

void DoodadsFillingDialog::OnAddDoodad(wxCommandEvent& event) {
	uint16_t id = doodad_id_spin->GetValue();
	int chance = doodad_chance_slider->GetValue();
	
	// Get item name
	wxString name;
	ItemType& it = g_items[id];
	if (it.id != 0) {
		name = wxstr(it.name);
	} else {
		name = wxString::Format("Unknown Item %d", id);
	}
	
	// Check if item already exists
	for (const auto& item : doodad_items) {
		if (item.id == id) {
			wxMessageBox("This item is already in the list!", "Duplicate Item", wxOK | wxICON_WARNING, this);
			return;
		}
	}
	
	doodad_items.emplace_back(id, chance, name);
	RefreshDoodadsList();
}

void DoodadsFillingDialog::OnRemoveDoodad(wxCommandEvent& event) {
	if (selected_doodad_index >= 0 && selected_doodad_index < (int)doodad_items.size()) {
		doodad_items.erase(doodad_items.begin() + selected_doodad_index);
		selected_doodad_index = -1;
		remove_doodad_button->Enable(false);
		RefreshDoodadsList();
	}
}

void DoodadsFillingDialog::OnDoodadItemSelected(wxListEvent& event) {
	selected_doodad_index = event.GetIndex();
	remove_doodad_button->Enable(true);
	
	if (selected_doodad_index >= 0 && selected_doodad_index < (int)doodad_items.size()) {
		const DoodadItem& item = doodad_items[selected_doodad_index];
		doodad_id_spin->SetValue(item.id);
		doodad_chance_slider->SetValue(item.chance);
		doodad_chance_label->SetLabel(wxString::Format("%d%%", item.chance));
	}
}

void DoodadsFillingDialog::OnDoodadItemActivated(wxListEvent& event) {
	// Double-click to edit - could be implemented later
}

void DoodadsFillingDialog::OnClickOK(wxCommandEvent& event) {
	// Validate input
	wxString tileRanges = tile_ranges_input->GetValue();
	tileRanges.Trim();
	if (tileRanges.IsEmpty()) {
		wxMessageBox("Please specify tile ID ranges!", "Input Required", wxOK | wxICON_WARNING, this);
		return;
	}
	
	// Try to parse ranges
	std::string rangesStr = std::string(tileRanges.mb_str());
	std::vector<std::pair<uint16_t, uint16_t>> ranges = parseIdRangesString(rangesStr);
	if (ranges.empty()) {
		wxMessageBox("Invalid tile range format!\nUse format like: 100-105,120,150-155", "Invalid Format", wxOK | wxICON_ERROR, this);
		return;
	}
	
	if (doodad_items.empty()) {
		wxMessageBox("Please add at least one doodad item!", "No Doodads", wxOK | wxICON_WARNING, this);
		return;
	}
	
	EndModal(wxID_OK);
}

void DoodadsFillingDialog::OnClickCancel(wxCommandEvent& event) {
	EndModal(wxID_CANCEL);
}

void DoodadsFillingDialog::OnClose(wxCloseEvent& event) {
	EndModal(wxID_CANCEL);
}

void DoodadsFillingDialog::RefreshDoodadsList() {
	doodads_list->DeleteAllItems();
	
	for (size_t i = 0; i < doodad_items.size(); ++i) {
		const DoodadItem& item = doodad_items[i];
		long index = doodads_list->InsertItem(i, wxString::Format("%d", item.id));
		doodads_list->SetItem(index, 1, item.name);
		doodads_list->SetItem(index, 2, wxString::Format("%d%%", item.chance));
	}
}

void DoodadsFillingDialog::UpdateSelectedDoodad() {
	if (selected_doodad_index >= 0 && selected_doodad_index < (int)doodad_items.size()) {
		DoodadItem& item = doodad_items[selected_doodad_index];
		item.id = doodad_id_spin->GetValue();
		item.chance = doodad_chance_slider->GetValue();
		
		// Update name
		ItemType& it = g_items[item.id];
		if (it.id != 0) {
			item.name = wxstr(it.name);
		} else {
			item.name = wxString::Format("Unknown Item %d", item.id);
		}
		
		RefreshDoodadsList();
	}
}

 