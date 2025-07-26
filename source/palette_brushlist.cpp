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

#include <set>
#include <algorithm> // Add this for std::min
#include "palette_brushlist.h"
#include "gui.h"
#include "brush.h"
#include "raw_brush.h"
#include "add_tileset_window.h"
#include "add_item_window.h"
#include "materials.h"
#include "border_editor_window.h"

// Define BrushPanelState class at the top of the file
class BrushPanelState {
public:
	BrushBoxInterface* grid_view;
	BrushBoxInterface* list_view;
	wxBoxSizer* zoom_sizer;
	wxStaticText* zoom_value_label;
	bool grid_view_shown;
	
	BrushPanelState() : grid_view(nullptr), list_view(nullptr), zoom_sizer(nullptr), 
					   zoom_value_label(nullptr), grid_view_shown(false) {}
					   
	~BrushPanelState() {}
};

// Keep a static map of constructed brush panels by tileset
std::map<const TilesetCategory*, BrushPanelState> g_brush_panel_cache;

// ============================================================================
// Brush Palette Panel
// A common class for terrain/doodad/item/raw palette

BEGIN_EVENT_TABLE(BrushPalettePanel, PalettePanel)
	EVT_BUTTON(wxID_ADD, BrushPalettePanel::OnClickAddItemTileset)
	EVT_BUTTON(wxID_NEW, BrushPalettePanel::OnClickAddTileset)
	EVT_BUTTON(BUTTON_QUICK_ADD_ITEM, BrushPalettePanel::OnClickQuickAddItemTileset)
	EVT_BUTTON(BUTTON_ADD_BORDER, BrushPalettePanel::OnClickCreateBorder)
	EVT_CHOICEBOOK_PAGE_CHANGING(wxID_ANY, BrushPalettePanel::OnSwitchingPage)
	EVT_CHOICEBOOK_PAGE_CHANGED(wxID_ANY, BrushPalettePanel::OnPageChanged)
END_EVENT_TABLE()

BrushPalettePanel::BrushPalettePanel(wxWindow* parent, const TilesetContainer& tilesets, TilesetCategoryType category, wxWindowID id) :
	PalettePanel(parent, id),
	palette_type(category),
	choicebook(nullptr),
	size_panel(nullptr),
	quick_add_button(nullptr),
	last_tileset_name("") {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	// Create the tileset panel
	wxSizer* ts_sizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Tileset");
	wxChoicebook* tmp_choicebook = newd wxChoicebook(this, wxID_ANY, wxDefaultPosition, wxSize(180, 250));
	ts_sizer->Add(tmp_choicebook, 1, wxEXPAND);
	topsizer->Add(ts_sizer, 1, wxEXPAND);

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
		// Create a vertical sizer to hold the two rows of buttons
		wxSizer* buttonSizer = newd wxBoxSizer(wxVERTICAL);
		
		// First row - Add Tileset and Add Item
		wxSizer* firstRowSizer = newd wxBoxSizer(wxHORIZONTAL);
		wxButton* buttonAddTileset = newd wxButton(this, wxID_NEW, "Add new Tileset");
		firstRowSizer->Add(buttonAddTileset, wxSizerFlags(1).Expand());
		
		wxButton* buttonAddItemToTileset = newd wxButton(this, wxID_ADD, "Add new Item");
		firstRowSizer->Add(buttonAddItemToTileset, wxSizerFlags(1).Expand());
		
		// Add first row to the button sizer
		buttonSizer->Add(firstRowSizer, wxSizerFlags(0).Expand());
		
		// Add a small space between rows
		buttonSizer->AddSpacer(5);
		
		// Second row - Quick Add Item and Create Border
		wxSizer* secondRowSizer = newd wxBoxSizer(wxHORIZONTAL);
		
		// Create the Quick Add Item button, initially disabled
		quick_add_button = newd wxButton(this, BUTTON_QUICK_ADD_ITEM, "Quick Add Item");
		quick_add_button->SetToolTip("Quickly add the currently selected brush to the last used tileset");
		quick_add_button->Enable(false); // Disabled until a tileset is added
		secondRowSizer->Add(quick_add_button, wxSizerFlags(1).Expand());
		
		// Create Border button
		wxButton* buttonCreateBorder = newd wxButton(this, BUTTON_ADD_BORDER, "Create Border");
		buttonCreateBorder->SetToolTip("Open the Border Editor to create or edit auto-borders");
		secondRowSizer->Add(buttonCreateBorder, wxSizerFlags(1).Expand());
		
		// Add second row to the button sizer
		buttonSizer->Add(secondRowSizer, wxSizerFlags(0).Expand());
		
		// Add the complete button sizer to the top sizer
		topsizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 5);
	}

	for (TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		const TilesetCategory* tcg = iter->second->getCategory(category);
		if (tcg && tcg->size() > 0) {
			BrushPanel* panel = newd BrushPanel(tmp_choicebook);
			panel->AssignTileset(tcg);
			tmp_choicebook->AddPage(panel, wxstr(iter->second->name));
		}
	}

	SetSizerAndFit(topsizer);

	choicebook = tmp_choicebook;
}

BrushPalettePanel::~BrushPalettePanel() {
	// Ensure all caches are destroyed
	DestroyAllCaches();
}

void BrushPalettePanel::DestroyAllCaches() {
	// Force cleanup of all panels to prevent memory leaks when application exits
	if (choicebook) {
		for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
			BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
			if (panel) {
				panel->InvalidateContents();
			}
		}
	}
	
	// Clear remembered brushes
	remembered_brushes.clear();
}

void BrushPalettePanel::InvalidateContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->InvalidateContents();
	}
	PalettePanel::InvalidateContents();
}

void BrushPalettePanel::LoadCurrentContents() {
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
	}
	PalettePanel::LoadCurrentContents();
}

void BrushPalettePanel::LoadAllContents() {
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->LoadContents();
	}
	PalettePanel::LoadAllContents();
}

PaletteType BrushPalettePanel::GetType() const {
	return palette_type;
}

void BrushPalettePanel::SetListType(wxString ltype) {
	if (!choicebook) {
		return;
	}
	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		panel->SetListType(ltype);
	}
}

Brush* BrushPalettePanel::GetSelectedBrush() const {
	if (!choicebook) {
		return nullptr;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	Brush* res = nullptr;
	if (panel) {
		for (ToolBarList::const_iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			res = (*iter)->GetSelectedBrush();
			if (res) {
				return res;
			}
		}
		res = panel->GetSelectedBrush();
	}
	return res;
}

void BrushPalettePanel::SelectFirstBrush() {
	if (!choicebook) {
		return;
	}
	wxWindow* page = choicebook->GetCurrentPage();
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	panel->SelectFirstBrush();
}

bool BrushPalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!choicebook) {
		return false;
	}

	BrushPanel* panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (!panel) {
		return false;
	}

	for (PalettePanel* toolBar : tool_bars) {
		if (toolBar->SelectBrush(whatbrush)) {
			panel->SelectBrush(nullptr);
			return true;
		}
	}

	for (PalettePanel* toolBar : tool_bars) {
		toolBar->DeselectAll();
	}

	if (panel->SelectBrush(whatbrush)) {
		for (PalettePanel* toolBar : tool_bars) {
			toolBar->SelectBrush(nullptr);
		}
		return true;
	}

	for (size_t iz = 0; iz < choicebook->GetPageCount(); ++iz) {
		if ((int)iz == choicebook->GetSelection()) {
			continue;
		}

		panel = dynamic_cast<BrushPanel*>(choicebook->GetPage(iz));
		if (panel && panel->SelectBrush(whatbrush)) {
			choicebook->ChangeSelection(iz);
			for (PalettePanel* toolBar : tool_bars) {
				toolBar->SelectBrush(nullptr);
			}
			return true;
		}
	}
	return false;
}

void BrushPalettePanel::OnSwitchingPage(wxChoicebookEvent& event) {
	event.Skip();
	if (!choicebook) {
		return;
	}
	
	// Get the old panel and clean it up
	BrushPanel* old_panel = dynamic_cast<BrushPanel*>(choicebook->GetCurrentPage());
	if (old_panel) {
		old_panel->OnSwitchOut();
		
		// Store selected brushes for later restoration
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			Brush* tmp = (*iter)->GetSelectedBrush();
			if (tmp) {
				remembered_brushes[old_panel] = tmp;
			}
		}
	}

	// Get the new panel and prepare it
	wxWindow* page = choicebook->GetPage(event.GetSelection());
	BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
	if (panel) {
		panel->OnSwitchIn();
		
		// Restore remembered brush selection if any
		for (ToolBarList::iterator iter = tool_bars.begin(); iter != tool_bars.end(); ++iter) {
			(*iter)->SelectBrush(remembered_brushes[panel]);
		}
	}
}

void BrushPalettePanel::OnPageChanged(wxChoicebookEvent& event) {
	if (!choicebook) {
		return;
	}
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void BrushPalettePanel::OnSwitchIn() {
	LoadCurrentContents();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSizeInternal(last_brush_size);
	OnUpdateBrushSize(g_gui.GetBrushShape(), last_brush_size);
}

void BrushPalettePanel::OnClickAddTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}

	wxDialog* w = newd AddTilesetWindow(g_gui.root, palette_type);
	int ret = w->ShowModal();
	w->Destroy();

	if (ret != 0) {
		g_gui.DestroyPalettes();
		g_gui.NewPalette();
	}
}

void BrushPalettePanel::OnClickAddItemTileset(wxCommandEvent& WXUNUSED(event)) {
	if (!choicebook) {
		return;
	}
	std::string tilesetName = choicebook->GetPageText(choicebook->GetSelection());

	auto _it = g_materials.tilesets.find(tilesetName);
	if (_it != g_materials.tilesets.end()) {
		// Get the currently selected brush
		Brush* brush = GetSelectedBrush();
		uint16_t item_id = 0;
		
		// Try to get the item ID from the brush if it's a RAW brush
		if (brush && brush->isRaw()) {
			RAWBrush* raw_brush = brush->asRaw();
			if (raw_brush) {
				item_id = raw_brush->getItemID();
			}
		}
		
		// Create the Add Item Window
		wxDialog* w = newd AddItemWindow(g_gui.root, palette_type, _it->second);
		
		// If we have a valid item ID, set it in the window
		if (item_id > 0) {
			AddItemWindow* addItemWindow = static_cast<AddItemWindow*>(w);
			addItemWindow->SetItemIdToItemButton(item_id);
		}
		
		int ret = w->ShowModal();
		
		// Get the selected tileset name from the dialog
		AddItemWindow* addItemWindow = static_cast<AddItemWindow*>(w);
		if (addItemWindow->tileset_choice && 
			addItemWindow->tileset_choice->GetSelection() != wxNOT_FOUND) {
			tilesetName = nstr(addItemWindow->tileset_choice->GetString(
				addItemWindow->tileset_choice->GetSelection()));
		}
		
		w->Destroy();

		if (ret != 0) {
			// Item was successfully added, store the tileset name for Quick Add
			last_tileset_name = tilesetName;
			
			// Enable the Quick Add button
			if (quick_add_button) {
				quick_add_button->Enable(true);
			}
			
			g_gui.RebuildPalettes();
		}
	}
}

