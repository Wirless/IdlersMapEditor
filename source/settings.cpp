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
#include "gui_ids.h"
#include "client_version.h"

#include <wx/confbase.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>

#include <iostream>
#include <string>

Settings g_settings;

Settings::Settings() :
	store(Config::LAST)
#ifdef __WINDOWS__
	,
	use_file_cfg(false)
#endif
{
	setDefaults();
}

Settings::~Settings() {
	////
}

wxConfigBase& Settings::getConfigObject() {
	return *dynamic_cast<wxConfigBase*>(wxConfig::Get());
}

bool Settings::getBoolean(uint32_t key) const {
	if (key > Config::LAST) {
		return false;
	}

	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		return dv.intval != 0;
	}
	return false;
}

int Settings::getInteger(uint32_t key) const {
	if (key > Config::LAST) {
		return 0;
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		return dv.intval;
	}
	return 0;
}

float Settings::getFloat(uint32_t key) const {
	if (key > Config::LAST) {
		return 0.0;
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_FLOAT) {
		return dv.floatval;
	}
	return 0.0;
}

std::string Settings::getString(uint32_t key) const {
	if (key > Config::LAST) {
		return "";
	}
	const DynamicValue& dv = store[key];
	if (dv.type == TYPE_STR && dv.strval != nullptr) {
		return *dv.strval;
	}
	return "";
}

void Settings::setInteger(uint32_t key, int newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_INT) {
		dv.intval = newval;
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_INT;
		dv.intval = newval;
	}
}

void Settings::setFloat(uint32_t key, float newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_FLOAT) {
		dv.floatval = newval;
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_FLOAT;
		dv.floatval = newval;
	}
}

void Settings::setString(uint32_t key, std::string newval) {
	if (key > Config::LAST) {
		return;
	}
	DynamicValue& dv = store[key];
	if (dv.type == TYPE_STR) {
		delete dv.strval;
		dv.strval = newd std::string(newval);
	} else if (dv.type == TYPE_NONE) {
		dv.type = TYPE_STR;
		dv.strval = newd std::string(newval);
	}
}

std::string Settings::DynamicValue::str() {
	switch (type) {
		case TYPE_FLOAT:
			return f2s(floatval);
		case TYPE_STR:
			return std::string(*strval);
		case TYPE_INT:
			return i2s(intval);
		default:
		case TYPE_NONE:
			return "";
	}
}

