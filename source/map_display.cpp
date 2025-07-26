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

#include <sstream>
#include <time.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include <algorithm>
#include <queue>
#include <set>

#include "gui.h"
#include "editor.h"
#include "brush.h"
#include "sprites.h"
#include "map.h"
#include "tile.h"
#include "old_properties_window.h"
#include "properties_window.h"
#include "tileset_window.h"
#include "palette_window.h"
#include "map_display.h"
#include "map_drawer.h"
#include "application.h"
#include "live_server.h"
#include "live_client.h"
#include "browse_tile_window.h"

#include "minimap_window.h"

#include "doodad_brush.h"
#include "house_exit_brush.h"
#include "house_brush.h"
#include "wall_brush.h"
#include "spawn_brush.h"
#include "creature_brush.h"
#include "ground_brush.h"
#include "waypoint_brush.h"
#include "raw_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"
#include "materials.h"
#include "selection.h"
#include "find_item_window.h"

BEGIN_EVENT_TABLE(MapCanvas, wxGLCanvas)
EVT_KEY_DOWN(MapCanvas::OnKeyDown)
EVT_KEY_UP(MapCanvas::OnKeyUp)

// Mouse events
EVT_MOTION(MapCanvas::OnMouseMove)
EVT_LEFT_UP(MapCanvas::OnMouseLeftRelease)
EVT_LEFT_DOWN(MapCanvas::OnMouseLeftClick)
EVT_LEFT_DCLICK(MapCanvas::OnMouseLeftDoubleClick)
EVT_MIDDLE_DOWN(MapCanvas::OnMouseCenterClick)
EVT_MIDDLE_UP(MapCanvas::OnMouseCenterRelease)
EVT_RIGHT_DOWN(MapCanvas::OnMouseRightClick)
EVT_RIGHT_UP(MapCanvas::OnMouseRightRelease)
EVT_MOUSEWHEEL(MapCanvas::OnWheel)
EVT_ENTER_WINDOW(MapCanvas::OnGainMouse)
EVT_LEAVE_WINDOW(MapCanvas::OnLoseMouse)

// Drawing events 
EVT_PAINT(MapCanvas::OnPaint)
EVT_ERASE_BACKGROUND(MapCanvas::OnEraseBackground)

// Menu events
EVT_MENU(MAP_POPUP_MENU_CUT, MapCanvas::OnCut)
EVT_MENU(MAP_POPUP_MENU_COPY, MapCanvas::OnCopy)
EVT_MENU(MAP_POPUP_MENU_COPY_POSITION, MapCanvas::OnCopyPosition)
EVT_MENU(MAP_POPUP_MENU_PASTE, MapCanvas::OnPaste)
EVT_MENU(MAP_POPUP_MENU_DELETE, MapCanvas::OnDelete)
EVT_MENU(MAP_POPUP_MENU_FILL, MapCanvas::OnFill)

EVT_MENU(MAP_POPUP_MENU_CREATE_HOUSE, MapCanvas::OnCreateHouse)
EVT_MENU(MAP_POPUP_MENU_FIND_SIMILAR_ITEMS, MapCanvas::OnFindSimilarItems)
//----
EVT_MENU(MAP_POPUP_MENU_COPY_SERVER_ID, MapCanvas::OnCopyServerId)
EVT_MENU(MAP_POPUP_MENU_COPY_CLIENT_ID, MapCanvas::OnCopyClientId)
EVT_MENU(MAP_POPUP_MENU_COPY_NAME, MapCanvas::OnCopyName)
EVT_MENU(MAP_POPUP_MENU_COPY_ACTION_ID, MapCanvas::OnCopyActionId)
EVT_MENU(MAP_POPUP_MENU_COPY_UNIQUE_ID, MapCanvas::OnCopyUniqueId)
EVT_MENU(MAP_POPUP_MENU_PASTE_ACTION_ID, MapCanvas::OnPasteActionId)
EVT_MENU(MAP_POPUP_MENU_PASTE_UNIQUE_ID, MapCanvas::OnPasteUniqueId)
// ----
EVT_MENU(MAP_POPUP_MENU_ROTATE, MapCanvas::OnRotateItem)
EVT_MENU(MAP_POPUP_MENU_GOTO, MapCanvas::OnGotoDestination)
EVT_MENU(MAP_POPUP_MENU_SWITCH_DOOR, MapCanvas::OnSwitchDoor)
// ----
EVT_MENU(MAP_POPUP_MENU_SELECT_RAW_BRUSH, MapCanvas::OnSelectRAWBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, MapCanvas::OnSelectGroundBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, MapCanvas::OnSelectDoodadBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, MapCanvas::OnSelectCollectionBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, MapCanvas::OnSelectDoorBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_WALL_BRUSH, MapCanvas::OnSelectWallBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, MapCanvas::OnSelectCarpetBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, MapCanvas::OnSelectTableBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, MapCanvas::OnSelectCreatureBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, MapCanvas::OnSelectSpawnBrush)
EVT_MENU(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, MapCanvas::OnSelectHouseBrush)
EVT_MENU(MAP_POPUP_MENU_MOVE_TO_TILESET, MapCanvas::OnSelectMoveTo)
// ----
EVT_MENU(MAP_POPUP_MENU_PROPERTIES, MapCanvas::OnProperties)
// ----
EVT_MENU(MAP_POPUP_MENU_BROWSE_TILE, MapCanvas::OnBrowseTile)
// Add to the event table after other MAP_POPUP_MENU items
EVT_MENU(MAP_POPUP_MENU_SELECTION_TO_DOODAD, MapCanvas::OnSelectionToDoodad)
EVT_MENU(MAP_POPUP_MENU_OPEN_REVSCRIPT, MapCanvas::OnOpenRevScript)
EVT_MENU(MAP_POPUP_MENU_OPEN_NPC_XML, MapCanvas::OnOpenNPCXML)
EVT_MENU(MAP_POPUP_MENU_OPEN_NPC_SCRIPT, MapCanvas::OnOpenNPCScript)
EVT_MENU(MAP_POPUP_MENU_OPEN_MONSTER_XML, MapCanvas::OnOpenMonsterXML)

END_EVENT_TABLE()

bool MapCanvas::processed[] = { 0 };

MapCanvas::MapCanvas(MapWindow* parent, Editor& editor, int* attriblist) :
	wxGLCanvas(parent, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS),
	editor(editor),
	floor(GROUND_LAYER),
	zoom(1.0),
	cursor_x(-1),
	cursor_y(-1),
	dragging(false),
	boundbox_selection(false),
	screendragging(false),
	drawing(false),
	dragging_draw(false),
	replace_dragging(false),

	screenshot_buffer(nullptr),

	drag_start_x(-1),
	drag_start_y(-1),
	drag_start_z(-1),

	last_cursor_map_x(-1),
	last_cursor_map_y(-1),
	last_cursor_map_z(-1),

	last_click_map_x(-1),
	last_click_map_y(-1),
	last_click_map_z(-1),
	last_click_abs_x(-1),
	last_click_abs_y(-1),
	last_click_x(-1),
	last_click_y(-1),

	last_mmb_click_x(-1),
	last_mmb_click_y(-1) {
	popup_menu = newd MapPopupMenu(editor);
	animation_timer = newd AnimationTimer(this);
	drawer = new MapDrawer(this);
	keyCode = WXK_NONE;
}

MapCanvas::~MapCanvas() {
	delete popup_menu;
	delete animation_timer;
	delete drawer;
	free(screenshot_buffer);
}

void MapCanvas::Refresh() {
	if (refresh_watch.Time() > g_settings.getInteger(Config::HARD_REFRESH_RATE)) {
		refresh_watch.Start();
		wxGLCanvas::Update();
	}
	wxGLCanvas::Refresh();
}

void MapCanvas::SetZoom(double value) {
	if (value < 0.125) {
		value = 0.125;
	}

	if (value > 25.00) {
		value = 25.0;
	}

	if (zoom != value) {
		int center_x, center_y;
		GetScreenCenter(&center_x, &center_y);

		zoom = value;
		static_cast<MapWindow*>(GetParent())->SetScreenCenterPosition(Position(center_x, center_y, floor));

		UpdatePositionStatus();
		UpdateZoomStatus();
		Refresh();
	}
}

void MapCanvas::GetViewBox(int* view_scroll_x, int* view_scroll_y, int* screensize_x, int* screensize_y) const {
	static_cast<MapWindow*>(GetParent())->GetViewSize(screensize_x, screensize_y);
	static_cast<MapWindow*>(GetParent())->GetViewStart(view_scroll_x, view_scroll_y);
}

void MapCanvas::OnPaint(wxPaintEvent& event) {
	SetCurrent(*g_gui.GetGLContext(this));

	if (g_gui.IsRenderingEnabled()) {
		DrawingOptions& options = drawer->getOptions();
		if (screenshot_buffer) {
			options.SetIngame();
		} else {
			options.transparent_floors = g_settings.getBoolean(Config::TRANSPARENT_FLOORS);
			options.transparent_items = g_settings.getBoolean(Config::TRANSPARENT_ITEMS);
			options.show_ingame_box = g_settings.getBoolean(Config::SHOW_INGAME_BOX);
			options.show_lights = g_settings.getBoolean(Config::SHOW_LIGHTS);
			options.show_light_str = g_settings.getBoolean(Config::SHOW_LIGHT_STR);
			options.show_tech_items = g_settings.getBoolean(Config::SHOW_TECHNICAL_ITEMS);
			options.show_waypoints = g_settings.getBoolean(Config::SHOW_WAYPOINTS);
			options.show_grid = g_settings.getInteger(Config::SHOW_GRID);
			options.ingame = !g_settings.getBoolean(Config::SHOW_EXTRA);
			options.show_all_floors = g_settings.getBoolean(Config::SHOW_ALL_FLOORS);
			options.show_creatures = g_settings.getBoolean(Config::SHOW_CREATURES);
			options.show_spawns = g_settings.getBoolean(Config::SHOW_SPAWNS);
			options.show_houses = g_settings.getBoolean(Config::SHOW_HOUSES);
			options.show_shade = g_settings.getBoolean(Config::SHOW_SHADE);
			options.show_special_tiles = g_settings.getBoolean(Config::SHOW_SPECIAL_TILES);
			options.show_zone_areas = g_settings.getBoolean(Config::SHOW_ZONE_AREAS);
			options.show_items = g_settings.getBoolean(Config::SHOW_ITEMS);
			options.highlight_items = g_settings.getBoolean(Config::HIGHLIGHT_ITEMS);
			options.highlight_locked_doors = g_settings.getBoolean(Config::HIGHLIGHT_LOCKED_DOORS);
			options.show_blocking = g_settings.getBoolean(Config::SHOW_BLOCKING);
			options.show_tooltips = g_settings.getBoolean(Config::SHOW_TOOLTIPS);
			options.show_as_minimap = g_settings.getBoolean(Config::SHOW_AS_MINIMAP);
			options.show_only_colors = g_settings.getBoolean(Config::SHOW_ONLY_TILEFLAGS);
			options.show_only_modified = g_settings.getBoolean(Config::SHOW_ONLY_MODIFIED_TILES);
			options.show_preview = g_settings.getBoolean(Config::SHOW_PREVIEW);
			options.show_hooks = g_settings.getBoolean(Config::SHOW_WALL_HOOKS);
			options.hide_items_when_zoomed = g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED);
			options.show_towns = g_settings.getBoolean(Config::SHOW_TOWNS);
			options.always_show_zones = g_settings.getBoolean(Config::ALWAYS_SHOW_ZONES);
			options.extended_house_shader = g_settings.getBoolean(Config::EXT_HOUSE_SHADER);

			options.experimental_fog = g_settings.getBoolean(Config::EXPERIMENTAL_FOG);
		}

		options.dragging = boundbox_selection;

		if (options.show_preview) {
			animation_timer->Start();
		} else {
			animation_timer->Stop();
		}

		drawer->SetupVars();
		drawer->SetupGL();
		drawer->Draw();

		if (screenshot_buffer) {
			drawer->TakeScreenshot(screenshot_buffer);
		}

		drawer->Release();
	}

	// Clean unused textures
	g_gui.gfx.garbageCollection();

	// Swap buffer
	SwapBuffers();

	// Send newd node requests
	editor.SendNodeRequests();
}

void MapCanvas::TakeScreenshot(wxFileName path, wxString format) {
	int screensize_x, screensize_y;
	GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

	delete[] screenshot_buffer;
	screenshot_buffer = newd uint8_t[3 * screensize_x * screensize_y];

	// Draw the window
	Refresh();
	wxGLCanvas::Update(); // Forces immediate redraws the window.

	// screenshot_buffer should now contain the screenbuffer
	if (screenshot_buffer == nullptr) {
		g_gui.PopupDialog("Capture failed", "Image capture failed. Old Video Driver?", wxOK);
	} else {
		// We got the shit
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);
		wxImage screenshot(screensize_x, screensize_y, screenshot_buffer);

		time_t t = time(nullptr);
		struct tm* current_time = localtime(&t);
		ASSERT(current_time);

		wxString date;
		date << "screenshot_" << (1900 + current_time->tm_year);
		if (current_time->tm_mon < 9) {
			date << "-"
				 << "0" << current_time->tm_mon + 1;
		} else {
			date << "-" << current_time->tm_mon + 1;
		}
		date << "-" << current_time->tm_mday;
		date << "-" << current_time->tm_hour;
		date << "-" << current_time->tm_min;
		date << "-" << current_time->tm_sec;

		int type = 0;
		path.SetName(date);
		if (format == "bmp") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_BMP;
		} else if (format == "png") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_PNG;
		} else if (format == "jpg" || format == "jpeg") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_JPEG;
		} else if (format == "tga") {
			path.SetExt(format);
			type = wxBITMAP_TYPE_TGA;
		} else {
			g_gui.SetStatusText("Unknown screenshot format \'" + format + "\', switching to default (png)");
			path.SetExt("png");
			;
			type = wxBITMAP_TYPE_PNG;
		}

		path.Mkdir(0755, wxPATH_MKDIR_FULL);
		wxFileOutputStream of(path.GetFullPath());
		if (of.IsOk()) {
			if (screenshot.SaveFile(of, static_cast<wxBitmapType>(type))) {
				g_gui.SetStatusText("Took screenshot and saved as " + path.GetFullName());
			} else {
				g_gui.PopupDialog("File error", "Couldn't save image file correctly.", wxOK);
			}
		} else {
			g_gui.PopupDialog("File error", "Couldn't open file " + path.GetFullPath() + " for writing.", wxOK);
		}
	}

	Refresh();

	screenshot_buffer = nullptr;
}

void MapCanvas::ScreenToMap(int screen_x, int screen_y, int* map_x, int* map_y) {
	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

	screen_x *= GetContentScaleFactor();
	screen_y *= GetContentScaleFactor();

	if (screen_x < 0) {
		*map_x = (start_x + screen_x) / TileSize;
	} else {
		*map_x = int(start_x + (screen_x * zoom)) / TileSize;
	}

	if (screen_y < 0) {
		*map_y = (start_y + screen_y) / TileSize;
	} else {
		*map_y = int(start_y + (screen_y * zoom)) / TileSize;
	}

	if (floor <= GROUND_LAYER) {
		*map_x += GROUND_LAYER - floor;
		*map_y += GROUND_LAYER - floor;
	} /* else {
		 *map_x += MAP_MAX_LAYER - floor;
		 *map_y += MAP_MAX_LAYER - floor;
	 }*/
}

void MapCanvas::GetScreenCenter(int* map_x, int* map_y) {
	int width, height;
	static_cast<MapWindow*>(GetParent())->GetViewSize(&width, &height);
	return ScreenToMap(width / 2, height / 2, map_x, map_y);
}

Position MapCanvas::GetCursorPosition() const {
	return Position(last_cursor_map_x, last_cursor_map_y, floor);
}

void MapCanvas::UpdatePositionStatus(int x, int y) {
	if (x == -1) {
		x = cursor_x;
	}
	if (y == -1) {
		y = cursor_y;
	}

	int map_x, map_y;
	ScreenToMap(x, y, &map_x, &map_y);

	wxString ss;
	ss << "x: " << map_x << " y:" << map_y << " z:" << floor;
	g_gui.root->SetStatusText(ss, 2);

	ss = "";
	Tile* tile = editor.map.getTile(map_x, map_y, floor);
	if (tile) {
		// Count total items on tile
		int itemCount = 0;
		if (tile->ground) itemCount++;
		itemCount += tile->items.size();
		
		if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			ss << "[" << itemCount << "] Spawn radius: " << tile->spawn->getSize();
		} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			ss << "[" << itemCount << "] " << (tile->creature->isNpc() ? "NPC" : "Monster");
			ss << " \"" << wxstr(tile->creature->getName()) << "\" spawntime: " << tile->creature->getSpawnTime();
		} else if (Item* item = tile->getTopItem()) {
			ss << "[" << itemCount << "] Item \"" << wxstr(item->getName()) << "\"";
			ss << " id:" << item->getID();
			ss << " cid:" << item->getClientID();
			if (item->getUniqueID()) {
				ss << " uid:" << item->getUniqueID();
			}
			if (item->getActionID()) {
				ss << " aid:" << item->getActionID();
			}
			if (item->hasWeight()) {
				wxString s;
				s.Printf("%.2f", item->getWeight());
				ss << " weight: " << s;
			}
		} else {
			ss << "[" << itemCount << "] Nothing";
		}
	} else {
		ss << "[0] Nothing";
	}

	if (editor.IsLive()) {
		editor.GetLive().updateCursor(Position(map_x, map_y, floor));
	}

	g_gui.root->SetStatusText(ss, 1);
}

void MapCanvas::UpdateZoomStatus() {
	int percentage = (int)((1.0 / zoom) * 100);
	wxString ss;
	ss << "zoom: " << percentage << "%";
	g_gui.root->SetStatusText(ss, 3);
}

void MapCanvas::OnMouseMove(wxMouseEvent& event) {
	if (screendragging) {
		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(g_settings.getFloat(Config::SCROLL_SPEED) * zoom * (event.GetX() - cursor_x)), int(g_settings.getFloat(Config::SCROLL_SPEED) * zoom * (event.GetY() - cursor_y)));
		Refresh();
	}

	cursor_x = event.GetX();
	cursor_y = event.GetY();

	int mouse_map_x, mouse_map_y;
	MouseToMap(&mouse_map_x, &mouse_map_y);

	bool map_update = false;
	if (last_cursor_map_x != mouse_map_x || last_cursor_map_y != mouse_map_y || last_cursor_map_z != floor) {
		map_update = true;
	}
	last_cursor_map_x = mouse_map_x;
	last_cursor_map_y = mouse_map_y;
	last_cursor_map_z = floor;

	if (map_update) {
		UpdatePositionStatus(cursor_x, cursor_y);
		UpdateZoomStatus();
	}

	if (g_gui.IsSelectionMode()) {
		if (map_update && isPasting()) {
			Refresh();
		} else if (map_update && dragging) {
			wxString ss;

			int move_x = drag_start_x - mouse_map_x;
			int move_y = drag_start_y - mouse_map_y;
			int move_z = drag_start_z - floor;
			
			// Add debugging output for drag calculations
			char debug_msg[512];
			sprintf(debug_msg, "DEBUG DRAG: Dragging detected - start_pos=(%d,%d,%d), mouse_pos=(%d,%d,%d), move_offset=(%d,%d,%d)\n", 
				drag_start_x, drag_start_y, drag_start_z, mouse_map_x, mouse_map_y, floor, move_x, move_y, move_z);
			OutputDebugStringA(debug_msg);
			
			ss << "Dragging " << -move_x << "," << -move_y << "," << -move_z;
			g_gui.SetStatusText(ss);

			Refresh();
		} else if (boundbox_selection) {
			if (map_update) {
				wxString ss;

				int move_x = std::abs(last_click_map_x - mouse_map_x);
				int move_y = std::abs(last_click_map_y - mouse_map_y);
				ss << "Selection " << move_x + 1 << ":" << move_y + 1;
				g_gui.SetStatusText(ss);
			}

			Refresh();
		}
	} else { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (map_update && drawing && brush) {
			if (brush->isDoodad()) {
				if (event.ControlDown()) {
					PositionVector tilestodraw;
					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, nullptr);
					editor.undraw(tilestodraw, event.ShiftDown() || event.AltDown());
				} else {
					editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());
				}
			} else if (brush->isDoor()) {
				if (!brush->canDraw(&editor.map, Position(mouse_map_x, mouse_map_y, floor))) {
					// We don't have to waste an action in this case...
				} else {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor));

					tilestoborder.push_back(Position(mouse_map_x, mouse_map_y - 1, floor));
					tilestoborder.push_back(Position(mouse_map_x - 1, mouse_map_y, floor));
					tilestoborder.push_back(Position(mouse_map_x, mouse_map_y + 1, floor));
					tilestoborder.push_back(Position(mouse_map_x + 1, mouse_map_y, floor));

					if (event.ControlDown()) {
						editor.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				}
			} else if (brush->needBorders()) {
				PositionVector tilestodraw, tilestoborder;

				getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, &tilestoborder);

				if (event.ControlDown()) {
					editor.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			} else if (brush->oneSizeFitsAll()) {
				drawing = true;
				PositionVector tilestodraw;
				tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor));

				if (event.ControlDown()) {
					editor.undraw(tilestodraw, event.AltDown());
				} else {
					editor.draw(tilestodraw, event.AltDown());
				}
			} else { // No borders
				PositionVector tilestodraw;

				for (int y = -g_gui.GetBrushSize(); y <= g_gui.GetBrushSize(); y++) {
					for (int x = -g_gui.GetBrushSize(); x <= g_gui.GetBrushSize(); x++) {
						if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
							tilestodraw.push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
						} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
							double distance = sqrt(double(x * x) + double(y * y));
							if (distance < g_gui.GetBrushSize() + 0.005) {
								tilestodraw.push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
							}
						}
					}
				}
				if (event.ControlDown()) {
					editor.undraw(tilestodraw, event.AltDown());
				} else {
					editor.draw(tilestodraw, event.AltDown());
				}
			}

			// Create newd doodad layout (does nothing if a non-doodad brush is selected)
			g_gui.FillDoodadPreviewBuffer();

			g_gui.RefreshView();
		} else if (dragging_draw) {
			g_gui.RefreshView();
		} else if (map_update && brush) {
			Refresh();
		}
	}
}