void BrushPalettePanel::OnClickQuickAddItemTileset(wxCommandEvent& WXUNUSED(event)) {
	// Check if we have a last used tileset name
	if (last_tileset_name.empty()) {
		g_gui.PopupDialog("Error", "No tileset has been used yet. Please use Add Item first.", wxOK);
		return;
	}
	
	// Get the currently selected brush
	Brush* brush = GetSelectedBrush();
	if (!brush) {
		g_gui.PopupDialog("Error", "No brush is currently selected.", wxOK);
		return;
	}
	
	// Check if the brush is a RAW brush that we can add to the tileset
	if (!brush->isRaw()) {
		g_gui.PopupDialog("Error", "Only raw items can be added to tilesets.", wxOK);
		return;
	}
	
	RAWBrush* raw_brush = brush->asRaw();
	if (!raw_brush) {
		g_gui.PopupDialog("Error", "Failed to get item data from the selected brush.", wxOK);
		return;
	}
	
	uint16_t item_id = raw_brush->getItemID();
	
	// Check if the tileset still exists
	auto it = g_materials.tilesets.find(last_tileset_name);
	if (it == g_materials.tilesets.end()) {
		g_gui.PopupDialog("Error", "The last used tileset no longer exists.", wxOK);
		last_tileset_name = "";
		if (quick_add_button) {
			quick_add_button->Enable(false);
		}
		return;
	}
	
	// Add the item to the tileset
	g_materials.addToTileset(last_tileset_name, item_id, palette_type);
	g_materials.modify();
	
	// Show success message with the item name and ID
	const ItemType& item_type = g_items.getItemType(item_id);
	g_gui.PopupDialog("Item Added", wxString::Format("Item '%s' (ID: %d) has been added to tileset '%s'", 
		wxString(item_type.name.c_str()), item_id, wxString(last_tileset_name.c_str())), wxOK);
	
	// Rebuild palettes to show the changes
	g_gui.RebuildPalettes();
}

void BrushPalettePanel::OnClickCreateBorder(wxCommandEvent& WXUNUSED(event)) {
	// Open the Border Editor to create or edit auto-borders
	BorderEditorDialog* dialog = new BorderEditorDialog(g_gui.root, "Auto Border Editor");
	dialog->Show();
	
	// After editing borders, refresh view to show any changes
	g_gui.RefreshView();
}

const BrushPalettePanel::SelectionInfo& BrushPalettePanel::GetSelectionInfo() const {
	static SelectionInfo selection;
	selection.brushes.clear();
	
	// First add the currently selected brush if available
	Brush* selected = GetSelectedBrush();
	if (selected) {
		selection.brushes.push_back(selected);
	}
	
	// Now, depending on the panel type, try to get more brushes
	if (choicebook) {
		wxWindow* page = choicebook->GetCurrentPage();
		BrushPanel* panel = dynamic_cast<BrushPanel*>(page);
		if (panel) {
			// Here we could add additional brushes based on multi-selection
			// if implemented in the various panel types
		}
	}
	
	return selection;
}

// ============================================================================
// Brush Panel
// A container of brush buttons

BEGIN_EVENT_TABLE(BrushPanel, wxPanel)
EVT_LISTBOX(wxID_ANY, BrushPanel::OnClickListBoxRow)
EVT_CHECKBOX(wxID_ANY, BrushPanel::OnViewModeToggle)
END_EVENT_TABLE()

BrushPanel::BrushPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	tileset(nullptr),
	brushbox(nullptr),
	loaded(false),
	list_type(BRUSHLIST_LISTBOX),
	view_mode_toggle(nullptr),
	view_type_choice(nullptr),
	show_ids_toggle(nullptr) {
	
	sizer = new wxBoxSizer(wxVERTICAL);
	
	// Add view mode toggle checkbox
	view_mode_toggle = new wxCheckBox(this, wxID_ANY, "Grid View");
	view_mode_toggle->SetValue(false);
	sizer->Add(view_mode_toggle, 0, wxALL, 5);
	
	// Always add the Show Item IDs checkbox right after the Grid View checkbox
	show_ids_toggle = new wxCheckBox(this, wxID_ANY, "Show Item IDs");
	show_ids_toggle->SetValue(false);
	show_ids_toggle->Bind(wxEVT_CHECKBOX, &BrushPanel::OnShowItemIDsToggle, this);
	sizer->Add(show_ids_toggle, 0, wxALL, 5);
	
	// Add a choice for view types if we're in RAW palette
	if(tileset && tileset->getType() == TILESET_RAW) {
		wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* label = new wxStaticText(this, wxID_ANY, "View Type:");
		choice_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		view_type_choice = new wxChoice(this, wxID_ANY);
		view_type_choice->Append("Normal");
		view_type_choice->Append("Direct Draw");
		view_type_choice->SetSelection(0);
		view_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& event) {
			if(loaded) {
				LoadViewMode();
			}
		});
		choice_sizer->Add(view_type_choice, 1);
		sizer->Add(choice_sizer, 0, wxEXPAND | wxALL, 5);
	} else {
		view_type_choice = nullptr;
	}
	
	SetSizer(sizer);
}

BrushPanel::~BrushPanel() {
	// Cleanup and remove any cached panels for this tileset
	if (tileset && g_brush_panel_cache.count(tileset) > 0) {
		BrushPanelState& state = g_brush_panel_cache[tileset];
		if (state.grid_view) {
			CleanupBrushbox(state.grid_view);
			state.grid_view = nullptr;
		}
		if (state.list_view) {
			CleanupBrushbox(state.list_view);
			state.list_view = nullptr;
		}
		g_brush_panel_cache.erase(tileset);
	}
}

void BrushPanel::AssignTileset(const TilesetCategory* _tileset) {
	if (_tileset != tileset) {
		InvalidateContents();
		tileset = _tileset;
	}
}

void BrushPanel::SetListType(BrushListType ltype) {
	if (list_type != ltype) {
		InvalidateContents();
		list_type = ltype;
		
		// Update the checkbox state when list type changes
		if (view_mode_toggle) {
			view_mode_toggle->SetValue(false);
		}
	}
}

void BrushPanel::SetListType(wxString ltype) {
	if (ltype == "small icons") {
		SetListType(BRUSHLIST_SMALL_ICONS);
	} else if (ltype == "large icons") {
		SetListType(BRUSHLIST_LARGE_ICONS);
	} else if (ltype == "listbox") {
		SetListType(BRUSHLIST_LISTBOX);
	} else if (ltype == "textlistbox") {
		SetListType(BRUSHLIST_TEXT_LISTBOX);
	} else if (ltype == "direct draw") {
		SetListType(BRUSHLIST_DIRECT_DRAW);
	} else if (ltype == "seamless grid") {
		SetListType(BRUSHLIST_SEAMLESS_GRID);
	}
}

void BrushPanel::CleanupBrushbox(BrushBoxInterface* box) {
	if (!box) return;
	
		// Special cleanup for DirectDrawBrushPanel
	DirectDrawBrushPanel* directPanel = dynamic_cast<DirectDrawBrushPanel*>(box);
		if (directPanel) {
			// Make sure any loading timer is stopped
			if (directPanel->loading_timer) {
				directPanel->loading_timer->Stop();
			}
		}
		
		// Special cleanup for SeamlessGridPanel
	SeamlessGridPanel* gridPanel = dynamic_cast<SeamlessGridPanel*>(box);
		if (gridPanel) {
			// Clear sprite cache
			gridPanel->ClearSpriteCache();
			// Make sure any loading timer is stopped
			if (gridPanel->loading_timer) {
				gridPanel->loading_timer->Stop();
			}
		}
		
		// Remove from sizer and destroy
	sizer->Detach(box->GetSelfWindow());
	box->GetSelfWindow()->Destroy();
}

void BrushPanel::InvalidateContents() {
	// First, properly clean up the existing brushbox if it exists
	if (brushbox) {
		CleanupBrushbox(brushbox);
		brushbox = nullptr;
	}
	
	// Now clear the sizer and recreate the UI elements
	sizer->Clear(true);
	loaded = false;
	
	// Add the view mode toggle back after clearing
	view_mode_toggle = new wxCheckBox(this, wxID_ANY, "Grid View");
	view_mode_toggle->SetValue(false);
	sizer->Add(view_mode_toggle, 0, wxALL, 5);
	
	// Always add the Show Item IDs checkbox right after the Grid View checkbox
	show_ids_toggle = new wxCheckBox(this, wxID_ANY, "Show Item IDs");
	show_ids_toggle->SetValue(false);
	show_ids_toggle->Bind(wxEVT_CHECKBOX, &BrushPanel::OnShowItemIDsToggle, this);
	sizer->Add(show_ids_toggle, 0, wxALL, 5);
	
	// Add a choice for view types if we're in RAW palette
	if(tileset && tileset->getType() == TILESET_RAW) {
		wxBoxSizer* choice_sizer = new wxBoxSizer(wxHORIZONTAL);
		wxStaticText* label = new wxStaticText(this, wxID_ANY, "View Type:");
		choice_sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		view_type_choice = new wxChoice(this, wxID_ANY);
		view_type_choice->Append("Normal");
		view_type_choice->Append("Direct Draw");
		view_type_choice->SetSelection(0);
		view_type_choice->Bind(wxEVT_CHOICE, [this](wxCommandEvent& event) {
			if(loaded) {
				LoadViewMode();
			}
		});
		choice_sizer->Add(view_type_choice, 1);
		sizer->Add(choice_sizer, 0, wxEXPAND | wxALL, 5);
	} else {
		view_type_choice = nullptr;
	}
}

void BrushPanel::LoadContents() {
	if (loaded) {
		return;
	}
	loaded = true;
	ASSERT(tileset != nullptr);
	
	LoadViewMode();
}

