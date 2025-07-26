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
 * TASK: Refresh Custom Palette After Selection to Doodad Conversion POSTPHONED IGNORE UNTIL THIS DOES NOT SAY IGNORE :D
 * --------------------------------------------------------------
 * 
 * Current Behavior:
 * - Selection to Doodad menu item creates a doodad brush from selected items
 * - Brush is saved as XML but not immediately visible in palette
 * 
 * Required Changes:
 * 1. Palette Update
 *    - After doodad creation, refresh custom brushes palette
 *    - New doodad should appear at the top of custom brushes list
 *    - Maintain proper sorting/grouping of brushes
 * 
 * Implementation Steps:
 * 1. Add Palette Refresh Function
 *    - Create method in GUI class to reload custom brushes
 *    - Clear existing custom brush entries
 *    - Reload all XML brush definitions
 *    - Sort brushes according to current palette settings
 * 
 * 2. Update Selection to Doodad Handler
 *    - After successful doodad creation and XML save
 *    - Call palette refresh function
 *    - Select newly created brush in palette
 * 
 * 3. XML Integration
 *    - Ensure proper XML file naming for new doodad
 *    - Maintain brush metadata for palette organization
 *    - Handle file paths consistently
 * 
 * Technical Requirements:
 * - Access to palette window through g_gui
 * - Proper brush management in BrushManager
 * - XML file handling for brush definitions
 * - Maintain undo/redo compatibility
 * 
 * Files to Modify:
 * - gui.cpp/h: Add refresh function
 * - palette_window.cpp: Update brush list
 * - doodad_brush.cpp: XML handling
 * 
 * Related Code:
 * - Selection to Doodad conversion (main_menubar.cpp)
 * - Palette management (palette_window.cpp)
 * - Brush XML handling (brush.cpp)
 */

/*
 * TASK: Refresh Custom Palette After Selection to Doodad Conversion
 * --------------------------------------------------------------
 * 
 * IMPLEMENTATION OPTIONS:
 * 
 * 1. QUICK REFRESH (Simple but Inefficient)
 * ---------------------------------------
 * - Use existing PaletteWindow::InvalidateContents()
 * - Call g_gui.RefreshPalettes() after doodad creation
 * Code:
 * ```cpp
 * PaletteWindow* palette = dynamic_cast<PaletteWindow*>(g_gui.GetPalette());
 * if(palette) {
 *     palette->InvalidateContents();
 *     palette->SelectPage(TILESET_DOODAD);
 * }
 * g_gui.RefreshPalettes();
 * ```
 * Pros: Simple, uses existing methods
 * Cons: Reloads entire palette, inefficient
 * 
 * 2. TARGETED REFRESH (Balanced)
 * ---------------------------
 * - Add refresh method to BrushPalettePanel
 * - Only reload doodad brushes section
 * - Update specific palette page
 * Code:
 * ```cpp
 * class BrushPalettePanel {
 *     void ReloadDoodadBrushes() {
 *         // Clear and reload only doodad brushes
 *         LoadDoodadBrushes();
 *         Refresh();
 *     }
 * };
 * ```
 * Pros: More efficient, better UX
 * Cons: Requires new methods in palette classes
 * 
 * 3. REACTIVE SYSTEM (Complex but Robust)
 * -----------------------------------
 * - Implement observer pattern for brush changes
 * - Palettes subscribe to brush manager updates
 * - Auto-refresh when brushes change
 * Code:
 * ```cpp
 * class BrushManager {
 *     std::vector<BrushObserver*> observers;
 *     void NotifyBrushChange(BrushCategory category) {
 *         for(auto* observer : observers) {
 *             observer->OnBrushesChanged(category);
 *         }
 *     }
 * };
 * ```
 * Pros: Clean architecture, handles all brush changes
 * Cons: Major refactoring required
 * 
 * 4. HYBRID APPROACH (Recommended)
 * -----------------------------
 * - Use existing GUI methods but add targeted refresh
 * - Add doodad-specific refresh to PaletteWindow
 * - Maintain current architecture while improving efficiency
 * Code:
 * ```cpp
 * class PaletteWindow {
 *     void RefreshDoodadPalette() {
 *         // Refresh only doodad page
 *         if(brush_panel) {
 *             brush_panel->LoadDoodadBrushes();
 *             SelectPage(TILESET_DOODAD);
 *         }
 *     }
 * };
 * ```
 * Pros: 
 * - Balance of efficiency and simplicity
 * - Minimal changes to existing code
 * - Maintains current architecture
 * Cons:
 * - Still requires some new methods
 * - Not as elegant as full reactive system
 */
#include "main.h"

#include <wx/display.h>
#include <wx/dir.h>

#include "gui.h"
#include "main_menubar.h"

#include "editor.h"
#include "brush.h"
#include "map.h"
#include "sprites.h"
#include "materials.h"
#include "doodad_brush.h"
#include "spawn_brush.h"

#include "common_windows.h"
#include "result_window.h"
#include "map_summary_window.h"
#include "minimap_window.h"
#include "palette_window.h"
#include "map_display.h"
#include "application.h"
#include "welcome_dialog.h"

#include "live_client.h"
#include "live_tab.h"
#include "live_server.h"
#include "recent_brushes_window.h"
#include "monster_maker_window.h"
#include "dark_mode_manager.h"
#include <wx/regex.h>

#ifdef __WXOSX__
	#include <AGL/agl.h>
#endif

const wxEventType EVT_UPDATE_MENUS = wxNewEventType();

// Global GUI instance
GUI g_gui;

// GUI class implementation
GUI::GUI() :
	aui_manager(nullptr),
	root(nullptr),
	minimap_enabled(false),
	mode(DRAWING_MODE),
	pasting(false),
	hotkeys_enabled(true),
	search_result_window(nullptr),
	map_summary_window(nullptr),
	monster_maker_window(nullptr),
	loaded_version(CLIENT_VERSION_NONE),
	secondary_map(nullptr),
	minimap(nullptr),
	recent_brushes_window(nullptr),
	has_last_search(false),
	last_search_itemid(0),
	last_search_on_selection(false),
	last_ignored_ids_enabled(false),
	creature_spawntime(0),
	gem(nullptr),
	doodad_buffer_map(std::make_unique<BaseMap>()),
	house_brush(nullptr),
	house_exit_brush(nullptr),
	waypoint_brush(nullptr),
	optional_brush(nullptr),
	eraser(nullptr),
	spawn_brush(nullptr),
	normal_door_brush(nullptr),
	locked_door_brush(nullptr),
	magic_door_brush(nullptr),
	quest_door_brush(nullptr),
	hatch_door_brush(nullptr),
	normal_door_alt_brush(nullptr),
	archway_door_brush(nullptr),
	window_door_brush(nullptr),
	pz_brush(nullptr),
	rook_brush(nullptr),
	nolog_brush(nullptr),
	pvp_brush(nullptr),
	OGLContext(nullptr),
	current_brush(nullptr),
	previous_brush(nullptr),
	brush_shape(BRUSHSHAPE_SQUARE),
	brush_size(1), // Initialize to 1 instead of 0 to prevent division by zero
	brush_variation(0),
	draw_locked_doors(false),
	use_custom_thickness(false),
	custom_thickness_mod(0.0),
	use_custom_brush_size(false),
	custom_brush_width(0),
	custom_brush_height(0),
	progressBar(nullptr),
	progressFrom(0),
	progressTo(0),
	currentProgress(0),
	winDisabler(nullptr),
	disabled_counter(0),
	last_autosave(time(nullptr)),
	last_autosave_check(time(nullptr))
{
}

GUI::~GUI() {
    // Close all editors before deleting other resources
    if (tabbook) {
        CloseAllEditors();
    }
    
    // Clean up brushes
    CleanupBrushes();
    
    // Close any remaining detached/dockable views
    for (auto& view_pair : detached_views) {
        for (wxFrame* frame : view_pair.second) {
            frame->Close(true);
        }
    }
    detached_views.clear();
    
    for (auto& view_pair : dockable_views) {
        for (MapWindow* window : view_pair.second) {
            if (aui_manager && aui_manager->GetPane(window).IsOk()) {
                aui_manager->DetachPane(window);
                window->Destroy();
            }
        }
    }
    dockable_views.clear();
    
    // doodad_buffer_map and OGLContext are now managed by unique_ptr
    
    // NOTE: aui_manager is deleted by MainFrame's destructor
}

void GUI::CleanupBrushes() {
    // The GUI doesn't own the brushes, they're owned by g_brushes
    // Just nullify the pointers to avoid dangling references
    house_brush = nullptr;
    house_exit_brush = nullptr;
    waypoint_brush = nullptr;
    optional_brush = nullptr;
    eraser = nullptr;
    spawn_brush = nullptr;
    normal_door_brush = nullptr;
    locked_door_brush = nullptr;
    magic_door_brush = nullptr;
    quest_door_brush = nullptr;
    hatch_door_brush = nullptr;
    normal_door_alt_brush = nullptr;
    archway_door_brush = nullptr;
    window_door_brush = nullptr;
    pz_brush = nullptr;
    rook_brush = nullptr;
    nolog_brush = nullptr;
    pvp_brush = nullptr;
    
    current_brush = nullptr;
    previous_brush = nullptr;
}

wxGLContext* GUI::GetGLContext(wxGLCanvas* win) {
	if (!OGLContext) {
#ifdef __WXOSX__
		/*
		wxGLContext(AGLPixelFormat fmt, wxGLCanvas *win,
					const wxPalette& WXUNUSED(palette),
					const wxGLContext *other
					);
		*/
		OGLContext = std::unique_ptr<wxGLContext>(new wxGLContext(win, nullptr));
#else
		OGLContext = std::unique_ptr<wxGLContext>(newd wxGLContext(win));
#endif
	}

	return OGLContext.get();
}

wxString GUI::GetDataDirectory() {
	std::string cfg_str = g_settings.getString(Config::DATA_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}

	// Silently reset directory
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast) {
		throw; // Crash application (this should never happend anyways...)
	}

	exec_directory.AppendDir("data");
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetExecDirectory() {
	// Silently reset directory
	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast) {
		wxLogError("Could not fetch executable directory.");
	}
	return exec_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

