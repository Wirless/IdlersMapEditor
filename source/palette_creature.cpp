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

#include "settings.h"
#include "brush.h"
#include "gui.h"
#include "palette_creature.h"
#include "creature_brush.h"
#include "spawn_brush.h"
#include "materials.h"
#include "add_creature_dialog.h"
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/textdlg.h>
#include "creature_sprite_manager.h"

// Initialize static member
wxLongLong CreaturePalettePanel::last_search_click(0);

// Define the new event ID for the Load NPCs button
#define PALETTE_LOAD_NPCS_BUTTON 1952
#define PALETTE_LOAD_MONSTERS_BUTTON 1953
#define PALETTE_PURGE_CREATURES_BUTTON 1954
#define PALETTE_SEARCH_BUTTON 1955
#define PALETTE_SEARCH_FIELD 1956
#define PALETTE_VIEW_TOGGLE_BUTTON 1957
#define PALETTE_CREATURE_LARGE_SPRITES_TOGGLE 1958
#define PALETTE_CREATURE_ZOOM_BUTTON 1959

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)
EVT_LISTBOX(PALETTE_CREATURE_LISTBOX, CreaturePalettePanel::OnListBoxChange)
EVT_COMMAND(wxID_ANY, wxEVT_COMMAND_LISTBOX_SELECTED, CreaturePalettePanel::OnSpriteSelected)
EVT_TOGGLEBUTTON(PALETTE_CREATURE_BRUSH_BUTTON, CreaturePalettePanel::OnClickCreatureBrushButton)
EVT_TOGGLEBUTTON(PALETTE_SPAWN_BRUSH_BUTTON, CreaturePalettePanel::OnClickSpawnBrushButton)
EVT_TOGGLEBUTTON(PALETTE_VIEW_TOGGLE_BUTTON, CreaturePalettePanel::OnClickViewToggle)
EVT_TOGGLEBUTTON(PALETTE_CREATURE_VIEW_STYLE_TOGGLE, CreaturePalettePanel::OnClickViewStyleToggle)
EVT_TOGGLEBUTTON(PALETTE_CREATURE_LARGE_SPRITES_TOGGLE, CreaturePalettePanel::OnClickLargeSpritesToggle)
EVT_BUTTON(PALETTE_CREATURE_ZOOM_BUTTON, CreaturePalettePanel::OnClickZoomButton)
EVT_BUTTON(PALETTE_LOAD_NPCS_BUTTON, CreaturePalettePanel::OnClickLoadNPCsButton)
EVT_BUTTON(PALETTE_LOAD_MONSTERS_BUTTON, CreaturePalettePanel::OnClickLoadMonstersButton)
EVT_BUTTON(PALETTE_PURGE_CREATURES_BUTTON, CreaturePalettePanel::OnClickPurgeCreaturesButton)
EVT_BUTTON(PALETTE_SEARCH_BUTTON, CreaturePalettePanel::OnClickSearchButton)
EVT_TEXT(PALETTE_SEARCH_FIELD, CreaturePalettePanel::OnSearchFieldText)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	tileset_choice(nullptr),
	creature_list(nullptr),
	sprite_panel(nullptr),
	seamless_panel(nullptr),
	view_toggle(nullptr),
	view_style_toggle(nullptr),
	large_sprites_toggle(nullptr),
	zoom_button(nullptr),
	view_sizer(nullptr),
	use_sprite_view(false),
	use_seamless_view(true), // Seamless is now the default
	use_large_sprites(false),
	zoom_factor(1),
	handling_event(false),
	search_field(nullptr),
	search_button(nullptr),
	load_npcs_button(nullptr),
	load_monsters_button(nullptr),
	purge_creatures_button(nullptr),
	creature_spawntime_spin(nullptr),
	spawn_size_spin(nullptr),
	creature_brush_button(nullptr),
	spawn_brush_button(nullptr) {
	
	// Create the controls
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	
	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	
	// Tileset choice
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	sidesizer->Add(tileset_choice, 0, wxEXPAND | wxALL, 5);
	
	// Search field
	wxBoxSizer* searchSizer = newd wxBoxSizer(wxHORIZONTAL);
	searchSizer->Add(newd wxStaticText(this, wxID_ANY, "Search:"), 0, wxCENTER | wxLEFT, 5);
	search_field = newd wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	searchSizer->Add(search_field, 1, wxCENTER | wxLEFT, 5);
	search_button = newd wxButton(this, PALETTE_SEARCH_BUTTON, "Go", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	searchSizer->Add(search_button, 0, wxLEFT, 5);
	sidesizer->Add(searchSizer, 0, wxEXPAND | wxTOP, 5);
	
	// Connect the focus events to disable hotkeys during typing
	search_field->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(CreaturePalettePanel::OnSearchFieldFocus), nullptr, this);
	search_field->Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(CreaturePalettePanel::OnSearchFieldKillFocus), nullptr, this);
	// Connect key down event to handle key presses in the search field
	search_field->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(CreaturePalettePanel::OnSearchFieldKeyDown), nullptr, this);

	// Create view container that will hold both list and sprite views
	view_sizer = newd wxBoxSizer(wxVERTICAL);
	
	// Create both views
	creature_list = newd SortableListBox(this, PALETTE_CREATURE_LISTBOX);
	sprite_panel = newd CreatureSpritePanel(this);
	seamless_panel = newd CreatureSeamlessGridPanel(this);
	
	// Add views to sizer (only one will be shown at a time)
	view_sizer->Add(creature_list, 1, wxEXPAND);
	view_sizer->Add(sprite_panel, 1, wxEXPAND);
	view_sizer->Add(seamless_panel, 1, wxEXPAND);
	sprite_panel->Hide(); // Initially hide the sprite view
	seamless_panel->Hide(); // Initially hide the seamless view
	
	sidesizer->Add(view_sizer, 1, wxEXPAND | wxTOP, 5);
	
	// Add buttons for loading NPCs, monsters, and purging creatures
	wxSizer* buttonSizer = newd wxBoxSizer(wxHORIZONTAL);
	
	load_npcs_button = newd wxButton(this, PALETTE_LOAD_NPCS_BUTTON, "Load NPCs Folder");
	buttonSizer->Add(load_npcs_button, 1, wxEXPAND | wxRIGHT, 5);
	
	load_monsters_button = newd wxButton(this, PALETTE_LOAD_MONSTERS_BUTTON, "Load Monsters Folder");
	buttonSizer->Add(load_monsters_button, 1, wxEXPAND | wxLEFT, 5);
	
	sidesizer->Add(buttonSizer, 0, wxEXPAND | wxTOP, 5);
	
	purge_creatures_button = newd wxButton(this, PALETTE_PURGE_CREATURES_BUTTON, "Purge Creatures");
	sidesizer->Add(purge_creatures_button, 0, wxEXPAND | wxTOP, 5);
	
	// View mode toggle
	wxBoxSizer* viewModeSizer = newd wxBoxSizer(wxHORIZONTAL);
	view_toggle = newd wxToggleButton(this, PALETTE_VIEW_TOGGLE_BUTTON, "Sprite View");
	viewModeSizer->Add(view_toggle, 1, wxEXPAND);
	
	// Large sprites toggle
	large_sprites_toggle = newd wxToggleButton(this, PALETTE_CREATURE_LARGE_SPRITES_TOGGLE, "64x64");
	large_sprites_toggle->Enable(false); // Only enabled in sprite view
	viewModeSizer->Add(large_sprites_toggle, 1, wxEXPAND | wxLEFT, 5);
	
	// Zoom button
	zoom_button = newd wxButton(this, PALETTE_CREATURE_ZOOM_BUTTON, "Zoom 2x");
	zoom_button->Enable(false); // Only enabled in sprite view with large sprites
	viewModeSizer->Add(zoom_button, 1, wxEXPAND | wxLEFT, 5);
	
	sidesizer->Add(viewModeSizer, 0, wxEXPAND | wxTOP, 5);
	
	// Add brush radio buttons
	wxToggleButton* creature_radio = newd wxToggleButton(this, PALETTE_CREATURE_BRUSH_BUTTON, "Creature");
	wxToggleButton* spawn_radio = newd wxToggleButton(this, PALETTE_SPAWN_BRUSH_BUTTON, "Spawn");
	
	wxBoxSizer* radiosizer = newd wxBoxSizer(wxHORIZONTAL);
	radiosizer->Add(creature_radio, 1, wxEXPAND);
	radiosizer->Add(spawn_radio, 1, wxEXPAND);
	
	sidesizer->Add(radiosizer, 0, wxEXPAND | wxTOP, 5);
	
	// Store references to the radio buttons
	creature_brush_button = creature_radio;
	spawn_brush_button = spawn_radio;
	
	// Add spawn settings
	wxFlexGridSizer* settings_sizer = newd wxFlexGridSizer(2, 5, 5);
	settings_sizer->AddGrowableCol(1);
	settings_sizer->Add(newd wxStaticText(this, wxID_ANY, "Spawntime"));
	
	creature_spawntime_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), 
											wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 3600, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	
	settings_sizer->Add(creature_spawntime_spin, 0, wxEXPAND);
	settings_sizer->Add(newd wxStaticText(this, wxID_ANY, "Size"));
	
	spawn_size_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), 
									  wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 5);
	
	settings_sizer->Add(spawn_size_spin, 0, wxEXPAND);
	
	sidesizer->Add(settings_sizer, 0, wxEXPAND | wxTOP, 5);
	topsizer->Add(sidesizer, 1, wxEXPAND | wxALL, 5);
	
	SetSizerAndFit(topsizer);
	
	// Load all creatures
	TilesetContainer tilesets;
	
	// Create a list of all creature tilesets
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		const TilesetCategory* tc = iter->second->getCategory(TILESET_CREATURE);
		if (tc && tc->size() > 0) {
			if (tilesets.count(iter->first) == 0)
				tilesets[iter->first] = iter->second;
		}
	}
	
	// Add them to the choice control
	for (TilesetContainer::const_iterator iter = tilesets.begin(); iter != tilesets.end(); ++iter) {
		tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(iter->second->getCategory(TILESET_CREATURE)));
	}
	
	// Add the rest of the tilesets as before
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->first == "All Creatures") continue;  // Skip since we already added it

		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if (iter->second->name == "NPCs" || iter->second->name == "Others") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			tileset_choice->Append(wxstr(ts->name), rtsc);
		}
	}
	SelectTileset(0);
}

