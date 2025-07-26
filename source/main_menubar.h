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

#ifndef RME_MAIN_BAR_H_
#define RME_MAIN_BAR_H_

#include <wx/docview.h>

namespace MenuBar {
	struct Action;

	enum ActionID {
		NEW,
		OPEN,
		SAVE,
		SAVE_AS,
		GENERATE_MAP,
		GENERATE_PROCEDURAL_MAP,
		GENERATE_ISLAND_MAP,
		GENERATE_DUNGEON_MAP,
		CLOSE,
		IMPORT_MAP,
		IMPORT_MONSTERS,
		IMPORT_MINIMAP,
		EXPORT_MINIMAP,
		EXPORT_TILESETS,
		RELOAD_DATA,
		RELOAD_REVSCRIPTS,
		RECENT_FILES,
		PREFERENCES,
		EXIT,
		UNDO,
		REDO,
		FIND_ITEM,
		FIND_CREATURE,
		REPLACE_ITEMS,
		AUTOMAGIC,
		SEARCH_ON_MAP_EVERYTHING,
		SEARCH_ON_MAP_ZONES,
		SEARCH_ON_MAP_UNIQUE,
		SEARCH_ON_MAP_ACTION,
		SEARCH_ON_MAP_CONTAINER,
		SEARCH_ON_MAP_WRITEABLE,
		SEARCH_ON_SELECTION_EVERYTHING,
		SEARCH_ON_SELECTION_ZONES,
		SEARCH_ON_SELECTION_UNIQUE,
		SEARCH_ON_SELECTION_ACTION,
		SEARCH_ON_SELECTION_CONTAINER,
		SEARCH_ON_SELECTION_WRITEABLE,
		SEARCH_ON_SELECTION_ITEM,
		REPLACE_ON_SELECTION_ITEMS,
		REMOVE_ON_SELECTION_ITEM,
		SELECT_MODE_COMPENSATE,
		SELECT_MODE_CURRENT,
		SELECT_MODE_LOWER,
		SELECT_MODE_VISIBLE,
		// AUTOMAGIC removed - now controlled by 'A' hotkey
		BORDERIZE_SELECTION,
		BORDERIZE_MAP,
		WALLIZE_SELECTION,
		WALLIZE_MAP,
		RANDOMIZE_SELECTION,
		RANDOMIZE_MAP,
		GOTO_PREVIOUS_POSITION,
		GOTO_POSITION,
		JUMP_TO_BRUSH,
		JUMP_TO_ITEM_BRUSH,
		CLEAR_INVALID_HOUSES,
		CLEAR_MODIFIED_STATE,
		CUT,
		COPY,
		PASTE,
		EDIT_TOWNS,
		EDIT_ITEMS,
		EDIT_MONSTERS,
		MAP_CLEANUP,
		MAP_REMOVE_ITEMS,
		MAP_REMOVE_CORPSES,
		MAP_REMOVE_UNREACHABLE_TILES,
		MAP_CLEAN_HOUSE_ITEMS,
		MAP_PROPERTIES,
		MAP_STATISTICS,
		VIEW_TOOLBARS_BRUSHES,
		VIEW_TOOLBARS_POSITION,
		VIEW_TOOLBARS_SIZES,
		VIEW_TOOLBARS_STANDARD,
		NEW_VIEW,
		//idler
		NEW_DETACHED_VIEW,
		TOGGLE_FULLSCREEN,
		ZOOM_IN,
		ZOOM_OUT,
		ZOOM_NORMAL,
		SHOW_SHADE,
		SHOW_ALL_FLOORS,
		GHOST_ITEMS,
		GHOST_HIGHER_FLOORS,
		HIGHLIGHT_ITEMS,
		HIGHLIGHT_LOCKED_DOORS,
		SHOW_INGAME_BOX,
		SHOW_LIGHTS,
		SHOW_LIGHT_STR,
		SHOW_TECHNICAL_ITEMS,
		SHOW_WAYPOINTS,
		SHOW_GRID,
		SHOW_EXTRA,
		SHOW_CREATURES,
		SHOW_SPAWNS,
		SHOW_SPECIAL,
		SHOW_ZONES,
		SHOW_AS_MINIMAP,
		SHOW_ONLY_COLORS,
		SHOW_ONLY_MODIFIED,
		SHOW_HOUSES,
		SHOW_PATHING,
		SHOW_TOOLTIPS,
		SHOW_PREVIEW,
		SHOW_WALL_HOOKS,
		SHOW_TOWNS,
		ALWAYS_SHOW_ZONES,
		EXT_HOUSE_SHADER,
		HOUSE_CUSTOM_COLORS,
		REFRESH_ITEMS,

