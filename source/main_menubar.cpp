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

	/*
	 * AUTOMAGIC SYSTEM OVERVIEW
	 * -------------------------
	 * The Automagic system in Remere's Map Editor provides automatic border and wall handling.
	 * 
	 * Files involved:
	 * - settings.h/cpp: Defines USE_AUTOMAGIC, BORDERIZE_PASTE, BORDERIZE_DRAG settings
	 * - main_menubar.h/cpp: Implements menu options for toggling Automagic and borderizing
	 * - tile.h/cpp: Contains borderize() and wallize() methods that apply automatic borders/walls
	 * - ground_brush.cpp: Implements GroundBrush::doBorders() which handles automatic borders
	 * - wall_brush.cpp: Implements WallBrush::doWalls() which handles automatic walls
	 * - borderize_window.cpp: UI for borderizing large selections or the entire map
	 * - editor.cpp: Contains borderizeSelection() and borderizeMap() methods
	 * - copybuffer.cpp: Applies borderize to pasted content
	 * 
	 * How it works:
	 * 1. When enabled (via Config::USE_AUTOMAGIC), the editor automatically applies borders
	 *    and wall connections when tiles are placed, moved, or modified.
	 * 2. Borderizing examines neighboring tiles to determine appropriate borders between
	 *    different terrain types.
	 * 3. Wallizing connects wall segments automatically based on adjacent walls.
	 * 4. The system can be triggered:
	 *    - Automatically during editing when Automagic is enabled
	 *    - Manually via Map > Borderize Selection (Ctrl+B)
	 *    - Manually via Map > Borderize Map (processes the entire map)
	 * 
	 * Settings:
	 * - BORDERIZE_PASTE: Automatically borderize after pasting
	 * - BORDERIZE_DRAG: Automatically borderize after drag operations
	 * - BORDERIZE_DRAG_THRESHOLD: Maximum selection size for auto-borderizing during drag
	 * - BORDERIZE_PASTE_THRESHOLD: Maximum selection size for auto-borderizing during paste
	 * 
	 * The BorderizeWindow provides a UI for processing large maps in chunks to avoid
	 * performance issues when borderizing extensive areas.
	 */

#include "main.h"

#include "main_menubar.h"
#include "application.h"
#include "preferences.h"
#include "about_window.h"
#include "minimap_window.h"
#include "dat_debug_view.h"
#include "result_window.h"
#include "extension_window.h"
#include "find_item_window.h"
#include "settings.h"
#include "automagic_settings.h"
#include "find_creature_window.h"
#include "map.h"
#include "editor.h"
#include "gui.h"
#include "border_editor_window.h"
#include "map_summary_window.h"
#include "otmapgen_dialog.h"
#include "notes_window.h"
#include "add_creature_dialog.h"
#include "doodads_filling_dialog.h"
#include "item_editor_window.h"

#include <wx/chartype.h>

#include "editor.h"
#include "materials.h"
#include "live_client.h"
#include "live_server.h"
#include "string_utils.h"
#include "hotkey_manager.h"
#include "chat_window.h"
#include "creature_brush.h"
#include "brush.h"
#include "tileset.h"

const wxEventType EVT_MENU = wxEVT_COMMAND_MENU_SELECTED;

BEGIN_EVENT_TABLE(MainMenuBar, wxEvtHandler)
	EVT_MENU(MenuBar::NEW, MainMenuBar::OnNew)
	EVT_MENU(MenuBar::OPEN, MainMenuBar::OnOpen)
	EVT_MENU(MenuBar::SAVE, MainMenuBar::OnSave)
	EVT_MENU(MenuBar::SAVE_AS, MainMenuBar::OnSaveAs)
	EVT_MENU(MenuBar::GENERATE_MAP, MainMenuBar::OnGenerateMap)
	EVT_MENU(MenuBar::GENERATE_PROCEDURAL_MAP, MainMenuBar::OnGenerateProceduralMap)

	
	EVT_MENU(MenuBar::FIND_CREATURE, MainMenuBar::OnSearchForCreature)
	EVT_MENU(MenuBar::RELOAD_REVSCRIPTS, MainMenuBar::OnReloadRevScripts)
	EVT_MENU(MenuBar::DOODADS_FILLING_TOOL, MainMenuBar::OnDoodadsFillingTool)
END_EVENT_TABLE()

MainMenuBar::MainMenuBar(MainFrame* frame) :
	frame(frame) {
	using namespace MenuBar;
	checking_programmaticly = false;

#define MAKE_ACTION(id, kind, handler) actions[#id] = new MenuBar::Action(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler))
#define MAKE_SET_ACTION(id, kind, setting_, handler)                                                  \
	actions[#id] = new MenuBar::Action(#id, id, kind, wxCommandEventFunction(&MainMenuBar::handler)); \
	actions[#id].setting = setting_

	MAKE_ACTION(NEW, wxITEM_NORMAL, OnNew);
	MAKE_ACTION(OPEN, wxITEM_NORMAL, OnOpen);
	MAKE_ACTION(SAVE, wxITEM_NORMAL, OnSave);
	MAKE_ACTION(SAVE_AS, wxITEM_NORMAL, OnSaveAs);
	MAKE_ACTION(GENERATE_MAP, wxITEM_NORMAL, OnGenerateMap);
	MAKE_ACTION(GENERATE_PROCEDURAL_MAP, wxITEM_NORMAL, OnGenerateProceduralMap);

	MAKE_ACTION(CLOSE, wxITEM_NORMAL, OnClose);

	MAKE_ACTION(IMPORT_MAP, wxITEM_NORMAL, OnImportMap);
	MAKE_ACTION(IMPORT_MONSTERS, wxITEM_NORMAL, OnImportMonsterData);
	MAKE_ACTION(IMPORT_MINIMAP, wxITEM_NORMAL, OnImportMinimap);
	MAKE_ACTION(EXPORT_MINIMAP, wxITEM_NORMAL, OnExportMinimap);
	MAKE_ACTION(EXPORT_TILESETS, wxITEM_NORMAL, OnExportTilesets);

	MAKE_ACTION(RELOAD_DATA, wxITEM_NORMAL, OnReloadDataFiles);
	MAKE_ACTION(RELOAD_REVSCRIPTS, wxITEM_NORMAL, OnReloadRevScripts);
	// MAKE_ACTION(RECENT_FILES, wxITEM_NORMAL, OnRecent);
	MAKE_ACTION(PREFERENCES, wxITEM_NORMAL, OnPreferences);
	MAKE_ACTION(EXIT, wxITEM_NORMAL, OnQuit);

	MAKE_ACTION(UNDO, wxITEM_NORMAL, OnUndo);
	MAKE_ACTION(REDO, wxITEM_NORMAL, OnRedo);

	MAKE_ACTION(FIND_ITEM, wxITEM_NORMAL, OnSearchForItem);
	MAKE_ACTION(REPLACE_ITEMS, wxITEM_NORMAL, OnReplaceItems);
	MAKE_ACTION(SEARCH_ON_MAP_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_ZONES, wxITEM_NORMAL, OnSearchForZonesOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_UNIQUE, wxITEM_NORMAL, OnSearchForUniqueOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_ACTION, wxITEM_NORMAL, OnSearchForActionOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnMap);
	MAKE_ACTION(SEARCH_ON_MAP_WRITEABLE, wxITEM_NORMAL, OnSearchForWriteableOnMap);
	MAKE_ACTION(SEARCH_ON_SELECTION_EVERYTHING, wxITEM_NORMAL, OnSearchForStuffOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ZONES, wxITEM_NORMAL, OnSearchForZonesOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_UNIQUE, wxITEM_NORMAL, OnSearchForUniqueOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ACTION, wxITEM_NORMAL, OnSearchForActionOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_CONTAINER, wxITEM_NORMAL, OnSearchForContainerOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_WRITEABLE, wxITEM_NORMAL, OnSearchForWriteableOnSelection);
	MAKE_ACTION(SEARCH_ON_SELECTION_ITEM, wxITEM_NORMAL, OnSearchForItemOnSelection);
	MAKE_ACTION(REPLACE_ON_SELECTION_ITEMS, wxITEM_NORMAL, OnReplaceItemsOnSelection);
	MAKE_ACTION(REMOVE_ON_SELECTION_ITEM, wxITEM_NORMAL, OnRemoveItemOnSelection);
	MAKE_ACTION(SELECT_MODE_COMPENSATE, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_LOWER, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_CURRENT, wxITEM_RADIO, OnSelectionTypeChange);
	MAKE_ACTION(SELECT_MODE_VISIBLE, wxITEM_RADIO, OnSelectionTypeChange);

	// Remove the AUTOMAGIC action as it's now handled by 'A' hotkey
	// MAKE_ACTION(AUTOMAGIC, wxITEM_CHECK, OnToggleAutomagic);
	
	MAKE_ACTION(BORDERIZE_SELECTION, wxITEM_NORMAL, OnBorderizeSelection);
	MAKE_ACTION(BORDERIZE_MAP, wxITEM_NORMAL, OnBorderizeMap);
	MAKE_ACTION(WALLIZE_SELECTION, wxITEM_NORMAL, OnWallizeSelection);
	MAKE_ACTION(WALLIZE_MAP, wxITEM_NORMAL, OnWallizeMap);
	MAKE_ACTION(RANDOMIZE_SELECTION, wxITEM_NORMAL, OnRandomizeSelection);
	MAKE_ACTION(RANDOMIZE_MAP, wxITEM_NORMAL, OnRandomizeMap);
	MAKE_ACTION(GOTO_PREVIOUS_POSITION, wxITEM_NORMAL, OnGotoPreviousPosition);
	MAKE_ACTION(GOTO_POSITION, wxITEM_NORMAL, OnGotoPosition);
	MAKE_ACTION(JUMP_TO_BRUSH, wxITEM_NORMAL, OnJumpToBrush);
	MAKE_ACTION(JUMP_TO_ITEM_BRUSH, wxITEM_NORMAL, OnJumpToItemBrush);
	MAKE_ACTION(MENU_REFRESH_VISIBLE_AREA, wxITEM_NORMAL, OnRefreshVisibleArea);

	MAKE_ACTION(CUT, wxITEM_NORMAL, OnCut);
	MAKE_ACTION(COPY, wxITEM_NORMAL, OnCopy);
	MAKE_ACTION(PASTE, wxITEM_NORMAL, OnPaste);

	MAKE_ACTION(EDIT_TOWNS, wxITEM_NORMAL, OnMapEditTowns);
	MAKE_ACTION(EDIT_ITEMS, wxITEM_NORMAL, OnMapEditItems);
	MAKE_ACTION(EDIT_MONSTERS, wxITEM_NORMAL, OnMapEditMonsters);

	MAKE_ACTION(CLEAR_INVALID_HOUSES, wxITEM_NORMAL, OnClearHouseTiles);
	MAKE_ACTION(CLEAR_MODIFIED_STATE, wxITEM_NORMAL, OnClearModifiedState);
	MAKE_ACTION(MAP_REMOVE_ITEMS, wxITEM_NORMAL, OnMapRemoveItems);
	MAKE_ACTION(MAP_REMOVE_CORPSES, wxITEM_NORMAL, OnMapRemoveCorpses);
	MAKE_ACTION(MAP_REMOVE_DUPLICATES, wxITEM_NORMAL, OnMapRemoveDuplicates);
	MAKE_ACTION(MAP_VALIDATE_GROUND, wxITEM_NORMAL, OnMapValidateGround);
	MAKE_ACTION(MAP_REMOVE_UNREACHABLE_TILES, wxITEM_NORMAL, OnMapRemoveUnreachable);
	MAKE_ACTION(MAP_CLEANUP, wxITEM_NORMAL, OnMapCleanup);
	MAKE_ACTION(MAP_CLEAN_HOUSE_ITEMS, wxITEM_NORMAL, OnMapCleanHouseItems);
	MAKE_ACTION(MAP_PROPERTIES, wxITEM_NORMAL, OnMapProperties);
	MAKE_ACTION(MAP_STATISTICS, wxITEM_NORMAL, OnMapStatistics);
	MAKE_ACTION(MAP_NOTES, wxITEM_NORMAL, OnMapNotes);
	MAKE_ACTION(DOODADS_FILLING_TOOL, wxITEM_NORMAL, OnDoodadsFillingTool);

	MAKE_ACTION(VIEW_TOOLBARS_BRUSHES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_POSITION, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_SIZES, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(VIEW_TOOLBARS_STANDARD, wxITEM_CHECK, OnToolbars);
	MAKE_ACTION(NEW_VIEW, wxITEM_NORMAL, OnNewView);
	MAKE_ACTION(NEW_DETACHED_VIEW, wxITEM_NORMAL, OnNewDetachedView);
	MAKE_ACTION(TOGGLE_FULLSCREEN, wxITEM_NORMAL, OnToggleFullscreen);

	MAKE_ACTION(ZOOM_IN, wxITEM_NORMAL, OnZoomIn);
	MAKE_ACTION(ZOOM_OUT, wxITEM_NORMAL, OnZoomOut);
	MAKE_ACTION(ZOOM_NORMAL, wxITEM_NORMAL, OnZoomNormal);

	MAKE_ACTION(SHOW_SHADE, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ALL_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(GHOST_HIGHER_FLOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HIGHLIGHT_LOCKED_DOORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_EXTRA, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_INGAME_BOX, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_LIGHT_STR, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TECHNICAL_ITEMS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WAYPOINTS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_GRID, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_CREATURES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPAWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_SPECIAL, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ZONES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_AS_MINIMAP, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_COLORS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_ONLY_MODIFIED, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_HOUSES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PATHING, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOOLTIPS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_PREVIEW, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_WALL_HOOKS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(SHOW_TOWNS, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(ALWAYS_SHOW_ZONES, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(EXT_HOUSE_SHADER, wxITEM_CHECK, OnChangeViewSettings);
	MAKE_ACTION(HOUSE_CUSTOM_COLORS, wxITEM_CHECK, OnChangeViewSettings);

	MAKE_ACTION(EXPERIMENTAL_FOG, wxITEM_CHECK, OnChangeViewSettings); // experimental

	MAKE_ACTION(WIN_MINIMAP, wxITEM_NORMAL, OnMinimapWindow);
	MAKE_ACTION(WIN_RECENT_BRUSHES, wxITEM_NORMAL, OnRecentBrushesWindow);
	MAKE_ACTION(NEW_PALETTE, wxITEM_NORMAL, OnNewPalette);
	MAKE_ACTION(TAKE_SCREENSHOT, wxITEM_NORMAL, OnTakeScreenshot);

	MAKE_ACTION(LIVE_START, wxITEM_NORMAL, OnStartLive);
	MAKE_ACTION(LIVE_JOIN, wxITEM_NORMAL, OnJoinLive);
	MAKE_ACTION(LIVE_CLOSE, wxITEM_NORMAL, OnCloseLive);
	MAKE_ACTION(ID_MENU_SERVER_HOST, wxITEM_NORMAL, onServerHost);
	MAKE_ACTION(ID_MENU_SERVER_CONNECT, wxITEM_NORMAL, onServerConnect);

	MAKE_ACTION(SELECT_TERRAIN, wxITEM_NORMAL, OnSelectTerrainPalette);
	MAKE_ACTION(SELECT_DOODAD, wxITEM_NORMAL, OnSelectDoodadPalette);
	MAKE_ACTION(SELECT_ITEM, wxITEM_NORMAL, OnSelectItemPalette);
	MAKE_ACTION(SELECT_COLLECTION, wxITEM_NORMAL, OnSelectCollectionPalette);
	MAKE_ACTION(SELECT_CREATURE, wxITEM_NORMAL, OnSelectCreaturePalette);
	MAKE_ACTION(SELECT_HOUSE, wxITEM_NORMAL, OnSelectHousePalette);
	MAKE_ACTION(SELECT_WAYPOINT, wxITEM_NORMAL, OnSelectWaypointPalette);
	MAKE_ACTION(SELECT_RAW, wxITEM_NORMAL, OnSelectRawPalette);

	MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_1, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_2, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_3, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_4, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_5, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_6, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_7, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_8, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_9, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_10, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_11, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_12, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_13, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_14, wxITEM_RADIO, OnChangeFloor);
	MAKE_ACTION(FLOOR_15, wxITEM_RADIO, OnChangeFloor);

	MAKE_ACTION(DEBUG_VIEW_DAT, wxITEM_NORMAL, OnDebugViewDat);
	MAKE_ACTION(EXTENSIONS, wxITEM_NORMAL, OnListExtensions);
	MAKE_ACTION(GOTO_WEBSITE, wxITEM_NORMAL, OnGotoWebsite);
	MAKE_ACTION(ABOUT, wxITEM_NORMAL, OnAbout);
	MAKE_ACTION(SHOW_HOTKEYS, wxITEM_NORMAL, OnShowHotkeys); // Add this line
	MAKE_ACTION(SHOW_MONSTER_MAKER, wxITEM_NORMAL, OnShowMonsterMaker);
	MAKE_ACTION(ADD_NEW_CREATURE, wxITEM_NORMAL, OnAddNewCreature);
	MAKE_ACTION(REFRESH_ITEMS, wxITEM_NORMAL, OnRefreshItems);
	// 669
	MAKE_ACTION(FIND_CREATURE, wxITEM_NORMAL, OnSearchForCreature);
	MAKE_ACTION(MAP_CREATE_BORDER, wxITEM_NORMAL, OnCreateBorder);
	// Chat actions
	MAKE_ACTION(CHAT_REGISTER, wxITEM_NORMAL, OnChatRegister);
	MAKE_ACTION(CHAT_CONNECT, wxITEM_NORMAL, OnChatConnect);
	MAKE_ACTION(MAP_SUMMARIZE, wxITEM_NORMAL, OnMapSummarize);
	MAKE_ACTION(RESET_HOUSE_IDS, wxITEM_NORMAL, OnResetHouseIDs);
	MAKE_ACTION(RESET_TOWN_IDS, wxITEM_NORMAL, OnResetTownIDs);
	MAKE_ACTION(DOODADS_FILLING_TOOL, wxITEM_NORMAL, OnDoodadsFillingTool);
	MAKE_ACTION(EDIT_ITEMS_OTB, wxITEM_NORMAL, OnEditItemsOTB);

	// A deleter, this way the frame does not need
	// to bother deleting us.
	class CustomMenuBar : public wxMenuBar {
	public:
		CustomMenuBar(MainMenuBar* mb) :
			mb(mb) { }
		~CustomMenuBar() {
			delete mb;
		}

	private:
		MainMenuBar* mb;
	};

	menubar = newd CustomMenuBar(this);
	frame->SetMenuBar(menubar);

	// Tie all events to this handler!

	for (std::map<std::string, MenuBar::Action*>::iterator ai = actions.begin(); ai != actions.end(); ++ai) {
		frame->Connect(MAIN_FRAME_MENU + ai->second->id, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)(wxEventFunction)(ai->second->handler), nullptr, this);
	}
	for (size_t i = 0; i < 10; ++i) {
		frame->Connect(recentFiles.GetBaseId() + i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainMenuBar::OnOpenRecent), nullptr, this);
	}
}

MainMenuBar::~MainMenuBar() {
	// Don't need to delete menubar, it's owned by the frame

	for (std::map<std::string, MenuBar::Action*>::iterator ai = actions.begin(); ai != actions.end(); ++ai) {
		delete ai->second;
	}
}

namespace OnMapRemoveItems {
	struct RemoveItemCondition {
		RemoveItemCondition(uint16_t itemId) :
			itemId(itemId) { }

		uint16_t itemId;

		bool operator()(Map& map, Item* item, int64_t removed, int64_t done) {
			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((uint32_t)(100 * done / map.getTileCount()));
			}
			return item->getID() == itemId && !item->isComplex();
		}
	};
}

void MainMenuBar::EnableItem(MenuBar::ActionID id, bool enable) {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::iterator fi = items.find(id);
	if (fi == items.end()) {
		return;
	}

	std::list<wxMenuItem*>& li = fi->second;

	for (std::list<wxMenuItem*>::iterator i = li.begin(); i != li.end(); ++i) {
		(*i)->Enable(enable);
	}
}

void MainMenuBar::CheckItem(MenuBar::ActionID id, bool enable) {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::iterator fi = items.find(id);
	if (fi == items.end()) {
		return;
	}

	std::list<wxMenuItem*>& li = fi->second;

	checking_programmaticly = true;
	for (std::list<wxMenuItem*>::iterator i = li.begin(); i != li.end(); ++i) {
		(*i)->Check(enable);
	}
	checking_programmaticly = false;
}

bool MainMenuBar::IsItemChecked(MenuBar::ActionID id) const {
	std::map<MenuBar::ActionID, std::list<wxMenuItem*>>::const_iterator fi = items.find(id);
	if (fi == items.end()) {
		return false;
	}

	const std::list<wxMenuItem*>& li = fi->second;

	for (std::list<wxMenuItem*>::const_iterator i = li.begin(); i != li.end(); ++i) {
		if ((*i)->IsChecked()) {
			return true;
		}
	}

	return false;
}

void MainMenuBar::Update() {
	using namespace MenuBar;
	// This updates all buttons and sets them to proper enabled/disabled state

	bool enable = !g_gui.IsWelcomeDialogShown();
	menubar->Enable(enable);
	if (!enable) {
		return;
	}

	Editor* editor = g_gui.GetCurrentEditor();
	if (editor) {
		EnableItem(UNDO, editor->actionQueue->canUndo());
		EnableItem(REDO, editor->actionQueue->canRedo());
		EnableItem(PASTE, editor->copybuffer.canPaste());
	} else {
		EnableItem(UNDO, false);
		EnableItem(REDO, false);
		EnableItem(PASTE, false);
	}

	bool loaded = g_gui.IsVersionLoaded();
	bool has_map = editor != nullptr;
	bool has_selection = editor && editor->hasSelection();
	bool is_live = editor && editor->IsLive();
	bool is_host = has_map && !editor->IsLiveClient();
	bool is_local = has_map && !is_live;

	EnableItem(CLOSE, is_local);
	EnableItem(SAVE, is_host);
	EnableItem(SAVE_AS, is_host);
	EnableItem(GENERATE_MAP, false);

	EnableItem(IMPORT_MAP, is_local);
	EnableItem(IMPORT_MONSTERS, is_local);
	EnableItem(IMPORT_MINIMAP, false);
	EnableItem(EXPORT_MINIMAP, is_local);
	EnableItem(EXPORT_TILESETS, loaded);

	EnableItem(RELOAD_REVSCRIPTS, is_local);

	EnableItem(FIND_ITEM, is_host);
	EnableItem(REPLACE_ITEMS, is_local);
	EnableItem(SEARCH_ON_MAP_EVERYTHING, is_host);
	EnableItem(SEARCH_ON_MAP_UNIQUE, is_host);
	EnableItem(SEARCH_ON_MAP_ACTION, is_host);
	EnableItem(SEARCH_ON_MAP_CONTAINER, is_host);
	EnableItem(SEARCH_ON_MAP_WRITEABLE, is_host);
	EnableItem(SEARCH_ON_SELECTION_EVERYTHING, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_UNIQUE, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_ACTION, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_CONTAINER, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_WRITEABLE, has_selection && is_host);
	EnableItem(SEARCH_ON_SELECTION_ITEM, has_selection && is_host);
	EnableItem(REPLACE_ON_SELECTION_ITEMS, has_selection && is_host);
	EnableItem(REMOVE_ON_SELECTION_ITEM, has_selection && is_host);

	EnableItem(CUT, has_map);
	EnableItem(COPY, has_map);

	EnableItem(BORDERIZE_SELECTION, has_map && has_selection);
	EnableItem(BORDERIZE_MAP, is_local);
	EnableItem(WALLIZE_SELECTION, has_map && has_selection);
	EnableItem(WALLIZE_MAP, is_local);
	EnableItem(RANDOMIZE_SELECTION, has_map && has_selection);
	EnableItem(RANDOMIZE_MAP, is_local);

	EnableItem(GOTO_PREVIOUS_POSITION, has_map);
	EnableItem(GOTO_POSITION, has_map);
	EnableItem(JUMP_TO_BRUSH, loaded);
	EnableItem(JUMP_TO_ITEM_BRUSH, loaded);

	EnableItem(MAP_REMOVE_ITEMS, is_host);
	EnableItem(MAP_REMOVE_CORPSES, is_local);
	EnableItem(MAP_REMOVE_DUPLICATES, is_local);
	EnableItem(MAP_REMOVE_UNREACHABLE_TILES, is_local);
	EnableItem(CLEAR_INVALID_HOUSES, is_local);
	EnableItem(CLEAR_MODIFIED_STATE, is_local);

	EnableItem(EDIT_TOWNS, is_local);
	EnableItem(EDIT_ITEMS, false);
	EnableItem(EDIT_MONSTERS, false);

	EnableItem(MAP_CLEANUP, is_local);
	EnableItem(MAP_PROPERTIES, is_local);
	EnableItem(MAP_STATISTICS, is_local);
	EnableItem(MAP_NOTES, is_local);

	EnableItem(NEW_VIEW, has_map);
	EnableItem(NEW_DETACHED_VIEW, has_map);
	EnableItem(ZOOM_IN, has_map);
	EnableItem(ZOOM_OUT, has_map);
	EnableItem(ZOOM_NORMAL, has_map);

	if (has_map) {
		CheckItem(SHOW_SPAWNS, g_settings.getBoolean(Config::SHOW_SPAWNS));
	}

	EnableItem(WIN_MINIMAP, loaded);
	EnableItem(NEW_PALETTE, loaded);
	EnableItem(SELECT_TERRAIN, loaded);
	EnableItem(SELECT_DOODAD, loaded);
	EnableItem(SELECT_ITEM, loaded);
	EnableItem(SELECT_COLLECTION, loaded);
	EnableItem(SELECT_HOUSE, loaded);
	EnableItem(SELECT_CREATURE, loaded);
	EnableItem(SELECT_WAYPOINT, loaded);
	EnableItem(SELECT_RAW, loaded);

	EnableItem(LIVE_START, is_local);
	EnableItem(LIVE_JOIN, loaded);
	EnableItem(LIVE_CLOSE, is_live);
	EnableItem(ID_MENU_SERVER_HOST, is_local);
	EnableItem(ID_MENU_SERVER_CONNECT, loaded);

	EnableItem(DEBUG_VIEW_DAT, loaded);

	UpdateFloorMenu();
}

void MainMenuBar::LoadValues() {
	using namespace MenuBar;

	CheckItem(VIEW_TOOLBARS_BRUSHES, g_settings.getBoolean(Config::SHOW_TOOLBAR_BRUSHES));
	CheckItem(VIEW_TOOLBARS_POSITION, g_settings.getBoolean(Config::SHOW_TOOLBAR_POSITION));
	CheckItem(VIEW_TOOLBARS_SIZES, g_settings.getBoolean(Config::SHOW_TOOLBAR_SIZES));
	CheckItem(VIEW_TOOLBARS_STANDARD, g_settings.getBoolean(Config::SHOW_TOOLBAR_STANDARD));

	CheckItem(SELECT_MODE_COMPENSATE, g_settings.getBoolean(Config::COMPENSATED_SELECT));

	if (IsItemChecked(MenuBar::SELECT_MODE_CURRENT)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_LOWER)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_VISIBLE)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}

	switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
		case SELECT_CURRENT_FLOOR:
			CheckItem(SELECT_MODE_CURRENT, true);
			break;
		case SELECT_ALL_FLOORS:
			CheckItem(SELECT_MODE_LOWER, true);
			break;
		default:
		case SELECT_VISIBLE_FLOORS:
			CheckItem(SELECT_MODE_VISIBLE, true);
			break;
	}

	// No longer needed as automagic is controlled through preferences and hotkey 'A'
	 CheckItem(AUTOMAGIC, g_settings.getBoolean(Config::USE_AUTOMAGIC));

	CheckItem(SHOW_SHADE, g_settings.getBoolean(Config::SHOW_SHADE));
	CheckItem(SHOW_INGAME_BOX, g_settings.getBoolean(Config::SHOW_INGAME_BOX));
	CheckItem(SHOW_LIGHTS, g_settings.getBoolean(Config::SHOW_LIGHTS));
	CheckItem(SHOW_LIGHT_STR, g_settings.getBoolean(Config::SHOW_LIGHT_STR));
	CheckItem(SHOW_TECHNICAL_ITEMS, g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS));
	CheckItem(SHOW_WAYPOINTS, g_settings.getBoolean(Config::SHOW_WAYPOINTS));
	CheckItem(SHOW_ALL_FLOORS, g_settings.getBoolean(Config::SHOW_ALL_FLOORS));
	CheckItem(GHOST_ITEMS, g_settings.getBoolean(Config::TRANSPARENT_ITEMS));
	CheckItem(GHOST_HIGHER_FLOORS, g_settings.getBoolean(Config::TRANSPARENT_FLOORS));
	CheckItem(SHOW_EXTRA, !g_settings.getBoolean(Config::SHOW_EXTRA));
	CheckItem(SHOW_GRID, g_settings.getBoolean(Config::SHOW_GRID));
	CheckItem(HIGHLIGHT_ITEMS, g_settings.getBoolean(Config::HIGHLIGHT_ITEMS));
	CheckItem(HIGHLIGHT_LOCKED_DOORS, g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS));
	CheckItem(SHOW_CREATURES, g_settings.getBoolean(Config::SHOW_CREATURES));
	CheckItem(SHOW_SPAWNS, g_settings.getBoolean(Config::SHOW_SPAWNS));
	CheckItem(SHOW_SPECIAL, g_settings.getBoolean(Config::SHOW_SPECIAL_TILES));
	CheckItem(SHOW_ZONES, g_settings.getBoolean(Config::SHOW_ZONE_AREAS));
	CheckItem(SHOW_AS_MINIMAP, g_settings.getBoolean(Config::SHOW_AS_MINIMAP));
	CheckItem(SHOW_ONLY_COLORS, g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS));
	CheckItem(SHOW_ONLY_MODIFIED, g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES));
	CheckItem(SHOW_HOUSES, g_settings.getBoolean(Config::SHOW_HOUSES));
	CheckItem(SHOW_PATHING, g_settings.getBoolean(Config::SHOW_BLOCKING));
	CheckItem(SHOW_TOOLTIPS, g_settings.getBoolean(Config::SHOW_TOOLTIPS));
	CheckItem(SHOW_PREVIEW, g_settings.getBoolean(Config::SHOW_PREVIEW));
	CheckItem(SHOW_WALL_HOOKS, g_settings.getBoolean(Config::SHOW_WALL_HOOKS));
	CheckItem(SHOW_TOWNS, g_settings.getBoolean(Config::SHOW_TOWNS));
	CheckItem(ALWAYS_SHOW_ZONES, g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES));
	CheckItem(EXT_HOUSE_SHADER, g_settings.getBoolean(Config::EXT_HOUSE_SHADER));
	CheckItem(HOUSE_CUSTOM_COLORS, g_settings.getBoolean(Config::HOUSE_CUSTOM_COLORS));

	CheckItem(EXPERIMENTAL_FOG, g_settings.getBoolean(Config::EXPERIMENTAL_FOG));
}