CreaturePalettePanel::~CreaturePalettePanel() {
	////
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	if (use_sprite_view) {
		if (use_seamless_view) {
			// Select first creature in seamless panel
			if (!seamless_panel->creatures.empty()) {
				seamless_panel->SelectIndex(0);
			}
		} else {
			// Select first creature in sprite panel
			if (!sprite_panel->creatures.empty()) {
				sprite_panel->SelectIndex(0);
			}
		}
	} else {
		// Select first creature in list
		if (creature_list->GetCount() > 0) {
			creature_list->SetSelection(0);
		}
	}
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	if (use_sprite_view) {
		if (use_seamless_view) {
			return seamless_panel->GetSelectedBrush();
		} else {
			return sprite_panel->GetSelectedBrush();
		}
	} else {
		if (creature_list->GetCount() > 0 && creature_list->GetSelection() != wxNOT_FOUND) {
			const Brush* brush = reinterpret_cast<const Brush*>(creature_list->GetClientData(creature_list->GetSelection()));
			if (brush) {
				if (g_gui.GetCurrentBrush() != brush) {
					g_gui.SelectBrush(const_cast<Brush*>(brush), TILESET_CREATURE);
				}
			}
			return const_cast<Brush*>(brush);
		}
		return nullptr;
	}
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		if (use_sprite_view) {
			if (use_seamless_view) {
				seamless_panel->SelectBrush(nullptr);
			} else {
				sprite_panel->SelectBrush(nullptr);
			}
		} else {
			creature_list->SetSelection(wxNOT_FOUND);
		}
		return true;
	}
	
	if (whatbrush->isCreature()) {
		if (use_sprite_view) {
			if (use_seamless_view) {
				return seamless_panel->SelectBrush(whatbrush);
			} else {
				return sprite_panel->SelectBrush(whatbrush);
			}
		} else {
			for (size_t i = 0; i < creature_list->GetCount(); ++i) {
				const Brush* tmp_brush = reinterpret_cast<const Brush*>(creature_list->GetClientData(i));
				if (tmp_brush == whatbrush) {
					creature_list->SetSelection(i);
					return true;
				}
			}
		}
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate() {
	tileset_choice->Clear();
	g_materials.createOtherTileset();

	// Create an "All Creatures" tileset that contains all creatures
	Tileset* allCreatures = nullptr;
	TilesetCategory* allCreaturesCategory = nullptr;
	
	// Check if the "All Creatures" tileset already exists, if not create it
	if (g_materials.tilesets.count("All Creatures") > 0) {
		allCreatures = g_materials.tilesets["All Creatures"];
		allCreaturesCategory = allCreatures->getCategory(TILESET_CREATURE);
		allCreaturesCategory->brushlist.clear();
	} else {
		allCreatures = newd Tileset(g_brushes, "All Creatures");
		g_materials.tilesets["All Creatures"] = allCreatures;
		allCreaturesCategory = allCreatures->getCategory(TILESET_CREATURE);
	}

	// Track added creatures to avoid duplicates
	std::set<std::string> addedCreatures;

	// Collect all creature brushes from all tilesets
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->first == "All Creatures") continue;  // Skip ourselves to avoid duplication
		
		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			// Add all creature brushes from this category to the All Creatures category
			for (BrushVector::const_iterator brushIter = tsc->brushlist.begin(); brushIter != tsc->brushlist.end(); ++brushIter) {
				if ((*brushIter)->isCreature()) {
					// Only add if not already added (avoid duplicates)
					std::string creatureName = (*brushIter)->getName();
					if (addedCreatures.count(creatureName) == 0) {
						allCreaturesCategory->brushlist.push_back(*brushIter);
						addedCreatures.insert(creatureName);
					}
				}
			}
		}
	}

	// Add the "All Creatures" tileset first
	tileset_choice->Append(wxstr(allCreatures->name), allCreaturesCategory);

	// Add the rest of the tilesets as before
	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->first == "All Creatures") continue;  // Skip since we already added it

		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if (iter->second->name == "NPCs" || iter->second->name == "Others") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			tileset_choice->Append(wxstr(ts->name), rtsc);
		}
	}
	SelectTileset(0);
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	return spawn_size_spin->SetValue(size);
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectTileset(size_t index) {
	ASSERT(tileset_choice->GetCount() >= index);

	creature_list->Clear();
	sprite_panel->Clear();
	seamless_panel->Clear();
	
	if (tileset_choice->GetCount() == 0) {
		// No tilesets :(
		creature_brush_button->Enable(false);
	} else {
		const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
		
		// Add creatures to appropriate view
		if (use_sprite_view) {
			sprite_panel->LoadCreatures(tsc->brushlist);
		} else {
			// Add creatures to list view
			for (BrushVector::const_iterator iter = tsc->brushlist.begin();
				iter != tsc->brushlist.end();
				++iter) {
				// Check if this creature has outfit colors
				CreatureBrush* cb = dynamic_cast<CreatureBrush*>(*iter);
				if (cb && cb->getType()) {
					const Outfit& outfit = cb->getType()->outfit;
					
					// Create name for list display
					std::string name = (*iter)->getName();
					
					// If this creature has custom outfit colors, add an indicator to the name
					if (outfit.lookHead > 0 || outfit.lookBody > 0 || outfit.lookLegs > 0 || outfit.lookFeet > 0) {
						name += " [outfit]";
					}
					
					creature_list->Append(wxstr(name), *iter);
				} else {
					// Regular creature without custom outfit
					creature_list->Append(wxstr((*iter)->getName()), *iter);
				}
			}
			creature_list->Sort();
		}
		
		// Apply filter if search field has text
		if (!search_field->IsEmpty()) {
			FilterCreatures(search_field->GetValue());
		} else {
			SelectCreature(0);
		}

		tileset_choice->SetSelection(index);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	// Select creature by index
	if (use_sprite_view) {
		// In sprite view, select by index
		if (index < sprite_panel->creatures.size()) {
			sprite_panel->SelectIndex(index);
		}
	} else {
		// In list view, select by index
		if (creature_list->GetCount() > 0 && index < creature_list->GetCount()) {
			creature_list->SetSelection(index);
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (use_sprite_view) {
		// In sprite view, find and select brush by name
		for (size_t i = 0; i < sprite_panel->creatures.size(); ++i) {
			if (sprite_panel->creatures[i]->getName() == name) {
				sprite_panel->SelectIndex(i);
				break;
			}
		}
	} else {
		// In list view, select by name string
		if (creature_list->GetCount() > 0) {
			if (!creature_list->SetStringSelection(wxstr(name))) {
				creature_list->SetSelection(0);
			}
		}
	}

	SelectCreatureBrush();
}

void CreaturePalettePanel::SelectCreatureBrush() {
	bool has_selection = false;
	
	if (use_sprite_view) {
		has_selection = (sprite_panel->GetSelectedBrush() != nullptr);
	} else {
		has_selection = (creature_list->GetCount() > 0);
	}
	
	if (has_selection) {
		creature_brush_button->Enable(true);
		creature_brush_button->SetValue(true);
		spawn_brush_button->SetValue(false);
	} else {
		creature_brush_button->Enable(false);
		SelectSpawnBrush();
	}
}

void CreaturePalettePanel::SelectSpawnBrush() {
	// g_gui.house_exit_brush->setHouse(house);
	creature_brush_button->SetValue(false);
	spawn_brush_button->SetValue(true);
}

void CreaturePalettePanel::OnTilesetChange(wxCommandEvent& event) {
	SelectTileset(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListBoxChange(wxCommandEvent& event) {
	// Get the selected brush before updating
	Brush* old_brush = g_gui.GetCurrentBrush();
	
	// Update selection
	SelectCreature(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	
	// Get the newly selected brush
	Brush* new_brush = g_gui.GetCurrentBrush();
	
	// If we selected the same brush, first set to nullptr then reselect
	if(old_brush && new_brush && old_brush == new_brush) {
		g_gui.SelectBrush(nullptr, TILESET_CREATURE);
	}
	
	// Now select the brush (either for the first time or re-selecting)
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickCreatureBrushButton(wxCommandEvent& event) {
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickSpawnBrushButton(wxCommandEvent& event) {
	SelectSpawnBrush();
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnClickLoadNPCsButton(wxCommandEvent& event) {
	wxDirDialog dlg(g_gui.root, "Select NPC folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxString folder = dlg.GetPath();
		LoadNPCsFromFolder(folder);
	}
}

void CreaturePalettePanel::OnClickLoadMonstersButton(wxCommandEvent& event) {
	wxDirDialog dlg(g_gui.root, "Select Monsters folder", "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxString folder = dlg.GetPath();
		LoadMonstersFromFolder(folder);
	}
}

void CreaturePalettePanel::OnClickPurgeCreaturesButton(wxCommandEvent& event) {
	// Confirmation dialog
	long response = wxMessageBox("Are you sure you want to purge all creatures from the palette? This cannot be undone.", 
		"Confirm Purge", wxYES_NO | wxICON_QUESTION, g_gui.root);
	
	if (response == wxYES) {
		PurgeCreaturePalettes();
	}
}

bool CreaturePalettePanel::LoadNPCsFromFolder(const wxString& folder) {
	// Get all .xml files in the folder
	wxArrayString files;
	wxDir::GetAllFiles(folder, &files, "*.xml", wxDIR_FILES);
	
	if (files.GetCount() == 0) {
		wxMessageBox("No XML files found in the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
	
	wxArrayString warnings;
	int loadedCount = 0;
	
	for (size_t i = 0; i < files.GetCount(); ++i) {
		wxString error;
		bool ok = g_creatures.importXMLFromOT(FileName(files[i]), error, warnings);
		if (ok) {
			loadedCount++;
		} else {
			warnings.Add("Failed to load " + files[i] + ": " + error);
		}
	}
	
	if (!warnings.IsEmpty()) {
		g_gui.ListDialog("NPC loader messages", warnings);
	}
	
	if (loadedCount > 0) {
		g_gui.PopupDialog("Success", wxString::Format("Successfully loaded %d NPC files.", loadedCount), wxOK);
		
		// Refresh the palette
		g_gui.RefreshPalettes();
		
		// Refresh current tileset and creature list
		OnUpdate();
		
		return true;
	} else {
		wxMessageBox("No NPCs could be loaded from the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
}

bool CreaturePalettePanel::LoadMonstersFromFolder(const wxString& folder) {
	// Get all .xml files in the folder
	wxArrayString files;
	wxDir::GetAllFiles(folder, &files, "*.xml", wxDIR_FILES);
	
	if (files.GetCount() == 0) {
		wxMessageBox("No XML files found in the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
	
	wxArrayString warnings;
	int loadedCount = 0;
	
	for (size_t i = 0; i < files.GetCount(); ++i) {
		wxString error;
		bool ok = g_creatures.importXMLFromOT(FileName(files[i]), error, warnings);
		if (ok) {
			loadedCount++;
		} else {
			warnings.Add("Failed to load " + files[i] + ": " + error);
		}
	}
	
	if (!warnings.IsEmpty()) {
		g_gui.ListDialog("Monster loader messages", warnings);
	}
	
	if (loadedCount > 0) {
		g_gui.PopupDialog("Success", wxString::Format("Successfully loaded %d monster files.", loadedCount), wxOK);
		
		// Refresh the palette
		g_gui.RefreshPalettes();
		
		// Refresh current tileset and creature list
		OnUpdate();
		
		return true;
	} else {
		wxMessageBox("No monsters could be loaded from the selected folder.", "Error", wxOK | wxICON_INFORMATION, g_gui.root);
		return false;
	}
}

bool CreaturePalettePanel::PurgeCreaturePalettes() {
	// Track success
	bool success = false;
	
	// Create vectors to store brushes that need to be removed
	std::vector<Brush*> brushesToRemove;
	
	// Collect creature brushes from the "NPCs", "Others", and "All Creatures" tilesets
	if (g_materials.tilesets.count("All Creatures") > 0) {
		Tileset* allCreaturesTileset = g_materials.tilesets["All Creatures"];
		TilesetCategory* allCreaturesCategory = allCreaturesTileset->getCategory(TILESET_CREATURE);
		if (allCreaturesCategory) {
			allCreaturesCategory->brushlist.clear();
			success = true;
		}
	}
	
	if (g_materials.tilesets.count("NPCs") > 0) {
		Tileset* npcTileset = g_materials.tilesets["NPCs"];
		TilesetCategory* npcCategory = npcTileset->getCategory(TILESET_CREATURE);
		if (npcCategory) {
			for (BrushVector::iterator it = npcCategory->brushlist.begin(); it != npcCategory->brushlist.end(); ++it) {
				brushesToRemove.push_back(*it);
			}
			npcCategory->brushlist.clear();
			success = true;
		}
	}
	
	if (g_materials.tilesets.count("Others") > 0) {
		Tileset* othersTileset = g_materials.tilesets["Others"];
		TilesetCategory* othersCategory = othersTileset->getCategory(TILESET_CREATURE);
		if (othersCategory) {
			for (BrushVector::iterator it = othersCategory->brushlist.begin(); it != othersCategory->brushlist.end(); ++it) {
				brushesToRemove.push_back(*it);
			}
			othersCategory->brushlist.clear();
			success = true;
		}
	}
	
	// Remove creature brushes from g_brushes
	// We need to collect the keys to remove first to avoid modifying the map during iteration
	const BrushMap& allBrushes = g_brushes.getMap();
	std::vector<std::string> brushKeysToRemove;
	
	for (BrushMap::const_iterator it = allBrushes.begin(); it != allBrushes.end(); ++it) {
		if (it->second && it->second->isCreature()) {
			brushKeysToRemove.push_back(it->first);
		}
	}
	
	// Now remove the brushes from g_brushes
	for (std::vector<std::string>::iterator it = brushKeysToRemove.begin(); it != brushKeysToRemove.end(); ++it) {
		g_brushes.removeBrush(*it);
	}
	
	// Delete the brush objects to prevent memory leaks
	for (std::vector<Brush*>::iterator it = brushesToRemove.begin(); it != brushesToRemove.end(); ++it) {
		delete *it;
	}
	
	// Clear creature database
	g_creatures.clear();
	
	// Recreate empty tilesets if needed
	g_materials.createOtherTileset();
	
	// Refresh the palette
	g_gui.RefreshPalettes();
	
	// Refresh current tileset and creature list in this panel
	OnUpdate();
	
	if (success) {
		g_gui.PopupDialog("Success", "All creatures have been purged from the palettes.", wxOK);
	} else {
		wxMessageBox("There was a problem purging the creatures.", "Error", wxOK | wxICON_ERROR, g_gui.root);
	}
	
	return success;
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event) {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SetBrushSize(event.GetPosition());
		handling_event = false;
	}
}

// Add this as a class member in the header file first
static wxLongLong last_search_click;

void CreaturePalettePanel::OnClickSearchButton(wxCommandEvent& event) {
    // Get the text from the search field
    wxString searchText = search_field->GetValue();
    
    // Check if this is a double-click (within 500ms)
    wxLongLong now = wxGetLocalTimeMillis();
    bool is_double_click = (now - last_search_click) < 500;
    last_search_click = now;
    
    // First try to find the creature
    FilterCreatures(searchText);
    
    // Show add dialog if:
    // 1. No results found, OR
    // 2. Double-clicked Go button
    if ((use_sprite_view && sprite_panel->creatures.empty()) || 
        (!use_sprite_view && creature_list->GetCount() == 0) ||
        is_double_click) {
        
        // Show dialog to add new creature
        AddCreatureDialog dialog(this, searchText);
        if (dialog.ShowModal() == wxID_OK) {
            // Get the created creature type
            CreatureType* new_creature = dialog.GetCreatureType();
            if (new_creature) {
                // Get the brush that was created in the dialog
                CreatureBrush* brush = new_creature->brush;
                if (brush) {
                    // Add to current view directly
                    if (use_sprite_view) {
                        if (use_seamless_view) {
                            seamless_panel->creatures.push_back(brush);
                            seamless_panel->SelectIndex(seamless_panel->creatures.size() - 1);
                            seamless_panel->RecalculateGrid();
                            seamless_panel->Refresh();
                        } else {
                            sprite_panel->creatures.push_back(brush);
                            sprite_panel->SelectIndex(sprite_panel->creatures.size() - 1);
                            sprite_panel->RecalculateGrid();
                            sprite_panel->Refresh();
                        }
                    } else {
                        // Add to list view
                        std::string name = brush->getName();
                        if (new_creature->outfit.lookHead > 0 || new_creature->outfit.lookBody > 0 || 
                            new_creature->outfit.lookLegs > 0 || new_creature->outfit.lookFeet > 0) {
                            name += " [outfit]";
                        }
                        creature_list->Append(wxstr(name), brush);
                        creature_list->SetSelection(creature_list->GetCount() - 1);
                    }

                    // Select the brush for use
                    SelectCreatureBrush();
                    g_gui.SelectBrush(brush, TILESET_CREATURE);
                }
            }
        }
    }
}

void CreaturePalettePanel::OnSearchFieldText(wxCommandEvent& event) {
	// Filter as user types
	FilterCreatures(search_field->GetValue());
}

void CreaturePalettePanel::OnSearchFieldFocus(wxFocusEvent& event) {
	// Disable hotkeys when search field receives focus
	g_gui.DisableHotkeys();
	event.Skip();
}

void CreaturePalettePanel::OnSearchFieldKillFocus(wxFocusEvent& event) {
	// Re-enable hotkeys when search field loses focus
	g_gui.EnableHotkeys();
	event.Skip();
}

void CreaturePalettePanel::OnSearchFieldKeyDown(wxKeyEvent& event) {
	// Handle Enter key specially
	if (event.GetKeyCode() == WXK_RETURN) {
		FilterCreatures(search_field->GetValue());
	} else if (event.GetKeyCode() == WXK_ESCAPE) {
		// Clear search field and reset the list on Escape
		search_field->Clear();
		FilterCreatures(wxEmptyString);
		// Set focus back to the map
		wxWindow* mapCanvas = g_gui.root->FindWindowByName("MapCanvas");
		if (mapCanvas) {
			mapCanvas->SetFocus();
		}
	} else {
		// Process the event normally for all other keys
		event.Skip();
	}
}

void CreaturePalettePanel::FilterCreatures(const wxString& search_text) {
	if (tileset_choice->GetCount() == 0) return;
	
	// If search is empty, reset to show all creatures
	if (search_text.IsEmpty()) {
		int currentSelection = tileset_choice->GetSelection();
		if (currentSelection != wxNOT_FOUND) {
			SelectTileset(currentSelection);
		}
		return;
	}
	
	wxString searchLower = search_text.Lower();
	
	// Check if we're searching for a specific looktype (format: "lt:123" or "looktype:123")
	bool isLooktypeSearch = false;
	int searchLooktype = 0;
	
	if (searchLower.StartsWith("lt:") || searchLower.StartsWith("looktype:")) {
		wxString looktypeStr = searchLower.AfterFirst(':');
		if (looktypeStr.ToInt(&searchLooktype)) {
			isLooktypeSearch = true;
		}
	}
	
	// Clear current content
	BrushVector filtered_brushes;
	std::set<std::string> seenCreatures;  // To avoid duplicates in "All Creatures"
	
	// Get current category
	int index = tileset_choice->GetSelection();
	if (index == wxNOT_FOUND) return;
	
	const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
	bool isAllCreaturesCategory = (tileset_choice->GetString(index) == "All Creatures");
	
	for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
		if (!(*iter)->isCreature()) continue;
		
		CreatureBrush* creatureBrush = dynamic_cast<CreatureBrush*>(*iter);
		if (!creatureBrush) continue;
		
		std::string baseCreatureName = (*iter)->getName();
		wxString name = wxstr(baseCreatureName).Lower();
		
		// For "All Creatures" category, don't add duplicates
		if (!isAllCreaturesCategory && seenCreatures.count(baseCreatureName) > 0) {
			continue;
		}
		
		bool match = false;
		
		// Check if this is a looktype search
		if (isLooktypeSearch) {
			// Match by looktype
			if (creatureBrush->getType() && creatureBrush->getType()->outfit.lookType == searchLooktype) {
				match = true;
			}
		} else {
			// Standard name search
			if (name.Find(searchLower) != wxNOT_FOUND) {
				match = true;
			}
		}
		
		if (match) {
			filtered_brushes.push_back(*iter);
			seenCreatures.insert(baseCreatureName);
		}
	}
	
	// Apply the filtered list to the appropriate view
	if (use_sprite_view) {
		// Update sprite view
		sprite_panel->Clear();
		sprite_panel->LoadCreatures(filtered_brushes);
	} else {
		// Update list view
		creature_list->Clear();
		
		for (BrushVector::const_iterator iter = filtered_brushes.begin(); iter != filtered_brushes.end(); ++iter) {
			CreatureBrush* cb = dynamic_cast<CreatureBrush*>(*iter);
			if (cb && cb->getType()) {
				const Outfit& outfit = cb->getType()->outfit;
				
				// Create name for list display
				std::string name = (*iter)->getName();
				
				// If this creature has custom outfit colors, add an indicator to the name
				if (outfit.lookHead > 0 || outfit.lookBody > 0 || outfit.lookLegs > 0 || outfit.lookFeet > 0) {
					name += " [outfit]";
				}
				
				creature_list->Append(wxstr(name), *iter);
			} else {
				// Regular creature without custom outfit
				creature_list->Append(wxstr((*iter)->getName()), *iter);
			}
		}
		
		// Sort the filtered list
		creature_list->Sort();
	}
	
	// Select first result if any
	if (!filtered_brushes.empty()) {
		SelectCreature(0);
		creature_brush_button->Enable(true);
	} else {
		creature_brush_button->Enable(false);
	}
}

void CreaturePalettePanel::OnSpriteSelected(wxCommandEvent& event) {
	Brush* old_brush = g_gui.GetCurrentBrush();
	
	// Update selection
	SelectCreatureBrush();
	g_gui.ActivatePalette(GetParentPalette());
	
	// Get the newly selected brush
	Brush* new_brush = g_gui.GetCurrentBrush();
	
	// If we selected the same brush, first set to nullptr then reselect
	if(old_brush && new_brush && old_brush == new_brush) {
		g_gui.SelectBrush(nullptr, TILESET_CREATURE);
	}
	
	// Now select the brush (either for the first time or re-selecting)
	g_gui.SelectBrush();
}

// New method to switch between list and sprite view
void CreaturePalettePanel::SetViewMode(bool use_sprites) {
	// Store original selection
	Brush* selected_brush = GetSelectedBrush();
	
	// Update mode flag
	use_sprite_view = use_sprites;
	
	// Update UI elements
	view_toggle->SetValue(use_sprites);
	large_sprites_toggle->Enable(use_sprites);  // Only enable large sprites toggle in sprite view mode
	zoom_button->Enable(use_sprites && use_large_sprites); // Only enable zoom button in large sprite mode
	
	if (use_sprites) {
		// Always use seamless view when sprite view is enabled
		use_seamless_view = true;
		
		// Switch to sprite view
		creature_list->Hide();
		sprite_panel->Hide();
		seamless_panel->Show();
		
		// Load creatures from the current category
		int index = tileset_choice->GetSelection();
		if (index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
			
			// Determine base cell size
			int base_cell_size = use_large_sprites ? 128 : 32;
			int cell_size = base_cell_size;
			
			// Apply zoom factor to cell size if in large mode
			if (use_large_sprites && zoom_factor > 1) {
				cell_size = base_cell_size * zoom_factor;
			}
			
			// Pre-generate creature sprites at their natural sizes
			g_creature_sprites.clear(); // Clear cache to ensure new sprites are generated
			
			// Also reset sprite dimensions cache in the view panel
			if (seamless_panel) {
				seamless_panel->sprite_dimensions.clear();
			}
			
			// Pre-calculate and store natural sizes for all creatures
			for (size_t i = 0; i < tsc->brushlist.size(); ++i) {
				Brush* brush = tsc->brushlist[i];
				if (brush->isCreature()) {
					CreatureBrush* cb = static_cast<CreatureBrush*>(brush);
					if (cb && cb->getType()) {
						CreatureType* type = cb->getType();
						
						// Calculate natural size for this creature
						int natural_size = 32;
						if (seamless_panel) {
							natural_size = seamless_panel->GetCreatureNaturalSize(type);
							
							// Store in the dimensions map for later use
							for (size_t j = 0; j < seamless_panel->creatures.size(); ++j) {
								CreatureBrush* panel_cb = static_cast<CreatureBrush*>(seamless_panel->creatures[j]);
								if (panel_cb && panel_cb->getType() == type) {
									seamless_panel->sprite_dimensions[j] = natural_size;
									break;
								}
							}
						}
						
						// Generate sprite at its natural size
						const Outfit& outfit = type->outfit;
						if (outfit.lookHead || outfit.lookBody || outfit.lookLegs || outfit.lookFeet) {
							g_creature_sprites.getSpriteBitmap(outfit.lookType, outfit.lookHead, outfit.lookBody, 
									outfit.lookLegs, outfit.lookFeet, natural_size, natural_size);
						} else {
							g_creature_sprites.getSpriteBitmap(outfit.lookType, natural_size, natural_size);
						}
					}
				}
			}
			
			// Update the cell size
			seamless_panel->sprite_size = cell_size;
			seamless_panel->need_full_redraw = true;
			seamless_panel->RecalculateGrid();
			seamless_panel->LoadCreatures(tsc->brushlist);
		}
	} else {
		// Switch to list view
		sprite_panel->Hide();
		seamless_panel->Hide();
		creature_list->Show();
	}
	
	// Update layout
	view_sizer->Layout();
	
	// Restore selection
	if (selected_brush) {
		SelectBrush(selected_brush);
	}
}

void CreaturePalettePanel::OnClickViewToggle(wxCommandEvent& event) {
	SetViewMode(view_toggle->GetValue());
}

void CreaturePalettePanel::OnClickViewStyleToggle(wxCommandEvent& event) {
	SetViewStyle(view_style_toggle->GetValue());
}

// ============================================================================
// CreatureSpritePanel - Panel to display creature sprites in a grid

BEGIN_EVENT_TABLE(CreatureSpritePanel, wxScrolledWindow)
EVT_PAINT(CreatureSpritePanel::OnPaint)
EVT_SIZE(CreatureSpritePanel::OnSize)
EVT_LEFT_DOWN(CreatureSpritePanel::OnMouseClick)
EVT_MOTION(CreatureSpritePanel::OnMouseMove)
END_EVENT_TABLE()

CreatureSpritePanel::CreatureSpritePanel(wxWindow* parent) : 
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS),
	columns(0),
	sprite_size(40),
	padding(6),
	selected_index(-1),
	hover_index(-1),
	buffer(nullptr) {
	
	// Set background color
	SetBackgroundColour(wxColour(245, 245, 245));
	
	// Enable scrolling
	SetScrollRate(1, 10);
}

CreatureSpritePanel::~CreatureSpritePanel() {
	delete buffer;
}

void CreatureSpritePanel::Clear() {
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	Refresh();
}

void CreatureSpritePanel::LoadCreatures(const BrushVector& brushlist) {
	// Clear any existing creatures
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	
	// Copy valid creature brushes
	for (BrushVector::const_iterator iter = brushlist.begin(); iter != brushlist.end(); ++iter) {
		if ((*iter)->isCreature()) {
			creatures.push_back(*iter);
		}
	}
	
	// Select first creature if any
	if (!creatures.empty()) {
		selected_index = 0;
	}
	
	// Calculate layout and refresh
	RecalculateGrid();
	Refresh();
}

void CreatureSpritePanel::RecalculateGrid() {
	// Get the client size of the panel
	int panel_width, panel_height;
	GetClientSize(&panel_width, &panel_height);
	
	// Calculate number of columns based on available width
	columns = std::max(1, (panel_width - padding) / (sprite_size + padding));
	
	// Calculate number of rows
	int rows = creatures.empty() ? 0 : (creatures.size() + columns - 1) / columns;
	
	// Set virtual size for scrolling
	int virtual_height = rows * (sprite_size + padding) + padding;
	SetVirtualSize(panel_width, virtual_height);
	
	// Recreate buffer with new size if needed
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	if (panel_width > 0 && panel_height > 0) {
		buffer = new wxBitmap(panel_width, panel_height);
	}
}

void CreatureSpritePanel::OnPaint(wxPaintEvent& event) {
	// Use wxAutoBufferedPaintDC for flicker-free drawing
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Get visible region
	int x_start, y_start;
	GetViewStart(&x_start, &y_start);
	int ppuX, ppuY;
	GetScrollPixelsPerUnit(&ppuX, &ppuY);
	y_start *= ppuY;
	
	int width, height;
	GetClientSize(&width, &height);
	
	// Calculate first and last visible row
	int first_row = std::max(0, y_start / (sprite_size + padding));
	int last_row = std::min((int)((creatures.size() + columns - 1) / columns), 
						   (y_start + height) / (sprite_size + padding) + 1);
	
	// Draw visible sprites
	for (int row = first_row; row < last_row; ++row) {
		for (int col = 0; col < columns; ++col) {
			int index = row * columns + col;
			if (index < (int)creatures.size()) {
				int x = padding + col * (sprite_size + padding);
				int y = padding + row * (sprite_size + padding);
				
				CreatureType* ctype = static_cast<CreatureBrush*>(creatures[index])->getType();
				DrawSprite(dc, x, y, ctype, index == selected_index);
			}
		}
	}
}

void CreatureSpritePanel::DrawSprite(wxDC& dc, int x, int y, CreatureType* ctype, bool selected) {
	if (!ctype) return;
	
	// Background
	if (selected) {
		dc.SetBrush(wxBrush(wxColour(0x80, 0x80, 0x80)));
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	}
	
	// Determine the target sprite size (64x64 for large mode, otherwise same as cell size)
	int actual_sprite_size = sprite_size > 64 ? 64 : sprite_size;
	
	// Get or create sprite bitmap at the correct size
	wxBitmap* bitmap = nullptr;
	if (ctype->outfit.lookType != 0) {
		if (ctype->outfit.lookHead || ctype->outfit.lookBody || ctype->outfit.lookLegs || ctype->outfit.lookFeet) {
			bitmap = g_creature_sprites.getSpriteBitmap(
				ctype->outfit.lookType,
				ctype->outfit.lookHead,
				ctype->outfit.lookBody,
				ctype->outfit.lookLegs,
				ctype->outfit.lookFeet,
				actual_sprite_size, actual_sprite_size);
		} else {
			bitmap = g_creature_sprites.getSpriteBitmap(ctype->outfit.lookType, actual_sprite_size, actual_sprite_size);
		}
		
		if (bitmap) {
			// Calculate position to center the sprite in the cell
			int offsetX = (sprite_size - actual_sprite_size) / 2;
			int offsetY = (sprite_size - actual_sprite_size) / 2;
			
			// Draw the sprite centered in the cell
			dc.DrawBitmap(*bitmap, x + offsetX, y + offsetY, true);
		}
	}
}

void CreatureSpritePanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	Refresh();
}

void CreatureSpritePanel::OnScroll(wxScrollWinEvent& event) {
	Refresh();
	event.Skip();
}

void CreatureSpritePanel::OnMouseClick(wxMouseEvent& event) {
	// Get the position in scroll coordinates
	int x, y;
	CalcUnscrolledPosition(event.GetX(), event.GetY(), &x, &y);
	
	// Find which sprite was clicked
	int index = GetSpriteIndexAt(x, y);
	
	if (index >= 0 && index < (int)creatures.size()) {
		SelectIndex(index);
		
		// Send selection event to parent
		wxCommandEvent selectionEvent(wxEVT_COMMAND_LISTBOX_SELECTED);
		selectionEvent.SetEventObject(this);
		GetParent()->GetEventHandler()->ProcessEvent(selectionEvent);
	}
}

void CreatureSpritePanel::OnMouseMove(wxMouseEvent& event) {
	// Update hover effect if needed
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != hover_index) {
		// Only redraw the cells that changed, not the entire panel
		int old_hover = hover_index;
		hover_index = index;
		
		// If we had a previous hover, just redraw that cell
		if (old_hover >= 0 && old_hover < static_cast<int>(creatures.size())) {
			int old_row = old_hover / columns;
			int old_col = old_hover % columns;
			int x = padding + old_col * (sprite_size + padding);
			int y = padding + old_row * (sprite_size + padding);
			wxRect oldRect(x, y, sprite_size, sprite_size);
			RefreshRect(oldRect, false);
		}
		
		// If we have a new hover, just redraw that cell
		if (hover_index >= 0 && hover_index < static_cast<int>(creatures.size())) {
			int new_row = hover_index / columns;
			int new_col = hover_index % columns;
			int x = padding + new_col * (sprite_size + padding);
			int y = padding + new_row * (sprite_size + padding);
			wxRect newRect(x, y, sprite_size, sprite_size);
			RefreshRect(newRect, false);
		}
	}
	
	event.Skip();
}

int CreatureSpritePanel::GetSpriteIndexAt(int x, int y) const {
	// Calculate the column and row
	int col = (x - padding) / (sprite_size + padding);
	int row = (y - padding) / (sprite_size + padding);
	
	// Check if within sprite bounds
	int sprite_x = padding + col * (sprite_size + padding);
	int sprite_y = padding + row * (sprite_size + padding);
	
	if (x >= sprite_x && x < sprite_x + sprite_size &&
		y >= sprite_y && y < sprite_y + sprite_size) {
		
		// Convert to index
		int index = row * columns + col;
		if (index >= 0 && index < (int)creatures.size()) {
			return index;
		}
	}
	
	return -1;
}

void CreatureSpritePanel::SelectIndex(int index) {
	if (index >= 0 && index < (int)creatures.size() && index != selected_index) {
		selected_index = index;
		Refresh();
		
		// Ensure the selected creature is visible
		if (selected_index >= 0) {
			int row = selected_index / columns;
			int col = selected_index % columns;
			int x = padding + col * (sprite_size + padding);
			int y = padding + row * (sprite_size + padding);
			
			// Scroll to make the selected creature visible
			int client_width, client_height;
			GetClientSize(&client_width, &client_height);
			
			int x_scroll, y_scroll;
			GetViewStart(&x_scroll, &y_scroll);
			
			// Adjust vertical scroll if needed
			if (y < y_scroll) {
				Scroll(-1, y / 10); // / 10 because of scroll rate
			} else if (y + sprite_size > y_scroll + client_height) {
				Scroll(-1, (y + sprite_size - client_height) / 10 + 1);
			}
		}
	}
}

Brush* CreatureSpritePanel::GetSelectedBrush() const {
	if (selected_index >= 0 && selected_index < (int)creatures.size()) {
		return creatures[selected_index];
	}
	return nullptr;
}

bool CreatureSpritePanel::SelectBrush(const Brush* brush) {
	if (!brush || !brush->isCreature()) {
		return false;
	}
	
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == brush) {
			SelectIndex(i);
			return true;
		}
	}
	
	return false;
}

void CreatureSpritePanel::EnsureVisible(const Brush* brush) {
	if (!brush || !brush->isCreature()) {
		return;
	}
	
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == brush) {
			// Calculate row of the item
			int row = i / columns;
			int y = row * sprite_size;
			
			int client_height;
			GetClientSize(nullptr, &client_height);
			
			int x_scroll, y_scroll;
			GetViewStart(&x_scroll, &y_scroll);
			
			// Adjust vertical scroll if needed
			if (y < y_scroll) {
				Scroll(-1, y / 10); // / 10 because of scroll rate
			}
			break;
		}
	}
}

int CreatureSpritePanel::GetSpriteSize() const {
	return sprite_size;
}

// Implementation of CreatureSeamlessGridPanel
BEGIN_EVENT_TABLE(CreatureSeamlessGridPanel, wxScrolledWindow)
EVT_PAINT(CreatureSeamlessGridPanel::OnPaint)
EVT_SIZE(CreatureSeamlessGridPanel::OnSize)
EVT_LEFT_DOWN(CreatureSeamlessGridPanel::OnMouseClick)
EVT_MOTION(CreatureSeamlessGridPanel::OnMouseMove)
EVT_SCROLLWIN(CreatureSeamlessGridPanel::OnScroll)
EVT_TIMER(wxID_ANY, CreatureSeamlessGridPanel::OnTimer)
END_EVENT_TABLE()

CreatureSeamlessGridPanel::CreatureSeamlessGridPanel(wxWindow* parent) : 
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxWANTS_CHARS),
	columns(1),
	sprite_size(32),
	selected_index(-1),
	hover_index(-1),
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
	
	// Enable background erase to prevent flicker
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	
	// Set background color
	SetBackgroundColour(wxColour(240, 240, 240));
	
	// Enable scrolling - pixel level for smoother scrolling
	SetScrollRate(1, 20);
	
	// Create loading timer for progressive loading
	loading_timer = new wxTimer(this);
}

CreatureSeamlessGridPanel::~CreatureSeamlessGridPanel() {
	if (loading_timer) {
		loading_timer->Stop();
		delete loading_timer;
	}
	delete buffer;
}

void CreatureSeamlessGridPanel::Clear() {
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	Refresh();
}

void CreatureSeamlessGridPanel::LoadCreatures(const BrushVector& brushlist) {
	// Clear any existing creatures
	creatures.clear();
	selected_index = -1;
	hover_index = -1;
	
	// Copy valid creature brushes
	for (BrushVector::const_iterator iter = brushlist.begin(); iter != brushlist.end(); ++iter) {
		if ((*iter)->isCreature()) {
			creatures.push_back(*iter);
		}
	}
	
	// Select first creature if any
	if (!creatures.empty()) {
		selected_index = 0;
	}
	
	// Store natural dimensions for each creature to use when drawing
	// This prevents constant recalculation
	sprite_dimensions.clear();
	for (size_t i = 0; i < creatures.size(); ++i) {
		CreatureBrush* cb = static_cast<CreatureBrush*>(creatures[i]);
		if (cb && cb->getType()) {
			// Determine natural size based on looktype
			int natural_size = GetCreatureNaturalSize(cb->getType());
			sprite_dimensions[i] = natural_size;
		}
	}
	
	// Calculate layout and refresh
	RecalculateGrid();
	Refresh();
}

void CreatureSeamlessGridPanel::StartProgressiveLoading() {
	if (!loading_timer) return;
	
	// Reset loading step
	loading_step = 0;
	
	// Set initial small margin for quick initial display
	visible_rows_margin = 3;
	
	// Force full redraw
	need_full_redraw = true;
	
	// Start timer for progressive loading
	loading_timer->Start(150); // 150ms interval for smooth loading
	
	// Force initial redraw to show progress
	Refresh();
}

void CreatureSeamlessGridPanel::OnTimer(wxTimerEvent& event) {
	// Progressively increase the loading step
	loading_step++;
	
	// Update viewable items with new margin
	UpdateViewableItems();
	
	// Force redraw to update progress
	Refresh();
	
	// Stop timer when we've reached max loading steps
	if (loading_step >= max_loading_steps) {
		loading_timer->Stop();
		visible_rows_margin = 20; // Set to higher value for regular scrolling
		need_full_redraw = true;
		Refresh();
	}
}

void CreatureSeamlessGridPanel::RecalculateGrid() {
	// Get the client size of the panel
	int panel_width, panel_height;
	GetClientSize(&panel_width, &panel_height);
	
	// Calculate number of columns based on available width
	columns = std::max(1, panel_width / sprite_size);
	
	// Calculate number of rows
	total_rows = creatures.empty() ? 0 : (creatures.size() + columns - 1) / columns;
	
	// Set virtual size for scrolling
	int virtual_height = total_rows * sprite_size;
	SetVirtualSize(panel_width, virtual_height);
	
	// Recreate buffer with new size if needed
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	
	if (panel_width > 0 && panel_height > 0) {
		buffer = new wxBitmap(panel_width, panel_height);
	}
	
	// Update viewable items
	UpdateViewableItems();
}

void CreatureSeamlessGridPanel::UpdateViewableItems() {
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

void CreatureSeamlessGridPanel::OnScroll(wxScrollWinEvent& event) {
	// Handle scroll events to update visible items
	UpdateViewableItems();
	event.Skip();
}

void CreatureSeamlessGridPanel::DrawItemsToPanel(wxDC& dc) {
	if (creatures.empty()) return;
	
	// Get client area size
	int width, height;
	GetClientSize(&width, &height);
	
	// Draw loading progress for large datasets during initial load
	if (loading_step < max_loading_steps && is_large_tileset) {
		// Show loading progress
		wxString loadingMessage = wxString::Format("Loading creatures... %d%%", 
			(loading_step * 100) / max_loading_steps);
		
		// Gray semi-transparent overlay with progress message
		dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
		dc.SetTextForeground(wxColour(50, 50, 50));
		dc.DrawLabel(loadingMessage, wxRect(0, 0, width, height), wxALIGN_CENTER);
	}
	
	// Draw visible sprites in grid
	for (int row = first_visible_row; row <= last_visible_row; ++row) {
		for (int col = 0; col < columns; ++col) {
			int index = row * columns + col;
			if (index < static_cast<int>(creatures.size())) {
				int x = col * sprite_size;
				int y = row * sprite_size;
				
				CreatureBrush* cb = static_cast<CreatureBrush*>(creatures[index]);
				if (cb && cb->getType()) {
					DrawCreature(dc, x, y, cb->getType(), index == selected_index);
				}
			}
		}
	}
}

void CreatureSeamlessGridPanel::OnPaint(wxPaintEvent& event) {
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);  // For correct scrolling
	
	// Clear background
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();
	
	// Draw items
	DrawItemsToPanel(dc);
}

void CreatureSeamlessGridPanel::DrawCreature(wxDC& dc, int x, int y, CreatureType* ctype, bool selected) {
	if (!ctype) return;
	
	// Draw selection highlight
	if (selected) {
		dc.SetBrush(wxBrush(wxColour(0x80, 0x80, 0xFF, 0x80)));
		dc.SetPen(wxPen(wxColour(0x80, 0x80, 0xFF), 1));
		dc.DrawRectangle(x, y, sprite_size, sprite_size);
	}
	
	// For hover effect
	if (!selected && selected_index != -1 && hover_index != -1 && hover_index != selected_index) {
		int hover_col = hover_index % columns;
		int hover_row = hover_index / columns;
		int hover_x = hover_col * sprite_size;
		int hover_y = hover_row * sprite_size;
		
		if (hover_x == x && hover_y == y) {
			dc.SetBrush(wxBrush(wxColour(0xC0, 0xC0, 0xC0, 0x80)));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(x, y, sprite_size, sprite_size);
		}
	}
	
	// Find the natural size of this creature
	int index = -1;
	for (size_t i = 0; i < creatures.size(); ++i) {
		CreatureBrush* cb = static_cast<CreatureBrush*>(creatures[i]);
		if (cb && cb->getType() == ctype) {
			index = i;
			break;
		}
	}
	
	// Get the natural size from the dimensions map if available
	int natural_size = 32;
	if (index >= 0 && sprite_dimensions.count(index) > 0) {
		natural_size = sprite_dimensions[index];
	} else {
		// Calculate natural size if not found in the map
		natural_size = GetCreatureNaturalSize(ctype);
	}
	
	// For zoomed view, determine the display size based on the cell size
	int display_size = natural_size;
	
	// Apply scaling based on cell size
	if (sprite_size < natural_size) {
		// If the cell is smaller than the natural size, scale down
		display_size = sprite_size;
	} else if (sprite_size > natural_size * 2) {
		// If the cell is more than twice the natural size, scale up by a factor
		int zoom_factor = sprite_size / natural_size;
		// Limit to reasonable zoom
		zoom_factor = std::min(zoom_factor, 4);
		display_size = natural_size * zoom_factor;
	}
	
	// Get or create sprite bitmap at the natural size
	wxBitmap* bitmap = nullptr;
	
	if (ctype->outfit.lookType != 0) {
		if (ctype->outfit.lookHead || ctype->outfit.lookBody || ctype->outfit.lookLegs || ctype->outfit.lookFeet) {
			bitmap = g_creature_sprites.getSpriteBitmap(
				ctype->outfit.lookType,
				ctype->outfit.lookHead,
				ctype->outfit.lookBody,
				ctype->outfit.lookLegs,
				ctype->outfit.lookFeet,
				natural_size, natural_size);
		} else {
			bitmap = g_creature_sprites.getSpriteBitmap(ctype->outfit.lookType, natural_size, natural_size);
		}
		
		if (bitmap) {
			// Calculate position to center the sprite in the grid cell
			int offsetX = (sprite_size - display_size) / 2;
			int offsetY = (sprite_size - display_size) / 2;
			
			// Ensure offsets are not negative
			offsetX = std::max(0, offsetX);
			offsetY = std::max(0, offsetY);
			
			// Scale the sprite if needed
			if (display_size != bitmap->GetWidth() || display_size != bitmap->GetHeight()) {
				// Create a temporary scaled bitmap
				wxImage original = bitmap->ConvertToImage();
				wxBitmap scaled(original.Scale(display_size, display_size, wxIMAGE_QUALITY_HIGH));
				
				// Draw the scaled bitmap
				dc.DrawBitmap(scaled, x + offsetX, y + offsetY, true);
			} else {
				// Draw the original bitmap
				dc.DrawBitmap(*bitmap, x + offsetX, y + offsetY, true);
			}
		}
	}
	
	// Draw name label below the sprite
	wxString name = wxString(ctype->name.c_str(), wxConvUTF8);
	if (!name.IsEmpty()) {
		// Set font size based on cell size
		int font_size = std::min(10, sprite_size / 12);
		font_size = std::max(7, font_size); // Make sure it's not too small
		
		wxFont font(font_size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
		dc.SetFont(font);
		dc.SetTextForeground(selected ? wxColour(50, 50, 120) : wxColour(80, 80, 80));
		
		// Position text at the bottom of the cell
		int text_y = y + sprite_size - font_size - 4;
		wxCoord text_width, text_height;
		dc.GetTextExtent(name, &text_width, &text_height);
		
		// Center the text and make sure it fits in the cell
		if (text_width > sprite_size - 4) {
			// Truncate the text
			wxString truncated_name;
			int chars_that_fit = 0;
			wxArrayInt partial_extents;
			dc.GetPartialTextExtents(name, partial_extents);
			
			for (size_t i = 0; i < name.Length(); ++i) {
				if (i < partial_extents.GetCount() && partial_extents[i] < sprite_size - 10) {
					chars_that_fit = i + 1;
				} else {
					break;
				}
			}
			
			if (chars_that_fit > 0) {
				truncated_name = name.Left(chars_that_fit) + "...";
				dc.DrawText(truncated_name, x + (sprite_size - dc.GetTextExtent(truncated_name).GetWidth()) / 2, text_y);
			}
		} else {
			// Text fits, center it
			dc.DrawText(name, x + (sprite_size - text_width) / 2, text_y);
		}
	}
}

void CreatureSeamlessGridPanel::OnSize(wxSizeEvent& event) {
	RecalculateGrid();
	event.Skip();
}

int CreatureSeamlessGridPanel::GetSpriteIndexAt(int x, int y) const {
	// Convert mouse position to logical position (accounting for scrolling)
	int logX, logY;
	CalcUnscrolledPosition(x, y, &logX, &logY);
	
	// Calculate row and column
	int col = logX / sprite_size;
	int row = logY / sprite_size;
	
	// Calculate index
	int index = row * columns + col;
	
	// Check if this is a valid index
	if (index >= 0 && index < static_cast<int>(creatures.size()) && 
		col >= 0 && col < columns) {
		return index;
	}
	
	return -1;
}

void CreatureSeamlessGridPanel::OnMouseClick(wxMouseEvent& event) {
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != -1) {
		selected_index = index;
		Refresh();
	
		// Notify parent of selection
		wxCommandEvent selectionEvent(wxEVT_COMMAND_LISTBOX_SELECTED);
		wxPostEvent(GetParent(), selectionEvent);
	}
	
	event.Skip();
}

void CreatureSeamlessGridPanel::OnMouseMove(wxMouseEvent& event) {
	// Update hover effect if needed
	int index = GetSpriteIndexAt(event.GetX(), event.GetY());
	if (index != hover_index) {
		// Only redraw the cells that changed, not the entire panel
		int old_hover = hover_index;
		hover_index = index;
		
		// If we had a previous hover, just redraw that cell
		if (old_hover >= 0 && old_hover < static_cast<int>(creatures.size())) {
			int old_row = old_hover / columns;
			int old_col = old_hover % columns;
			wxRect oldRect(old_col * sprite_size, old_row * sprite_size, sprite_size, sprite_size);
			RefreshRect(oldRect, false);
		}
		
		// If we have a new hover, just redraw that cell
		if (hover_index >= 0 && hover_index < static_cast<int>(creatures.size())) {
			int new_row = hover_index / columns;
			int new_col = hover_index % columns;
			wxRect newRect(new_col * sprite_size, new_row * sprite_size, sprite_size, sprite_size);
			RefreshRect(newRect, false);
		}
	}
	
	event.Skip();
}

Brush* CreatureSeamlessGridPanel::GetSelectedBrush() const {
	if (selected_index >= 0 && selected_index < static_cast<int>(creatures.size())) {
		return creatures[selected_index];
	}
	return nullptr;
}

bool CreatureSeamlessGridPanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) return false;
	
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == whatbrush) {
			SelectIndex(i);
			return true;
		}
	}
	return false;
}