void BrushPanel::LoadViewMode() {
	// Remove old brushbox if it exists
	if (brushbox) {
		CleanupBrushbox(brushbox);
		brushbox = nullptr;
	}
	
	// Clear any existing zoom controls before adding new ones
	wxSizerItemList children = sizer->GetChildren();
	for (wxSizerItemList::iterator it = children.begin(); it != children.end(); ++it) {
		wxSizerItem* item = *it;
		wxWindow* window = item->GetWindow();
		// Only remove zoom controls and not checkboxes or choice controls
		if (window && window != view_mode_toggle && window != show_ids_toggle && 
			(view_type_choice == nullptr || window != view_type_choice) &&
			dynamic_cast<wxStaticText*>(window) == nullptr) {
			// Check if it's part of a zoom control group
			wxString label = window->GetLabel();
			if (label == "-" || label == "+" || label.EndsWith("%") || 
				dynamic_cast<wxButton*>(window) != nullptr) {
				sizer->Detach(window);
				window->Destroy();
			}
		}
	}
	
	// Check if we're using DirectDraw for RAW palette
	if(tileset && tileset->getType() == TILESET_RAW && view_type_choice && view_type_choice->GetSelection() == 1) {
		brushbox = new DirectDrawBrushPanel(this, tileset);
	}
	// Direct draw from list type setting
	else if(list_type == BRUSHLIST_DIRECT_DRAW && tileset && tileset->getType() == TILESET_RAW) {
		brushbox = new DirectDrawBrushPanel(this, tileset);
	}
	// Seamless Grid View - new option that takes precedence when Grid View is enabled
	else if (view_mode_toggle && view_mode_toggle->GetValue()) {
		SeamlessGridPanel* sgp = new SeamlessGridPanel(this, tileset);
		// Set show IDs based on the checkbox value if it exists
		if (show_ids_toggle) {
			sgp->SetShowItemIDs(show_ids_toggle->GetValue());
		}
		
		// Add zoom controls to the grid panel
		wxBoxSizer* zoomSizer = new wxBoxSizer(wxHORIZONTAL);
		
		wxStaticText* zoomLabel = new wxStaticText(this, wxID_ANY, "Zoom:");
		zoomSizer->Add(zoomLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		wxButton* zoomOutBtn = new wxButton(this, wxID_ANY, "-", wxDefaultPosition, wxSize(30, -1));
		zoomSizer->Add(zoomOutBtn, 0, wxRIGHT, 5);
		
		wxStaticText* zoomValueLabel = new wxStaticText(this, wxID_ANY, "100%", wxDefaultPosition, wxSize(50, -1), 
													  wxALIGN_CENTER_HORIZONTAL);
		zoomSizer->Add(zoomValueLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		
		wxButton* zoomInBtn = new wxButton(this, wxID_ANY, "+", wxDefaultPosition, wxSize(30, -1));
		zoomSizer->Add(zoomInBtn, 0);
		
		// Add the zoom controls above the grid
		sizer->Add(zoomSizer, 0, wxEXPAND | wxALL, 5);
		
		// Set up zoom button event handlers to force immediate refresh
		zoomOutBtn->Bind(wxEVT_BUTTON, [sgp, zoomValueLabel](wxCommandEvent& event) {
			int newZoom = sgp->DecrementZoom();
			zoomValueLabel->SetLabel(wxString::Format("%d%%", newZoom * 100));
			
			// Explicitly force layout update and refresh to ensure immediate drawing
			sgp->GetParent()->Layout();
			sgp->Update();
		});
		
		zoomInBtn->Bind(wxEVT_BUTTON, [sgp, zoomValueLabel](wxCommandEvent& event) {
			int newZoom = sgp->IncrementZoom();
			zoomValueLabel->SetLabel(wxString::Format("%d%%", newZoom * 100));
			
			// Explicitly force layout update and refresh to ensure immediate drawing
			sgp->GetParent()->Layout();
			sgp->Update();
		});
		
		brushbox = sgp;
	}
	// Otherwise use the list type setting
	else {
		switch (list_type) {
			case BRUSHLIST_LARGE_ICONS:
				brushbox = new BrushIconBox(this, tileset, RENDER_SIZE_32x32);
				break;
			case BRUSHLIST_SMALL_ICONS:
				brushbox = new BrushIconBox(this, tileset, RENDER_SIZE_16x16);
				break;
			case BRUSHLIST_SEAMLESS_GRID: {
				SeamlessGridPanel* sgp = new SeamlessGridPanel(this, tileset);
				// Set show IDs based on the checkbox value if it exists
				if (show_ids_toggle) {
					sgp->SetShowItemIDs(show_ids_toggle->GetValue());
				}
				
				// Add zoom controls to the grid panel
				wxBoxSizer* zoomSizer = new wxBoxSizer(wxHORIZONTAL);
				
				wxStaticText* zoomLabel = new wxStaticText(this, wxID_ANY, "Zoom:");
				zoomSizer->Add(zoomLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
				
				wxButton* zoomOutBtn = new wxButton(this, wxID_ANY, "-", wxDefaultPosition, wxSize(30, -1));
				zoomSizer->Add(zoomOutBtn, 0, wxRIGHT, 5);
				
				wxStaticText* zoomValueLabel = new wxStaticText(this, wxID_ANY, "100%", wxDefaultPosition, wxSize(50, -1), 
														  wxALIGN_CENTER_HORIZONTAL);
				zoomSizer->Add(zoomValueLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
				
				wxButton* zoomInBtn = new wxButton(this, wxID_ANY, "+", wxDefaultPosition, wxSize(30, -1));
				zoomSizer->Add(zoomInBtn, 0);
				
				// Add the zoom controls above the grid
				sizer->Add(zoomSizer, 0, wxEXPAND | wxALL, 5);
				
				// Set up zoom button event handlers to force immediate refresh
				zoomOutBtn->Bind(wxEVT_BUTTON, [sgp, zoomValueLabel](wxCommandEvent& event) {
					int newZoom = sgp->DecrementZoom();
					zoomValueLabel->SetLabel(wxString::Format("%d%%", newZoom * 100));
					
					// Explicitly force layout update and refresh to ensure immediate drawing
					sgp->GetParent()->Layout();
					sgp->Update();
				});
				
				zoomInBtn->Bind(wxEVT_BUTTON, [sgp, zoomValueLabel](wxCommandEvent& event) {
					int newZoom = sgp->IncrementZoom();
					zoomValueLabel->SetLabel(wxString::Format("%d%%", newZoom * 100));
					
					// Explicitly force layout update and refresh to ensure immediate drawing
					sgp->GetParent()->Layout();
					sgp->Update();
				});
				
				brushbox = sgp;
				break;
			}
			case BRUSHLIST_LISTBOX:
			default:
				brushbox = new BrushListBox(this, tileset);
				break;
		}
	}
	
	sizer->Add(brushbox->GetSelfWindow(), 1, wxEXPAND);
	Layout();
	brushbox->SelectFirstBrush();
}

void BrushPanel::SelectFirstBrush() {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		brushbox->SelectFirstBrush();
	}
}

Brush* BrushPanel::GetSelectedBrush() const {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->GetSelectedBrush();
	}

	if (tileset && tileset->size() > 0) {
		return tileset->brushlist[0];
	}
	return nullptr;
}

bool BrushPanel::SelectBrush(const Brush* whatbrush) {
	if (loaded) {
		ASSERT(brushbox != nullptr);
		return brushbox->SelectBrush(whatbrush);
	}

	for (BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		if (*iter == whatbrush) {
			LoadContents();
			return brushbox->SelectBrush(whatbrush);
		}
	}
	return false;
}

void BrushPanel::OnSwitchIn() {
	LoadContents();
}

void BrushPanel::OnSwitchOut() {
	////
}

void BrushPanel::OnClickListBoxRow(wxCommandEvent& event) {
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	// We just notify the GUI of the action, it will take care of everything else
	ASSERT(brushbox);
	size_t n = event.GetSelection();

	wxWindow* w = this;
	while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);

	if (w) {
		g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
	}

	// Get the brush that was clicked
	Brush* clicked_brush = tileset->brushlist[n];
	
	// Add to recent brushes list (this is a manual user selection)
	g_gui.AddRecentBrush(clicked_brush);
	
	// If this brush is already selected, deselect it first
	if(clicked_brush == g_gui.GetCurrentBrush()) {
		g_gui.SelectBrush(nullptr, tileset->getType());
	}
	
	// Now select the brush (either for the first time or re-selecting)
	g_gui.SelectBrush(clicked_brush, tileset->getType());
}

void BrushPanel::OnViewModeToggle(wxCommandEvent& event) {
	if (loaded && tileset) {
		// First check if we have a cached state for this tileset
		bool new_grid_view = view_mode_toggle->GetValue();
		BrushPanelState& state = g_brush_panel_cache[tileset];
		
		// If we're switching to grid view and don't have a cached grid view
		if (new_grid_view && !state.grid_view) {
			// Store the current list view if it's not already cached
			if (!state.list_view && brushbox) {
				state.list_view = brushbox;
				// Don't detach, just hide until we create the grid view
				brushbox->GetSelfWindow()->Hide();
				brushbox = nullptr;
			}
			
			// Create the grid view
			SeamlessGridPanel* sgp = new SeamlessGridPanel(this, tileset);
			if (show_ids_toggle) {
				sgp->SetShowItemIDs(show_ids_toggle->GetValue());
			}
			
			// Create zoom controls
			state.zoom_sizer = new wxBoxSizer(wxHORIZONTAL);
			wxStaticText* zoomLabel = new wxStaticText(this, wxID_ANY, "Zoom:");
			state.zoom_sizer->Add(zoomLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			
			wxButton* zoomOutBtn = new wxButton(this, wxID_ANY, "-", wxDefaultPosition, wxSize(30, -1));
			state.zoom_sizer->Add(zoomOutBtn, 0, wxRIGHT, 5);
			
			state.zoom_value_label = new wxStaticText(this, wxID_ANY, "100%", wxDefaultPosition, wxSize(50, -1), 
													wxALIGN_CENTER_HORIZONTAL);
			state.zoom_sizer->Add(state.zoom_value_label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
			
			wxButton* zoomInBtn = new wxButton(this, wxID_ANY, "+", wxDefaultPosition, wxSize(30, -1));
			state.zoom_sizer->Add(zoomInBtn, 0);
			
			// Add the zoom controls above the grid
			sizer->Add(state.zoom_sizer, 0, wxEXPAND | wxALL, 5);
			
			// Set up zoom button handlers
			zoomOutBtn->Bind(wxEVT_BUTTON, [sgp, this](wxCommandEvent& event) {
				int newZoom = sgp->DecrementZoom();
				g_brush_panel_cache[tileset].zoom_value_label->SetLabel(wxString::Format("%d%%", newZoom * 100));
				
				// Explicitly force layout update and refresh to ensure immediate drawing
				sgp->GetParent()->Layout();
				sgp->Update();
			});
			
			zoomInBtn->Bind(wxEVT_BUTTON, [sgp, this](wxCommandEvent& event) {
				int newZoom = sgp->IncrementZoom();
				g_brush_panel_cache[tileset].zoom_value_label->SetLabel(wxString::Format("%d%%", newZoom * 100));
				
				// Explicitly force layout update and refresh to ensure immediate drawing
				sgp->GetParent()->Layout();
				sgp->Update();
			});
			
			// Add the grid view to the sizer
			sizer->Add(sgp->GetSelfWindow(), 1, wxEXPAND);
			
			// Cache and set the current brush box
			state.grid_view = sgp;
			brushbox = sgp;
			state.grid_view_shown = true;
		}
		// If we're switching to list view and don't have a cached list view
		else if (!new_grid_view && !state.list_view) {
			// Store the current grid view if it's not cached
			if (!state.grid_view && brushbox) {
				state.grid_view = brushbox;
				
				// Hide the zoom controls and grid view
				if (state.zoom_sizer) {
					state.zoom_sizer->Show(false);
				}
				brushbox->GetSelfWindow()->Hide();
				brushbox = nullptr;
			}
			
			// Create list view according to the list type
			BrushBoxInterface* list_box = nullptr;
			switch (list_type) {
				case BRUSHLIST_LARGE_ICONS:
					list_box = new BrushIconBox(this, tileset, RENDER_SIZE_32x32);
					break;
				case BRUSHLIST_SMALL_ICONS:
					list_box = new BrushIconBox(this, tileset, RENDER_SIZE_16x16);
					break;
				case BRUSHLIST_SEAMLESS_GRID:
					list_box = new BrushListBox(this, tileset);
					break;
				case BRUSHLIST_LISTBOX:
				default:
					list_box = new BrushListBox(this, tileset);
					break;
			}
			
			// Add to sizer
			sizer->Add(list_box->GetSelfWindow(), 1, wxEXPAND);
			
			// Cache and set the current brush box
			state.list_view = list_box;
			brushbox = list_box;
			state.grid_view_shown = false;
		}
		// If we're toggling views and already have both views cached
		else if (state.grid_view && state.list_view) {
			// Hide the current view
			brushbox->GetSelfWindow()->Hide();
			
			// Show zoom controls if switching to grid view
			if (new_grid_view && state.zoom_sizer) {
				state.zoom_sizer->ShowItems(true);
				brushbox = state.grid_view;
			} else {
				// Hide zoom controls if switching to list view
				if (state.zoom_sizer) {
					state.zoom_sizer->ShowItems(false);
				}
				brushbox = state.list_view;
			}
			
			// Show the new view
			brushbox->GetSelfWindow()->Show();
			state.grid_view_shown = new_grid_view;
		}
		
		// Update the layout
		Layout();
		Update();
	} else {
		// If not loaded yet, load the view mode
		LoadViewMode();
	}
}

void BrushPanel::OnShowItemIDsToggle(wxCommandEvent& event) {
	if (loaded && brushbox) {
		// If we're using a seamless grid panel, update the display of item IDs
		SeamlessGridPanel* sgp = dynamic_cast<SeamlessGridPanel*>(brushbox);
		if (sgp) {
			sgp->SetShowItemIDs(show_ids_toggle->GetValue());
		} else {
			// If we're not using a seamless grid panel, reload the view
			LoadViewMode();
		}
	}
}

void BrushPanel::SetShowItemIDs(bool show) {
	if (show_ids_toggle) {
		show_ids_toggle->SetValue(show);
	}
	
	// Update the seamless grid panel if we're using one
	SeamlessGridPanel* sgp = dynamic_cast<SeamlessGridPanel*>(brushbox);
	if (sgp) {
		sgp->SetShowItemIDs(show);
	}
}

// ============================================================================
// BrushIconBox

BEGIN_EVENT_TABLE(BrushIconBox, wxScrolledWindow)
// Listbox style
EVT_TOGGLEBUTTON(wxID_ANY, BrushIconBox::OnClickBrushButton)
END_EVENT_TABLE()

BrushIconBox::BrushIconBox(wxWindow* parent, const TilesetCategory* _tileset, RenderSize rsz) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	icon_size(rsz) {
	ASSERT(tileset->getType() >= TILESET_UNKNOWN && tileset->getType() <= TILESET_HOUSE);
	int width;
	if (icon_size == RENDER_SIZE_32x32) {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) / 2 + 1, 1);
	} else {
		width = max(g_settings.getInteger(Config::PALETTE_COL_COUNT) + 1, 1);
	}

	// Create buttons
	wxSizer* stacksizer = newd wxBoxSizer(wxVERTICAL);
	wxSizer* rowsizer = nullptr;
	int item_counter = 0;
	for (BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		ASSERT(*iter);
		++item_counter;

		if (!rowsizer) {
			rowsizer = newd wxBoxSizer(wxHORIZONTAL);
		}

		BrushButton* bb = newd BrushButton(this, *iter, rsz);
		rowsizer->Add(bb);
		brush_buttons.push_back(bb);

		if (item_counter % width == 0) { // newd row
			stacksizer->Add(rowsizer);
			rowsizer = nullptr;
		}
	}
	if (rowsizer) {
		stacksizer->Add(rowsizer);
	}

	SetScrollbars(20, 20, 8, item_counter / width, 0, 0);
	SetSizer(stacksizer);
}

BrushIconBox::~BrushIconBox() {
	////
}

void BrushIconBox::SelectFirstBrush() {
	if (tileset && tileset->size() > 0) {
		DeselectAll();
		brush_buttons[0]->SetValue(true);
		EnsureVisible((size_t)0);
	}
}

Brush* BrushIconBox::GetSelectedBrush() const {
	if (!tileset) {
		return nullptr;
	}

	for (std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->GetValue()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushIconBox::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if ((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			EnsureVisible(*it);
			return true;
		}
	}
	return false;
}

void BrushIconBox::DeselectAll() {
	for (std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushIconBox::EnsureVisible(BrushButton* btn) {
	int windowSizeX, windowSizeY;
	GetVirtualSize(&windowSizeX, &windowSizeY);

	int scrollUnitX;
	int scrollUnitY;
	GetScrollPixelsPerUnit(&scrollUnitX, &scrollUnitY);

	wxRect rect = btn->GetRect();
	int y;
	CalcUnscrolledPosition(0, rect.y, nullptr, &y);

	int maxScrollPos = windowSizeY / scrollUnitY;
	int scrollPosY = std::min(maxScrollPos, (y / scrollUnitY));

	int startScrollPosY;
	GetViewStart(nullptr, &startScrollPosY);

	int clientSizeX, clientSizeY;
	GetClientSize(&clientSizeX, &clientSizeY);
	int endScrollPosY = startScrollPosY + clientSizeY / scrollUnitY;

	if (scrollPosY < startScrollPosY || scrollPosY > endScrollPosY) {
		// only scroll if the button isnt visible
		Scroll(-1, scrollPosY);
	}
}

void BrushIconBox::EnsureVisible(size_t n) {
	EnsureVisible(brush_buttons[n]);
}

void BrushIconBox::OnClickBrushButton(wxCommandEvent& event) {
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if (btn) {
		wxWindow* w = this;
		while ((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
		if (w) {
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		}

		// Add to recent brushes list (this is a manual user selection)
		g_gui.AddRecentBrush(btn->brush);

		// If this brush is already selected, deselect it first
		if(btn->brush == g_gui.GetCurrentBrush()) {
			g_gui.SelectBrush(nullptr, tileset->getType());
		}

		// Now select the brush (either for the first time or re-selecting)
		g_gui.SelectBrush(btn->brush, tileset->getType());
	}
}

// ============================================================================
// BrushListBox

BEGIN_EVENT_TABLE(BrushListBox, wxVListBox)
EVT_KEY_DOWN(BrushListBox::OnKey)
END_EVENT_TABLE()

BrushListBox::BrushListBox(wxWindow* parent, const TilesetCategory* tileset) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE),
	BrushBoxInterface(tileset) {
	SetItemCount(tileset->size());
}

BrushListBox::~BrushListBox() {
	////
}

void BrushListBox::SelectFirstBrush() {
	SetSelection(0);
	wxWindow::ScrollLines(-1);
}

Brush* BrushListBox::GetSelectedBrush() const {
	if (!tileset) {
		return nullptr;
	}

	int n = GetSelection();
	if (n != wxNOT_FOUND) {
		return tileset->brushlist[n];
	} else if (tileset->size() > 0) {
		return tileset->brushlist[0];
	}
	return nullptr;
}

bool BrushListBox::SelectBrush(const Brush* whatbrush) {
	for (size_t n = 0; n < tileset->size(); ++n) {
		if (tileset->brushlist[n] == whatbrush) {
			SetSelection(n);
			return true;
		}
	}
	return false;
}

void BrushListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const {
	ASSERT(n < tileset->size());
	Sprite* spr = g_gui.gfx.getSprite(tileset->brushlist[n]->getLookID());
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

	// Compose label: ItemID [ClientID] ItemName
	wxString label;
	Brush* brush = tileset->brushlist[n];
	uint16_t itemId = 0;
	uint16_t clientId = 0;
	std::string itemName;
	if (brush->isRaw()) {
		RAWBrush* raw = static_cast<RAWBrush*>(brush);
		itemId = raw->getItemID();
		ItemType* it = raw->getItemType();
		if (it) {
			clientId = it->clientID;
			itemName = it->name;
		}
	} else {
		itemId = static_cast<uint16_t>(brush->getID());
		const ItemType& it = g_items.getItemType(itemId);
		clientId = it.clientID;
		itemName = it.name;
	}
	label.Printf("%d [%d] %s", itemId, clientId, wxString(itemName));
	dc.DrawText(label, rect.GetX() + 40, rect.GetY() + 6);
}

wxCoord BrushListBox::OnMeasureItem(size_t n) const {
	return 32;
}

void BrushListBox::OnKey(wxKeyEvent& event) {
	switch (event.GetKeyCode()) {
		case WXK_UP:
		case WXK_DOWN:
		case WXK_LEFT:
		case WXK_RIGHT:
			if (g_settings.getInteger(Config::LISTBOX_EATS_ALL_EVENTS)) {
				case WXK_PAGEUP:
				case WXK_PAGEDOWN:
				case WXK_HOME:
				case WXK_END:
					event.Skip(true);
			} else {
				[[fallthrough]];
				default:
					if (g_gui.GetCurrentTab() != nullptr) {
						g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
					}
			}
	}
}

BEGIN_EVENT_TABLE(BrushGridBox, wxScrolledWindow)
EVT_TOGGLEBUTTON(wxID_ANY, BrushGridBox::OnClickBrushButton)
EVT_SIZE(BrushGridBox::OnSize)
END_EVENT_TABLE()

BrushGridBox::BrushGridBox(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	grid_sizer(nullptr),
	columns(1) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Create grid sizer
	grid_sizer = new wxFlexGridSizer(0, columns, 2, 2); // 0 rows (dynamic), 1 column, 2px gaps
	
	// Create buttons for each brush
	for(BrushVector::const_iterator iter = tileset->brushlist.begin(); iter != tileset->brushlist.end(); ++iter) {
		ASSERT(*iter);
		BrushButton* bb = new BrushButton(this, *iter, RENDER_SIZE_32x32);
		
		// Set tooltip with item name and ID
		wxString tooltip;
		if((*iter)->isRaw()) {
			RAWBrush* raw = static_cast<RAWBrush*>(*iter);
			tooltip = wxString::Format("%s [%d]", raw->getName(), raw->getItemID());
		} else {
			tooltip = wxString::Format("%s", (*iter)->getName());
		}
		bb->SetToolTip(tooltip);
		
		grid_sizer->Add(bb, 0, wxALL, 1);
		brush_buttons.push_back(bb);
	}
	
	SetSizer(grid_sizer);
	
	// Enable scrolling
	FitInside();
	SetScrollRate(32, 32);
	
	// Calculate initial grid layout
	RecalculateGrid();
}

BrushGridBox::~BrushGridBox() {
	// Grid sizer will delete all children automatically
}

void BrushGridBox::SelectFirstBrush() {
	if(tileset && tileset->size() > 0) {
		DeselectAll();
		brush_buttons[0]->SetValue(true);
	}
}

Brush* BrushGridBox::GetSelectedBrush() const {
	if(!tileset) return nullptr;
	
	for(std::vector<BrushButton*>::const_iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->GetValue()) {
			return (*it)->brush;
		}
	}
	return nullptr;
}

bool BrushGridBox::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		if((*it)->brush == whatbrush) {
			(*it)->SetValue(true);
			return true;
		}
	}
	return false;
}

void BrushGridBox::DeselectAll() {
	for(std::vector<BrushButton*>::iterator it = brush_buttons.begin(); it != brush_buttons.end(); ++it) {
		(*it)->SetValue(false);
	}
}

void BrushGridBox::OnClickBrushButton(wxCommandEvent& event) {
	wxObject* obj = event.GetEventObject();
	BrushButton* btn = dynamic_cast<BrushButton*>(obj);
	if(btn) {
		wxWindow* w = this;
		while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
		if(w) {
			g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
		}
		
		// Add to recent brushes list (this is a manual user selection)
		g_gui.AddRecentBrush(btn->brush);
		
		g_gui.SelectBrush(btn->brush, tileset->getType());
	}
}

void BrushGridBox::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	event.Skip();
}

void BrushGridBox::RecalculateGrid() {
	if(!grid_sizer) return;
	
	// Calculate how many columns can fit
	int window_width = GetClientSize().GetWidth();
	int button_width = 36; // 32px + 4px padding
	int new_columns = std::max(1, (window_width - 4) / button_width);
	
	if(new_columns != columns) {
		columns = new_columns;
		grid_sizer->SetCols(columns);
		grid_sizer->Layout();
		FitInside(); // Update scroll window virtual size
	}
}

// Implementation of DirectDrawBrushPanel

BEGIN_EVENT_TABLE(DirectDrawBrushPanel, wxScrolledWindow)
EVT_LEFT_DOWN(DirectDrawBrushPanel::OnMouseClick)
EVT_PAINT(DirectDrawBrushPanel::OnPaint)
EVT_SIZE(DirectDrawBrushPanel::OnSize)
EVT_SCROLLWIN(DirectDrawBrushPanel::OnScroll)
EVT_TIMER(wxID_ANY, DirectDrawBrushPanel::OnTimer)
END_EVENT_TABLE()

DirectDrawBrushPanel::DirectDrawBrushPanel(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
	BrushBoxInterface(_tileset),
	columns(1),
	item_width(32),
	item_height(32),
	selected_index(-1),
	buffer(nullptr),
	first_visible_row(0),
	last_visible_row(0),
	visible_rows_margin(10),
	total_rows(0),
	need_full_redraw(true),
	use_progressive_loading(true),
	is_large_tileset(false),
	loading_step(0),
	max_loading_steps(5),
	loading_timer(nullptr) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Default values
	columns = 10;
	item_width = 36; // 32px + 4px padding
	item_height = 36;
	
	// Check if we're dealing with a large tileset
	is_large_tileset = tileset && tileset->size() > LARGE_TILESET_THRESHOLD;
	
	// Create loading timer for large tilesets
	if (is_large_tileset && use_progressive_loading) {
		loading_timer = new wxTimer(this);
		max_loading_steps = 10; // Increase steps for smoother progress indication
	}
	
	// Calculate initial grid layout
	RecalculateGrid();
	
	// Enable scrolling and focus events
	SetScrollRate(5, 5);
	SetFocusIgnoringChildren();
	
	// Start progressive loading if needed
	if (is_large_tileset && use_progressive_loading) {
		StartProgressiveLoading();
	}
}

DirectDrawBrushPanel::~DirectDrawBrushPanel() {
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	if (loading_timer) {
		loading_timer->Stop();
		delete loading_timer;
		loading_timer = nullptr;
	}
}

void DirectDrawBrushPanel::StartProgressiveLoading() {
	if (!loading_timer) return;
	
	// Reset loading step
	loading_step = 0;
	
	// Set initial small margin
	visible_rows_margin = 3;
	
	// Force full redraw
	need_full_redraw = true;
	
	// Check if this is a small tileset that shouldn't use progressive loading
	if(tileset->size() < 1000) {
		// For very small tilesets, just load everything at once
		loading_step = max_loading_steps; // Mark as fully loaded
		visible_rows_margin = 30; // Set full margin
		if(loading_timer->IsRunning()) {
			loading_timer->Stop();
		}
		UpdateViewableItems();
		Refresh();
		return;
	}
	
	// Only show a limited number of items initially
	int itemsToShowInitially = std::min(100, static_cast<int>(tileset->size()));
	
	// Calculate how many items to add with each loading step
	int itemsPerStep = (tileset->size() - itemsToShowInitially) / max_loading_steps;
	if (itemsPerStep < 50) {
		// For smaller tilesets, load faster
		max_loading_steps = std::max(3, static_cast<int>(tileset->size() / 50));
	}
	
	// Start timer for progressive loading
	loading_timer->Start(200); // 200ms interval for better visual feedback
	
	// Force initial redraw to show progress
	Refresh();
}

void DirectDrawBrushPanel::OnTimer(wxTimerEvent& event) {
	// Progressively increase the visible margin
	loading_step++;
	
	// Gradually increase the margin with each step
	visible_rows_margin = std::min(3 + loading_step * 5, 30);
	
	// Update viewable items with new margin
	UpdateViewableItems();
	
	// Force redraw to update progress
	Refresh();
	
	// Stop timer when we've reached max loading steps or loaded all items
	if (loading_step >= max_loading_steps || static_cast<int>(tileset->size()) <= 1000) {
		loading_timer->Stop();
		loading_step = max_loading_steps; // Mark as fully loaded
		visible_rows_margin = 30; // Ensure full margin
		need_full_redraw = true;
		Refresh();
	}
}

void DirectDrawBrushPanel::OnScroll(wxScrollWinEvent& event) {
	// Handle scroll events to update visible items
	UpdateViewableItems();
	
	// Reset progressive loading on scroll for large tilesets
	if (is_large_tileset && use_progressive_loading && tileset->size() > 1000) {
		// Stop any existing timer
		if (loading_timer && loading_timer->IsRunning()) {
			loading_timer->Stop();
		}
		
		// If we haven't completed loading yet
		if (loading_step < max_loading_steps) {
			// Temporarily use small margin for immediate response
			visible_rows_margin = 3;
			UpdateViewableItems();
			
			// Show loading message in the area being scrolled to
			Refresh();
			
			// Restart progressive loading
			StartProgressiveLoading();
		} else {
			// If loading was already complete, just continue with full view
			visible_rows_margin = 30;
			UpdateViewableItems();
			Refresh();
		}
	} else {
		// For small tilesets, always use full view
		visible_rows_margin = 30;
		UpdateViewableItems();
		Refresh();
	}
	
	event.Skip();
}

void DirectDrawBrushPanel::UpdateViewableItems() {
	int xStart, yStart;
	GetViewStart(&xStart, &yStart);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	yStart *= ppuY;
	
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate visible range with margins
	int new_first_row = std::max(0, (yStart / item_height) - visible_rows_margin);
	int new_last_row = std::min(total_rows - 1, ((yStart + height) / item_height) + visible_rows_margin);
	
	// Only trigger redraw if visible range changes
	if (new_first_row != first_visible_row || new_last_row != last_visible_row) {
		first_visible_row = new_first_row;
		last_visible_row = new_last_row;
		Refresh();
	}
}

void DirectDrawBrushPanel::DrawItemsToPanel(wxDC& dc) {
	if(!tileset || tileset->size() == 0) return;
	
	// Calculate client area size
	int width, height;
	GetClientSize(&width, &height);
	
	// Draw loading progress for large datasets during initial load
	if (is_large_tileset && loading_step < max_loading_steps && tileset->size() > 1000) {
		// Draw progress bar
		int progressWidth = width - 40;
		int progressHeight = 20;
		int progressX = 20;
		int progressY = 20;
		
		// Progress background
		dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
		dc.SetPen(wxPen(wxColor(100, 100, 100)));
		dc.DrawRectangle(progressX, progressY, progressWidth, progressHeight);
		
		// Progress fill
		float progress = static_cast<float>(loading_step + 1) / max_loading_steps;
		dc.SetBrush(wxBrush(wxColor(0, 150, 0)));
		dc.SetPen(wxPen(wxColor(0, 100, 0)));
		dc.DrawRectangle(progressX, progressY, progressWidth * progress, progressHeight);
		
		// Progress text
		wxString loadingMsg = wxString::Format("Loading %zu items... (%d%%)", 
			tileset->size(), 
			static_cast<int>((loading_step + 1) * 100 / max_loading_steps));
		
		wxSize textSize = dc.GetTextExtent(loadingMsg);
		dc.SetTextForeground(wxColor(0, 0, 0));
		dc.DrawText(loadingMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 5);
		
		// Draw info about how many items are being processed
		int itemsProcessed = tileset->size() * progress;
		wxString itemsMsg = wxString::Format("Processed: %d / %zu items", 
			itemsProcessed, tileset->size());
		
		textSize = dc.GetTextExtent(itemsMsg);
		dc.DrawText(itemsMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 25);
		
		// Only draw a limited number of items during loading phase
		int maxItemsToDraw = itemsProcessed;
		
		// Only draw items in the visible range up to the processed amount
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size()) || index >= maxItemsToDraw) break;
				
				int x = col * item_width; 
				int y = row * item_height;
				
				// Skip items that would appear where our progress bar is
				if (y < progressY + progressHeight + 40) continue;
				
				// Draw background for selected item
				if(index == selected_index) {
					dc.SetBrush(wxBrush(wxColor(180, 180, 255)));
					dc.SetPen(wxPen(wxColor(100, 100, 200)));
					dc.DrawRectangle(x, y, item_width, item_height);
				}
				
				// Draw item sprite
				Brush* brush = tileset->brushlist[index];
				if(brush) {
					Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
					if(sprite) {
						sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x + 2, y + 2);
					}
					
					// For RAW brushes, also draw the ID
					if(brush->isRaw()) {
						RAWBrush* raw = static_cast<RAWBrush*>(brush);
						wxString label = wxString::Format("%d", raw->getItemID());
						dc.SetTextForeground(wxColor(0, 0, 0));
						dc.DrawText(label, x + 2, y + item_height - 16);
					}
				}
			}
		}
	} else {
		// Normal drawing when fully loaded
		// Only draw items in the visible range
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				if(index >= static_cast<int>(tileset->size())) break;
				
				int x = col * item_width;
				int y = row * item_height;
				
				// Draw background for selected item
				if(index == selected_index) {
					dc.SetBrush(wxBrush(wxColor(180, 180, 255)));
					dc.SetPen(wxPen(wxColor(100, 100, 200)));
					dc.DrawRectangle(x, y, item_width, item_height);
				}
				
				// Draw item sprite
				Brush* brush = tileset->brushlist[index];
				if(brush) {
					Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
					if(sprite) {
						sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x + 2, y + 2);
					}
					
					// For RAW brushes, also draw the ID
					if(brush->isRaw()) {
						RAWBrush* raw = static_cast<RAWBrush*>(brush);
						wxString label = wxString::Format("%d", raw->getItemID());
						dc.SetTextForeground(wxColor(0, 0, 0));
						dc.DrawText(label, x + 2, y + item_height - 16);
					}
				}
			}
		}
	}
}

void DirectDrawBrushPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // For correct scrolling
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Draw items
	DrawItemsToPanel(dc);
}

void DirectDrawBrushPanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	if(buffer) {
		delete buffer;
		buffer = nullptr;
	}
	Refresh();
	event.Skip();
}

void DirectDrawBrushPanel::SelectFirstBrush() {
	if(tileset && tileset->size() > 0) {
		selected_index = 0;
		Refresh();
	}
}

Brush* DirectDrawBrushPanel::GetSelectedBrush() const {
	if(!tileset || selected_index < 0 || selected_index >= static_cast<int>(tileset->size())) {
		return nullptr;
	}
	
	return tileset->brushlist[selected_index];
}

bool DirectDrawBrushPanel::SelectBrush(const Brush* whatbrush) {
	if(!tileset) return false;
	
	for(size_t i = 0; i < tileset->size(); ++i) {
		if(tileset->brushlist[i] == whatbrush) {
			selected_index = i;
			Refresh();
			
			// Ensure the selected item is visible
			int row = selected_index / columns;
			int yPos = row * item_height;
			
			// Get the visible area
			int xStart, yStart;
			GetViewStart(&xStart, &yStart);
			int ppuX, ppuY;
			GetScrollPixelsPerUnit(&ppuX, &ppuY);
			yStart *= ppuY;
			
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			
			// Scroll if necessary
			if(yPos < yStart) {
				Scroll(-1, yPos / ppuY);
				UpdateViewableItems();
			} else if(yPos + item_height > yStart + clientHeight) {
				Scroll(-1, (yPos + item_height - clientHeight) / ppuY + 1);
				UpdateViewableItems();
			}
			
			return true;
		}
	}
	return false;
}