void MainMenuBar::LoadRecentFiles() {
	recentFiles.Load(g_settings.getConfigObject());
}

void MainMenuBar::SaveRecentFiles() {
	recentFiles.Save(g_settings.getConfigObject());
}

void MainMenuBar::AddRecentFile(FileName file) {
	recentFiles.AddFileToHistory(file.GetFullPath());
}

std::vector<wxString> MainMenuBar::GetRecentFiles() {
	std::vector<wxString> files(recentFiles.GetCount());
	for (size_t i = 0; i < recentFiles.GetCount(); ++i) {
		files[i] = recentFiles.GetHistoryFile(i);
	}
	return files;
}

void MainMenuBar::UpdateFloorMenu() {
	// this will have to be changed if you want to have more floors
	// see MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	if (MAP_MAX_LAYER < 16) {
		if (g_gui.IsEditorOpen()) {
			for (int i = 0; i < MAP_LAYERS; ++i) {
				CheckItem(MenuBar::ActionID(MenuBar::FLOOR_0 + i), false);
			}
			CheckItem(MenuBar::ActionID(MenuBar::FLOOR_0 + g_gui.GetCurrentFloor()), true);
		}
	}
}

bool MainMenuBar::Load(const FileName& path, wxArrayString& warnings, wxString& error) {
	// Open the XML file
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.GetFullPath().mb_str());
	if (!result) {
		error = "Could not open " + path.GetFullName() + " (file not found or syntax error)";
		return false;
	}

	pugi::xml_node node = doc.child("menubar");
	if (!node) {
		error = path.GetFullName() + ": Invalid rootheader.";
		return false;
	}

	// Clear the menu
	while (menubar->GetMenuCount() > 0) {
		menubar->Remove(0);
	}

	// Load succeded
	for (pugi::xml_node menuNode = node.first_child(); menuNode; menuNode = menuNode.next_sibling()) {
		// For each child node, load it
		wxObject* i = LoadItem(menuNode, nullptr, warnings, error);
		wxMenu* m = dynamic_cast<wxMenu*>(i);
		if (m) {
			menubar->Append(m, m->GetTitle());
#ifdef __APPLE__
			m->SetTitle(m->GetTitle());
#else
			m->SetTitle("");
#endif
		} else if (i) {
			delete i;
			warnings.push_back(path.GetFullName() + ": Only menus can be subitems of main menu");
		}
	}

#ifdef __LINUX__
	const int count = 44;
	wxAcceleratorEntry entries[count];
	// Edit
	entries[0].Set(wxACCEL_CTRL, (int)'Z', MAIN_FRAME_MENU + MenuBar::UNDO);
	entries[1].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'Z', MAIN_FRAME_MENU + MenuBar::REDO);
	entries[2].Set(wxACCEL_CTRL, (int)'F', MAIN_FRAME_MENU + MenuBar::FIND_ITEM);
	entries[3].Set(wxACCEL_CTRL | wxACCEL_SHIFT, (int)'F', MAIN_FRAME_MENU + MenuBar::REPLACE_ITEMS);
	entries[4].Set(wxACCEL_NORMAL, (int)'A', MAIN_FRAME_MENU + MenuBar::AUTOMAGIC);
	entries[5].Set(wxACCEL_CTRL, (int)'B', MAIN_FRAME_MENU + MenuBar::BORDERIZE_SELECTION);
	entries[6].Set(wxACCEL_NORMAL, (int)'P', MAIN_FRAME_MENU + MenuBar::GOTO_PREVIOUS_POSITION);
	entries[7].Set(wxACCEL_CTRL, (int)'G', MAIN_FRAME_MENU + MenuBar::GOTO_POSITION);
	entries[8].Set(wxACCEL_NORMAL, (int)'J', MAIN_FRAME_MENU + MenuBar::JUMP_TO_BRUSH);
	entries[9].Set(wxACCEL_CTRL, (int)'X', MAIN_FRAME_MENU + MenuBar::CUT);
	entries[10].Set(wxACCEL_CTRL, (int)'C', MAIN_FRAME_MENU + MenuBar::COPY);
	entries[11].Set(wxACCEL_CTRL, (int)'V', MAIN_FRAME_MENU + MenuBar::PASTE);
	// View
	entries[12].Set(wxACCEL_CTRL, (int)'=', MAIN_FRAME_MENU + MenuBar::ZOOM_IN);
	entries[13].Set(wxACCEL_CTRL, (int)'-', MAIN_FRAME_MENU + MenuBar::ZOOM_OUT);
	entries[14].Set(wxACCEL_CTRL, (int)'0', MAIN_FRAME_MENU + MenuBar::ZOOM_NORMAL);
	entries[15].Set(wxACCEL_NORMAL, (int)'Q', MAIN_FRAME_MENU + MenuBar::SHOW_SHADE);
	entries[16].Set(wxACCEL_CTRL, (int)'W', MAIN_FRAME_MENU + MenuBar::SHOW_ALL_FLOORS);
	entries[17].Set(wxACCEL_NORMAL, (int)'Q', MAIN_FRAME_MENU + MenuBar::GHOST_ITEMS);
	entries[18].Set(wxACCEL_CTRL, (int)'L', MAIN_FRAME_MENU + MenuBar::GHOST_HIGHER_FLOORS);
	entries[19].Set(wxACCEL_SHIFT, (int)'I', MAIN_FRAME_MENU + MenuBar::SHOW_INGAME_BOX);
	entries[20].Set(wxACCEL_SHIFT, (int)'L', MAIN_FRAME_MENU + MenuBar::SHOW_LIGHTS);
	entries[21].Set(wxACCEL_SHIFT, (int)'G', MAIN_FRAME_MENU + MenuBar::SHOW_GRID);
	entries[22].Set(wxACCEL_NORMAL, (int)'V', MAIN_FRAME_MENU + MenuBar::HIGHLIGHT_ITEMS);
	entries[23].Set(wxACCEL_NORMAL, (int)'X', MAIN_FRAME_MENU + MenuBar::HIGHLIGHT_LOCKED_DOORS);
	entries[24].Set(wxACCEL_NORMAL, (int)'F', MAIN_FRAME_MENU + MenuBar::SHOW_CREATURES);
	entries[25].Set(wxACCEL_NORMAL, (int)'S', MAIN_FRAME_MENU + MenuBar::SHOW_SPAWNS);
	entries[26].Set(wxACCEL_NORMAL, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_SPECIAL);
	entries[27].Set(wxACCEL_SHIFT, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_AS_MINIMAP);
	entries[28].Set(wxACCEL_CTRL, (int)'E', MAIN_FRAME_MENU + MenuBar::SHOW_ONLY_COLORS);
	entries[29].Set(wxACCEL_CTRL, (int)'M', MAIN_FRAME_MENU + MenuBar::SHOW_ONLY_MODIFIED);
	entries[30].Set(wxACCEL_CTRL, (int)'H', MAIN_FRAME_MENU + MenuBar::SHOW_HOUSES);
	entries[31].Set(wxACCEL_NORMAL, (int)'O', MAIN_FRAME_MENU + MenuBar::SHOW_PATHING);
	entries[32].Set(wxACCEL_NORMAL, (int)'Y', MAIN_FRAME_MENU + MenuBar::SHOW_TOOLTIPS);
	entries[33].Set(wxACCEL_NORMAL, (int)'L', MAIN_FRAME_MENU + MenuBar::SHOW_PREVIEW);
	entries[34].Set(wxACCEL_NORMAL, (int)'K', MAIN_FRAME_MENU + MenuBar::SHOW_WALL_HOOKS);

	// Window
	entries[35].Set(wxACCEL_NORMAL, (int)'M', MAIN_FRAME_MENU + MenuBar::WIN_MINIMAP);
	entries[36].Set(wxACCEL_NORMAL, (int)'T', MAIN_FRAME_MENU + MenuBar::SELECT_TERRAIN);
	entries[37].Set(wxACCEL_NORMAL, (int)'D', MAIN_FRAME_MENU + MenuBar::SELECT_DOODAD);
	entries[38].Set(wxACCEL_NORMAL, (int)'I', MAIN_FRAME_MENU + MenuBar::SELECT_ITEM);
	entries[39].Set(wxACCEL_NORMAL, (int)'N', MAIN_FRAME_MENU + MenuBar::SELECT_COLLECTION);
	entries[40].Set(wxACCEL_NORMAL, (int)'H', MAIN_FRAME_MENU + MenuBar::SELECT_HOUSE);
	entries[41].Set(wxACCEL_NORMAL, (int)'C', MAIN_FRAME_MENU + MenuBar::SELECT_CREATURE);
	entries[42].Set(wxACCEL_NORMAL, (int)'W', MAIN_FRAME_MENU + MenuBar::SELECT_WAYPOINT);
	entries[43].Set(wxACCEL_NORMAL, (int)'R', MAIN_FRAME_MENU + MenuBar::SELECT_RAW);

	wxAcceleratorTable accelerator(count, entries);
	frame->SetAcceleratorTable(accelerator);
#endif

	/*
	// Create accelerator table
	accelerator_table = newd wxAcceleratorTable(accelerators.size(), &accelerators[0]);

	// Tell all clients of the renewed accelerators
	RenewClients();
	*/

	recentFiles.AddFilesToMenu();
	Update();
	LoadValues();
	return true;
}