void CreatureSeamlessGridPanel::SelectIndex(int index) {
	if (index >= 0 && index < static_cast<int>(creatures.size())) {
		// Store the old selection
		int old_selection = selected_index;
		selected_index = index;
		
		// Only redraw if selection changed
		if (old_selection != selected_index) {
			Refresh();
		}
		
		// Ensure the selected item is visible
		EnsureVisible(creatures[index]);
	}
}

void CreatureSeamlessGridPanel::EnsureVisible(const Brush* brush) {
	for (size_t i = 0; i < creatures.size(); ++i) {
		if (creatures[i] == brush) {
			// Calculate row of the item
			int row = i / columns;
			int y = row * sprite_size;
			
			// Get the visible area
			int xStart, yStart;
			GetViewStart(&xStart, &yStart);
			int ppuX, ppuY;
			GetScrollPixelsPerUnit(&ppuX, &ppuY);
			yStart *= ppuY;
			
			int clientHeight;
			GetClientSize(nullptr, &clientHeight);
			
			// Scroll if necessary
			if (y < yStart) {
				Scroll(-1, y / ppuY);
			} else if (y + sprite_size > yStart + clientHeight) {
				Scroll(-1, (y - clientHeight + sprite_size) / ppuY);
			}
			
			// Update which items are visible after scrolling
			UpdateViewableItems();
			break;
		}
	}
}