void DirectDrawBrushPanel::OnMouseClick(wxMouseEvent& event) {
	// Convert mouse position to logical position (accounting for scrolling)
	int xPos, yPos;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &xPos, &yPos);
	
	// Calculate which item was clicked
	int col = xPos / item_width;
	int row = yPos / item_height;
	
	// Bounds check
	if(col >= 0 && col < columns) {
		int index = row * columns + col;
		if(index >= 0 && index < static_cast<int>(tileset->size())) {
			selected_index = index;
			Refresh();
			
			// Notify parent about the selection
			wxWindow* w = this;
			while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
			if(w) {
				g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
			}

			// Add to recent brushes list (this is a manual user selection)
			g_gui.AddRecentBrush(tileset->brushlist[index]);

			// If this brush is already selected, deselect it first
			if(tileset->brushlist[index] == g_gui.GetCurrentBrush()) {
				g_gui.SelectBrush(nullptr, tileset->getType());
			}

			// Now select the brush (either for the first time or re-selecting)
			g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
		}
	}
	
	event.Skip();
}

void DirectDrawBrushPanel::RecalculateGrid() {
	// Calculate columns based on client width
	int width;
	GetClientSize(&width, nullptr);
	columns = std::max(1, width / item_width);
	
	// Calculate total rows needed
	total_rows = (tileset->size() + columns - 1) / columns;  // Ceiling division
	int virtual_height = total_rows * item_height;
	
	// Set virtual size for scrolling
	SetVirtualSize(width, virtual_height);
	
	// Update which items are currently visible
	UpdateViewableItems();
	
	// Clean up old buffer if it exists
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	need_full_redraw = true;
}

// ============================================================================
// SeamlessGridPanel
// A direct rendering class for dense sprite grid with zero margins

BEGIN_EVENT_TABLE(SeamlessGridPanel, wxScrolledWindow)
EVT_LEFT_DOWN(SeamlessGridPanel::OnMouseClick)
EVT_MOTION(SeamlessGridPanel::OnMouseMove)
EVT_PAINT(SeamlessGridPanel::OnPaint)
EVT_SIZE(SeamlessGridPanel::OnSize)
EVT_SCROLLWIN(SeamlessGridPanel::OnScroll)
EVT_TIMER(wxID_ANY, SeamlessGridPanel::OnTimer)
EVT_KEY_DOWN(SeamlessGridPanel::OnKeyDown)
END_EVENT_TABLE()

