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
#include "recent_brushes_window.h"
#include "palette_common.h"
#include "gui.h"
#include "brush.h"
#include "raw_brush.h"
#include "dcbutton.h"
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/menu.h>
#include <wx/choice.h>
#include <wx/textdlg.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <pugixml.hpp>
#include <algorithm>

// Event IDs
enum {
	RECENT_BRUSH_ID = 20000,
	PRESET_CHOICE_ID,
	SAVE_PRESET_ID,
	LOAD_PRESET_ID,
	DELETE_PRESET_ID,
	DELETE_BRUSH_ID
};

// Event table for RecentBrushesPanel
BEGIN_EVENT_TABLE(RecentBrushesPanel, wxScrolledWindow)
	EVT_SIZE(RecentBrushesPanel::OnSize)
	EVT_PAINT(RecentBrushesPanel::OnPaint)
	EVT_ERASE_BACKGROUND(RecentBrushesPanel::OnEraseBackground)
	EVT_RIGHT_DOWN(RecentBrushesPanel::OnRightClick)
	EVT_MENU(DELETE_BRUSH_ID, RecentBrushesPanel::OnDeleteBrush)
END_EVENT_TABLE()

// Event table for RecentBrushesWindow
BEGIN_EVENT_TABLE(RecentBrushesWindow, wxPanel)
	EVT_KEY_DOWN(RecentBrushesWindow::OnKey)
	EVT_CLOSE(RecentBrushesWindow::OnClose)
	EVT_BUTTON(wxID_CLEAR, RecentBrushesWindow::OnClearButton)
	EVT_CHOICE(PRESET_CHOICE_ID, RecentBrushesWindow::OnPresetChoice)
	EVT_BUTTON(SAVE_PRESET_ID, RecentBrushesWindow::OnSavePreset)
	EVT_BUTTON(LOAD_PRESET_ID, RecentBrushesWindow::OnLoadPreset)
	EVT_BUTTON(DELETE_PRESET_ID, RecentBrushesWindow::OnDeletePreset)
	EVT_PAINT(RecentBrushesWindow::OnPaint)
	EVT_ERASE_BACKGROUND(RecentBrushesWindow::OnEraseBackground)
END_EVENT_TABLE()

//=============================================================================
// RecentBrushesPanel Implementation
//=============================================================================

RecentBrushesPanel::RecentBrushesPanel(wxWindow* parent) 
	: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL),
	  columns(1), brush_to_delete(nullptr) {
	
	// Set background style and color properly
	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
	SetBackgroundColour(*wxWHITE);
	
	// Create main sizer
	main_sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(main_sizer);
	
	// Ensure immediate white background
	ClearBackground();
}

RecentBrushesPanel::~RecentBrushesPanel() {
	// Clear all brush buttons
	for (BrushButton* button : brush_buttons) {
		button->Destroy();
	}
	brush_buttons.clear();
}

void RecentBrushesPanel::AddRecentBrush(Brush* brush, PaletteType palette_type, bool manual_add) {
	if (!brush) {
		return;
	}
	
	// Only add manually selected brushes to avoid automatic palette switching additions
	if (!manual_add) {
		return;
	}
	
	// Don't add system brushes that shouldn't be in recent list
	if (brush == g_gui.eraser) {
		return;
	}
	
	// Remove existing entry if it exists (to avoid duplicates)
	auto it = std::find_if(recent_brushes.begin(), recent_brushes.end(),
		[brush](const RecentBrushEntry& entry) { return entry.brush == brush; });
	
	if (it != recent_brushes.end()) {
		// Don't reorder existing brushes - this prevents crashes from position changes
		return;
	}
	
	// Add new brush to the front
	recent_brushes.push_front(RecentBrushEntry(brush, palette_type));
	
	// Limit the number of recent brushes
	while (recent_brushes.size() > MAX_RECENT_BRUSHES) {
		recent_brushes.pop_back();
	}
	
	RefreshDisplay();
}

void RecentBrushesPanel::ClearRecentBrushes() {
	recent_brushes.clear();
	RefreshDisplay();
}