void MapCanvas::OnMouseLeftRelease(wxMouseEvent& event) {
	OnMouseActionRelease(event);
}

void MapCanvas::OnMouseLeftClick(wxMouseEvent& event) {
	OnMouseActionClick(event);
}

void MapCanvas::OnMouseLeftDoubleClick(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::DOUBLECLICK_PROPERTIES)) {
		int mouse_map_x, mouse_map_y;
		ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
		Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);

		if (tile && tile->size() > 0) {
			Tile* new_tile = tile->deepCopy(editor.map);
			wxDialog* w = nullptr;
			if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
				w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->spawn);
			} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
				w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->creature);
			} else if (Item* item = new_tile->getTopItem()) {
				if (editor.map.getVersion().otbm >= MAP_OTBM_4) {
					w = newd PropertiesWindow(g_gui.root, &editor.map, new_tile, item);
				} else {
					w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, item);
				}
			} else {
				return;
			}

			int ret = w->ShowModal();
			if (ret != 0) {
				Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
				action->addChange(newd Change(new_tile));
				editor.addAction(action);
			} else {
				// Cancel!a
				delete new_tile;
			}
			w->Destroy();
		}
	}
}

void MapCanvas::OnMouseCenterClick(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesClick(event);
	} else {
		OnMouseCameraClick(event);
	}
}

void MapCanvas::OnMouseCenterRelease(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMousePropertiesRelease(event);
	} else {
		OnMouseCameraRelease(event);
	}
}

void MapCanvas::OnMouseRightClick(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraClick(event);
	} else {
		OnMousePropertiesClick(event);
	}
}

void MapCanvas::OnMouseRightRelease(wxMouseEvent& event) {
	if (g_settings.getInteger(Config::SWITCH_MOUSEBUTTONS)) {
		OnMouseCameraRelease(event);
	} else {
		OnMousePropertiesRelease(event);
	}
}

void MapCanvas::OnMouseActionClick(wxMouseEvent& event) {
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	if (event.ControlDown() && event.AltDown()) {
		Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
		if (tile && tile->size() > 0) {
			Item* item = tile->getTopItem();
			if (item && item->getRAWBrush()) {
				g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
			}
		}
	} else if (g_gui.IsSelectionMode()) {
		if (isPasting()) {
			// Set paste to false (no rendering etc.)
			EndPasting();

			// Paste to the map
			editor.copybuffer.paste(editor, Position(mouse_map_x, mouse_map_y, floor));

			// Start dragging
			dragging = true;
			drag_start_x = mouse_map_x;
			drag_start_y = mouse_map_y;
			drag_start_z = floor;
		} else {
			do {
				boundbox_selection = false;
				if (event.ShiftDown()) {
					boundbox_selection = true;

					if (!event.ControlDown()) {
						editor.selection.start(); // Start selection session
						editor.selection.clear(); // Clear out selection
						editor.selection.finish(); // End selection session
						editor.selection.updateSelectionCount();
					}
				} else if (event.ControlDown()) {
					Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
					if (tile) {
						if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
							editor.selection.start(); // Start selection session
							if (tile->spawn->isSelected()) {
								editor.selection.remove(tile, tile->spawn);
							} else {
								editor.selection.add(tile, tile->spawn);
							}
							editor.selection.finish(); // Finish selection session
							editor.selection.updateSelectionCount();
						} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
							editor.selection.start(); // Start selection session
							if (tile->creature->isSelected()) {
								editor.selection.remove(tile, tile->creature);
							} else {
								editor.selection.add(tile, tile->creature);
							}
							editor.selection.finish(); // Finish selection session
							editor.selection.updateSelectionCount();
						} else {
							Item* item = tile->getTopItem();
							if (item) {
								editor.selection.start(); // Start selection session
								if (item->isSelected()) {
									editor.selection.remove(tile, item);
								} else {
									editor.selection.add(tile, item);
								}
								editor.selection.finish(); // Finish selection session
								editor.selection.updateSelectionCount();
							}
						}
					}
				} else {
					Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
					if (!tile) {
						editor.selection.start(); // Start selection session
						editor.selection.clear(); // Clear out selection
						editor.selection.finish(); // End selection session
						editor.selection.updateSelectionCount();
					} else if (tile->isSelected()) {
						dragging = true;
						drag_start_x = mouse_map_x;
						drag_start_y = mouse_map_y;
						drag_start_z = floor;
					} else {
						editor.selection.start(); // Start a selection session
						editor.selection.clear();
						editor.selection.commit();
						if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
							editor.selection.add(tile, tile->spawn);
							dragging = true;
							drag_start_x = mouse_map_x;
							drag_start_y = mouse_map_y;
							drag_start_z = floor;
						} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
							editor.selection.add(tile, tile->creature);
							dragging = true;
							drag_start_x = mouse_map_x;
							drag_start_y = mouse_map_y;
							drag_start_z = floor;
						} else {
							Item* item = tile->getTopItem();
							if (item) {
								editor.selection.add(tile, item);
								dragging = true;
								drag_start_x = mouse_map_x;
								drag_start_y = mouse_map_y;
								drag_start_z = floor;
							}
						}
						editor.selection.finish(); // Finish the selection session
						editor.selection.updateSelectionCount();
					}
				}
			} while (false);
		}
	} else if (g_gui.GetCurrentBrush()) { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (event.ShiftDown() && brush->canDrag()) {
			dragging_draw = true;
		} else {
			if (g_gui.GetBrushSize() == 0 && !brush->oneSizeFitsAll()) {
				drawing = true;
			} else {
				drawing = g_gui.GetCurrentBrush()->canSmear();
			}
			if (brush->isWall()) {
				if (event.AltDown() && g_gui.GetBrushSize() == 0) {
					// z0mg, just clicked a tile, shift variaton.
					if (event.ControlDown()) {
						editor.undraw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					} else {
						editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					}
				} else {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					int start_map_x = mouse_map_x - g_gui.GetBrushSize();
					int start_map_y = mouse_map_y - g_gui.GetBrushSize();
					int end_map_x = mouse_map_x + g_gui.GetBrushSize();
					int end_map_y = mouse_map_y + g_gui.GetBrushSize();

					for (int y = start_map_y - 1; y <= end_map_y + 1; ++y) {
						for (int x = start_map_x - 1; x <= end_map_x + 1; ++x) {
							if ((x <= start_map_x + 1 || x >= end_map_x - 1) || (y <= start_map_y + 1 || y >= end_map_y - 1)) {
								tilestoborder.push_back(Position(x, y, floor));
							}
							if (((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) && ((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
								tilestodraw.push_back(Position(x, y, floor));
							}
						}
					}
					if (event.ControlDown()) {
						editor.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				}
			} else if (brush->isDoor()) {
				PositionVector tilestodraw;
				PositionVector tilestoborder;

				tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor));

				tilestoborder.push_back(Position(mouse_map_x, mouse_map_y - 1, floor));
				tilestoborder.push_back(Position(mouse_map_x - 1, mouse_map_y, floor));
				tilestoborder.push_back(Position(mouse_map_x, mouse_map_y + 1, floor));
				tilestoborder.push_back(Position(mouse_map_x + 1, mouse_map_y, floor));

				if (event.ControlDown()) {
					editor.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			} else if (brush->isDoodad() || brush->isSpawn() || brush->isCreature()) {
				if (event.ControlDown()) {
					if (brush->isDoodad()) {
						PositionVector tilestodraw;
						getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, nullptr);
						editor.undraw(tilestodraw, event.AltDown() || event.ShiftDown());
					} else {
						editor.undraw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());
					}
				} else {
					bool will_show_spawn = false;
					if (brush->isSpawn() || brush->isCreature()) {
						if (!g_settings.getBoolean(Config::SHOW_SPAWNS)) {
							Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
							if (!tile || !tile->spawn) {
								will_show_spawn = true;
							}
						}
					}

					editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.ShiftDown() || event.AltDown());

					if (will_show_spawn) {
						Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
						if (tile && tile->spawn) {
							g_settings.setInteger(Config::SHOW_SPAWNS, true);
							g_gui.UpdateMenubar();
						}
					}
				}
			} else {
				if (brush->isGround() && event.AltDown()) {
					replace_dragging = true;
					Tile* draw_tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
					if (draw_tile) {
						editor.replace_brush = draw_tile->getGroundBrush();
					} else {
						editor.replace_brush = nullptr;
					}
				}

				if (brush->needBorders()) {
					PositionVector tilestodraw;
					PositionVector tilestoborder;

					bool fill = keyCode == WXK_CONTROL_D && event.ControlDown() && brush->isGround();
					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, &tilestoborder, fill);

					if (!fill && event.ControlDown()) {
						editor.undraw(tilestodraw, tilestoborder, event.AltDown());
					} else {
						editor.draw(tilestodraw, tilestoborder, event.AltDown());
					}
				} else if (brush->oneSizeFitsAll()) {
					if (brush->isHouseExit() || brush->isWaypoint()) {
						editor.draw(Position(mouse_map_x, mouse_map_y, floor), event.AltDown());
					} else {
						PositionVector tilestodraw;
						tilestodraw.push_back(Position(mouse_map_x, mouse_map_y, floor));
						if (event.ControlDown()) {
							editor.undraw(tilestodraw, event.AltDown());
						} else {
							editor.draw(tilestodraw, event.AltDown());
						}
					}
				} else {
					PositionVector tilestodraw;

					getTilesToDraw(mouse_map_x, mouse_map_y, floor, &tilestodraw, nullptr);

					if (event.ControlDown()) {
						editor.undraw(tilestodraw, event.AltDown());
					} else {
						editor.draw(tilestodraw, event.AltDown());
					}
				}
			}
			// Change the doodad layout brush
			g_gui.FillDoodadPreviewBuffer();
		}
	}
	last_click_x = int(event.GetX() * zoom);
	last_click_y = int(event.GetY() * zoom);

	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	last_click_map_z = floor;
	g_gui.RefreshView();
	g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseActionRelease(wxMouseEvent& event) {
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

	int move_x = last_click_map_x - mouse_map_x;
	int move_y = last_click_map_y - mouse_map_y;
	int move_z = last_click_map_z - floor;
	
	// Add debugging output for mouse action release
	char debug_msg[512];
	sprintf(debug_msg, "DEBUG DRAG: OnMouseActionRelease - mouse_pos=(%d,%d), last_click=(%d,%d,%d), move_offset=(%d,%d,%d)\n", 
		mouse_map_x, mouse_map_y, last_click_map_x, last_click_map_y, last_click_map_z, move_x, move_y, move_z);
	OutputDebugStringA(debug_msg);

	if (g_gui.IsSelectionMode()) {
		if (dragging && (move_x != 0 || move_y != 0 || move_z != 0)) {
			sprintf(debug_msg, "DEBUG DRAG: Calling editor.moveSelection with Position(%d,%d,%d), dragging=%d\n", 
				move_x, move_y, move_z, dragging);
			OutputDebugStringA(debug_msg);
			
			editor.moveSelection(Position(move_x, move_y, move_z));
			
			OutputDebugStringA("DEBUG DRAG: editor.moveSelection completed\n");
		} else {
			sprintf(debug_msg, "DEBUG DRAG: Not moving selection - dragging=%d, move_offset=(%d,%d,%d)\n", 
				dragging, move_x, move_y, move_z);
			OutputDebugStringA(debug_msg);
			
			if (boundbox_selection) {
				if (mouse_map_x == last_click_map_x && mouse_map_y == last_click_map_y && event.ControlDown()) {
					// Mouse hasn't moved, do control+shift thingy!
					Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
					if (tile) {
						editor.selection.start(); // Start a selection session
						if (tile->isSelected()) {
							editor.selection.remove(tile);
						} else {
							editor.selection.add(tile);
						}
						editor.selection.finish(); // Finish the selection session
						editor.selection.updateSelectionCount();
					}
				} else {
					// The cursor has moved, do some boundboxing!
					if (last_click_map_x > mouse_map_x) {
						int tmp = mouse_map_x;
						mouse_map_x = last_click_map_x;
						last_click_map_x = tmp;
					}
					if (last_click_map_y > mouse_map_y) {
						int tmp = mouse_map_y;
						mouse_map_y = last_click_map_y;
						last_click_map_y = tmp;
					}

					int numtiles = 0;
					int threadcount = std::max(g_settings.getInteger(Config::WORKER_THREADS), 1);

					int start_x = 0, start_y = 0, start_z = 0;
					int end_x = 0, end_y = 0, end_z = 0;

					switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
						case SELECT_CURRENT_FLOOR: {
							start_z = end_z = floor;
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							break;
						}
						case SELECT_ALL_FLOORS: {
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							start_z = MAP_MAX_LAYER;
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor;

							if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
								start_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
								start_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

								end_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
								end_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
							}

							numtiles = (start_z - end_z) * (end_x - start_x) * (end_y - start_y);
							break;
						}
						case SELECT_VISIBLE_FLOORS: {
							start_x = last_click_map_x;
							start_y = last_click_map_y;
							if (floor <= GROUND_LAYER) {
								start_z = GROUND_LAYER;
							} else {
								start_z = std::min(MAP_MAX_LAYER, floor + 2);
							}
							end_x = mouse_map_x;
							end_y = mouse_map_y;
							end_z = floor;

							if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
								start_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
								start_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

								end_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
								end_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
							}
							break;
						}
					}

					if (numtiles < 500) {
						// No point in threading for such a small set.
						threadcount = 1;
					}
					// Subdivide the selection area
					// We know it's a square, just split it into several areas
					int width = end_x - start_x;
					if (width < threadcount) {
						threadcount = min(1, width);
					}
					
					// CRITICAL FIX: Prevent division by zero
					if (threadcount <= 0) {
						char debug_msg[256];
						sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! threadcount=%d, width=%d - FORCING threadcount to 1\n", threadcount, width);
						OutputDebugStringA(debug_msg);
						threadcount = 1;
					}
					
					// Let's divide!
					int remainder = width;
					int cleared = 0;
					std::vector<SelectionThread*> threads;
					if (width == 0) {
						threads.push_back(newd SelectionThread(editor, Position(start_x, start_y, start_z), Position(start_x, end_y, end_z)));
					} else {
						for (int i = 0; i < threadcount; ++i) {
							int chunksize = width / threadcount;
							// The last threads takes all the remainder
							if (i == threadcount - 1) {
								chunksize = remainder;
							}
							threads.push_back(newd SelectionThread(editor, Position(start_x + cleared, start_y, start_z), Position(start_x + cleared + chunksize, end_y, end_z)));
							cleared += chunksize;
							remainder -= chunksize;
						}
					}
					ASSERT(cleared == width);
					ASSERT(remainder == 0);

					editor.selection.start(); // Start a selection session
					for (std::vector<SelectionThread*>::iterator iter = threads.begin(); iter != threads.end(); ++iter) {
						(*iter)->Execute();
					}
					for (std::vector<SelectionThread*>::iterator iter = threads.begin(); iter != threads.end(); ++iter) {
						editor.selection.join(*iter);
					}
					editor.selection.finish(); // Finish the selection session
					editor.selection.updateSelectionCount();
				}
			} else if (event.ControlDown()) {
				////
			} else {
				// User hasn't moved anything, meaning selection/deselection
				Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
				if (tile) {
					if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
						if (!tile->spawn->isSelected()) {
							editor.selection.start(); // Start a selection session
							editor.selection.add(tile, tile->spawn);
							editor.selection.finish(); // Finish the selection session
							editor.selection.updateSelectionCount();
						}
					} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
						if (!tile->creature->isSelected()) {
							editor.selection.start(); // Start a selection session
							editor.selection.add(tile, tile->creature);
							editor.selection.finish(); // Finish the selection session
							editor.selection.updateSelectionCount();
						}
					} else {
						Item* item = tile->getTopItem();
						if (item && !item->isSelected()) {
							editor.selection.start(); // Start a selection session
							editor.selection.add(tile, item);
							editor.selection.finish(); // Finish the selection session
							editor.selection.updateSelectionCount();
						}
					}
				}
			}
		}
		editor.actionQueue->resetTimer();
		dragging = false;
		boundbox_selection = false;
	} else if (g_gui.GetCurrentBrush()) { // Drawing mode
		Brush* brush = g_gui.GetCurrentBrush();
		if (dragging_draw) {
			if (brush->isSpawn()) {
				int start_map_x = std::min(last_click_map_x, mouse_map_x);
				int start_map_y = std::min(last_click_map_y, mouse_map_y);
				int end_map_x = std::max(last_click_map_x, mouse_map_x);
				int end_map_y = std::max(last_click_map_y, mouse_map_y);

				int map_x = start_map_x + (end_map_x - start_map_x) / 2;
				int map_y = start_map_y + (end_map_y - start_map_y) / 2;

				// CRITICAL FIX: Prevent division by zero in spawn size calculation
				int raw_width = (end_map_x - start_map_x) / 2 + (end_map_y - start_map_y) / 2;
				if (raw_width <= 0) {
					char debug_msg[256];
					sprintf(debug_msg, "DEBUG DRAG: WARNING! Spawn calculation resulted in width %d - FORCING TO 1\n", raw_width);
					OutputDebugStringA(debug_msg);
					raw_width = 1;
				}
				
				int width = min(g_settings.getInteger(Config::MAX_SPAWN_RADIUS), raw_width / 2);
				if (width <= 0) {
					char debug_msg[256];
					sprintf(debug_msg, "DEBUG DRAG: WARNING! Final spawn width %d - FORCING TO 1\n", width);
					OutputDebugStringA(debug_msg);
					width = 1;
				}
				
				int old = g_gui.GetBrushSize();
				g_gui.SetBrushSize(width);
				editor.draw(Position(map_x, map_y, floor), event.AltDown());
				g_gui.SetBrushSize(old);
			} else {
				PositionVector tilestodraw;
				PositionVector tilestoborder;
				if (brush->isWall()) {
					int start_map_x = std::min(last_click_map_x, mouse_map_x);
					int start_map_y = std::min(last_click_map_y, mouse_map_y);
					int end_map_x = std::max(last_click_map_x, mouse_map_x);
					int end_map_y = std::max(last_click_map_y, mouse_map_y);

					for (int y = start_map_y - 1; y <= end_map_y + 1; y++) {
						for (int x = start_map_x - 1; x <= end_map_x + 1; x++) {
							if ((x <= start_map_x + 1 || x >= end_map_x - 1) || (y <= start_map_y + 1 || y >= end_map_y - 1)) {
								tilestoborder.push_back(Position(x, y, floor));
							}
							if (((x == start_map_x || x == end_map_x) || (y == start_map_y || y == end_map_y)) && ((x >= start_map_x && x <= end_map_x) && (y >= start_map_y && y <= end_map_y))) {
								tilestodraw.push_back(Position(x, y, floor));
							}
						}
					}
				} else {
					if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
						if (last_click_map_x > mouse_map_x) {
							int tmp = mouse_map_x;
							mouse_map_x = last_click_map_x;
							last_click_map_x = tmp;
						}
						if (last_click_map_y > mouse_map_y) {
							int tmp = mouse_map_y;
							mouse_map_y = last_click_map_y;
							last_click_map_y = tmp;
						}

						for (int x = last_click_map_x - 1; x <= mouse_map_x + 1; x++) {
							for (int y = last_click_map_y - 1; y <= mouse_map_y + 1; y++) {
								if ((x <= last_click_map_x || x >= mouse_map_x) || (y <= last_click_map_y || y >= mouse_map_y)) {
									tilestoborder.push_back(Position(x, y, floor));
								}
								if ((x >= last_click_map_x && x <= mouse_map_x) && (y >= last_click_map_y && y <= mouse_map_y)) {
									tilestodraw.push_back(Position(x, y, floor));
								}
							}
						}
					} else {
						int start_x, end_x;
						int start_y, end_y;
						int width = std::max(
							std::abs(
								std::max(mouse_map_y, last_click_map_y) - std::min(mouse_map_y, last_click_map_y)
							),
							std::abs(
								std::max(mouse_map_x, last_click_map_x) - std::min(mouse_map_x, last_click_map_x)
							)
						);
						if (mouse_map_x < last_click_map_x) {
							start_x = last_click_map_x - width;
							end_x = last_click_map_x;
						} else {
							start_x = last_click_map_x;
							end_x = last_click_map_x + width;
						}
						if (mouse_map_y < last_click_map_y) {
							start_y = last_click_map_y - width;
							end_y = last_click_map_y;
						} else {
							start_y = last_click_map_y;
							end_y = last_click_map_y + width;
						}

						int center_x = start_x + (end_x - start_x) / 2;
						int center_y = start_y + (end_y - start_y) / 2;
						float radii = width / 2.0f + 0.005f;

						for (int y = start_y - 1; y <= end_y + 1; y++) {
							float dy = center_y - y;
							for (int x = start_x - 1; x <= end_x + 1; x++) {
								float dx = center_x - x;
								// printf("%f;%f\n", dx, dy);
								float distance = sqrt(dx * dx + dy * dy);
								if (distance < radii) {
									tilestodraw.push_back(Position(x, y, floor));
								}
								if (std::abs(distance - radii) < 1.5) {
									tilestoborder.push_back(Position(x, y, floor));
								}
							}
						}
					}
				}
				if (event.ControlDown()) {
					editor.undraw(tilestodraw, tilestoborder, event.AltDown());
				} else {
					editor.draw(tilestodraw, tilestoborder, event.AltDown());
				}
			}
		}
		editor.actionQueue->resetTimer();
		drawing = false;
		dragging_draw = false;
		replace_dragging = false;
		editor.replace_brush = nullptr;
	}
	g_gui.RefreshView();
	g_gui.UpdateMinimap();
}