void Settings::IO(IOMode mode) {
	wxConfigBase* conf = (mode == DEFAULT ? nullptr : dynamic_cast<wxConfigBase*>(wxConfig::Get()));

	using namespace Config;
#define section(s) \
	if (conf)      \
	conf->SetPath("/" s)
#define Int(key, dflt)                                     \
	do {                                                   \
		if (mode == DEFAULT) {                             \
			setInteger(key, dflt);                         \
		} else if (mode == SAVE) {                         \
			conf->Write(#key, getInteger(key));            \
		} else if (mode == LOAD) {                         \
			setInteger(key, conf->Read(#key, long(dflt))); \
		}                                                  \
	} while (false)
#define IntToSave(key, dflt)                               \
	do {                                                   \
		if (mode == DEFAULT) {                             \
			setInteger(key, dflt);                         \
		} else if (mode == SAVE) {                         \
			conf->Write(#key, getInteger(key##_TO_SAVE));  \
		} else if (mode == LOAD) {                         \
			setInteger(key, conf->Read(#key, (long)dflt)); \
			setInteger(key##_TO_SAVE, getInteger(key));    \
		}                                                  \
	} while (false)
#define Float(key, dflt)                        \
	do {                                        \
		if (mode == DEFAULT) {                  \
			setFloat(key, dflt);                \
		} else if (mode == SAVE) {              \
			conf->Write(#key, getFloat(key));   \
		} else if (mode == LOAD) {              \
			double tmp_float;                   \
			conf->Read(#key, &tmp_float, dflt); \
			setFloat(key, tmp_float);           \
		}                                       \
	} while (false)
#define String(key, dflt)                             \
	do {                                              \
		if (mode == DEFAULT) {                        \
			setString(key, dflt);                     \
		} else if (mode == SAVE) {                    \
			conf->Write(#key, wxstr(getString(key))); \
		} else if (mode == LOAD) {                    \
			wxString str;                             \
			conf->Read(#key, &str, dflt);             \
			setString(key, nstr(str));                \
		}                                             \
	} while (false)

	section("View");
	Int(TRANSPARENT_FLOORS, 0);
	Int(TRANSPARENT_ITEMS, 0);
	Int(SHOW_ALL_FLOORS, 1);
	Int(SHOW_INGAME_BOX, 0);
	Int(SHOW_LIGHTS, 0);
	Int(SHOW_LIGHT_STR, 0);
	Int(SHOW_TECHNICAL_ITEMS, 1);
	Int(SHOW_WAYPOINTS, 1);
	Int(SHOW_GRID, 0);
	Int(SHOW_EXTRA, 1);
	Int(SHOW_SHADE, 1);
	Int(SHOW_SPECIAL_TILES, 1);
	Int(SHOW_ZONE_AREAS, 1);
	Int(SHOW_SPAWNS, 1);
	Int(SHOW_ITEMS, 1);
	Int(HIGHLIGHT_ITEMS, 0);
	Int(HIGHLIGHT_LOCKED_DOORS, 1);
	Int(SHOW_CREATURES, 1);
	Int(SHOW_HOUSES, 1);
	Int(SHOW_BLOCKING, 0);
	Int(SHOW_TOOLTIPS, 1);
	Int(SHOW_ONLY_TILEFLAGS, 0);
	Int(SHOW_ONLY_MODIFIED_TILES, 0);
	Int(SHOW_PREVIEW, 1);
	Int(SHOW_WALL_HOOKS, 0);
	Int(SHOW_TOWNS, 0);
	Int(ALWAYS_SHOW_ZONES, 1);
	Int(EXT_HOUSE_SHADER, 1);
	

	section("Version");
	Int(VERSION_ID, 0);
	Int(CHECK_SIGNATURES, 0);
	Int(FORCE_CLIENT_ITEMS_OTB, 0);
	Int(USE_CUSTOM_DATA_DIRECTORY, 0);
	String(DATA_DIRECTORY, "");
	String(EXTENSIONS_DIRECTORY, "");
	String(ASSETS_DATA_DIRS, "");

	section("Editor");
	String(RECENT_FILES, "");
	Int(WORKER_THREADS, 1);
	Int(MERGE_MOVE, 0);
	Int(MERGE_PASTE, 0);
	Int(UNDO_SIZE, 40);
	Int(UNDO_MEM_SIZE, 64);
	Int(GROUP_ACTIONS, 1);
	Int(SELECTION_TYPE, SELECT_CURRENT_FLOOR);
	Int(COMPENSATED_SELECT, 1);
	Float(SCROLL_SPEED, 3.5f);
	Float(ZOOM_SPEED, 1.4f);
	Int(SWITCH_MOUSEBUTTONS, 0);
	Int(DOUBLECLICK_PROPERTIES, 1);
	Int(LISTBOX_EATS_ALL_EVENTS, 1);
	Int(BORDER_IS_GROUND, 0);
	Int(BORDERIZE_PASTE, 1);
	Int(BORDERIZE_DRAG, 1);
	Int(BORDERIZE_DRAG_THRESHOLD, 6000);
	Int(BORDERIZE_PASTE_THRESHOLD, 10000);
	Int(BORDERIZE_DELETE, 0);
	Int(ALWAYS_MAKE_BACKUP, 0);
	Int(USE_AUTOMAGIC, 1);
	Int(SAME_GROUND_TYPE_BORDER, 0);
	Int(WALLS_REPEL_BORDERS, 0);
	Int(LAYER_CARPETS, 0);
	Int(CUSTOM_BORDER_ENABLED, 0);
	Int(CUSTOM_BORDER_ID, 1);
	Int(HOUSE_BRUSH_REMOVE_ITEMS, 0);
	Int(AUTO_ASSIGN_DOORID, 1);
	Int(AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE, 0);
	Int(ERASER_LEAVE_UNIQUE, 1);
	Int(DOODAD_BRUSH_ERASE_LIKE, 0);
	Int(WARN_FOR_DUPLICATE_ID, 1);
	Int(AUTO_CREATE_SPAWN, 1);
	Int(DEFAULT_SPAWNTIME, 60);
	Int(MAX_SPAWN_RADIUS, 30);
	Int(CURRENT_SPAWN_RADIUS, 5);
	Int(DEFAULT_CLIENT_VERSION, CLIENT_VERSION_NONE);
	Int(RAW_LIKE_SIMONE, 1);
	Int(ONLY_ONE_INSTANCE, 1);
	Int(SHOW_TILESET_EDITOR, 0);
	Int(USE_OTBM_4_FOR_ALL_MAPS, 0);
	Int(USE_OTGZ, 1);
	Int(SAVE_WITH_OTB_MAGIC_NUMBER, 0);
	Int(REPLACE_SIZE, 500);
	Int(COPY_POSITION_FORMAT, 0);
	Int(REFRESH_RADIUS, 15);

	section("Graphics");
	Int(TEXTURE_MANAGEMENT, 1);
	Int(TEXTURE_CLEAN_PULSE, 15);
	Int(TEXTURE_LONGEVITY, 20);
	Int(TEXTURE_CLEAN_THRESHOLD, 2500);
	Int(SOFTWARE_CLEAN_THRESHOLD, 1800);
	Int(SOFTWARE_CLEAN_SIZE, 500);
	Int(ICON_BACKGROUND, 0);
	Int(HARD_REFRESH_RATE, 200);
	Int(HIDE_ITEMS_WHEN_ZOOMED, 1);
	String(SCREENSHOT_DIRECTORY, "");
	String(SCREENSHOT_FORMAT, "png");
	IntToSave(USE_MEMCACHED_SPRITES, 0);
	Int(MINIMAP_UPDATE_DELAY, 333);
	Int(MINIMAP_VIEW_BOX, 1);
	String(MINIMAP_EXPORT_DIR, "");
	String(TILESET_EXPORT_DIR, "");

	Int(CURSOR_RED, 0);
	Int(CURSOR_GREEN, 166);
	Int(CURSOR_BLUE, 0);
	Int(CURSOR_ALPHA, 128);
	Int(CURSOR_ALT_RED, 0);
	Int(CURSOR_ALT_GREEN, 166);
	Int(CURSOR_ALT_BLUE, 0);
	Int(CURSOR_ALT_ALPHA, 128);

	section("UI");
	Int(USE_LARGE_CONTAINER_ICONS, 1);
	Int(USE_LARGE_CHOOSE_ITEM_ICONS, 1);
	Int(USE_LARGE_TERRAIN_TOOLBAR, 1);
	Int(USE_LARGE_COLLECTION_TOOLBAR, 1);
	Int(USE_LARGE_DOODAD_SIZEBAR, 1);
	Int(USE_LARGE_ITEM_SIZEBAR, 1);
	Int(USE_LARGE_HOUSE_SIZEBAR, 1);
	Int(USE_LARGE_RAW_SIZEBAR, 1);
	Int(USE_GUI_SELECTION_SHADOW, 0);
	Int(PALETTE_COL_COUNT, 8);
	String(PALETTE_TERRAIN_STYLE, "large icons");
	String(PALETTE_COLLECTION_STYLE, "large icons");
	String(PALETTE_DOODAD_STYLE, "large icons");
	String(PALETTE_ITEM_STYLE, "listbox");
	String(PALETTE_RAW_STYLE, "listbox");

	section("Window");
	String(PALETTE_LAYOUT, "name=02c30f6048629894000011bc00000002;caption=Palette;state=2099148;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=245;besth=100;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=-1;floath=-1");
	Int(MINIMAP_VISIBLE, 0);
	String(MINIMAP_LAYOUT, "name=066e2bc8486298990000259a00000003;caption=Minimap;state=2099151;dir=4;layer=0;row=0;pos=0;prop=100000;bestw=170;besth=130;minw=-1;minh=-1;maxw=-1;maxh=-1;floatx=-1;floaty=-1;floatw=221;floath=164");
	Int(WINDOW_HEIGHT, 500);
	Int(WINDOW_WIDTH, 700);
	Int(WINDOW_MAXIMIZED, 0);
	Int(WELCOME_DIALOG, 1);

	section("Hotkeys");
	String(NUMERICAL_HOTKEYS, "none:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\nnone:{}\n");

	Int(SHOW_TOOLBAR_STANDARD, 1);
	Int(SHOW_TOOLBAR_BRUSHES, 0);
	Int(SHOW_TOOLBAR_POSITION, 0);
	Int(SHOW_TOOLBAR_SIZES, 0);
	String(TOOLBAR_STANDARD_LAYOUT, "");
	String(TOOLBAR_BRUSHES_LAYOUT, "");
	String(TOOLBAR_POSITION_LAYOUT, "");
	String(TOOLBAR_SIZES_LAYOUT, "");

	// experimental
	section("experimental");
	Int(EXPERIMENTAL_FOG, 0);

	// Network settings
	section("Network");
	String(LIVE_HOST, "localhost");
	Int(LIVE_PORT, 12356);
	String(LIVE_PASSWORD, "");
	String(LIVE_USERNAME, wxGetUserId().ToStdString());

	section("");
	Int(GOTO_WEBSITE_ON_BOOT, 0);
	Int(USE_UPDATER, 1);
	String(RECENT_EDITED_MAP_PATH, "");
	String(RECENT_EDITED_MAP_POSITION, "");
	String(OTS_DATA_DIRECTORY, "");
	Int(LAST_WEBSITES_OPEN_TIME, 0);

	Int(FIND_ITEM_MODE, 0);
	Int(JUMP_TO_ITEM_MODE, 0);

	// checkbox in terrain palette
	Int(DRAW_LOCKED_DOOR, 0);
	//Int(HIGHLIGHT_LOCKED_DOORS, 0);

	Int(AUTO_SAVE_ENABLED, 0);
	Int(AUTO_SAVE_INTERVAL, 5);
	Int(SUPPRESS_MAP_WARNINGS, 1);

	// Dark Mode
	section("Interface");
	Int(DARK_MODE, 0);
	Int(DARK_MODE_CUSTOM_COLOR, 0);
	Int(DARK_MODE_RED, 45);
	Int(DARK_MODE_GREEN, 45);
	Int(DARK_MODE_BLUE, 48);

	// Invisible Items Colors
	section("InvisibleItems");
	Int(INVISIBLE_ITEMS_ENABLE_CUSTOM, 0);
	Int(INVISIBLE_INVALID_RED, 255);
	Int(INVISIBLE_INVALID_GREEN, 0);
	Int(INVISIBLE_INVALID_BLUE, 0);
	Int(INVISIBLE_STAIRS_RED, 255);
	Int(INVISIBLE_STAIRS_GREEN, 255);
	Int(INVISIBLE_STAIRS_BLUE, 0);
	Int(INVISIBLE_WALKABLE_RED, 255);
	Int(INVISIBLE_WALKABLE_GREEN, 0);
	Int(INVISIBLE_WALKABLE_BLUE, 0);
	Int(INVISIBLE_WALL_RED, 0);
	Int(INVISIBLE_WALL_GREEN, 255);
	Int(INVISIBLE_WALL_BLUE, 255);
	String(INVISIBLE_CUSTOM_IDS, "");

	// Client Box Settings
	section("ClientBox");
	Int(INGAME_BOX_CUSTOM_SIZE_ENABLED, 0);
	Int(INGAME_BOX_WIDTH, 17);
	Int(INGAME_BOX_HEIGHT, 13);
	Int(INGAME_BOX_OFFSET_X, 0);
	Int(INGAME_BOX_OFFSET_Y, 2);

	// House creation settings
	section("HouseCreation");
	Int(MAX_HOUSE_TILES, 5000);
	Int(HOUSE_FLOOR_SCAN, 1);
	Int(AUTO_DETECT_HOUSE_EXIT, 1);

	// LOD (Level of Detail) settings
	section("LOD");
	Int(TOOLTIP_MAX_ZOOM, 10);
	Int(GROUND_ONLY_ZOOM_THRESHOLD, 8);
	Int(ITEM_DISPLAY_ZOOM_THRESHOLD, 10);
	Int(SPECIAL_FEATURES_ZOOM_THRESHOLD, 10);
	Int(ANIMATION_ZOOM_THRESHOLD, 2); 
	Int(EFFECTS_ZOOM_THRESHOLD, 6);
	Int(LIGHT_ZOOM_THRESHOLD, 4);
	Int(SHADE_ZOOM_THRESHOLD, 8);
	Int(TOWN_ZONE_ZOOM_THRESHOLD, 6);
	Int(GRID_ZOOM_THRESHOLD, 12);

	// Palette grid settings
	section("PaletteGrid");
	Int(GRID_CHUNK_SIZE, 3000);
	Int(GRID_VISIBLE_ROWS_MARGIN, 30);

	// Tooltip settings
	section("ToolTip");
	Int(TOOLTIP_SHOW, 0);
	Int(TOOLTIP_SHOW_HASSCRIPT, 0);
	Int(TOOLTIP_SHOW_TEXT, 0);
	Int(TOOLTIP_SHOW_ITEMID, 0);
	Int(TOOLTIP_SHOW_AID, 0);
	Int(TOOLTIP_SHOW_UID, 0);
	Int(TOOLTIP_SHOW_DOORID, 0);
	Int(TOOLTIP_SHOW_DESTINATION, 0);
	Int(TOOLTIP_SHOW_HOUSEID, 0);
	Int(HOUSE_CUSTOM_COLORS, 1);
	String(TOOLTIP_IGNORE_IDS, "");



#undef section
#undef Int
#undef IntToSave
#undef Float
#undef String
}

void Settings::load() {
	wxConfigBase* conf;
#ifdef __WINDOWS__
	FileName filename("editor.cfg");
	if (filename.FileExists()) { // Use local file if it exists
		wxFileInputStream file(filename.GetFullPath());
		conf = newd wxFileConfig(file);
		use_file_cfg = true;
		g_settings.setInteger(Config::INDIRECTORY_INSTALLATION, 1);
	} else { // Use registry
		conf = newd wxConfig("Mapeditor", "IdlerMapEditor", "", "", wxCONFIG_USE_GLOBAL_FILE);
		g_settings.setInteger(Config::INDIRECTORY_INSTALLATION, 0);
	}
#else
	FileName filename("./editor.cfg");
	if (filename.FileExists()) { // Use local file if it exists
		wxFileInputStream file(filename.GetFullPath());
		conf = newd wxFileConfig(file);
		g_settings.setInteger(Config::INDIRECTORY_INSTALLATION, 1);
	} else { // Else use global (user-specific) conf
		filename.Assign(wxStandardPaths::Get().GetUserConfigDir() + "/.rme/editor.cfg");
		if (filename.FileExists()) {
			wxFileInputStream file(filename.GetFullPath());
			conf = newd wxFileConfig(file);
		} else {
			wxStringInputStream dummy("");
			conf = newd wxFileConfig(dummy, wxConvAuto());
		}
		g_settings.setInteger(Config::INDIRECTORY_INSTALLATION, 0);
	}
#endif
	wxConfig::Set(conf);
	IO(LOAD);
}

void Settings::save(bool endoftheworld) {
	IO(SAVE);
#ifdef __WINDOWS__
	if (use_file_cfg) {
		wxFileConfig* conf = dynamic_cast<wxFileConfig*>(wxConfig::Get());
		if (!conf) {
			return;
		}
		FileName filename("editor.cfg");
		wxFileOutputStream file(filename.GetFullPath());
		conf->Save(file);
	}
#else
	wxFileConfig* conf = dynamic_cast<wxFileConfig*>(wxConfig::Get());
	if (!conf) {
		return;
	}
	FileName filename("./editor.cfg");
	if (filename.FileExists()) { // Use local file if it exists
		wxFileOutputStream file(filename.GetFullPath());
		conf->Save(file);
	} else { // Else use global (user-specific) conf
		wxString path = wxStandardPaths::Get().GetUserConfigDir() + "/.rme/editor.cfg";
		filename.Assign(path);
		filename.Mkdir(0755, wxPATH_MKDIR_FULL);
		wxFileOutputStream file(filename.GetFullPath());
		conf->Save(file);
	}
#endif
	if (endoftheworld) {
		wxConfigBase* conf = dynamic_cast<wxConfigBase*>(wxConfig::Get());
		wxConfig::Set(nullptr);
		delete conf;
	}
}