void RecentBrushesPanel::RemoveBrush(Brush* brush) {
	if (!brush) {
		return;
	}
	
	auto it = std::find_if(recent_brushes.begin(), recent_brushes.end(),
		[brush](const RecentBrushEntry& entry) { return entry.brush == brush; });
	
	if (it != recent_brushes.end()) {
		recent_brushes.erase(it);
		RefreshDisplay();
	}
}

void RecentBrushesPanel::RefreshDisplay() {
	// Clear existing UI elements
	for (BrushButton* button : brush_buttons) {
		main_sizer->Detach(button);
		button->Destroy();
	}
	brush_buttons.clear();
	
	for (wxStaticText* label : section_labels) {
		main_sizer->Detach(label);
		label->Destroy();
	}
	section_labels.clear();
	
	// Clear all remaining items from the sizer (including lines and spacers)
	main_sizer->Clear(true); // true = delete windows
	
	if (recent_brushes.empty()) {
		Layout();
		return;
	}
	
	// Group brushes by palette type while maintaining order
	std::map<PaletteType, std::vector<const RecentBrushEntry*>> grouped_brushes;
	std::vector<PaletteType> type_order; // To maintain insertion order
	
	for (const auto& entry : recent_brushes) {
		if (grouped_brushes.find(entry.palette_type) == grouped_brushes.end()) {
			type_order.push_back(entry.palette_type);
		}
		grouped_brushes[entry.palette_type].push_back(&entry);
	}
	
	// Create sections with spacers
	for (PaletteType type : type_order) {
		const auto& brushes = grouped_brushes[type];
		
		// Add section label
		wxString label_text = GetPaletteTypeName(type);
		wxStaticText* section_label = new wxStaticText(this, wxID_ANY, label_text);
		
		// Style the label to look like a separator
		wxFont font = section_label->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		font.SetPointSize(font.GetPointSize() - 1);
		section_label->SetFont(font);
		section_label->SetForegroundColour(wxColour(100, 100, 100));
		
		main_sizer->Add(section_label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
		section_labels.push_back(section_label);
		
		// Add horizontal line
		wxPanel* line = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
		line->SetBackgroundColour(wxColour(200, 200, 200));
		main_sizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
		
		// Create grid sizer for this section's brushes
		int brushes_per_row = std::max(1, GetSize().GetWidth() / 38); // 32px + 6px margin
		wxFlexGridSizer* grid_sizer = new wxFlexGridSizer(0, brushes_per_row, 2, 2);
		
		// Add brushes for this palette type
		for (const RecentBrushEntry* entry : brushes) {
			BrushButton* button = new BrushButton(this, entry->brush, RENDER_SIZE_32x32);
			
			// Set tooltip with brush name and palette type
			wxString tooltip;
			if (entry->brush->isRaw()) {
				RAWBrush* raw = static_cast<RAWBrush*>(entry->brush);
				tooltip = wxString::Format("%s [%d] (%s)", raw->getName(), raw->getItemID(), label_text);
			} else {
				tooltip = wxString::Format("%s (%s)", entry->brush->getName(), label_text);
			}
			button->SetToolTip(tooltip);
			
			// Bind click event
			button->Bind(wxEVT_TOGGLEBUTTON, &RecentBrushesPanel::OnBrushClick, this);
			
			grid_sizer->Add(button, 0, wxALL, 1);
			brush_buttons.push_back(button);
		}
		
		main_sizer->Add(grid_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);
		
		// Add spacing between sections
		if (type != type_order.back()) {
			main_sizer->AddSpacer(5);
		}
	}
	
	Layout();
}

Brush* RecentBrushesPanel::GetSelectedBrush() const {
	for (BrushButton* button : brush_buttons) {
		if (button->GetValue()) {
			return button->brush;
		}
	}
	return nullptr;
}

bool RecentBrushesPanel::SelectBrush(const Brush* whatbrush) {
	DeselectAll();
	for (BrushButton* button : brush_buttons) {
		if (button->brush == whatbrush) {
			button->SetValue(true);
			return true;
		}
	}
	return false;
}

void RecentBrushesPanel::DeselectAll() {
	for (BrushButton* button : brush_buttons) {
		button->SetValue(false);
	}
}

void RecentBrushesPanel::SetRecentBrushes(const std::deque<RecentBrushEntry>& brushes) {
	recent_brushes = brushes;
	RefreshDisplay();
}

void RecentBrushesPanel::OnBrushClick(wxCommandEvent& event) {
	BrushButton* clicked_button = static_cast<BrushButton*>(event.GetEventObject());
	if (!clicked_button) {
		return;
	}
	
	// Deselect all other buttons
	for (BrushButton* button : brush_buttons) {
		if (button != clicked_button) {
			button->SetValue(false);
		}
	}
	
	// Select the clicked brush in the GUI
	if (clicked_button->GetValue()) {
		g_gui.SelectBrush(clicked_button->brush);
	}
}

void RecentBrushesPanel::OnRightClick(wxMouseEvent& event) {
	// Find which brush button was right-clicked
	wxPoint pos = event.GetPosition();
	Brush* target_brush = nullptr;
	
	for (BrushButton* button : brush_buttons) {
		wxRect button_rect = button->GetRect();
		if (button_rect.Contains(pos)) {
			target_brush = button->brush;
			break;
		}
	}
	
	if (target_brush) {
		wxMenu menu;
		wxString brush_name = target_brush->getName();
		if (target_brush->isRaw()) {
			RAWBrush* raw = static_cast<RAWBrush*>(target_brush);
			brush_name = wxString::Format("%s [%d]", raw->getName(), raw->getItemID());
		}
		
		menu.Append(DELETE_BRUSH_ID, "Delete " + brush_name);
		
		// Store the brush to delete in our member variable
		brush_to_delete = target_brush;
		PopupMenu(&menu);
	}
}

void RecentBrushesPanel::OnDeleteBrush(wxCommandEvent& event) {
	// Use the brush stored in our member variable
	if (brush_to_delete) {
		RemoveBrush(brush_to_delete);
		brush_to_delete = nullptr; // Clear the reference
	}
}

void RecentBrushesPanel::OnSize(wxSizeEvent& event) {
	RefreshDisplay();
	event.Skip();
}

void RecentBrushesPanel::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	
	// Paint white background
	dc.SetBackground(wxBrush(*wxWHITE));
	dc.Clear();
	
	event.Skip();
}