void MapCanvas::OnMouseCameraClick(wxMouseEvent& event) {
	SetFocus();

	last_mmb_click_x = event.GetX();
	last_mmb_click_y = event.GetY();
	if (event.ControlDown()) {
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(-screensize_x * (1.0 - zoom) * (std::max(cursor_x, 1) / double(screensize_x))), int(-screensize_y * (1.0 - zoom) * (std::max(cursor_y, 1) / double(screensize_y))));
		zoom = 1.0;
		Refresh();
	} else {
		screendragging = true;
	}
}

void MapCanvas::OnMouseCameraRelease(wxMouseEvent& event) {
	SetFocus();
	screendragging = false;
	if (event.ControlDown()) {
		// ...
		// Haven't moved much, it's a click!
	} else if (last_mmb_click_x > event.GetX() - 3 && last_mmb_click_x < event.GetX() + 3 && last_mmb_click_y > event.GetY() - 3 && last_mmb_click_y < event.GetY() + 3) {
		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);
		static_cast<MapWindow*>(GetParent())->ScrollRelative(int(zoom * (2 * cursor_x - screensize_x)), int(zoom * (2 * cursor_y - screensize_y)));
		Refresh();
	}
}

void MapCanvas::OnMousePropertiesClick(wxMouseEvent& event) {
	SetFocus();

	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);
	Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);

	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	EndPasting();

	boundbox_selection = false;
	if (event.ShiftDown()) {
		boundbox_selection = true;

		if (!event.ControlDown()) {
			editor.selection.start(); // Start selection session
			editor.selection.clear(); // Clear out selection
			editor.selection.finish(); // End selection session
			editor.selection.updateSelectionCount();
		}
	} else if (!tile) {
		editor.selection.start(); // Start selection session
		editor.selection.clear(); // Clear out selection
		editor.selection.finish(); // End selection session
		editor.selection.updateSelectionCount();
	} else if (tile->isSelected()) {
		// Do nothing!
	} else {
		editor.selection.start(); // Start a selection session
		editor.selection.clear();
		editor.selection.commit();
		if (tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
			editor.selection.add(tile, tile->spawn);
		} else if (tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
			editor.selection.add(tile, tile->creature);
		} else {
			Item* item = tile->getTopItem();
			if (item) {
				editor.selection.add(tile, item);
			}
		}
		editor.selection.finish(); // Finish the selection session
		editor.selection.updateSelectionCount();
	}

	last_click_x = int(event.GetX() * zoom);
	last_click_y = int(event.GetY() * zoom);

	int start_x, start_y;
	static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);
	last_click_abs_x = last_click_x + start_x;
	last_click_abs_y = last_click_y + start_y;

	last_click_map_x = mouse_map_x;
	last_click_map_y = mouse_map_y;
	g_gui.RefreshView();
}

void MapCanvas::OnMousePropertiesRelease(wxMouseEvent& event) {
	int mouse_map_x, mouse_map_y;
	ScreenToMap(event.GetX(), event.GetY(), &mouse_map_x, &mouse_map_y);

#ifdef __WXDEBUG__
	printf("DEBUG: Right-click release at map position %d,%d,%d\n", mouse_map_x, mouse_map_y, floor);
	Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
	if (tile && tile->ground) {
		printf("DEBUG: Tile has ground at %p\n", tile->ground);
	}
#endif

	if (g_gui.IsDrawingMode()) {
		g_gui.SetSelectionMode();
	}

	if (boundbox_selection) {
		if (mouse_map_x == last_click_map_x && mouse_map_y == last_click_map_y && event.ControlDown()) {
			// Mouse hasn't move, do control+shift thingy!
			Tile* tile = editor.map.getTile(mouse_map_x, mouse_map_y, floor);
			if (tile) {
				editor.selection.start(); // Start a selection session
				if (tile->isSelected()) {
					editor.selection.remove(tile);
				} else {
					editor.selection.add(tile);
				}
				editor.selection.finish(); // Finish the selection session
				editor.selection.updateSelectionCount();
			}
		} else {
			// The cursor has moved, do some boundboxing!
			if (last_click_map_x > mouse_map_x) {
				int tmp = mouse_map_x;
				mouse_map_x = last_click_map_x;
				last_click_map_x = tmp;
			}
			if (last_click_map_y > mouse_map_y) {
				int tmp = mouse_map_y;
				mouse_map_y = last_click_map_y;
				last_click_map_y = tmp;
			}

			editor.selection.start(); // Start a selection session
			switch (g_settings.getInteger(Config::SELECTION_TYPE)) {
				case SELECT_CURRENT_FLOOR: {
					for (int x = last_click_map_x; x <= mouse_map_x; x++) {
						for (int y = last_click_map_y; y <= mouse_map_y; y++) {
							Tile* tile = editor.map.getTile(x, y, floor);
							if (!tile) {
								continue;
							}
							editor.selection.add(tile);
						}
					}
					break;
				}
				case SELECT_ALL_FLOORS: {
					int start_x, start_y, start_z;
					int end_x, end_y, end_z;

					start_x = last_click_map_x;
					start_y = last_click_map_y;
					start_z = MAP_MAX_LAYER;
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor;

					if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
						start_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
						start_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

						end_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
						end_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
					}

					for (int z = start_z; z >= end_z; z--) {
						for (int x = start_x; x <= end_x; x++) {
							for (int y = start_y; y <= end_y; y++) {
								Tile* tile = editor.map.getTile(x, y, z);
								if (!tile) {
									continue;
								}
								editor.selection.add(tile);
							}
						}
						if (z <= GROUND_LAYER && g_settings.getInteger(Config::COMPENSATED_SELECT)) {
							start_x++;
							start_y++;
							end_x++;
							end_y++;
						}
					}
					break;
				}
				case SELECT_VISIBLE_FLOORS: {
					int start_x, start_y, start_z;
					int end_x, end_y, end_z;

					start_x = last_click_map_x;
					start_y = last_click_map_y;
					if (floor <= GROUND_LAYER) {
						start_z = GROUND_LAYER;
					} else {
						start_z = std::min(MAP_MAX_LAYER, floor + 2);
					}
					end_x = mouse_map_x;
					end_y = mouse_map_y;
					end_z = floor;

					if (g_settings.getInteger(Config::COMPENSATED_SELECT)) {
						start_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
						start_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);

						end_x -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
						end_y -= (floor < GROUND_LAYER ? GROUND_LAYER - floor : 0);
					}

					for (int z = start_z; z >= end_z; z--) {
						for (int x = start_x; x <= end_x; x++) {
							for (int y = start_y; y <= end_y; y++) {
								Tile* tile = editor.map.getTile(x, y, z);
								if (!tile) {
									continue;
								}
								editor.selection.add(tile);
							}
						}
						if (z <= GROUND_LAYER && g_settings.getInteger(Config::COMPENSATED_SELECT)) {
							start_x++;
							start_y++;
							end_x++;
							end_y++;
						}
					}
					break;
				}
			}
			editor.selection.finish(); // Finish the selection session
			editor.selection.updateSelectionCount();
		}
	} else if (event.ControlDown()) {
		// Nothing
	}

	popup_menu->Update();
	
#ifdef __WXDEBUG__
	// Debug the state of the tiles before showing popup menu
	int debug_mouse_x, debug_mouse_y;
	ScreenToMap(event.GetX(), event.GetY(), &debug_mouse_x, &debug_mouse_y);
	Tile* debug_tile = editor.map.getTile(debug_mouse_x, debug_mouse_y, floor);
	
	printf("DEBUG: Before popup menu - Tile at %d,%d,%d: %p\n", 
		debug_mouse_x, debug_mouse_y, floor, debug_tile);
	if (debug_tile && debug_tile->ground) {
		printf("DEBUG: Before popup menu - Tile has ground %p (ID:%d)\n", 
			debug_tile->ground, debug_tile->ground->getID());
	}
#endif

	PopupMenu(popup_menu);

#ifdef __WXDEBUG__
	// Debug the state of the tiles after showing popup menu
	debug_tile = editor.map.getTile(debug_mouse_x, debug_mouse_y, floor);
	printf("DEBUG: After popup menu - Tile at %d,%d,%d: %p\n", 
		debug_mouse_x, debug_mouse_y, floor, debug_tile);
	if (debug_tile && debug_tile->ground) {
		printf("DEBUG: After popup menu - Tile has ground %p (ID:%d)\n", 
			debug_tile->ground, debug_tile->ground->getID());
	}
#endif

	editor.actionQueue->resetTimer();
	dragging = false;
	boundbox_selection = false;

	last_cursor_map_x = mouse_map_x;
	last_cursor_map_y = mouse_map_y;
	last_cursor_map_z = floor;

	g_gui.RefreshView();
}

void MapCanvas::OnWheel(wxMouseEvent& event) {
	if (event.ControlDown()) {
		static double diff = 0.0;
		diff += event.GetWheelRotation();
		if (diff <= 1.0 || diff >= 1.0) {
			if (diff < 0.0) {
				g_gui.ChangeFloor(floor - 1);
			} else {
				g_gui.ChangeFloor(floor + 1);
			}
			diff = 0.0;
		}
		UpdatePositionStatus();
	} else if (event.AltDown()) {
		static double diff = 0.0;
		diff += event.GetWheelRotation();
		if (diff <= 1.0 || diff >= 1.0) {
			if (diff < 0.0) {
				g_gui.IncreaseBrushSize();
			} else {
				g_gui.DecreaseBrushSize();
			}
			diff = 0.0;
		}
	} else {
		double diff = -event.GetWheelRotation() * g_settings.getFloat(Config::ZOOM_SPEED) / 640.0;
		double oldzoom = zoom;
		zoom += diff;

		if (zoom < 0.125) {
			diff = 0.125 - oldzoom;
			zoom = 0.125;
		}
		if (zoom > 25.00) {
			diff = 25.00 - oldzoom;
			zoom = 25.0;
		}

		UpdateZoomStatus();

		int screensize_x, screensize_y;
		static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

		// This took a day to figure out!
		int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x))) * GetContentScaleFactor();
		int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y))) * GetContentScaleFactor();

		static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);
	}

	Refresh();
}

void MapCanvas::OnLoseMouse(wxMouseEvent& event) {
	Refresh();
}

void MapCanvas::OnGainMouse(wxMouseEvent& event) {
	if (!event.LeftIsDown()) {
		dragging = false;
		boundbox_selection = false;
		drawing = false;
	}
	if (!event.MiddleIsDown()) {
		screendragging = false;
	}

	Refresh();
}

void MapCanvas::OnKeyDown(wxKeyEvent& event) {
	// char keycode = event.GetKeyCode();
	//  std::cout << "Keycode " << keycode << std::endl;
	switch (event.GetKeyCode()) {
		case WXK_NUMPAD_ADD:
		case WXK_PAGEUP: {
			g_gui.ChangeFloor(floor - 1);
			break;
		}
		case WXK_NUMPAD_SUBTRACT:
		case WXK_PAGEDOWN: {
			g_gui.ChangeFloor(floor + 1);
			break;
		}
		case WXK_NUMPAD_MULTIPLY: {
			double diff = -0.3;

			double oldzoom = zoom;
			zoom += diff;

			if (zoom < 0.125) {
				diff = 0.125 - oldzoom;
				zoom = 0.125;
			}

			int screensize_x, screensize_y;
			static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y)));

			static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		case WXK_NUMPAD_DIVIDE: {
			double diff = 0.3;
			double oldzoom = zoom;
			zoom += diff;

			if (zoom > 25.00) {
				diff = 25.00 - oldzoom;
				zoom = 25.0;
			}

			int screensize_x, screensize_y;
			static_cast<MapWindow*>(GetParent())->GetViewSize(&screensize_x, &screensize_y);

			// This took a day to figure out!
			int scroll_x = int(screensize_x * diff * (std::max(cursor_x, 1) / double(screensize_x)));
			int scroll_y = int(screensize_y * diff * (std::max(cursor_y, 1) / double(screensize_y)));

			static_cast<MapWindow*>(GetParent())->ScrollRelative(-scroll_x, -scroll_y);

			UpdatePositionStatus();
			UpdateZoomStatus();
			Refresh();
			break;
		}
		// This will work like crap with non-us layouts, well, sucks for them until there is another solution.
		case '[':
		case '+': {
			g_gui.IncreaseBrushSize();
			Refresh();
			break;
		}
		case ']':
		case '-': {
			g_gui.DecreaseBrushSize();
			Refresh();
			break;
		}
		case WXK_NUMPAD_UP:
		case WXK_UP: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(start_x, int(start_y - TileSize * tiles * zoom));
			UpdatePositionStatus();
			g_gui.UpdateMinimap(true);  // Add immediate update
			Refresh();
			break;
		}
		case WXK_NUMPAD_DOWN:
		case WXK_DOWN: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(start_x, int(start_y + TileSize * tiles * zoom));
			UpdatePositionStatus();
			g_gui.UpdateMinimap(true);  // Add immediate update
			Refresh();
			break;
		}
		case WXK_NUMPAD_LEFT:
		case WXK_LEFT: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(int(start_x - TileSize * tiles * zoom), start_y);
			UpdatePositionStatus();
			g_gui.UpdateMinimap(true);  // Add immediate update
			Refresh();
			break;
		}
		case WXK_NUMPAD_RIGHT:
		case WXK_RIGHT: {
			int start_x, start_y;
			static_cast<MapWindow*>(GetParent())->GetViewStart(&start_x, &start_y);

			int tiles = 3;
			if (event.ControlDown()) {
				tiles = 10;
			} else if (zoom == 1.0) {
				tiles = 1;
			}

			static_cast<MapWindow*>(GetParent())->Scroll(int(start_x + TileSize * tiles * zoom), start_y);
			UpdatePositionStatus();
			g_gui.UpdateMinimap(true);  // Add immediate update
			Refresh();
			break;
		}
		case WXK_SPACE: { // Utility keys
			if (event.ControlDown()) {
				g_gui.FillDoodadPreviewBuffer();
				g_gui.RefreshView();
			} else {
				g_gui.SwitchMode();
			}
			break;
		}
		case WXK_TAB: { // Tab switch
			if (event.ShiftDown()) {
				g_gui.CycleTab(false);
			} else {
				g_gui.CycleTab(true);
			}
			break;
		}
		case WXK_F5: { // Refresh visible area (for multiplayer)
			LiveClient* client = dynamic_cast<LiveClient*>(editor.GetLiveClient());
			if (client) {
				client->requestVisibleRefresh();
				g_gui.SetStatusText("Refreshing visible area...");
			}
			break;
		}
		case WXK_DELETE: { // Delete
			editor.destroySelection();
			g_gui.RefreshView();
			break;
		}
		case 'z':
		case 'Z': { // Rotate counterclockwise (actually shift variaton, but whatever... :P)
			int nv = g_gui.GetBrushVariation();
			--nv;
			if (nv < 0) {
				nv = max(0, (g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getMaxVariation() - 1 : 0));
			}
			g_gui.SetBrushVariation(nv);
			g_gui.RefreshView();
			break;
		}
		case 'x':
		case 'X': { // Rotate clockwise (actually shift variaton, but whatever... :P)
			int nv = g_gui.GetBrushVariation();
			++nv;
			if (nv >= (g_gui.GetCurrentBrush() ? g_gui.GetCurrentBrush()->getMaxVariation() : 0)) {
				nv = 0;
			}
			g_gui.SetBrushVariation(nv);
			g_gui.RefreshView();
			break;
		}
		case 'q':
		case 'Q': { // Select previous brush
			g_gui.SelectPreviousBrush();
			break;
		}
		// Hotkeys
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			int index = event.GetKeyCode() - '0';
			if (event.ControlDown()) {
				Hotkey hk;
				if (g_gui.IsSelectionMode()) {
					int view_start_x, view_start_y;
					static_cast<MapWindow*>(GetParent())->GetViewStart(&view_start_x, &view_start_y);
					int view_start_map_x = view_start_x / TileSize, view_start_map_y = view_start_y / TileSize;

					int view_screensize_x, view_screensize_y;
					static_cast<MapWindow*>(GetParent())->GetViewSize(&view_screensize_x, &view_screensize_y);

					int map_x = int(view_start_map_x + (view_screensize_x * zoom) / TileSize / 2);
					int map_y = int(view_start_map_y + (view_screensize_y * zoom) / TileSize / 2);

					hk = Hotkey(Position(map_x, map_y, floor));
				} else if (g_gui.GetCurrentBrush()) {
					// Drawing mode
					hk = Hotkey(g_gui.GetCurrentBrush());
				} else {
					break;
				}
				g_gui.SetHotkey(index, hk);
			} else {
				// Click hotkey
				Hotkey hk = g_gui.GetHotkey(index);
				if (hk.IsPosition()) {
					g_gui.SetSelectionMode();

					int map_x = hk.GetPosition().x;
					int map_y = hk.GetPosition().y;
					int map_z = hk.GetPosition().z;

					static_cast<MapWindow*>(GetParent())->Scroll(TileSize * map_x, TileSize * map_y, true);
					floor = map_z;

					g_gui.SetStatusText("Used hotkey " + i2ws(index));
					g_gui.RefreshView();
				} else if (hk.IsBrush()) {
					g_gui.SetDrawingMode();

					std::string name = hk.GetBrushname();
					Brush* brush = g_brushes.getBrush(name);
					if (brush == nullptr) {
						g_gui.SetStatusText("Brush \"" + wxstr(name) + "\" not found");
						return;
					}

					if (!g_gui.SelectBrush(brush)) {
						g_gui.SetStatusText("Brush \"" + wxstr(name) + "\" is not in any palette");
						return;
					}

					g_gui.SetStatusText("Used hotkey " + i2ws(index));
					g_gui.RefreshView();
				} else {
					g_gui.SetStatusText("Unassigned hotkey " + i2ws(index));
				}
			}
			break;
		}
		case 'd':
		case 'D': {
			keyCode = WXK_CONTROL_D;
			break;
		}
		case 'a':
		case 'A': {
			bool new_state = !g_settings.getBoolean(Config::USE_AUTOMAGIC);
			g_settings.setInteger(Config::USE_AUTOMAGIC, new_state ? 1 : 0);
			if (new_state) {
				g_gui.SetStatusText("Automagic enabled.");
			} else {
				g_gui.SetStatusText("Automagic disabled.");
			}
			break;
		}
		default: {
			event.Skip();
			break;
		}
	}
}