wxObject* MainMenuBar::LoadItem(pugi::xml_node node, wxMenu* parent, wxArrayString& warnings, wxString& error) {
	pugi::xml_attribute attribute;

	const std::string& nodeName = as_lower_str(node.name());
	if (nodeName == "menu") {
		if (!(attribute = node.attribute("name"))) {
			return nullptr;
		}

		std::string name = attribute.as_string();
		std::replace(name.begin(), name.end(), '$', '&');

		wxMenu* menu = newd wxMenu;
		if ((attribute = node.attribute("special")) && std::string(attribute.as_string()) == "RECENT_FILES") {
			recentFiles.UseMenu(menu);
		} else {
			for (pugi::xml_node menuNode = node.first_child(); menuNode; menuNode = menuNode.next_sibling()) {
				// Load an add each item in order
				LoadItem(menuNode, menu, warnings, error);
			}
		}

		// If we have a parent, add ourselves.
		// If not, we just return the item and the parent function
		// is responsible for adding us to wherever
		if (parent) {
			parent->AppendSubMenu(menu, wxstr(name));
		} else {
			menu->SetTitle((name));
		}
		return menu;
	} else if (nodeName == "item") {
		// We must have a parent when loading items
		if (!parent) {
			return nullptr;
		} else if (!(attribute = node.attribute("name"))) {
			return nullptr;
		}

		std::string name = attribute.as_string();
		std::replace(name.begin(), name.end(), '$', '&');
		if (!(attribute = node.attribute("action"))) {
			return nullptr;
		}

		const std::string& action = attribute.as_string();
		std::string hotkey = node.attribute("hotkey").as_string();
		if (!hotkey.empty()) {
			hotkey = '\t' + hotkey;
		}

		const std::string& help = node.attribute("help").as_string();
		name += hotkey;

		auto it = actions.find(action);
		if (it == actions.end()) {
			warnings.push_back("Invalid action type '" + wxstr(action) + "'.");
			return nullptr;
		}

		const MenuBar::Action& act = *it->second;
		wxAcceleratorEntry* entry = wxAcceleratorEntry::Create(wxstr(hotkey));
		if (entry) {
			delete entry; // accelerators.push_back(entry);
		} else {
			warnings.push_back("Invalid hotkey.");
		}

		wxMenuItem* tmp = parent->Append(
			MAIN_FRAME_MENU + act.id, // ID
			wxstr(name), // Title of button
			wxstr(help), // Help text
			act.kind // Kind of item
		);
		items[MenuBar::ActionID(act.id)].push_back(tmp);
		return tmp;
	} else if (nodeName == "separator") {
		// We must have a parent when loading items
		if (!parent) {
			return nullptr;
		}
		return parent->AppendSeparator();
	}
	return nullptr;
}

void MainMenuBar::OnNew(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewMap();
}

void MainMenuBar::OnGenerateMap(wxCommandEvent& WXUNUSED(event)) {
	/*
	if(!DoQuerySave()) return;

	std::ostringstream os;
	os << "Untitled-" << untitled_counter << ".otbm";
	++untitled_counter;

	editor.generateMap(wxstr(os.str()));

	g_gui.SetStatusText("Generated newd map");

	g_gui.UpdateTitle();
	g_gui.RefreshPalettes();
	g_gui.UpdateMinimap();
	g_gui.FitViewToMap();
	UpdateMenubar();
	Refresh();
	*/
}

void MainMenuBar::OnGenerateProceduralMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		wxMessageBox("Please create or open a map first before generating procedural content.", "No Map Open", wxOK | wxICON_WARNING);
		return;
	}
	
	OTMapGenDialog dialog(frame);
	if (dialog.ShowModal() == wxID_OK) {
		// Map was generated successfully
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	}
}


void MainMenuBar::OnOpenRecent(wxCommandEvent& event) {
	FileName fn(recentFiles.GetHistoryFile(event.GetId() - recentFiles.GetBaseId()));
	frame->LoadMap(fn);
}

void MainMenuBar::OnOpen(wxCommandEvent& WXUNUSED(event)) {
	g_gui.OpenMap();
}

void MainMenuBar::OnClose(wxCommandEvent& WXUNUSED(event)) {
	frame->DoQuerySave(true); // It closes the editor too
}

void MainMenuBar::OnSave(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMap();
}

void MainMenuBar::OnSaveAs(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SaveMapAs();
}

void MainMenuBar::OnPreferences(wxCommandEvent& WXUNUSED(event)) {
	PreferencesWindow dialog(frame);
	dialog.ShowModal();
	dialog.Destroy();
}

void MainMenuBar::OnQuit(wxCommandEvent& WXUNUSED(event)) {
	/*
	while(g_gui.IsEditorOpen())
		if(!frame->DoQuerySave(true))
			return;
			*/
	//((Application*)wxTheApp)->Unload();
	g_gui.root->Close();
}

void MainMenuBar::OnImportMap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.GetCurrentEditor());
	wxDialog* importmap = newd ImportMapWindow(frame, *g_gui.GetCurrentEditor());
	importmap->ShowModal();
}

void MainMenuBar::OnImportMonsterData(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dlg(g_gui.root, "Import monster/npc file", "", "", "*.xml", wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		wxArrayString paths;
		dlg.GetPaths(paths);
		for (uint32_t i = 0; i < paths.GetCount(); ++i) {
			wxString error;
			wxArrayString warnings;
			bool ok = g_creatures.importXMLFromOT(FileName(paths[i]), error, warnings);
			if (ok) {
				g_gui.ListDialog("Monster loader errors", warnings);
			} else {
				wxMessageBox("Error OT data file \"" + paths[i] + "\".\n" + error, "Error", wxOK | wxICON_INFORMATION, g_gui.root);
			}
		}
	}
}

void MainMenuBar::OnImportMinimap(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(g_gui.IsEditorOpen());
	// wxDialog* importmap = newd ImportMapWindow();
	// importmap->ShowModal();
}

void MainMenuBar::OnExportMinimap(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		ExportMiniMapWindow dlg(frame, *g_gui.GetCurrentEditor());
		dlg.ShowModal();
		dlg.Destroy();
	}
}

void MainMenuBar::OnExportTilesets(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		ExportTilesetsWindow dlg(frame, *g_gui.GetCurrentEditor());
		dlg.ShowModal();
		dlg.Destroy();
	}
}

void MainMenuBar::OnDebugViewDat(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg(frame, wxID_ANY, "Debug .dat file", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	new DatDebugView(&dlg);
	dlg.ShowModal();
}

void MainMenuBar::OnReloadDataFiles(wxCommandEvent& WXUNUSED(event)) {
	wxString error;
	wxArrayString warnings;
	g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true);
	g_gui.PopupDialog("Error", error, wxOK);
	g_gui.ListDialog("Warnings", warnings);
}

void MainMenuBar::OnReloadRevScripts(wxCommandEvent& WXUNUSED(event)) {
	g_gui.ReloadRevScripts();
}

void MainMenuBar::OnListExtensions(wxCommandEvent& WXUNUSED(event)) {
	ExtensionsDialog exts(frame);
	exts.ShowModal();
}

void MainMenuBar::OnGotoWebsite(wxCommandEvent& WXUNUSED(event)) {
	::wxLaunchDefaultBrowser(__SITE_URL__, wxBROWSER_NEW_WINDOW);
}

void MainMenuBar::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	AboutWindow about(frame);
	about.ShowModal();
}

void MainMenuBar::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoUndo();
}

void MainMenuBar::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoRedo();
}

namespace OnSearchForItem {
	struct Finder {
		Finder(uint16_t itemId, uint32_t maxCount) :
			itemId(itemId), maxCount(maxCount) { }

		uint16_t itemId;
		uint32_t maxCount;
		std::vector<std::pair<Tile*, Item*>> result;

		bool limitReached() const {
			return result.size() >= (size_t)maxCount;
		}

		void operator()(Map& map, Tile* tile, Item* item, long long done) {
			if (result.size() >= (size_t)maxCount) {
				return;
			}

			if (done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			if (item->getID() == itemId) {
				result.push_back(std::make_pair(tile, item));
			}
		}
	};

	struct RangeFinder {
		RangeFinder(const std::vector<std::pair<uint16_t, uint16_t>>& ranges, 
				   const std::vector<uint16_t>& ignored_ids = {},
				   const std::vector<std::pair<uint16_t, uint16_t>>& ignored_ranges = {}) : 
			ranges(ranges), 
			ignored_ids(ignored_ids),
			ignored_ranges(ignored_ranges),
			maxCount((uint32_t)g_settings.getInteger(Config::REPLACE_SIZE)) { }
			
		std::vector<std::pair<uint16_t, uint16_t>> ranges;
		std::vector<uint16_t> ignored_ids;
		std::vector<std::pair<uint16_t, uint16_t>> ignored_ranges;
		uint32_t maxCount;
		std::vector<std::pair<Tile*, Item*>> result;
		
		bool limitReached() const {
			return maxCount > 0 && result.size() >= size_t(maxCount);
		}
		
		void operator()(Map& map, Tile* tile, Item* item, long long done) {
			if(limitReached()) return;
			
			if(done % 0x8000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}
			
			uint16_t itemId = item->getID();
			
			// Check if item should be ignored
			for(const auto& id : ignored_ids) {
				if(itemId == id) return;
			}
			
			for(const auto& range : ignored_ranges) {
				if(itemId >= range.first && itemId <= range.second) return;
			}
			
			// Check if item is in search ranges
			for(const auto& range : ranges) {
				if(itemId >= range.first && itemId <= range.second) {
					result.push_back(std::make_pair(tile, item));
					break;
				}
			}
		}
	};
}

void MainMenuBar::OnSearchForItem(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    FindItemDialog dialog(frame, "Search for Item");
    dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
    if (dialog.ShowModal() == wxID_OK) {
        if (dialog.getUseRange()) {
            auto ranges = dialog.ParseRangeString(dialog.GetRangeInput());
            if (!ranges.empty()) {
                // Parse ignored IDs if enabled
                std::vector<uint16_t> ignored_ids;
                std::vector<std::pair<uint16_t, uint16_t>> ignored_ranges;
                if(dialog.IsIgnoreIdsEnabled()) {
                    ignored_ids = dialog.GetIgnoredIds();
                    ignored_ranges = dialog.ParseRangeString(dialog.GetIgnoreIdsText());
                }
                
                OnSearchForItem::RangeFinder finder(ranges, ignored_ids, ignored_ranges);
                g_gui.CreateLoadBar("Searching map...");
                
                foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
                std::vector<std::pair<Tile*, Item*>>& result = finder.result;
                
                g_gui.DestroyLoadBar();
                
                if (finder.limitReached()) {
                    wxString msg;
                    msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
                    g_gui.PopupDialog("Notice", msg, wxOK);
                }
                
                SearchResultWindow* resultWindow = g_gui.ShowSearchWindow();
                resultWindow->Clear();
                for (const auto& pair : result) {
                    resultWindow->AddPosition(wxString::Format("%s (ID: %d)", 
                        wxstr(pair.second->getName()),
                        pair.second->getID()), pair.first->getPosition());
                }
            }
        } else {
            OnSearchForItem::Finder finder(dialog.getResultID(), (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
            g_gui.CreateLoadBar("Searching map...");

            foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
            std::vector<std::pair<Tile*, Item*>>& result = finder.result;

            g_gui.DestroyLoadBar();

            if (finder.limitReached()) {
                wxString msg;
                msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
                g_gui.PopupDialog("Notice", msg, wxOK);
            }

            SearchResultWindow* window = g_gui.ShowSearchWindow();
            window->Clear();

            for (std::vector<std::pair<Tile*, Item*>>::const_iterator iter = result.begin(); iter != result.end(); ++iter) {
                Tile* tile = iter->first;
                Item* item = iter->second;
                
                // Format description to include both name and ID
                wxString description = wxString::Format("%s (ID: %d)", 
                    wxstr(item->getName()),
                    item->getID());
                
                OutputDebugStringA(wxString::Format("Adding search result: %s at pos(%d,%d,%d)\n", 
                    description, tile->getPosition().x, tile->getPosition().y, tile->getPosition().z).c_str());
                
                window->AddPosition(description, tile->getPosition());
            }
        }

        g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
    }
    dialog.Destroy();
}

void MainMenuBar::OnSearchForCreature(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    FindCreatureDialog dialog(frame, "Search for Creature");
    if (dialog.ShowModal() == wxID_OK) {
        wxString creatureName = dialog.getResultName();
        if (!creatureName.IsEmpty()) {
            Editor* editor = g_gui.GetCurrentEditor();
            if (editor) {
                Map& map = editor->getMap();
                SearchResultWindow* resultWindow = g_gui.ShowSearchWindow();
                resultWindow->Clear();

                g_gui.CreateLoadBar("Searching for creatures...");

                size_t creatureFoundCount = 0;
                const std::string creatureNameStr = creatureName.ToStdString();

                // Get the spawn XML data
                pugi::xml_document doc;
                pugi::xml_parse_result result = doc.load_file(map.getSpawnFilename().c_str());
                
                if (result) {
                    pugi::xml_node spawnsNode = doc.child("spawns");
                    if (spawnsNode) {
                        // Iterate through all spawns
                        for (pugi::xml_node spawnNode = spawnsNode.first_child(); spawnNode; spawnNode = spawnNode.next_sibling()) {
                            if (as_lower_str(spawnNode.name()) != "spawn") {
                                continue;
                            }

                            Position spawnPos;
                            spawnPos.x = spawnNode.attribute("centerx").as_int();
                            spawnPos.y = spawnNode.attribute("centery").as_int();
                            spawnPos.z = spawnNode.attribute("centerz").as_int();

                            // Check each creature in the spawn
                            for (pugi::xml_node creatureNode = spawnNode.first_child(); creatureNode; creatureNode = creatureNode.next_sibling()) {
                                const std::string& creatureNodeName = as_lower_str(creatureNode.name());
                                if (creatureNodeName != "monster" && creatureNodeName != "npc") {
                                    continue;
                                }

                                const std::string& name = creatureNode.attribute("name").as_string();
                                if (name == creatureNameStr) {
                                    // Calculate the actual position of the creature
                                    Position creaturePos = spawnPos;
                                    creaturePos.x += creatureNode.attribute("x").as_int();
                                    creaturePos.y += creatureNode.attribute("y").as_int();

                                    wxString description = wxString::Format("%s at (%d,%d,%d)", 
                                        creatureName,
                                        creaturePos.x,
                                        creaturePos.y,
                                        creaturePos.z);
                                    resultWindow->AddPosition(description, creaturePos);
                                    ++creatureFoundCount;
                                }
                            }
                        }
                    }
                }

                // Also check for loose creatures (not in spawns)
                for (MapIterator mit = map.begin(); mit != map.end(); ++mit) {
                    Tile* tile = (*mit)->get();
                    if (!tile) continue;

                    if (tile->creature && !tile->spawn) { // Only check tiles with creatures that aren't part of a spawn
                        if (tile->creature->getName() == creatureNameStr) {
                            wxString description = wxString::Format("%s (loose) at (%d,%d,%d)", 
                                creatureName,
                                tile->getPosition().x,
                                tile->getPosition().y,
                                tile->getPosition().z);
                            resultWindow->AddPosition(description, tile->getPosition());
                            ++creatureFoundCount;
                        }
                    }
                }

                g_gui.DestroyLoadBar();

                wxString resultMessage;
                if (creatureFoundCount == 0) {
                    resultMessage = wxString::Format("No %s found on the map.", creatureName);
                    g_gui.PopupDialog("Search completed", resultMessage, wxOK);
                } else {
                    resultMessage = wxString::Format("Found %d instances of %s on the map.", 
                        creatureFoundCount, creatureName);
                    g_gui.SetStatusText(resultMessage);
                }
            }
        }
    }
    dialog.Destroy();
}

void MainMenuBar::OnReplaceItems(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	if (MapTab* tab = g_gui.GetCurrentMapTab()) {
		if (MapWindow* window = tab->GetView()) {
			window->ShowReplaceItemsDialog(false);
		}
	}
}

namespace OnSearchForStuff {
    struct Searcher {
        Searcher() :
			search_zones(false),
            search_unique(false),
            search_action(false),
            search_container(false),
            search_writeable(false) { }

		bool search_zones;
        bool search_unique;
        bool search_action;
        bool search_container;
        bool search_writeable;
        std::vector<std::pair<uint16_t, uint16_t>> uniqueRanges;
        std::vector<std::pair<uint16_t, uint16_t>> actionRanges;
        std::vector<std::pair<Tile*, Item*>> found;

        bool isInRanges(uint16_t id, const std::vector<std::pair<uint16_t, uint16_t>>& ranges) {
            if (ranges.empty()) return true;
            for (const auto& range : ranges) {
                if (id >= range.first && id <= range.second) {
                    return true;
                }
            }
            return false;
        }

        void operator()(Map& map, Tile* tile, Item* item, long long done) {
            if (done % 0x8000 == 0) {
                g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
            }
            Container* container;
            bool shouldAdd = false;

			if (search_zones && item->isGroundTile() && !tile->getZoneIds().empty()) {
				shouldAdd = true;
			}

            if (search_unique && item->getUniqueID() > 0 && isInRanges(item->getUniqueID(), uniqueRanges)) {
                shouldAdd = true;
            }
            if (search_action && item->getActionID() > 0 && isInRanges(item->getActionID(), actionRanges)) {
                shouldAdd = true;
            }
            if (search_container && ((container = dynamic_cast<Container*>(item)) && container->getItemCount())) {
                shouldAdd = true;
            }
            if (search_writeable && item->getText().length() > 0) {
                shouldAdd = true;
            }

            if (shouldAdd) {
                found.push_back(std::make_pair(tile, item));
            }
        }

        wxString desc(Tile* tile, Item* item) {
			wxString label;
			if (search_zones) {
				label << "Zone ID: ";
				size_t zones = tile->getZoneIds().size();
				for (const auto& zoneId : tile->getZoneIds()) {
					label << zoneId;
					if (--zones > 0) {
						label << "/";
					}
				}
			} else {
				if (item->getActionID() > 0) {
					label << "AID:" << item->getActionID() << " ";
				}

				if (item->getUniqueID() > 0) {
					label << "UID:" << item->getUniqueID() << " ";
				}

				label << wxstr(item->getName());

				if (dynamic_cast<Container*>(item)) {
					label << " (Container) ";
				}

				if (item->getText().length() > 0) {
					label << " (Text: " << wxstr(item->getText()) << ") ";
				}
			}

            return label;
        }

        void sort() {
            if (search_unique || search_action) {
                std::sort(found.begin(), found.end(), Searcher::compare);
            } else if (search_zones) {
				std::sort(found.begin(), found.end(), Searcher::compareZones);
			}
        }

        static bool compare(const std::pair<Tile*, Item*>& pair1, const std::pair<Tile*, Item*>& pair2) {
            const Item* item1 = pair1.second;
            const Item* item2 = pair2.second;

            if (item1->getActionID() != 0 || item2->getActionID() != 0) {
                return item1->getActionID() < item2->getActionID();
            } else if (item1->getUniqueID() != 0 || item2->getUniqueID() != 0) {
                return item1->getUniqueID() < item2->getUniqueID();
            }

            return false;
        }

		static bool compareZones(const std::pair<Tile*, Item*>& pair1, const std::pair<Tile*, Item*>& pair2) {
			return pair1.first->getZoneId() < pair2.first->getZoneId();
		}
    };
}

void MainMenuBar::OnSearchForStuffOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(true, true, true, true, false);
}

void MainMenuBar::OnSearchForZonesOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, false, false, true);
}

void MainMenuBar::OnSearchForUniqueOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(true, false, false, false, false);
}

void MainMenuBar::OnSearchForActionOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, true, false, false, false);
}

void MainMenuBar::OnSearchForContainerOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, true, false, false);
}

void MainMenuBar::OnSearchForWriteableOnMap(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, false, true, false);
}

void MainMenuBar::OnSearchForStuffOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(true, true, true, true, false, true);
}

void MainMenuBar::OnSearchForZonesOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, false, false, true, true);
}

void MainMenuBar::OnSearchForUniqueOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(true, false, false, false, false, true);
}

void MainMenuBar::OnSearchForActionOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, true, false, false, false, true);
}

void MainMenuBar::OnSearchForContainerOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, true, false, false, true);
}

void MainMenuBar::OnSearchForWriteableOnSelection(wxCommandEvent& WXUNUSED(event)) {
	SearchItems(false, false, false, true, false, true);
}

