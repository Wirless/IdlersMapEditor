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

#ifndef RME_DOODADS_FILLING_DIALOG_H_
#define RME_DOODADS_FILLING_DIALOG_H_

#include <wx/radiobox.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/slider.h>
#include <vector>

class DoodadsFillingDialog : public wxDialog {
public:
	struct DoodadItem {
		uint16_t id;
		int chance;
		wxString name;
		
		DoodadItem(uint16_t id = 0, int chance = 100, const wxString& name = wxEmptyString) 
			: id(id), chance(chance), name(name) {}
	};

	DoodadsFillingDialog(wxWindow* parent, const wxString& title);
	virtual ~DoodadsFillingDialog();

	// Configuration getters
	wxString GetTileRanges() const { return tile_ranges_input->GetValue(); }
	std::vector<DoodadItem> GetDoodadItems() const { return doodad_items; }
	int GetClumping() const { return clumping_slider->GetValue(); }
	double GetClumpingFactor() const { return clumping_factor_slider->GetValue() / 100.0; }
	int GetSpacing() const { return spacing_spin->GetValue(); }
	bool GetPlaceOnSelection() const { return place_on_selection->GetValue(); }

protected:
	void OnAddDoodad(wxCommandEvent& event);
	void OnRemoveDoodad(wxCommandEvent& event);
	void OnDoodadItemSelected(wxListEvent& event);
	void OnDoodadItemActivated(wxListEvent& event);
	void OnClickOK(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

private:
	void RefreshDoodadsList();
	void UpdateSelectedDoodad();

	// Tile selection controls
	wxTextCtrl* tile_ranges_input;
	
	// Doodad items configuration
	wxListCtrl* doodads_list;
	wxButton* add_doodad_button;
	wxButton* remove_doodad_button;
	wxSpinCtrl* doodad_id_spin;
	wxSlider* doodad_chance_slider;
	wxStaticText* doodad_chance_label;
	
	// Placement parameters
	wxSlider* clumping_slider;
	wxStaticText* clumping_label;
	wxSlider* clumping_factor_slider;
	wxStaticText* clumping_factor_label;
	wxSpinCtrl* spacing_spin;
	
	// Options
	wxCheckBox* place_on_selection;
	
	// Buttons
	wxStdDialogButtonSizer* buttons_sizer;
	wxButton* ok_button;
	wxButton* cancel_button;

	// Data
	std::vector<DoodadItem> doodad_items;
	int selected_doodad_index;

	DECLARE_EVENT_TABLE()
};

#endif // RME_DOODADS_FILLING_DIALOG_H_ 