wxString GUI::GetLocalDataDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
		;
	} else {
		FileName dir = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.AppendDir("data");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString GUI::GetLocalDirectory() {
	if (g_settings.getInteger(Config::INDIRECTORY_INSTALLATION)) {
		FileName dir = GetDataDirectory();
		dir.AppendDir("user");
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
		;
	} else {
		FileName dir = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetUserDataDir();
#ifdef __WINDOWS__
		dir.AppendDir("Remere's Map Editor");
#else
		dir.AppendDir(".rme");
#endif
		dir.Mkdir(0755, wxPATH_MKDIR_FULL);
		return dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	}
}

wxString GUI::GetExtensionsDirectory() {
	std::string cfg_str = g_settings.getString(Config::EXTENSIONS_DIRECTORY);
	if (!cfg_str.empty()) {
		FileName dir;
		dir.Assign(wxstr(cfg_str));
		wxString path;
		if (dir.DirExists()) {
			path = dir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			return path;
		}
	}

	// Silently reset directory
	FileName local_directory = GetLocalDirectory();
	local_directory.AppendDir("extensions");
	local_directory.Mkdir(0755, wxPATH_MKDIR_FULL);
	return local_directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
}

void GUI::discoverDataDirectory(const wxString& existentFile) {
	wxString currentDir = wxGetCwd();
	wxString execDir = GetExecDirectory();

	wxString possiblePaths[] = {
		execDir,
		currentDir + "/",

		// these are used usually when running from build directories
		execDir + "/../",
		execDir + "/../../",
		execDir + "/../../../",
		currentDir + "/../",
	};

	bool found = false;
	for (const wxString& path : possiblePaths) {
		if (wxFileName(path + "data/" + existentFile).FileExists()) {
			m_dataDirectory = path + "data/";
			found = true;
			break;
		}
	}

	if (!found) {
		wxLogError(wxString() + "Could not find data directory.\n");
	}
}

bool GUI::LoadVersion(ClientVersionID version, wxString& error, wxArrayString& warnings, bool force) {
	if (ClientVersion::get(version) == nullptr) {
		error = "Unsupported client version! (8)";
		return false;
	}

	if (version != loaded_version || force) {
		if (getLoadedVersion() != nullptr) {
			// There is another version loaded right now, save window layout
			g_gui.SavePerspective();
		}

		// Disable all rendering so the data is not accessed while reloading
		UnnamedRenderingLock();
		DestroyPalettes();
		DestroyMinimap();

		// Destroy the previous version
		UnloadVersion();

		loaded_version = version;
		if (!getLoadedVersion()->hasValidPaths()) {
			if (!getLoadedVersion()->loadValidPaths()) {
				error = "Couldn't load relevant asset files";
				loaded_version = CLIENT_VERSION_NONE;
				return false;
			}
		}

		bool ret = LoadDataFiles(error, warnings);
		if (ret) {
			g_gui.LoadPerspective();
		} else {
			loaded_version = CLIENT_VERSION_NONE;
		}

		return ret;
	}
	return true;
}

void GUI::EnableHotkeys() {
	hotkeys_enabled = true;
}

void GUI::DisableHotkeys() {
	hotkeys_enabled = false;
}

bool GUI::AreHotkeysEnabled() const {
	return hotkeys_enabled;
}

ClientVersionID GUI::GetCurrentVersionID() const {
	if (loaded_version != CLIENT_VERSION_NONE) {
		return getLoadedVersion()->getID();
	}
	return CLIENT_VERSION_NONE;
}

const ClientVersion& GUI::GetCurrentVersion() const {
	assert(loaded_version);
	return *getLoadedVersion();
}

void GUI::CycleTab(bool forward) {
	tabbook->CycleTab(forward);
}

bool GUI::LoadDataFiles(wxString& error, wxArrayString& warnings) {
	minimap_enabled = false;
	FileName data_path = getLoadedVersion()->getDataPath();
	FileName client_path = getLoadedVersion()->getClientPath();
	FileName extension_path = GetExtensionsDirectory();

	FileName exec_directory;
	try {
		exec_directory = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (std::bad_cast&) {
		error = "Couldn't establish working directory...";
		return false;
	}

	g_gui.gfx.client_version = getLoadedVersion();

	if (!g_gui.gfx.loadOTFI(client_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), error, warnings)) {
		error = "Couldn't load otfi file: " + error;
		g_gui.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_gui.CreateLoadBar("Loading asset files");
	g_gui.SetLoadDone(0, "Loading metadata file...");

	wxFileName metadata_path = g_gui.gfx.getMetadataFileName();
	if (!g_gui.gfx.loadSpriteMetadata(metadata_path, error, warnings)) {
		error = "Couldn't load metadata: " + error;
		g_gui.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_gui.SetLoadDone(10, "Loading sprites file...");

	wxFileName sprites_path = g_gui.gfx.getSpritesFileName();
	if (!g_gui.gfx.loadSpriteData(sprites_path.GetFullPath(), error, warnings)) {
		error = "Couldn't load sprites: " + error;
		g_gui.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_gui.SetLoadDone(20, "Loading items.otb file...");
	wxString otb_path;
	if (g_settings.getBoolean(Config::FORCE_CLIENT_ITEMS_OTB)) {
		// Load from client folder
		otb_path = client_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.otb";
	} else {
		// Load from data folder (default behavior)
		otb_path = data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.otb";
	}
	if (!g_items.loadFromOtb(otb_path, error, warnings)) {
		error = "Couldn't load items.otb: " + error;
		g_gui.DestroyLoadBar();
		UnloadVersion();
		return false;
	}

	g_gui.SetLoadDone(30, "Loading items.xml ...");
	wxString xml_path;
	if (g_settings.getBoolean(Config::FORCE_CLIENT_ITEMS_OTB)) {
		// Load from client folder
		xml_path = client_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.xml";
	} else {
		// Load from data folder (default behavior)
		xml_path = data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "items.xml";
	}
	if (!g_items.loadFromGameXml(xml_path, error, warnings)) {
		warnings.push_back("Couldn't load items.xml: " + error);
	}

	g_gui.SetLoadDone(45, "Loading creatures.xml ...");
	if (!g_creatures.loadFromXML(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "creatures.xml"), true, error, warnings)) {
		warnings.push_back("Couldn't load creatures.xml: " + error);
	}

	g_gui.SetLoadDone(45, "Loading user creatures.xml ...");
	{
		FileName cdb = getLoadedVersion()->getLocalDataPath();
		cdb.SetFullName("creatures.xml");
		wxString nerr;
		wxArrayString nwarn;
		g_creatures.loadFromXML(cdb, false, nerr, nwarn);
	}

	g_gui.SetLoadDone(50, "Loading materials.xml ...");
	if (!g_materials.loadMaterials(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "materials.xml"), error, warnings)) {
		warnings.push_back("Couldn't load materials.xml: " + error);
	}
	
	g_gui.SetLoadDone(60, "Loading collections.xml ...");
	if (!g_materials.loadMaterials(wxString(data_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "collections.xml"), error, warnings)) {
		warnings.push_back("Couldn't load collections.xml: " + error);
	}
	
	g_gui.SetLoadDone(70, "Loading extensions...");
	if (!g_materials.loadExtensions(extension_path, error, warnings)) {
		// warnings.push_back("Couldn't load extensions: " + error);
	}

	g_gui.SetLoadDone(70, "Finishing...");
	g_brushes.init();
	g_materials.createOtherTileset();

	g_gui.DestroyLoadBar();
	return true;
}

void GUI::UnloadVersion() {
	UnnamedRenderingLock();
	gfx.clear();
	current_brush = nullptr;
	previous_brush = nullptr;

	house_brush = nullptr;
	house_exit_brush = nullptr;
	waypoint_brush = nullptr;
	optional_brush = nullptr;
	eraser = nullptr;
	normal_door_brush = nullptr;
	locked_door_brush = nullptr;
	magic_door_brush = nullptr;
	quest_door_brush = nullptr;
	hatch_door_brush = nullptr;
	window_door_brush = nullptr;

	if (loaded_version != CLIENT_VERSION_NONE) {
		// Close all detached and dockable views
		for (auto it = detached_views.begin(); it != detached_views.end();) {
			auto editor_it = it;
			++it; // Advance iterator before we potentially erase elements
			
			// Close all views for this editor
			CloseDetachedViews(editor_it->first);
		}
		
		// g_gui.UnloadVersion();
		g_materials.clear();
		g_brushes.clear();
		g_items.clear();
		gfx.clear();

		FileName cdb = getLoadedVersion()->getLocalDataPath();
		cdb.SetFullName("creatures.xml");
		g_creatures.saveToXML(cdb);
		g_creatures.clear();

		loaded_version = CLIENT_VERSION_NONE;
	}
}

void GUI::SaveCurrentMap(FileName filename, bool showdialog) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		Editor* editor = mapTab->GetEditor();
		if (editor) {
			editor->saveMap(filename, showdialog);

			const std::string& filename = editor->map.getFilename();
			const Position& position = mapTab->GetScreenCenterPosition();
			std::ostringstream stream;
			stream << position;
			g_settings.setString(Config::RECENT_EDITED_MAP_PATH, filename);
			g_settings.setString(Config::RECENT_EDITED_MAP_POSITION, stream.str());
		}
	}

	UpdateTitle();
	root->UpdateMenubar();
	root->Refresh();
}

bool GUI::IsEditorOpen() const {
	return tabbook != nullptr && GetCurrentMapTab();
}

double GUI::GetCurrentZoom() {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		return tab->GetCanvas()->GetZoom();
	}
	return 1.0;
}

void GUI::SetCurrentZoom(double zoom) {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		tab->GetCanvas()->SetZoom(zoom);
	}
}

void GUI::FitViewToMap() {
	for (int index = 0; index < tabbook->GetTabCount(); ++index) {
		if (auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(index))) {
			tab->GetView()->FitToMap();
		}
	}
}

void GUI::FitViewToMap(MapTab* mt) {
	for (int index = 0; index < tabbook->GetTabCount(); ++index) {
		if (auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(index))) {
			if (tab->HasSameReference(mt)) {
				tab->GetView()->FitToMap();
			}
		}
	}
}

bool GUI::NewMap() {
	FinishWelcomeDialog();

	Editor* editor;
	try {
		editor = newd Editor(copybuffer);
	} catch (std::runtime_error& e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK);
		return false;
	}

	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);
	editor->map.clearChanges();

	SetStatusText("Created new map");
	UpdateTitle();
	RefreshPalettes();
	root->UpdateMenubar();
	root->Refresh();

	return true;
}

void GUI::OpenMap() {
	wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? MAP_LOAD_FILE_WILDCARD_OTGZ : MAP_LOAD_FILE_WILDCARD;
	wxFileDialog dialog(root, "Open map file", wxEmptyString, wxEmptyString, wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialog.ShowModal() == wxID_OK) {
		LoadMap(dialog.GetPath());
	}
}

void GUI::SaveMap() {
	if (!IsEditorOpen()) {
		return;
	}

	if (GetCurrentMap().hasFile()) {
		SaveCurrentMap(true);
	} else {
		wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? MAP_SAVE_FILE_WILDCARD_OTGZ : MAP_SAVE_FILE_WILDCARD;
		wxFileDialog dialog(root, "Save...", wxEmptyString, wxEmptyString, wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if (dialog.ShowModal() == wxID_OK) {
			SaveCurrentMap(dialog.GetPath(), true);
		}
	}
}

void GUI::SaveMapAs() {
	if (!IsEditorOpen()) {
		return;
	}

	wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? MAP_SAVE_FILE_WILDCARD_OTGZ : MAP_SAVE_FILE_WILDCARD;
	wxFileDialog dialog(root, "Save As...", "", "", wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_OK) {
		SaveCurrentMap(dialog.GetPath(), true);
		UpdateTitle();
		root->menu_bar->AddRecentFile(dialog.GetPath());
		root->UpdateMenubar();
	}
}

bool GUI::LoadMap(const FileName& fileName) {
	FinishWelcomeDialog();

	if (GetCurrentEditor() && !GetCurrentMap().hasChanged() && !GetCurrentMap().hasFile()) {
		g_gui.CloseCurrentEditor();
	}

	Editor* editor;
	try {
		editor = newd Editor(copybuffer, fileName);
	} catch (std::runtime_error& e) {
		PopupDialog(root, "Error!", wxString(e.what(), wxConvUTF8), wxOK);
		return false;
	}

	auto* mapTab = newd MapTab(tabbook, editor);
	mapTab->OnSwitchEditorMode(mode);

	root->AddRecentFile(fileName);

	mapTab->GetView()->FitToMap();
	UpdateTitle();
	

	
	root->DoQueryImportCreatures();

	FitViewToMap(mapTab);
	root->UpdateMenubar();

	// Pre-cache the entire minimap for smooth performance
	if (minimap && IsMinimapVisible()) {
		minimap->PreCacheEntireMap();
	}

	// Load OTS scripts directory for the map
	wxFileName mapPath(fileName.GetFullPath());
	wxString mapDir = mapPath.GetPath();
	
	// Check if we can find an OTS data directory
	bool scriptsLoaded = false;
	bool monstersLoaded = false;
	bool npcsLoaded = false;
	
	// Strategy 0: Use custom OTS Data directory if set
	std::string customDataPath = g_settings.getString(Config::OTS_DATA_DIRECTORY);
	if (!customDataPath.empty()) {
		wxString customPath = wxstr(customDataPath);
		if (wxDir::Exists(customPath)) {
			if (revscript_manager.scanRevScriptsDirectory(customDataPath)) {
				scriptsLoaded = true;
			}
			if (monster_manager.scanMonstersDirectory(customDataPath)) {
				monstersLoaded = true;
			}
			if (npc_manager.scanNPCDirectory(customDataPath)) {
				npcsLoaded = true;
			}
			if (scriptsLoaded || monstersLoaded || npcsLoaded) {
				SetStatusText("OTS data directory loaded (custom): " + customPath);
			}
		}
	}
	
	// Strategy 1: Look for data directory in same directory as map
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxString dataPath = mapDir + wxFileName::GetPathSeparator() + "data";
		if (wxDir::Exists(dataPath)) {
			if (revscript_manager.scanRevScriptsDirectory(nstr(dataPath))) {
				scriptsLoaded = true;
			}
			if (monster_manager.scanMonstersDirectory(nstr(dataPath))) {
				monstersLoaded = true;
			}
			if (npc_manager.scanNPCDirectory(nstr(dataPath))) {
				npcsLoaded = true;
			}
			if (scriptsLoaded || monstersLoaded || npcsLoaded) {
				SetStatusText("OTS data directory loaded: " + dataPath);
			}
		}
	}
	
	// Strategy 2: Go up one directory level and look for data
	// This handles the common case where map is in /data/world/ and we need to find /data/
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxFileName parentPath(mapDir);
		if (parentPath.GetDirCount() > 0) {
			parentPath.RemoveLastDir();  // Go up one level (from world to data)
			wxString parentDataPath = parentPath.GetPath();
			
			// Check if this looks like the data directory (contains scripts, actions, movements, monsters, or npcs)
			wxString scriptsCheck = parentDataPath + wxFileName::GetPathSeparator() + "scripts";
			wxString actionsCheck = parentDataPath + wxFileName::GetPathSeparator() + "actions";
			wxString monstersCheck = parentDataPath + wxFileName::GetPathSeparator() + "monsters";
			wxString npcsCheck = parentDataPath + wxFileName::GetPathSeparator() + "npc";
			if (wxDir::Exists(scriptsCheck) || wxDir::Exists(actionsCheck) || wxDir::Exists(monstersCheck) || wxDir::Exists(npcsCheck)) {
				if (revscript_manager.scanRevScriptsDirectory(nstr(parentDataPath))) {
					scriptsLoaded = true;
				}
				if (monster_manager.scanMonstersDirectory(nstr(parentDataPath))) {
					monstersLoaded = true;
				}
				if (npc_manager.scanNPCDirectory(nstr(parentDataPath))) {
					npcsLoaded = true;
				}
				if (scriptsLoaded || monstersLoaded || npcsLoaded) {
					SetStatusText("OTS data directory loaded: " + parentDataPath);
				}
			}
		}
	}
	
	// Strategy 3: Look for data directory as sibling to map directory
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxFileName mapParent(mapDir);
		if (mapParent.GetDirCount() > 0) {
			mapParent.RemoveLastDir();  // Go up to parent of map directory
			wxString siblingDataPath = mapParent.GetPath() + wxFileName::GetPathSeparator() + "data";
			if (wxDir::Exists(siblingDataPath)) {
				if (revscript_manager.scanRevScriptsDirectory(nstr(siblingDataPath))) {
					scriptsLoaded = true;
				}
				if (monster_manager.scanMonstersDirectory(nstr(siblingDataPath))) {
					monstersLoaded = true;
				}
				if (npc_manager.scanNPCDirectory(nstr(siblingDataPath))) {
					npcsLoaded = true;
				}
				if (scriptsLoaded || monstersLoaded || npcsLoaded) {
					SetStatusText("OTS data directory loaded: " + siblingDataPath);
				}
			}
		}
	}
	
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		SetStatusText("No OTS data directory found");
	}

	std::string path = g_settings.getString(Config::RECENT_EDITED_MAP_PATH);
	if (!path.empty()) {
		FileName file(path);
		if (file == fileName) {
			std::istringstream stream(g_settings.getString(Config::RECENT_EDITED_MAP_POSITION));
			Position position;
			stream >> position;
			mapTab->SetScreenCenterPosition(position);
		}
	}
	return true;
}