void MainMenuBar::OnSearchForItemOnSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Search on Selection");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
	if (dialog.ShowModal() == wxID_OK) {
		if (dialog.getUseRange()) {
			auto ranges = dialog.ParseRangeString(dialog.GetRangeInput());
			if (!ranges.empty()) {
				OnSearchForItem::RangeFinder finder(ranges);
				g_gui.CreateLoadBar("Searching on selected area...");
				
				foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, true);
				std::vector<std::pair<Tile*, Item*>>& result = finder.result;
				
				g_gui.DestroyLoadBar();
				
				if (finder.limitReached()) {
					wxString msg;
					msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
					g_gui.PopupDialog("Notice", msg, wxOK);
				}
				
				SearchResultWindow* resultWindow = g_gui.ShowSearchWindow();
				resultWindow->Clear();
				
				// Pass the ignored IDs configuration from the dialog
				resultWindow->SetIgnoredIds(dialog.GetIgnoreIdsText(), dialog.IsIgnoreIdsEnabled());
				
				// Store search parameters for range searches to enable continuation
				uint16_t firstItemId = result.empty() ? 0 : result[0].second->getID();
				resultWindow->StoreSearchInfo(firstItemId, true);

				for (const auto& pair : result) {
					resultWindow->AddPosition(wxString::Format("%s (ID: %d)", 
						wxstr(pair.second->getName()),
						pair.second->getID()), pair.first->getPosition());
				}
			}
		} else {
			OnSearchForItem::Finder finder(dialog.getResultID(), (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
			g_gui.CreateLoadBar("Searching on selected area...");

			foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, true);
			std::vector<std::pair<Tile*, Item*>>& result = finder.result;

			g_gui.DestroyLoadBar();

			if (finder.limitReached()) {
				wxString msg;
				msg << "The configured limit has been reached. Only " << finder.maxCount << " results will be displayed.";
				g_gui.PopupDialog("Notice", msg, wxOK);
			}

			SearchResultWindow* window = g_gui.ShowSearchWindow();
			window->Clear();

			// Pass the ignored IDs configuration from the dialog
			window->SetIgnoredIds(dialog.GetIgnoreIdsText(), dialog.IsIgnoreIdsEnabled());
            
            // Store search parameters for continuation
            window->StoreSearchInfo(dialog.getResultID(), true);

			for (std::vector<std::pair<Tile*, Item*>>::const_iterator iter = result.begin(); iter != result.end(); ++iter) {
				Tile* tile = iter->first;
				Item* item = iter->second;
				
				// Format description to include both name and ID
				wxString description = wxString::Format("%s (ID: %d)", 
					wxstr(item->getName()),
					item->getID());
				
				OutputDebugStringA(wxString::Format("Adding search result: %s at pos(%d,%d,%d)\n", 
					description, tile->getPosition().x, tile->getPosition().y, tile->getPosition().z).c_str());
				
				window->AddPosition(description, tile->getPosition());
			}
		}

		g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
	}

	dialog.Destroy();
}

void MainMenuBar::OnReplaceItemsOnSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	if (MapTab* tab = g_gui.GetCurrentMapTab()) {
		if (MapWindow* window = tab->GetView()) {
			window->ShowReplaceItemsDialog(true);
		}
	}
}

void MainMenuBar::OnRemoveItemOnSelection(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    FindItemDialog dialog(frame, "Remove Items on Selection");
    dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
    
    if (dialog.ShowModal() == wxID_OK) {
        g_gui.GetCurrentEditor()->actionQueue->clear();
        g_gui.CreateLoadBar("Searching items on selection to remove...");
        
        int64_t count = 0;
        
        if (dialog.getUseRange()) {
            auto ranges = dialog.ParseRangeString(dialog.GetRangeInput());
            if (!ranges.empty()) {
                // Create a condition that checks if an item's ID is within any of the ranges
                struct RangeRemoveCondition {
                    std::vector<std::pair<uint16_t, uint16_t>> ranges;
                    
                    RangeRemoveCondition(const std::vector<std::pair<uint16_t, uint16_t>>& r) : ranges(r) {}
                    
                    bool operator()(Map& map, Item* item, long long removed, long long done) {
                        if (done % 0x800 == 0) {
                            g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
                        }
                        
                        for (const auto& range : ranges) {
                            if (item->getID() >= range.first && item->getID() <= range.second) {
                                return true;
                            }
                        }
                        return false;
                    }
                } condition(ranges);
                
                count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, true);
            }
        } else {
            OnMapRemoveItems::RemoveItemCondition condition(dialog.getResultID());
            count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, true);
        }
        
        g_gui.DestroyLoadBar();

        wxString msg;
        msg << count << " items removed.";
        g_gui.PopupDialog("Remove Items", msg, wxOK);
        g_gui.GetCurrentMap().doChange();
        g_gui.RefreshView();
    }
    dialog.Destroy();
}

void MainMenuBar::OnSelectionTypeChange(wxCommandEvent& WXUNUSED(event)) {
	g_settings.setInteger(Config::COMPENSATED_SELECT, IsItemChecked(MenuBar::SELECT_MODE_COMPENSATE));

	if (IsItemChecked(MenuBar::SELECT_MODE_CURRENT)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_LOWER)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_ALL_FLOORS);
	} else if (IsItemChecked(MenuBar::SELECT_MODE_VISIBLE)) {
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_VISIBLE_FLOORS);
	}
}

void MainMenuBar::OnCopy(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoCopy();
}

void MainMenuBar::OnCut(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoCut();
}

void MainMenuBar::OnPaste(wxCommandEvent& WXUNUSED(event)) {
	g_gui.PreparePaste();
}


void MainMenuBar::OnBorderizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->borderizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnBorderizeMap(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    int ret = g_gui.PopupDialog("Borderize Map", 
        "Do you want to borderize the entire map? This will process the map in chunks.", wxYES | wxNO);
    if (ret == wxID_YES) {
        g_gui.GetCurrentEditor()->borderizeMap(true);
    }

    g_gui.RefreshView();
}

void MainMenuBar::OnWallizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->wallizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnWallizeMap(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    int ret = g_gui.PopupDialog("Wallize Map", 
        "Do you want to wallize the entire map? This will process the map in chunks.", wxYES | wxNO);
    if (ret == wxID_YES) {
        g_gui.GetCurrentEditor()->wallizeMap(true);
    }

    g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeSelection(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.GetCurrentEditor()->randomizeSelection();
	g_gui.RefreshView();
}

void MainMenuBar::OnRandomizeMap(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ret = g_gui.PopupDialog("Randomize Map", "Are you sure you want to randomize the entire map (this action cannot be undone)?", wxYES | wxNO);
	if (ret == wxID_YES) {
		g_gui.GetCurrentEditor()->randomizeMap(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnJumpToBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindDialog* dlg = newd FindBrushDialog(frame);

	// Display dialog to user
	dlg->ShowModal();

	// Retrieve result, if null user canceled
	const Brush* brush = dlg->getResult();
	if (brush) {
		g_gui.SelectBrush(brush, TILESET_UNKNOWN);
	}
	delete dlg;
}

void MainMenuBar::OnJumpToItemBrush(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsVersionLoaded()) {
		return;
	}

	// Create the jump to dialog
	FindItemDialog dialog(frame, "Jump to Item");
	dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::JUMP_TO_ITEM_MODE));
	if (dialog.ShowModal() == wxID_OK) {
		// Retrieve result, if null user canceled
		const Brush* brush = dialog.getResult();
		if (brush) {
			g_gui.SelectBrush(brush, TILESET_RAW);
		}
		g_settings.setInteger(Config::JUMP_TO_ITEM_MODE, (int)dialog.getSearchMode());
	}
	dialog.Destroy();
}

void MainMenuBar::OnGotoPreviousPosition(wxCommandEvent& WXUNUSED(event)) {
	MapTab* mapTab = g_gui.GetCurrentMapTab();
	if (mapTab) {
		mapTab->GoToPreviousCenterPosition();
	}
}

void MainMenuBar::OnGotoPosition(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Display dialog, it also controls the actual jump
	GotoPositionDialog dlg(frame, *g_gui.GetCurrentEditor());
	dlg.ShowModal();
}

void MainMenuBar::OnMapRemoveItems(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	FindItemDialog dialog(frame, "Item Type to Remove");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t itemid = dialog.getResultID();

		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		OnMapRemoveItems::RemoveItemCondition condition(itemid);
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), condition, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";

		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
		g_gui.RefreshView();
	}
	dialog.Destroy();
}

namespace OnMapRemoveCorpses {
	struct condition {
		condition() { }

		bool operator()(Map& map, Item* item, long long removed, long long done) {
			if (done % 0x800 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
			}

			return g_materials.isInTileset(item, "Corpses") & !item->isComplex();
		}
	};
}

void MainMenuBar::OnMapRemoveCorpses(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	int ok = g_gui.PopupDialog("Remove Corpses", "Do you want to remove all corpses from the map?", wxYES | wxNO);

	if (ok == wxID_YES) {
		g_gui.GetCurrentEditor()->selection.clear();
		g_gui.GetCurrentEditor()->actionQueue->clear();

		OnMapRemoveCorpses::condition func;
		g_gui.CreateLoadBar("Searching map for items to remove...");

		int64_t count = RemoveItemOnMap(g_gui.GetCurrentMap(), func, false);

		g_gui.DestroyLoadBar();

		wxString msg;
		msg << count << " items deleted.";
		g_gui.PopupDialog("Search completed", msg, wxOK);
		g_gui.GetCurrentMap().doChange();
	}
}

namespace OnMapRemoveUnreachable {
	struct condition {
		condition() { }

		bool isReachable(Tile* tile) {
			if (tile == nullptr) {
				return false;
			}
			if (!tile->isBlocking()) {
				return true;
			}
			return false;
		}

		bool operator()(Map& map, Tile* tile, long long removed, long long done, long long total) {
			if (done % 0x1000 == 0) {
				g_gui.SetLoadDone((unsigned int)(100 * done / total));
			}

			Position pos = tile->getPosition();
			int sx = std::max(pos.x - 10, 0);
			int ex = std::min(pos.x + 10, 65535);
			int sy = std::max(pos.y - 8, 0);
			int ey = std::min(pos.y + 8, 65535);
			int sz, ez;

			if (pos.z <= GROUND_LAYER) {
				sz = 0;
				ez = 9;
	} else {
				// underground
				sz = std::max(pos.z - 2, GROUND_LAYER);
				ez = std::min(pos.z + 2, MAP_MAX_LAYER);
			}

			for (int z = sz; z <= ez; ++z) {
				for (int y = sy; y <= ey; ++y) {
					for (int x = sx; x <= ex; ++x) {
						if (isReachable(map.getTile(x, y, z))) {
							return false;
						}
					}
				}
			}
			return true;
		}
	};
}