SeamlessGridPanel::SeamlessGridPanel(wxWindow* parent, const TilesetCategory* _tileset) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxWANTS_CHARS),
	BrushBoxInterface(_tileset),
	columns(1),
	sprite_size(32),
	zoom_level(1), // Add zoom level, default 1x (32px)
	selected_index(-1),
	hover_index(-1),
	buffer(nullptr),
	show_item_ids(false),
	first_visible_row(0),
	last_visible_row(0),
	visible_rows_margin(10),
	total_rows(0),
	need_full_redraw(true),
	use_progressive_loading(true),
	is_large_tileset(false),
	loading_step(0),
	max_loading_steps(5),
	loading_timer(nullptr),
	chunk_size(g_settings.getInteger(Config::GRID_CHUNK_SIZE)),
	current_chunk(0),
	total_chunks(1),
	navigation_panel(nullptr) {
	
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Enable keyboard focus
	SetWindowStyle(GetWindowStyle() | wxWANTS_CHARS);
	
	// Check if we're dealing with a large tileset
	is_large_tileset = tileset && tileset->size() > LARGE_TILESET_THRESHOLD;
	
	// For extremely large tilesets, use chunking
	if (tileset && tileset->size() > 10000) {
		// Calculate total chunks
		total_chunks = (tileset->size() + chunk_size - 1) / chunk_size;
		
		// Create navigation panel for chunk navigation
		CreateNavigationPanel(parent);
	}
	
	// Get visible rows margin from settings
	visible_rows_margin = g_settings.getInteger(Config::GRID_VISIBLE_ROWS_MARGIN);
	
	// Create loading timer for large tilesets
	if (is_large_tileset && use_progressive_loading) {
		loading_timer = new wxTimer(this);
		max_loading_steps = 10; // Increase steps for smoother progress indication
	}
	
	// Initialize the sprite cache
	sprite_cache.clear();
	
	// Enable scrolling
	SetScrollRate(sprite_size, sprite_size);
	
	// Calculate initial grid layout
	RecalculateGrid();
	
	// Start with first brush selected
	SelectFirstBrush();
	
	// Start progressive loading if needed
	if (is_large_tileset && use_progressive_loading) {
		StartProgressiveLoading();
	}
}

SeamlessGridPanel::~SeamlessGridPanel() {
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	if (loading_timer) {
		loading_timer->Stop();
		delete loading_timer;
		loading_timer = nullptr;
	}
	
	// Clear all cached sprites to prevent memory leaks
	ClearSpriteCache();
}

void SeamlessGridPanel::StartProgressiveLoading() {
	if (!loading_timer) return;
	
	// Reset loading step
	loading_step = 0;
	
	// Set initial small margin
	visible_rows_margin = 3;
	
	// Force full redraw
	need_full_redraw = true;
	
	// Check if this is a small tileset that shouldn't use progressive loading
	if(tileset->size() < 200) {
		// For very small tilesets, just load everything at once
		loading_step = max_loading_steps; // Mark as fully loaded
		visible_rows_margin = 30; // Set full margin
		if(loading_timer->IsRunning()) {
			loading_timer->Stop();
		}
		UpdateViewableItems();
		Refresh();
		return;
	}
	
	// Adjust batch size based on zoom level - higher zoom means fewer items per batch
	int zoom_factor = zoom_level * zoom_level; // Square the zoom level for scaling factor
	
	// Only show a limited number of items initially, adjusted for zoom level
	int itemsToShowInitially = std::min(100 / zoom_factor, static_cast<int>(tileset->size()));
	itemsToShowInitially = std::max(20, itemsToShowInitially); // Ensure minimum batch size
	
	// Calculate how many items to add with each loading step, adjusted for zoom level
	int itemsPerStep = (tileset->size() - itemsToShowInitially) / max_loading_steps;
	
	// Scale items per step based on zoom level
	itemsPerStep = std::max(30, itemsPerStep / zoom_factor);
	
	// For smaller tilesets or high zoom levels, adjust loading steps
	if (itemsPerStep < 50) {
		// Calculate an appropriate number of steps based on tileset size and zoom level
		max_loading_steps = std::max(3, static_cast<int>(tileset->size() / (50 / zoom_factor)));
	}
	
	// Start timer for progressive loading with interval based on zoom
	// Higher zoom levels need slightly more time per frame
	int interval = 200 + (zoom_level - 1) * 50; // 200ms at zoom 1, +50ms per zoom level
	loading_timer->Start(interval);
	
	// Force initial redraw to show progress
	Refresh();
}

void SeamlessGridPanel::OnTimer(wxTimerEvent& event) {
	// Progressively increase the visible margin
	loading_step++;
	
	// Gradually increase the margin with each step
	visible_rows_margin = std::min(3 + loading_step * 5, 30);
	
	// Update viewable items with new margin
	UpdateViewableItems();
	
	// Force redraw to update progress
	Refresh();
	
	// Stop timer when we've reached max loading steps or loaded all items
	if (loading_step >= max_loading_steps || static_cast<int>(tileset->size()) <= 1000) {
		loading_timer->Stop();
		loading_step = max_loading_steps; // Mark as fully loaded
		visible_rows_margin = 30; // Ensure full margin
		need_full_redraw = true;
		Refresh();
	}
}

void SeamlessGridPanel::UpdateViewableItems() {
	int xStart, yStart;
	GetViewStart(&xStart, &yStart);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	yStart *= ppuY;
	
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate visible range with margins
	int new_first_row = std::max(0, (yStart / sprite_size) - visible_rows_margin);
	int new_last_row = std::min(total_rows - 1, ((yStart + height) / sprite_size) + visible_rows_margin);
	
	// Only trigger redraw if visible range changes
	if (new_first_row != first_visible_row || new_last_row != last_visible_row) {
		first_visible_row = new_first_row;
		last_visible_row = new_last_row;
		Refresh();
	}
}

void SeamlessGridPanel::DrawItemsToPanel(wxDC& dc) {
	if(!tileset || tileset->size() == 0) return;
	
	// Check if we need to manage cache size
	if (need_full_redraw) {
		ManageSpriteCache();
	}
	
	// Calculate client area size
	int width, height;
	GetClientSize(&width, &height);
	
	// Draw loading progress for large datasets during initial load
	if (loading_step < max_loading_steps && tileset->size() > 1000) {
		// Draw progress bar
		int progressWidth = width - 40;
		int progressHeight = 20;
		int progressX = 20;
		int progressY = 20;
		
		// Progress background
		dc.SetBrush(wxBrush(wxColor(200, 200, 200)));
		dc.SetPen(wxPen(wxColor(100, 100, 100)));
		dc.DrawRectangle(progressX, progressY, progressWidth, progressHeight);
		
		// Progress fill
		float progress = static_cast<float>(loading_step + 1) / max_loading_steps;
		dc.SetBrush(wxBrush(wxColor(0, 150, 0)));
		dc.SetPen(wxPen(wxColor(0, 100, 0)));
		dc.DrawRectangle(progressX, progressY, progressWidth * progress, progressHeight);
		
		// Progress text - show different messages based on zoom level
		wxString zoomInfo = zoom_level > 1 ? wxString::Format(" (Zoom %dx)", zoom_level) : "";
		wxString loadingMsg = wxString::Format("Loading %zu items%s... (%d%%)", 
			tileset->size(),
			zoomInfo,
			static_cast<int>((loading_step + 1) * 100 / max_loading_steps));
		
		wxSize textSize = dc.GetTextExtent(loadingMsg);
		dc.SetTextForeground(wxColor(0, 0, 0));
		dc.DrawText(loadingMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 5);
		
		// Draw info about how many items are being processed
		int itemsProcessed = tileset->size() * progress;
		wxString itemsMsg = wxString::Format("Processed: %d / %zu items", 
			itemsProcessed, tileset->size());
		
		textSize = dc.GetTextExtent(itemsMsg);
		dc.DrawText(itemsMsg, (width - textSize.GetWidth()) / 2, progressY + progressHeight + 25);
		
		// Only draw a limited number of items during loading phase
		int maxItemsToDraw = itemsProcessed;
		
		// Only draw items in the visible range up to the processed amount
		for(int row = first_visible_row; row <= last_visible_row; ++row) {
			for(int col = 0; col < columns; ++col) {
				int index = row * columns + col;
				
				// For chunked mode, adjust the index
				if (tileset->size() > 10000) {
					index = current_chunk * chunk_size + index;
				}
				
				if(index >= static_cast<int>(tileset->size()) || index >= maxItemsToDraw) break;
				
				int x = col * sprite_size;
				int y = row * sprite_size;
				
				// Skip items that would appear where our progress bar is
				if (y < progressY + progressHeight + 40) continue;
				
				DrawSpriteAt(dc, x, y, index);
			}
		}
	} else {
		// Normal drawing when fully loaded
		if (tileset->size() > 10000) {
			// No need to draw chunk header or buttons since we now have a separate navigation panel
			
			// Calculate items in current chunk
			size_t chunk_start = current_chunk * chunk_size;
			size_t items_in_chunk = std::min(static_cast<size_t>(chunk_size), tileset->size() - chunk_start);
			
			// Only draw items in the visible range for this chunk
			for(int row = first_visible_row; row <= last_visible_row; ++row) {
				for(int col = 0; col < columns; ++col) {
					int local_index = row * columns + col;
					if(local_index >= static_cast<int>(items_in_chunk)) break;
					
					int global_index = chunk_start + local_index;
					
					int x = col * sprite_size;
					int y = row * sprite_size;
					
					DrawSpriteAt(dc, x, y, global_index);
				}
			}
		} else {
			// Standard drawing for normal-sized tilesets
			// Only draw items in the visible range
			for(int row = first_visible_row; row <= last_visible_row; ++row) {
				for(int col = 0; col < columns; ++col) {
					int index = row * columns + col;
					if(index >= static_cast<int>(tileset->size())) break;
					
					int x = col * sprite_size;
					int y = row * sprite_size;
					
					DrawSpriteAt(dc, x, y, index);
				}
			}
		}
	}
	
	// Reset full redraw flag
	need_full_redraw = false;
}