void GUI::ReloadRevScripts() {
	if (!IsEditorOpen()) {
		SetStatusText("No map open - cannot reload scripts");
		return;
	}
	
	// Get the current map file path
	Editor* editor = GetCurrentEditor();
	if (!editor || !editor->map.hasFile()) {
		SetStatusText("Map has no file - cannot determine OTS data directory");
		return;
	}
	
	FileName fileName(wxstr(editor->map.getFilename()));
	wxFileName mapPath(fileName.GetFullPath());
	wxString mapDir = mapPath.GetPath();
	
	// Check if we can find an OTS data directory
	bool scriptsLoaded = false;
	bool monstersLoaded = false;
	bool npcsLoaded = false;
	
	// Strategy 0: Use custom OTS Data directory if set
	// ADDITIONALY THERE SHOULD BE FALLBACK TO CURRENTLY LOADED MAP DIR cd ..
	// since the map dir 99% of time is /data/world which means we are 1 back dir from /data folder :D
	std::string customDataPath = g_settings.getString(Config::OTS_DATA_DIRECTORY);
	if (!customDataPath.empty()) {
		wxString customPath = wxstr(customDataPath);
		if (wxDir::Exists(customPath)) {
			if (revscript_manager.scanRevScriptsDirectory(customDataPath)) {
				scriptsLoaded = true;
			}
			if (monster_manager.scanMonstersDirectory(customDataPath)) {
				monstersLoaded = true;
			}
			if (npc_manager.scanNPCDirectory(customDataPath)) {
				npcsLoaded = true;
			}
			if (scriptsLoaded || monstersLoaded || npcsLoaded) {
				SetStatusText("OTS data directory reloaded (custom): " + customPath);
			}
		}
	}
	
	// Strategy 1: Look for data directory in same directory as map
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxString dataPath = mapDir + wxFileName::GetPathSeparator() + "data";
		if (wxDir::Exists(dataPath)) {
			if (revscript_manager.scanRevScriptsDirectory(nstr(dataPath))) {
				scriptsLoaded = true;
			}
			if (monster_manager.scanMonstersDirectory(nstr(dataPath))) {
				monstersLoaded = true;
			}
			if (npc_manager.scanNPCDirectory(nstr(dataPath))) {
				npcsLoaded = true;
			}
			if (scriptsLoaded || monstersLoaded || npcsLoaded) {
				SetStatusText("OTS data directory reloaded: " + dataPath);
			}
		}
	}
	
	// Strategy 2: Go up one directory level and look for data
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxFileName parentPath(mapDir);
		if (parentPath.GetDirCount() > 0) {
			parentPath.RemoveLastDir();  // Go up one level (from world to data)
			wxString parentDataPath = parentPath.GetPath();
			
			// Check if this looks like the data directory (contains scripts, actions, movements, monsters, or npcs)
			wxString scriptsCheck = parentDataPath + wxFileName::GetPathSeparator() + "scripts";
			wxString actionsCheck = parentDataPath + wxFileName::GetPathSeparator() + "actions";
			wxString monstersCheck = parentDataPath + wxFileName::GetPathSeparator() + "monsters";
			wxString npcsCheck = parentDataPath + wxFileName::GetPathSeparator() + "npc";
			if (wxDir::Exists(scriptsCheck) || wxDir::Exists(actionsCheck) || wxDir::Exists(monstersCheck) || wxDir::Exists(npcsCheck)) {
				if (revscript_manager.scanRevScriptsDirectory(nstr(parentDataPath))) {
					scriptsLoaded = true;
				}
				if (monster_manager.scanMonstersDirectory(nstr(parentDataPath))) {
					monstersLoaded = true;
				}
				if (npc_manager.scanNPCDirectory(nstr(parentDataPath))) {
					npcsLoaded = true;
				}
				if (scriptsLoaded || monstersLoaded || npcsLoaded) {
					SetStatusText("OTS data directory reloaded: " + parentDataPath);
				}
			}
		}
	}
	
	// Strategy 3: Look for data directory as sibling to map directory
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		wxFileName mapParent(mapDir);
		if (mapParent.GetDirCount() > 0) {
			mapParent.RemoveLastDir();  // Go up to parent of map directory
			wxString siblingDataPath = mapParent.GetPath() + wxFileName::GetPathSeparator() + "data";
			if (wxDir::Exists(siblingDataPath)) {
				if (revscript_manager.scanRevScriptsDirectory(nstr(siblingDataPath))) {
					scriptsLoaded = true;
				}
				if (monster_manager.scanMonstersDirectory(nstr(siblingDataPath))) {
					monstersLoaded = true;
				}
				if (npc_manager.scanNPCDirectory(nstr(siblingDataPath))) {
					npcsLoaded = true;
				}
				if (scriptsLoaded || monstersLoaded || npcsLoaded) {
					SetStatusText("OTS data directory reloaded: " + siblingDataPath);
				}
			}
		}
	}
	
	if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
		SetStatusText("No OTS data directory found for reload");
	}
}

void GUI::ReloadNPCs() {
	if (!IsEditorOpen()) {
		SetStatusText("No map open - cannot reload NPCs");
		return;
	}
	
	// Get the current map file path
	Editor* editor = GetCurrentEditor();
	if (!editor || !editor->map.hasFile()) {
		SetStatusText("Map has no file - cannot determine OTS data directory");
		return;
	}
	
	FileName fileName(wxstr(editor->map.getFilename()));
	wxFileName mapPath(fileName.GetFullPath());
	wxString mapDir = mapPath.GetPath();
	
	// Check if we can find an OTS data directory
	bool npcsLoaded = false;
	
	// Strategy 0: Use custom OTS Data directory if set
	std::string customDataPath = g_settings.getString(Config::OTS_DATA_DIRECTORY);
	if (!customDataPath.empty()) {
		wxString customPath = wxstr(customDataPath);
		if (wxDir::Exists(customPath)) {
			if (npc_manager.scanNPCDirectory(customDataPath)) {
				npcsLoaded = true;
				SetStatusText("NPCs reloaded (custom): " + customPath);
			}
		}
	}
	
	// Strategy 1: Look for data directory in same directory as map
	if (!npcsLoaded) {
		wxString dataPath = mapDir + wxFileName::GetPathSeparator() + "data";
		if (wxDir::Exists(dataPath)) {
			if (npc_manager.scanNPCDirectory(nstr(dataPath))) {
				npcsLoaded = true;
				SetStatusText("NPCs reloaded: " + dataPath);
			}
		}
	}
	
	// Strategy 2: Go up one directory level and look for data
	if (!npcsLoaded) {
		wxFileName parentPath(mapDir);
		if (parentPath.GetDirCount() > 0) {
			parentPath.RemoveLastDir();  // Go up one level (from world to data)
			wxString parentDataPath = parentPath.GetPath();
			
			// Check if this looks like the data directory (contains npc directory)
			wxString npcsCheck = parentDataPath + wxFileName::GetPathSeparator() + "npc";
			if (wxDir::Exists(npcsCheck)) {
				if (npc_manager.scanNPCDirectory(nstr(parentDataPath))) {
					npcsLoaded = true;
					SetStatusText("NPCs reloaded: " + parentDataPath);
				}
			}
		}
	}
	
	// Strategy 3: Look for data directory as sibling to map directory
	if (!npcsLoaded) {
		wxFileName mapParent(mapDir);
		if (mapParent.GetDirCount() > 0) {
			mapParent.RemoveLastDir();  // Go up to parent of map directory
			wxString siblingDataPath = mapParent.GetPath() + wxFileName::GetPathSeparator() + "data";
			if (wxDir::Exists(siblingDataPath)) {
				if (npc_manager.scanNPCDirectory(nstr(siblingDataPath))) {
					npcsLoaded = true;
					SetStatusText("NPCs reloaded: " + siblingDataPath);
				}
			}
		}
	}
	
	if (!npcsLoaded) {
		SetStatusText("No NPC data directory found for reload");
	} else {
		// Show statistics for debugging
		std::string stats = npc_manager.getScanStatistics();
		wxString message = "NPCs reloaded!\n\n" + wxstr(stats);
		wxMessageBox(message, "NPC Reload Complete", wxOK | wxICON_INFORMATION, this->root);
	}
}

Editor* GUI::GetCurrentEditor() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		return mapTab->GetEditor();
	}
	return nullptr;
}

EditorTab* GUI::GetTab(int idx) {
	return tabbook->GetTab(idx);
}

int GUI::GetTabCount() const {
	return tabbook->GetTabCount();
}

EditorTab* GUI::GetCurrentTab() {
	return tabbook->GetCurrentTab();
}

MapTab* GUI::GetCurrentMapTab() const {
	if (tabbook && tabbook->GetTabCount() > 0) {
		EditorTab* editorTab = tabbook->GetCurrentTab();
		auto* mapTab = dynamic_cast<MapTab*>(editorTab);
		return mapTab;
	}
	return nullptr;
}

Map& GUI::GetCurrentMap() {
	Editor* editor = GetCurrentEditor();
	ASSERT(editor);
	return editor->map;
}

int GUI::GetOpenMapCount() {
	std::set<Map*> open_maps;

	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* tab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (tab) {
			open_maps.insert(open_maps.begin(), tab->GetMap());
		}
	}

	return static_cast<int>(open_maps.size());
}

bool GUI::ShouldSave() {
	const Map& map = GetCurrentMap();
	if (map.hasChanged()) {
		if (map.getTileCount() == 0) {
			Editor* editor = GetCurrentEditor();
			ASSERT(editor);
			return editor->actionQueue->canUndo();
		}
		return true;
	}
	return false;
}

void GUI::AddPendingCanvasEvent(wxEvent& event) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		mapTab->GetCanvas()->GetEventHandler()->AddPendingEvent(event);
	}
}

void GUI::CloseCurrentEditor() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		// Check if the map has detached views and warn the user
		if (HasDetachedViews(mapTab->GetEditor())) {
			wxString message = "This map has one or more detached views open.\n";
			message += "You must close all detached views before closing the map.";
			
			int choice = wxMessageBox(
				message,
				"Detached Views Open",
				wxOK | wxCANCEL | wxICON_EXCLAMATION
			);
			
			if (choice == wxOK) {
				// User chose to close detached views
				CloseDetachedViews(mapTab->GetEditor());
			} else {
				// User canceled operation
				return;
			}
		}
	}
	
	RefreshPalettes();
	tabbook->DeleteTab(tabbook->GetSelection());
	root->UpdateMenubar();
}

bool GUI::CloseLiveEditors(LiveSocket* sock) {
	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (mapTab) {
			Editor* editor = mapTab->GetEditor();
			if (editor->GetLiveClient() == sock) {
				tabbook->DeleteTab(i--);
			}
		}
		auto* liveLogTab = dynamic_cast<LiveLogTab*>(tabbook->GetTab(i));
		if (liveLogTab) {
			if (liveLogTab->GetSocket() == sock) {
				liveLogTab->Disconnect();
				tabbook->DeleteTab(i--);
			}
		}
	}
	root->UpdateMenubar();
	return true;
}

bool GUI::CloseAllEditors() {
	for (int i = 0; i < tabbook->GetTabCount(); ++i) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
		if (mapTab) {
			// Check if the map has detached views and warn the user
			if (HasDetachedViews(mapTab->GetEditor())) {
				tabbook->SetFocusedTab(i);
				
				wxString message = "This map has one or more detached views open.\n";
				message += "You must close all detached views before closing the map.";
				
				int choice = wxMessageBox(
					message,
					"Detached Views Open",
					wxOK | wxCANCEL | wxICON_EXCLAMATION
				);
				
				if (choice == wxOK) {
					// User chose to close detached views
					CloseDetachedViews(mapTab->GetEditor());
				} else {
					// User canceled operation
					return false;
				}
			}
		
			if (mapTab->IsUniqueReference() && mapTab->GetMap() && mapTab->GetMap()->hasChanged()) {
				tabbook->SetFocusedTab(i);
				if (!root->DoQuerySave(false)) {
					return false;
				} else {
					RefreshPalettes();
					tabbook->DeleteTab(i--);
				}
			} else {
				tabbook->DeleteTab(i--);
			}
		}
	}
	if (root) {
		root->UpdateMenubar();
	}
	return true;
}