// Update CreaturePalettePanel constructor
void CreaturePalettePanel::SetViewStyle(bool use_seamless) {
	// Store original selection
	Brush* selected_brush = GetSelectedBrush();
	
	// Update mode flag
	use_seamless_view = use_seamless;
	
	// Update UI elements
	if (use_sprite_view) {
		if (use_seamless_view) {
			// Switch to seamless grid view
			sprite_panel->Hide();
			seamless_panel->Show();
			
			// Load creatures from the current category
			int index = tileset_choice->GetSelection();
			if (index != wxNOT_FOUND) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
				// Pre-generate creature sprites
				int sprite_size = seamless_panel->GetSpriteSize();
				g_creature_sprites.generateCreatureSprites(tsc->brushlist, sprite_size, sprite_size);
				seamless_panel->LoadCreatures(tsc->brushlist);
			}
		} else {
			// Switch to regular grid view
			seamless_panel->Hide();
			sprite_panel->Show();
			
			// Load creatures from the current category
			int index = tileset_choice->GetSelection();
			if (index != wxNOT_FOUND) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
				// Pre-generate creature sprites
				int sprite_size = sprite_panel->GetSpriteSize();
				g_creature_sprites.generateCreatureSprites(tsc->brushlist, sprite_size, sprite_size);
				sprite_panel->LoadCreatures(tsc->brushlist);
			}
		}
		
		// Update layout
		view_sizer->Layout();
	}
	
	// Restore selection
	if (selected_brush) {
		SelectBrush(selected_brush);
	}
}