void SeamlessGridPanel::DrawSpriteAt(wxDC& dc, int x, int y, int index) {
	// Common drawing function to reduce code duplication
	if (index < 0 || index >= static_cast<int>(tileset->size())) return;
	
	Brush* brush = tileset->brushlist[index];
	if (!brush) return;
	
	// Draw background for selected/hover items with semi-transparency
	if (index == selected_index) {
		// Use transparent background for selection
		wxColor selectColor(120, 120, 200, 180); // More transparent blue
		dc.SetBrush(wxBrush(selectColor));
		dc.SetPen(wxPen(wxColor(80, 80, 160), 2));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	} else if (index == hover_index) {
		// Use even more transparent background for hover
		wxColor hoverColor(200, 200, 255, 120); // Very transparent light blue
		dc.SetBrush(wxBrush(hoverColor));
		dc.SetPen(wxPen(wxColor(150, 150, 230, 180), 1));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	}
	
	// Check if we have the sprite already cached at current zoom level
	bool need_to_create_sprite = true;
	
	if (sprite_cache.count(index) > 0 && sprite_cache[index].is_valid) {
		// Check if the cached sprite is at the current zoom level
		if (sprite_cache[index].zoom_level == zoom_level) {
			// Use the cached sprite bitmap
			dc.DrawBitmap(sprite_cache[index].bitmap, x, y, true);
			need_to_create_sprite = false;
		}
	}
	
	// If not in cache or wrong zoom level, create and cache it
	if (need_to_create_sprite) {
		Sprite* sprite = g_gui.gfx.getSprite(brush->getLookID());
		if (sprite) {
			// Create appropriate sized bitmap based on zoom level
			if (zoom_level == 1) {
				// For 1x zoom, create a standard 32x32 bitmap
				wxBitmap bmp(32, 32);
				wxMemoryDC memDC(bmp);
				memDC.SetBackground(*wxTRANSPARENT_BRUSH);
				memDC.Clear();
				
				// Draw to the memory DC
				sprite->DrawTo(&memDC, SPRITE_SIZE_32x32, 0, 0);
				memDC.SelectObject(wxNullBitmap);
				
				// Store in cache
				CachedSprite cached;
				cached.bitmap = bmp;
				cached.zoom_level = zoom_level;
				cached.is_valid = true;
				sprite_cache[index] = cached;
				
				// Draw to screen
				dc.DrawBitmap(bmp, x, y, true);
			} 
			else if (zoom_level == 2) {
				// For 2x zoom (64x64), create a 64x64 bitmap
				wxBitmap bmp(64, 64);
				wxMemoryDC memDC(bmp);
				memDC.SetBackground(*wxTRANSPARENT_BRUSH);
				memDC.Clear();
				
				// Draw to the memory DC with 64x64 size
				sprite->DrawTo(&memDC, SPRITE_SIZE_64x64, 0, 0);
				memDC.SelectObject(wxNullBitmap);
				
				// Store in cache
				CachedSprite cached;
				cached.bitmap = bmp;
				cached.zoom_level = zoom_level;
				cached.is_valid = true;
				sprite_cache[index] = cached;
				
				// Draw to screen
				dc.DrawBitmap(bmp, x, y, true);
			} 
			else {
				// For other zoom levels (3x, 4x), create a scaled bitmap
				// First draw at 32x32
				wxBitmap temp_bmp(32, 32);
				wxMemoryDC temp_dc(temp_bmp);
				temp_dc.SetBackground(*wxTRANSPARENT_BRUSH);
				temp_dc.Clear();
				
				// Draw the sprite to the temp DC
				sprite->DrawTo(&temp_dc, SPRITE_SIZE_32x32, 0, 0);
				temp_dc.SelectObject(wxNullBitmap); // Deselect to finalize drawing
				
				// Convert to image, scale, and draw
				wxImage img = temp_bmp.ConvertToImage();
				img.SetMaskColour(255, 0, 255); // Keep transparency
				img = img.Rescale(sprite_size, sprite_size, wxIMAGE_QUALITY_HIGH);
				
				wxBitmap scaled(img);
				
				// Store in cache
				CachedSprite cached;
				cached.bitmap = scaled;
				cached.zoom_level = zoom_level;
				cached.is_valid = true;
				sprite_cache[index] = cached;
				
				// Draw to screen
				dc.DrawBitmap(scaled, x, y, true);
			}
		}
	}
	
	// For RAW brushes, draw the ID if enabled
	if (show_item_ids && brush->isRaw()) {
		RAWBrush* raw = static_cast<RAWBrush*>(brush);
		
		// Scale the font size based on zoom level
		wxFont font = dc.GetFont();
		font.SetPointSize(std::max(8, 8 + (zoom_level - 1) * 2));
		dc.SetFont(font);
		
		// Draw with semi-transparent background for better readability
		wxString idText = wxString::Format("%d", raw->getItemID());
		wxSize textSize = dc.GetTextExtent(idText);
		int textHeight = std::max(14, 14 + (zoom_level - 1) * 4);
		
		// More transparent background for ID text
		wxColor bgColor(0, 0, 0, 140); // Semi-transparent black
		dc.SetBrush(wxBrush(bgColor));
		dc.SetPen(wxPen(wxColor(0, 0, 0, 0)));
		dc.DrawRectangle(x, y + sprite_size - textHeight, textSize.GetWidth() + 4, textHeight);
		
		dc.SetTextForeground(wxColor(255, 255, 255));
		dc.DrawText(idText, x + 2, y + sprite_size - textHeight);
	}
}

void SeamlessGridPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // For correct scrolling
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Draw items
	DrawItemsToPanel(dc);
}

void SeamlessGridPanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	Refresh();
	event.Skip();
}

void SeamlessGridPanel::RecalculateGrid() {
	// Calculate columns based on client width
	int width;
	GetClientSize(&width, nullptr);
	columns = std::max(1, width / sprite_size);
	
	// For extremely large tilesets, use chunking
	if (tileset && tileset->size() > 10000) {
		// Calculate items in current chunk
		size_t chunk_start = current_chunk * chunk_size;
		size_t items_in_chunk = std::min(static_cast<size_t>(chunk_size), tileset->size() - chunk_start);
		
		// Calculate total rows needed for this chunk
		total_rows = (items_in_chunk + columns - 1) / columns;  // Ceiling division
		int virtual_height = total_rows * sprite_size;
		
		// Add space for chunk separators if not the last chunk
		if (current_chunk < total_chunks - 1) {
			virtual_height += 40; // Space for separator
		}
		
		// Set virtual size for scrolling
		SetVirtualSize(width, virtual_height);
	} else {
		// Normal calculation for smaller tilesets
		total_rows = (tileset->size() + columns - 1) / columns;  // Ceiling division
		int virtual_height = total_rows * sprite_size;
		
		// Set virtual size for scrolling
		SetVirtualSize(width, virtual_height);
	}
	
	// Update which items are currently visible
	UpdateViewableItems();
	
	// Clean up old buffer if it exists
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	// Limit sprite cache size when grid layout changes
	ManageSpriteCache();
	
	need_full_redraw = true;
}

void SeamlessGridPanel::OnScroll(wxScrollWinEvent& event) {
	// Handle scroll events to update visible items
	UpdateViewableItems();
	
	// Reset progressive loading on scroll for large tilesets
	if (loading_step < max_loading_steps && tileset->size() > 1000) {
		// Temporarily use small margin for immediate response
		visible_rows_margin = 3;
		UpdateViewableItems();
		
		// Show loading message in the area being scrolled to
		Refresh();
		
		// Restart progressive loading
		StartProgressiveLoading();
	} else {
		// If loading was already complete or it's a small tileset, just continue with full view
		visible_rows_margin = 30;
		UpdateViewableItems();
		Refresh();
	}
	
	event.Skip();
}

int SeamlessGridPanel::GetSpriteIndexAt(int x, int y) const {
	// Convert mouse position to logical position (accounting for scrolling)
	int logX, logY;
	CalcUnscrolledPosition(x, y, &logX, &logY);
	
	// Calculate row and column
	int col = logX / sprite_size;
	int row = logY / sprite_size;
	
	// Calculate index
	int index = row * columns + col;
	
	// Check if this is a valid index
	if (index >= 0 && index < static_cast<int>(tileset->size()) && 
		col >= 0 && col < columns) {
		return index;
	}
	
	return -1;
}

void SeamlessGridPanel::OnMouseClick(wxMouseEvent& event) {
	// Convert mouse position to logical position (accounting for scrolling)
	int xPos, yPos;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &xPos, &yPos);
	
	// Calculate which item was clicked
	int col = xPos / sprite_size;
	int row = yPos / sprite_size;
	
	// Bounds check
	if (col >= 0 && col < columns && row >= 0) {
		int index = row * columns + col;
		
		// For chunked mode, adjust the index
		if (tileset->size() > 10000) {
			// Calculate items in current chunk
			size_t chunk_start = current_chunk * chunk_size;
			size_t items_in_chunk = std::min(static_cast<size_t>(chunk_size), tileset->size() - chunk_start);
			
			if (index >= static_cast<int>(items_in_chunk)) return;
			
			index = chunk_start + index;
		}
		
		if (index >= 0 && index < static_cast<int>(tileset->size())) {
			selected_index = index;
			Refresh();
			
			// Notify parent about the selection
			wxWindow* w = this;
			while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
			if(w) {
				g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
			}
			
			// Add to recent brushes list (this is a manual user selection)
			g_gui.AddRecentBrush(tileset->brushlist[index]);
			
			g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
		}
	}
	
	event.Skip();
}

void SeamlessGridPanel::OnMouseMove(wxMouseEvent& event) {
	// Update hover effect
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != hover_index) {
		hover_index = index;
		Refresh();
	}
	
	event.Skip();
}

void SeamlessGridPanel::SelectFirstBrush() {
	if (tileset && tileset->size() > 0) {
		selected_index = 0;
		Refresh();
	}
}

Brush* SeamlessGridPanel::GetSelectedBrush() const {
	if (!tileset || selected_index < 0 || selected_index >= static_cast<int>(tileset->size())) {
		return nullptr;
	}
	
	return tileset->brushlist[selected_index];
}

bool SeamlessGridPanel::SelectBrush(const Brush* whatbrush) {
	if (!tileset) return false;
	
	for (size_t i = 0; i < tileset->size(); ++i) {
		if (tileset->brushlist[i] == whatbrush) {
			selected_index = i;
			hover_index = -1;
			Refresh();
			
			// Ensure the selected item is visible
			int row = selected_index / columns;
			int yPos = row * sprite_size;
			
			// Get the visible area
			int xStart, yStart;
			GetViewStart(&xStart, &yStart);
			int ppuX, ppuY;
			GetScrollPixelsPerUnit(&ppuX, &ppuY);
			yStart *= ppuY;
			
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			
			// Scroll if necessary
			if (yPos < yStart) {
				Scroll(-1, yPos / ppuY);
				UpdateViewableItems();
			} else if (yPos + sprite_size > yStart + clientHeight) {
				Scroll(-1, (yPos + sprite_size - clientHeight) / ppuY + 1);
				UpdateViewableItems();
			}
			
			return true;
		}
	}
	return false;
}

void SeamlessGridPanel::SelectIndex(int index) {
    if (!tileset || index < 0 || index >= static_cast<int>(tileset->size())) {
        return;
    }

    // Check if we need to change chunks for large tilesets
    if (tileset->size() > 10000) {
        int target_chunk = index / chunk_size;
        if (target_chunk != current_chunk) {
            // We need to switch chunks first
            current_chunk = target_chunk;
            
            // Clear cache for new chunk
            sprite_cache.clear();
            
            // Recalculate grid with new chunk
            RecalculateGrid();
            
            // Update navigation panel
            if (navigation_panel) {
                UpdateNavigationPanel();
            }
            
            // Force complete refresh
            need_full_redraw = true;
        }
    }

    selected_index = index;
    hover_index = -1;

    // For chunked mode, calculate the local index within the current chunk
    int local_index = index;
    if (tileset->size() > 10000) {
        size_t chunk_start = current_chunk * chunk_size;
        local_index = index - static_cast<int>(chunk_start);
    }

    // Calculate row/column for scrolling
    int row = local_index / columns;
    int yPos = row * sprite_size;

    // Get the visible area
    int xStart, yStart;
    GetViewStart(&xStart, &yStart);
    int ppuX, ppuY;
    GetScrollPixelsPerUnit(&ppuX, &ppuY);
    yStart *= ppuY;

    int clientHeight;
    GetClientSize(nullptr, &clientHeight);

    // Calculate what should be visible
    int visibleRows = clientHeight / sprite_size;
    
    // Scroll to position the selected item properly - aim to center it
    int targetRow = std::max(0, row - (visibleRows / 2) + 1);
    
    // Don't let selection jump to bottom - ensure we don't scroll too far
    int maxRow = (total_rows - visibleRows) + 1;
    if (maxRow < 0) maxRow = 0;
    
    targetRow = std::min(targetRow, maxRow);
    
    // Apply the scroll position
    Scroll(-1, targetRow * sprite_size / ppuY);
    
    UpdateViewableItems();
    Refresh();

    // Notify parent about the selection
    wxWindow* w = this;
    while((w = w->GetParent()) && dynamic_cast<PaletteWindow*>(w) == nullptr);
    if(w) {
        g_gui.ActivatePalette(static_cast<PaletteWindow*>(w));
    }
    g_gui.SelectBrush(tileset->brushlist[index], tileset->getType());
}