		WIN_MINIMAP,
		WIN_RECENT_BRUSHES,
		NEW_PALETTE,
		TAKE_SCREENSHOT,
		LIVE_START,
		LIVE_JOIN,
		LIVE_CLOSE,
		SELECT_TERRAIN,
		SELECT_DOODAD,
		SELECT_ITEM,
		SELECT_COLLECTION,
		SELECT_CREATURE,
		SELECT_HOUSE,
		SELECT_WAYPOINT,
		SELECT_RAW,
		FLOOR_0,
		FLOOR_1,
		FLOOR_2,
		FLOOR_3,
		FLOOR_4,
		FLOOR_5,
		FLOOR_6,
		FLOOR_7,
		FLOOR_8,
		FLOOR_9,
		FLOOR_10,
		FLOOR_11,
		FLOOR_12,
		FLOOR_13,
		FLOOR_14,
		FLOOR_15,
		DEBUG_VIEW_DAT,
		EXTENSIONS,
		GOTO_WEBSITE,
		ABOUT,
		ID_MENU_SERVER_HOST,
		ID_MENU_SERVER_CONNECT,

		EXPERIMENTAL_FOG,
		MAP_REMOVE_DUPLICATES,
		SHOW_HOTKEYS,
			SHOW_MONSTER_MAKER,
	ADD_NEW_CREATURE,
	MAP_MENU_REPLACE_ITEMS,
		MAP_MENU_GENERATE_ISLAND,
		MAP_VALIDATE_GROUND,
		MAP_CREATE_BORDER,
		// Chat actions
		CHAT_REGISTER,
		CHAT_CONNECT,
		MAP_SUMMARIZE,
		MAP_NOTES,
		MENU_REFRESH_VISIBLE_AREA,
		RESET_HOUSE_IDS,
	RESET_TOWN_IDS,
	DOODADS_FILLING_TOOL,
	EDIT_ITEMS_OTB,
	};
}

class MainFrame;

class MainMenuBar : public wxEvtHandler {
public:
	MainMenuBar(MainFrame* frame);
	virtual ~MainMenuBar();

	bool Load(const FileName&, wxArrayString& warnings, wxString& error);

	// Update
	// Turn on/off all buttons according to current editor state
	void Update();
	void UpdateFloorMenu(); // Only concerns the floor menu

	void AddRecentFile(FileName file);
	void LoadRecentFiles();
	void SaveRecentFiles();
	std::vector<wxString> GetRecentFiles();

	// Interface
	void EnableItem(MenuBar::ActionID id, bool enable);
	void CheckItem(MenuBar::ActionID id, bool enable);
	bool IsItemChecked(MenuBar::ActionID id) const;