void CreaturePalettePanel::SetLargeSpriteMode(bool use_large) {
	if (use_large_sprites != use_large) {
		use_large_sprites = use_large;
		large_sprites_toggle->SetValue(use_large);
		
		// Update zoom button state - only enable when in large sprite mode
		zoom_button->Enable(use_large);
		
		// Reset zoom factor when changing sprite size mode
		if (!use_large) {
			zoom_factor = 1;
			zoom_button->SetLabel("Zoom 2x");
		}
		
		// Store the currently selected brush
		Brush* old_brush = GetSelectedBrush();
		
		// Get current tileset
		int index = tileset_choice->GetSelection();
		if (index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
			
			// Determine base sprite size and cell size
			int base_sprite_size = use_large ? 64 : 32;
			int base_cell_size = use_large ? 128 : 32;
			
			// Apply zoom factor to cell size if needed
			int cell_size = base_cell_size;
			if (use_large && zoom_factor > 1) {
				cell_size = base_cell_size * zoom_factor;
			}
			
			// Force regeneration of creature sprites with new size
			g_creature_sprites.clear(); // Clear cache to ensure new sprites are generated
			g_creature_sprites.generateCreatureSprites(tsc->brushlist, base_sprite_size, base_sprite_size);
			
			// Update panel settings
			if (use_seamless_view) {
				// Update seamless panel - use cell_size for grid cell dimensions
				seamless_panel->sprite_size = cell_size;
				seamless_panel->need_full_redraw = true;
				seamless_panel->RecalculateGrid();
				seamless_panel->Refresh();
			} else {
				// Update regular panel - use cell_size for grid cell dimensions
				sprite_panel->sprite_size = cell_size;
				sprite_panel->RecalculateGrid();
				sprite_panel->Refresh();
			}
			
			// Reselect the brush that was selected before
			if (old_brush) {
				SelectBrush(old_brush);
			}
		}
	}
}

