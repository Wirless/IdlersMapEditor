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

#include "result_window.h"
#include "gui.h"
#include "position.h"
#include "main_menubar.h"
#include <vector>
#include <algorithm>
#include "string_utils.h"

BEGIN_EVENT_TABLE(SearchResultWindow, wxPanel)
EVT_LISTBOX(wxID_ANY, SearchResultWindow::OnClickResult)
EVT_BUTTON(wxID_FILE, SearchResultWindow::OnClickExport)
EVT_BUTTON(wxID_CLEAR, SearchResultWindow::OnClickClear)
EVT_BUTTON(SEARCH_RESULT_NEXT_BUTTON, SearchResultWindow::OnClickNext)
EVT_BUTTON(SEARCH_RESULT_SORT_NUMERICAL, SearchResultWindow::OnClickSortNumerical)
EVT_BUTTON(SEARCH_RESULT_SORT_ALPHABETICAL, SearchResultWindow::OnClickSortAlphabetical)
EVT_BUTTON(SEARCH_RESULT_SORT_POSITION, SearchResultWindow::OnClickSortPosition)
END_EVENT_TABLE()

SearchResultWindow::SearchResultWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	use_ignored_ids(false),
	last_search_itemid(0),
	last_search_on_selection(false),
	has_last_search(false),
	last_ignored_ids_enabled(false) {
	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	result_list = newd wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(200, 330), 0, nullptr, wxLB_SINGLE | wxLB_ALWAYS_SB);
	sizer->Add(result_list, wxSizerFlags(1).Expand());

	// Create a static box for sorting controls
	wxStaticBoxSizer* sortBoxSizer = newd wxStaticBoxSizer(wxHORIZONTAL, this, "Sort By");
	sortBoxSizer->Add(newd wxButton(this, SEARCH_RESULT_SORT_NUMERICAL, "ID"), wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	sortBoxSizer->Add(newd wxButton(this, SEARCH_RESULT_SORT_ALPHABETICAL, "Name"), wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	sortBoxSizer->Add(newd wxButton(this, SEARCH_RESULT_SORT_POSITION, "Position"), wxSizerFlags(0).Center());
	sizer->Add(sortBoxSizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT | wxTOP, 5));

	// Create action buttons sizer
	wxSizer* actionsSizer = newd wxBoxSizer(wxHORIZONTAL);
	actionsSizer->Add(newd wxButton(this, SEARCH_RESULT_NEXT_BUTTON, "Next"), wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	actionsSizer->Add(newd wxButton(this, wxID_FILE, "Export"), wxSizerFlags(0).Center().Border(wxRIGHT, 5));
	actionsSizer->Add(newd wxButton(this, wxID_CLEAR, "Clear"), wxSizerFlags(0).Center());
	sizer->Add(actionsSizer, wxSizerFlags(0).Center().DoubleBorder());
	SetSizerAndFit(sizer);
}

SearchResultWindow::~SearchResultWindow() {
	Clear();
}

void SearchResultWindow::Clear() {
	for (uint32_t n = 0; n < result_list->GetCount(); ++n) {
		delete reinterpret_cast<Position*>(result_list->GetClientData(n));
	}
	result_list->Clear();
	has_last_search = false;
}

void SearchResultWindow::AddPosition(wxString description, Position pos) {
	uint16_t item_id = 0;
	wxString item_name;
	
	// Extract the item ID from the description
	size_t start = description.Find("(ID: ");
	if (start != wxString::npos) {
		start += 4;
		size_t end = description.find(')', start);
		if (end != wxString::npos) {
			wxString id_str = description.SubString(start, end - 1);
			long temp_id;
			if (id_str.ToLong(&temp_id)) {
				item_id = static_cast<uint16_t>(temp_id);
				item_name = description.SubString(0, start - 5).Trim(); // Get name without ID
				OutputDebugStringA(wxString::Format("Parsed item: %s with ID: %d\n", 
					item_name, item_id).c_str());
				
				if (use_ignored_ids) {
					if (std::find(ignored_ids.begin(), ignored_ids.end(), item_id) != ignored_ids.end()) {
						OutputDebugStringA(wxString::Format("Skipping ignored ID: %d\n", item_id).c_str());
						return;
					}
					
					for (const auto& range : ignored_ranges) {
						if (item_id >= range.first && item_id <= range.second) {
							OutputDebugStringA(wxString::Format("Skipping ID in ignored range %d-%d: %d\n", 
								range.first, range.second, item_id).c_str());
							return;
						}
					}
				}
			}
		}
	}
	
	// If we couldn't parse an ID, try to get the item name directly
	if (item_name.empty()) {
		size_t space_pos = description.Find(' ');
		if (space_pos != wxString::npos) {
			item_name = description.SubString(0, space_pos - 1);
		} else {
			item_name = description;
		}
		OutputDebugStringA(wxString::Format("Using item name without ID: %s\n", item_name).c_str());
	}
	
	// Format the display text
	wxString display_text;
	if (item_id > 0) {
		display_text = wxString::Format("%s [ID: %d] at (%d,%d,%d)", 
			item_name, item_id, pos.x, pos.y, pos.z);
	} else {
		display_text = wxString::Format("%s at (%d,%d,%d)", 
			item_name, pos.x, pos.y, pos.z);
	}
	
	result_list->Append(display_text, newd Position(pos));
	OutputDebugStringA(wxString::Format("Added to result list: %s\n", display_text).c_str());
}

std::vector<Position> SearchResultWindow::GetFoundPositions() const {
	std::vector<Position> positions;
	for (uint32_t n = 0; n < result_list->GetCount(); ++n) {
		Position* pos = reinterpret_cast<Position*>(result_list->GetClientData(n));
		if (pos) {
			positions.push_back(*pos);
		}
	}
	return positions;
}

void SearchResultWindow::StoreSearchInfo(uint16_t itemId, bool onSelection) {
	last_search_itemid = itemId;
	last_search_on_selection = onSelection;
	has_last_search = true;
}

void SearchResultWindow::OnClickResult(wxCommandEvent& event) {
	Position* pos = reinterpret_cast<Position*>(event.GetClientData());
	if (pos) {
		g_gui.SetScreenCenterPosition(*pos);
	}
}

void SearchResultWindow::OnClickExport(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dialog(this, "Save file...", "", "", "Text Documents (*.txt) | *.txt", wxFD_SAVE);
	if (dialog.ShowModal() == wxID_OK) {
		wxFile file(dialog.GetPath(), wxFile::write);
		if (file.IsOpened()) {
			g_gui.CreateLoadBar("Exporting search result...");

			file.Write("Generated by Remere's Map Editor " + __RME_VERSION__);
			file.Write("\n=============================================\n\n");
			wxArrayString lines = result_list->GetStrings();
			size_t count = lines.Count();
			for (size_t i = 0; i < count; ++i) {
				file.Write(lines[i] + "\n");
				g_gui.SetLoadScale((int32_t)i, (int32_t)count);
			}
			file.Close();

			g_gui.DestroyLoadBar();
		}
	}
}

void SearchResultWindow::OnClickClear(wxCommandEvent& WXUNUSED(event)) {
	Clear();
}

void SearchResultWindow::OnClickNext(wxCommandEvent& WXUNUSED(event)) {
	ContinueSearch();
}

void SearchResultWindow::OnClickSortNumerical(wxCommandEvent& WXUNUSED(event)) {
	SortResultsByID();
}

void SearchResultWindow::OnClickSortAlphabetical(wxCommandEvent& WXUNUSED(event)) {
	SortResultsByName();
}

void SearchResultWindow::OnClickSortPosition(wxCommandEvent& WXUNUSED(event)) {
	SortResultsByPosition();
}

void SearchResultWindow::SortResultsByID() {
	if (result_list->GetCount() == 0) return;
	
	// Create vector of items with their data
	std::vector<std::pair<wxString, Position*>> items;
	for (uint32_t i = 0; i < result_list->GetCount(); ++i) {
		wxString text = result_list->GetString(i);
		Position* pos = reinterpret_cast<Position*>(result_list->GetClientData(i));
		items.push_back(std::make_pair(text, pos));
	}
	
	// Sort by ID (extract ID from string)
	std::sort(items.begin(), items.end(), [](const std::pair<wxString, Position*>& a, const std::pair<wxString, Position*>& b) {
		// Extract ID from "[ID: xxx]" pattern
		int idA = 0, idB = 0;
		size_t startA = a.first.Find("[ID: ");
		size_t startB = b.first.Find("[ID: ");
		
		if (startA != wxString::npos) {
			startA += 5; // Skip "[ID: "
			size_t endA = a.first.find(']', startA);
			if (endA != wxString::npos) {
				wxString idStrA = a.first.SubString(startA, endA - 1);
				long tempA;
				if (idStrA.ToLong(&tempA)) idA = tempA;
			}
		}
		
		if (startB != wxString::npos) {
			startB += 5; // Skip "[ID: "
			size_t endB = b.first.find(']', startB);
			if (endB != wxString::npos) {
				wxString idStrB = b.first.SubString(startB, endB - 1);
				long tempB;
				if (idStrB.ToLong(&tempB)) idB = tempB;
			}
		}
		
		return idA < idB;
	});
	
	// Rebuild the list
	result_list->Clear();
	for (const auto& item : items) {
		result_list->Append(item.first, item.second);
	}
}

void SearchResultWindow::SortResultsByName() {
	if (result_list->GetCount() == 0) return;
	
	// Create vector of items with their data
	std::vector<std::pair<wxString, Position*>> items;
	for (uint32_t i = 0; i < result_list->GetCount(); ++i) {
		wxString text = result_list->GetString(i);
		Position* pos = reinterpret_cast<Position*>(result_list->GetClientData(i));
		items.push_back(std::make_pair(text, pos));
	}
	
	// Sort by name (extract name before "[ID:")
	std::sort(items.begin(), items.end(), [](const std::pair<wxString, Position*>& a, const std::pair<wxString, Position*>& b) {
		// Extract name part (everything before " [ID:")
		wxString nameA = a.first;
		wxString nameB = b.first;
		
		size_t posA = nameA.Find(" [ID:");
		if (posA != wxString::npos) {
			nameA = nameA.Left(posA);
		}
		
		size_t posB = nameB.Find(" [ID:");
		if (posB != wxString::npos) {
			nameB = nameB.Left(posB);
		}
		
		return nameA.CmpNoCase(nameB) < 0;
	});
	
	// Rebuild the list
	result_list->Clear();
	for (const auto& item : items) {
		result_list->Append(item.first, item.second);
	}
}

void SearchResultWindow::SortResultsByPosition() {
	if (result_list->GetCount() == 0) return;
	
	// Create vector of items with their data
	std::vector<std::pair<wxString, Position*>> items;
	for (uint32_t i = 0; i < result_list->GetCount(); ++i) {
		wxString text = result_list->GetString(i);
		Position* pos = reinterpret_cast<Position*>(result_list->GetClientData(i));
		items.push_back(std::make_pair(text, pos));
	}
	
	// Sort by position (z first, then y, then x)
	std::sort(items.begin(), items.end(), [](const std::pair<wxString, Position*>& a, const std::pair<wxString, Position*>& b) {
		if (!a.second || !b.second) return false;
		
		if (a.second->z != b.second->z) return a.second->z < b.second->z;
		if (a.second->y != b.second->y) return a.second->y < b.second->y;
		return a.second->x < b.second->x;
	});
	
	// Rebuild the list
	result_list->Clear();
	for (const auto& item : items) {
		result_list->Append(item.first, item.second);
	}
}

void SearchResultWindow::ContinueSearch() {
	if (!has_last_search) {
		g_gui.PopupDialog("Search Error", "No previous search available to continue.", wxOK | wxICON_INFORMATION);
		return;
	}
	
	// Store current positions to avoid duplicates
	std::vector<Position> existingPositions = GetFoundPositions();
	
	// Custom Finder that skips already found positions
	class ContinuedFinder {
	public:
		ContinuedFinder(uint16_t itemId, const std::vector<Position>& existingPositions, uint32_t maxCount) :
			itemId(itemId), existingPositions(existingPositions), maxCount(maxCount) {}
		
		uint16_t itemId;
		std::vector<Position> existingPositions;
		uint32_t maxCount;
		std::vector<std::pair<Tile*, Item*>> result;
		
		bool limitReached() const {
			return result.size() >= (size_t)maxCount;
		}
		
		bool isPositionAlreadyFound(const Position& pos) const {
			for (const Position& existingPos : existingPositions) {
				if (existingPos.x == pos.x && existingPos.y == pos.y && existingPos.z == pos.z) {
					return true;
				}
			}
			return false;
		}
		
		void operator()(Map& map, Tile* tile, Item* item, long long done) {
			if (result.size() >= (size_t)maxCount) {
				return;
			}
			
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}
			
			if (item->getID() == itemId) {
				// Skip if this position was already found
				if (!isPositionAlreadyFound(tile->getPosition())) {
					result.push_back(std::make_pair(tile, item));
				}
			}
		}
	};
	
	// Run the continued search
	g_gui.CreateLoadBar("Continuing search...");
	
	ContinuedFinder finder(last_search_itemid, existingPositions, 
		(uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
	
	foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, last_search_on_selection);
	std::vector<std::pair<Tile*, Item*>>& result = finder.result;
	
	g_gui.DestroyLoadBar();
	
	if (finder.limitReached()) {
		wxString msg;
		msg << "The configured limit has been reached. Only " << finder.maxCount << " additional results will be displayed.";
		g_gui.PopupDialog("Notice", msg, wxOK);
	}
	
	if (result.empty()) {
		g_gui.PopupDialog("Search Complete", "No more matching items found.", wxOK | wxICON_INFORMATION);
		return;
	}
	
	// Add new results to the window
	for (const auto& pair : result) {
		Tile* tile = pair.first;
		Item* item = pair.second;
		
		wxString description = wxString::Format("%s (ID: %d)", 
			wxstr(item->getName()),
			item->getID());
		
		AddPosition(description, tile->getPosition());
	}
}

void SearchResultWindow::SetIgnoredIds(const wxString& ignored_ids_str, bool enable) {
	use_ignored_ids = enable;
	last_ignored_ids_text = ignored_ids_str;
	last_ignored_ids_enabled = enable;
	
	// Clear previous
	ignored_ids.clear();
	ignored_ranges.clear();
	
	if (!ignored_ids_str.empty() && enable) {
		// Parse the ignored IDs string
		wxArrayString parts = wxStringTokenize(ignored_ids_str, ",");
		
		for (const wxString& part : parts) {
			// Trim whitespace - create a non-const copy to use Trim
			wxString trimmed = part;
			trimmed.Trim(true).Trim(false);
			
			// Check if it's a range (contains "-")
			size_t rangePos = trimmed.Find('-');
			if (rangePos != wxString::npos) {
				// It's a range
				wxString firstStr = trimmed.Left(rangePos);
				wxString secondStr = trimmed.Mid(rangePos + 1);
				
				// Trim these strings as well
				firstStr.Trim(true).Trim(false);
				secondStr.Trim(true).Trim(false);
				
				long first, second;
				if (firstStr.ToLong(&first) && secondStr.ToLong(&second)) {
					ignored_ranges.push_back(std::make_pair(
						static_cast<uint16_t>(first), 
						static_cast<uint16_t>(second)));
				}
			}
			else {
				// It's a single ID
				long id;
				if (trimmed.ToLong(&id)) {
					ignored_ids.push_back(static_cast<uint16_t>(id));
				}
			}
		}
	}
}

wxString SearchResultWindow::GetIgnoredItemsText() const {
	return last_ignored_ids_text;
}

bool SearchResultWindow::IsIgnoreListEnabled() const {
	return last_ignored_ids_enabled;
}