void MainMenuBar::OnMapRemoveUnreachable(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    // Create custom dialog
    wxDialog* dialog = new wxDialog(frame, wxID_ANY, "Remove Unreachable Tiles", 
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* gridSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Create spin controls for X and Y ranges
    wxStaticText* xLabel = new wxStaticText(dialog, wxID_ANY, "X Range:");
    wxSpinCtrl* xRange = new wxSpinCtrl(dialog, wxID_ANY, "10", 
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 10);
    
    wxStaticText* yLabel = new wxStaticText(dialog, wxID_ANY, "Y Range:");
    wxSpinCtrl* yRange = new wxSpinCtrl(dialog, wxID_ANY, "8", 
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 8);
    
    gridSizer->Add(xLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gridSizer->Add(xRange, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gridSizer->Add(yLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    gridSizer->Add(yRange, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    mainSizer->Add(gridSizer, 0, wxALL | wxALIGN_CENTER, 5);
    
    // Add warning text
    wxStaticText* warning = new wxStaticText(dialog, wxID_ANY, 
        "Warning: This operation will remove all tiles that are not\n"
        "reachable within the specified X and Y ranges.");
    mainSizer->Add(warning, 0, wxALL | wxALIGN_CENTER, 10);
    
    // Add buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    dialog->SetSizer(mainSizer);
    mainSizer->Fit(dialog);
    dialog->Center();

    // Show dialog and process result
    if (dialog->ShowModal() == wxID_OK) {
        // Create modified condition with custom ranges
        struct CustomRangeCondition : public OnMapRemoveUnreachable::condition {
            int xRange;
            int yRange;
            
            CustomRangeCondition(int x, int y) : xRange(x), yRange(y) {}
            
            bool operator()(Map& map, Tile* tile, long long removed, long long done, long long total) {
                if (done % 0x1000 == 0) {
                    g_gui.SetLoadDone((unsigned int)(100 * done / total));
                }

                Position pos = tile->getPosition();
                int sx = std::max(pos.x - xRange, 0);
                int ex = std::min(pos.x + xRange, 65535);
                int sy = std::max(pos.y - yRange, 0);
                int ey = std::min(pos.y + yRange, 65535);
                int sz, ez;

                if (pos.z <= GROUND_LAYER) {
                    sz = 0;
                    ez = 9;
                } else {
                    sz = std::max(pos.z - 2, GROUND_LAYER);
                    ez = std::min(pos.z + 2, MAP_MAX_LAYER);
                }

                for (int z = sz; z <= ez; ++z) {
                    for (int y = sy; y <= ey; ++y) {
                        for (int x = sx; x <= ex; ++x) {
                            if (isReachable(map.getTile(x, y, z))) {
                                return false;
                            }
                        }
                    }
                }
                return true;
            }
        };

        g_gui.GetCurrentEditor()->selection.clear();
        g_gui.GetCurrentEditor()->actionQueue->clear();

        CustomRangeCondition func(xRange->GetValue(), yRange->GetValue());
        g_gui.CreateLoadBar("Searching map for tiles to remove...");

        long long removed = remove_if_TileOnMap(g_gui.GetCurrentMap(), func);

        g_gui.DestroyLoadBar();

        wxString msg;
        msg << removed << " tiles deleted.";
        g_gui.PopupDialog("Search completed", msg, wxOK);

        g_gui.GetCurrentMap().doChange();
    }
    
    dialog->Destroy();
}

void MainMenuBar::OnClearHouseTiles(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Invalid House Tiles",
		"Are you sure you want to remove all house tiles that do not belong to a house (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearInvalidHouseTiles(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnClearModifiedState(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Modified State",
		"This will have the same effect as closing the map and opening it again. Do you want to proceed?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		editor->clearModifiedTileState(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnMapCleanHouseItems(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}

	int ret = g_gui.PopupDialog(
		"Clear Moveable House Items",
		"Are you sure you want to remove all items inside houses that can be moved (this action cannot be undone)?",
		wxYES | wxNO
	);

	if (ret == wxID_YES) {
		// Editor will do the work
		// editor->removeHouseItems(true);
	}

	g_gui.RefreshView();
}

void MainMenuBar::OnMapEditTowns(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.GetCurrentEditor()) {
		wxDialog* town_dialog = newd EditTownsDialog(frame, *g_gui.GetCurrentEditor());
		town_dialog->ShowModal();
		town_dialog->Destroy();
	}
}

void MainMenuBar::OnMapEditItems(wxCommandEvent& WXUNUSED(event)) {
	;
}

void MainMenuBar::OnMapEditMonsters(wxCommandEvent& WXUNUSED(event)) {
	;
}

void MainMenuBar::OnMapStatistics(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	g_gui.CreateLoadBar("Collecting data...");

	Map* map = &g_gui.GetCurrentMap();

	int load_counter = 0;

	uint64_t tile_count = 0;
	uint64_t detailed_tile_count = 0;
	uint64_t blocking_tile_count = 0;
	uint64_t walkable_tile_count = 0;
	double percent_pathable = 0.0;
	double percent_detailed = 0.0;
	uint64_t spawn_count = 0;
	uint64_t creature_count = 0;
	double creatures_per_spawn = 0.0;

	uint64_t item_count = 0;
	uint64_t loose_item_count = 0;
	uint64_t depot_count = 0;
	uint64_t action_item_count = 0;
	uint64_t unique_item_count = 0;
	uint64_t container_count = 0; // Only includes containers containing more than 1 item

	int town_count = map->towns.count();
	int house_count = map->houses.count();
	std::map<uint32_t, uint32_t> town_sqm_count;
	const Town* largest_town = nullptr;
	uint64_t largest_town_size = 0;
	uint64_t total_house_sqm = 0;
	const House* largest_house = nullptr;
	uint64_t largest_house_size = 0;
	double houses_per_town = 0.0;
	double sqm_per_house = 0.0;
	double sqm_per_town = 0.0;

	for (MapIterator mit = map->begin(); mit != map->end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (load_counter % 8192 == 0) {
			g_gui.SetLoadDone((unsigned int)(int64_t(load_counter) * 95ll / int64_t(map->getTileCount())));
		}

		if (tile->empty()) {
			continue;
		}

		tile_count += 1;

		bool is_detailed = false;
#define ANALYZE_ITEM(_item)                                         \
	{                                                               \
		item_count += 1;                                            \
		if (!(_item)->isGroundTile() && !(_item)->isBorder()) {     \
			is_detailed = true;                                     \
			ItemType& it = g_items[(_item)->getID()];               \
			if (it.moveable) {                                      \
				loose_item_count += 1;                              \
			}                                                       \
			if (it.isDepot()) {                                     \
				depot_count += 1;                                   \
			}                                                       \
			if ((_item)->getActionID() > 0) {                       \
				action_item_count += 1;                             \
			}                                                       \
			if ((_item)->getUniqueID() > 0) {                       \
				unique_item_count += 1;                             \
			}                                                       \
			if (Container* c = dynamic_cast<Container*>((_item))) { \
				if (c->getVector().size()) {                        \
					container_count += 1;                           \
				}                                                   \
			}                                                       \
		}                                                           \
	}

		if (tile->ground) {
			ANALYZE_ITEM(tile->ground);
		}

		for (ItemVector::const_iterator item_iter = tile->items.begin(); item_iter != tile->items.end(); ++item_iter) {
			Item* item = *item_iter;
			ANALYZE_ITEM(item);
		}
#undef ANALYZE_ITEM

		if (tile->spawn) {
			spawn_count += 1;
		}

		if (tile->creature) {
			creature_count += 1;
		}

		if (tile->isBlocking()) {
			blocking_tile_count += 1;
		} else {
			walkable_tile_count += 1;
		}

		if (is_detailed) {
			detailed_tile_count += 1;
		}

		load_counter += 1;
	}

	creatures_per_spawn = (spawn_count != 0 ? double(creature_count) / double(spawn_count) : -1.0);
	percent_pathable = 100.0 * (tile_count != 0 ? double(walkable_tile_count) / double(tile_count) : -1.0);
	percent_detailed = 100.0 * (tile_count != 0 ? double(detailed_tile_count) / double(tile_count) : -1.0);

	load_counter = 0;
	Houses& houses = map->houses;
	for (HouseMap::const_iterator hit = houses.begin(); hit != houses.end(); ++hit) {
		const House* house = hit->second;

		if (load_counter % 64) {
			g_gui.SetLoadDone((unsigned int)(95ll + int64_t(load_counter) * 5ll / int64_t(house_count)));
		}

		if (house->size() > largest_house_size) {
			largest_house = house;
			largest_house_size = house->size();
		}
		total_house_sqm += house->size();
		town_sqm_count[house->townid] += house->size();
	}

	houses_per_town = (town_count != 0 ? double(house_count) / double(town_count) : -1.0);
	sqm_per_house = (house_count != 0 ? double(total_house_sqm) / double(house_count) : -1.0);
	sqm_per_town = (town_count != 0 ? double(total_house_sqm) / double(town_count) : -1.0);

	Towns& towns = map->towns;
	for (std::map<uint32_t, uint32_t>::iterator town_iter = town_sqm_count.begin();
		 town_iter != town_sqm_count.end();
		 ++town_iter) {
		// No load bar for this, load is non-existant
		uint32_t town_id = town_iter->first;
		uint32_t town_sqm = town_iter->second;
		Town* town = towns.getTown(town_id);
		if (town && town_sqm > largest_town_size) {
			largest_town = town;
			largest_town_size = town_sqm;
		} else {
			// Non-existant town!
		}
	}

	g_gui.DestroyLoadBar();

	std::ostringstream os;
	os.setf(std::ios::fixed, std::ios::floatfield);
	os.precision(2);
	os << "Map statistics for the map \"" << map->getMapDescription() << "\"\n";

	// Add map dimensions information
	os << "\tMap dimensions:\n";
	os << "\t\tWidth: " << map->getWidth() << " tiles\n";
	os << "\t\tHeight: " << map->getHeight() << " tiles\n";
	os << "\t\tTotal area: " << (map->getWidth() * map->getHeight()) << " square tiles\n";
	os << "\t\tNumber of floors: " << (MAP_MAX_LAYER + 1) << "\n";

	os << "\tTile data:\n";
	os << "\t\tTotal number of tiles: " << tile_count << "\n";
	os << "\t\tNumber of pathable tiles: " << walkable_tile_count << "\n";
	os << "\t\tNumber of unpathable tiles: " << blocking_tile_count << "\n";
	if (percent_pathable >= 0.0) {
		os << "\t\tPercent walkable tiles: " << percent_pathable << "%\n";
	}
	os << "\t\tDetailed tiles: " << detailed_tile_count << "\n";
	if (percent_detailed >= 0.0) {
		os << "\t\tPercent detailed tiles: " << percent_detailed << "%\n";
	}

	os << "\tItem data:\n";
	os << "\t\tTotal number of items: " << item_count << "\n";
	os << "\t\tNumber of moveable tiles: " << loose_item_count << "\n";
	os << "\t\tNumber of depots: " << depot_count << "\n";
	os << "\t\tNumber of containers: " << container_count << "\n";
	os << "\t\tNumber of items with Action ID: " << action_item_count << "\n";
	os << "\t\tNumber of items with Unique ID: " << unique_item_count << "\n";
	os << "\t\tItems per tile ratio: " << (tile_count > 0 ? (double)item_count / tile_count : 0) << "\n";

	os << "\tCreature data:\n";
	os << "\t\tTotal creature count: " << creature_count << "\n";
	os << "\t\tTotal spawn count: " << spawn_count << "\n";
	if (creatures_per_spawn >= 0) {
		os << "\t\tMean creatures per spawn: " << creatures_per_spawn << "\n";
	}
	os << "\t\tCreature density: " << (tile_count > 0 ? (double)creature_count / tile_count * 100 : 0) << "% of tiles\n";

	os << "\tTown/House data:\n";
	os << "\t\tTotal number of towns: " << town_count << "\n";
	os << "\t\tTotal number of houses: " << house_count << "\n";
	if (houses_per_town >= 0) {
		os << "\t\tMean houses per town: " << houses_per_town << "\n";
	}
	os << "\t\tTotal amount of housetiles: " << total_house_sqm << "\n";
	if (sqm_per_house >= 0) {
		os << "\t\tMean tiles per house: " << sqm_per_house << "\n";
	}
	if (sqm_per_town >= 0) {
		os << "\t\tMean tiles per town: " << sqm_per_town << "\n";
	}
	os << "\t\tPercentage of map covered by houses: " << (tile_count > 0 ? (double)total_house_sqm / tile_count * 100 : 0) << "%\n";
	/*
	// Add waypoint statistics
	int waypoint_count = map->waypoints.getWaypointCount();
	os << "\tWaypoint data:\n";
	os << "\t\tTotal number of waypoints: " << waypoint_count << "\n";

	// Add zone statistics if available
	os << "\tZone data:\n";
	os << "\t\tTotal number of zones: " << map->zones.size() << "\n";
	*/
	if (largest_town) {
		os << "\t\tLargest Town: \"" << largest_town->getName() << "\" (" << largest_town_size << " sqm)\n";
	}
	if (largest_house) {
		os << "\t\tLargest House: \"" << largest_house->name << "\" (" << largest_house_size << " sqm)\n";
	}

	// Add map file information
	os << "\tMap file information:\n";
	os << "\t\tOTBM version: " << map->getVersion().otbm << "\n";
	os << "\t\tClient version: " << map->getVersion().client << "\n";
	os << "\t\tFile size (approximate): " << (map->getTileCount() * 512 / 1024) << " KB\n";

	os << "\n";
	os << "Generated by Remere's Map Editor version OTARMEIE " + __RME_VERSION__ + "\n";

	wxDialog* dg = newd wxDialog(frame, wxID_ANY, "Map Statistics", wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dg, wxID_ANY, wxstr(os.str()), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 300));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	wxButton* export_button = newd wxButton(dg, wxID_OK, "Export as XML");
	choicesizer->Add(export_button, wxSizerFlags(1).Center());
	export_button->Enable(false);
	choicesizer->Add(newd wxButton(dg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(1).Center());
	dg->SetSizerAndFit(topsizer);
	dg->Centre(wxBOTH);

	int ret = dg->ShowModal();

	if (ret == wxID_OK) {
		// std::cout << "XML EXPORT";
	} else if (ret == wxID_CANCEL) {
		// std::cout << "OK";
	}
}

void MainMenuBar::OnMapCleanup(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    // Create custom cleanup dialog
    wxDialog* dialog = new wxDialog(frame, wxID_ANY, "Map Cleanup Options", 
        wxDefaultPosition, wxSize(600, 500), wxDEFAULT_DIALOG_STYLE);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create cleanup options
    wxStaticBoxSizer* optionsSizer = new wxStaticBoxSizer(wxVERTICAL, dialog, "Cleanup Options");
    
    // Invalid items checkbox
    wxCheckBox* cleanInvalid = new wxCheckBox(dialog, wxID_ANY, "Remove Invalid Items");
    optionsSizer->Add(cleanInvalid, 0, wxALL, 5);
    
    // Add monster cleanup option
    wxCheckBox* cleanMonsters = new wxCheckBox(dialog, wxID_ANY, "Remove Monsters in Blocking Tiles");
    optionsSizer->Add(cleanMonsters, 0, wxALL, 5);
    
    // Add empty spawn cleanup option
    wxCheckBox* cleanEmptySpawns = new wxCheckBox(dialog, wxID_ANY, "Remove Empty Spawns");
    optionsSizer->Add(cleanEmptySpawns, 0, wxALL, 5);
    
    // Add problematic items cleanup option
    wxCheckBox* cleanWheyItems = new wxCheckBox(dialog, wxID_ANY, "Remove Problematic Items (whey/invalid)");
    optionsSizer->Add(cleanWheyItems, 0, wxALL, 5);
    
    // ID Range cleanup section
    wxStaticBoxSizer* rangeSizer = new wxStaticBoxSizer(wxVERTICAL, dialog, "Clean Items by ID Range");
    
    // Enable ID range checkbox
    wxCheckBox* useRange = new wxCheckBox(dialog, wxID_ANY, "Clean Items by ID Range");
    rangeSizer->Add(useRange, 0, wxALL, 5);
    
    // ID range input
    wxTextCtrl* rangeInput = new wxTextCtrl(dialog, wxID_ANY);
    rangeInput->SetToolTip("Enter IDs or ranges separated by commas (e.g., 2222,2244-2266,5219)");
    rangeInput->Enable(false);
    rangeSizer->Add(rangeInput, 0, wxEXPAND | wxALL, 5);
    
    // Bind enable/disable of range input
    useRange->Bind(wxEVT_CHECKBOX, [rangeInput](wxCommandEvent& evt) {
        rangeInput->Enable(evt.IsChecked());
    });
    
    // Ignored IDs section
    wxStaticBoxSizer* ignoreSizer = new wxStaticBoxSizer(wxVERTICAL, dialog, "Ignored IDs");
    
    // Enable ignored IDs checkbox
    wxCheckBox* useIgnored = new wxCheckBox(dialog, wxID_ANY, "Use Ignored IDs");
    ignoreSizer->Add(useIgnored, 0, wxALL, 5);
    
    // Ignored IDs input
    wxTextCtrl* ignoreInput = new wxTextCtrl(dialog, wxID_ANY);
    ignoreInput->SetToolTip("Enter IDs to ignore, separated by commas. Use '-' for ranges (e.g., 1212,1241,1256-1261)");
    ignoreInput->Enable(false);
    ignoreSizer->Add(ignoreInput, 0, wxEXPAND | wxALL, 5);
    
    // Bind enable/disable of ignore input
    useIgnored->Bind(wxEVT_CHECKBOX, [ignoreInput](wxCommandEvent& evt) {
        ignoreInput->Enable(evt.IsChecked());
    });

    // Add options to main sizer
    mainSizer->Add(optionsSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(rangeSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(ignoreSizer, 0, wxEXPAND | wxALL, 5);

    // Add warning text
    wxStaticText* warning = new wxStaticText(dialog, wxID_ANY, 
        "Warning: This operation cannot be undone!\nPlease save your map before proceeding.");
    warning->SetForegroundColour(*wxRED);
    mainSizer->Add(warning, 0, wxALL | wxALIGN_CENTER, 10);

    // Add buttons
    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    wxButton* okButton = new wxButton(dialog, wxID_OK, "Clean");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->AddButton(okButton);
    buttonSizer->AddButton(cancelButton);
    buttonSizer->Realize();
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(mainSizer);
    mainSizer->Fit(dialog);
    dialog->Center();

    // Show dialog and process result
    if (dialog->ShowModal() == wxID_OK) {
        bool hasOptions = cleanInvalid->GetValue() || useRange->GetValue() || cleanMonsters->GetValue() || 
                         cleanEmptySpawns->GetValue() || cleanWheyItems->GetValue();
        if (!hasOptions) {
            g_gui.PopupDialog("Error", "Please select at least one cleanup option!", wxOK);
            dialog->Destroy();
            return;
        }

        int64_t totalCount = 0;
        Map& currentMap = g_gui.GetCurrentMap();

        g_gui.CreateLoadBar("Cleaning map...");

        try {
            int progressStep = 0;
            int numOptions = 0;
            if (cleanInvalid->GetValue()) numOptions++;
            if (useRange->GetValue()) numOptions++;
            if (cleanMonsters->GetValue()) numOptions++;
            if (cleanEmptySpawns->GetValue()) numOptions++;
            if (cleanWheyItems->GetValue()) numOptions++;
            int progressIncrement = numOptions > 0 ? 100 / numOptions : 0;
            
            // Process invalid items if selected
            if (cleanInvalid->GetValue()) {
                // Update loading bar message but keep the same bar
                g_gui.SetLoadDone(progressStep, "Removing invalid tiles...");
                currentMap.cleanInvalidTiles(true);
                progressStep += progressIncrement;
                g_gui.SetLoadDone(progressStep);
            }

            // Process monster cleanup if selected
            if (cleanMonsters->GetValue()) {
                // Define condition for monsters in blocking tiles
                struct RemoveMonsterCondition {
                    int removedCount = 0;
                    long long totalTiles;
                    int startProgress;
                    int endProgress;
                    
                    bool operator()(Map& map, Tile* tile, long long done, long long total) {
                        if (done % 1024 == 0) {
                            int progress = startProgress + ((done * (endProgress - startProgress)) / totalTiles);
                            g_gui.SetLoadDone(progress);
                        }
                        
                        // Check for monsters in invalid locations:
                        // 1. Blocking tiles
                        // 2. Tiles without ground
                        // 3. Empty tiles
                        if (tile->creature && (tile->isBlocking() || !tile->hasGround() || tile->empty())) {
                            // Monster is in an invalid location, remove it
                            delete tile->creature;
                            tile->creature = nullptr;
                            tile->modify(); // Mark as modified for saving
                            removedCount++;
                            return true;
                        }
                        return false;
                    }
                } monsterCondition;
                
                monsterCondition.totalTiles = currentMap.getTileCount();
                monsterCondition.startProgress = progressStep;
                monsterCondition.endProgress = progressStep + progressIncrement;
                
                g_gui.SetLoadDone(progressStep, "Removing monsters in blocking tiles...");
                
                // Need to iterate through all map tiles
                long long total = currentMap.getTileCount();
                long long done = 0;
                for (MapIterator mit = currentMap.begin(); mit != currentMap.end(); ++mit) {
                    if (Tile* tile = (*mit)->get()) {
                        monsterCondition(currentMap, tile, done, total);
                    }
                    done++;
                }
                
                totalCount += monsterCondition.removedCount;
                progressStep += progressIncrement;
                g_gui.SetLoadDone(progressStep);
            }
            
            // Process empty spawn cleanup if selected
            if (cleanEmptySpawns->GetValue()) {
                // Define condition for empty spawns
                struct RemoveEmptySpawnCondition {
                    int removedCount = 0;
                    long long totalTiles;
                    int startProgress;
                    int endProgress;
                    
                    bool operator()(Map& map, Tile* tile, long long done, long long total) {
                        if (done % 1024 == 0) {
                            int progress = startProgress + ((done * (endProgress - startProgress)) / totalTiles);
                            g_gui.SetLoadDone(progress);
                        }
                        
                        if (tile->spawn) {
                            // Check if there are no monsters in the spawn radius
                            bool hasMonster = false;
                            Position pos = tile->getPosition();
                            int radius = tile->spawn->getSize();
                            
                            for (int x = -radius; x <= radius && !hasMonster; ++x) {
                                for (int y = -radius; y <= radius && !hasMonster; ++y) {
                                    Tile* checkTile = map.getTile(pos.x + x, pos.y + y, pos.z);
                                    if (checkTile && checkTile->creature) {
                                        hasMonster = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!hasMonster) {
                                // No monsters in spawn radius, remove it properly
                                map.removeSpawn(tile); // Remove from map's spawn registry
                                delete tile->spawn;
                                tile->spawn = nullptr;
                                tile->deselect(); // Make sure tile is not selected
                                tile->update();  // Update tile to refresh display state
                                tile->modify(); // Mark as modified for saving
                                removedCount++;
                                return true;
                            }
                        }
                        return false;
                    }
                } spawnCondition;
                
                spawnCondition.totalTiles = currentMap.getTileCount();
                spawnCondition.startProgress = progressStep;
                spawnCondition.endProgress = progressStep + progressIncrement;
                
                g_gui.SetLoadDone(progressStep, "Removing empty spawns...");
                
                // Need to iterate through all map tiles
                long long total = currentMap.getTileCount();
                long long done = 0;
                for (MapIterator mit = currentMap.begin(); mit != currentMap.end(); ++mit) {
                    if (Tile* tile = (*mit)->get()) {
                        spawnCondition(currentMap, tile, done, total);
                    }
                    done++;
                }
                
                totalCount += spawnCondition.removedCount;
                progressStep += progressIncrement;
                g_gui.SetLoadDone(progressStep);
            }

            // Process ID range cleanup if selected
            if (useRange->GetValue()) {
                auto ranges = ParseRangeString(rangeInput->GetValue());
                if (!ranges.empty()) {
                    std::vector<uint16_t> ignoredIds;
                    std::vector<std::pair<uint16_t, uint16_t>> ignoredRanges;

                    // Parse ignored IDs if enabled
                    if (useIgnored->GetValue()) {
                        wxString ignoreText = ignoreInput->GetValue();
                        auto ignoredPairs = ParseRangeString(ignoreText);
                        for (const auto& pair : ignoredPairs) {
                            if (pair.first == pair.second) {
                                ignoredIds.push_back(pair.first);
                            } else {
                                ignoredRanges.push_back(pair);
                            }
                        }
                    }

                    // Create cleanup condition with progress tracking
                    struct CleanupCondition {
                        std::vector<std::pair<uint16_t, uint16_t>> ranges;
                        std::vector<uint16_t> ignoredIds;
                        std::vector<std::pair<uint16_t, uint16_t>> ignoredRanges;
                        long long totalTiles;
                        int startProgress;
                        int endProgress;

                        bool operator()(Map& map, Item* item, long long removed, long long done) {
                            // Update progress every 1024 tiles
                            if (done % 1024 == 0) {
                                int progress = startProgress + ((done * (endProgress - startProgress)) / totalTiles);
                                g_gui.SetLoadDone(progress);
                            }

                            uint16_t id = item->getID();

                            // Check if item should be ignored
                            for (uint16_t ignoredId : ignoredIds) {
                                if (id == ignoredId) return false;
                            }
                            for (const auto& range : ignoredRanges) {
                                if (id >= range.first && id <= range.second) return false;
                            }

                            // Check if item is in cleanup ranges
                            for (const auto& range : ranges) {
                                if (id >= range.first && id <= range.second) return true;
                            }
                            return false;
                        }
                    } condition;

                    condition.ranges = ranges;
                    condition.ignoredIds = ignoredIds;
                    condition.ignoredRanges = ignoredRanges;
                    condition.totalTiles = currentMap.getTileCount();
                    condition.startProgress = progressStep;
                    condition.endProgress = 100;

                    // Update loading bar message but keep the same bar
                    g_gui.SetLoadDone(condition.startProgress, "Removing items by ID range...");
                    int64_t count = RemoveItemOnMap(currentMap, condition, false);
                    totalCount += count;
                }
            }

            // Process whey items cleanup if selected
            if (cleanWheyItems->GetValue()) {
                // Define condition for whey items (ID 53)
                struct RemoveWheyItemsCondition {
                    int removedCount = 0;
                    long long totalTiles;
                    int startProgress;
                    int endProgress;
                    
                    bool operator()(Map& map, Item* item, long long removed, long long done) {
                        if (done % 1024 == 0) {
                            int progress = startProgress + ((done * (endProgress - startProgress)) / totalTiles);
                            g_gui.SetLoadDone(progress);
                        }
                        
                        // Check for problematic items
                        std::string name = item->getName();
                        
                        // Original check for "whey" items
                        if (name == "whey") {
                            removedCount++;
                            return true;
                        }
                        
                       
                        
                        // Check for items with ID 0 - these are invalid
                        if (item->getID() == 0) {
                            removedCount++;
                            return true;
                        }
                        
                        return false;
                    }
                } wheyCondition;
                
                wheyCondition.totalTiles = currentMap.getTileCount();
                wheyCondition.startProgress = progressStep;
                wheyCondition.endProgress = progressStep + progressIncrement;
                
                g_gui.SetLoadDone(progressStep, "Removing problematic items...");
                int64_t count = RemoveItemOnMap(currentMap, wheyCondition, false);
                
                // Update tiles after removing items to ensure proper state
                for (MapIterator it = currentMap.begin(); it != currentMap.end(); ++it) {
                    Tile* tile = (*it)->get();
                    if (tile) {
                        tile->update();
                    }
                }
                
                totalCount += count;
                
                progressStep += progressIncrement;
                g_gui.SetLoadDone(progressStep);
            }

            // Ensure progress bar reaches 100%
            g_gui.SetLoadDone(100);
            
            // Destroy the loading bar before showing the popup
            g_gui.DestroyLoadBar();
            
            // Show results
            wxString msg;
            msg << totalCount << " items removed in total.";
            g_gui.PopupDialog("Cleanup Complete", msg, wxOK);
            
            currentMap.doChange();
        }
        catch (...) {
            // Make sure to destroy the loading bar on error
            g_gui.DestroyLoadBar();
            g_gui.PopupDialog("Error", "An error occurred during cleanup.", wxOK | wxICON_ERROR);
        }
    }

    dialog->Destroy();
}

void MainMenuBar::OnMapProperties(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* properties = newd MapPropertiesWindow(
		frame,
		static_cast<MapTab*>(g_gui.GetCurrentTab()),
		*g_gui.GetCurrentEditor()
	);

	if (properties->ShowModal() == 0) {
		// FAIL!
		g_gui.CloseAllEditors();
	}
	properties->Destroy();
}

void MainMenuBar::OnToolbars(wxCommandEvent& event) {
	using namespace MenuBar;

	ActionID id = static_cast<ActionID>(event.GetId() - (wxID_HIGHEST + 1));
	switch (id) {
		case VIEW_TOOLBARS_BRUSHES:
			g_gui.ShowToolbar(TOOLBAR_BRUSHES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_BRUSHES, event.IsChecked());
			break;
		case VIEW_TOOLBARS_POSITION:
			g_gui.ShowToolbar(TOOLBAR_POSITION, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_POSITION, event.IsChecked());
			break;
		case VIEW_TOOLBARS_SIZES:
			g_gui.ShowToolbar(TOOLBAR_SIZES, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_SIZES, event.IsChecked());
			break;
		case VIEW_TOOLBARS_STANDARD:
			g_gui.ShowToolbar(TOOLBAR_STANDARD, event.IsChecked());
			g_settings.setInteger(Config::SHOW_TOOLBAR_STANDARD, event.IsChecked());
			break;
		default:
			break;
	}
}

void MainMenuBar::OnNewView(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewMapView();
}

void MainMenuBar::OnNewDetachedView(wxCommandEvent& WXUNUSED(event)) {
	g_gui.NewDetachedMapView();
}

void MainMenuBar::OnToggleFullscreen(wxCommandEvent& WXUNUSED(event)) {
	if (frame->IsFullScreen()) {
		frame->ShowFullScreen(false);
	} else {
		frame->ShowFullScreen(true, wxFULLSCREEN_NOBORDER | wxFULLSCREEN_NOCAPTION);
	}
}

void MainMenuBar::OnTakeScreenshot(wxCommandEvent& WXUNUSED(event)) {
	wxString path = wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY));
	if (path.size() > 0 && (path.Last() == '/' || path.Last() == '\\')) {
		path = path + "/";
	}

	g_gui.GetCurrentMapTab()->GetView()->GetCanvas()->TakeScreenshot(
		path, wxstr(g_settings.getString(Config::SCREENSHOT_FORMAT))
	);
}

void MainMenuBar::OnZoomIn(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom - 0.1);
}

void MainMenuBar::OnZoomOut(wxCommandEvent& event) {
	double zoom = g_gui.GetCurrentZoom();
	g_gui.SetCurrentZoom(zoom + 0.1);
}

void MainMenuBar::OnZoomNormal(wxCommandEvent& event) {
	g_gui.SetCurrentZoom(1.0);
}

void MainMenuBar::OnChangeViewSettings(wxCommandEvent& event) {
	g_settings.setInteger(Config::SHOW_ALL_FLOORS, IsItemChecked(MenuBar::SHOW_ALL_FLOORS));
	if (IsItemChecked(MenuBar::SHOW_ALL_FLOORS)) {
		EnableItem(MenuBar::SELECT_MODE_VISIBLE, true);
		EnableItem(MenuBar::SELECT_MODE_LOWER, true);
	} else {
		EnableItem(MenuBar::SELECT_MODE_VISIBLE, false);
		EnableItem(MenuBar::SELECT_MODE_LOWER, false);
		CheckItem(MenuBar::SELECT_MODE_CURRENT, true);
		g_settings.setInteger(Config::SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	}
	g_settings.setInteger(Config::TRANSPARENT_FLOORS, IsItemChecked(MenuBar::GHOST_HIGHER_FLOORS));
	g_settings.setInteger(Config::TRANSPARENT_ITEMS, IsItemChecked(MenuBar::GHOST_ITEMS));
	g_settings.setInteger(Config::SHOW_INGAME_BOX, IsItemChecked(MenuBar::SHOW_INGAME_BOX));
	g_settings.setInteger(Config::SHOW_LIGHTS, IsItemChecked(MenuBar::SHOW_LIGHTS));
	g_settings.setInteger(Config::SHOW_LIGHT_STR, IsItemChecked(MenuBar::SHOW_LIGHT_STR));
	g_settings.setInteger(Config::SHOW_TECHNICAL_ITEMS, IsItemChecked(MenuBar::SHOW_TECHNICAL_ITEMS));
	g_settings.setInteger(Config::SHOW_WAYPOINTS, IsItemChecked(MenuBar::SHOW_WAYPOINTS));
	g_settings.setInteger(Config::SHOW_GRID, IsItemChecked(MenuBar::SHOW_GRID));
	g_settings.setInteger(Config::SHOW_EXTRA, !IsItemChecked(MenuBar::SHOW_EXTRA));

	g_settings.setInteger(Config::SHOW_SHADE, IsItemChecked(MenuBar::SHOW_SHADE));
	g_settings.setInteger(Config::SHOW_SPECIAL_TILES, IsItemChecked(MenuBar::SHOW_SPECIAL));
	g_settings.setInteger(Config::SHOW_ZONE_AREAS, IsItemChecked(MenuBar::SHOW_ZONES));
	g_settings.setInteger(Config::SHOW_AS_MINIMAP, IsItemChecked(MenuBar::SHOW_AS_MINIMAP));
	g_settings.setInteger(Config::SHOW_ONLY_TILEFLAGS, IsItemChecked(MenuBar::SHOW_ONLY_COLORS));
	g_settings.setInteger(Config::SHOW_ONLY_MODIFIED_TILES, IsItemChecked(MenuBar::SHOW_ONLY_MODIFIED));
	g_settings.setInteger(Config::SHOW_CREATURES, IsItemChecked(MenuBar::SHOW_CREATURES));
	g_settings.setInteger(Config::SHOW_SPAWNS, IsItemChecked(MenuBar::SHOW_SPAWNS));
	g_settings.setInteger(Config::SHOW_HOUSES, IsItemChecked(MenuBar::SHOW_HOUSES));
	g_settings.setInteger(Config::HIGHLIGHT_ITEMS, IsItemChecked(MenuBar::HIGHLIGHT_ITEMS));
	g_settings.setInteger(Config::HIGHLIGHT_LOCKED_DOORS, IsItemChecked(MenuBar::HIGHLIGHT_LOCKED_DOORS));
	g_settings.setInteger(Config::SHOW_BLOCKING, IsItemChecked(MenuBar::SHOW_PATHING));
	g_settings.setInteger(Config::SHOW_TOOLTIPS, IsItemChecked(MenuBar::SHOW_TOOLTIPS));
	g_settings.setInteger(Config::SHOW_PREVIEW, IsItemChecked(MenuBar::SHOW_PREVIEW));
	g_settings.setInteger(Config::SHOW_WALL_HOOKS, IsItemChecked(MenuBar::SHOW_WALL_HOOKS));
	g_settings.setInteger(Config::SHOW_TOWNS, IsItemChecked(MenuBar::SHOW_TOWNS));
	g_settings.setInteger(Config::ALWAYS_SHOW_ZONES, IsItemChecked(MenuBar::ALWAYS_SHOW_ZONES));
	g_settings.setInteger(Config::EXT_HOUSE_SHADER, IsItemChecked(MenuBar::EXT_HOUSE_SHADER));
	g_settings.setInteger(Config::HOUSE_CUSTOM_COLORS, IsItemChecked(MenuBar::HOUSE_CUSTOM_COLORS));

	g_settings.setInteger(Config::EXPERIMENTAL_FOG, IsItemChecked(MenuBar::EXPERIMENTAL_FOG));

	g_gui.RefreshView();
}

void MainMenuBar::OnChangeFloor(wxCommandEvent& event) {
	// Workaround to stop events from looping
	if (checking_programmaticly) {
		return;
	}

	// this will have to be changed if you want to have more floors
	// see MAKE_ACTION(FLOOR_0, wxITEM_RADIO, OnChangeFloor);
	//
	if (MAP_MAX_LAYER < 16) {
		for (int i = 0; i < MAP_LAYERS; ++i) {
			if (IsItemChecked(MenuBar::ActionID(MenuBar::FLOOR_0 + i))) {
				g_gui.ChangeFloor(i);
			}
		}
	}
}

void MainMenuBar::OnMinimapWindow(wxCommandEvent& event) {
	g_gui.CreateMinimap();
}

void MainMenuBar::OnRecentBrushesWindow(wxCommandEvent& event) {
	g_gui.ShowRecentBrushesWindow();
}

void MainMenuBar::OnNewPalette(wxCommandEvent& event) {
	g_gui.NewPalette();
}

void MainMenuBar::OnSelectTerrainPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_TERRAIN);
}

void MainMenuBar::OnSelectDoodadPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_DOODAD);
}

void MainMenuBar::OnSelectItemPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_ITEM);
}