void MapCanvas::OnKeyUp(wxKeyEvent& event) {
	// ESC key - cancel custom brush size
	if (event.GetKeyCode() == WXK_ESCAPE && g_gui.UseCustomBrushSize()) {
		g_gui.SetCustomBrushSize(false);
		g_gui.SetStatusText("Restored standard brush size");
		Refresh();
		return;
	}
	
	// If Shift key is released while dragging with brush, set custom brush size (square brushes only)
	if (event.GetKeyCode() == WXK_SHIFT && dragging_draw && g_gui.GetCurrentBrush() && g_gui.GetCurrentBrush()->canDrag()) {
		int current_map_x, current_map_y;
		MouseToMap(&current_map_x, &current_map_y);
		
		// Only allow custom brush size for square brushes
		if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
			int width = std::abs(current_map_x - last_click_map_x) + 1;  // +1 to include the tile itself
			int height = std::abs(current_map_y - last_click_map_y) + 1; // +1 to include the tile itself
			
			// CRITICAL FIX: Prevent zero-sized brushes after movement
			if (width <= 0) {
				char debug_msg[256];
				sprintf(debug_msg, "DEBUG DRAG: WARNING! Movement resulted in width %d - FORCING TO 1\n", width);
				OutputDebugStringA(debug_msg);
				width = 1;
			}
			
			if (height <= 0) {
				char debug_msg[256];
				sprintf(debug_msg, "DEBUG DRAG: WARNING! Movement resulted in height %d - FORCING TO 1\n", height);
				OutputDebugStringA(debug_msg);
				height = 1;
			}
			
			if (width > 0 && height > 0) {
				// Enable custom brush dimensions with specific width and height
				// No need to adjust for center pixel - accept any dimensions
				g_gui.SetCustomBrushSize(true, width, height);
				
				// End drag drawing mode but keep drawing mode active
				dragging_draw = false;
				
				// Update status bar
				wxString ss;
				ss << "Custom brush size set to " << width << "x" << height;
				g_gui.SetStatusText(ss);
			}
			Refresh();
		} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
			// Circle brushes use traditional radius-based sizing - no custom sizes allowed
			// End drag drawing mode
			dragging_draw = false;
			g_gui.SetStatusText("Circle brushes use traditional radius sizing");
			Refresh();
		}
	}
	
	keyCode = WXK_NONE;
}

void MapCanvas::OnCopy(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.IsSelectionMode()) {
		editor.copybuffer.copy(editor, GetFloor());
	}
}

void MapCanvas::OnCut(wxCommandEvent& WXUNUSED(event)) {
	if (g_gui.IsSelectionMode()) {
		editor.copybuffer.cut(editor, GetFloor());
	}
	g_gui.RefreshView();
}

void MapCanvas::OnPaste(wxCommandEvent& WXUNUSED(event)) {
	g_gui.DoPaste();
	g_gui.RefreshView();
}

void MapCanvas::OnDelete(wxCommandEvent& WXUNUSED(event)) {
	editor.destroySelection();
	g_gui.RefreshView();
}

void MapCanvas::OnCopyPosition(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() == 0) {
		return;
	}

	Position minPos = editor.selection.minPosition();
	Position maxPos = editor.selection.maxPosition();

	std::ostringstream clip;
	if (minPos != maxPos) {
		clip << "{";
		clip << "fromx = " << minPos.x << ", ";
		clip << "tox = " << maxPos.x << ", ";
		clip << "fromy = " << minPos.y << ", ";
		clip << "toy = " << maxPos.y << ", ";
		if (minPos.z != maxPos.z) {
			clip << "fromz = " << minPos.z << ", ";
			clip << "toz = " << maxPos.z;
		} else {
			clip << "z = " << minPos.z;
		}
		clip << "}";
	} else {
		switch (g_settings.getInteger(Config::COPY_POSITION_FORMAT)) {
			case 0:
				clip << "{x = " << minPos.x << ", y = " << minPos.y << ", z = " << minPos.z << "}";
				break;
			case 1:
				clip << "{\"x\":" << minPos.x << ",\"y\":" << minPos.y << ",\"z\":" << minPos.z << "}";
				break;
			case 2:
				clip << minPos.x << ", " << minPos.y << ", " << minPos.z;
				break;
			case 3:
				clip << "(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")";
				break;
			case 4:
				clip << "Position(" << minPos.x << ", " << minPos.y << ", " << minPos.z << ")";
				break;
		}
	}

	if (wxTheClipboard->Open()) {
		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(wxstr(clip.str()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyServerId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyClientId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getClientID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyName(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(wxstr(item->getName()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyActionId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getActionID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnCopyUniqueId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		Tile* tile = editor.selection.getSelectedTile();
		ItemVector selected_items = tile->getSelectedItems();
		ASSERT(selected_items.size() == 1);

		const Item* item = selected_items.front();

		wxTextDataObject* obj = new wxTextDataObject();
		obj->SetText(i2ws(item->getUniqueID()));
		wxTheClipboard->SetData(obj);

		wxTheClipboard->Close();
	}
}

void MapCanvas::OnPasteActionId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
			wxTextDataObject data;
			wxTheClipboard->GetData(data);
			
			long actionId;
			if (data.GetText().ToLong(&actionId) && actionId >= 0 && actionId <= 0xFFFF) {
				Tile* tile = editor.selection.getSelectedTile();
				if (!tile) {
					wxTheClipboard->Close();
					return;
				}
				
				Tile* new_tile = tile->deepCopy(editor.map);
				ItemVector selected_items = new_tile->getSelectedItems();
				
				if (selected_items.size() == 1) {
					Item* item = selected_items.front();
					item->setActionID(static_cast<uint16_t>(actionId));
					
					Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
					action->addChange(newd Change(new_tile));
					editor.addAction(action);
					g_gui.RefreshView();
				} else {
					delete new_tile;
				}
			} else {
				g_gui.PopupDialog(g_gui.root, "Error", "Invalid Action ID value in clipboard. Must be a number between 0 and 65535.", wxOK);
			}
		}
		wxTheClipboard->Close();
	}
}

void MapCanvas::OnPasteUniqueId(wxCommandEvent& WXUNUSED(event)) {
	ASSERT(editor.selection.size() == 1);

	if (wxTheClipboard->Open()) {
		if (wxTheClipboard->IsSupported(wxDF_TEXT)) {
			wxTextDataObject data;
			wxTheClipboard->GetData(data);
			
			long uniqueId;
			if (data.GetText().ToLong(&uniqueId) && uniqueId >= 0 && uniqueId <= 0xFFFF) {
				// Validate unique ID range (typically 1000-65535 for non-zero values)
				if (uniqueId != 0 && (uniqueId < 1000 || uniqueId > 0xFFFF)) {
					g_gui.PopupDialog(g_gui.root, "Error", "Unique ID must be 0 or between 1000 and 65535.", wxOK);
					wxTheClipboard->Close();
					return;
				}
				
				Tile* tile = editor.selection.getSelectedTile();
				if (!tile) {
					wxTheClipboard->Close();
					return;
				}
				
				Tile* new_tile = tile->deepCopy(editor.map);
				ItemVector selected_items = new_tile->getSelectedItems();
				
				if (selected_items.size() == 1) {
					Item* item = selected_items.front();
					item->setUniqueID(static_cast<uint16_t>(uniqueId));
					
					Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
					action->addChange(newd Change(new_tile));
					editor.addAction(action);
					g_gui.RefreshView();
				} else {
					delete new_tile;
				}
			} else {
				g_gui.PopupDialog(g_gui.root, "Error", "Invalid Unique ID value in clipboard. Must be a number between 0 and 65535.", wxOK);
			}
		}
		wxTheClipboard->Close();
	}
}

void MapCanvas::OnBrowseTile(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	
#ifdef __WXDEBUG__
	if (tile->ground) {
		printf("DEBUG: Original tile %p has ground %p before deepCopy\n", tile, tile->ground);
	}
#endif

	Tile* new_tile = tile->deepCopy(editor.map);

#ifdef __WXDEBUG__
	if (new_tile->ground) {
		printf("DEBUG: New tile %p has ground %p after deepCopy\n", new_tile, new_tile->ground);
	}
#endif

	wxDialog* w = new BrowseTileWindow(g_gui.root, new_tile, wxPoint(cursor_x, cursor_y));

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_DELETE_TILES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);
	} else {
		// Cancel
		delete new_tile;
	}

	w->Destroy();
}

void MapCanvas::OnRotateItem(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();

	Action* action = editor.actionQueue->createAction(ACTION_ROTATE_ITEM);

	Tile* new_tile = tile->deepCopy(editor.map);

	ItemVector selected_items = new_tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);

	selected_items.front()->doRotate();

	action->addChange(newd Change(new_tile));

	editor.actionQueue->addAction(action);
	g_gui.RefreshView();
}

void MapCanvas::OnGotoDestination(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	ItemVector selected_items = tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);
	Teleport* teleport = dynamic_cast<Teleport*>(selected_items.front());
	if (teleport) {
		Position pos = teleport->getDestination();
		g_gui.SetScreenCenterPosition(pos);
	}
}

void MapCanvas::OnSwitchDoor(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();

	Action* action = editor.actionQueue->createAction(ACTION_SWITCHDOOR);

	Tile* new_tile = tile->deepCopy(editor.map);

	ItemVector selected_items = new_tile->getSelectedItems();
	ASSERT(selected_items.size() > 0);

	DoorBrush::switchDoor(selected_items.front());

	action->addChange(newd Change(new_tile));

	editor.actionQueue->addAction(action);
	g_gui.RefreshView();
}

void MapCanvas::OnSelectRAWBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item && item->getRAWBrush()) {
		g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
	}
}

void MapCanvas::OnSelectGroundBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	GroundBrush* bb = tile->getGroundBrush();

	if (bb) {
		g_gui.SelectBrush(bb, TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectDoodadBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item) {
		g_gui.SelectBrush(item->getDoodadBrush(), TILESET_DOODAD);
	}
}

void MapCanvas::OnSelectDoorBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* item = tile->getTopSelectedItem();

	if (item) {
		g_gui.SelectBrush(item->getDoorBrush(), TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectWallBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getWall();
	WallBrush* wb = wall->getWallBrush();

	if (wb) {
		g_gui.SelectBrush(wb, TILESET_TERRAIN);
	}
}

void MapCanvas::OnSelectCarpetBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getCarpet();
	CarpetBrush* cb = wall->getCarpetBrush();

	if (cb) {
		g_gui.SelectBrush(cb);
	}
}

void MapCanvas::OnSelectTableBrush(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	Item* wall = tile->getTable();
	TableBrush* tb = wall->getTableBrush();

	if (tb) {
		g_gui.SelectBrush(tb);
	}
}

void MapCanvas::OnSelectHouseBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	if (tile->isHouseTile()) {
		House* house = editor.map.houses.getHouse(tile->getHouseID());
		if (house) {
			g_gui.house_brush->setHouse(house);
			g_gui.SelectBrush(g_gui.house_brush, TILESET_HOUSE);
		}
	}
}

void MapCanvas::OnSelectCollectionBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	for (auto* item : tile->items) {
		if (item->isWall()) {
			WallBrush* wb = item->getWallBrush();
			if (wb && wb->visibleInPalette() && wb->hasCollection()) {
				g_gui.SelectBrush(wb, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isTable()) {
			TableBrush* tb = item->getTableBrush();
			if (tb && tb->visibleInPalette() && tb->hasCollection()) {
				g_gui.SelectBrush(tb, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isCarpet()) {
			CarpetBrush* cb = item->getCarpetBrush();
			if (cb && cb->visibleInPalette() && cb->hasCollection()) {
				g_gui.SelectBrush(cb, TILESET_COLLECTION);
				return;
			}
		}
		if (Brush* db = item->getDoodadBrush()) {
			if (db && db->visibleInPalette() && db->hasCollection()) {
				g_gui.SelectBrush(db, TILESET_COLLECTION);
				return;
			}
		}
		if (item->isSelected()) {
			RAWBrush* rb = item->getRAWBrush();
			if (rb && rb->hasCollection()) {
				g_gui.SelectBrush(rb, TILESET_COLLECTION);
				return;
			}
		}
	}
	GroundBrush* gb = tile->getGroundBrush();
	if (gb && gb->visibleInPalette() && gb->hasCollection()) {
		g_gui.SelectBrush(gb, TILESET_COLLECTION);
		return;
	}
}

void MapCanvas::OnSelectCreatureBrush(wxCommandEvent& WXUNUSED(event)) {
	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	if (tile->creature) {
		g_gui.SelectBrush(tile->creature->getBrush(), TILESET_CREATURE);
	}
}

void MapCanvas::OnSelectSpawnBrush(wxCommandEvent& WXUNUSED(event)) {
	g_gui.SelectBrush(g_gui.spawn_brush, TILESET_CREATURE);
}

void MapCanvas::OnSelectMoveTo(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor.map);

	wxDialog* w = nullptr;

	ItemVector selected_items = new_tile->getSelectedItems();

	Item* item = nullptr;
	int count = 0;
	for (ItemVector::iterator it = selected_items.begin(); it != selected_items.end(); ++it) {
		++count;
		if ((*it)->isSelected()) {
			item = *it;
		}
	}

	if (item) {
		w = newd TilesetWindow(g_gui.root, &editor.map, new_tile, item);
	} else {
		return;
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);

		g_gui.RebuildPalettes();
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
}

void MapCanvas::OnProperties(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}
	ASSERT(tile->isSelected());
	Tile* new_tile = tile->deepCopy(editor.map);

	wxDialog* w = nullptr;

	if (new_tile->spawn && g_settings.getInteger(Config::SHOW_SPAWNS)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->spawn);
	} else if (new_tile->creature && g_settings.getInteger(Config::SHOW_CREATURES)) {
		w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, new_tile->creature);
	} else {
		ItemVector selected_items = new_tile->getSelectedItems();

		Item* item = nullptr;
		int count = 0;
		for (ItemVector::iterator it = selected_items.begin(); it != selected_items.end(); ++it) {
			++count;
			if ((*it)->isSelected()) {
				item = *it;
			}
		}

		if (item) {
			if (editor.map.getVersion().otbm >= MAP_OTBM_4) {
				w = newd PropertiesWindow(g_gui.root, &editor.map, new_tile, item);
			} else {
				w = newd OldPropertiesWindow(g_gui.root, &editor.map, new_tile, item);
			}
		} else {
			return;
		}
	}

	int ret = w->ShowModal();
	if (ret != 0) {
		Action* action = editor.actionQueue->createAction(ACTION_CHANGE_PROPERTIES);
		action->addChange(newd Change(new_tile));
		editor.addAction(action);
	} else {
		// Cancel!
		delete new_tile;
	}
	w->Destroy();
}

void MapCanvas::ChangeFloor(int new_floor) {
	ASSERT(new_floor >= 0 || new_floor < MAP_LAYERS);
	int old_floor = floor;
	floor = new_floor;
	
	// Check if crossing between underground and ground level (but no longer clearing cache)
	bool crossing_ground_level = (old_floor > GROUND_LAYER && new_floor <= GROUND_LAYER) || 
							   (old_floor <= GROUND_LAYER && new_floor > GROUND_LAYER);
	
	if (old_floor != new_floor) {
		UpdatePositionStatus();
		g_gui.root->UpdateFloorMenu();
		
		// No longer clearing cache or updating minimap on floor changes
		// This allows the minimap to maintain its cache across floor changes
		// for better performance
	}
	Refresh();
}

void MapCanvas::EnterDrawingMode() {
	dragging = false;
	boundbox_selection = false;
	EndPasting();
	Refresh();
}

void MapCanvas::EnterSelectionMode() {
	drawing = false;
	dragging_draw = false;
	replace_dragging = false;
	editor.replace_brush = nullptr;
	Refresh();
}

bool MapCanvas::isPasting() const {
	return g_gui.IsPasting();
}

void MapCanvas::StartPasting() {
	g_gui.StartPasting();
}

void MapCanvas::EndPasting() {
	g_gui.EndPasting();
}

void MapCanvas::Reset() {
	cursor_x = 0;
	cursor_y = 0;

	zoom = 1.0;
	floor = GROUND_LAYER;

	dragging = false;
	boundbox_selection = false;
	screendragging = false;
	drawing = false;
	dragging_draw = false;

	replace_dragging = false;
	editor.replace_brush = nullptr;

	drag_start_x = -1;
	drag_start_y = -1;
	drag_start_z = -1;

	last_click_map_x = -1;
	last_click_map_y = -1;
	last_click_map_z = -1;

	last_mmb_click_x = -1;
	last_mmb_click_y = -1;

	editor.selection.clear();
	editor.actionQueue->clear();
}

MapPopupMenu::MapPopupMenu(Editor& editor) :
	wxMenu(""), editor(editor) {
	////
}

MapPopupMenu::~MapPopupMenu() {
	////
}

