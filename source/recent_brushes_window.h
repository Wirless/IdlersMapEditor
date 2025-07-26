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

#ifndef RME_RECENT_BRUSHES_WINDOW_H_
#define RME_RECENT_BRUSHES_WINDOW_H_

#include "main.h"
#include "palette_common.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <vector>
#include <deque>
#include <map>

class Brush;
class BrushButton;

// Structure to hold brush with its palette type
struct RecentBrushEntry {
	Brush* brush;
	PaletteType palette_type;
	
	RecentBrushEntry(Brush* b, PaletteType pt) : brush(b), palette_type(pt) {}
};

class RecentBrushesPanel : public wxScrolledWindow {
public:
	RecentBrushesPanel(wxWindow* parent);
	~RecentBrushesPanel();

	// Interface
	void AddRecentBrush(Brush* brush, PaletteType palette_type, bool manual_add = true);
	void ClearRecentBrushes();
	void RefreshDisplay();
	void RemoveBrush(Brush* brush);
	
	// Get the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select a specific brush
	bool SelectBrush(const Brush* whatbrush);
	// Deselect all brushes
	void DeselectAll();
	
	// Access methods for preset functionality
	const std::deque<RecentBrushEntry>& GetRecentBrushes() const { return recent_brushes; }
	void SetRecentBrushes(const std::deque<RecentBrushEntry>& brushes);
	size_t GetMaxRecentBrushes() const { return MAX_RECENT_BRUSHES; }

	// Event handlers
	void OnBrushClick(wxCommandEvent& event);
	void OnRightClick(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	void OnDeleteBrush(wxCommandEvent& event);

private:
	void RecalculateLayout();
	void UpdateButtonLayout();
	wxString GetPaletteTypeName(PaletteType type) const;
	
	std::deque<RecentBrushEntry> recent_brushes;
	std::vector<BrushButton*> brush_buttons;
	std::vector<wxStaticText*> section_labels;
	wxBoxSizer* main_sizer;
	int columns;
	Brush* brush_to_delete; // Temporary storage for context menu operations
	static const size_t MAX_RECENT_BRUSHES = 20;

	DECLARE_EVENT_TABLE()
};

class RecentBrushesWindow : public wxPanel {
public:
	RecentBrushesWindow(wxWindow* parent);
	~RecentBrushesWindow();

	// Interface
	void AddRecentBrush(Brush* brush, PaletteType palette_type = TILESET_UNKNOWN, bool manual_add = true);
	void ClearRecentBrushes();
	void RefreshDisplay();
	void RemoveBrush(Brush* brush);
	
	// Get the currently selected brush
	Brush* GetSelectedBrush() const;
	// Select a specific brush
	bool SelectBrush(const Brush* whatbrush);

	// Event handlers
	void OnKey(wxKeyEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnClearButton(wxCommandEvent& event);
	void OnPresetChoice(wxCommandEvent& event);
	void OnSavePreset(wxCommandEvent& event);
	void OnLoadPreset(wxCommandEvent& event);
	void OnDeletePreset(wxCommandEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

private:
	void RefreshPresetList();
	void SavePresetToXML(const wxString& name);
	void LoadPresetFromXML(const wxString& name);
	
	RecentBrushesPanel* recent_panel;
	
	// UI controls
	wxButton* clear_button;
	
	// Preset controls
	wxChoice* preset_choice;
	wxButton* save_button;
	wxButton* load_button;
	wxButton* delete_button;

	DECLARE_EVENT_TABLE()
};

#endif 