void MainMenuBar::OnSelectCollectionPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_COLLECTION);
}

void MainMenuBar::OnSelectHousePalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_HOUSE);
}

void MainMenuBar::OnSelectCreaturePalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_CREATURE);
}

void MainMenuBar::OnSelectWaypointPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_WAYPOINT);
}

void MainMenuBar::OnSelectRawPalette(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectPalettePage(TILESET_RAW);
}

void MainMenuBar::OnStartLive(wxCommandEvent& event) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		g_gui.PopupDialog("Error", "You need to have a map open to start a live mapping session.", wxOK);
		return;
	}
	if (editor->IsLive()) {
		g_gui.PopupDialog("Error", "You can not start two live servers on the same map (or a server using a remote map).", wxOK);
		return;
	}

	wxDialog* live_host_dlg = newd wxDialog(frame, wxID_ANY, "Host Live Server", wxDefaultPosition, wxDefaultSize);

	wxSizer* top_sizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gsizer = newd wxFlexGridSizer(2, 10, 10);
	gsizer->AddGrowableCol(0, 2);
	gsizer->AddGrowableCol(1, 3);

	// Data fields
	wxTextCtrl* hostname;
	wxSpinCtrl* port;
	wxTextCtrl* password;
	wxCheckBox* allow_copy;

	gsizer->Add(newd wxStaticText(live_host_dlg, wxID_ANY, "Server Name:"));
	gsizer->Add(hostname = newd wxTextCtrl(live_host_dlg, wxID_ANY, "RME Live Server"), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_host_dlg, wxID_ANY, "Port:"));
	gsizer->Add(port = newd wxSpinCtrl(live_host_dlg, wxID_ANY, "31313", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 31313), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_host_dlg, wxID_ANY, "Password:"));
	gsizer->Add(password = newd wxTextCtrl(live_host_dlg, wxID_ANY), 0, wxEXPAND);

	top_sizer->Add(gsizer, 0, wxALL, 20);

	top_sizer->Add(allow_copy = newd wxCheckBox(live_host_dlg, wxID_ANY, "Allow copy & paste between maps."), 0, wxRIGHT | wxLEFT, 20);
	allow_copy->SetToolTip("Allows remote clients to copy & paste from the hosted map to local maps.");

	wxSizer* ok_sizer = newd wxBoxSizer(wxHORIZONTAL);
	ok_sizer->Add(newd wxButton(live_host_dlg, wxID_OK, "OK"), 1, wxCENTER);
	ok_sizer->Add(newd wxButton(live_host_dlg, wxID_CANCEL, "Cancel"), wxCENTER, 1);
	top_sizer->Add(ok_sizer, 0, wxCENTER | wxALL, 20);

	live_host_dlg->SetSizerAndFit(top_sizer);

	while (true) {
		int ret = live_host_dlg->ShowModal();
		if (ret == wxID_OK) {
			LiveServer* liveServer = editor->StartLiveServer();
			liveServer->setName(hostname->GetValue());
			liveServer->setPassword(password->GetValue());
			liveServer->setPort(port->GetValue());

			const wxString& error = liveServer->getLastError();
			if (!error.empty()) {
				g_gui.PopupDialog(live_host_dlg, "Error", error, wxOK);
				editor->CloseLiveServer();
				continue;
			}

			if (!liveServer->bind()) {
				g_gui.PopupDialog("Socket Error", "Could not bind socket! Try another port?", wxOK);
				editor->CloseLiveServer();
			} else {
				liveServer->createLogWindow(g_gui.tabbook);
			}
			break;
		} else {
			break;
		}
	}
	live_host_dlg->Destroy();
	Update();
}

void MainMenuBar::OnJoinLive(wxCommandEvent& event) {
	wxDialog* live_join_dlg = newd wxDialog(frame, wxID_ANY, "Join Live Server", wxDefaultPosition, wxDefaultSize);

	wxSizer* top_sizer = newd wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* gsizer = newd wxFlexGridSizer(2, 10, 10);
	gsizer->AddGrowableCol(0, 2);
	gsizer->AddGrowableCol(1, 3);

	// Data fields
	wxTextCtrl* name;
	wxTextCtrl* ip;
	wxSpinCtrl* port;
	wxTextCtrl* password;

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "Name:"));
	gsizer->Add(name = newd wxTextCtrl(live_join_dlg, wxID_ANY, ""), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "IP:"));
	gsizer->Add(ip = newd wxTextCtrl(live_join_dlg, wxID_ANY, "localhost"), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "Port:"));
	gsizer->Add(port = newd wxSpinCtrl(live_join_dlg, wxID_ANY, "31313", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 65535, 31313), 0, wxEXPAND);

	gsizer->Add(newd wxStaticText(live_join_dlg, wxID_ANY, "Password:"));
	gsizer->Add(password = newd wxTextCtrl(live_join_dlg, wxID_ANY), 0, wxEXPAND);

	top_sizer->Add(gsizer, 0, wxALL, 20);

	wxSizer* ok_sizer = newd wxBoxSizer(wxHORIZONTAL);
	ok_sizer->Add(newd wxButton(live_join_dlg, wxID_OK, "OK"), 1, wxRIGHT);
	ok_sizer->Add(newd wxButton(live_join_dlg, wxID_CANCEL, "Cancel"), 1, wxRIGHT);
	top_sizer->Add(ok_sizer, 0, wxCENTER | wxALL, 20);

	live_join_dlg->SetSizerAndFit(top_sizer);

	while (true) {
		int ret = live_join_dlg->ShowModal();
		if (ret == wxID_OK) {
			LiveClient* liveClient = newd LiveClient();
			liveClient->setPassword(password->GetValue());

			wxString tmp = name->GetValue();
			if (tmp.empty()) {
				tmp = "User";
			}
			liveClient->setName(tmp);

			const wxString& error = liveClient->getLastError();
			if (!error.empty()) {
				g_gui.PopupDialog(live_join_dlg, "Error", error, wxOK);
				delete liveClient;
				continue;
			}

			const wxString& address = ip->GetValue();
			int32_t portNumber = port->GetValue();

			liveClient->createLogWindow(g_gui.tabbook);
			if (!liveClient->connect(nstr(address), portNumber)) {
				g_gui.PopupDialog("Connection Error", liveClient->getLastError(), wxOK);
				delete liveClient;
			}

			break;
		} else {
			break;
		}
	}
	live_join_dlg->Destroy();
	Update();
}

void MainMenuBar::OnCloseLive(wxCommandEvent& event) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (editor && editor->IsLive()) {
		g_gui.CloseLiveEditors(&editor->GetLive());
	}

	Update();
}

void MainMenuBar::SearchItems(bool unique, bool action, bool container, bool writable, bool zones, bool onSelection /* = false*/) {
	if (!unique && !action && !container && !writable && !zones) {
        return;
    }

    if (!g_gui.IsEditorOpen()) {
        return;
    }

    // Create search dialog
    wxDialog dialog(frame, wxID_ANY, "Advanced Search", wxDefaultPosition, wxDefaultSize);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create input fields based on search type
    wxTextCtrl* uniqueRangeCtrl = nullptr;
    wxTextCtrl* actionRangeCtrl = nullptr;

    if (unique) {
        wxStaticBoxSizer* uniqueSizer = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Unique ID Range");
        wxStaticText* uniqueHelp = new wxStaticText(&dialog, wxID_ANY, 
            "Enter ranges (e.g., 1000-2000) or individual IDs separated by commas");
        uniqueRangeCtrl = new wxTextCtrl(&dialog, wxID_ANY);
        uniqueSizer->Add(uniqueHelp, 0, wxALL, 5);
        uniqueSizer->Add(uniqueRangeCtrl, 0, wxEXPAND | wxALL, 5);
        mainSizer->Add(uniqueSizer, 0, wxEXPAND | wxALL, 5);
    }

    if (action) {
        wxStaticBoxSizer* actionSizer = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Action ID Range");
        wxStaticText* actionHelp = new wxStaticText(&dialog, wxID_ANY, 
            "Enter ranges (e.g., 100-200) or individual IDs separated by commas");
        actionRangeCtrl = new wxTextCtrl(&dialog, wxID_ANY);
        actionSizer->Add(actionHelp, 0, wxALL, 5);
        actionSizer->Add(actionRangeCtrl, 0, wxEXPAND | wxALL, 5);
        mainSizer->Add(actionSizer, 0, wxEXPAND | wxALL, 5);
    }

    // Add OK/Cancel buttons
    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    buttonSizer->AddButton(new wxButton(&dialog, wxID_OK));
    buttonSizer->AddButton(new wxButton(&dialog, wxID_CANCEL));
    buttonSizer->Realize();
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog.SetSizer(mainSizer);
    mainSizer->Fit(&dialog);
    dialog.Center();

    // Show dialog and process result
    if (dialog.ShowModal() == wxID_OK) {
        std::vector<std::pair<uint16_t, uint16_t>> uniqueRanges;
        std::vector<std::pair<uint16_t, uint16_t>> actionRanges;

        if (uniqueRangeCtrl) {
            uniqueRanges = ParseRangeString(uniqueRangeCtrl->GetValue());
        }
        if (actionRangeCtrl) {
            actionRanges = ParseRangeString(actionRangeCtrl->GetValue());
        }

        if (onSelection) {
            g_gui.CreateLoadBar("Searching on selected area...");
        } else {
            g_gui.CreateLoadBar("Searching on map...");
        }

        OnSearchForStuff::Searcher searcher;
		searcher.search_zones = zones;
        searcher.search_unique = unique;
        searcher.search_action = action;
        searcher.search_container = container;
        searcher.search_writeable = writable;
        searcher.uniqueRanges = uniqueRanges;
        searcher.actionRanges = actionRanges;

        foreach_ItemOnMap(g_gui.GetCurrentMap(), searcher, onSelection);
        searcher.sort();
        std::vector<std::pair<Tile*, Item*>>& found = searcher.found;

        g_gui.DestroyLoadBar();

        SearchResultWindow* result = g_gui.ShowSearchWindow();
        result->Clear();
		for (std::vector<std::pair<Tile*, Item*>>::iterator iter = found.begin(); iter != found.end(); ++iter) {
			result->AddPosition(searcher.desc(iter->first, iter->second), iter->first->getPosition());
        }
    }
}

// Helper function to parse range string into pairs of IDs
std::vector<std::pair<uint16_t, uint16_t>> ParseRangeString(const wxString& input) {
    std::vector<std::pair<uint16_t, uint16_t>> ranges;
    std::string str = as_lower_str(nstr(input));
    std::vector<std::string> parts = splitString(str, ',');
    
    for(const auto& part : parts) {
        if(part.find('-') != std::string::npos) {
            std::vector<std::string> range = splitString(part, '-');
            if(range.size() == 2 && isInteger(range[0]) && isInteger(range[1])) {
                uint16_t from = static_cast<uint16_t>(std::stoi(range[0]));
                uint16_t to = static_cast<uint16_t>(std::stoi(range[1]));
                if(from <= to) {
                    ranges.emplace_back(from, to);
                }
            }
        } else if(isInteger(part)) {
            uint16_t id = static_cast<uint16_t>(std::stoi(part));
            ranges.emplace_back(id, id);
        }
    }
    
    return ranges;
}

void MainMenuBar::OnMapRemoveDuplicates(wxCommandEvent& WXUNUSED(event)) {
    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) return;

    wxDialog dialog(frame, wxID_ANY, "Remove Duplicates", wxDefaultPosition, wxSize(800, 600));
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Warning text
    wxStaticText* warning = new wxStaticText(&dialog, wxID_ANY, 
        "WARNING: Save your map before proceeding!\n"
        "Choose removal mode and properties to ignore:");
    main_sizer->Add(warning, 0, wxALL, 5);

    // Create horizontal sizer for buttons and properties
    wxBoxSizer* content_sizer = new wxBoxSizer(wxHORIZONTAL);

    // Left side - Buttons
    wxStaticBoxSizer* button_sizer = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Removal Options");
    
    wxButton* removeAll = new wxButton(&dialog, wxID_ANY, "Remove All Duplicates");
    button_sizer->Add(removeAll, 0, wxALL | wxEXPAND, 5);

    wxButton* removeSelected = new wxButton(&dialog, wxID_ANY, "Remove Selected Item Duplicates");
    const Brush* brush = g_gui.GetCurrentBrush();
    removeSelected->Enable(brush && brush->isRaw());
    button_sizer->Add(removeSelected, 0, wxALL | wxEXPAND, 5);

    wxButton* removeFromSelection = new wxButton(&dialog, wxID_ANY, "Remove Duplicates of Selected Items");
    removeFromSelection->Enable(editor->selection.size() > 0);
    button_sizer->Add(removeFromSelection, 0, wxALL | wxEXPAND, 5);

    wxButton* removeInSelection = new wxButton(&dialog, wxID_ANY, "Remove Duplicates in Selection Area");
    removeInSelection->Enable(editor->selection.size() > 0);
    button_sizer->Add(removeInSelection, 0, wxALL | wxEXPAND, 5);

    content_sizer->Add(button_sizer, 1, wxEXPAND | wxALL, 5);

    // Right side - Properties to ignore
    wxStaticBoxSizer* props_sizer = new wxStaticBoxSizer(wxVERTICAL, &dialog, "Ignore Property Differences");
    
    // Create property checkboxes
    wxCheckBox* ignore_unpassable = new wxCheckBox(&dialog, wxID_ANY, "Unpassable");
    wxCheckBox* ignore_unmovable = new wxCheckBox(&dialog, wxID_ANY, "Unmovable");
    wxCheckBox* ignore_block_missiles = new wxCheckBox(&dialog, wxID_ANY, "Block Missiles");
    wxCheckBox* ignore_block_pathfinder = new wxCheckBox(&dialog, wxID_ANY, "Block Pathfinder");
    wxCheckBox* ignore_readable = new wxCheckBox(&dialog, wxID_ANY, "Readable");
    wxCheckBox* ignore_writeable = new wxCheckBox(&dialog, wxID_ANY, "Writeable");
    wxCheckBox* ignore_pickupable = new wxCheckBox(&dialog, wxID_ANY, "Pickupable");
    wxCheckBox* ignore_stackable = new wxCheckBox(&dialog, wxID_ANY, "Stackable");
    wxCheckBox* ignore_rotatable = new wxCheckBox(&dialog, wxID_ANY, "Rotatable");
    wxCheckBox* ignore_hangable = new wxCheckBox(&dialog, wxID_ANY, "Hangable");
    wxCheckBox* ignore_hook_east = new wxCheckBox(&dialog, wxID_ANY, "Hook East");
    wxCheckBox* ignore_hook_south = new wxCheckBox(&dialog, wxID_ANY, "Hook South");
    wxCheckBox* ignore_elevation = new wxCheckBox(&dialog, wxID_ANY, "Has Elevation");

    // Add checkboxes to properties sizer
    props_sizer->Add(ignore_unpassable, 0, wxALL, 3);
    props_sizer->Add(ignore_unmovable, 0, wxALL, 3);
    props_sizer->Add(ignore_block_missiles, 0, wxALL, 3);
    props_sizer->Add(ignore_block_pathfinder, 0, wxALL, 3);
    props_sizer->Add(ignore_readable, 0, wxALL, 3);
    props_sizer->Add(ignore_writeable, 0, wxALL, 3);
    props_sizer->Add(ignore_pickupable, 0, wxALL, 3);
    props_sizer->Add(ignore_stackable, 0, wxALL, 3);
    props_sizer->Add(ignore_rotatable, 0, wxALL, 3);
    props_sizer->Add(ignore_hangable, 0, wxALL, 3);
    props_sizer->Add(ignore_hook_east, 0, wxALL, 3);
    props_sizer->Add(ignore_hook_south, 0, wxALL, 3);
    props_sizer->Add(ignore_elevation, 0, wxALL, 3);

    content_sizer->Add(props_sizer, 1, wxEXPAND | wxALL, 5);

    main_sizer->Add(content_sizer, 1, wxEXPAND);

    // Cancel button at bottom
    wxButton* cancel = new wxButton(&dialog, wxID_CANCEL, "Cancel");
    main_sizer->Add(cancel, 0, wxALL | wxCENTER, 5);