void MapPopupMenu::Update() {
	// Clear the menu of all items
	while (GetMenuItemCount() != 0) {
		wxMenuItem* m_item = FindItemByPosition(0);
		// If you add a submenu, this won't delete it.
		Delete(m_item);
	}

	bool anything_selected = editor.selection.size() != 0;

	wxMenuItem* cutItem = Append(MAP_POPUP_MENU_CUT, "&Cut\tCTRL+X", "Cut out all selected items");
	cutItem->Enable(anything_selected);

	wxMenuItem* copyItem = Append(MAP_POPUP_MENU_COPY, "&Copy\tCTRL+C", "Copy all selected items");
	copyItem->Enable(anything_selected);

	wxMenuItem* copyPositionItem = Append(MAP_POPUP_MENU_COPY_POSITION, "&Copy Position", "Copy the position as a lua table");
	copyPositionItem->Enable(anything_selected);

	wxMenuItem* pasteItem = Append(MAP_POPUP_MENU_PASTE, "&Paste\tCTRL+V", "Paste items in the copybuffer here");
	pasteItem->Enable(editor.copybuffer.canPaste());

	wxMenuItem* deleteItem = Append(MAP_POPUP_MENU_DELETE, "&Delete\tDEL", "Removes all seleceted items");
	deleteItem->Enable(anything_selected);

	// Add the Fill option
	wxMenuItem* fillItem = Append(MAP_POPUP_MENU_FILL, "&Fill Area", "Fill enclosed area with current brush");
	fillItem->Enable(g_gui.GetCurrentBrush() != nullptr);

	// Add the Generate Island option
	Append(MAP_POPUP_MENU_GENERATE_ISLAND, "Generate &Island", "Generate an island at this location");

	// Add the Create House option
	Append(MAP_POPUP_MENU_CREATE_HOUSE, "Create &House", "Auto-detect house boundaries based on walls and doors");

	// Add the Find Similar option
	wxMenuItem* findSimilarItem = Append(MAP_POPUP_MENU_FIND_SIMILAR_ITEMS, "Find &Similar Items", "Find similar items on the map");
	findSimilarItem->Enable(anything_selected);

	wxMenuItem* selectionToDoodadItem = Append(MAP_POPUP_MENU_SELECTION_TO_DOODAD, "&Selection to Doodad", "Create a doodad brush from the selected items");
	selectionToDoodadItem->Enable(anything_selected);

	if (anything_selected) {
		if (editor.selection.size() == 1) {
			Tile* tile = editor.selection.getSelectedTile();
			ItemVector selected_items = tile->getSelectedItems();

			bool hasWall = false;
			bool hasCarpet = false;
			bool hasTable = false;
			bool hasCollection = false;
			Item* topItem = nullptr;
			//this is where we have info about whats selected
			Item* topSelectedItem = (selected_items.size() == 1 ? selected_items.back() : nullptr);
			// With a valid single selected item, trigger auto-select RAW if enabled.
			if (g_settings.getBoolean(Config::AUTO_SELECT_RAW_ON_RIGHTCLICK) &&
				topSelectedItem && topSelectedItem->getRAWBrush()) {
				// Call the existing handler to change the current brush to RAW.
				// This reuses the logic in OnSelectRAWBrush and maintains consistent behavior.
				if (editor.selection.size() == 1) {
					Tile* tile = editor.selection.getSelectedTile();
					if (tile) {
						Item* item = tile->getTopSelectedItem();
						if (item && item->getRAWBrush()) {
							g_gui.SelectBrush(item->getRAWBrush(), TILESET_RAW);
						}
					}
				}
			}

			Creature* topCreature = tile->creature;
			Spawn* topSpawn = tile->spawn;

			for (auto* item : tile->items) {
				if (item->isWall()) {
					Brush* wb = item->getWallBrush();
					if (wb && wb->visibleInPalette()) {
						hasWall = true;
						hasCollection = hasCollection || wb->hasCollection();
					}
				}
				if (item->isTable()) {
					Brush* tb = item->getTableBrush();
					if (tb && tb->visibleInPalette()) {
						hasTable = true;
						hasCollection = hasCollection || tb->hasCollection();
					}
				}
				if (item->isCarpet()) {
					Brush* cb = item->getCarpetBrush();
					if (cb && cb->visibleInPalette()) {
						hasCarpet = true;
						hasCollection = hasCollection || cb->hasCollection();
					}
				}
				if (Brush* db = item->getDoodadBrush()) {
					hasCollection = hasCollection || db->hasCollection();
				}
				if (item->isSelected()) {
					topItem = item;
				}
			}
			if (!topItem) {
				topItem = tile->ground;
			}

			AppendSeparator();

			if (topSelectedItem) {
				Append(MAP_POPUP_MENU_COPY_SERVER_ID, "Copy Item Server Id", "Copy the server id of this item");
				Append(MAP_POPUP_MENU_COPY_CLIENT_ID, "Copy Item Client Id", "Copy the client id of this item");
				Append(MAP_POPUP_MENU_COPY_NAME, "Copy Item Name", "Copy the name of this item");
				
				// Add copy/paste for ActionID and UniqueID
				wxString actionIdLabel = wxString::Format("Copy action ID (%d)", topSelectedItem->getActionID());
				wxString uniqueIdLabel = wxString::Format("Copy unique ID (%d)", topSelectedItem->getUniqueID());
				
				Append(MAP_POPUP_MENU_COPY_ACTION_ID, actionIdLabel, "Copy the action ID of this item");
				Append(MAP_POPUP_MENU_COPY_UNIQUE_ID, uniqueIdLabel, "Copy the unique ID of this item");
				Append(MAP_POPUP_MENU_PASTE_ACTION_ID, "Paste action ID", "Paste action ID from clipboard to this item");
				Append(MAP_POPUP_MENU_PASTE_UNIQUE_ID, "Paste unique ID", "Paste unique ID from clipboard to this item");
				
				AppendSeparator();
			}

			if (topSelectedItem || topCreature || topItem) {
				Teleport* teleport = dynamic_cast<Teleport*>(topSelectedItem);
				if (topSelectedItem && (topSelectedItem->isBrushDoor() || topSelectedItem->isRoteable() || teleport)) {

					if (topSelectedItem->isRoteable()) {
						Append(MAP_POPUP_MENU_ROTATE, "&Rotate item", "Rotate this item");
					}

					if (teleport && teleport->hasDestination()) {
						Append(MAP_POPUP_MENU_GOTO, "&Go To Destination", "Go to the destination of this teleport");
					}

					if (topSelectedItem->isDoor()) {
						if (topSelectedItem->isOpen()) {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Close door", "Close this door");
						} else {
							Append(MAP_POPUP_MENU_SWITCH_DOOR, "&Open door", "Open this door");
						}
						AppendSeparator();
					}
				}

				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
					
					// Add creature-specific menu items
					if (topCreature->isNpc()) {
						AppendSeparator();
						Append(MAP_POPUP_MENU_OPEN_NPC_XML, "Open NPC &XML", "Open the NPC XML file in external editor");
						Append(MAP_POPUP_MENU_OPEN_NPC_SCRIPT, "Open NPC &Script", "Open the NPC Lua script file in external editor");
					} else {
						// This is a monster
						AppendSeparator();
						Append(MAP_POPUP_MENU_OPEN_MONSTER_XML, "Open Monster &XML", "Open the Monster XML file in external editor");
					}
				}

				if (topSpawn) {
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
				}

				Append(MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");

				if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR)) {
					Append(MAP_POPUP_MENU_MOVE_TO_TILESET, "Move To Tileset", "Move this item to any tileset");
				}

				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}

				if (hasCarpet) {
					Append(MAP_POPUP_MENU_SELECT_CARPET_BRUSH, "Select Carpetbrush", "Uses the current item as a carpetbrush");
				}

				if (hasTable) {
					Append(MAP_POPUP_MENU_SELECT_TABLE_BRUSH, "Select Tablebrush", "Uses the current item as a tablebrush");
				}

				if (topSelectedItem && topSelectedItem->getDoodadBrush() && topSelectedItem->getDoodadBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_DOODAD_BRUSH, "Select Doodadbrush", "Use this doodad brush");
				}

				if (topSelectedItem && topSelectedItem->isBrushDoor() && topSelectedItem->getDoorBrush()) {
					Append(MAP_POPUP_MENU_SELECT_DOOR_BRUSH, "Select Doorbrush", "Use this door brush");
				}

				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current item as a groundbrush");
				}

				if (hasCollection || topSelectedItem && topSelectedItem->hasCollectionBrush() || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				if (tile->isHouseTile()) {
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");
				}

				AppendSeparator();

				Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
				
				// Add RevScript menu item if the selected item has scripts available
				if (topSelectedItem) {
					int actionId = topSelectedItem->getActionID();
					int uniqueId = topSelectedItem->getUniqueID();
					int itemId = topSelectedItem->getID();
					
					bool hasActionScript = actionId != 0 && g_gui.revscript_manager.hasScriptForActionID(actionId);
					bool hasUniqueScript = uniqueId != 0 && g_gui.revscript_manager.hasScriptForUniqueID(uniqueId);
					bool hasItemScript = g_gui.revscript_manager.hasScriptForItemID(itemId);
					
					if (actionId != 0 || uniqueId != 0 || hasItemScript) {
						wxString revscriptLabel = "Open &RevScript";
						wxString tooltip = "Open the script file for this item";
						
						// Build detailed label showing what scripts are available
						wxArrayString scriptTypes;
						if (hasActionScript) {
							scriptTypes.Add(wxString::Format("AID:%d", actionId));
						}
						if (hasUniqueScript) {
							scriptTypes.Add(wxString::Format("UID:%d", uniqueId));
						}
						if (hasItemScript) {
							scriptTypes.Add(wxString::Format("ID:%d", itemId));
						}
						
						// If scripts exist, show which ones. If not, show what we're looking for
						if (hasActionScript || hasUniqueScript || hasItemScript) {
							revscriptLabel = wxString::Format("Open &RevScript (%s)", wxJoin(scriptTypes, ','));
							tooltip = "Open the script file(s) for this item";
						} else {
							// No scripts found, but show what we could look for
							wxArrayString potentialTypes;
							if (actionId != 0) potentialTypes.Add(wxString::Format("AID:%d", actionId));
							if (uniqueId != 0) potentialTypes.Add(wxString::Format("UID:%d", uniqueId));
							potentialTypes.Add(wxString::Format("ID:%d", itemId));
							
							revscriptLabel = wxString::Format("Look for &RevScript (%s)", wxJoin(potentialTypes, ','));
							tooltip = "Search for script files for this item (no scripts currently found)";
						}
						
						Append(MAP_POPUP_MENU_OPEN_REVSCRIPT, revscriptLabel, tooltip);
					}
				}
			} else {

				if (topCreature) {
					Append(MAP_POPUP_MENU_SELECT_CREATURE_BRUSH, "Select Creature", "Uses the current creature as a creature brush");
					
					// Add creature-specific menu items
					if (topCreature->isNpc()) {
						AppendSeparator();
						Append(MAP_POPUP_MENU_OPEN_NPC_XML, "Open NPC &XML", "Open the NPC XML file in external editor");
						Append(MAP_POPUP_MENU_OPEN_NPC_SCRIPT, "Open NPC &Script", "Open the NPC Lua script file in external editor");
					} else {
						// This is a monster
						AppendSeparator();
						Append(MAP_POPUP_MENU_OPEN_MONSTER_XML, "Open Monster &XML", "Open the Monster XML file in external editor");
					}
				}

				if (topSpawn) {
					Append(MAP_POPUP_MENU_SELECT_SPAWN_BRUSH, "Select Spawn", "Select the spawn brush");
				}

				Append(MAP_POPUP_MENU_SELECT_RAW_BRUSH, "Select RAW", "Uses the top item as a RAW brush");
				if (hasWall) {
					Append(MAP_POPUP_MENU_SELECT_WALL_BRUSH, "Select Wallbrush", "Uses the current item as a wallbrush");
				}
				if (tile->hasGround() && tile->getGroundBrush() && tile->getGroundBrush()->visibleInPalette()) {
					Append(MAP_POPUP_MENU_SELECT_GROUND_BRUSH, "Select Groundbrush", "Uses the current tile as a groundbrush");
				}

				if (hasCollection || tile->getGroundBrush() && tile->getGroundBrush()->hasCollection()) {
					Append(MAP_POPUP_MENU_SELECT_COLLECTION_BRUSH, "Select Collection", "Use this collection");
				}

				if (tile->isHouseTile()) {
					Append(MAP_POPUP_MENU_SELECT_HOUSE_BRUSH, "Select House", "Draw with the house on this tile.");
				}

				if (tile->hasGround() || topCreature || topSpawn) {
					AppendSeparator();
					Append(MAP_POPUP_MENU_PROPERTIES, "&Properties", "Properties for the current object");
				}
			}

			AppendSeparator();

			wxMenuItem* browseTile = Append(MAP_POPUP_MENU_BROWSE_TILE, "Browse Field", "Navigate from tile items");
			browseTile->Enable(anything_selected);
		}
	}
	

}

void MapCanvas::getTilesToDraw(int mouse_map_x, int mouse_map_y, int floor, PositionVector* tilestodraw, PositionVector* tilestoborder, bool fill /*= false*/) {
	if (fill) {
		Brush* brush = g_gui.GetCurrentBrush();
		if (!brush || !brush->isGround()) {
			return;
		}

		GroundBrush* newBrush = brush->asGround();
		Position position(mouse_map_x, mouse_map_y, floor);

		Tile* tile = editor.map.getTile(position);
		GroundBrush* oldBrush = nullptr;
		if (tile) {
			oldBrush = tile->getGroundBrush();
		}

		if (oldBrush && oldBrush->getID() == newBrush->getID()) {
			return;
		}

		if ((tile && tile->ground && !oldBrush) || (!tile && oldBrush)) {
			return;
		}

		if (tile && oldBrush) {
			GroundBrush* groundBrush = tile->getGroundBrush();
			if (!groundBrush || groundBrush->getID() != oldBrush->getID()) {
				return;
			}
		}

		std::fill(std::begin(processed), std::end(processed), false);
		floodFill(&editor.map, position, BLOCK_SIZE / 2, BLOCK_SIZE / 2, oldBrush, tilestodraw);

	} else {
		if (g_gui.GetBrushShape() == BRUSHSHAPE_SQUARE) {
			// Use custom dimensions for square brushes
			int brush_width = g_gui.GetBrushWidth();
			int brush_height = g_gui.GetBrushHeight();
			
			// Add debugging output for brush dimensions
			char debug_msg[512];
			sprintf(debug_msg, "DEBUG DRAG: Initial brush dimensions - width=%d, height=%d\n", brush_width, brush_height);
			OutputDebugStringA(debug_msg);
			
			// Make sure we have valid non-zero dimensions
			if (brush_width <= 0) {
				OutputDebugStringA("DEBUG DRAG: WARNING! brush_width <= 0, forcing to 1\n");
				brush_width = 1;
			}
			if (brush_height <= 0) {
				OutputDebugStringA("DEBUG DRAG: WARNING! brush_height <= 0, forcing to 1\n");
				brush_height = 1;
			}
			
			sprintf(debug_msg, "DEBUG DRAG: Corrected brush dimensions - width=%d, height=%d\n", brush_width, brush_height);
			OutputDebugStringA(debug_msg);
			
			// For even-sized brushes, we need to adjust the offset
			int width_offset = (brush_width % 2 == 0) ? 0 : 1;
			int height_offset = (brush_height % 2 == 0) ? 0 : 1;
			
			// Calculate the start position (top-left corner of the brush)
			int start_x = -brush_width / 2;
			int start_y = -brush_height / 2;
			
			// For even width, we need to shift by 1 to avoid center bias
			if (brush_width % 2 == 0) start_x += 1;
			if (brush_height % 2 == 0) start_y += 1;
			
			// Calculate the end position (bottom-right corner of the brush)
			int end_x = start_x + brush_width - 1;
			int end_y = start_y + brush_height - 1;
			
			// Extend by 1 for border calculation
			int border_start_x = start_x - 1;
			int border_start_y = start_y - 1;
			int border_end_x = end_x + 1;
			int border_end_y = end_y + 1;
			
			sprintf(debug_msg, "DEBUG DRAG: Calculated bounds - start_x=%d, start_y=%d, end_x=%d, end_y=%d\n", 
				start_x, start_y, end_x, end_y);
			OutputDebugStringA(debug_msg);
				
			// Draw with a 1-tile margin around the brush for the border/preview
			for (int y = border_start_y; y <= border_end_y; y++) {
				for (int x = border_start_x; x <= border_end_x; x++) {
					// For square brushes, all tiles within the rectangle are drawn
					if (x >= start_x && x <= end_x && y >= start_y && y <= end_y) {
						if (tilestodraw) {
							tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
						}
					}
					
					// Border is 1 tile around the brush edges
					bool is_border = 
						(x >= border_start_x && x <= border_end_x && y >= border_start_y && y <= border_end_y) && // Within extended area
						!(x > start_x && x < end_x && y > start_y && y < end_y); // But not fully inside
						
					if (is_border) {
						if (tilestoborder) {
							tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
						}
					}
				}
			}
		} else if (g_gui.GetBrushShape() == BRUSHSHAPE_CIRCLE) {
			// Circle brushes use traditional radius-based calculation
			for (int y = -g_gui.GetBrushSize() - 1; y <= g_gui.GetBrushSize() + 1; y++) {
				for (int x = -g_gui.GetBrushSize() - 1; x <= g_gui.GetBrushSize() + 1; x++) {
					// Circle brushes use traditional radius-based calculation (ignore custom brush size)
					double distance = sqrt(double(x * x) + double(y * y));
					if (distance < g_gui.GetBrushSize() + 0.005) {
						if (tilestodraw) {
							tilestodraw->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
						}
					}
					if (std::abs(distance - g_gui.GetBrushSize()) < 1.5) {
						if (tilestoborder) {
							tilestoborder->push_back(Position(mouse_map_x + x, mouse_map_y + y, floor));
						}
					}
				}
			}
		}
	}
}

bool MapCanvas::floodFill(Map* map, const Position& center, int x, int y, GroundBrush* brush, PositionVector* positions) {
	countMaxFills++;
	if (countMaxFills > (BLOCK_SIZE * 4 * 4)) {
		countMaxFills = 0;
		return true;
	}

	if (x <= 0 || y <= 0 || x >= BLOCK_SIZE || y >= BLOCK_SIZE) {
		return false;
	}

	processed[getFillIndex(x, y)] = true;

	int px = (center.x + x) - (BLOCK_SIZE / 2);
	int py = (center.y + y) - (BLOCK_SIZE / 2);
	if (px <= 0 || py <= 0 || px >= map->getWidth() || py >= map->getHeight()) {
		return false;
	}

	Tile* tile = map->getTile(px, py, center.z);
	if ((tile && tile->ground && !brush) || (!tile && brush)) {
		return false;
	}

	if (tile && brush) {
		GroundBrush* groundBrush = tile->getGroundBrush();
		if (!groundBrush || groundBrush->getID() != brush->getID()) {
			return false;
		}
	}

	positions->push_back(Position(px, py, center.z));

	bool deny = false;
	if (!processed[getFillIndex(x - 1, y)]) {
		deny = floodFill(map, center, x - 1, y, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x, y - 1)]) {
		deny = floodFill(map, center, x, y - 1, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x + 1, y)]) {
		deny = floodFill(map, center, x + 1, y, brush, positions);
	}

	if (!deny && !processed[getFillIndex(x, y + 1)]) {
		deny = floodFill(map, center, x, y + 1, brush, positions);
	}

	return deny;
}

// ============================================================================
// AnimationTimer

AnimationTimer::AnimationTimer(MapCanvas* canvas) :
	wxTimer(),
	map_canvas(canvas),
	started(false) {
		////
	};

AnimationTimer::~AnimationTimer() {
	////
};

void AnimationTimer::Notify() {
	if (map_canvas->GetZoom() <= 2.0) {
		map_canvas->Refresh();
	}
};

void AnimationTimer::Start() {
	if (!started) {
		started = true;
		wxTimer::Start(100);
	}
};

void AnimationTimer::Stop() {
	if (started) {
		started = false;
		wxTimer::Stop();
	}
};

