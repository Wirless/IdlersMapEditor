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

#ifndef RME_SETTINGS_H_
#define RME_SETTINGS_H_

#include "main.h"

namespace Config {
	enum Key {
		NONE,
		VERSION_ID,

		USE_CUSTOM_DATA_DIRECTORY,
		DATA_DIRECTORY,
		EXTENSIONS_DIRECTORY,

		MERGE_MOVE,
		TEXTURE_MANAGEMENT,
		TEXTURE_CLEAN_PULSE,
		TEXTURE_CLEAN_THRESHOLD,
		TEXTURE_LONGEVITY,
		HARD_REFRESH_RATE,
		USE_MEMCACHED_SPRITES,
		USE_MEMCACHED_SPRITES_TO_SAVE,
		SOFTWARE_CLEAN_THRESHOLD,
		SOFTWARE_CLEAN_SIZE,
		TRANSPARENT_FLOORS,
		TRANSPARENT_ITEMS,
		SHOW_INGAME_BOX,
		SHOW_GRID,
		SHOW_EXTRA,
		SHOW_ALL_FLOORS,
		SHOW_CREATURES,
		SHOW_SPAWNS,
		SHOW_HOUSES,
		SHOW_SHADE,
		SHOW_SPECIAL_TILES,
		SHOW_ZONE_AREAS,
		HIGHLIGHT_ITEMS,
		SHOW_ITEMS,
		SHOW_BLOCKING,
		SHOW_TOOLTIPS,
		SHOW_PREVIEW,
		SHOW_WALL_HOOKS,
		SHOW_AS_MINIMAP,
		SHOW_ONLY_TILEFLAGS,
		SHOW_ONLY_MODIFIED_TILES,
		HIDE_ITEMS_WHEN_ZOOMED,
		GROUP_ACTIONS,
		SCROLL_SPEED,
		ZOOM_SPEED,
		UNDO_SIZE,
		UNDO_MEM_SIZE,
		MERGE_PASTE,
		SELECTION_TYPE,
		COMPENSATED_SELECT,
		BORDER_IS_GROUND,
		BORDERIZE_PASTE,
		BORDERIZE_DRAG,
		BORDERIZE_DRAG_THRESHOLD,
		BORDERIZE_PASTE_THRESHOLD,
		BORDERIZE_DELETE,
		ICON_BACKGROUND,
		ALWAYS_MAKE_BACKUP,
		USE_AUTOMAGIC,
		SAME_GROUND_TYPE_BORDER,
		WALLS_REPEL_BORDERS,
		LAYER_CARPETS,
		CUSTOM_BORDER_ENABLED,
		CUSTOM_BORDER_ID,
		HOUSE_BRUSH_REMOVE_ITEMS,
		AUTO_ASSIGN_DOORID,
		ERASER_LEAVE_UNIQUE,
		DOODAD_BRUSH_ERASE_LIKE,
		WARN_FOR_DUPLICATE_ID,
		USE_UPDATER,
		USE_OTBM_4_FOR_ALL_MAPS,
		USE_OTGZ,
		SAVE_WITH_OTB_MAGIC_NUMBER,
		REPLACE_SIZE,

		USE_LARGE_CONTAINER_ICONS,
		USE_LARGE_CHOOSE_ITEM_ICONS,
		USE_LARGE_TERRAIN_TOOLBAR,
		USE_LARGE_DOODAD_SIZEBAR,
		USE_LARGE_ITEM_SIZEBAR,
		USE_LARGE_HOUSE_SIZEBAR,
		USE_LARGE_RAW_SIZEBAR,
		USE_GUI_SELECTION_SHADOW,
		PALETTE_COL_COUNT,
		PALETTE_TERRAIN_STYLE,
		PALETTE_DOODAD_STYLE,
		PALETTE_ITEM_STYLE,
		PALETTE_RAW_STYLE,

		ASSETS_DATA_DIRS,
		DEFAULT_CLIENT_VERSION,
		CHECK_SIGNATURES,
		FORCE_CLIENT_ITEMS_OTB,

		CURSOR_RED,
		CURSOR_GREEN,
		CURSOR_BLUE,
		CURSOR_ALPHA,

		CURSOR_ALT_RED,
		CURSOR_ALT_GREEN,
		CURSOR_ALT_BLUE,
		CURSOR_ALT_ALPHA,

		SCREENSHOT_DIRECTORY,
		SCREENSHOT_FORMAT,
		MAX_SPAWN_RADIUS,
		CURRENT_SPAWN_RADIUS,
		AUTO_CREATE_SPAWN,
		DEFAULT_SPAWNTIME,
		SWITCH_MOUSEBUTTONS,
		DOUBLECLICK_PROPERTIES,
		LISTBOX_EATS_ALL_EVENTS,
		RAW_LIKE_SIMONE,
		WORKER_THREADS,
		COPY_POSITION_FORMAT,