//main_sizer 
    dialog.SetSizer(main_sizer);

    // Previous button handlers remain the same, but need to capture checkbox states
    // and pass them to the cleanDuplicateItems function...

    // Create a struct to hold property flags
    struct PropertyFlags {
        bool ignore_unpassable;
        bool ignore_unmovable;
        bool ignore_block_missiles;
        bool ignore_block_pathfinder;
        bool ignore_readable;
        bool ignore_writeable;
        bool ignore_pickupable;
        bool ignore_stackable;
        bool ignore_rotatable;
        bool ignore_hangable;
        bool ignore_hook_east;
        bool ignore_hook_south;
        bool ignore_elevation;
    };

    // Button 1 handler: Remove all duplicates
    removeAll->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        // Use global PropertyFlags instead of local definition
        ::PropertyFlags flags = {
            ignore_unpassable->GetValue(),
            ignore_unmovable->GetValue(),
            ignore_block_missiles->GetValue(),
            ignore_block_pathfinder->GetValue(),
            ignore_readable->GetValue(),
            ignore_writeable->GetValue(),
            ignore_pickupable->GetValue(),
            ignore_stackable->GetValue(),
            ignore_rotatable->GetValue(),
            ignore_hangable->GetValue(),
            ignore_hook_east->GetValue(),
            ignore_hook_south->GetValue(),
            ignore_elevation->GetValue()
        };

        g_gui.CreateLoadBar("Removing all duplicate items...");
        uint32_t removed = editor->map.cleanDuplicateItems(std::vector<std::pair<uint16_t, uint16_t>>(), flags);
        g_gui.DestroyLoadBar();

        std::ostringstream ss;
        ss << "Remove Duplicates completed:\n" << removed << " duplicate items removed.";
        g_gui.PopupDialog("Remove Duplicates", ss.str(), wxOK);
        dialog.EndModal(wxID_OK);
    });

    // Button 2 handler: Remove RAW brush item duplicates
    removeSelected->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        const RAWBrush* rawBrush = dynamic_cast<const RAWBrush*>(g_gui.GetCurrentBrush());
        if (!rawBrush) return;

        // Use global PropertyFlags instead of local definition
        ::PropertyFlags flags = {
            ignore_unpassable->GetValue(),
            ignore_unmovable->GetValue(),
            ignore_block_missiles->GetValue(),
            ignore_block_pathfinder->GetValue(),
            ignore_readable->GetValue(),
            ignore_writeable->GetValue(),
            ignore_pickupable->GetValue(),
            ignore_stackable->GetValue(),
            ignore_rotatable->GetValue(),
            ignore_hangable->GetValue(),
            ignore_hook_east->GetValue(),
            ignore_hook_south->GetValue(),
            ignore_elevation->GetValue()
        };

        uint16_t itemId = rawBrush->getItemType()->id;
        std::vector<std::pair<uint16_t, uint16_t>> range = {{itemId, itemId}};

        g_gui.CreateLoadBar("Removing selected item duplicates...");
        uint32_t removed = editor->map.cleanDuplicateItems(range, flags);
        g_gui.DestroyLoadBar();

        std::ostringstream ss;
        ss << "Remove Duplicates completed:\n" << removed << " duplicates of item " << itemId << " removed.";
        g_gui.PopupDialog("Remove Duplicates", ss.str(), wxOK);
        dialog.EndModal(wxID_OK);
    });

    // Button 3 handler: Remove duplicates of items in selection
    removeFromSelection->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        const TileSet& tiles = editor->selection.getTiles();
        if(tiles.empty()) {
            g_gui.PopupDialog("Error", "No area selected!", wxOK);
            return;
        }

        // Collect all unique item IDs from selection
        std::set<uint16_t> selectedIds;
        for(Tile* tile : tiles) {
            if(!tile) continue;
            
            if(tile->ground) {
                selectedIds.insert(tile->ground->getID());
            }
            
            for(Item* item : tile->items) {
                if(item) {
                    selectedIds.insert(item->getID());
                }
            }
        }

        // Convert to ranges for single IDs
        std::vector<std::pair<uint16_t, uint16_t>> ranges;
        for(uint16_t id : selectedIds) {
            ranges.push_back({id, id});
        }

        // Use global PropertyFlags instead of local definition
        ::PropertyFlags flags = {
            ignore_unpassable->GetValue(),
            ignore_unmovable->GetValue(),
            ignore_block_missiles->GetValue(),
            ignore_block_pathfinder->GetValue(),
            ignore_readable->GetValue(),
            ignore_writeable->GetValue(),
            ignore_pickupable->GetValue(),
            ignore_stackable->GetValue(),
            ignore_rotatable->GetValue(),
            ignore_hangable->GetValue(),
            ignore_hook_east->GetValue(),
            ignore_hook_south->GetValue(),
            ignore_elevation->GetValue()
        };

        g_gui.CreateLoadBar("Removing duplicates of selected items...");
        uint32_t removed = editor->map.cleanDuplicateItems(ranges, flags);
        g_gui.DestroyLoadBar();

        std::ostringstream ss;
        ss << "Remove Duplicates completed:\n" << removed << " duplicates of " 
           << selectedIds.size() << " selected item types removed.";
        g_gui.PopupDialog("Remove Duplicates", ss.str(), wxOK);
        dialog.EndModal(wxID_OK);
    });

    // Button 4 handler: Remove duplicates within selection area
    removeInSelection->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        const TileSet& tiles = editor->selection.getTiles();
        if(tiles.empty()) {
            g_gui.PopupDialog("Error", "No area selected!", wxOK);
            return;
        }

        // Use global PropertyFlags instead of local definition
        ::PropertyFlags flags = {
            ignore_unpassable->GetValue(),
            ignore_unmovable->GetValue(),
            ignore_block_missiles->GetValue(),
            ignore_block_pathfinder->GetValue(),
            ignore_readable->GetValue(),
            ignore_writeable->GetValue(),
            ignore_pickupable->GetValue(),
            ignore_stackable->GetValue(),
            ignore_rotatable->GetValue(),
            ignore_hangable->GetValue(),
            ignore_hook_east->GetValue(),
            ignore_hook_south->GetValue(),
            ignore_elevation->GetValue()
        };

        uint32_t total_removed = 0;
        g_gui.CreateLoadBar("Removing duplicates in selection...");
        
        // Process each selected tile independently
        for(Tile* tile : tiles) {
            if(!tile) continue;
            
            std::vector<std::pair<uint16_t, uint16_t>> empty_ranges; // Process all IDs within tile
            // Create a temporary map with single tile for processing
            Map tempMap;
            Tile* tempTile = tile->deepCopy(tempMap); // Pass tempMap instead of Position
            tempMap.setTile(tempTile->getPosition(), tempTile);
            
            uint32_t removed = tempMap.cleanDuplicateItems(empty_ranges, flags);
            if(removed > 0) {
                // If items were removed, replace original tile with cleaned tile
                Tile* cleanedTile = tempMap.getTile(tempTile->getPosition());
                editor->map.setTile(tile->getPosition(), cleanedTile->deepCopy(editor->map)); // Pass editor->map
                total_removed += removed;
            }
        }
        
        g_gui.DestroyLoadBar();

        std::ostringstream ss;
        ss << "Remove Duplicates completed:\n" << total_removed 
           << " duplicate items removed from " << tiles.size() << " selected tiles.";
        g_gui.PopupDialog("Remove Duplicates", ss.str(), wxOK);
        dialog.EndModal(wxID_OK);
    });

    dialog.ShowModal();
}

void MainMenuBar::OnShowHotkeys(wxCommandEvent& WXUNUSED(event)) {
	g_hotkey_manager.ShowHotkeyDialog(frame);
}

void MainMenuBar::OnShowMonsterMaker(wxCommandEvent& WXUNUSED(event)) {
	g_gui.ShowMonsterMakerWindow();
}

void MainMenuBar::OnAddNewCreature(wxCommandEvent& WXUNUSED(event)) {
	AddCreatureDialog dialog(frame, "");
	if (dialog.ShowModal() == wxID_OK) {
		// Get the created creature type
		CreatureType* createdType = dialog.GetCreatureType();
		if (createdType) {
			// Refresh palettes to show the new creature
			g_gui.RefreshPalettes();
			
			// Find and select the newly created creature brush
			Brush* createdBrush = nullptr;
			const BrushMap& brushes = g_brushes.getMap();
			for (const auto& brushPair : brushes) {
				Brush* brush = brushPair.second;
				if (brush && brush->isCreature()) {
					CreatureBrush* cb = brush->asCreature();
					if (cb && cb->getType() == createdType) {
						createdBrush = brush;
						break;
					}
				}
			}
			
			// Set the creature palette as active and select the new brush
			if (createdBrush) {
				g_gui.SelectBrush(createdBrush);
				g_gui.SelectPalettePage(TILESET_CREATURE);
			}
		}
	}
}

void MainMenuBar::OnRefreshVisibleArea(wxCommandEvent& WXUNUSED(event)) {
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) {
		return;
	}
	
	LiveClient* client = dynamic_cast<LiveClient*>(editor->GetLiveClient());
	if (client) {
		client->requestVisibleRefresh();
		g_gui.SetStatusText("Refreshing visible area...");
	} else {
		g_gui.SetStatusText("Refresh only available in multiplayer mode.");
	}
}

void MainMenuBar::OnChatRegister(wxCommandEvent& WXUNUSED(event)) {
    ChatManager::ShowRegisterDialog();
}

void MainMenuBar::OnChatConnect(wxCommandEvent& WXUNUSED(event)) {
    ChatManager::ShowChatWindow();
}