void MapCanvas::OnFill(wxCommandEvent& WXUNUSED(event)) {
    OutputDebugStringA("INITIATING IMPROVED FILL PROTOCOL! NOW WITH BORDER AWARENESS!\n");



// Check if we should show the warning do not remove this warning functionality
    if (show_fill_warning) {
        wxDialog* dialog = new wxDialog(g_gui.root, wxID_ANY, "Fill Area", 
            wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
            
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        // Add message text
        wxStaticText* message = new wxStaticText(dialog, wxID_ANY, 
            "This operation might take a while if the area is large.\nDo you want to continue?");
        sizer->Add(message, 0, wxALL, 10);
        
        // Add checkbox
        wxCheckBox* checkbox = new wxCheckBox(dialog, wxID_ANY, "Don't show this warning again");
        sizer->Add(checkbox, 0, wxALL, 10);
        
        // Add buttons
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        wxButton* okButton = new wxButton(dialog, wxID_OK, "Continue");
        wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
        buttonSizer->Add(okButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);
        sizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
        
        dialog->SetSizer(sizer);
        sizer->Fit(dialog);
        
        int answer = dialog->ShowModal();
        
        if (answer != wxID_OK) {
            dialog->Destroy();
            return;
        }
        
        // Save preference if checkbox was checked
        if (checkbox->GetValue()) {
            show_fill_warning = false;
        }
        dialog->Destroy();
    }
	// DONT TOUCH this warning box pls 

    if (!g_gui.GetCurrentBrush()) {
        OutputDebugStringA("NO BRUSH SELECTED! THE VOID CANNOT BE FILLED!\n");
        return;
    }

    PositionVector tilestodraw;
    PositionVector tilestoborder;

    // Get cursor position
    int map_x, map_y;
    ScreenToMap(cursor_x, cursor_y, &map_x, &map_y);
    Position start(map_x, map_y, floor);
    
    Tile* start_tile = editor.map.getTile(start);
    bool is_border_fill = false;

    // Check if we have a border item on the starting tile
    if(start_tile) {
        for(Item* item : start_tile->items) {
            if(item && item->isBorder()) {
                is_border_fill = true;
                break;
            }
        }
    }

    if(is_border_fill) {
        OutputDebugStringA("BORDER DETECTED! INITIATING SNAKE-LIKE BORDER FILL!\n");
        
        const int MAX_BORDERS_PER_BATCH = BLOCK_SIZE * 4; // Safety limit per batch
        std::queue<Position> border_queue;
        std::set<Position> processed_borders;
        std::set<Position> to_fill;
        std::set<Position> remaining_borders; // Store borders for next batch

        border_queue.push(start);
        bool continue_filling = true;

        while(continue_filling) {
            int current_batch_count = 0;
            
            while(!border_queue.empty() && current_batch_count < MAX_BORDERS_PER_BATCH) {
                Position current = border_queue.front();
                border_queue.pop();

                if(processed_borders.count(current) > 0) continue;
                processed_borders.insert(current);
                to_fill.insert(current);
                current_batch_count++;

                // Check adjacent tiles for more borders (8 directions)
                Position adjacent[8] = {
                    Position(current.x + 1, current.y, floor),     // Right
                    Position(current.x - 1, current.y, floor),     // Left
                    Position(current.x, current.y + 1, floor),     // Down
                    Position(current.x, current.y - 1, floor),     // Up
                    Position(current.x + 1, current.y + 1, floor), // Bottom-right
                    Position(current.x - 1, current.y - 1, floor), // Top-left
                    Position(current.x - 1, current.y + 1, floor), // Bottom-left
                    Position(current.x + 1, current.y - 1, floor)  // Top-right
                };

                for(const Position& pos : adjacent) {
                    if(processed_borders.count(pos) > 0) continue;
                    
                    Tile* tile = editor.map.getTile(pos);
                    if(!tile) continue;

                    for(Item* item : tile->items) {
                        if(item && item->isBorder()) {
                            if(current_batch_count >= MAX_BORDERS_PER_BATCH) {
                                remaining_borders.insert(pos);
                            } else {
                                border_queue.push(pos);
                            }
                            break;
                        }
                    }
                }
            }

            // Process current batch
            if(!to_fill.empty()) {
                OutputDebugStringA(wxString::Format("PROCESSING BATCH OF %d BORDERS...\n", to_fill.size()).c_str());
                
                Action* action = editor.actionQueue->createAction(ACTION_DRAW);
                for(const Position& pos : to_fill) {
                    Tile* tile = editor.map.getTile(pos);
                    if(!tile) {
                        tile = editor.map.createTile(pos.x, pos.y, pos.z);
                    }
                    Tile* new_tile = tile->deepCopy(editor.map);
                    g_gui.GetCurrentBrush()->draw(&editor.map, new_tile, nullptr);
                    action->addChange(newd Change(new_tile));
                }
                editor.addAction(action);
                g_gui.RefreshView();
            }

            // Check if we should continue with remaining borders
            if(!remaining_borders.empty()) {
                wxString message = wxString::Format(
                    "Processed %d borders. There are %d more borders to process.\nContinue filling?",
                    processed_borders.size(), remaining_borders.size());
                
                int answer = g_gui.PopupDialog(
                    "Continue Border Fill?", 
                    message,
                    wxYES_NO
                );

                if(answer == wxID_YES) {
                    OutputDebugStringA("CONTINUING WITH NEXT BATCH OF BORDERS...\n");
                    // Move remaining borders to queue for next batch
                    for(const Position& pos : remaining_borders) {
                        border_queue.push(pos);
                    }
                    remaining_borders.clear();
                    to_fill.clear();
                } else {
                    OutputDebugStringA("BORDER FILL STOPPED BY USER!\n");
                    continue_filling = false;
                }
            } else {
                OutputDebugStringA("ALL BORDERS PROCESSED! THE SNAKE IS SATISFIED!\n");
                continue_filling = false;
            }
        }
    } else {
        // Normal fill with area validation
        OutputDebugStringA("NORMAL FILL INITIATED! VALIDATING AREA...\n");
        
        std::queue<Position> to_check;
        std::set<Position> checked;
        to_check.push(start);
        bool escape_found = false;

        while (!to_check.empty() && !escape_found) {
            Position pos = to_check.front();
            to_check.pop();

            if (checked.count(pos) > 0) continue;
            checked.insert(pos);

            // Check map boundaries
            if (pos.x <= 0 || pos.y <= 0 || 
                pos.x >= editor.map.getWidth() - 1 || 
                pos.y >= editor.map.getHeight() - 1) {
                escape_found = true;
                break;
            }

            Tile* tile = editor.map.getTile(pos);
            bool is_empty = (!tile || 
                           (!tile->spawn || !g_settings.getInteger(Config::SHOW_SPAWNS)) && 
                           (!tile->creature || !g_settings.getInteger(Config::SHOW_CREATURES)) && 
                           !tile->getTopItem());

            if (!is_empty) continue;

            // Check adjacent tiles (4-directional)
            Position adjacent[4] = {
                Position(pos.x + 1, pos.y, floor),
                Position(pos.x - 1, pos.y, floor),
                Position(pos.x, pos.y + 1, floor),
                Position(pos.x, pos.y - 1, floor)
            };

            for (const Position& next : adjacent) {
                if (checked.count(next) > 0) continue;
                
                Tile* next_tile = editor.map.getTile(next);
                bool next_is_empty = (!next_tile || 
                                    (!next_tile->spawn || !g_settings.getInteger(Config::SHOW_SPAWNS)) && 
                                    (!next_tile->creature || !g_settings.getInteger(Config::SHOW_CREATURES)) && 
                                    !next_tile->getTopItem());
                
                if (next_is_empty) {
                    to_check.push(next);
                }
            }
        }

        if (escape_found) {
            OutputDebugStringA("AREA NOT ENCLOSED! THE VOID LEAKS!\n");
            g_gui.PopupDialog("Error", "Cannot fill - area is not enclosed.", wxOK);
            return;
        }

        OutputDebugStringA(wxString::Format("FOUND %d TILES TO FILL NORMALLY!\n", checked.size()).c_str());
        
        Action* action = editor.actionQueue->createAction(ACTION_DRAW);
        for (const Position& pos : checked) {
            Tile* tile = editor.map.getTile(pos);
            if (!tile) {
                tile = editor.map.createTile(pos.x, pos.y, pos.z);
            }
            Tile* new_tile = tile->deepCopy(editor.map);
            g_gui.GetCurrentBrush()->draw(&editor.map, new_tile, nullptr);
            action->addChange(newd Change(new_tile));
        }
        
        editor.addAction(action);
        g_gui.RefreshView();
        OutputDebugStringA("NORMAL FILL COMPLETE! THE VOID HAS BEEN FILLED!\n");
    }
}

/*
============================================================================
SELECTION TO DOODAD BRUSH TASK - COMPLETED ✓
============================================================================

OBJECTIVE:
Ensure proper handling of ground tiles and borders when converting area selections 
to doodad brushes.

FINAL BEHAVIOR:
✓ Individual item selection works correctly
✓ Ground tiles are only included when explicitly selected
✓ Border items are properly preserved
✓ Collections.xml integration working
✓ Doodads.xml saving working
✓ Layer-specific selection preserved (no unwanted grounds under borders)
✓ Item stacking order maintained
✓ All selected items included correctly
✓ No unselected items included in brush

IMPLEMENTATION DETAILS:
- Uses tile->ground for ground layer items
- Checks isBorder() for border items
- Maintains proper XML structure in doodads.xml
- Updates collections.xml name reference
- Preserves all item properties
- Only includes explicitly selected items
- Proper debug logging added

============================================================================
*/


void MapCanvas::OnSelectionToDoodad(wxCommandEvent& WXUNUSED(event)) {
    OutputDebugStringA("INITIATING DOODAD CREATION PROTOCOL! MUAHAHAHA!\n");

    if (editor.selection.size() == 0) {
        OutputDebugStringA("OH THE HUMANITY! THE SELECTION IS AS EMPTY AS MY SOUL!\n");
        g_gui.PopupDialog(this, "Error", "Y U GIVE EMPTY SELECTION?! (╯°□°）╯︵ ┻━┻", wxOK);
        return;
    }

    OutputDebugStringA(wxString::Format("DETECTED %d TILES! TIME TO PERFORM UNSPEAKABLE ACTS OF XML CREATION!\n", 
        editor.selection.size()).c_str());

    Position minPos(0xFFFF, 0xFFFF, 0xFFFF);
    Position maxPos(0, 0, 0);
    
    int tileCount = 0;
    int totalItems = 0;
    // Use a map with Position as key to store items by their 3D coordinates
    std::map<Position, std::vector<Item*>> tileItems;
    
    OutputDebugStringA("COMMENCING TILE INSPECTION! RESISTANCE IS FUTILE!\n");
    
    // First, analyze what's actually in the selection
    for(auto tile : editor.selection) {
        if(!tile) continue;
        
        Position tilePos(tile->getX(), tile->getY(), tile->getZ());
        OutputDebugStringA(wxString::Format("\nTile at %d,%d,%d:\n", 
            tilePos.x, tilePos.y, tilePos.z).c_str());
        
        // Check if the tile itself is selected (vs just items on the tile)
        bool tileSelected = editor.selection.getTiles().count(tile) > 0;
        
        // If entire tile is selected, add all its items
        if(tileSelected) {
            if(tile->ground) {
                tileItems[tilePos].push_back(tile->ground);
                totalItems++;
                OutputDebugStringA(wxString::Format("Adding ground %d from selected tile at %d,%d,%d\n", 
                    tile->ground->getID(), tilePos.x, tilePos.y, tilePos.z).c_str());
            }
            
            for(Item* item : tile->items) {
                if(!item) continue;
                if(!item->isBorder()) {
                    tileItems[tilePos].push_back(item);
                    totalItems++;
                    OutputDebugStringA(wxString::Format("Adding item %d from selected tile at %d,%d,%d\n", 
                        item->getID(), tilePos.x, tilePos.y, tilePos.z).c_str());
                }
            }
        } 
        // Otherwise, only add individually selected items
        else {
            // First check if ground is selected
            if(tile->ground && tile->ground->isSelected()) {
                tileItems[tilePos].push_back(tile->ground);
                totalItems++;
                OutputDebugStringA(wxString::Format("Adding selected ground %d at %d,%d,%d\n",
                    tile->ground->getID(), tilePos.x, tilePos.y, tilePos.z).c_str());
            }
            
            // Then add selected items
            for(Item* item : tile->items) {
                if(item && item->isSelected()) {
                    tileItems[tilePos].push_back(item);
                    totalItems++;
                    OutputDebugStringA(wxString::Format("Adding selected item %d at %d,%d,%d\n",
                        item->getID(), tilePos.x, tilePos.y, tilePos.z).c_str());
                }
            }
        }
        
        // Only count this as a tile if we added items from it
        if(!tileItems[tilePos].empty()) {
            tileCount++;
            
            // Update min/max positions
            if(tilePos.x < minPos.x) minPos.x = tilePos.x;
            if(tilePos.y < minPos.y) minPos.y = tilePos.y;
            if(tilePos.z < minPos.z) minPos.z = tilePos.z;
            if(tilePos.x > maxPos.x) maxPos.x = tilePos.x;
            if(tilePos.y > maxPos.y) maxPos.y = tilePos.y;
            if(tilePos.z > maxPos.z) maxPos.z = tilePos.z;
        } else {
            // Remove empty position entry
            tileItems.erase(tilePos);
        }
    }

    OutputDebugStringA(wxString::Format("MWAHAHAHA! ACQUIRED %d ITEMS FROM %d TILES! THE COLLECTION GROWS!\n", 
        totalItems, tileCount).c_str());

    // Display multi-floor information
    if (minPos.z != maxPos.z) {
        OutputDebugStringA(wxString::Format("MULTI-FLOOR SELECTION DETECTED! FROM FLOOR %d TO %d\n", 
            minPos.z, maxPos.z).c_str());
    }

    if(totalItems == 0) {
        OutputDebugStringA("WHAT IS THIS MADNESS?! NO ITEMS TO STEAL?! INCONCEIVABLE!\n");
        g_gui.PopupDialog(this, "Error", "WHERE ARE THE ITEMS?! ┻━┻ ︵ヽ(`Д´)ﾉ︵ ┻━┻", wxOK);
        return;
    }

    // Get the proper data directory paths
    wxString versionString = g_gui.GetCurrentVersion().getName();
    std::string versionStr = std::string(versionString.mb_str());
    
    // Convert version number to data directory format
    // Remove dots first
    versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());
    
    // Handle special cases for 2-digit versions (add 0)
    if(versionStr.length() == 2) {
        versionStr += "0";
    }
    // Handle special case for 10.10 -> 10100
    else if(versionStr == "1010") {
        versionStr = "10100";
    }
    
    OutputDebugStringA(wxString::Format("CONVERTED VERSION %s TO DIRECTORY %s\n", 
        versionString, versionStr).c_str());
    
    FileName doodadsPath = g_gui.GetDataDirectory();
    doodadsPath.SetPath(doodadsPath.GetPath() + "/" + versionStr);
    doodadsPath.SetName("doodads.xml");
    
    wxString doodadsPathStr = doodadsPath.GetFullPath();
    OutputDebugStringA(wxString::Format("ATTEMPTING TO ACCESS DOODADS AT: %s\n", doodadsPathStr).c_str());
    
    FileName collectionsPath = g_gui.GetDataDirectory();
    collectionsPath.SetPath(collectionsPath.GetPath() + "/" + versionStr);
    collectionsPath.SetName("collections.xml");
    
    wxString collectionsPathStr = collectionsPath.GetFullPath();

    // After loading paths but before creating the brush...
    // Load collections.xml to get existing tilesets
    std::vector<wxString> tilesetNames;
    wxXmlDocument collectionsDoc;
    if (collectionsDoc.Load(collectionsPathStr)) {
        wxXmlNode* root = collectionsDoc.GetRoot();
        if (root) {
            for (wxXmlNode* node = root->GetChildren(); node; node = node->GetNext()) {
                if (node->GetName() == "tileset") {
                    tilesetNames.push_back(node->GetAttribute("name"));
                }
            }
        }
    } else {
        OutputDebugStringA("THE COLLECTIONS TOME IS MISSING! CREATING A NEW ONE!\n");
        wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "materials");
        collectionsDoc.SetRoot(root);
    }

    if (tilesetNames.empty()) {
        g_gui.PopupDialog(this, "Error", "No tilesets found in collections.xml!", wxOK);
        return;
    }

    // Create tileset selection dialog
    wxDialog* dialog = new wxDialog(g_gui.root, wxID_ANY, "Select Collection", 
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);

    wxBoxSizer* dialogSizer = new wxBoxSizer(wxVERTICAL);

    // Add dropdown for existing collections
    wxBoxSizer* choiceSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* choiceLabel = new wxStaticText(dialog, wxID_ANY, "Collection:");
    wxChoice* collectionChoice = new wxChoice(dialog, wxID_ANY);
    
    for (const wxString& name : tilesetNames) {
        collectionChoice->Append(name);
    }
    collectionChoice->SetSelection(0);
    
    choiceSizer->Add(choiceLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    choiceSizer->Add(collectionChoice, 1, wxALL | wxEXPAND, 5);
    dialogSizer->Add(choiceSizer, 0, wxEXPAND);

    // Add buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    dialogSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(dialogSizer);
    dialogSizer->Fit(dialog);

    if (dialog->ShowModal() != wxID_OK) {
        dialog->Destroy();
        return;
    }

    wxString selectedTileset = collectionChoice->GetString(collectionChoice->GetSelection());
    dialog->Destroy();

    // Create doodad brush
    wxXmlDocument doodadsDoc;
    if(!doodadsDoc.Load(doodadsPathStr)) {
        OutputDebugStringA("THE DOODADS TOME DOES NOT EXIST! CREATING A NEW ONE!\n");
        wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "materials");
        doodadsDoc.SetRoot(root);
    }

    // Continue with existing doodad creation code...
    wxXmlNode* collectionsRoot = collectionsDoc.GetRoot();
    wxXmlNode* firstTileset = nullptr;

    for(wxXmlNode* node = collectionsRoot->GetChildren(); node; node = node->GetNext()) {
        if(node->GetName() == "tileset" && node->GetAttribute("name") == selectedTileset) {
            firstTileset = node;
            break;
        }
    }

    if(!firstTileset) {
        g_gui.PopupDialog(this, "Error", "Selected tileset not found!", wxOK);
        return;
    }

    // Find or create collections node
    wxXmlNode* collections = firstTileset->GetChildren();
    if(!collections || collections->GetName() != "collections") {
        OutputDebugStringA("NO COLLECTIONS NODE FOUND! CREATING ONE FROM THE VOID!\n");
        collections = new wxXmlNode(wxXML_ELEMENT_NODE, "collections");
        firstTileset->AddChild(collections);
    }

    // Find highest custom number
    int highestNum = 0;
    wxXmlNode* doodadsRoot = doodadsDoc.GetRoot();
    for(wxXmlNode* node = doodadsRoot->GetChildren(); node; node = node->GetNext()) {
        if(node->GetName() == "brush") {
            wxString name = node->GetAttribute("name");
            if(name.StartsWith("custom_")) {
                long num;
                if(name.Mid(7).ToLong(&num)) {
                    highestNum = std::max(highestNum, (int)num);
                }
            }
        }
    }

    wxString newBrushName = wxString::Format("custom_%d", highestNum + 1);
    OutputDebugStringA(wxString::Format("BEHOLD! NEW BRUSH SHALL BE NAMED %s!\n", newBrushName).c_str());

    // Create the ULTIMATE BRUSH NODE with full pattern
    wxXmlNode* newBrushNode = new wxXmlNode(wxXML_ELEMENT_NODE, "brush");
    newBrushNode->AddAttribute("name", newBrushName);
    newBrushNode->AddAttribute("type", "doodad");
    newBrushNode->AddAttribute("server_lookid", wxString::Format("%d", tileItems.begin()->second.front()->getID()));
    newBrushNode->AddAttribute("draggable", "true");
    newBrushNode->AddAttribute("on_blocking", "true");
    newBrushNode->AddAttribute("thickness", "100/100");
    
    // Add multi-floor support attribute if this is a multi-floor doodad
    if (minPos.z != maxPos.z) {
        newBrushNode->AddAttribute("multi_floor", "true");
    }

    // Create the alternate/composite structure
    wxXmlNode* alternateNode = new wxXmlNode(wxXML_ELEMENT_NODE, "alternate");
    wxXmlNode* compositeNode = new wxXmlNode(wxXML_ELEMENT_NODE, "composite");
    compositeNode->AddAttribute("chance", "10");

    OutputDebugStringA("\n=== GENERATING XML ===\n");
    
    // Add ALL the items to their relative positions
    for(const auto& tilePair : tileItems) {
        const Position& pos = tilePair.first;
        int relX = pos.x - minPos.x;
        int relY = pos.y - minPos.y;
        int relZ = pos.z - minPos.z;  // Calculate relative Z
        
        // Create tile node once per position
        wxXmlNode* tileNode = new wxXmlNode(wxXML_ELEMENT_NODE, "tile");
        tileNode->AddAttribute("x", wxString::Format("%d", relX));
        tileNode->AddAttribute("y", wxString::Format("%d", relY));
        tileNode->AddAttribute("z", wxString::Format("%d", relZ));  // Add Z coordinate
        
        OutputDebugStringA(wxString::Format("Creating tile node at x=%d y=%d z=%d\n", 
            relX, relY, relZ).c_str());
        
        // Add all collected items for this position
        for(Item* item : tilePair.second) {
            wxXmlNode* itemNode = new wxXmlNode(wxXML_ELEMENT_NODE, "item");
            itemNode->AddAttribute("id", wxString::Format("%d", item->getID()));
            tileNode->AddChild(itemNode);
            OutputDebugStringA(wxString::Format("  Adding item id=%d\n", item->getID()).c_str());
        }
        
        compositeNode->AddChild(tileNode);
    }

    OutputDebugStringA("=== XML GENERATION COMPLETE ===\n");

    alternateNode->AddChild(compositeNode);
    newBrushNode->AddChild(alternateNode);
    doodadsDoc.GetRoot()->AddChild(newBrushNode);

    // Save doodads.xml
    if(!doodadsDoc.Save(doodadsPathStr)) {
        OutputDebugStringA("THE DOODADS TOME RESISTS OUR CHANGES!\n");
        g_gui.PopupDialog(this, "Error", "Could not write to doodads.xml!", wxOK);
        return;
    }

    // Find or create collections node in the selected tileset
    collections = firstTileset->GetChildren();
    if(!collections || collections->GetName() != "collections") {
        OutputDebugStringA("NO COLLECTIONS NODE FOUND! CREATING ONE FROM THE VOID!\n");
        collections = new wxXmlNode(wxXML_ELEMENT_NODE, "collections");
        firstTileset->AddChild(collections);
    }

    // Add brush reference to the selected tileset
    wxXmlNode* brushRef = new wxXmlNode(wxXML_ELEMENT_NODE, "brush");
    brushRef->AddAttribute("name", newBrushName);
    collections->AddChild(brushRef);

    // Save collections.xml
    if(!collectionsDoc.Save(collectionsPathStr)) {
        OutputDebugStringA("THE COLLECTIONS TOME RESISTS OUR CHANGES!\n");
        g_gui.PopupDialog(this, "Error", "Could not write to collections.xml!", wxOK);
        return;
    }
    OutputDebugStringA(wxString::Format("THE BRUSH HAS BEEN BOUND TO THE %s TILESET!\n", selectedTileset).c_str());

    OutputDebugStringA("THE RITUAL IS COMPLETE! THE DOODAD HAS BEEN BOUND!\n");
    
    // Create success dialog with two buttons
    wxDialog* successDoodadDialog = new wxDialog(g_gui.root, wxID_ANY, "Success!", 
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
        
    wxBoxSizer* doodadDialogSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add message text
    wxStaticText* doodadMessage = new wxStaticText(successDoodadDialog, wxID_ANY, 
        "IT'S ALIVE! IT'S ALIVE!\nCreated: " + newBrushName);
    doodadDialogSizer->Add(doodadMessage, 0, wxALL | wxALIGN_CENTER, 10);
    
    // Add buttons
    wxBoxSizer* doodadButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* doodadContinueButton = new wxButton(successDoodadDialog, wxID_OK, "Continue");
    wxButton* doodadRefreshButton = new wxButton(successDoodadDialog, wxID_APPLY, "Refresh Palette");
    doodadButtonSizer->Add(doodadContinueButton, 0, wxALL, 5);
    doodadButtonSizer->Add(doodadRefreshButton, 0, wxALL, 5);
    doodadDialogSizer->Add(doodadButtonSizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    successDoodadDialog->SetSizer(doodadDialogSizer);
    doodadDialogSizer->Fit(successDoodadDialog);
    
    // Handle button clicks
    doodadRefreshButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        OutputDebugStringA("INITIATING DARK RITUAL OF PALETTE RECONSTRUCTION!\n");
        wxString error;
        wxArrayString warnings;
        if(!g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true)) {
            OutputDebugStringA("THE RITUAL HAS FAILED! FALLING BACK TO PLAN B!\n");
            PaletteWindow* palette = dynamic_cast<PaletteWindow*>(g_gui.GetPalette());
            if(palette) {
                OutputDebugStringA("COMMANDING THE PALETTE TO RECONSTRUCT ITSELF!\n");
                palette->InvalidateContents();
                palette->SelectPage(TILESET_DOODAD);
                g_gui.RefreshPalettes();
            }
        } else {
            PaletteWindow* palette = dynamic_cast<PaletteWindow*>(g_gui.GetPalette());
            if(palette) {
                palette->SelectPage(TILESET_DOODAD);
                g_gui.RefreshPalettes();
            }
        }
        successDoodadDialog->EndModal(wxID_APPLY);
    });
    
    doodadContinueButton->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
        OutputDebugStringA("CONTINUING WITHOUT PALETTE REFRESH!\n");
        successDoodadDialog->EndModal(wxID_OK);
    });
    
    successDoodadDialog->ShowModal();
    successDoodadDialog->Destroy();
}