void RecentBrushesPanel::OnEraseBackground(wxEraseEvent& event) {
	wxClientDC dc(this);
	dc.SetBackground(wxBrush(*wxWHITE));
	dc.Clear();
}

void RecentBrushesPanel::RecalculateLayout() {
	// This method is kept for compatibility but the new RefreshDisplay handles layout
	RefreshDisplay();
}

void RecentBrushesPanel::UpdateButtonLayout() {
	// This method is kept for compatibility but the new RefreshDisplay handles layout
	RefreshDisplay();
}

wxString RecentBrushesPanel::GetPaletteTypeName(PaletteType type) const {
	switch (type) {
		case TILESET_TERRAIN:
			return "Terrain";
		case TILESET_DOODAD:
			return "Doodads";
		case TILESET_ITEM:
			return "Items";
		case TILESET_RAW:
			return "RAW";
		case TILESET_CREATURE:
			return "Creatures";
		case TILESET_HOUSE:
			return "Houses";
		case TILESET_WAYPOINT:
			return "Waypoints";
		default:
			return "Other";
	}
}

//=============================================================================
// RecentBrushesWindow Implementation
//=============================================================================

RecentBrushesWindow::RecentBrushesWindow(wxWindow* parent) 
	: wxPanel(parent, wxID_ANY),
	  recent_panel(nullptr),
	  clear_button(nullptr),
	  preset_choice(nullptr),
	  save_button(nullptr),
	  load_button(nullptr),
	  delete_button(nullptr) {
	
	// Set white background for the entire window
	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
	SetBackgroundColour(*wxWHITE);
	
	// Create main sizer
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
	
	// Create title
	wxStaticText* title = new wxStaticText(this, wxID_ANY, "Recent Brushes");
	wxFont font = title->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(font);
	title->SetBackgroundColour(*wxWHITE);
	main_sizer->Add(title, 0, wxALL | wxALIGN_CENTER, 5);
	
	// Create recent brushes panel
	recent_panel = new RecentBrushesPanel(this);
	main_sizer->Add(recent_panel, 1, wxEXPAND | wxALL, 2);
	
	// Create clear button
	clear_button = new wxButton(this, wxID_CLEAR, "Clear All");
	main_sizer->Add(clear_button, 0, wxALL | wxALIGN_CENTER, 2);
	
	// Add preset controls
	wxBoxSizer* preset_sizer = new wxBoxSizer(wxVERTICAL);
	
	wxStaticText* preset_label = new wxStaticText(this, wxID_ANY, "Presets:");
	wxFont preset_font = preset_label->GetFont();
	preset_font.SetWeight(wxFONTWEIGHT_BOLD);
	preset_label->SetFont(preset_font);
	preset_label->SetBackgroundColour(*wxWHITE);
	preset_sizer->Add(preset_label, 0, wxALL | wxALIGN_LEFT, 2);
	
	preset_choice = new wxChoice(this, PRESET_CHOICE_ID);
	preset_sizer->Add(preset_choice, 0, wxALL | wxEXPAND, 2);
	
	wxBoxSizer* preset_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	load_button = new wxButton(this, LOAD_PRESET_ID, "Load", wxDefaultPosition, wxSize(50, -1));
	preset_buttons_sizer->Add(load_button, 0, wxALL, 1);
	
	save_button = new wxButton(this, SAVE_PRESET_ID, "Save", wxDefaultPosition, wxSize(50, -1));
	preset_buttons_sizer->Add(save_button, 0, wxALL, 1);
	
	delete_button = new wxButton(this, DELETE_PRESET_ID, "Del", wxDefaultPosition, wxSize(50, -1));
	preset_buttons_sizer->Add(delete_button, 0, wxALL, 1);
	
	preset_sizer->Add(preset_buttons_sizer, 0, wxALL | wxALIGN_CENTER, 2);
	main_sizer->Add(preset_sizer, 0, wxALL | wxEXPAND, 2);
	
	SetSizer(main_sizer);
	
	// Set minimum size
	SetMinSize(wxSize(120, 280));
	
	// Force initial refresh to apply background
	Refresh();
	Update();
	
	// Load initial preset data
	RefreshPresetList();
	
	// Bind preset events
	preset_choice->Bind(wxEVT_CHOICE, &RecentBrushesWindow::OnPresetChoice, this);
	load_button->Bind(wxEVT_BUTTON, &RecentBrushesWindow::OnLoadPreset, this);
	save_button->Bind(wxEVT_BUTTON, &RecentBrushesWindow::OnSavePreset, this);
	delete_button->Bind(wxEVT_BUTTON, &RecentBrushesWindow::OnDeletePreset, this);
}