		GOTO_WEBSITE_ON_BOOT,
		INDIRECTORY_INSTALLATION,
		AUTOCHECK_FOR_UPDATES,
		ONLY_ONE_INSTANCE,
		SHOW_TILESET_EDITOR,

		PALETTE_LAYOUT,
		MINIMAP_VISIBLE,
		MINIMAP_LAYOUT,
		MINIMAP_UPDATE_DELAY,
		MINIMAP_VIEW_BOX,
		MINIMAP_EXPORT_DIR,
		TILESET_EXPORT_DIR,
		WINDOW_HEIGHT,
		WINDOW_WIDTH,
		WINDOW_MAXIMIZED,
		WELCOME_DIALOG,

		NUMERICAL_HOTKEYS,
		RECENT_FILES,

		RECENT_EDITED_MAP_PATH,
		RECENT_EDITED_MAP_POSITION,

		OTS_DATA_DIRECTORY,

		FIND_ITEM_MODE,
		JUMP_TO_ITEM_MODE,

		SHOW_TOOLBAR_STANDARD,
		SHOW_TOOLBAR_BRUSHES,
		SHOW_TOOLBAR_POSITION,
		SHOW_TOOLBAR_SIZES,
		TOOLBAR_STANDARD_LAYOUT,
		TOOLBAR_BRUSHES_LAYOUT,
		TOOLBAR_POSITION_LAYOUT,
		TOOLBAR_SIZES_LAYOUT,

		// add new settings at the end to make sure nothing gets misread
		DRAW_LOCKED_DOOR,
		HIGHLIGHT_LOCKED_DOORS,
		PALETTE_COLLECTION_STYLE,
		USE_LARGE_COLLECTION_TOOLBAR,
		SHOW_LIGHTS,
		SHOW_LIGHT_STR,
		SHOW_TECHNICAL_ITEMS,
		SHOW_WAYPOINTS,

		EXPERIMENTAL_FOG,

		SHOW_TOWNS,
		ALWAYS_SHOW_ZONES,
		EXT_HOUSE_SHADER,

		AUTO_SELECT_RAW_ON_RIGHTCLICK,
		AUTO_SAVE_ENABLED,
		AUTO_SAVE_INTERVAL,
		SUPPRESS_MAP_WARNINGS,

		// Network/Live settings
		LIVE_HOST,
		LIVE_PORT,
		LIVE_PASSWORD,
		LIVE_USERNAME,

		DARK_MODE,
		DARK_MODE_CUSTOM_COLOR,
		DARK_MODE_RED,
		DARK_MODE_GREEN,
		DARK_MODE_BLUE,

		// House creation settings
		MAX_HOUSE_TILES,
		HOUSE_FLOOR_SCAN,
		AUTO_DETECT_HOUSE_EXIT,

		// LOD (Level of Detail) settings
		TOOLTIP_MAX_ZOOM,
		GROUND_ONLY_ZOOM_THRESHOLD,
		ITEM_DISPLAY_ZOOM_THRESHOLD,
		SPECIAL_FEATURES_ZOOM_THRESHOLD,
		ANIMATION_ZOOM_THRESHOLD,
		EFFECTS_ZOOM_THRESHOLD,
		LIGHT_ZOOM_THRESHOLD,
		SHADE_ZOOM_THRESHOLD,
		TOWN_ZONE_ZOOM_THRESHOLD,
		GRID_ZOOM_THRESHOLD,

		// Palette grid settings
		GRID_CHUNK_SIZE,
		GRID_VISIBLE_ROWS_MARGIN,

		// Website link control setting
		LAST_WEBSITES_OPEN_TIME,

		TOOLTIP_IGNORE_IDS,         // string: e.g. "658-660,6891,136"
		TOOLTIP_SHOW,               // bool: master switch
		TOOLTIP_SHOW_HASSCRIPT,     // bool: show hasScript
		TOOLTIP_SHOW_TEXT,          // bool: show text
		TOOLTIP_SHOW_ITEMID,         // bool: show item ID in tooltips
		TOOLTIP_SHOW_AID,             // bool: show action ID in tooltips
		TOOLTIP_SHOW_UID,             // bool: show unique ID in tooltips
		TOOLTIP_SHOW_DOORID,          // bool: show door ID in tooltips
		TOOLTIP_SHOW_DESTINATION,     // bool: show teleport destination in tooltips
		TOOLTIP_SHOW_HOUSEID,         // bool: show house ID in tooltips
		HOUSE_CUSTOM_COLORS,          // bool: use house ID colors to color house tiles
		