void CreaturePalettePanel::SetZoomLevel(int new_zoom_factor) {
	if (zoom_factor != new_zoom_factor) {
		zoom_factor = new_zoom_factor;
		
		// Update button label
		zoom_button->SetLabel(wxString::Format("Zoom %dx", new_zoom_factor));
		
		// Only apply zoom when in large sprite mode
		if (use_large_sprites) {
			// Store the currently selected brush
			Brush* old_brush = GetSelectedBrush();
			
			// Get current tileset
			int index = tileset_choice->GetSelection();
			if (index != wxNOT_FOUND) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
				
				// Base cell size is 128x128 in large mode
				int base_cell_size = 128;
				int cell_size = base_cell_size * zoom_factor;
				
				// Pre-generate creature sprites at their natural sizes
				g_creature_sprites.clear(); // Clear cache to ensure new sprites are generated
				
				// Also reset sprite dimensions cache in the view panel
				if (seamless_panel) {
					seamless_panel->sprite_dimensions.clear();
				}
				
				// Pre-generate sprites at their natural sizes
				for (Brush* brush : tsc->brushlist) {
					if (brush->isCreature()) {
						CreatureBrush* cb = static_cast<CreatureBrush*>(brush);
						if (cb && cb->getType()) {
							CreatureType* type = cb->getType();
							
							// Get natural size
							int natural_size = 32;
							if (seamless_panel) {
								natural_size = seamless_panel->GetCreatureNaturalSize(type);
							}
							
							// Generate sprite at its natural size
							const Outfit& outfit = type->outfit;
							if (outfit.lookHead || outfit.lookBody || outfit.lookLegs || outfit.lookFeet) {
								g_creature_sprites.getSpriteBitmap(outfit.lookType, outfit.lookHead, outfit.lookBody, 
										outfit.lookLegs, outfit.lookFeet, natural_size, natural_size);
							} else {
								g_creature_sprites.getSpriteBitmap(outfit.lookType, natural_size, natural_size);
							}
						}
					}
				}
				
				// Update panel settings
				if (use_seamless_view) {
					// Update seamless panel
					seamless_panel->sprite_size = cell_size;
					seamless_panel->need_full_redraw = true;
					seamless_panel->RecalculateGrid();
					seamless_panel->Refresh();
				} else {
					// Update regular panel
					sprite_panel->sprite_size = cell_size;
					sprite_panel->RecalculateGrid();
					sprite_panel->Refresh();
				}
				
				// Reselect the brush that was selected before
				if (old_brush) {
					SelectBrush(old_brush);
				}
			}
		}
	}
}