void GUI::NewMapView() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		auto* newMapTab = newd MapTab(mapTab);
		newMapTab->OnSwitchEditorMode(mode);

		SetStatusText("Created new view");
		UpdateTitle();
		RefreshPalettes();
		root->UpdateMenubar();
		root->Refresh();
	}
}

void GUI::NewDetachedMapView() {
    MapTab* mapTab = GetCurrentMapTab();
    if (mapTab) {
        // Display options dialog for the type of view
        wxArrayString choices;
        choices.Add("Detached Window (Can be moved to another monitor)");
        choices.Add("Always-on-top Window (Will stay on top of other windows)");
        choices.Add("Dockable Panel (Can be attached like palette/minimap)");
        
        wxSingleChoiceDialog dialog(root, "Select type of view:", "Map View Options", choices);
        
        if (dialog.ShowModal() == wxID_OK) {
            int selection = dialog.GetSelection();
            
            if (selection == 0 || selection == 1) {
                // Create a standalone top-level window
                wxFrame* detachedFrame = newd wxFrame(root, wxID_ANY, "Detached Map View", 
                    wxDefaultPosition, wxSize(800, 600), 
                    wxDEFAULT_FRAME_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxMINIMIZE_BOX);
                    
                // Create a map window in the new frame
                MapWindow* newMapWindow = newd MapWindow(detachedFrame, *mapTab->GetEditor());
                
                // Set up a basic sizer for the frame
                wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
                sizer->Add(newMapWindow, 1, wxEXPAND);
                detachedFrame->SetSizer(sizer);
                
                // Initialize the map window to match current map view
                newMapWindow->FitToMap();
                
                // Set the center position to match the current view
                Position pos = mapTab->GetScreenCenterPosition();
                newMapWindow->SetScreenCenterPosition(pos);
                
                // Configure the map window based on current editor mode
                if (mode == SELECTION_MODE) {
                    newMapWindow->GetCanvas()->EnterSelectionMode();
                } else {
                    newMapWindow->GetCanvas()->EnterDrawingMode();
                }
                
                // Add a small toolbar for common functions
                wxToolBar* toolbar = new wxToolBar(detachedFrame, wxID_ANY);
                wxButton* syncButton = new wxButton(toolbar, wxID_ANY, "Sync View");
                wxCheckBox* pinCheckbox = new wxCheckBox(toolbar, wxID_ANY, "Keep on Top");
                wxCheckBox* keepOpenCheckbox = new wxCheckBox(toolbar, wxID_ANY, "Keep Open");
                toolbar->AddControl(syncButton);
                toolbar->AddSeparator();
                toolbar->AddControl(pinCheckbox);
                toolbar->AddSeparator();
                toolbar->AddControl(keepOpenCheckbox);
                toolbar->Realize();
                
                // Add the toolbar to the frame
                sizer->Insert(0, toolbar, 0, wxEXPAND);
                
                // Bind toolbar events
                syncButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
                    // Sync button clicked
                    MapTab* currentTab = GetCurrentMapTab();
                    if (currentTab) {
                        Position mainPos = currentTab->GetScreenCenterPosition();
                        newMapWindow->SetScreenCenterPosition(mainPos);
                    }
                });
                
                pinCheckbox->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent& event) {
                    // Pin checkbox toggled
                    bool checked = pinCheckbox->GetValue();
                    if (checked) {
                        detachedFrame->SetWindowStyleFlag(detachedFrame->GetWindowStyleFlag() | wxSTAY_ON_TOP);
                    } else {
                        detachedFrame->SetWindowStyleFlag(detachedFrame->GetWindowStyleFlag() & ~wxSTAY_ON_TOP);
                    }
                    
                    // Need to update the window for the style change to take effect
                    detachedFrame->Refresh();
                });
                
                // Create a client data object to store the keep-open flag
                // This avoids a memory leak from the heap-allocated bool
                struct WindowData : public wxClientData {
                    bool keepOpen = false;
                };
                WindowData* windowData = new WindowData();
                detachedFrame->SetClientObject(windowData);
                
                // Default close behavior with flag check
                detachedFrame->Bind(wxEVT_CLOSE_WINDOW, [detachedFrame](wxCloseEvent& event) {
                    WindowData* data = static_cast<WindowData*>(detachedFrame->GetClientObject());
                    if (data && data->keepOpen && event.CanVeto()) {
                        // Minimize instead of closing
                        event.Veto();
                        detachedFrame->Iconize(true);
                    } else {
                        // Regular close
                        detachedFrame->Destroy();
                    }
                });
                
                // Keep Open checkbox handler - updates the flag in client data
                keepOpenCheckbox->Bind(wxEVT_CHECKBOX, [detachedFrame](wxCommandEvent& event) {
                    wxCheckBox* cb = static_cast<wxCheckBox*>(event.GetEventObject());
                    WindowData* data = static_cast<WindowData*>(detachedFrame->GetClientObject());
                    if (data) {
                        data->keepOpen = cb->GetValue();
                    }
                });
                
                // Add context menu for quick floor navigation
                newMapWindow->Bind(wxEVT_RIGHT_DOWN, [=](wxMouseEvent& event) {
                    wxMenu popupMenu;
                    
                    // Floor navigation submenu
                    wxMenu* floorMenu = new wxMenu();
                    for (int floor = 0; floor <= 15; floor++) {
                        wxMenuItem* floorItem = floorMenu->Append(wxID_ANY, wxString::Format("Floor %d", floor));
                        
                        floorMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, [=](wxCommandEvent& event) {
                            newMapWindow->GetCanvas()->ChangeFloor(floor);
                        }, floorItem->GetId());
                    }
                    popupMenu.Append(wxID_ANY, "Go to Floor", floorMenu);
                    
                    // Show popup menu
                    newMapWindow->PopupMenu(&popupMenu);
                });
                
                // Title should include map name
                detachedFrame->SetTitle(wxString::Format("Detached View: %s", wxstr(mapTab->GetEditor()->map.getName())));
                
                // If user selected always-on-top, enable that flag
                if (selection == 1) {
                    detachedFrame->SetWindowStyleFlag(detachedFrame->GetWindowStyleFlag() | wxSTAY_ON_TOP);
                    detachedFrame->SetTitle(wxString::Format("Always-on-top View: %s", wxstr(mapTab->GetEditor()->map.getName())));
                    pinCheckbox->SetValue(true);
                }
                
                // Register the detached view
                RegisterDetachedView(mapTab->GetEditor(), detachedFrame);
                
                // Add cleanup to close handler to remove the frame from our registry
                detachedFrame->Bind(wxEVT_DESTROY, [=](wxWindowDestroyEvent& event) {
                    UnregisterDetachedView(mapTab->GetEditor(), detachedFrame);
                });
                
                // Show the window
                detachedFrame->Show();
                
                SetStatusText(selection == 0 ? "Created new detached view" : "Created new always-on-top view");
            } 
            else if (selection == 2) {
                // Create a MapWindow as a dockable panel
                MapWindow* newMapWindow = newd MapWindow(root, *mapTab->GetEditor());
                
                // Add the window to the AUI manager
                wxAuiPaneInfo paneInfo;
                paneInfo.Caption("Map View")
                    .Float()
                    .Floatable(true)
                    .Dockable(true)
                    .Movable(true)
                    .Resizable(true)
                    .MinSize(400, 300)
                    .BestSize(640, 480);
                    
                aui_manager->AddPane(newMapWindow, paneInfo);
                aui_manager->Update();
                
                // Initialize the map window to match current map view
                newMapWindow->FitToMap();
                
                // Set the center position to match the current view
                Position pos = mapTab->GetScreenCenterPosition();
                newMapWindow->SetScreenCenterPosition(pos);
                
                // Configure the map window based on current editor mode
                if (mode == SELECTION_MODE) {
                    newMapWindow->GetCanvas()->EnterSelectionMode();
                } else {
                    newMapWindow->GetCanvas()->EnterDrawingMode();
                }
                
                // Register the dockable view
                RegisterDockableView(mapTab->GetEditor(), newMapWindow);
                
                // Bind cleanup event to remove the window from our registry when destroyed
                newMapWindow->Bind(wxEVT_DESTROY, [=](wxWindowDestroyEvent& event) {
                    UnregisterDockableView(mapTab->GetEditor(), newMapWindow);
                    event.Skip();
                });
                
                SetStatusText("Created new dockable map view");
            }
        }
    }
}

void GUI::LoadPerspective() {
	if (!IsVersionLoaded()) {
		if (g_settings.getInteger(Config::WINDOW_MAXIMIZED)) {
			root->Maximize();
		} else {
			root->SetSize(wxSize(
				g_settings.getInteger(Config::WINDOW_WIDTH),
				g_settings.getInteger(Config::WINDOW_HEIGHT)
			));
		}
	} else {
		std::string tmp;
		std::string layout = g_settings.getString(Config::PALETTE_LAYOUT);

		std::vector<std::string> palette_list;
		for (char c : layout) {
			if (c == '|') {
				palette_list.push_back(tmp);
				tmp.clear();
			} else {
				tmp.push_back(c);
			}
		}

		if (!tmp.empty()) {
			palette_list.push_back(tmp);
		}

		for (const std::string& name : palette_list) {
			PaletteWindow* palette = CreatePalette();

			wxAuiPaneInfo& info = aui_manager->GetPane(palette);
			aui_manager->LoadPaneInfo(wxstr(name), info);

			if (info.IsFloatable()) {
				bool offscreen = true;
				for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
					wxDisplay display(index);
					wxRect rect = display.GetClientArea();
					if (rect.Contains(info.floating_pos)) {
						offscreen = false;
						break;
					}
				}

				if (offscreen) {
					info.Dock();
				}
			}
		}

		if (g_settings.getInteger(Config::MINIMAP_VISIBLE)) {
			if (!minimap) {
				wxAuiPaneInfo info;

				const wxString& data = wxstr(g_settings.getString(Config::MINIMAP_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);

				minimap = newd MinimapWindow(root);
				// Ensure the minimap is always dockable regardless of saved state
				info.Dockable(true).Resizable(true).MinSize(300, 200);
				
				aui_manager->AddPane(minimap, info);
			} else {
				wxAuiPaneInfo& info = aui_manager->GetPane(minimap);

				const wxString& data = wxstr(g_settings.getString(Config::MINIMAP_LAYOUT));
				aui_manager->LoadPaneInfo(data, info);
				
				// Ensure the minimap is always dockable regardless of saved state
				info.Dockable(true).Resizable(true).MinSize(300, 200);
			}

			wxAuiPaneInfo& info = aui_manager->GetPane(minimap);
			if (info.IsFloatable()) {
				bool offscreen = true;
				for (uint32_t index = 0; index < wxDisplay::GetCount(); ++index) {
					wxDisplay display(index);
					wxRect rect = display.GetClientArea();
					if (rect.Contains(info.floating_pos)) {
						offscreen = false;
						break;
					}
				}

				if (offscreen) {
					info.Dock();
				}
			}
		}

		aui_manager->Update();
		root->UpdateMenubar();
	}

	root->GetAuiToolBar()->LoadPerspective();
}

void GUI::SavePerspective() {
	g_settings.setInteger(Config::WINDOW_MAXIMIZED, root->IsMaximized());
	g_settings.setInteger(Config::WINDOW_WIDTH, root->GetSize().GetWidth());
	g_settings.setInteger(Config::WINDOW_HEIGHT, root->GetSize().GetHeight());

	g_settings.setInteger(Config::MINIMAP_VISIBLE, minimap ? 1 : 0);

	wxString pinfo;
	for (auto& palette : palettes) {
		if (aui_manager->GetPane(palette).IsShown()) {
			pinfo << aui_manager->SavePaneInfo(aui_manager->GetPane(palette)) << "|";
		}
	}
	g_settings.setString(Config::PALETTE_LAYOUT, nstr(pinfo));

	if (minimap) {
		wxString s = aui_manager->SavePaneInfo(aui_manager->GetPane(minimap));
		g_settings.setString(Config::MINIMAP_LAYOUT, nstr(s));
	}

	root->GetAuiToolBar()->SavePerspective();
}

void GUI::HideSearchWindow() {
	if (search_result_window) {
		aui_manager->GetPane(search_result_window).Show(false);
		aui_manager->Update();
	}
}

SearchResultWindow* GUI::GetSearchWindow() {
	return search_result_window;
}

SearchResultWindow* GUI::ShowSearchWindow() {
	if (search_result_window == nullptr) {
		search_result_window = newd SearchResultWindow(root);
		aui_manager->AddPane(search_result_window, wxAuiPaneInfo().Caption("Search Results"));
	} else {
		aui_manager->GetPane(search_result_window).Show();
	}
	aui_manager->Update();
	return search_result_window;
}

void GUI::HideMapSummaryWindow() {
	if (map_summary_window) {
		aui_manager->GetPane(map_summary_window).Show(false);
		aui_manager->Update();
	}
}

