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

#ifndef RME_RESULT_WINDOW_H_
#define RME_RESULT_WINDOW_H_

#include "main.h"

// Forward declarations
class Position;
class Map;

// Constant for Next search button ID
#define SEARCH_RESULT_NEXT_BUTTON 1001
#define SEARCH_RESULT_SORT_NUMERICAL 1002
#define SEARCH_RESULT_SORT_ALPHABETICAL 1003
#define SEARCH_RESULT_SORT_POSITION 1004

class SearchResultWindow : public wxPanel {
public:
	SearchResultWindow(wxWindow* parent);
	virtual ~SearchResultWindow();

	void Clear();
	void AddPosition(wxString description, Position pos);
	void SetIgnoredIds(const wxString& ignored_ids_str, bool enable);
	
	// Get all found positions for continuation search
	std::vector<Position> GetFoundPositions() const;
	
	// Store search parameters for continuation
	void StoreSearchInfo(uint16_t itemId, bool onSelection = false);
	
	// Continue the search with the stored parameters
	void ContinueSearch();
	
	// Methods for retrieving ignored items info
	wxString GetIgnoredItemsText() const;
	bool IsIgnoreListEnabled() const;

	void OnClickResult(wxCommandEvent&);
	void OnClickExport(wxCommandEvent&);
	void OnClickClear(wxCommandEvent&);
	void OnClickNext(wxCommandEvent&);
	void OnClickSortNumerical(wxCommandEvent&);
	void OnClickSortAlphabetical(wxCommandEvent&);
	void OnClickSortPosition(wxCommandEvent&);

protected:
	wxListBox* result_list;
	std::vector<uint16_t> ignored_ids;
	std::vector<std::pair<uint16_t, uint16_t>> ignored_ranges;
	bool use_ignored_ids;
	
	// Search continuation data
	uint16_t last_search_itemid;
	bool last_search_on_selection;
	bool has_last_search;
	wxString last_ignored_ids_text;
	bool last_ignored_ids_enabled;

	// Sorting helper methods
	void SortResultsByID();
	void SortResultsByName();
	void SortResultsByPosition();

	DECLARE_EVENT_TABLE()
};

#endif