void CreaturePalettePanel::OnClickZoomButton(wxCommandEvent& event) {
	// Toggle between zoom levels (1x, 2x, 3x, back to 1x)
	int new_zoom_factor = (zoom_factor % 3) + 1;
	SetZoomLevel(new_zoom_factor);
}

void CreaturePalettePanel::OnClickLargeSpritesToggle(wxCommandEvent& event) {
	SetLargeSpriteMode(event.IsChecked());
}

int CreatureSeamlessGridPanel::GetCreatureNaturalSize(CreatureType* ctype) const {
	if (!ctype) return 32;
	
	// Get sprite from graphics system to check dimensions
	GameSprite* spr = g_gui.gfx.getCreatureSprite(ctype->outfit.lookType);
	if (!spr) return 32;
	
	// Get natural dimensions from sprite
	int natural_width = spr->width > 0 ? spr->width : 32;
	int natural_height = spr->height > 0 ? spr->height : 32;
	
	// Calculate natural size as the maximum of width and height
	int natural_size = std::max(natural_width, natural_height);
	
	// Round up to nearest standard size (32, 64, 96, 128)
	if (natural_size <= 32) {
		natural_size = 32;
	} else if (natural_size <= 64) {
		natural_size = 64;
	} else if (natural_size <= 96) {
		natural_size = 96;
	} else if (natural_size <= 128) {
		natural_size = 128;
	} else {
		natural_size = ((natural_size + 31) / 32) * 32; // Round up to nearest multiple of 32
	}
	
	// Fallback based on looktype for sprites without proper dimensions
	if (natural_size == 32 && ctype->outfit.lookType >= 800) {
		natural_size = 64; // Many higher looktype monsters are larger
	}
	
	// Fallback for very high looktypes
	if (ctype->outfit.lookType >= 1200 && natural_size < 96) {
		natural_size = 96; // Some newer monsters can be much larger
	}
	
	return natural_size;
}