void MapCanvas::OnFindSimilarItems(wxCommandEvent& WXUNUSED(event)) {
    if (editor.selection.size() == 0) {
        return;
    }

    // Get the selected tile and items
    Tile* tile = editor.selection.getSelectedTile();
    if (!tile) {
        return;
    }

    ItemVector selected_items = tile->getSelectedItems();
    if (selected_items.empty()) {
        return;
    }

    // Create and configure the find dialog
    FindItemDialog* dialog = new FindItemDialog(g_gui.root, "Find Similar Items", false);
    
    if (selected_items.size() == 1) {
        // Single item selected - search by exact ID
        Item* item = selected_items[0];
        dialog->setSearchMode(FindItemDialog::SearchMode::ServerIDs);
        
        // Set the server ID using the spin control
        if (wxSpinCtrl* server_id_spin = (wxSpinCtrl*)dialog->FindWindow(wxID_ANY)) {
            server_id_spin->SetValue(item->getID());
        }
    } else {
        // Multiple items selected - use range search
        dialog->setSearchMode(FindItemDialog::SearchMode::ServerIDs);
        
        // Build range string from selected items
        std::ostringstream range;
        bool first = true;
        for (Item* item : selected_items) {
            if (!first) {
                range << ",";
            }
            range << item->getID();
            first = false;
        }

        // Enable range searching
        if (wxCheckBox* use_range = (wxCheckBox*)dialog->FindWindow(wxID_ANY)) {
            use_range->SetValue(true);
        }
        
        // Set the range text
        if (wxTextCtrl* range_input = (wxTextCtrl*)dialog->FindWindow(wxID_ANY)) {
            range_input->SetValue(wxString(range.str()));
        }
    }

    // Show dialog
    dialog->ShowModal();
    dialog->Destroy();
}