void MainMenuBar::onServerHost(wxCommandEvent& event) {
    wxDialog* hostDialog = new wxDialog(frame, wxID_ANY, "Host Server", wxDefaultPosition, wxSize(300, 200));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 10, 10);
    
    // Port
    gridSizer->Add(new wxStaticText(hostDialog, wxID_ANY, "Port:"));
    wxSpinCtrl* portCtrl = new wxSpinCtrl(hostDialog, wxID_ANY);
    portCtrl->SetRange(1, 65535);
    portCtrl->SetValue(g_settings.getInteger(Config::LIVE_PORT));
    gridSizer->Add(portCtrl);
    
    // Password
    gridSizer->Add(new wxStaticText(hostDialog, wxID_ANY, "Password:"));
    wxTextCtrl* passwordCtrl = new wxTextCtrl(hostDialog, wxID_ANY);
    passwordCtrl->SetValue(wxstr(g_settings.getString(Config::LIVE_PASSWORD)));
    gridSizer->Add(passwordCtrl);
    
    sizer->Add(gridSizer, 0, wxALL, 10);
    
    // Host button
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* hostButton = new wxButton(hostDialog, wxID_OK, "Host");
    wxButton* cancelButton = new wxButton(hostDialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(hostButton);
    buttonSizer->Add(cancelButton);
    
    sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    hostDialog->SetSizer(sizer);
    
    if (hostDialog->ShowModal() == wxID_OK) {
        // Get port and password from controls
        int port = portCtrl->GetValue();
        wxString password = passwordCtrl->GetValue();
        
        // Save settings
        g_settings.setInteger(Config::LIVE_PORT, port);
        g_settings.setString(Config::LIVE_PASSWORD, nstr(password));
        
        // Create server
        LiveServer* server = new LiveServer(*g_gui.GetCurrentEditor());
        
        // Set the server name to HOST for easy identification in chat
        server->setName("HOST");
        
        if (!server->setPort(port)) {
            wxMessageBox(wxString(server->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete server;
            return;
        }
        
        if (!server->setPassword(password)) {
            wxMessageBox(wxString(server->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete server;
            return;
        }
        
        // Start server
        if (!server->bind()) {
            wxMessageBox(wxString(server->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete server;
            return;
        }
        
        // Create log window
        LiveLogTab* log = server->createLogWindow(g_gui.tabbook);
        g_gui.RefreshPalettes();
    }
    
    hostDialog->Destroy();
}

void MainMenuBar::onServerConnect(wxCommandEvent& event) {
    wxDialog* connectDialog = new wxDialog(frame, wxID_ANY, "Connect to Server", wxDefaultPosition, wxSize(300, 240));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 10, 10);
    
    // Host
    gridSizer->Add(new wxStaticText(connectDialog, wxID_ANY, "Host:"));
    wxTextCtrl* hostCtrl = new wxTextCtrl(connectDialog, wxID_ANY);
    hostCtrl->SetValue(wxstr(g_settings.getString(Config::LIVE_HOST)));
    gridSizer->Add(hostCtrl);
    
    // Port
    gridSizer->Add(new wxStaticText(connectDialog, wxID_ANY, "Port:"));
    wxSpinCtrl* portCtrl = new wxSpinCtrl(connectDialog, wxID_ANY);
    portCtrl->SetRange(1, 65535);
    portCtrl->SetValue(g_settings.getInteger(Config::LIVE_PORT));
    gridSizer->Add(portCtrl);
    
    // Username
    gridSizer->Add(new wxStaticText(connectDialog, wxID_ANY, "Username:"));
    wxTextCtrl* usernameCtrl = new wxTextCtrl(connectDialog, wxID_ANY);
    usernameCtrl->SetValue(wxstr(g_settings.getString(Config::LIVE_USERNAME)));
    gridSizer->Add(usernameCtrl);
    
    // Password
    gridSizer->Add(new wxStaticText(connectDialog, wxID_ANY, "Password:"));
    wxTextCtrl* passwordCtrl = new wxTextCtrl(connectDialog, wxID_ANY);
    passwordCtrl->SetValue(wxstr(g_settings.getString(Config::LIVE_PASSWORD)));
    gridSizer->Add(passwordCtrl);
    
    sizer->Add(gridSizer, 0, wxALL, 10);
    
    // Connect button
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* connectButton = new wxButton(connectDialog, wxID_OK, "Connect");
    wxButton* cancelButton = new wxButton(connectDialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(connectButton);
    buttonSizer->Add(cancelButton);
    
    sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);
    connectDialog->SetSizer(sizer);
    
    if (connectDialog->ShowModal() == wxID_OK) {
        // Get connection parameters from controls
        wxString host = hostCtrl->GetValue();
        int port = portCtrl->GetValue();
        wxString username = usernameCtrl->GetValue();
        wxString password = passwordCtrl->GetValue();
        
        // Save settings
        g_settings.setString(Config::LIVE_HOST, nstr(host));
        g_settings.setInteger(Config::LIVE_PORT, port);
        g_settings.setString(Config::LIVE_USERNAME, nstr(username));
        g_settings.setString(Config::LIVE_PASSWORD, nstr(password));
        
        // Create client
        LiveClient* client = new LiveClient();
        if (!client->setName(username)) {
            wxMessageBox(wxString(client->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete client;
            return;
        }
        
        if (!client->setPassword(password)) {
            wxMessageBox(wxString(client->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete client;
            return;
        }
        
        // Connect to server
        if (!client->connect(nstr(host), port)) {
            wxMessageBox(wxString(client->getLastError()), "Error", wxOK | wxICON_ERROR);
            delete client;
            return;
        }
        
        // Create log window
        client->createLogWindow(g_gui.tabbook);
    }
    
    connectDialog->Destroy();
}

void MainMenuBar::OnRefreshItems(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    FindItemDialog dialog(frame, "Refresh Items");
    dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(Config::FIND_ITEM_MODE));
    
    if (dialog.ShowModal() == wxID_OK) {
        Editor* editor = g_gui.GetCurrentEditor();
        if (!editor) return;

        g_gui.CreateLoadBar("Refreshing items...");
        
        // First find all matching items
        OnSearchForItem::Finder finder(dialog.getResultID(), (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
        foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
        std::vector<std::pair<Tile*, Item*>>& items = finder.result;

        // Store properties of found items
        struct ItemData {
            Position pos;
            uint16_t id;
            uint32_t actionId;
            uint32_t uniqueId;
            std::string text;
            size_t stackpos;     // Store index in tile's item vector
            Container* container; // Store container if item is inside one
            size_t containerIndex; // Store index in container
        };
        std::vector<ItemData> itemsToRecreate;

        for(const auto& pair : items) {
            Tile* tile = pair.first;
            Item* item = pair.second;
            
            ItemData data;
            data.pos = tile->getPosition();
            data.id = item->getID();
            data.actionId = item->getActionID();
            data.uniqueId = item->getUniqueID();
            data.text = item->getText();
            data.container = nullptr;
            data.containerIndex = 0;
            
            // Find item's position in tile or container
            bool found = false;
            
            // First check if item is in a container on this tile
            for(Item* tileItem : tile->items) {
                if(Container* container = dynamic_cast<Container*>(tileItem)) {
                    const ItemVector& containerItems = container->getVector();
                    for(size_t idx = 0; idx < containerItems.size(); ++idx) {
                        if(containerItems[idx] == item) {
                            data.container = container;
                            data.containerIndex = idx;
                            found = true;
                            break;
                        }
                    }
                }
                if(found) break;
            }
            
            // If not in container, find position in tile
            if(!found) {
                for(size_t idx = 0; idx < tile->items.size(); ++idx) {
                    if(tile->items[idx] == item) {
                        data.stackpos = idx;
                        break;
                    }
                }
            }
            
            itemsToRecreate.push_back(data);
        }

        // Remove and recreate items
        for(const auto& data : itemsToRecreate) {
            Item* oldItem = nullptr;
            
            if(data.container) {
                // Item is in container
                ItemVector& containerItems = data.container->getVector();
                if(data.containerIndex < containerItems.size()) {
                    oldItem = containerItems[data.containerIndex];
                    containerItems.erase(containerItems.begin() + data.containerIndex);
                }
            } else {
                // Item is on tile
                Tile* tile = editor->map.getTile(data.pos);
                if(!tile) continue;
                
                if(data.stackpos < tile->items.size()) {
                    oldItem = tile->items[data.stackpos];
                    tile->items.erase(tile->items.begin() + data.stackpos);
                }
            }
            
            if(oldItem) {
                delete oldItem;
            }

            Item* newItem = Item::Create(data.id);
            if(!newItem) continue;

            newItem->setActionID(data.actionId);
            newItem->setUniqueID(data.uniqueId);
            newItem->setText(data.text);
            
            if(data.container) {
                // Insert back into container at same position
                ItemVector& containerItems = data.container->getVector();
                if(data.containerIndex >= containerItems.size()) {
                    containerItems.push_back(newItem);
                } else {
                    containerItems.insert(
                        containerItems.begin() + data.containerIndex,
                        newItem
                    );
                }
            } else {
                // Insert back into tile at same position
                Tile* tile = editor->map.getTile(data.pos);
                if(!tile) continue;
                
                if(data.stackpos >= tile->items.size()) {
                    tile->items.push_back(newItem);
                } else {
                    tile->items.insert(
                        tile->items.begin() + data.stackpos,
                        newItem
                    );
                }
            }
        }

        g_gui.DestroyLoadBar();

        wxString msg;
        msg << itemsToRecreate.size() << " items have been refreshed.";
        g_gui.PopupDialog("Refresh completed", msg, wxOK);

        editor->map.doChange();
        g_gui.RefreshView();
    }
    dialog.Destroy();
}



void MainMenuBar::OnMapValidateGround(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) {
        return;
    }

    if (MapTab* tab = g_gui.GetCurrentMapTab()) {
        if (MapWindow* window = tab->GetView()) {
            window->ShowGroundValidationDialog();
        }
    }
}

void MainMenuBar::OnCreateBorder(wxCommandEvent& WXUNUSED(event)) {
	// Open the Border Editor to create or edit auto-borders
	BorderEditorDialog* dialog = new BorderEditorDialog(g_gui.root, "Auto Border Editor");
	dialog->Show();

	// After editing borders, refresh view to show any changes
	g_gui.RefreshView();
}

void MainMenuBar::OnMapSummarize(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		g_gui.PopupDialog("Error", "No map is currently open!", wxOK | wxICON_ERROR);
		return;
	}

	// Show the map summary window
	g_gui.ShowMapSummaryWindow();
}

void MainMenuBar::OnMapNotes(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) {
        return;
    }

    // Check if the map has a filename (has been saved)
    if (!editor->map.hasFile()) {
        int ret = g_gui.PopupDialog("Map Notes", 
            "You need to save your map before creating notes.\nDo you want to save now?", 
            wxYES | wxNO);
            
        if (ret == wxID_YES) {
            g_gui.SaveMap();
            // Check again if save was successful
            if (!editor->map.hasFile()) {
                return;
            }
        } else {
            return;
        }
    }

    // Create and show the notes window
    NotesWindow* notesWindow = new NotesWindow(frame, editor);
    notesWindow->ShowModal();
    notesWindow->Destroy();
}

void MainMenuBar::OnResetHouseIDs(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) {
        return;
    }

    Map& map = editor->map;
    Houses& houses = map.houses;

    if (houses.count() == 0) {
        g_gui.PopupDialog("Reset House IDs", "No houses found on the map!", wxOK);
        return;
    }

    // Show confirmation dialog with preview
    std::vector<std::pair<uint32_t, uint32_t>> id_changes;
    std::vector<House*> house_list;
    
    // Collect all houses and sort by current ID
    for (HouseMap::iterator it = houses.begin(); it != houses.end(); ++it) {
        house_list.push_back(it->second);
    }
    
    std::sort(house_list.begin(), house_list.end(), [](House* a, House* b) {
        return a->getID() < b->getID();
    });

    // Calculate new IDs (sequential starting from 1)
    for (size_t i = 0; i < house_list.size(); ++i) {
        uint32_t old_id = house_list[i]->getID();
        uint32_t new_id = i + 1;
        if (old_id != new_id) {
            id_changes.push_back(std::make_pair(old_id, new_id));
        }
    }

    if (id_changes.empty()) {
        g_gui.PopupDialog("Reset House IDs", "House IDs are already sequential. No changes needed!", wxOK);
        return;
    }

    // Create dialog showing what will change
    wxDialog* dialog = new wxDialog(frame, wxID_ANY, "Reset House IDs", 
        wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Warning text
    wxStaticText* warning = new wxStaticText(dialog, wxID_ANY, 
        "WARNING: This will reset house IDs to fill gaps and create continuous numbering!\n"
        "This operation will have DATABASE CONSEQUENCES in production servers!\n"
        "House ownership, guest lists, and items may be affected!");
    warning->SetForegroundColour(*wxRED);
    mainSizer->Add(warning, 0, wxALL | wxALIGN_CENTER, 10);

    // Preview list
    wxStaticText* previewLabel = new wxStaticText(dialog, wxID_ANY, "Preview of changes:");
    mainSizer->Add(previewLabel, 0, wxALL, 5);
    
    wxListCtrl* previewList = new wxListCtrl(dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
        wxLC_REPORT | wxLC_SINGLE_SEL);
    previewList->AppendColumn("House Name", wxLIST_FORMAT_LEFT, 300);
    previewList->AppendColumn("Old ID", wxLIST_FORMAT_CENTER, 80);
    previewList->AppendColumn("New ID", wxLIST_FORMAT_CENTER, 80);
    
    for (const auto& change : id_changes) {
        House* house = houses.getHouse(change.first);
        if (house) {
            long index = previewList->InsertItem(previewList->GetItemCount(), wxstr(house->name));
            previewList->SetItem(index, 1, wxString::Format("%d", change.first));
            previewList->SetItem(index, 2, wxString::Format("%d", change.second));
        }
    }
    
    mainSizer->Add(previewList, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "Apply Changes");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(mainSizer);
    dialog->Center();

    if (dialog->ShowModal() == wxID_OK) {
        // Apply the changes
        g_gui.CreateLoadBar("Resetting house IDs...");
        
        try {
            // Create a mapping of old ID to new ID for quick lookup
            std::map<uint32_t, uint32_t> id_mapping;
            for (const auto& change : id_changes) {
                id_mapping[change.first] = change.second;
            }

            // Step 1: Convert house tiles manually (avoid multiple loading dialogs)
            uint64_t tiles_done = 0;
            uint64_t total_tiles = map.getTileCount();
            
            for (MapIterator miter = map.begin(); miter != map.end(); ++miter) {
                if (tiles_done % 0x4000 == 0) {
                    g_gui.SetLoadDone(int((tiles_done * 80) / total_tiles), "Converting house tiles...");
                }
                
                Tile* tile = (*miter)->get();
                if (tile && tile->isHouseTile()) {
                    uint32_t old_house_id = tile->getHouseID();
                    auto it = id_mapping.find(old_house_id);
                    if (it != id_mapping.end()) {
                        tile->setHouseID(it->second);
                    }
                }
                ++tiles_done;
            }

            // Step 2: Update house objects themselves
            g_gui.SetLoadDone(90, "Updating house registry...");
            
            // We need to be careful here to avoid iterator invalidation
            std::vector<House*> houses_to_update;
            for (const auto& change : id_changes) {
                House* house = houses.getHouse(change.first);
                if (house) {
                    houses_to_update.push_back(house);
                }
            }
            
            for (House* house : houses_to_update) {
                uint32_t old_id = house->getID();
                // Find the new ID for this house
                for (const auto& change : id_changes) {
                    if (change.first == old_id) {
                        houses.changeId(house, change.second);
                        break;
                    }
                }
            }

            g_gui.SetLoadDone(100);
            g_gui.DestroyLoadBar();
            
            // Mark map as modified
            map.doChange();
            
            wxString msg;
            msg << "Successfully reset " << id_changes.size() << " house IDs!\n"
                << "House IDs are now sequential from 1 to " << house_list.size() << ".";
            g_gui.PopupDialog("Reset House IDs", msg, wxOK);
            
            // Refresh the view and palettes
            g_gui.RefreshView();
            g_gui.RefreshPalettes();
            
        } catch (...) {
            g_gui.DestroyLoadBar();
            g_gui.PopupDialog("Error", "An error occurred while resetting house IDs!", wxOK | wxICON_ERROR);
        }
    }

    dialog->Destroy();
}

void MainMenuBar::OnResetTownIDs(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) {
        return;
    }

    Map& map = editor->map;
    Towns& towns = map.towns;

    if (towns.count() == 0) {
        g_gui.PopupDialog("Reset Town IDs", "No towns found on the map!", wxOK);
        return;
    }

    // Show confirmation dialog with preview
    std::vector<std::pair<uint32_t, uint32_t>> id_changes;
    std::vector<Town*> town_list;
    
    // Collect all towns and sort by current ID
    for (TownMap::iterator it = towns.begin(); it != towns.end(); ++it) {
        town_list.push_back(it->second);
    }
    
    std::sort(town_list.begin(), town_list.end(), [](Town* a, Town* b) {
        return a->getID() < b->getID();
    });

    // Calculate new IDs (sequential starting from 1)
    for (size_t i = 0; i < town_list.size(); ++i) {
        uint32_t old_id = town_list[i]->getID();
        uint32_t new_id = i + 1;
        if (old_id != new_id) {
            id_changes.push_back(std::make_pair(old_id, new_id));
        }
    }

    if (id_changes.empty()) {
        g_gui.PopupDialog("Reset Town IDs", "Town IDs are already sequential. No changes needed!", wxOK);
        return;
    }

    // Create dialog showing what will change
    wxDialog* dialog = new wxDialog(frame, wxID_ANY, "Reset Town IDs", 
        wxDefaultPosition, wxSize(600, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Warning text
    wxStaticText* warning = new wxStaticText(dialog, wxID_ANY, 
        "WARNING: This will reset town IDs to fill gaps and create continuous numbering!\n"
        "This operation will have DATABASE CONSEQUENCES in production servers!\n"
        "All houses belonging to these towns will be updated to use the new town IDs!");
    warning->SetForegroundColour(*wxRED);
    mainSizer->Add(warning, 0, wxALL | wxALIGN_CENTER, 10);

    // Preview list
    wxStaticText* previewLabel = new wxStaticText(dialog, wxID_ANY, "Preview of changes:");
    mainSizer->Add(previewLabel, 0, wxALL, 5);
    
    wxListCtrl* previewList = new wxListCtrl(dialog, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
        wxLC_REPORT | wxLC_SINGLE_SEL);
    previewList->AppendColumn("Town Name", wxLIST_FORMAT_LEFT, 300);
    previewList->AppendColumn("Old ID", wxLIST_FORMAT_CENTER, 80);
    previewList->AppendColumn("New ID", wxLIST_FORMAT_CENTER, 80);
    previewList->AppendColumn("Houses", wxLIST_FORMAT_CENTER, 80);
    
    for (const auto& change : id_changes) {
        Town* town = towns.getTown(change.first);
        if (town) {
            // Count houses in this town
            int house_count = 0;
            for (HouseMap::iterator house_iter = map.houses.begin(); 
                 house_iter != map.houses.end(); ++house_iter) {
                if (house_iter->second->townid == change.first) {
                    house_count++;
                }
            }
            
            long index = previewList->InsertItem(previewList->GetItemCount(), wxstr(town->getName()));
            previewList->SetItem(index, 1, wxString::Format("%d", change.first));
            previewList->SetItem(index, 2, wxString::Format("%d", change.second));
            previewList->SetItem(index, 3, wxString::Format("%d", house_count));
        }
    }
    
    mainSizer->Add(previewList, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "Apply Changes");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(mainSizer);
    dialog->Center();

    if (dialog->ShowModal() == wxID_OK) {
        // Apply the changes
        g_gui.CreateLoadBar("Resetting town IDs...");
        
        try {
            // Create a mapping of old ID to new ID for quick lookup
            std::map<uint32_t, uint32_t> id_mapping;
            for (const auto& change : id_changes) {
                id_mapping[change.first] = change.second;
            }

            // Step 1: Update all houses that belong to these towns
            g_gui.SetLoadDone(25, "Updating houses with new town IDs...");
            
            for (HouseMap::iterator house_iter = map.houses.begin(); 
                 house_iter != map.houses.end(); ++house_iter) {
                House* house = house_iter->second;
                auto it = id_mapping.find(house->townid);
                if (it != id_mapping.end()) {
                    house->townid = it->second;
                }
            }

            // Step 2: Update town objects themselves
            g_gui.SetLoadDone(75, "Updating town registry...");
            
            // We need to be careful here to avoid iterator invalidation
            std::vector<Town*> towns_to_update;
            for (const auto& change : id_changes) {
                Town* town = towns.getTown(change.first);
                if (town) {
                    towns_to_update.push_back(town);
                }
            }
            
            for (Town* town : towns_to_update) {
                uint32_t old_id = town->getID();
                // Find the new ID for this town
                for (const auto& change : id_changes) {
                    if (change.first == old_id) {
                        towns.changeId(town, change.second);
                        break;
                    }
                }
            }

            g_gui.SetLoadDone(100);
            g_gui.DestroyLoadBar();
            
            // Mark map as modified
            map.doChange();
            
            wxString msg;
            msg << "Successfully reset " << id_changes.size() << " town IDs!\n"
                << "Town IDs are now sequential from 1 to " << town_list.size() << ".";
            g_gui.PopupDialog("Reset Town IDs", msg, wxOK);
            
            // Refresh the view and palettes
            g_gui.RefreshView();
            g_gui.RefreshPalettes();
            
        } catch (...) {
            g_gui.DestroyLoadBar();
            g_gui.PopupDialog("Error", "An error occurred while resetting town IDs!", wxOK | wxICON_ERROR);
        }
    }

    dialog->Destroy();
}

void MainMenuBar::OnDoodadsFillingTool(wxCommandEvent& WXUNUSED(event)) {
    if (!g_gui.IsEditorOpen()) {
        return;
    }

    Editor* editor = g_gui.GetCurrentEditor();
    if (!editor) {
        return;
    }

    Map& map = editor->map;

    // Show the doodads filling dialog
    DoodadsFillingDialog dialog(frame, "Doodads Filling Tool");
    
    if (dialog.ShowModal() == wxID_OK) {
        // Get configuration from dialog
        wxString tileRangesStr = dialog.GetTileRanges();
        std::vector<DoodadsFillingDialog::DoodadItem> doodadItems = dialog.GetDoodadItems();
        int clumping = dialog.GetClumping();
        double clumpingFactor = dialog.GetClumpingFactor();
        int spacing = dialog.GetSpacing();
        bool placeOnSelection = dialog.GetPlaceOnSelection();
        
        // Parse tile ranges
        std::string rangesStr = std::string(tileRangesStr.mb_str());
        std::vector<std::pair<uint16_t, uint16_t>> tileRanges = parseIdRangesString(rangesStr);
        
        if (tileRanges.empty() || doodadItems.empty()) {
            return;
        }
        
        // Calculate total chance for weighted random selection
        int totalChance = 0;
        for (const auto& item : doodadItems) {
            totalChance += item.chance;
        }
        
        if (totalChance <= 0) {
            g_gui.PopupDialog("Error", "Total doodad chances must be greater than 0!", wxOK | wxICON_ERROR);
            return;
        }
        
        // Start the placement process
        g_gui.CreateLoadBar("Placing doodads...");
        
        try {
            BatchAction* batch = editor->actionQueue->createBatch(ACTION_DRAW);
            Action* action = editor->actionQueue->createAction(batch);
            
            std::vector<Position> candidateTiles;
            std::vector<Position> placedPositions;
            int itemsPlaced = 0;
            long long totalTiles = 0;
            long long processedTiles = 0;
            
            // First pass: collect all candidate tiles
            g_gui.SetLoadDone(10, "Finding candidate tiles...");
            
            if (placeOnSelection && editor->selection.size() > 0) {
                // Search within selection
                for (TileSet::iterator it = editor->selection.begin(); it != editor->selection.end(); ++it) {
                    Tile* tile = *it;
                    if (tile && tile->ground) {
                        uint16_t groundId = tile->ground->getID();
                        if (isIdInRanges(groundId, tileRanges)) {
                            candidateTiles.push_back(tile->getPosition());
                        }
                    }
                    totalTiles++;
                }
            } else {
                // Search entire map
                totalTiles = map.getTileCount();
                long long checked = 0;
                
                for (MapIterator it = map.begin(); it != map.end(); ++it) {
                    Tile* tile = (*it)->get();
                    if (tile && tile->ground) {
                        uint16_t groundId = tile->ground->getID();
                        if (isIdInRanges(groundId, tileRanges)) {
                            candidateTiles.push_back(tile->getPosition());
                        }
                    }
                    
                    checked++;
                    if (checked % 10000 == 0) {
                        g_gui.SetLoadDone(10 + (checked * 40 / totalTiles), "Finding candidate tiles...");
                    }
                }
            }
            
            if (candidateTiles.empty()) {
                g_gui.DestroyLoadBar();
                g_gui.PopupDialog("No Tiles Found", "No tiles matching the specified ranges were found!", wxOK | wxICON_INFORMATION);
                return;
            }
            
            g_gui.SetLoadDone(50, "Placing doodads...");
            
            // Initialize random seed
            srand(static_cast<unsigned int>(time(nullptr)));
            
            // Second pass: place doodads with spacing and clumping logic
            for (size_t i = 0; i < candidateTiles.size(); ++i) {
                Position pos = candidateTiles[i];
                
                // Check spacing constraint
                bool tooClose = false;
                for (const Position& placedPos : placedPositions) {
                    int dx = abs(pos.x - placedPos.x);
                    int dy = abs(pos.y - placedPos.y);
                    int dz = abs(pos.z - placedPos.z);
                    
                    // Consider z-level differences as well
                    int distance = std::max({dx, dy, dz});
                    if (distance < spacing) {
                        tooClose = true;
                        break;
                    }
                }
                
                if (tooClose) {
                    continue;
                }
                
                // Apply clumping logic
                double placementChance = clumping / 100.0;
                
                // Modify chance based on proximity to already placed items (clumping factor)
                if (!placedPositions.empty() && clumpingFactor > 1.0) {
                    // Find closest placed item
                    double minDistance = DBL_MAX;
                    for (const Position& placedPos : placedPositions) {
                        double dx = pos.x - placedPos.x;
                        double dy = pos.y - placedPos.y;
                        double distance = sqrt(dx*dx + dy*dy);
                        minDistance = std::min(minDistance, distance);
                    }
                    
                    // Increase chance if close to other placed items
                    if (minDistance < 10.0) { // Within 10 tiles
                        placementChance *= clumpingFactor * (10.0 - minDistance) / 10.0;
                    }
                }
                
                // Random placement check
                if ((rand() % 100) >= (placementChance * 100)) {
                    continue;
                }
                
                // Select a random doodad item based on weights
                int randomChance = rand() % totalChance;
                int currentChance = 0;
                uint16_t selectedItemId = 0;
                
                for (const auto& item : doodadItems) {
                    currentChance += item.chance;
                    if (randomChance < currentChance) {
                        selectedItemId = item.id;
                        break;
                    }
                }
                
                if (selectedItemId == 0) {
                    continue;
                }
                
                // Place the item
                Tile* tile = map.getTile(pos);
                if (tile) {
                    Item* newItem = Item::Create(selectedItemId);
                    if (newItem) {
                        // Create a copy of the tile for modification
                        Tile* newTile = tile->deepCopy(map);
                        newTile->addItem(newItem);
                        action->addChange(newd Change(newTile));
                        placedPositions.push_back(pos);
                        itemsPlaced++;
                    }
                }
                
                // Update progress
                if (i % 1000 == 0) {
                    int progress = 50 + (i * 45 / candidateTiles.size());
                    g_gui.SetLoadDone(progress, wxString::Format("Placing doodads... (%d placed)", itemsPlaced));
                }
            }
            
            g_gui.SetLoadDone(100);
            g_gui.DestroyLoadBar();
            
            // Commit the action
            editor->actionQueue->addBatch(batch);
            
            // Show results
            wxString message = wxString::Format(
                "Doodads filling completed!\n\n"
                "Candidate tiles found: %zu\n"
                "Items placed: %d\n"
                "Placement success rate: %.1f%%",
                candidateTiles.size(),
                itemsPlaced,
                candidateTiles.empty() ? 0.0 : (itemsPlaced * 100.0 / candidateTiles.size())
            );
            
            g_gui.PopupDialog("Doodads Filling Complete", message, wxOK | wxICON_INFORMATION);
            
            // Refresh the view
            g_gui.RefreshView();
            
        } catch (const std::exception& e) {
            g_gui.DestroyLoadBar();
            wxString errorMsg = wxString::Format("Error during doodads placement: %s", e.what());
            g_gui.PopupDialog("Error", errorMsg, wxOK | wxICON_ERROR);
        } catch (...) {
            g_gui.DestroyLoadBar();
            g_gui.PopupDialog("Error", "An unknown error occurred during doodads placement!", wxOK | wxICON_ERROR);
        }
    }
}

void MainMenuBar::OnEditItemsOTB(wxCommandEvent& WXUNUSED(event)) {
	// Check if items are loaded
	if (!g_gui.IsVersionLoaded()) {
		wxMessageBox("Please load a client version first.", "No Version Loaded", wxOK | wxICON_WARNING, frame);
		return;
	}

	// Create and show the item editor window
	ItemEditorWindow* itemEditor = new ItemEditorWindow(frame);
	itemEditor->ShowModal();
	itemEditor->Destroy();
}