	// Event handlers for all menu buttons
	// File Menu
	void OnNew(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnGenerateMap(wxCommandEvent& event);
	void OnGenerateProceduralMap(wxCommandEvent& event);

	void OnOpenRecent(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnSaveAs(wxCommandEvent& event);
	void OnClose(wxCommandEvent& event);
	void OnPreferences(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);

	// Import Menu
	// Export Menu
	void OnImportMap(wxCommandEvent& event);
	void OnImportMonsterData(wxCommandEvent& event);
	void OnImportMinimap(wxCommandEvent& event);
	void OnExportMinimap(wxCommandEvent& event);
	void OnExportTilesets(wxCommandEvent& event);
	void OnReloadDataFiles(wxCommandEvent& event);
	void OnReloadRevScripts(wxCommandEvent& event);

	// Edit Menu
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);
	void OnBorderizeSelection(wxCommandEvent& event);
	void OnBorderizeMap(wxCommandEvent& event);
	void OnWallizeSelection(wxCommandEvent& event);
	void OnWallizeMap(wxCommandEvent& event);
	void OnRandomizeSelection(wxCommandEvent& event);
	void OnRandomizeMap(wxCommandEvent& event);
	void OnJumpToBrush(wxCommandEvent& event);
	void OnJumpToItemBrush(wxCommandEvent& event);
	void OnGotoPreviousPosition(wxCommandEvent& event);
	void OnGotoPosition(wxCommandEvent& event);
	void OnMapRemoveItems(wxCommandEvent& event);
	void OnMapRemoveCorpses(wxCommandEvent& event);
	void OnMapRemoveUnreachable(wxCommandEvent& event);
	void OnClearHouseTiles(wxCommandEvent& event);
	void OnClearModifiedState(wxCommandEvent& event);
	void OnSelectionTypeChange(wxCommandEvent& event);
	void OnCut(wxCommandEvent& event);
	void OnCopy(wxCommandEvent& event);
	void OnPaste(wxCommandEvent& event);
	void OnSearchForItem(wxCommandEvent& event);
	void OnSearchForCreature(wxCommandEvent& event);
	void OnReplaceItems(wxCommandEvent& event);
	void OnSearchForStuffOnMap(wxCommandEvent& event);
	void OnSearchForZonesOnMap(wxCommandEvent& event);
	void OnSearchForUniqueOnMap(wxCommandEvent& event);
	void OnSearchForActionOnMap(wxCommandEvent& event);
	void OnSearchForContainerOnMap(wxCommandEvent& event);
	void OnSearchForWriteableOnMap(wxCommandEvent& event);

	// Select menu
	void OnSearchForStuffOnSelection(wxCommandEvent& event);
	void OnSearchForZonesOnSelection(wxCommandEvent& event);
	void OnSearchForUniqueOnSelection(wxCommandEvent& event);
	void OnSearchForActionOnSelection(wxCommandEvent& event);
	void OnSearchForContainerOnSelection(wxCommandEvent& event);
	void OnSearchForWriteableOnSelection(wxCommandEvent& event);
	void OnSearchForItemOnSelection(wxCommandEvent& event);
	void OnReplaceItemsOnSelection(wxCommandEvent& event);
	void OnRemoveItemOnSelection(wxCommandEvent& event);

	// Map menu
	void OnMapEditTowns(wxCommandEvent& event);
	void OnMapEditItems(wxCommandEvent& event);
	void OnMapEditMonsters(wxCommandEvent& event);
	void OnMapCleanHouseItems(wxCommandEvent& event);
	void OnMapCleanup(wxCommandEvent& event);
	void OnMapProperties(wxCommandEvent& event);
	void OnMapStatistics(wxCommandEvent& event);
	void OnMapRemoveDuplicates(wxCommandEvent& event);
	void OnMapValidateGround(wxCommandEvent& event);
	void OnMapNotes(wxCommandEvent& event);
	void OnResetHouseIDs(wxCommandEvent& event);
	void OnResetTownIDs(wxCommandEvent& event);
	void OnDoodadsFillingTool(wxCommandEvent& event);
	void OnEditItemsOTB(wxCommandEvent& event);

	// View Menu
	void OnToolbars(wxCommandEvent& event);
	void OnNewView(wxCommandEvent& event);
	void OnNewDetachedView(wxCommandEvent& event);
	void OnToggleFullscreen(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	void OnZoomNormal(wxCommandEvent& event);
	void OnChangeViewSettings(wxCommandEvent& event);

	// Network menu
	void OnStartLive(wxCommandEvent& event);
	void OnJoinLive(wxCommandEvent& event);
	void OnCloseLive(wxCommandEvent& event);
	void onServerHost(wxCommandEvent& event);
	void onServerConnect(wxCommandEvent& event);

	// Window Menu
	void OnMinimapWindow(wxCommandEvent& event);
	void OnRecentBrushesWindow(wxCommandEvent& event);
	void OnNewPalette(wxCommandEvent& event);
	void OnTakeScreenshot(wxCommandEvent& event);
	void OnSelectTerrainPalette(wxCommandEvent& event);
	void OnSelectDoodadPalette(wxCommandEvent& event);
	void OnSelectItemPalette(wxCommandEvent& event);
	void OnSelectCollectionPalette(wxCommandEvent& event);
	void OnSelectHousePalette(wxCommandEvent& event);
	void OnSelectCreaturePalette(wxCommandEvent& event);
	void OnSelectWaypointPalette(wxCommandEvent& event);
	void OnSelectRawPalette(wxCommandEvent& event);

	// Floor menu
	void OnChangeFloor(wxCommandEvent& event);

	// About Menu
	void OnDebugViewDat(wxCommandEvent& event);
	void OnListExtensions(wxCommandEvent& event);
	void OnGotoWebsite(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnShowHotkeys(wxCommandEvent& event);
	void OnShowMonsterMaker(wxCommandEvent& event);
	void OnAddNewCreature(wxCommandEvent& event);

	// Add to class MainMenuBar private section:
	void OnRefreshItems(wxCommandEvent& event);
	void OnCreateBorder(wxCommandEvent& event);
	// Chat handlers
	void OnChatRegister(wxCommandEvent& event);
	void OnChatConnect(wxCommandEvent& event);

	void OnMapSummarize(wxCommandEvent& event);
	void OnRefreshVisibleArea(wxCommandEvent& event);


protected:
	// Load and returns a menu item, also sets accelerator
	wxObject* LoadItem(pugi::xml_node node, wxMenu* parent, wxArrayString& warnings, wxString& error);
	// Checks the items in the menus according to the settings (in config)
	void LoadValues();
	void SearchItems(bool unique, bool action, bool container, bool writable, bool zones, bool onSelection = false);

protected:
	MainFrame* frame;
	wxMenuBar* menubar;

	// Used so that calling Check on menu items don't trigger events (avoids infinite recursion)
	bool checking_programmaticly;

	std::map<MenuBar::ActionID, std::list<wxMenuItem*>> items;

	// Hardcoded recent files
	wxFileHistory recentFiles;

	std::map<std::string, MenuBar::Action*> actions;

	DECLARE_EVENT_TABLE();
};

namespace MenuBar {
	struct Action {
		Action() :
			id(0), kind(wxITEM_NORMAL) { }
		Action(std::string s, int id, wxItemKind kind, wxCommandEventFunction handler) :
			id(id), setting(0), name(s), kind(kind), handler(handler) { }

		int id;
		int setting;
		std::string name;
		wxItemKind kind;
		wxCommandEventFunction handler;
	};
}

// Add this declaration before the MainMenuBar class
std::vector<std::pair<uint16_t, uint16_t>> ParseRangeString(const wxString& input);

#endif