void MapCanvas::OnCreateHouse(wxCommandEvent& event) {
    int start_map_x = last_click_map_x;
    int start_map_y = last_click_map_y;
    int current_floor = floor;

    if (start_map_x == -1 || start_map_y == -1) {
        g_gui.PopupDialog("Error", "You must click on a tile inside the house you want to create.", wxOK);
        return;
    }

    // Check if we're already on a house tile
    Tile* start_tile = editor.map.getTile(start_map_x, start_map_y, current_floor);
    if (start_tile && start_tile->isHouseTile()) {
        g_gui.PopupDialog("Warning", "This tile already belongs to a house. Please select a tile that doesn't belong to any house.", wxOK);
        return;
    }

    // Structure to hold detected house data by floor
    struct HouseFloorData {
        std::set<Position> wall_positions;
        std::set<Position> interior_positions;
        Position exit_pos;
        bool has_exit = false;
    };
    
    std::map<int, HouseFloorData> house_floors;
    auto& current_floor_data = house_floors[current_floor];
    
    // Step 1: First, detect the house from the clicked position
    // We'll look for walls in a small radius to determine if we're inside a house
    const int WALL_SEARCH_RADIUS = 10;
    bool found_wall = false;
    
    for (int r = 1; r <= WALL_SEARCH_RADIUS && !found_wall; r++) {
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                // Only check perimeter
                if (std::abs(x) != r && std::abs(y) != r) continue;
                
                Position pos(start_map_x + x, start_map_y + y, current_floor);
                Tile* tile = editor.map.getTile(pos);
                
                if (tile && hasHouseWall(tile)) {
                    found_wall = true;
                    break;
                }
            }
            if (found_wall) break;
        }
    }
    
    if (!found_wall) {
        g_gui.PopupDialog("Error", "Could not find any walls near the selected position. Please click inside a house.", wxOK);
        return;
    }
    
    // Step 2: Find a door - it's likely that a house has at least one door
    Position door_pos;
    bool found_door = false;
    const int DOOR_SEARCH_RADIUS = 15;
    
    for (int r = 1; r <= DOOR_SEARCH_RADIUS && !found_door; r++) {
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                // Only check perimeter
                if (std::abs(x) != r && std::abs(y) != r) continue;
                
                Position pos(start_map_x + x, start_map_y + y, current_floor);
                Tile* tile = editor.map.getTile(pos);
                
                if (tile && hasDoor(tile)) {
                    door_pos = pos;
                            found_door = true;
                                break;
                        }
            }
            if (found_door) break;
        }
    }

    // If no door is found, we'll just use the start position
    if (!found_door) {
        door_pos = Position(start_map_x, start_map_y, current_floor);
    }
    
    // Step 3: First pass - identify all walls
    std::set<Position> all_walls;
    std::queue<Position> wall_queue;
    
    // Begin by finding walls around the start position
    const int INITIAL_WALL_RADIUS = 3;
    for (int y = -INITIAL_WALL_RADIUS; y <= INITIAL_WALL_RADIUS; y++) {
        for (int x = -INITIAL_WALL_RADIUS; x <= INITIAL_WALL_RADIUS; x++) {
            Position pos(start_map_x + x, start_map_y + y, current_floor);
            Tile* tile = editor.map.getTile(pos);
            
            if (tile && hasHouseWall(tile)) {
                wall_queue.push(pos);
                all_walls.insert(pos);
            }
        }
    }
    
    // BFS to find all connected walls (limited depth to prevent crossing to adjacent houses)
    const int MAX_WALL_EXPANSION = 25; // Limit how far we expand from start
    std::set<Position> visited_walls;
    
    while (!wall_queue.empty()) {
        Position current = wall_queue.front();
        wall_queue.pop();
        
        if (visited_walls.count(current) > 0) continue;
        visited_walls.insert(current);
        
        current_floor_data.wall_positions.insert(current);
        
        // Check for door exits
            Tile* tile = editor.map.getTile(current);
        if (tile && hasDoor(tile)) {
            // Check outside the door
            const int dirs[4][2] = {{0,-1}, {1,0}, {0,1}, {-1,0}};
            for (int d = 0; d < 4; d++) {
                Position exit_pos(current.x + dirs[d][0], current.y + dirs[d][1], current.z);
                Tile* exit_tile = editor.map.getTile(exit_pos);
                
                // Skip walls and existing house tiles
                if (!exit_tile || hasHouseWall(exit_tile) || exit_tile->isHouseTile()) continue;
                
                // Found a potential exit
                current_floor_data.exit_pos = exit_pos;
                current_floor_data.has_exit = true;
                                    break;
            }
        }
        
        // Only expand to nearby walls
        if (std::abs(current.x - start_map_x) <= MAX_WALL_EXPANSION && 
            std::abs(current.y - start_map_y) <= MAX_WALL_EXPANSION) {
            
            // Check adjacent positions for more walls (including diagonals for better connectivity)
            for (int y = -1; y <= 1; y++) {
                for (int x = -1; x <= 1; x++) {
                    if (x == 0 && y == 0) continue;
                    
                    Position next(current.x + x, current.y + y, current.z);
                    
                    // Only visit each wall once
                    if (visited_walls.count(next) > 0) continue;
                    
                    Tile* next_tile = editor.map.getTile(next);
                    if (next_tile && hasHouseWall(next_tile)) {
                        wall_queue.push(next);
                    }
                }
            }
        }
    }
    
    // Step 4: Calculate bounding box of house walls
            int min_x = std::numeric_limits<int>::max();
            int min_y = std::numeric_limits<int>::max();
            int max_x = std::numeric_limits<int>::min();
            int max_y = std::numeric_limits<int>::min();
            
    for (const Position& pos : current_floor_data.wall_positions) {
        min_x = std::min(min_x, pos.x);
        min_y = std::min(min_y, pos.y);
        max_x = std::max(max_x, pos.x);
        max_y = std::max(max_y, pos.y);
    }
    
    // Expand bounds slightly
    min_x = std::max(0, min_x - 1);
    min_y = std::max(0, min_y - 1);
    max_x = std::min(editor.map.getWidth(), max_x + 1);
    max_y = std::min(editor.map.getHeight(), max_y + 1);
    
    // Step 5: Flood fill interior from clicked position
    std::set<Position> visited_interior;
    std::queue<Position> interior_queue;
    
    // Start from the user's click position
    Position fill_start(start_map_x, start_map_y, current_floor);
    interior_queue.push(fill_start);
    
    while (!interior_queue.empty()) {
        Position current = interior_queue.front();
        interior_queue.pop();
        
        // Skip if outside bounds, already visited, or a wall
        if (current.x < min_x || current.x > max_x || 
            current.y < min_y || current.y > max_y ||
            visited_interior.count(current) > 0 ||
            current_floor_data.wall_positions.count(current) > 0) {
                        continue;
                    }
                    
        // Mark as visited
        visited_interior.insert(current);
        
        // For interior, we require being inside the wall boundary
        // Check if this position is inside using a smarter approach
        
        // Use a more reliable method to check if point is inside
        bool is_inside = true;
        
        // Cast rays in 8 directions to better detect enclosed spaces
        const int directions[8][2] = {
            {1, 0},   // right
            {-1, 0},  // left
            {0, 1},   // down
            {0, -1},  // up
            {1, 1},   // down-right
            {-1, -1}, // up-left
            {1, -1},  // up-right
            {-1, 1}   // down-left
        };
        
        int wall_hits = 0;
        for (int d = 0; d < 8; d++) {
            bool found_wall = false;
            int ray_dist = 0;
            int max_ray_dist = 2 * std::max(max_x - min_x, max_y - min_y); // Increase ray distance
            
            for (int dist = 1; dist <= max_ray_dist; dist++) {
                int ray_x = current.x + (directions[d][0] * dist);
                int ray_y = current.y + (directions[d][1] * dist);
                
                // Stop if we're outside the bounding box with larger margin
                if (ray_x < min_x - 5 || ray_x > max_x + 5 || ray_y < min_y - 5 || ray_y > max_y + 5) {
                    break;
                }
                
                Position ray_pos(ray_x, ray_y, current.z);
                if (current_floor_data.wall_positions.count(ray_pos) > 0) {
                    found_wall = true;
                    ray_dist = dist;
                    break;
                }
            }
            
            if (found_wall) {
                wall_hits++;
            }
        }
        
        // More permissive check - if we hit walls in at least 5 of 8 directions for diagonal rays
        // or at least 2 of 4 cardinal directions, consider it inside
        if (wall_hits >= 5 || (
            (current_floor_data.wall_positions.count(Position(current.x + 1, current.y, current.z)) > 0 ? 1 : 0) +
            (current_floor_data.wall_positions.count(Position(current.x - 1, current.y, current.z)) > 0 ? 1 : 0) +
            (current_floor_data.wall_positions.count(Position(current.x, current.y + 1, current.z)) > 0 ? 1 : 0) +
            (current_floor_data.wall_positions.count(Position(current.x, current.y - 1, current.z)) > 0 ? 1 : 0) >= 2
        )) {
            current_floor_data.interior_positions.insert(current);
            
            // Use only cardinal directions (4-way) for expanding the fill
            const int fillDirections[4][2] = {
                {1, 0}, {-1, 0}, {0, 1}, {0, -1}
            };
            
            // Add neighboring tiles
            for (int d = 0; d < 4; d++) {
                Position next(current.x + fillDirections[d][0], 
                             current.y + fillDirections[d][1], 
                             current.z);
                
                // Only queue if we haven't visited and it's not a wall
                if (visited_interior.count(next) == 0 && 
                    current_floor_data.wall_positions.count(next) == 0) {
                    interior_queue.push(next);
                }
            }
        }
    }
    
    // Step 6: Check for floors above and below (expanded to handle multiple floors)
    std::set<int> processed_floors;
    processed_floors.insert(current_floor);
    std::queue<int> floors_to_check;
    
    // First add adjacent floors
    if (current_floor - 1 >= 0) floors_to_check.push(current_floor - 1);
    if (current_floor + 1 < MAP_LAYERS) floors_to_check.push(current_floor + 1);
    
    // Process floors in queue and potentially add more
    while (!floors_to_check.empty()) {
        int check_floor = floors_to_check.front();
        floors_to_check.pop();
        
        // Skip if already processed or outside valid range
        if (processed_floors.count(check_floor) > 0 || check_floor < 0 || check_floor >= MAP_LAYERS) 
            continue;
        
        processed_floors.insert(check_floor);
        auto& floor_data = house_floors[check_floor];
        
        // Connect via walls/stairs from already processed floors
        bool floor_connected = false;
        
        // Try to find connections from any previously processed floor
        for (int processed_floor : processed_floors) {
            if (processed_floor == check_floor) continue;
            
            Position stair_pos;
            bool found_stair_connection = false;
            
            // Check for stair connections only if floors are not adjacent
            if (std::abs(processed_floor - check_floor) > 1) {
                // Look for stairs in the processed floor
                for (const Position& pos : house_floors[processed_floor].interior_positions) {
                    Position check_stair_pos(pos.x, pos.y, processed_floor);
                    Tile* tile = editor.map.getTile(check_stair_pos);
                    
                    if (tile && hasStairsOrLadder(tile)) {
                        // Check if there's a corresponding stair/ladder on target floor
                        Position target_stair_pos(pos.x, pos.y, check_floor);
                        Tile* target_tile = editor.map.getTile(target_stair_pos);
                        
                        if (target_tile && hasStairsOrLadder(target_tile)) {
                            found_stair_connection = true;
                            stair_pos = target_stair_pos;
                break;
                        }
                    }
                }
                
                if (found_stair_connection) {
                    floor_connected = true;
                    // Start wall and interior detection from stair position
                    Position start_point = stair_pos;
                    
                    // Find walls around stair position
                    std::set<Position> floor_visited_walls;
                    std::queue<Position> floor_wall_queue;
                    
                    for (int y = -2; y <= 2; y++) {
                        for (int x = -2; x <= 2; x++) {
                            Position pos(start_point.x + x, start_point.y + y, check_floor);
                            Tile* tile = editor.map.getTile(pos);
                            
                            if (tile && hasHouseWall(tile)) {
                                floor_wall_queue.push(pos);
                                break;
                            }
                        }
                        if (!floor_wall_queue.empty()) break;
                    }
                    
                    // If no walls found near stairs, just use the stair position
                    if (floor_wall_queue.empty()) {
                        floor_wall_queue.push(start_point);
                    }
                    
                    // Continue with wall detection...
                    // Rest of code similar to the original detection for other floors
                }
            } else {
                // For adjacent floors, use wall matching as before
                for (const Position& wall_pos : house_floors[processed_floor].wall_positions) {
                    Position check_pos(wall_pos.x, wall_pos.y, check_floor);
                    Tile* tile = editor.map.getTile(check_pos);
                    
                    if (tile && hasHouseWall(tile)) {
                        floor_connected = true;
                        Position start_point = check_pos;
                        
                        // Find walls
                        std::set<Position> floor_visited_walls;
                        std::queue<Position> floor_wall_queue;
                        floor_wall_queue.push(start_point);
                        
                        while (!floor_wall_queue.empty()) {
                            Position current = floor_wall_queue.front();
                            floor_wall_queue.pop();
                            
                            if (floor_visited_walls.count(current) > 0) continue;
                            floor_visited_walls.insert(current);
                            
                            floor_data.wall_positions.insert(current);
                            
                            // Check for door exits
                            Tile* tile = editor.map.getTile(current);
                            if (tile && hasDoor(tile)) {
                                // Check outside the door
                                const int dirs[4][2] = {{0,-1}, {1,0}, {0,1}, {-1,0}};
                                for (int d = 0; d < 4; d++) {
                                    Position exit_pos(current.x + dirs[d][0], current.y + dirs[d][1], current.z);
                                    Tile* exit_tile = editor.map.getTile(exit_pos);
                                    
                                    if (!exit_tile || hasHouseWall(exit_tile) || exit_tile->isHouseTile()) continue;
                                    
                                    floor_data.exit_pos = exit_pos;
                                    floor_data.has_exit = true;
                                    break;
                                }
                            }
                            
                            // Only expand within the same bounds as the current floor
                            if (current.x >= min_x && current.x <= max_x && 
                                current.y >= min_y && current.y <= max_y) {
                                
                                for (int y = -1; y <= 1; y++) {
                                    for (int x = -1; x <= 1; x++) {
                                        if (x == 0 && y == 0) continue;
                                        
                                        Position next(current.x + x, current.y + y, current.z);
                                        
                                        if (floor_visited_walls.count(next) > 0) continue;
                                        
                                        Tile* next_tile = editor.map.getTile(next);
                                        if (next_tile && hasHouseWall(next_tile)) {
                                            floor_wall_queue.push(next);
                                        }
                                    }
                                }
                            }
                        }
                        
                        // Find interior
                        std::set<Position> floor_visited_interior;
                        std::queue<Position> floor_interior_queue;
                        
                        // Try to find a good interior starting point
                        Position floor_fill_start;
                        bool found_interior_start = false;
                        
                        // First try to match with a stair position from the previous floor
                        for (const Position& interior_pos : house_floors[processed_floor].interior_positions) {
                            Position check_pos(interior_pos.x, interior_pos.y, check_floor);
                            Tile* tile = editor.map.getTile(check_pos);
                            
                            if (tile && !hasHouseWall(tile)) {
                                // If there's a stair here, this is an ideal starting point
                                if (hasStairsOrLadder(tile)) {
                                    floor_fill_start = check_pos;
                                    found_interior_start = true;
                                    break;
                                }
                                
                                // Otherwise remember this as a potential start
                                if (!found_interior_start) {
                                    floor_fill_start = check_pos;
                                    found_interior_start = true;
                                }
                            }
                        }
                        
                        // If we didn't find a good start point, use same x,y as original start
                        if (!found_interior_start) {
                            floor_fill_start = Position(start_map_x, start_map_y, check_floor);
                        }
                        
                        Tile* start_floor_tile = editor.map.getTile(floor_fill_start);
                        if (start_floor_tile && !hasHouseWall(start_floor_tile)) {
                            floor_interior_queue.push(floor_fill_start);
                            
                            // Similar interior flood fill as before...
                            while (!floor_interior_queue.empty()) {
                                Position current = floor_interior_queue.front();
                                floor_interior_queue.pop();
                                
                                if (current.x < min_x || current.x > max_x || 
                                    current.y < min_y || current.y > max_y ||
                                    floor_visited_interior.count(current) > 0 ||
                                    floor_data.wall_positions.count(current) > 0) {
                                    continue;
                                }
                                
                                floor_visited_interior.insert(current);
                                
                                // Cast rays to check if inside
                                int wall_hits = 0;
                                // Define directions array
                                const int directions[8][2] = {
                                    {1, 0},   // right
                                    {-1, 0},  // left
                                    {0, 1},   // down
                                    {0, -1},  // up
                                    {1, 1},   // down-right
                                    {-1, -1}, // up-left
                                    {1, -1},  // up-right
                                    {-1, 1}   // down-left
                                };
                                
                                for (int d = 0; d < 8; d++) {
                                    bool found_wall = false;
                                    int max_ray_dist = 2 * std::max(max_x - min_x, max_y - min_y);
                                    
                                    for (int dist = 1; dist <= max_ray_dist; dist++) {
                                        int ray_x = current.x + (directions[d][0] * dist);
                                        int ray_y = current.y + (directions[d][1] * dist);
                                        
                                        if (ray_x < min_x - 5 || ray_x > max_x + 5 || ray_y < min_y - 5 || ray_y > max_y + 5) {
                                            break;
                                        }
                                        
                                        Position ray_pos(ray_x, ray_y, current.z);
                                        if (floor_data.wall_positions.count(ray_pos) > 0) {
                                            found_wall = true;
                                            break;
                                        }
                                    }
                                    
                                    if (found_wall) {
                                        wall_hits++;
                                    }
                                }
                                
                                // More permissive check - if we hit walls in at least 5 of 8 directions for diagonal rays
                                // or at least 2 of 4 cardinal directions, consider it inside
                                if (wall_hits >= 5 || (
                                    (floor_data.wall_positions.count(Position(current.x + 1, current.y, current.z)) > 0 ? 1 : 0) +
                                    (floor_data.wall_positions.count(Position(current.x - 1, current.y, current.z)) > 0 ? 1 : 0) +
                                    (floor_data.wall_positions.count(Position(current.x, current.y + 1, current.z)) > 0 ? 1 : 0) +
                                    (floor_data.wall_positions.count(Position(current.x, current.y - 1, current.z)) > 0 ? 1 : 0) >= 2
                                )) {
                                    floor_data.interior_positions.insert(current);
                                    
                                    // Define directions array for neighbors - use only cardinal directions for expansion
                                    const int fillDirections[4][2] = {
                                        {1, 0},   // right
                                        {-1, 0},  // left
                                        {0, 1},   // down
                                        {0, -1}   // up
                                    };
                                    
                                    for (int d = 0; d < 4; d++) {
                                        Position next(current.x + fillDirections[d][0], 
                                                     current.y + fillDirections[d][1], 
                                                     current.z);
                                        
                                        // Only queue if we haven't visited and it's not a wall
                                        if (floor_visited_interior.count(next) == 0 && 
                                            floor_data.wall_positions.count(next) == 0) {
                                            floor_interior_queue.push(next);
                                        }
                                    }
                                }
                            }
                        }
                        break; // Found a connection, no need to check more walls
                    }
                }
            }
            
            if (floor_connected) {
                // Once we connect a floor, check for floors above and below that one as well
                if (check_floor - 1 >= 0 && processed_floors.count(check_floor - 1) == 0) 
                    floors_to_check.push(check_floor - 1);
                if (check_floor + 1 < MAP_LAYERS && processed_floors.count(check_floor + 1) == 0) 
                    floors_to_check.push(check_floor + 1);
                break;
            }
        }
    }
    
    // Step 7: Create the house
    std::set<Position> all_house_tiles;
    int total_floors = 0;
    int total_tiles = 0;
    Position exit_position(0, 0, 0);

    // Add a second pass of aggressive fill for enclosed rooms
    for (auto& [floor_z, floor_data] : house_floors) {
        if (!floor_data.wall_positions.empty()) {
            // Find min/max bounds for this floor
            int local_min_x = std::numeric_limits<int>::max();
            int local_min_y = std::numeric_limits<int>::max();
            int local_max_x = std::numeric_limits<int>::min();
            int local_max_y = std::numeric_limits<int>::min();
            
            for (const Position& pos : floor_data.wall_positions) {
                local_min_x = std::min(local_min_x, pos.x);
                local_min_y = std::min(local_min_y, pos.y);
                local_max_x = std::max(local_max_x, pos.x);
                local_max_y = std::max(local_max_y, pos.y);
            }
            
            // Expand bounds slightly inward for safety
            local_min_x = local_min_x + 1;
            local_min_y = local_min_y + 1;
            local_max_x = local_max_x - 1;
            local_max_y = local_max_y - 1;
            
            // Create a map of enclosed cells
            std::vector<std::vector<bool>> enclosed_map(local_max_y - local_min_y + 3, 
                                                      std::vector<bool>(local_max_x - local_min_x + 3, false));
            
            // Mark all wall positions on the map
            for (const Position& wall : floor_data.wall_positions) {
                int map_x = wall.x - local_min_x + 1;
                int map_y = wall.y - local_min_y + 1;
                
                if (map_x >= 0 && map_y >= 0 && 
                    map_x < enclosed_map[0].size() && map_y < enclosed_map.size()) {
                    enclosed_map[map_y][map_x] = true;
                }
            }
            
            // Do a simple flood fill from outside to mark external cells
            std::vector<std::vector<bool>> external(enclosed_map.size(), 
                                                  std::vector<bool>(enclosed_map[0].size(), false));
            std::queue<std::pair<int, int>> fill_queue;
            
            // Add border points
            for (int y = 0; y < enclosed_map.size(); y++) {
                fill_queue.push({0, y});
                fill_queue.push({enclosed_map[0].size() - 1, y});
            }
            for (int x = 0; x < enclosed_map[0].size(); x++) {
                fill_queue.push({x, 0});
                fill_queue.push({x, enclosed_map.size() - 1});
            }
            
            while (!fill_queue.empty()) {
                auto [x, y] = fill_queue.front();
                fill_queue.pop();
                
                if (x < 0 || y < 0 || x >= enclosed_map[0].size() || y >= enclosed_map.size() ||
                    external[y][x] || enclosed_map[y][x]) {
                    continue;
                }
                
                external[y][x] = true;
                
                fill_queue.push({x+1, y});
                fill_queue.push({x-1, y});
                fill_queue.push({x, y+1});
                fill_queue.push({x, y-1});
            }
            
            // Any cell not marked as external and not a wall is internal
            for (int y = 1; y < enclosed_map.size()-1; y++) {
                for (int x = 1; x < enclosed_map[0].size()-1; x++) {
                    if (!enclosed_map[y][x] && !external[y][x]) {
                        Position interior_pos(x + local_min_x - 1, y + local_min_y - 1, floor_z);
                        floor_data.interior_positions.insert(interior_pos);
                    }
                }
            }
        }
    }
    
    // Now assemble all house tiles
    for (const auto& [floor_z, floor_data] : house_floors) {
        if (!floor_data.wall_positions.empty() || !floor_data.interior_positions.empty()) {
            total_floors++;
            
            // Add walls
            all_house_tiles.insert(floor_data.wall_positions.begin(), floor_data.wall_positions.end());
            total_tiles += floor_data.wall_positions.size();
            
            // Add interior
            all_house_tiles.insert(floor_data.interior_positions.begin(), floor_data.interior_positions.end());
            total_tiles += floor_data.interior_positions.size();
            
            // Store exit if found
            if (floor_data.has_exit) {
                exit_position = floor_data.exit_pos;
            }
        }
    }
    
    if (all_house_tiles.empty() || total_tiles < 4) {
        g_gui.PopupDialog("Error", "Could not detect any valid house area. Please ensure you clicked inside a house with proper walls.", wxOK);
        return;
    }

    // Create house dialog
    wxDialog* dialog = new wxDialog(this, wxID_ANY, "Create House", wxDefaultPosition, wxSize(300, 200));
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    // House name
    wxBoxSizer* name_sizer = new wxBoxSizer(wxHORIZONTAL);
    name_sizer->Add(new wxStaticText(dialog, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    wxTextCtrl* name_field = new wxTextCtrl(dialog, wxID_ANY, "New House");
    name_sizer->Add(name_field, 1, wxEXPAND);
    topsizer->Add(name_sizer, 0, wxEXPAND | wxALL, 5);

    // Town selection
    wxBoxSizer* town_sizer = new wxBoxSizer(wxHORIZONTAL);
    town_sizer->Add(new wxStaticText(dialog, wxID_ANY, "Town:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    wxChoice* town_field = new wxChoice(dialog, wxID_ANY);

    const Towns& towns = editor.map.towns;
    if (towns.count() > 0) {
        for (TownMap::const_iterator town_iter = towns.begin(); 
             town_iter != towns.end(); ++town_iter) {
            town_field->Append(wxstr(town_iter->second->getName()), 
                (void*)(intptr_t)town_iter->second->getID());
        }
        town_field->SetSelection(0);
    } else {
        town_field->Append("No towns available");
        town_field->Enable(false);
    }

    town_sizer->Add(town_field, 1, wxEXPAND);
    topsizer->Add(town_sizer, 0, wxEXPAND | wxALL, 5);

    // Rent
    wxBoxSizer* rent_sizer = new wxBoxSizer(wxHORIZONTAL);
    rent_sizer->Add(new wxStaticText(dialog, wxID_ANY, "Rent:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    wxSpinCtrl* rent_field = new wxSpinCtrl(dialog, wxID_ANY, "0", 
        wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0);
    rent_sizer->Add(rent_field, 1, wxEXPAND);
    topsizer->Add(rent_sizer, 0, wxEXPAND | wxALL, 5);

    // Guildhall checkbox
    wxCheckBox* guildhall_field = new wxCheckBox(dialog, wxID_ANY, "Guildhall");
    topsizer->Add(guildhall_field, 0, wxEXPAND | wxALL, 5);

    // Size and floor info display
    wxBoxSizer* info_sizer = new wxBoxSizer(wxHORIZONTAL);
    info_sizer->Add(new wxStaticText(dialog, wxID_ANY, 
        wxString::Format("Size: %d tiles on %d floors", total_tiles, total_floors)), 
        1, wxEXPAND);
    topsizer->Add(info_sizer, 0, wxEXPAND | wxALL, 5);

    // Dialog buttons
    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    buttons->AddButton(new wxButton(dialog, wxID_OK, "OK"));
    buttons->AddButton(new wxButton(dialog, wxID_CANCEL, "Cancel"));
    buttons->Realize();
    topsizer->Add(buttons, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(topsizer);
    topsizer->Fit(dialog);

    if (dialog->ShowModal() == wxID_OK) {
        // Create new house
        House* house = new House(editor.map);
        house->name = nstr(name_field->GetValue());
        
        if (towns.count() > 0) {
            int sel = town_field->GetSelection();
            if (sel != wxNOT_FOUND) {
                house->townid = (uint32_t)(intptr_t)town_field->GetClientData(sel);
            }
        }
        
        house->rent = rent_field->GetValue();
        house->guildhall = guildhall_field->GetValue();

        // Add house to map
        house->setID(editor.map.houses.getEmptyID());
        editor.map.houses.addHouse(house);

        // Add all tiles to house
        for (const Position& pos : all_house_tiles) {
            Tile* tile = editor.map.getTile(pos);
            if (!tile) {
                tile = editor.map.createTile(pos.x, pos.y, pos.z);
            }
            house->addTile(tile);
        }

        // Set exit position
        if (exit_position != Position(0, 0, 0)) {
            house->setExit(exit_position);
        }

        g_gui.PopupDialog("Success", 
            wxString::Format("Created house '%s' with %d tiles on %d floors.", 
            wxstr(house->name).c_str(), total_tiles, total_floors), wxOK);

        editor.map.doChange();
    }

    dialog->Destroy();
    Refresh();
}

// Helper function to check if a tile has house walls or blocking items
bool MapCanvas::hasHouseWall(Tile* tile) {
    if (!tile) return false;
				
    for (Item* item : tile->items) {
        if (!item) continue;
        
        // Direct checks for brush properties - most reliable
        if (item->getWallBrush() || item->isDoor() || item->isBrushDoor()) {
            return true;
        }
        
        // Name-based identification for common boundary elements
        const std::string& name = item->getName();
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        // Only use specific wall-related terms
        if (lowerName.find("wall") != std::string::npos ||
            lowerName.find("door") != std::string::npos ||
            lowerName.find("window") != std::string::npos ||
            lowerName.find("fence") != std::string::npos ||
            lowerName.find("gate") != std::string::npos ||
            lowerName.find("rail") != std::string::npos ||
			lowerName.find("pillar") != std::string::npos ||
            lowerName.find("archway") != std::string::npos) {
            return true;
        }
    }
    
    return false;
}


// Check if tile has a door
bool MapCanvas::hasDoor(Tile* tile) {
    if (!tile) return false;
    
    for (Item* item : tile->items) {
        if (!item) continue;
        
        if (item->isDoor() || item->isBrushDoor()) {
            return true;
        }
        
        const std::string& name = item->getName();
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find("door") != std::string::npos ||
            lowerName.find("gate") != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// Check if tile has stairs or a ladder
bool MapCanvas::hasStairsOrLadder(Tile* tile) {
    // Check if the tile is null
    if (!tile) return false;
    
    // First pass: check for obviously visible stairs/ladders
    for (Item* item : tile->items) {
        if (!item) continue;
        
        // Check special types of stairs and ladders
        if (item->isStairs() || item->isLadder()) {
            return true;
        }
        
        // Check names for various stair and ladder types
        std::string lowerName = item->getName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find("stair") != std::string::npos ||
            lowerName.find("ladder") != std::string::npos ||
            lowerName.find("ramp") != std::string::npos ||
            lowerName.find("elevator") != std::string::npos ||
            lowerName.find("escalator") != std::string::npos) {
            return true;
        }
    }
    
    // Second pass: check for railings that might be on top of stairs
    bool has_railing = false;
    for (Item* item : tile->items) {
        if (!item) continue;
        
        std::string lowerName = item->getName();
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        if (lowerName.find("rail") != std::string::npos) {
            has_railing = true;
            break;
        }
    }
    
    // If we found a railing, we need to check the floor below for stairs
    if (has_railing && tile->getPosition().z > 0) {
        Position pos_below(tile->getPosition().x, tile->getPosition().y, tile->getPosition().z - 1);
        Tile* tile_below = editor.map.getTile(pos_below);
        
        if (tile_below) {
            for (Item* item : tile_below->items) {
                if (!item) continue;
                
                if (item->isStairs() || item->isLadder()) {
                    return true;
                }
                
                std::string lowerName = item->getName();
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                
                if (lowerName.find("stair") != std::string::npos ||
                    lowerName.find("ladder") != std::string::npos ||
                    lowerName.find("ramp") != std::string::npos) {
                    return true;
                }
            }
        }
    }
    
    // Check for possible floor transition items underneath
    if (tile->ground && (tile->ground->isStairs() || tile->ground->isLadder())) {
        return true;
    }
    
    return false;
}

void MapCanvas::OnOpenRevScript(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile) {
		return;
	}

	ItemVector selected_items = tile->getSelectedItems();
	if (selected_items.empty()) {
		return;
	}

	Item* item = selected_items.back(); // Get the top selected item
	if (!item) {
		return;
	}

	// Check if the RevScript manager is loaded
	if (!g_gui.revscript_manager.isLoaded()) {
		g_gui.PopupDialog("Script Error", "No OTS data directory has been loaded. Please load a map first or configure the OTS data directory in preferences.", wxOK);
		return;
	}

	// Get action ID, unique ID, and item ID from the item
	uint16_t action_id = item->getActionID();
	uint16_t unique_id = item->getUniqueID();
	uint16_t item_id = item->getID();

	std::vector<RevScriptEntry> entries;

	// Search for scripts by action ID
	if (action_id > 0) {
		auto aid_entries = g_gui.revscript_manager.findByActionID(action_id);
		entries.insert(entries.end(), aid_entries.begin(), aid_entries.end());
	}

	// Search for scripts by unique ID
	if (unique_id > 0) {
		auto uid_entries = g_gui.revscript_manager.findByUniqueID(unique_id);
		entries.insert(entries.end(), uid_entries.begin(), uid_entries.end());
	}

	// Search for scripts by item ID (includes XML-based actions and movements)
	auto id_entries = g_gui.revscript_manager.findByItemID(item_id);
	entries.insert(entries.end(), id_entries.begin(), id_entries.end());

	if (entries.empty()) {
		wxString message = "No script entries found for this item.";
		message += wxString::Format("\nChecked: Item ID %d", item_id);
		if (action_id > 0) {
			message += wxString::Format(", Action ID %d", action_id);
		}
		if (unique_id > 0) {
			message += wxString::Format(", Unique ID %d", unique_id);
		}
		message += "\nSearched: RevScript files, Actions XML, and Movements XML";
		g_gui.PopupDialog("Script Not Found", message, wxOK);
		return;
	}

	// If only one entry found, open it directly
	if (entries.size() == 1) {
		const RevScriptEntry& entry = entries[0];
		if (!g_gui.revscript_manager.openRevScript(entry)) {
			g_gui.PopupDialog("Error", "Failed to open the script file. Make sure you have a text editor installed.", wxOK);
		}
		return;
	}

	// Multiple entries found, show a selection dialog
	wxArrayString choices;
	for (const auto& entry : entries) {
		wxString choice;
		if (entry.script_type == "revscript") {
			choice = wxString::Format("[RevScript] %s:%d - %s (%s: %d)",
				wxstr(entry.filename), entry.line_number, 
				wxstr(entry.function_name.empty() ? "global" : entry.function_name),
				wxstr(entry.id_type), entry.id_value);
		} else if (entry.script_type == "action") {
			choice = wxString::Format("[Action] %s (%s: %d)",
				wxstr(entry.filename),
				wxstr(entry.id_type), entry.id_value);
		} else if (entry.script_type == "movement") {
			choice = wxString::Format("[Movement] %s (%s: %d)",
				wxstr(entry.filename),
				wxstr(entry.id_type), entry.id_value);
		} else {
			choice = wxString::Format("[%s] %s (%s: %d)",
				wxstr(entry.script_type),
				wxstr(entry.filename),
				wxstr(entry.id_type), entry.id_value);
		}
		choices.Add(choice);
	}

	int selection = wxGetSingleChoiceIndex(
		wxString::Format("Multiple script entries found for this item (Item ID: %d). Select one to open:", item_id),
		"Select Script",
		choices,
		g_gui.root
	);

	if (selection != -1) {
		const RevScriptEntry& entry = entries[selection];
		if (!g_gui.revscript_manager.openRevScript(entry)) {
			g_gui.PopupDialog("Error", "Failed to open the script file. Make sure you have a text editor installed.", wxOK);
		}
	}
}

void MapCanvas::OnOpenNPCXML(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile || !tile->creature) {
		return;
	}

	Creature* creature = tile->creature;
	if (!creature->isNpc()) {
		return;
	}

	// Check if the NPC manager is loaded
	if (!g_gui.npc_manager.isLoaded()) {
		g_gui.PopupDialog("NPC Error", "No OTS data directory has been loaded. Please load a map first or configure the OTS data directory in preferences.", wxOK);
		return;
	}

	// Find the NPC entry by name
	NPCEntry* npc_entry = g_gui.npc_manager.findByName(creature->getName());
	if (!npc_entry) {
		g_gui.PopupDialog("NPC Not Found", 
			wxString::Format("No NPC XML file found for '%s'.\nMake sure the NPC data directory is correctly configured in preferences.", 
			wxstr(creature->getName())), wxOK);
		return;
	}

	// Open the XML file
	if (!g_gui.npc_manager.openNPCXML(*npc_entry)) {
		g_gui.PopupDialog("Error", "Failed to open the NPC XML file. Make sure you have a text editor installed.", wxOK);
	}
}

void MapCanvas::OnOpenNPCScript(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile || !tile->creature) {
		return;
	}

	Creature* creature = tile->creature;
	if (!creature->isNpc()) {
		return;
	}

	// Check if the NPC manager is loaded
	if (!g_gui.npc_manager.isLoaded()) {
		g_gui.PopupDialog("NPC Error", "No OTS data directory has been loaded. Please load a map first or configure the OTS data directory in preferences.", wxOK);
		return;
	}

	// Find the NPC entry by name
	NPCEntry* npc_entry = g_gui.npc_manager.findByName(creature->getName());
	if (!npc_entry) {
		g_gui.PopupDialog("NPC Not Found", 
			wxString::Format("No NPC XML file found for '%s'.\nMake sure the NPC data directory is correctly configured in preferences.", 
			wxstr(creature->getName())), wxOK);
		return;
	}

	// Check if the NPC has a script
	if (!npc_entry->hasScript || npc_entry->script_path.empty()) {
		g_gui.PopupDialog("No Script", 
			wxString::Format("NPC '%s' does not have a script file configured.", 
			wxstr(creature->getName())), wxOK);
		return;
	}

	// Open the script file
	if (!g_gui.npc_manager.openNPCScript(*npc_entry)) {
		g_gui.PopupDialog("Error", "Failed to open the NPC script file. Make sure the script file exists and you have a text editor installed.", wxOK);
	}
}

void MapCanvas::OnOpenMonsterXML(wxCommandEvent& WXUNUSED(event)) {
	if (editor.selection.size() != 1) {
		return;
	}

	Tile* tile = editor.selection.getSelectedTile();
	if (!tile || !tile->creature) {
		return;
	}

	Creature* creature = tile->creature;
	if (creature->isNpc()) {
		return; // This is for monsters only
	}

	// Check if the Monster manager is loaded
	if (!g_gui.monster_manager.isLoaded()) {
		g_gui.PopupDialog("Monster Error", "No OTS data directory has been loaded. Please load a map first or configure the OTS data directory in preferences.", wxOK);
		return;
	}

	// Find the Monster entry by name
	MonsterEntry* monster_entry = g_gui.monster_manager.findByName(creature->getName());
	if (!monster_entry) {
		g_gui.PopupDialog("Monster Not Found", 
			wxString::Format("No Monster XML file found for '%s'.\nMake sure the Monster data directory is correctly configured in preferences.", 
			wxstr(creature->getName())), wxOK);
		return;
	}

	// Open the XML file
	if (!g_gui.monster_manager.openMonsterXML(*monster_entry)) {
		g_gui.PopupDialog("Error", "Failed to open the Monster XML file. Make sure you have a text editor installed.", wxOK);
	}
}