MapSummaryWindow* GUI::GetMapSummaryWindow() {
	return map_summary_window;
}

MapSummaryWindow* GUI::ShowMapSummaryWindow() {
	if (map_summary_window == nullptr) {
		map_summary_window = newd MapSummaryWindow(root);
		aui_manager->AddPane(map_summary_window, wxAuiPaneInfo().Caption("Map Summary"));
	} else {
		aui_manager->GetPane(map_summary_window).Show();
	}
	aui_manager->Update();
	return map_summary_window;
}

//=============================================================================
// Recent Brushes Window Interface Implementation

void GUI::HideRecentBrushesWindow() {
	if (recent_brushes_window) {
		aui_manager->GetPane(recent_brushes_window).Show(false);
		aui_manager->Update();
	}
}

RecentBrushesWindow* GUI::GetRecentBrushesWindow() {
	return recent_brushes_window;
}

RecentBrushesWindow* GUI::ShowRecentBrushesWindow() {
	if (recent_brushes_window == nullptr) {
		recent_brushes_window = newd RecentBrushesWindow(root);
		
		// Add as dockable pane on the right side
		wxAuiPaneInfo paneInfo;
		paneInfo.Caption("Recent Brushes")
			.Right()  // Dock on the right side initially
			.Floatable(true)
			.Movable(true)
			.Dockable(true)
			.Resizable(true)
			.MinSize(120, 200)
			.BestSize(150, 400)
			.DestroyOnClose(false);
			
		aui_manager->AddPane(recent_brushes_window, paneInfo);
	} else {
		aui_manager->GetPane(recent_brushes_window).Show();
	}
	aui_manager->Update();
	return recent_brushes_window;
}

void GUI::AddRecentBrush(Brush* brush) {
	// Don't add null brushes or system brushes that shouldn't be in recent list
	if (!brush || brush == eraser) {
		return;
	}
	
	// Don't add creature, house, or waypoint brushes to recent list
	if (brush->isCreature() || brush->isHouse() || brush->isWaypoint()) {
		return;
	}
	
	// Determine palette type for this brush
	PaletteType palette_type = TILESET_UNKNOWN;
	
	// Check what type of brush this is
	if (brush->isRaw()) {
		palette_type = TILESET_RAW;
	} else {
		// For other brushes, try to determine from current palette selection
		PaletteWindow* palette = GetPalette();
		if (palette) {
			palette_type = palette->GetSelectedPage();
		}
		
		// If we still don't know, try to categorize by brush properties
		if (palette_type == TILESET_UNKNOWN) {
			if (brush->isGround()) {
				palette_type = TILESET_TERRAIN;
			} else if (brush->isDoodad()) {
				palette_type = TILESET_DOODAD;
			} else {
				palette_type = TILESET_ITEM;
			}
		}
	}
	
	if (recent_brushes_window) {
		// Only add as manual selection (true) to prevent automatic palette switching additions
		recent_brushes_window->AddRecentBrush(brush, palette_type, true);
	}
}

//=============================================================================
// Monster Maker Window Interface Implementation

void GUI::HideMonsterMakerWindow() {
	if (monster_maker_window) {
		monster_maker_window->Hide();
	}
}

MonsterMakerWindow* GUI::GetMonsterMakerWindow() {
	return monster_maker_window;
}

MonsterMakerWindow* GUI::ShowMonsterMakerWindow() {
	if (monster_maker_window == nullptr) {
		monster_maker_window = newd MonsterMakerWindow(root);
	}
	
	// Refresh the monster list when showing
	monster_maker_window->RefreshMonsterList();
	
	// Show as detached window
	monster_maker_window->Show(true);
	monster_maker_window->Raise();
	
	return monster_maker_window;
}

//=============================================================================
// Palette Window Interface implementation

PaletteWindow* GUI::GetPalette() const {
	if (palettes.empty()) {
		return nullptr;
	}
	return palettes.front();
}

PaletteWindow* GUI::NewPalette() {
	return CreatePalette();
}

void GUI::RefreshPalettes(Map* m, bool usedefault) {
	for (auto& palette : palettes) {
		palette->OnUpdate(m ? m : (usedefault ? (IsEditorOpen() ? &GetCurrentMap() : nullptr) : nullptr));
	}
	SelectBrush();
}

void GUI::RefreshOtherPalettes(PaletteWindow* p) {
	for (auto& palette : palettes) {
		if (palette != p) {
			palette->OnUpdate(IsEditorOpen() ? &GetCurrentMap() : nullptr);
		}
	}
	SelectBrush();
}

PaletteWindow* GUI::CreatePalette() {
	if (!IsVersionLoaded()) {
		return nullptr;
	}

	auto* palette = new PaletteWindow(root, g_materials.tilesets);
	
	// Add the palette with resizable properties
	wxAuiPaneInfo paneInfo;
	paneInfo.Caption("Palette")
		.TopDockable(false)
		.BottomDockable(false)
		.Resizable(true)  // Allow resizing
		.MinSize(225, 250)  // Minimum size
		.BestSize(230, 400);  // Default size
		
	aui_manager->AddPane(palette, paneInfo);
	aui_manager->Update();

	// Make us the active palette
	palettes.push_front(palette);
	// Select brush from this palette
	SelectBrushInternal(palette->GetSelectedBrush());
	// fix for blank house list on f5 or new palette
	palette->OnUpdate(IsEditorOpen() ? &GetCurrentMap() : nullptr);
	return palette;
}

void GUI::ActivatePalette(PaletteWindow* p) {
	palettes.erase(std::find(palettes.begin(), palettes.end(), p));
	palettes.push_front(p);
}

void GUI::DestroyPalettes() {
	for (auto palette : palettes) {
		aui_manager->DetachPane(palette);
		palette->Destroy();
		palette = nullptr;
	}
	palettes.clear();
	aui_manager->Update();
}

void GUI::RebuildPalettes() {
	// Completely recreate palettes to include/exclude tileset editing buttons
	if (!palettes.empty()) {
		// Remember which palette was active
		Map* current_map = IsEditorOpen() ? &GetCurrentMap() : nullptr;
		
		// Destroy all palettes
		DestroyPalettes();
		
		// Recreate palette
		CreatePalette();
		
		// Force update
		RefreshPalettes(current_map);
	}
	
	// Update the AUI manager
	aui_manager->Update();
}

void GUI::ShowPalette() {
	if (palettes.empty()) {
		return;
	}

	for (auto& palette : palettes) {
		if (aui_manager->GetPane(palette).IsShown()) {
			return;
		}
	}

	aui_manager->GetPane(palettes.front()).Show(true);
	aui_manager->Update();
}

void GUI::SelectPalettePage(PaletteType pt) {
	if (palettes.empty()) {
		CreatePalette();
	}
	PaletteWindow* p = GetPalette();
	if (!p) {
		return;
	}

	ShowPalette();
	p->SelectPage(pt);
	aui_manager->Update();
	SelectBrushInternal(p->GetSelectedBrush());
}

//=============================================================================
// Minimap Window Interface Implementation

void GUI::CreateMinimap() {
	if (!IsVersionLoaded()) {
		return;
	}

	if (minimap && minimap_enabled) {
		// If minimap exists and is enabled, hide it
		HideMinimap();
		minimap_enabled = false;
	} else if (minimap) {
		// If minimap exists but is hidden, show it
		ShowMinimap();
		minimap_enabled = true;
	} else {
		// Create new minimap
		minimap = newd MinimapWindow(root);
		minimap->SetSize(wxSize(640, 320));
		
		// Add as dockable pane with configurable size
		wxAuiPaneInfo paneInfo;
		paneInfo.Caption("Minimap")
				.Float()
				.FloatingSize(wxSize(640, 320))
				.Floatable(true)
				.Movable(true)
				.Dockable(true)  // Allow docking
				.Resizable(true) // Allow resizing
				.MinSize(300, 200) // Minimum size
				.DestroyOnClose(false);
				
		aui_manager->AddPane(minimap, paneInfo);
		minimap_enabled = true;
		aui_manager->Update();
		
		// Show the minimap immediately without caching
		minimap->Show(true);
	}
}

void GUI::HideMinimap() {
	if (minimap) {
		
		aui_manager->GetPane(minimap).Show(false);
		aui_manager->Update();
	}
}

void GUI::ShowMinimap() {
	if (minimap) {
		aui_manager->GetPane(minimap).Show(true);
		aui_manager->Update();
	} else {
		// Create minimap if it doesn't exist
		CreateMinimap();
	}
}

void GUI::DestroyMinimap() {
	if (minimap) {
		aui_manager->DetachPane(minimap);
		aui_manager->Update();
		minimap->Destroy();
		minimap = nullptr;
		minimap_enabled = false;
	}
}

void GUI::UpdateMinimap(bool immediate) {
	if (IsMinimapVisible()) {
		if (immediate) {
			minimap->Refresh();
		} else {
			minimap->DelayedUpdate();
		}
	}
}

bool GUI::IsMinimapVisible() const {
	if (minimap) {
		const wxAuiPaneInfo& pi = aui_manager->GetPane(minimap);
		if (pi.IsShown()) {
			return true;
		}
	}
	return false;
}

//=============================================================================

void GUI::RefreshView() {
	EditorTab* editorTab = GetCurrentTab();
	if (!editorTab) {
		return;
	}

	if (!dynamic_cast<MapTab*>(editorTab)) {
		editorTab->GetWindow()->Refresh();
		return;
	}

	std::vector<EditorTab*> editorTabs;
	for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(index));
		if (mapTab) {
			editorTabs.push_back(mapTab);
		}
	}

	for (EditorTab* editorTab : editorTabs) {
		editorTab->GetWindow()->Refresh();
	}
}

void GUI::CreateLoadBar(wxString message, bool canCancel /* = false */) {
	progressText = message;

	progressFrom = 0;
	progressTo = 100;
	currentProgress = -1;

	progressBar = newd wxGenericProgressDialog("Loading", progressText + " (0%)", 100, root, wxPD_APP_MODAL | wxPD_SMOOTH | (canCancel ? wxPD_CAN_ABORT : 0));
	progressBar->SetSize(280, -1);
	progressBar->Show(true);

	for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
		auto* mt = dynamic_cast<MapTab*>(tabbook->GetTab(idx));
		if (mt && mt->GetEditor()->IsLiveServer()) {
			mt->GetEditor()->GetLiveServer()->startOperation(progressText);
		}
	}
	progressBar->Update(0);
}

void GUI::SetLoadScale(int32_t from, int32_t to) {
	progressFrom = from;
	progressTo = to;
}

bool GUI::SetLoadDone(int32_t done, const wxString& newMessage) {
	if (done == 100) {
		DestroyLoadBar();
		return true;
	} else if (done == currentProgress) {
		return true;
	}

	if (!newMessage.empty()) {
		progressText = newMessage;
	}

	int32_t newProgress = progressFrom + static_cast<int32_t>((done / 100.f) * (progressTo - progressFrom));
	newProgress = std::max<int32_t>(0, std::min<int32_t>(100, newProgress));

	bool skip = false;
	if (progressBar) {
		progressBar->Update(
			newProgress,
			wxString::Format("%s (%d%%)", progressText, newProgress),
			&skip
		);
		currentProgress = newProgress;
	}

	for (int32_t index = 0; index < tabbook->GetTabCount(); ++index) {
		auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(index));
		if (mapTab && mapTab->GetEditor()) {
			LiveServer* server = mapTab->GetEditor()->GetLiveServer();
			if (server) {
				server->updateOperation(newProgress);
			}
		}
	}

	return skip;
}

void GUI::DestroyLoadBar() {
	if (progressBar) {
		progressBar->Show(false);
		currentProgress = -1;

		// Store pointer temporarily to avoid null issues
		wxGenericProgressDialog* tempBar = progressBar;
		progressBar = nullptr; // Set to null before destruction to prevent recursion
		
		try {
			tempBar->Destroy();
		} catch(...) {
			// Ignore any exceptions during destroy
		}

		if (root && root->IsActive()) {
			root->Raise();
		} else if (root) {
			root->RequestUserAttention();
		}
	}
}

void GUI::ShowWelcomeDialog(const wxBitmap& icon) {
	std::vector<wxString> recent_files = root->GetRecentFiles();
	welcomeDialog = newd WelcomeDialog(__W_RME_APPLICATION_NAME__, "Version " + __W_RME_VERSION__, FROM_DIP(root, wxSize(1000, 480)), icon, recent_files);
	welcomeDialog->Bind(wxEVT_CLOSE_WINDOW, &GUI::OnWelcomeDialogClosed, this);
	welcomeDialog->Bind(WELCOME_DIALOG_ACTION, &GUI::OnWelcomeDialogAction, this);
	welcomeDialog->Show();
	UpdateMenubar();
}

void GUI::FinishWelcomeDialog() {
	if (welcomeDialog != nullptr) {
		welcomeDialog->Hide();
		root->Show();
		welcomeDialog->Destroy();
		welcomeDialog = nullptr;
	}
}

bool GUI::IsWelcomeDialogShown() {
	return welcomeDialog != nullptr && welcomeDialog->IsShown();
}

void GUI::OnWelcomeDialogClosed(wxCloseEvent& event) {
	welcomeDialog->Destroy();
	root->Close();
}