		REFRESH_RADIUS,               // int: radius for visible area refresh in multiplayer

		// Invisible Items Color Settings
		INVISIBLE_ITEMS_ENABLE_CUSTOM,    // bool: enable custom invisible item colors
		INVISIBLE_INVALID_RED,            // int: red component for invalid items (id=0)
		INVISIBLE_INVALID_GREEN,          // int: green component for invalid items (id=0)
		INVISIBLE_INVALID_BLUE,           // int: blue component for invalid items (id=0)
		INVISIBLE_STAIRS_RED,             // int: red component for invisible stairs (469)
		INVISIBLE_STAIRS_GREEN,           // int: green component for invisible stairs (469)
		INVISIBLE_STAIRS_BLUE,            // int: blue component for invisible stairs (469)
		INVISIBLE_WALKABLE_RED,           // int: red component for invisible walkable tiles
		INVISIBLE_WALKABLE_GREEN,         // int: green component for invisible walkable tiles
		INVISIBLE_WALKABLE_BLUE,          // int: blue component for invisible walkable tiles
		INVISIBLE_WALL_RED,               // int: red component for invisible walls (2187)
		INVISIBLE_WALL_GREEN,             // int: green component for invisible walls (2187)
		INVISIBLE_WALL_BLUE,              // int: blue component for invisible walls (2187)
		INVISIBLE_CUSTOM_IDS,             // string: custom invisible item IDs (e.g. "1234:255,0,0;5678:0,255,0")

		// Client Box Settings
		INGAME_BOX_CUSTOM_SIZE_ENABLED,   // bool: enable custom client box size
		INGAME_BOX_WIDTH,                 // int: custom client box width (default 17)
		INGAME_BOX_HEIGHT,                // int: custom client box height (default 13)
		INGAME_BOX_OFFSET_X,              // int: custom X border offset (default 0)
		INGAME_BOX_OFFSET_Y,              // int: custom Y border offset (default 2)

		// Depot auto-assignment settings
		AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE,  // bool: auto-assign depot town ID to closest temple

		LAST,
	};

	// Hotkey settings
	const std::string HOTKEY_PREFIX = "hotkey.";

	enum HotkeySettings : uint32_t {
		HOTKEY_BASE = 1000, // Start at 1000 to avoid conflicts
		// Add more specific hotkey settings as needed
	};
}

class wxConfigBase;

class Settings {
public:
	Settings();
	~Settings();

	bool getBoolean(uint32_t key) const;
	int getInteger(uint32_t key) const;
	float getFloat(uint32_t key) const;
	std::string getString(uint32_t key) const;

	void setInteger(uint32_t key, int newval);
	void setFloat(uint32_t key, float newval);
	void setString(uint32_t key, std::string newval);

	wxConfigBase& getConfigObject();
	void setDefaults() {
		IO(DEFAULT);
	}
	void load();
	void save(bool endoftheworld = false);

public:
	enum DynamicType {
		TYPE_NONE,
		TYPE_STR,
		TYPE_INT,
		TYPE_FLOAT,
	};
	class DynamicValue {
	public:
		DynamicValue() :
			type(TYPE_NONE) {
			intval = 0;
		};
		DynamicValue(DynamicType t) :
			type(t) {
			if (t == TYPE_STR) {
				strval = nullptr;
			} else if (t == TYPE_INT) {
				intval = 0;
			} else if (t == TYPE_FLOAT) {
				floatval = 0.0;
			} else {
				intval = 0;
			}
		};
		~DynamicValue() {
			if (type == TYPE_STR) {
				delete strval;
			}
		}
		DynamicValue(const DynamicValue& dv) :
			type(dv.type) {
			if (dv.type == TYPE_STR) {
				strval = newd std::string(*dv.strval);
			} else if (dv.type == TYPE_INT) {
				intval = dv.intval;
			} else if (dv.type == TYPE_FLOAT) {
				floatval = dv.floatval;
			} else {
				intval = 0;
			}
		};

		std::string str();

	private:
		DynamicType type;
		union {
			int intval;
			std::string* strval;
			float floatval;
		};

		friend class Settings;
	};

private:
	enum IOMode {
		DEFAULT,
		LOAD,
		SAVE,
	};
	void IO(IOMode mode);
	std::vector<DynamicValue> store;
#ifdef __WINDOWS__
	bool use_file_cfg;
#endif
};

extern Settings g_settings;

#endif