void SeamlessGridPanel::OnKeyDown(wxKeyEvent& event) {
	if (!tileset || tileset->size() == 0) {
		event.Skip();
		return;
	}

	int newIndex = selected_index;
	bool handled = true;

	switch (event.GetKeyCode()) {
		case WXK_LEFT:
			if (selected_index > 0) {
				newIndex--;
			}
			break;

		case WXK_RIGHT:
			if (selected_index < static_cast<int>(tileset->size()) - 1) {
				newIndex++;
			}
			break;

		case WXK_UP:
			if (selected_index >= columns) {
				newIndex -= columns;
			}
			break;

		case WXK_DOWN:
			if (selected_index + columns < static_cast<int>(tileset->size())) {
				newIndex += columns;
			}
			break;

		case WXK_HOME:
			newIndex = 0;
			break;

		case WXK_END:
			newIndex = tileset->size() - 1;
			break;

		case WXK_PAGEUP: {
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			int rowsPerPage = clientHeight / sprite_size;
			newIndex = std::max(0, selected_index - (rowsPerPage * columns));
			break;
		}

		case WXK_PAGEDOWN: {
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			int rowsPerPage = clientHeight / sprite_size;
			newIndex = std::min(static_cast<int>(tileset->size()) - 1, 
							  selected_index + (rowsPerPage * columns));
			break;
		}

		default:
			handled = false;
			break;
	}

	if (handled && newIndex != selected_index) {
		SelectIndex(newIndex);
		SetFocus(); // Keep focus for more keyboard input
	} else {
		event.Skip(); // Allow other handlers to process the event
	}
}

// Add these implementations before the end of the file
int SeamlessGridPanel::IncrementZoom() {
	if (zoom_level < 4) { // Max zoom 4x (128px)
		zoom_level++;
		UpdateGridSize();
		
		// Force a complete redraw of visible items
		need_full_redraw = true;
		first_visible_row = 0; // Reset visible rows to force redraw
		last_visible_row = 0;
		UpdateViewableItems();
		Refresh(true); // Force a full refresh
		
		return zoom_level;
	}
	return zoom_level;
}

int SeamlessGridPanel::DecrementZoom() {
	if (zoom_level > 1) { // Min zoom 1x (32px)
		zoom_level--;
		UpdateGridSize();
		
		// Force a complete redraw of visible items
		need_full_redraw = true;
		first_visible_row = 0; // Reset visible rows to force redraw
		last_visible_row = 0;
		UpdateViewableItems();
		Refresh(true); // Force a full refresh
		
		return zoom_level;
	}
	return zoom_level;
}

void SeamlessGridPanel::SetZoomLevel(int level) {
	if (level >= 1 && level <= 4) {
		zoom_level = level;
		UpdateGridSize();
		
		// Force a complete redraw of visible items
		need_full_redraw = true;
		first_visible_row = 0; // Reset visible rows to force redraw
		last_visible_row = 0;
		UpdateViewableItems();
		Refresh(true); // Force a full refresh
	}
}

void SeamlessGridPanel::UpdateGridSize() {
	// Calculate the actual sprite size based on zoom level
	sprite_size = 32 * zoom_level;
	
	// Clear the sprite cache when zoom level changes
	ClearSpriteCache();
	
	// Recalculate grid layout with the new size
	RecalculateGrid();
	
	// Update scroll rate to match the new size
	SetScrollRate(sprite_size / 4, sprite_size / 4);
}

// Add ClearSpriteCache method implementation before UpdateGridSize
void SeamlessGridPanel::ClearSpriteCache() {
	// Properly free all bitmap resources before clearing the cache
	for (auto& pair : sprite_cache) {
		if (pair.second.is_valid && pair.second.bitmap.IsOk()) {
			// Ensure bitmap data is released
			pair.second.bitmap = wxBitmap(); // Assign empty bitmap to release resources
			pair.second.is_valid = false;
		}
	}
	
	// Clear the sprite cache
	sprite_cache.clear();
}

// Add this method to limit the sprite cache size
void SeamlessGridPanel::ManageSpriteCache() {
	// For extremely large tilesets, be much more aggressive with cache management
	if (tileset->size() > 10000) {
		// Keep only the sprites in the current chunk and visible area
		size_t chunk_start = current_chunk * chunk_size;
		size_t chunk_end = std::min(chunk_start + static_cast<size_t>(chunk_size), tileset->size()) - 1;
		
		// Get the current visible range
		int firstVisRow = first_visible_row;
		int lastVisRow = last_visible_row;
		
		// Calculate visible indices
		int firstIndex = firstVisRow * columns;
		int lastIndex = (lastVisRow + 1) * columns - 1;
		
		// Adjust to global indices
		firstIndex = chunk_start + firstIndex;
		lastIndex = chunk_start + lastIndex;
		
		// Bound to valid range
		firstIndex = std::max(static_cast<int>(chunk_start), std::min(firstIndex, static_cast<int>(chunk_end)));
		lastIndex = std::max(static_cast<int>(chunk_start), std::min(lastIndex, static_cast<int>(chunk_end)));
		
		// Create a set of visible indices with a small margin
		std::set<int> visibleIndices;
		int margin = columns * 5; // 5 rows margin
		
		for (int i = std::max(static_cast<int>(chunk_start), firstIndex - margin); 
			 i <= std::min(static_cast<int>(chunk_end), lastIndex + margin); ++i) {
			visibleIndices.insert(i);
		}
		
		// Remove sprites that aren't in the current visible area
		std::vector<int> keysToRemove;
		for (auto& pair : sprite_cache) {
			if (visibleIndices.find(pair.first) == visibleIndices.end()) {
				keysToRemove.push_back(pair.first);
			}
		}
		
		// Remove from cache
		for (int key : keysToRemove) {
			sprite_cache.erase(key);
		}
	}
	// For large but not extreme tilesets, use moderate cache management
	else if (sprite_cache.size() > 500) {
		// Get the current visible range
		int firstIndex = first_visible_row * columns;
		int lastIndex = (last_visible_row + 1) * columns - 1;
		lastIndex = std::min(lastIndex, static_cast<int>(tileset->size()) - 1);
		
		// Create a set of visible indices with a margin
		std::set<int> visibleIndices;
		int margin = columns * 10; // 10 rows margin
		
		for (int i = std::max(0, firstIndex - margin); 
			 i <= std::min(static_cast<int>(tileset->size()) - 1, lastIndex + margin); ++i) {
			visibleIndices.insert(i);
		}
		
		// Remove sprites that aren't currently visible
		std::vector<int> keysToRemove;
		for (auto& pair : sprite_cache) {
			if (visibleIndices.find(pair.first) == visibleIndices.end()) {
				keysToRemove.push_back(pair.first);
			}
		}
		
		// Remove from cache
		for (int key : keysToRemove) {
			sprite_cache.erase(key);
		}
	}
}

void SeamlessGridPanel::CreateNavigationPanel(wxWindow* parent) {
    // Don't create if it already exists
    if (navigation_panel) return;
    
    // Create a panel to hold navigation buttons
    navigation_panel = new wxPanel(parent, wxID_ANY);
    wxBoxSizer* nav_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create Previous button
    wxButton* prev_btn = new wxButton(navigation_panel, wxID_ANY, "< Previous", 
                                     wxDefaultPosition, wxDefaultSize);
    prev_btn->Bind(wxEVT_BUTTON, &SeamlessGridPanel::OnNavigationButtonClicked, this);
    prev_btn->SetToolTip("Go to previous chunk of items");
    prev_btn->SetClientData(reinterpret_cast<void*>(-1)); // -1 means previous
    
    // Create chunk info text with a unique ID (100) for easier finding
    wxStaticText* chunk_info = new wxStaticText(navigation_panel, 100, 
                                              wxString::Format("Chunk %d/%d", current_chunk + 1, total_chunks),
                                              wxDefaultPosition, wxSize(100, -1),
                                              wxALIGN_CENTER);
    
    // Create Next button
    wxButton* next_btn = new wxButton(navigation_panel, wxID_ANY, "Next >", 
                                     wxDefaultPosition, wxDefaultSize);
    next_btn->Bind(wxEVT_BUTTON, &SeamlessGridPanel::OnNavigationButtonClicked, this);
    next_btn->SetToolTip("Go to next chunk of items");
    next_btn->SetClientData(reinterpret_cast<void*>(1)); // 1 means next
    
    // Add buttons to sizer
    nav_sizer->Add(prev_btn, 0, wxRIGHT, 5);
    nav_sizer->Add(chunk_info, 1, wxALIGN_CENTER);
    nav_sizer->Add(next_btn, 0, wxLEFT, 5);
    
    navigation_panel->SetSizer(nav_sizer);
    
    // Add to parent's sizer right after the zoom controls
    wxWindow* p = parent;
    while (p && !dynamic_cast<BrushPanel*>(p)) {
        p = p->GetParent();
    }
    
    if (p) {
        BrushPanel* bp = dynamic_cast<BrushPanel*>(p);
        if (bp) {
            // Get the sizer from the panel
            wxSizer* panel_sizer = bp->GetSizer();
            if (panel_sizer) {
                // Add the navigation panel to the sizer
                panel_sizer->Add(navigation_panel, 0, wxEXPAND | wxALL, 5);
                bp->Layout();
            }
        }
    }
    
    // Update button states
    UpdateNavigationPanel();
}

void SeamlessGridPanel::UpdateNavigationPanel() {
    if (!navigation_panel) return;
    
    // Update chunk info text - use ID 100 that we set in CreateNavigationPanel
    wxStaticText* chunk_info = dynamic_cast<wxStaticText*>(navigation_panel->FindWindow(100));
    
    if (chunk_info) {
        chunk_info->SetLabel(wxString::Format("Chunk %d/%d", current_chunk + 1, total_chunks));
    }
    
    // Update Previous button state
    wxButton* prev_btn = nullptr;
    wxButton* next_btn = nullptr;
    
    // Find the buttons
    wxWindowList& children = navigation_panel->GetChildren();
    for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
        wxButton* btn = dynamic_cast<wxButton*>(*it);
        if (btn) {
            if (btn->GetLabel().Contains("Previous")) {
                prev_btn = btn;
            } else if (btn->GetLabel().Contains("Next")) {
                next_btn = btn;
            }
        }
    }
    
    if (prev_btn) {
        prev_btn->Enable(current_chunk > 0);
    }
    
    if (next_btn) {
        next_btn->Enable(current_chunk < total_chunks - 1);
    }
}

void SeamlessGridPanel::OnNavigationButtonClicked(wxCommandEvent& event) {
    wxButton* btn = dynamic_cast<wxButton*>(event.GetEventObject());
    if (!btn) return;
    
    // Get the direction from client data (-1 for previous, 1 for next)
    intptr_t direction = reinterpret_cast<intptr_t>(btn->GetClientData());
    
    int old_chunk = current_chunk;
    
    if (direction == -1 && current_chunk > 0) {
        // Go to previous chunk
        current_chunk--;
    } else if (direction == 1 && current_chunk < total_chunks - 1) {
        // Go to next chunk
        current_chunk++;
    } else {
        return; // Invalid direction or at the edge
    }
    
    // Only proceed if the chunk actually changed
    if (old_chunk != current_chunk) {
        // Clear cache when changing chunks
        sprite_cache.clear();
        
        // Recalculate grid with new chunk
        RecalculateGrid();
        
        // Update navigation panel to show new chunk number
        UpdateNavigationPanel();
        
        // Calculate new chunk bounds
        size_t new_chunk_start = current_chunk * chunk_size;
        size_t items_in_new_chunk = std::min(static_cast<size_t>(chunk_size), 
                                           tileset->size() - new_chunk_start);
        
        // Always select the first item in the new chunk
        selected_index = static_cast<int>(new_chunk_start);
        hover_index = -1;
        
        // Reset scroll position to top
        Scroll(0, 0);
        UpdateViewableItems();
        
        // Force complete refresh
        need_full_redraw = true;
        Refresh(true);
    }
}