void GUI::OnWelcomeDialogAction(wxCommandEvent& event) {
	if (event.GetId() == wxID_NEW) {
		NewMap();
	} else if (event.GetId() == wxID_OPEN) {
		LoadMap(FileName(event.GetString()));
	}
}

void GUI::UpdateMenubar() {
	root->UpdateMenubar();
}

void GUI::SetScreenCenterPosition(Position position) {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		// Store old position for comparison
		Position oldPosition = mapTab->GetScreenCenterPosition();
		
		// Set the new position
		mapTab->SetScreenCenterPosition(position);
		
		// Update minimap if the position changed significantly (e.g., teleport/goto)
		// or if the floor changed
		if (minimap && IsMinimapVisible() && 
		   (abs(oldPosition.x - position.x) > 10 || 
		    abs(oldPosition.y - position.y) > 10 || 
		    oldPosition.z != position.z)) {
			minimap->Refresh();
		}
	}
}

void GUI::DoCut() {
	if (!IsSelectionMode()) {
		return;
	}

	Editor* editor = GetCurrentEditor();
	if (!editor) {
		return;
	}

	editor->copybuffer.cut(*editor, GetCurrentFloor());
	RefreshView();
	root->UpdateMenubar();
}

void GUI::DoCopy() {
	if (!IsSelectionMode()) {
		return;
	}

	Editor* editor = GetCurrentEditor();
	if (!editor) {
		return;
	}

	editor->copybuffer.copy(*editor, GetCurrentFloor());
	RefreshView();
	root->UpdateMenubar();
}

void GUI::DoPaste() {
	MapTab* mapTab = GetCurrentMapTab();
	if (mapTab) {
		copybuffer.paste(*mapTab->GetEditor(), mapTab->GetCanvas()->GetCursorPosition());
	}
}

void GUI::PreparePaste() {
	Editor* editor = GetCurrentEditor();
	if (editor) {
		SetSelectionMode();
		editor->selection.start();
		editor->selection.clear();
		editor->selection.finish();
		StartPasting();
		RefreshView();
	}
}

void GUI::StartPasting() {
	if (GetCurrentEditor()) {
		pasting = true;
		secondary_map = &copybuffer.getBufferMap();
	}
}

void GUI::EndPasting() {
	if (pasting) {
		pasting = false;
		secondary_map = nullptr;
	}
}

bool GUI::CanUndo() {
	Editor* editor = GetCurrentEditor();
	return (editor && editor->actionQueue->canUndo());
}

bool GUI::CanRedo() {
	Editor* editor = GetCurrentEditor();
	return (editor && editor->actionQueue->canRedo());
}

bool GUI::DoUndo() {
	Editor* editor = GetCurrentEditor();
	if (editor && editor->actionQueue->canUndo()) {
		// Store the current mode before undoing
		EditorMode previous_mode = mode;
		
		// Perform the undo operation
		editor->actionQueue->undo();
		
		// Switch to selection mode if there's a selection
		if (editor->selection.size() > 0) {
			SetSelectionMode();
		} 
		// If we were in drawing mode before and there's no selection now, restore drawing mode
		else if (previous_mode == DRAWING_MODE && editor->selection.size() == 0) {
			SetDrawingMode();
		}
		
		SetStatusText("Undo action");
		UpdateMinimap();
		root->UpdateMenubar();
		root->Refresh();
		return true;
	}
	return false;
}

bool GUI::DoRedo() {
	Editor* editor = GetCurrentEditor();
	if (editor && editor->actionQueue->canRedo()) {
		// Store the current mode before redoing
		EditorMode previous_mode = mode;
		
		// Perform the redo operation
		editor->actionQueue->redo();
		
		// Switch to selection mode if there's a selection
		if (editor->selection.size() > 0) {
			SetSelectionMode();
		}
		// If we were in drawing mode before and there's no selection now, restore drawing mode
		else if (previous_mode == DRAWING_MODE && editor->selection.size() == 0) {
			SetDrawingMode();
		}
		
		SetStatusText("Redo action");
		UpdateMinimap();
		root->UpdateMenubar();
		root->Refresh();
		return true;
	}
	return false;
}

int GUI::GetCurrentFloor() {
	MapTab* tab = GetCurrentMapTab();
	ASSERT(tab);
	return tab->GetCanvas()->GetFloor();
}

void GUI::ChangeFloor(int new_floor) {
	MapTab* tab = GetCurrentMapTab();
	if (tab) {
		int old_floor = GetCurrentFloor();
		if (new_floor < 0 || new_floor > MAP_MAX_LAYER) {
			return;
		}

		if (old_floor != new_floor) {
			tab->GetCanvas()->ChangeFloor(new_floor);
			// Only refresh minimap if it's visible - it will use cached blocks for the new floor
			if (minimap && IsMinimapVisible()) {
				minimap->SetMinimapFloor(new_floor);
			}
		}
	}
}

void GUI::SetStatusText(wxString text) {
	g_gui.root->SetStatusText(text, 0);
}

void GUI::SetTitle(wxString title) {
	if (g_gui.root == nullptr) {
		return;
	}

#ifdef NIGHTLY_BUILD
	#ifdef SVN_BUILD
		#define TITLE_APPEND (wxString(" (Nightly Build #") << i2ws(SVN_BUILD) << ")")
	#else
		#define TITLE_APPEND (wxString(" (Nightly Build)"))
	#endif
#else
	#ifdef SVN_BUILD
		#define TITLE_APPEND (wxString(" (Build #") << i2ws(SVN_BUILD) << ")")
	#else
		#define TITLE_APPEND (wxString(""))
	#endif
#endif
#ifdef __EXPERIMENTAL__
	if (title != "") {
		g_gui.root->SetTitle(title << " - OTAcademy Map Editor BETA" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString("OTAcademy Map Editor BETA") << TITLE_APPEND);
	}
#elif __SNAPSHOT__
	if (title != "") {
		g_gui.root->SetTitle(title << " - OTAcademy Map Editor - SNAPSHOT" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString("OTAcademy Map Editor - SNAPSHOT") << TITLE_APPEND);
	}
#else
	if (!title.empty()) {
		g_gui.root->SetTitle(title << " Idler Map Editor - JOIN IDLERS TAVERN FOR FREE C++ CODES https://discord.gg/FD2cYKBq5E" << TITLE_APPEND);
	} else {
		g_gui.root->SetTitle(wxString(" Idler Map Editor - JOIN IDLERS TAVERN FOR FREE C++ CODES https://discord.gg/FD2cYKBq5E") << TITLE_APPEND);
	}
#endif
}

void GUI::UpdateTitle() {
	if (tabbook->GetTabCount() > 0) {
		SetTitle(tabbook->GetCurrentTab()->GetTitle());
		for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
			if (tabbook->GetTab(idx)) {
				tabbook->SetTabLabel(idx, tabbook->GetTab(idx)->GetTitle());
				
				// Update detached view titles if this is a map tab
				if (auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(idx))) {
					UpdateDetachedViewsTitle(mapTab->GetEditor());
				}
			}
		}
	} else {
		SetTitle("");
	}
}

void GUI::UpdateMenus() {
	wxCommandEvent evt(EVT_UPDATE_MENUS);
	g_gui.root->AddPendingEvent(evt);
}

void GUI::ShowToolbar(ToolBarID id, bool show) {
	if (root && root->GetAuiToolBar()) {
		root->GetAuiToolBar()->Show(id, show);
	}
}

void GUI::SwitchMode() {
	if (mode == DRAWING_MODE) {
		SetSelectionMode();
	} else {
		SetDrawingMode();
	}
}

void GUI::SetSelectionMode() {
	if (mode == SELECTION_MODE) {
		return;
	}

	if (current_brush && current_brush->isDoodad()) {
		secondary_map = nullptr;
	}

	tabbook->OnSwitchEditorMode(SELECTION_MODE);
	mode = SELECTION_MODE;
}

void GUI::SetDrawingMode() {
	if (mode == DRAWING_MODE) {
		return;
	}

	std::set<MapTab*> al;
	for (int idx = 0; idx < tabbook->GetTabCount(); ++idx) {
		EditorTab* editorTab = tabbook->GetTab(idx);
		if (auto* mapTab = dynamic_cast<MapTab*>(editorTab)) {
			if (al.find(mapTab) != al.end()) {
				continue;
			}

			Editor* editor = mapTab->GetEditor();
			editor->selection.start();
			//editor->selection.clear();
			editor->selection.finish();
			al.insert(mapTab);
		}
	}

	if (current_brush && current_brush->isDoodad()) {
		secondary_map = doodad_buffer_map.get();
	} else {
		secondary_map = nullptr;
	}

	tabbook->OnSwitchEditorMode(DRAWING_MODE);
	mode = DRAWING_MODE;
}

void GUI::SetBrushSizeInternal(int nz) {
	// Add safety check to prevent brush_size from being 0
	if (nz < 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: WARNING! SetBrushSizeInternal called with negative value %d - FORCING TO 0\n", nz);
		OutputDebugStringA(debug_msg);
		nz = 0; // Minimum valid brush_size index
	}
	
	brush_size = nz;
	
	// ENHANCED FIX: Validate that the calculated actual size is safe
	int actual_width = GetBrushWidth();
	int actual_height = GetBrushHeight();
	
	if (actual_width <= 0 || actual_height <= 0) {
		char debug_msg[512];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! SetBrushSizeInternal resulted in invalid dimensions: width=%d, height=%d, brush_size=%d\n", 
			actual_width, actual_height, brush_size);
		OutputDebugStringA(debug_msg);
		
		// Force to safe values
		if (actual_width <= 0) brush_size = 0; // Reset to minimal safe brush size
		if (actual_height <= 0) brush_size = 0;
	}
}

void GUI::SetBrushSize(int nz) {
	// CRITICAL FIX: Prevent brush_size from being set to invalid values
	if (nz < 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: WARNING! SetBrushSize called with negative value %d - FORCING TO 0\n", nz);
		OutputDebugStringA(debug_msg);
		nz = 0; // Minimum valid brush_size index
	}
	
	brush_size = nz;
	
	// ENHANCED FIX: Validate that the calculated actual size is safe
	int actual_width = GetBrushWidth();
	int actual_height = GetBrushHeight();
	
	if (actual_width <= 0 || actual_height <= 0) {
		char debug_msg[512];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! SetBrushSize resulted in invalid dimensions: width=%d, height=%d, brush_size=%d\n", 
			actual_width, actual_height, brush_size);
		OutputDebugStringA(debug_msg);
		
		// Force to safe values
		if (actual_width <= 0) brush_size = 0; // Reset to minimal safe brush size
		if (actual_height <= 0) brush_size = 0;
	}
	
	for (PaletteList::iterator iter = palettes.begin(); iter != palettes.end(); ++iter) {
		(*iter)->OnUpdateBrushSize(brush_shape, brush_size);
	}
}

void GUI::SetBrushVariation(int nz) {
	if (nz != brush_variation && current_brush && current_brush->isDoodad()) {
		// Monkey!
		brush_variation = nz;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map.get();
	}
}

void GUI::SetBrushShape(BrushShape bs) {
	if (bs != brush_shape && current_brush && current_brush->isDoodad() && !current_brush->oneSizeFitsAll()) {
		// Donkey!
		brush_shape = bs;
		FillDoodadPreviewBuffer();
		secondary_map = doodad_buffer_map.get();
	}
	brush_shape = bs;
	
	// Disable custom brush size when switching to circle brush
	if (bs == BRUSHSHAPE_CIRCLE && use_custom_brush_size) {
		use_custom_brush_size = false;
		SetStatusText("Custom brush size disabled for circle brush");
	}

	for (auto& palette : palettes) {
		palette->OnUpdateBrushSize(brush_shape, brush_size);
	}

	root->GetAuiToolBar()->UpdateBrushSize(brush_shape, brush_size);
}