RecentBrushesWindow::~RecentBrushesWindow() {
	// Cleanup handled by wxWidgets
}

void RecentBrushesWindow::AddRecentBrush(Brush* brush, PaletteType palette_type, bool manual_add) {
	if (recent_panel) {
		recent_panel->AddRecentBrush(brush, palette_type, manual_add);
	}
}

void RecentBrushesWindow::ClearRecentBrushes() {
	if (recent_panel) {
		recent_panel->ClearRecentBrushes();
	}
}

void RecentBrushesWindow::RemoveBrush(Brush* brush) {
	if (recent_panel) {
		recent_panel->RemoveBrush(brush);
	}
}

void RecentBrushesWindow::RefreshDisplay() {
	if (recent_panel) {
		recent_panel->RefreshDisplay();
	}
}

Brush* RecentBrushesWindow::GetSelectedBrush() const {
	if (recent_panel) {
		return recent_panel->GetSelectedBrush();
	}
	return nullptr;
}

bool RecentBrushesWindow::SelectBrush(const Brush* whatbrush) {
	if (recent_panel) {
		return recent_panel->SelectBrush(whatbrush);
	}
	return false;
}

void RecentBrushesWindow::OnKey(wxKeyEvent& event) {
	// Forward key events to the main map window
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

void RecentBrushesWindow::OnClose(wxCloseEvent& event) {
	if (!event.CanVeto()) {
		// We can't do anything! This sucks!
		// (application is closed, we have to destroy ourselves)
		Destroy();
	} else {
		Show(false);
		event.Veto(true);
	}
}

void RecentBrushesWindow::OnClearButton(wxCommandEvent& event) {
	ClearRecentBrushes();
}

void RecentBrushesWindow::OnPaint(wxPaintEvent& event) {
	wxPaintDC dc(this);
	
	// Fill background with white
	dc.SetBackground(wxBrush(*wxWHITE));
	dc.Clear();
	
	event.Skip();
}

void RecentBrushesWindow::OnEraseBackground(wxEraseEvent& event) {
	wxDC* dc = event.GetDC();
	if (dc) {
		dc->SetBackground(wxBrush(*wxWHITE));
		dc->Clear();
	}
}

//=============================================================================
// Preset Management Implementation
//=============================================================================

void RecentBrushesWindow::RefreshPresetList() {
	if (!preset_choice) return;
	
	preset_choice->Clear();
	preset_choice->Append("-- Select Preset --");
	
	wxString presets_dir = g_gui.GetDataDirectory() + "recent_brushes_presets\\";
	if (!wxDir::Exists(presets_dir)) {
		wxMkdir(presets_dir, wxS_DIR_DEFAULT);
		preset_choice->SetSelection(0);
		return;
	}
	
	wxDir dir(presets_dir);
	if (!dir.IsOpened()) {
		preset_choice->SetSelection(0);
		return;
	}
	
	wxString filename;
	bool found = dir.GetFirst(&filename, "*.xml", wxDIR_FILES);
	while (found) {
		wxString name = filename.BeforeLast('.');
		preset_choice->Append(name);
		found = dir.GetNext(&filename);
	}
	
	preset_choice->SetSelection(0);
}

void RecentBrushesWindow::SavePresetToXML(const wxString& name) {
	if (!recent_panel) return;
	
	wxString presets_dir = g_gui.GetDataDirectory() + "recent_brushes_presets\\";
	if (!wxDir::Exists(presets_dir)) {
		wxMkdir(presets_dir, wxS_DIR_DEFAULT);
	}
	
	wxString path = presets_dir + name + ".xml";
	
	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("recent_brushes");
	
	const auto& brushes = recent_panel->GetRecentBrushes();
	for (const auto& entry : brushes) {
		pugi::xml_node brush_node = root.append_child("brush");
		
		if (entry.brush->isRaw()) {
			RAWBrush* raw = static_cast<RAWBrush*>(entry.brush);
			brush_node.append_attribute("type").set_value("raw");
			brush_node.append_attribute("name").set_value(raw->getName().c_str());
			brush_node.append_attribute("item_id").set_value(raw->getItemID());
		} else {
			// Determine brush type for identification
			wxString brushType = "other";
			if (entry.brush->isGround()) brushType = "ground";
			else if (entry.brush->isWall()) brushType = "wall";
			else if (entry.brush->isDoodad()) brushType = "doodad";
			
			brush_node.append_attribute("type").set_value(brushType.c_str());
			brush_node.append_attribute("name").set_value(entry.brush->getName().c_str());
		}
		
		// Store palette type
		brush_node.append_attribute("palette_type").set_value(static_cast<int>(entry.palette_type));
	}
	
	doc.save_file(path.mb_str());
}

void RecentBrushesWindow::LoadPresetFromXML(const wxString& name) {
	if (!recent_panel) return;
	
	wxString path = g_gui.GetDataDirectory() + "recent_brushes_presets\\" + name + ".xml";
	
	pugi::xml_document doc;
	if (!doc.load_file(path.mb_str())) {
		return;
	}

	// Create new brushes list
	std::deque<RecentBrushEntry> newBrushes;
	
	pugi::xml_node root = doc.child("recent_brushes");
	for (pugi::xml_node brush_node = root.child("brush"); brush_node; brush_node = brush_node.next_sibling("brush")) {
		wxString brushType = wxString(brush_node.attribute("type").value());
		wxString brushName = wxString(brush_node.attribute("name").value());
		PaletteType paletteType = static_cast<PaletteType>(brush_node.attribute("palette_type").as_int(TILESET_UNKNOWN));
		
		Brush* foundBrush = nullptr;
		
		if (brushType == "raw") {
			uint16_t itemId = brush_node.attribute("item_id").as_uint();
			// Find RAW brush by item ID
			for (const auto& brushPair : g_brushes.getMap()) {
				if (brushPair.second && brushPair.second->isRaw()) {
					RAWBrush* raw = static_cast<RAWBrush*>(brushPair.second);
					if (raw->getItemID() == itemId) {
						foundBrush = raw;
						break;
					}
				}
			}
		} else {
			// First try to find brush directly by name using the brush manager
			foundBrush = g_brushes.getBrush(brushName.ToStdString());
			
			// If found, verify the type matches
			if (foundBrush) {
				bool typeMatches = false;
				if (brushType == "ground" && foundBrush->isGround()) typeMatches = true;
				else if (brushType == "wall" && foundBrush->isWall()) typeMatches = true;
				else if (brushType == "doodad" && foundBrush->isDoodad()) typeMatches = true;
				else if (brushType == "other") typeMatches = true;
				
				if (!typeMatches) {
					foundBrush = nullptr; // Type doesn't match, clear the brush
				}
			}
			
			// If still not found, search through all brushes manually
			if (!foundBrush) {
				for (const auto& brushPair : g_brushes.getMap()) {
					if (brushPair.second && brushPair.second->getName() == brushName.ToStdString()) {
						Brush* brush = brushPair.second;
						bool typeMatches = false;
						
						if (brushType == "ground" && brush->isGround()) typeMatches = true;
						else if (brushType == "wall" && brush->isWall()) typeMatches = true;
						else if (brushType == "doodad" && brush->isDoodad()) typeMatches = true;
						else if (brushType == "other") typeMatches = true;
						
						if (typeMatches) {
							foundBrush = brush;
							break;
						}
					}
				}
			}
		}
		
		if (foundBrush) {
			newBrushes.push_back(RecentBrushEntry(foundBrush, paletteType));
		}
	}
	
	// Limit to max brushes
	while (newBrushes.size() > recent_panel->GetMaxRecentBrushes()) {
		newBrushes.pop_back();
	}
	
	// Set the new brushes list
	recent_panel->SetRecentBrushes(newBrushes);
}

void RecentBrushesWindow::OnPresetChoice(wxCommandEvent& event) {
	// Do nothing on selection change - user must click Load button
}

void RecentBrushesWindow::OnSavePreset(wxCommandEvent& event) {
	if (!recent_panel) return;
	
	wxString name = wxGetTextFromUser("Enter preset name:", "Save Preset");
	if (name.IsEmpty()) return;
	
	SavePresetToXML(name);
	RefreshPresetList();
	
	// Select the newly saved preset
	int index = preset_choice->FindString(name);
	if (index != wxNOT_FOUND) {
		preset_choice->SetSelection(index);
	}
}

void RecentBrushesWindow::OnLoadPreset(wxCommandEvent& event) {
	if (!recent_panel || !preset_choice) return;
	
	int selection = preset_choice->GetSelection();
	if (selection <= 0) return; // "-- Select Preset --" is at index 0
	
	wxString name = preset_choice->GetString(selection);
	LoadPresetFromXML(name);
}

void RecentBrushesWindow::OnDeletePreset(wxCommandEvent& event) {
	if (!preset_choice) return;
	
	int selection = preset_choice->GetSelection();
	if (selection <= 0) return;
	
	wxString name = preset_choice->GetString(selection);
	
	int result = wxMessageBox(
		"Are you sure you want to delete preset '" + name + "'?",
		"Delete Preset",
		wxYES_NO | wxICON_QUESTION
	);
	
	if (result == wxYES) {
		wxString path = g_gui.GetDataDirectory() + "recent_brushes_presets\\" + name + ".xml";
		if (wxFileExists(path)) {
			wxRemoveFile(path);
		}
		RefreshPresetList();
	}
} 