void GUI::SetBrushThickness(bool on, int x, int y) {
	use_custom_thickness = on;

	if (x != -1 || y != -1) {
		custom_thickness_mod = float(max(x, 1)) / float(max(y, 1));
	}

	if (current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void GUI::SetBrushThickness(int low, int ceil) {
	custom_thickness_mod = float(max(low, 1)) / float(max(ceil, 1));

	if (use_custom_thickness && current_brush && current_brush->isDoodad()) {
		FillDoodadPreviewBuffer();
	}

	RefreshView();
}

void GUI::DecreaseBrushSize(bool wrap) {
	switch (brush_size) {
		case 0: {
			if (wrap) {
				SetBrushSize(11);
			}
			break;
		}
		case 1: {
			SetBrushSize(0);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(1);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(2);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(4);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(6);
			break;
		}
		case 11:
		default: {
			SetBrushSize(8);
			break;
		}
	}
}

void GUI::IncreaseBrushSize(bool wrap) {
	switch (brush_size) {
		case 0: {
			SetBrushSize(1);
			break;
		}
		case 1: {
			SetBrushSize(2);
			break;
		}
		case 2:
		case 3: {
			SetBrushSize(4);
			break;
		}
		case 4:
		case 5: {
			SetBrushSize(6);
			break;
		}
		case 6:
		case 7: {
			SetBrushSize(8);
			break;
		}
		case 8:
		case 9:
		case 10: {
			SetBrushSize(11);
			break;
		}
		case 11:
		default: {
			if (wrap) {
				SetBrushSize(0);
			}
			break;
		}
	}
}

void GUI::SetDoorLocked(bool on) {
	draw_locked_doors = on;
	RefreshView();
}

bool GUI::HasDoorLocked() {
	return draw_locked_doors;
}

Brush* GUI::GetCurrentBrush() const {
	return current_brush;
}

BrushShape GUI::GetBrushShape() const {
	if (current_brush == spawn_brush) {
		return BRUSHSHAPE_SQUARE;
	}

	return brush_shape;
}

int GUI::GetBrushSize() const {
	return brush_size;
}

int GUI::GetBrushVariation() const {
	return brush_variation;
}

int GUI::GetSpawnTime() const {
	return creature_spawntime;
}

void GUI::SelectBrush() {
	if (palettes.empty()) {
		return;
	}

	SelectBrushInternal(palettes.front()->GetSelectedBrush());

	RefreshView();
}

bool GUI::SelectBrush(const Brush* whatbrush, PaletteType primary) {
	if (palettes.empty()) {
		if (!CreatePalette()) {
			return false;
		}
	}
	
	// Reset custom brush size when switching brushes
	if (use_custom_brush_size) {
		use_custom_brush_size = false;
	}

	if (!palettes.front()->OnSelectBrush(whatbrush, primary)) {
		return false;
	}

	SelectBrushInternal(const_cast<Brush*>(whatbrush));
	root->GetAuiToolBar()->UpdateBrushButtons();
	return true;
}

void GUI::SelectBrushInternal(Brush* brush) {
	// Store previous brush before changing current brush
	previous_brush = current_brush;
	current_brush = brush;
	if (!current_brush) {
		return;
	}

	// Note: AddRecentBrush is now called manually from palette clicks and other user actions
	// This prevents automatic additions when palettes switch and select their first brush

	brush_variation = min(brush_variation, brush->getMaxVariation());
	FillDoodadPreviewBuffer();
	if (brush->isDoodad()) {
		secondary_map = doodad_buffer_map.get();
	}

	SetDrawingMode();
	RefreshView();
}

void GUI::SelectPreviousBrush() {
	if (previous_brush) {
		SelectBrush(previous_brush);
	}
}

void GUI::FillDoodadPreviewBuffer() {
	if (!current_brush || !current_brush->isDoodad()) {
		return;
	}

	doodad_buffer_map.get()->clear();

	DoodadBrush* brush = current_brush->asDoodad();
	if (brush->isEmpty(GetBrushVariation())) {
		return;
	}

	int object_count = 0;
	int area;
	if (GetBrushShape() == BRUSHSHAPE_SQUARE) {
		area = 2 * GetBrushSize();
		area = area * area + 1;
	} else {
		if (GetBrushSize() == 1) {
			// There is a huge deviation here with the other formula.
			area = 5;
		} else {
			area = int(0.5 + GetBrushSize() * GetBrushSize() * PI);
		}
	}
	const int object_range = (use_custom_thickness ? int(area * custom_thickness_mod) : brush->getThickness() * area / max(1, brush->getThicknessCeiling()));
	const int final_object_count = max(1, object_range + random(object_range));

	// Starting position for the center of the preview
	Position center_pos(0x8000, 0x8000, 0x8);

	if (brush_size > 0 && !brush->oneSizeFitsAll()) {
		while (object_count < final_object_count) {
			int retries = 0;
			bool exit = false;

			// Try to place objects 5 times
			while (retries < 5 && !exit) {

				int pos_retries = 0;
				int xpos = 0, ypos = 0;
				bool found_pos = false;
				if (GetBrushShape() == BRUSHSHAPE_CIRCLE) {
					while (pos_retries < 5 && !found_pos) {
						xpos = random(-brush_size, brush_size);
						ypos = random(-brush_size, brush_size);
						float distance = sqrt(float(xpos * xpos) + float(ypos * ypos));
						if (distance < g_gui.GetBrushSize() + 0.005) {
							found_pos = true;
						} else {
							++pos_retries;
						}
					}
				} else {
					found_pos = true;
					xpos = random(-brush_size, brush_size);
					ypos = random(-brush_size, brush_size);
				}

				if (!found_pos) {
					++retries;
					continue;
				}

				// Decide whether the zone should have a composite or several single objects.
				bool fail = false;
				if (random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
					// Composite
					const CompositeTileList& composites = brush->getComposite(GetBrushVariation());

					// Figure out if the placement is valid
					for (const auto& composite : composites) {
						// Include the z offset in the position calculation
						Position pos = center_pos + Position(composite.first.x + xpos, composite.first.y + ypos, composite.first.z);
						if (Tile* tile = doodad_buffer_map.get()->getTile(pos)) {
							if (!tile->empty()) {
								fail = true;
								break;
							}
						}
					}
					if (fail) {
						++retries;
						break;
					}

					// Transfer items to the stack
					for (const auto& composite : composites) {
						// Include the z offset in the position calculation
						Position pos = center_pos + Position(composite.first.x + xpos, composite.first.y + ypos, composite.first.z);
						const ItemVector& items = composite.second;
						Tile* tile = doodad_buffer_map.get()->getTile(pos);

						if (!tile) {
							tile = doodad_buffer_map.get()->allocator(doodad_buffer_map.get()->createTileL(pos));
						}

						for (auto item : items) {
							tile->addItem(item->deepCopy());
						}
						doodad_buffer_map.get()->setTile(tile->getPosition(), tile);
					}
					exit = true;
				} else if (brush->hasSingleObjects(GetBrushVariation())) {
					Position pos = center_pos + Position(xpos, ypos, 0);
					Tile* tile = doodad_buffer_map.get()->getTile(pos);
					if (tile) {
						if (!tile->empty()) {
							fail = true;
							break;
						}
					} else {
						tile = doodad_buffer_map.get()->allocator(doodad_buffer_map.get()->createTileL(pos));
					}
					int variation = GetBrushVariation();
					brush->draw(doodad_buffer_map.get(), tile, &variation);
					// std::cout << "\tpos: " << tile->getPosition() << std::endl;
					doodad_buffer_map.get()->setTile(tile->getPosition(), tile);
					exit = true;
				}
				if (fail) {
					++retries;
					break;
				}
			}
			++object_count;
		}
	} else {
		if (brush->hasCompositeObjects(GetBrushVariation()) && random(brush->getTotalChance(GetBrushVariation())) <= brush->getCompositeChance(GetBrushVariation())) {
			// Composite
			const CompositeTileList& composites = brush->getComposite(GetBrushVariation());

			// All placement is valid...

			// Transfer items to the buffer
			for (const auto& composite : composites) {
				// Include the z offset in the position calculation 
				Position pos = center_pos + composite.first;
				const ItemVector& items = composite.second;
				Tile* tile = doodad_buffer_map.get()->allocator(doodad_buffer_map.get()->createTileL(pos));
				// std::cout << pos << " = " << center_pos << " + " << buffer_tile->getPosition() << std::endl;

				for (auto item : items) {
					tile->addItem(item->deepCopy());
				}
				doodad_buffer_map.get()->setTile(tile->getPosition(), tile);
			}
		} else if (brush->hasSingleObjects(GetBrushVariation())) {
			Tile* tile = doodad_buffer_map.get()->allocator(doodad_buffer_map.get()->createTileL(center_pos));
			int variation = GetBrushVariation();
			brush->draw(doodad_buffer_map.get(), tile, &variation);
			doodad_buffer_map.get()->setTile(center_pos, tile);
		}
	}
}

long GUI::PopupDialog(wxWindow* parent, wxString title, wxString text, long style, wxString confisavename, uint32_t configsavevalue) {
	if (text.empty()) {
		return wxID_ANY;
	}

	wxMessageDialog dlg(parent, text, title, style);
	return dlg.ShowModal();
}

long GUI::PopupDialog(wxString title, wxString text, long style, wxString configsavename, uint32_t configsavevalue) {
	return g_gui.PopupDialog(g_gui.root, title, text, style, configsavename, configsavevalue);
}

void GUI::ListDialog(wxWindow* parent, wxString title, const wxArrayString& param_items) {
    if (param_items.empty()) {
        return;
    }

    // Check if we should suppress map warnings
    if ((title.CmpNoCase("Warnings") == 0 || title.CmpNoCase("Warning") == 0) &&
        g_settings.getBoolean(Config::SUPPRESS_MAP_WARNINGS)) {
        return;
    }

    wxArrayString list_items(param_items);

    // Create the window
    wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);

    wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
    wxListBox* item_list = newd wxListBox(dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
    item_list->SetMinSize(wxSize(500, 300));

    for (size_t i = 0; i != list_items.GetCount();) {
        wxString str = list_items[i];
        size_t pos = str.find("\n");
        if (pos != wxString::npos) {
            // Split string!
            item_list->Append(str.substr(0, pos));
            list_items[i] = str.substr(pos + 1);
            continue;
        }
        item_list->Append(list_items[i]);
        ++i;
    }
    sizer->Add(item_list, 1, wxEXPAND);

    wxCheckBox* never_show_again = nullptr;
    if (title.CmpNoCase("Warnings") == 0 || title.CmpNoCase("Warning") == 0) {
        never_show_again = newd wxCheckBox(dlg, wxID_ANY, "Never show again");
        sizer->Add(never_show_again, 0, wxALL, 8);
    }

    wxSizer* stdsizer = newd wxBoxSizer(wxHORIZONTAL);
    stdsizer->Add(newd wxButton(dlg, wxID_OK, "OK"), wxSizerFlags(1).Center());
    sizer->Add(stdsizer, wxSizerFlags(0).Center());

    dlg->SetSizerAndFit(sizer);

    // Show the window
    dlg->ShowModal();

    // Save the setting if checked
    if (never_show_again && never_show_again->IsChecked()) {
        g_settings.setInteger(Config::SUPPRESS_MAP_WARNINGS, 1);
    }

    delete dlg;
}

void GUI::ShowTextBox(wxWindow* parent, wxString title, wxString content) {
	wxDialog* dlg = newd wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX);
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxTextCtrl* text_field = newd wxTextCtrl(dlg, wxID_ANY, content, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	text_field->SetMinSize(wxSize(400, 550));
	topsizer->Add(text_field, wxSizerFlags(5).Expand());

	wxSizer* choicesizer = newd wxBoxSizer(wxHORIZONTAL);
	choicesizer->Add(newd wxButton(dlg, wxID_CANCEL, "OK"), wxSizerFlags(1).Center());
	topsizer->Add(choicesizer, wxSizerFlags(0).Center());
	dlg->SetSizerAndFit(topsizer);

	dlg->ShowModal();
}

void GUI::SetHotkey(int index, Hotkey& hotkey) {
	ASSERT(index >= 0 && index <= 9);
	hotkeys[index] = hotkey;
	SetStatusText("Set hotkey " + i2ws(index) + ".");
}

const Hotkey& GUI::GetHotkey(int index) const {
	ASSERT(index >= 0 && index <= 9);
	return hotkeys[index];
}

void GUI::SaveHotkeys() const {
	std::ostringstream os;
	for (const auto& hotkey : hotkeys) {
		os << hotkey << '\n';
	}
	g_settings.setString(Config::NUMERICAL_HOTKEYS, os.str());
}

void GUI::LoadHotkeys() {
	std::istringstream is;
	is.str(g_settings.getString(Config::NUMERICAL_HOTKEYS));

	std::string line;
	int index = 0;
	while (getline(is, line)) {
		std::istringstream line_is;
		line_is.str(line);
		line_is >> hotkeys[index];

		++index;
	}
}

Hotkey::Hotkey() :
	type(NONE) {
	////
}

Hotkey::Hotkey(Position _pos) :
	type(POSITION), pos(_pos) {
	////
}

Hotkey::Hotkey(Brush* brush) :
	type(BRUSH), brushname(brush->getName()) {
	////
}

Hotkey::Hotkey(std::string _name) :
	type(BRUSH), brushname(_name) {
	////
}

Hotkey::~Hotkey() {
	////
}

std::ostream& operator<<(std::ostream& os, const Hotkey& hotkey) {
	switch (hotkey.type) {
		case Hotkey::POSITION: {
			os << "pos:{" << hotkey.pos << "}";
		} break;
		case Hotkey::BRUSH: {
			if (hotkey.brushname.find('{') != std::string::npos || hotkey.brushname.find('}') != std::string::npos) {
				break;
			}
			os << "brush:{" << hotkey.brushname << "}";
		} break;
		default: {
			os << "none:{}";
		} break;
	}
	return os;
}

std::istream& operator>>(std::istream& is, Hotkey& hotkey) {
	std::string type;
	getline(is, type, ':');
	if (type == "none") {
		is.ignore(2); // ignore "{}"
	} else if (type == "pos") {
		is.ignore(1); // ignore "{"
		Position pos;
		is >> pos;
		hotkey = Hotkey(pos);
		is.ignore(1); // ignore "}"
	} else if (type == "brush") {
		is.ignore(1); // ignore "{"
		std::string brushname;
		getline(is, brushname, '}');
		hotkey = Hotkey(brushname);
	} else {
		// Do nothing...
	}

	return is;
}

void SetWindowToolTip(wxWindow* a, const wxString& tip) {
	a->SetToolTip(tip);
}

void SetWindowToolTip(wxWindow* a, wxWindow* b, const wxString& tip) {
	a->SetToolTip(tip);
	b->SetToolTip(tip);
}

int GUI::GetHotkeyEventId(const std::string& action) {
	// Convert action string to menu event ID
	// This needs to be implemented based on your menu system
	// Return -1 if no matching action is found
	return -1; // Temporary return until implementation
}

void GUI::CheckAutoSave() {
	uint32_t now = time(nullptr);
	
	// Only check once per second
	if (now - last_autosave_check < 1) {
		return;
	}
	last_autosave_check = now;

	if (!g_settings.getBoolean(Config::AUTO_SAVE_ENABLED)) {
		//OutputDebugStringA("Autosave disabled\n");
		return;
	}
	
	if (!IsEditorOpen()) {
		OutputDebugStringA("No editor open - skipping autosave check\n");
		return;
	}

	uint32_t interval = g_settings.getInteger(Config::AUTO_SAVE_INTERVAL); // Already in seconds
	uint32_t time_passed = now - last_autosave;
	
	char debug_buffer[256];
	sprintf(debug_buffer, "Autosave check - Time passed: %u seconds, Interval: %u seconds, Next save in: %u seconds\n", 
			time_passed, interval, interval - (time_passed % interval));
	OutputDebugStringA(debug_buffer);
	
	if (now - last_autosave >= interval) {
		Editor* editor = GetCurrentEditor();
		if (editor) {
			OutputDebugStringA("Performing autosave...\n");
			
			// Create autosave directory in RME data folder
			wxString data_dir = GetDataDirectory();
			wxString autosave_dir = data_dir + "maps/autosave/";
			
			if (!wxDirExists(data_dir + "maps")) {
				wxMkdir(data_dir + "maps", wxS_DIR_DEFAULT);
			}
			if (!wxDirExists(autosave_dir)) {
				wxMkdir(autosave_dir, wxS_DIR_DEFAULT);
			}
			
			// Create autosave filename based on current map name
			FileName current = wxstr(editor->map.getName());
			wxString name = current.GetName();
			if (name.empty()) {
				name = "untitled";
			} else {
				// Remove any existing autosave timestamps from the name
				size_t pos;
				while ((pos = name.find("_autosave_")) != wxString::npos) {
					// Find the end of the timestamp (next underscore or end of string)
					size_t end = name.find("_", pos + 10); // Skip past "_autosave_"
					if (end == wxString::npos) {
						// If no underscore found, remove everything after _autosave_
						name = name.substr(0, pos);
					} else {
						// Remove the _autosave_ and timestamp portion
						name = name.substr(0, pos);
					}
				}

			/*
			} else {
				// Strip any existing autosave timestamps from the name
				wxString pattern = "_autosave_[0-9]{4}-[0-9]{2}-[0-9]{2}_[0-9]{2}-[0-9]{2}-[0-9]{2}";
				wxRegEx regex(pattern);
				while (regex.Matches(name)) {
					size_t start, len;
					regex.GetMatch(&start, &len);
					name = name.Mid(0, start) + name.Mid(start + len);
				}
			}
			
			*/
			}

			wxString ext = current.GetExt();
			if (ext.empty()) {
				ext = "otbm";
			}
			
			wxString autosave_name = autosave_dir + name + "_autosave_" + 
				wxDateTime::Now().Format("%Y-%m-%d_%H-%M-%S") + "." + ext;

			OutputDebugStringA("Saving to: ");
			OutputDebugStringA(autosave_name.c_str());
			OutputDebugStringA("\n");

			// Use existing SaveCurrentMap with our autosave filename
			SaveCurrentMap(autosave_name, false);
			last_autosave = now;
			OutputDebugStringA("Autosave complete\n");
		}
	}
}

void GUI::ApplyDarkMode() {
	if (root) {
		g_darkMode.ApplyTheme(root);
        
		// Apply to all floating windows and panes
		if (aui_manager) {
			wxAuiPaneInfoArray& panes = aui_manager->GetAllPanes();
			for (size_t i = 0; i < panes.GetCount(); ++i) {
				if (panes[i].window) {
					g_darkMode.ApplyTheme(panes[i].window);
				}
			}
			aui_manager->Update();
		}
        
		// Apply to the menu bar
		if (MainMenuBar* menuBar = dynamic_cast<MainMenuBar*>(root->GetMenuBar())) {
			g_darkMode.ApplyThemeToMainMenuBar(menuBar);
		}
        
		// Apply to minimap if it exists
		if (minimap) {
			g_darkMode.ApplyTheme(minimap);
		}
        
		// Apply to search window if it exists
		if (search_result_window) {
			g_darkMode.ApplyTheme(search_result_window);
		}
        
		// Apply to all palette windows
		for (PaletteWindow* palette : palettes) {
			if (palette) {
				g_darkMode.ApplyTheme(palette);
			}
		}
	}
}

// Detached views management
void GUI::RegisterDetachedView(Editor* editor, wxFrame* frame) {
	// Add the frame to the list of detached views for this editor
	detached_views[editor].push_back(frame);
	
	// Store a pointer to our detached_views map in the frame to allow proper cleanup
	// We'll use a custom event handler to check if the editor is still valid
	frame->Bind(wxEVT_IDLE, [this, editor, frame](wxIdleEvent& event) {
		// Check if the editor is still in our map (it may have been deleted)
		if (detached_views.find(editor) == detached_views.end()) {
			// Editor was deleted, close this detached view
			frame->Close(true);
		}
		event.Skip();
	});
}

void GUI::RegisterDockableView(Editor* editor, MapWindow* window) {
    // Skip registration if editor or window is null
    if (!editor || !window)
        return;
        
    // Add the dockable window to the list for this editor
    dockable_views[editor].push_back(window);
    
    // Add a handler to check for editor validity periodically
    window->Bind(wxEVT_IDLE, [this, editor, window](wxIdleEvent& event) {
        // Check if the editor is still valid (might have been deleted)
        bool editorExists = false;
        for (int i = 0; i < tabbook->GetTabCount(); ++i) {
            auto* mapTab = dynamic_cast<MapTab*>(tabbook->GetTab(i));
            if (mapTab && mapTab->GetEditor() == editor) {
                editorExists = true;
                break;
            }
        }
        
        // If editor no longer exists, close this window
        if (!editorExists && dockable_views.find(editor) != dockable_views.end()) {
            if (aui_manager->GetPane(window).IsOk()) {
                aui_manager->DetachPane(window);
                window->Destroy();
            }
            // The destroy event will trigger UnregisterDockableView
        }
        
        event.Skip();
    });
}

void GUI::UnregisterDetachedView(Editor* editor, wxFrame* frame) {
	// Remove the frame from the list of detached views for this editor
	if (detached_views.find(editor) != detached_views.end()) {
		detached_views[editor].remove(frame);
		
		// If the list is now empty, remove the editor from the map
		if (detached_views[editor].empty()) {
			detached_views.erase(editor);
		}
	}
}

void GUI::UnregisterDockableView(Editor* editor, MapWindow* window) {
	// Remove the window from the list of dockable views for this editor
	if (dockable_views.find(editor) != dockable_views.end()) {
		dockable_views[editor].remove(window);
		
		// If the list is now empty, remove the editor from the map
		if (dockable_views[editor].empty()) {
			dockable_views.erase(editor);
		}
	}
}

bool GUI::HasDetachedViews(Editor* editor) const {
	// Check if the editor has any detached views or dockable panels
	auto detached_it = detached_views.find(editor);
	auto dockable_it = dockable_views.find(editor);
	
	return (detached_it != detached_views.end() && !detached_it->second.empty()) || 
	       (dockable_it != dockable_views.end() && !dockable_it->second.empty());
}

bool GUI::CloseDetachedViews(Editor* editor) {
	bool had_views = false;

	// Close all detached frame views for the given editor
	auto frame_it = detached_views.find(editor);
	if (frame_it != detached_views.end()) {
		// Make a copy of the list since closing frames will modify the original
		std::list<wxFrame*> frames_to_close = frame_it->second;
		
		for (wxFrame* frame : frames_to_close) {
			// Force close the frame immediately instead of destroy
			// This ensures synchronous closing rather than asynchronous destruction
			frame->Close(true);
		}
		
		// Clear the list
		detached_views.erase(editor);
		had_views = true;
	}
	
	// Close all dockable panel views for the given editor
	auto dock_it = dockable_views.find(editor);
	if (dock_it != dockable_views.end()) {
		// Make a copy of the list since closing windows will modify the original
		std::list<MapWindow*> windows_to_close = dock_it->second;
		
		for (MapWindow* window : windows_to_close) {
			// For dockable panels, we need to remove them from the AUI manager
			if (aui_manager->GetPane(window).IsOk()) {
				aui_manager->DetachPane(window);
				window->Destroy();
			}
		}
		
		// Clear the list
		dockable_views.erase(editor);
		had_views = true;
		
		// Update the AUI manager to reflect the changes
		aui_manager->Update();
	}
	
	// Process any pending events to ensure everything is closed
	wxTheApp->ProcessPendingEvents();
	
	return had_views;
}

void GUI::UpdateDetachedViewsTitle(Editor* editor) {
	// Update titles of all detached views for this editor
	auto it = detached_views.find(editor);
	if (it != detached_views.end()) {
		for (wxFrame* frame : it->second) {
			wxString title = frame->GetTitle();
			if (title.Contains("Always-on-top View:")) {
				frame->SetTitle(wxString::Format("Always-on-top View: %s", wxstr(editor->map.getName())));
			} else {
				frame->SetTitle(wxString::Format("Detached View: %s", wxstr(editor->map.getName())));
			}
		}
	}
}

void GUI::StoreSearchState(uint16_t itemId, bool onSelection) {
	has_last_search = true;
	last_search_itemid = itemId;
	last_search_on_selection = onSelection;
	
	if (search_result_window) {
		last_ignored_ids_text = search_result_window->GetIgnoredItemsText();
		last_ignored_ids_enabled = search_result_window->IsIgnoreListEnabled();
	}
	
	OutputDebugStringA(wxString::Format("GUI::StoreSearchState - Stored search for item ID %d, ignore list %s\n",
		last_search_itemid, last_ignored_ids_enabled ? "enabled" : "disabled").c_str());
}

void GUI::RestoreSearchState(SearchResultWindow* window) {
	if (!window || !has_last_search)
		return;
		
	window->SetIgnoredIds(last_ignored_ids_text, last_ignored_ids_enabled);
	
	OutputDebugStringA(wxString::Format("GUI::RestoreSearchState - Restored search for item ID %d\n", last_search_itemid).c_str());
}

uint16_t GUI::GetCurrentActionID() const {
    PaletteWindow* palette = GetPalette();
    if (palette) {
        return palette->GetActionID();
    }
    return 0;
}

bool GUI::IsCurrentActionIDEnabled() const {
    PaletteWindow* palette = GetPalette();
    if (palette) {
        return palette->IsActionIDEnabled();
    }
    return false;
}

void GUI::SetCustomBrushSize(bool enable, int width, int height) {
	// Only allow custom brush size for square brushes
	if (enable && brush_shape != BRUSHSHAPE_SQUARE) {
		// Silently ignore request to enable custom brush size for circle brushes
		return;
	}
	
	use_custom_brush_size = enable;
	
	if (width != -1) {
		// CRITICAL FIX: Prevent zero or negative width
		if (width <= 0) {
			char debug_msg[256];
			sprintf(debug_msg, "DEBUG DRAG: WARNING! SetCustomBrushSize called with invalid width %d - FORCING TO 1\n", width);
			OutputDebugStringA(debug_msg);
			width = 1;
		}
		custom_brush_width = width;
	}
	
	if (height != -1) {
		// CRITICAL FIX: Prevent zero or negative height
		if (height <= 0) {
			char debug_msg[256];
			sprintf(debug_msg, "DEBUG DRAG: WARNING! SetCustomBrushSize called with invalid height %d - FORCING TO 1\n", height);
			OutputDebugStringA(debug_msg);
			height = 1;
		}
		custom_brush_height = height;
	}
	
	// ENHANCED FIX: Always validate current brush dimensions
	if (custom_brush_width <= 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! custom_brush_width is %d - FORCING TO 1\n", custom_brush_width);
		OutputDebugStringA(debug_msg);
		custom_brush_width = 1;
	}
	
	if (custom_brush_height <= 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! custom_brush_height is %d - FORCING TO 1\n", custom_brush_height);
		OutputDebugStringA(debug_msg);
		custom_brush_height = 1;
	}
	
	// Additional safety: make sure we have non-zero fallback values
	if (custom_brush_width <= 0) {
		custom_brush_width = std::max(1, brush_size + 1);
	}
	
	if (custom_brush_height <= 0) {
		custom_brush_height = std::max(1, brush_size + 1);
	}
	
	// Unselect all brush size buttons in the UI
	if (enable && brush_shape == BRUSHSHAPE_SQUARE) {
		for (auto& palette : palettes) {
			palette->OnUpdateBrushSize(brush_shape, -1);
		}
	} else {
		for (auto& palette : palettes) {
			palette->OnUpdateBrushSize(brush_shape, brush_size);
		}
	}
	
	RefreshView();
}
