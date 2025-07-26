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
CURRENT SITUATION:
1. Minimap refresh issues:
   - Updates when changing floors (minimap_window.cpp:93-101)
   - Does not update properly after paste operations (copybuffer.cpp:232-257)
   - No caching mechanism for faster reloading
   - No binary file storage for minimap data

IMPROVEMENT TASKS:
1. Paste Operation Minimap Update:
   - Add minimap update calls in copybuffer.cpp after paste operations
   - Track modified positions during paste for efficient updates
   - Reference: copybuffer.cpp:232-257

2. Minimap Caching:
   - Implement block-based caching system (already started in MinimapBlock struct)
   - Add floor-specific caching
   - Optimize block size for better performance (currently 256)
   - Reference: minimap_window.cpp:53-55

3. Binary File Storage:
   - Create binary file format for minimap data
   - Store color information and floor data
   - Implement save/load functions
   - Cache binary data for quick loading

4. Drawing Optimization:
   - Batch rendering operations
   - Implement dirty region tracking
   - Use hardware acceleration where possible
   - Reference: minimap_window.cpp:290-339

RELEVANT CODE SECTIONS:
- Minimap window core: minimap_window.cpp:82-177
- Copy buffer paste: copybuffer.cpp:232-257
- Selection handling: selection.cpp:1-365
*/
#include "main.h"

#include "graphics.h"
#include "editor.h"
#include "map.h"

#include "gui.h"
#include "map_display.h"
#include "minimap_window.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/dir.h>
#include <wx/ffile.h>

BEGIN_EVENT_TABLE(MinimapWindow, wxPanel)
	EVT_PAINT(MinimapWindow::OnPaint)
	EVT_ERASE_BACKGROUND(MinimapWindow::OnEraseBackground)
	EVT_LEFT_DOWN(MinimapWindow::OnMouseClick)
	EVT_KEY_DOWN(MinimapWindow::OnKey)
	EVT_SIZE(MinimapWindow::OnSize)
	EVT_CLOSE(MinimapWindow::OnClose)
	EVT_TIMER(ID_MINIMAP_UPDATE, MinimapWindow::OnDelayedUpdate)
	EVT_TIMER(ID_RESIZE_TIMER, MinimapWindow::OnResizeTimer)

END_EVENT_TABLE()

MinimapWindow::MinimapWindow(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(205, 130), wxFULL_REPAINT_ON_RESIZE),
	update_timer(this),
	thread_running(false),
	needs_update(true),
	last_center_x(0),
	last_center_y(0),
	last_floor(0),
	last_start_x(0),
	last_start_y(0),
	is_resizing(false),
	empty_tile_atlas_initialized(false)
{
	for (int i = 0; i < 256; ++i) {
		pens[i] = newd wxPen(wxColor(minimap_color[i].red, minimap_color[i].green, minimap_color[i].blue));
	}
	
	// Initialize the update timer
	update_timer.SetOwner(this, ID_MINIMAP_UPDATE);
	
	// Initialize the resize timer
	resize_timer.SetOwner(this, ID_RESIZE_TIMER);
	
	// Floor initialization fix
	if (g_gui.IsEditorOpen()) {
		minimap_floor = g_gui.GetCurrentFloor();
	} else {
		minimap_floor = 7; // Safe default
	}
	
	// Minimap waypoint UI
	minimap_waypoint_combo = new wxComboBox(this, wxID_ANY, wxEmptyString, wxPoint(-100, -100), wxSize(120, 22));
	add_minimap_waypoint_btn = new wxButton(this, wxID_ANY, "+", wxPoint(-100, -100), wxSize(28, 22));
	save_minimap_waypoints_btn = new wxButton(this, wxID_ANY, "Save", wxPoint(-100, -100), wxSize(48, 22));
	load_minimap_waypoints_btn = new wxButton(this, wxID_ANY, "Load", wxPoint(-100, -100), wxSize(48, 22));
	minimap_waypoint_combo->Bind(wxEVT_COMBOBOX, &MinimapWindow::OnMinimapWaypointSelected, this);
	add_minimap_waypoint_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnAddMinimapWaypoint, this);
	save_minimap_waypoints_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnSaveMinimapWaypoints, this);
	load_minimap_waypoints_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnLoadMinimapWaypoints, this);
	UpdateMinimapWaypointCombo();
	
	// In constructor, create save_cache_checkbox and bind event
	save_cache_checkbox = new wxCheckBox(this, wxID_ANY, "Save cache to disk", wxPoint(-100, -100), wxSize(130, 22));
	save_cache_checkbox->SetValue(false);
	save_cache_checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
		save_cache_to_disk = save_cache_checkbox->GetValue();
	});
	
	StartRenderThread();
	
	// Schedule initial loading after a short delay
	update_timer.Start(100, true); // Start a one-shot timer for 100ms
}

MinimapWindow::~MinimapWindow() {
	StopRenderThread();
	for (int i = 0; i < 256; ++i) {
		delete pens[i];
	}
}

void MinimapWindow::StartRenderThread() {
	thread_running = true;
	render_thread = std::thread(&MinimapWindow::RenderThreadFunction, this);
}

void MinimapWindow::StopRenderThread() {
	thread_running = false;
	if(render_thread.joinable()) {
		render_thread.join();
	}
}

void MinimapWindow::RenderThreadFunction() {
	while(thread_running) {
		if(needs_update && g_gui.IsEditorOpen()) {
			Editor& editor = *g_gui.GetCurrentEditor();
			MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
			
			int center_x, center_y;
			canvas->GetScreenCenter(&center_x, &center_y);
			int floor = g_gui.GetCurrentFloor();
			
			// Force update if floor changed
			if(floor != last_floor) {
				// Clear the buffer when floor changes
				std::lock_guard<std::mutex> lock(buffer_mutex);
				buffer = wxBitmap(GetSize().GetWidth(), GetSize().GetHeight());
				
				// Clear block cache
				std::lock_guard<std::mutex> blockLock(m_mutex);
				m_blocks.clear();
			}
			
			// Always update if floor changed or position changed
			if(center_x != last_center_x || 
			   center_y != last_center_y || 
			   floor != last_floor) {
				
				int window_width = GetSize().GetWidth();
				int window_height = GetSize().GetHeight();
				
				// Create temporary bitmap
				wxBitmap temp_buffer(window_width, window_height);
				wxMemoryDC dc(temp_buffer);
				
				dc.SetBackground(*wxBLACK_BRUSH);
				dc.Clear();
				
				// CRITICAL FIX: Prevent division by zero in minimap rendering
				if (window_width <= 0 || window_height <= 0) {
					char debug_msg[256];
					sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! Minimap window dimensions are zero: width=%d, height=%d\n", 
						window_width, window_height);
					OutputDebugStringA(debug_msg);
					return; // Exit early to prevent division by zero
				}
				
				int start_x = center_x - window_width / 2;
				int start_y = center_y - window_height / 2;
				
				// Batch drawing by color
				std::vector<std::vector<wxPoint>> colorPoints(256);
				
				for(int y = 0; y < window_height; ++y) {
					for(int x = 0; x < window_width; ++x) {
						int map_x = start_x + x;
						int map_y = start_y + y;
						
						if(map_x >= 0 && map_y >= 0 && 
						   map_x < editor.map.getWidth() && 
						   map_y < editor.map.getHeight()) {
							
							Tile* tile = editor.map.getTile(map_x, map_y, floor);
							if(tile) {
								uint8_t color = tile->getMiniMapColor();
								if(color) {
									colorPoints[color].push_back(wxPoint(x, y));
								}
							}
						}
					}
				}
				
				// Draw points by color
				for(int color = 0; color < 256; ++color) {
					if(!colorPoints[color].empty()) {
						dc.SetPen(*pens[color]);
						for(const wxPoint& pt : colorPoints[color]) {
							dc.DrawPoint(pt.x, pt.y);
						}
					}
				}
				
				// Update buffer safely
				{
					std::lock_guard<std::mutex> lock(buffer_mutex);
					buffer = temp_buffer;
				}
				
				// Store current state
				last_center_x = center_x;
				last_center_y = center_y;
				last_floor = floor;
				
				// Request refresh of the window
				wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
				evt.SetId(ID_MINIMAP_UPDATE);
				wxPostEvent(this, evt);
			}
			
			needs_update = false;
		}
		
		// Sleep to prevent excessive CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	// Get the new size
	wxSize newSize = event.GetSize();
	
	// If we're already at this size, skip
	if (newSize == GetSize()) {
		event.Skip();
		return;
	}
	
	// Mark that we're resizing
	is_resizing = true;
	
	// Stop any previous resize timer
	if (resize_timer.IsRunning()) {
		resize_timer.Stop();
	}
	
	// Create a new buffer at the new size
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer = wxBitmap(newSize.GetWidth(), newSize.GetHeight());
	}
	
	// Clear the block cache since we're changing size
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_blocks.clear();
	}
	
	// Start the resize timer (will fire when resize is complete)
	resize_timer.Start(50, true); // Reduced to 50ms for faster response
	
	// Skip the event to allow default handling
	event.Skip();
}

void MinimapWindow::OnClose(wxCloseEvent& event) {
	// Hide the window instead of destroying it
	// This allows the minimap to be reopened in the same position
	if (wxWindow::GetParent()) {
		g_gui.HideMinimap();
		event.Veto(); // Prevent the window from being destroyed
	} else {
		event.Skip(); // Allow default handling if no parent
	}
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	if (g_gui.IsEditorOpen()) {
		// If editor is already open, load the minimap
		if (event.GetId() == ID_MINIMAP_UPDATE) {
			InitialLoad();
		}
	}
	
	needs_update = true;
}

void MinimapWindow::DelayedUpdate() {
	update_timer.Start(100, true);  // 100ms single-shot timer
}

void MinimapWindow::OnResizeTimer(wxTimerEvent& event) {
	// Resizing has stopped
	is_resizing = false;
	
	// Force a complete redraw with the new size
	needs_update = true;
	
	// Request a full refresh
	Refresh();
	
	// Trigger an immediate update
	if (g_gui.IsEditorOpen()) {
		Editor& editor = *g_gui.GetCurrentEditor();
		MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
		
		int center_x, center_y;
		canvas->GetScreenCenter(&center_x, &center_y);
		
		// Force update of the render thread
		last_center_x = center_x;
		last_center_y = center_y;
		last_floor = g_gui.GetCurrentFloor();
	}
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxBufferedPaintDC dc(this);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	if (!g_gui.IsEditorOpen()) return;
	
	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	int floor = minimap_floor;
	
	// Store current state
	last_center_x = centerX;
	last_center_y = centerY;
	last_floor = floor;

	// Get window dimensions
	int windowWidth = wxPanel::GetSize().GetWidth();
	int windowHeight = wxPanel::GetSize().GetHeight();
	
	// CRITICAL FIX: Prevent division by zero in minimap OnPaint
	if (windowWidth <= 0 || windowHeight <= 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! Minimap OnPaint dimensions are zero: width=%d, height=%d\n", 
			windowWidth, windowHeight);
		OutputDebugStringA(debug_msg);
		return; // Exit early to prevent division by zero
	}
	
	// Draw header with map info
	int headerHeight = 30;
	wxRect headerRect(0, 0, windowWidth, headerHeight);
	dc.SetBrush(wxBrush(wxColour(40, 40, 40)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(headerRect);
	
	// Draw map info text
	dc.SetTextForeground(wxColour(220, 220, 220));
	wxFont font = dc.GetFont();
	font.SetPointSize(9);
	dc.SetFont(font);
	
	wxString mapInfo = wxString::Format("Floor: %d | Position: %d,%d", 
		floor, centerX, centerY);
	dc.DrawText(mapInfo, 10, 8);

	// Draw separator after position
	int textWidth, textHeight;
	dc.GetTextExtent(mapInfo, &textWidth, &textHeight);
	int sepX = 10 + textWidth + 10;
	dc.SetTextForeground(wxColour(120, 120, 120));
	dc.DrawText("|", sepX, 8);
	int btnStartX = sepX + 15;

	// Draw header buttons in order: DOWN, UP, CACHE, then floor number
	int btnW = 28, btnH = 22, margin = 4;
	int y = (headerHeight - btnH) / 2;
	// Down button
	btn_down = wxRect(btnStartX, y, btnW, btnH);
	dc.SetBrush(wxBrush(wxColour(80, 80, 200)));
	dc.SetPen(*wxBLACK_PEN);
	dc.DrawRectangle(btn_down);
	dc.SetTextForeground(*wxWHITE);
	dc.DrawText("UP", btn_down.x + 7, btn_down.y - 2);
	// Up button
	btn_up = wxRect(btnStartX + btnW + margin, y, btnW, btnH);
	dc.SetBrush(wxBrush(wxColour(80, 80, 200)));
	dc.DrawRectangle(btn_up);
	dc.SetTextForeground(*wxWHITE);
	dc.DrawText("DOWN", btn_up.x + 7, btn_up.y - 2);
	// Cache button
	btn_cache = wxRect(btnStartX + 2 * (btnW + margin), y, btnW + 10, btnH);
	dc.SetBrush(wxBrush(wxColour(60, 180, 60)));
	dc.DrawRectangle(btn_cache);
	dc.SetTextForeground(*wxWHITE);
	dc.DrawText("Cache", btn_cache.x + 2, btn_cache.y + 2);
	// Floor number display
	wxString floorStr = wxString::Format("Floor: %d", minimap_floor);
	dc.SetTextForeground(wxColour(220, 220, 220));
	int floorTextX = btn_cache.x + btn_cache.width + margin;
	dc.DrawText(floorStr, floorTextX, 8);

	// Draw separator before waypoint menu
	int floorTextWidth, floorTextHeight;
	dc.GetTextExtent(floorStr, &floorTextWidth, &floorTextHeight);
	int sep2X = floorTextX + floorTextWidth + 10;
	dc.SetTextForeground(wxColour(120, 120, 120));
	dc.DrawText("|", sep2X, 8);

	// Minimap waypoint combo and add button
	int comboX = sep2X + 15;
	int comboY = y;
	int comboW = 120;
	int comboH = btnH;
	int addBtnW = 28;
	if (minimap_waypoint_combo && add_minimap_waypoint_btn) {
		minimap_waypoint_combo->SetSize(comboX, comboY, comboW, comboH);
		minimap_waypoint_combo->Show();
		add_minimap_waypoint_btn->SetSize(comboX + comboW + margin, comboY, addBtnW, comboH);
		add_minimap_waypoint_btn->Show();
	}
	
	// Draw separator after waypoint menu
	int sep3X = comboX + comboW + addBtnW + 2 * margin;
	dc.SetTextForeground(wxColour(120, 120, 120));
	dc.DrawText("|", sep3X, 8);
	int saveBtnX = sep3X + 15;
	int saveBtnW = 48;
	int loadBtnW = 48;
	if (save_minimap_waypoints_btn && load_minimap_waypoints_btn) {
		save_minimap_waypoints_btn->SetSize(saveBtnX, comboY, saveBtnW, comboH);
		save_minimap_waypoints_btn->Show();
		load_minimap_waypoints_btn->SetSize(saveBtnX + saveBtnW + margin, comboY, loadBtnW, comboH);
		load_minimap_waypoints_btn->Show();
	}
	
	// In OnPaint, after the last separator, place the checkbox before Save/Load buttons
	int sep4X = sep3X + saveBtnW + loadBtnW + 2 * margin;
	dc.SetTextForeground(wxColour(120, 120, 120));
	dc.DrawText("|", sep4X, 8);
	int checkboxX = sep4X + 15;
	if (save_cache_checkbox) {
		save_cache_checkbox->SetSize(checkboxX, comboY, 130, comboH);
		save_cache_checkbox->Show();
	}
	
	// Draw minimap using cached blocks
	int padding = 10;
	int startX = std::max(0, centerX - (windowWidth / 2) - padding);
	int startY = std::max(0, centerY - ((windowHeight - headerHeight) / 2) - padding);
	int endX = std::min(editor.map.getWidth(), startX + windowWidth + padding * 2);
	int endY = std::min(editor.map.getHeight(), startY + (windowHeight - headerHeight) + padding * 2);
	int blockStartX = startX / BLOCK_SIZE;
	int blockEndX = (endX + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int blockStartY = startY / BLOCK_SIZE;
	int blockEndY = (endY + BLOCK_SIZE - 1) / BLOCK_SIZE;
	for (int by = blockStartY; by < blockEndY; ++by) {
		for (int bx = blockStartX; bx < blockEndX; ++bx) {
			BlockKey key{bx, by, floor};
			wxBitmap* bmp = nullptr;
			auto it = block_cache.find(key);
			if (it != block_cache.end()) {
				bmp = &it->second;
			} else if (IsBlockFilled(bx, by, floor)) {
				wxBitmap rendered = RenderBlock(bx, by, floor);
				block_cache[key] = rendered;
				bmp = &block_cache[key];
			}
			if (bmp) {
				int drawX = bx * BLOCK_SIZE - startX;
				int drawY = by * BLOCK_SIZE - startY + headerHeight;
				dc.DrawBitmap(*bmp, drawX, drawY, false);
			}
		}
	}
	
	// Draw center marker
	dc.SetPen(wxPen(wxColour(255, 0, 0), 2));
	int centerDrawX = windowWidth / 2;
	int centerDrawY = (windowHeight - headerHeight) / 2 + headerHeight;
	dc.DrawLine(centerDrawX - 5, centerDrawY, centerDrawX + 5, centerDrawY);
	dc.DrawLine(centerDrawX, centerDrawY - 5, centerDrawX, centerDrawY + 5);
}

void MinimapWindow::HandleHeaderButtonClick(const wxPoint& pt) {
	if (btn_cache.Contains(pt)) {
		StartCacheCurrentFloor();
	} else if (btn_up.Contains(pt)) {
		minimap_floor = std::min(minimap_floor + 1, 15); // Clamp to max floor
		needs_update = true;
		Refresh();
	} else if (btn_down.Contains(pt)) {
		minimap_floor = std::max(minimap_floor - 1, 0); // Clamp to min floor
		needs_update = true;
		Refresh();
	}
}

void MinimapWindow::StartCacheCurrentFloor() {
	wxString msg = wxString::Format("Caching minimap floor %d...", minimap_floor);
	g_gui.CreateLoadBar(msg);
	CacheFilledBlocksForFloor(minimap_floor);
	if (save_cache_to_disk) {
		SaveBlockCacheToDisk(minimap_floor);
	}
	g_gui.DestroyLoadBar();
	needs_update = true;
	Refresh();
}

void MinimapWindow::BatchCacheFloor(int floor) {
	if (!g_gui.IsEditorOpen()) return;
	Editor& editor = *g_gui.GetCurrentEditor();
	int mapWidth = editor.map.getWidth();
	int mapHeight = editor.map.getHeight();
	int totalRows = mapHeight;
	int doneRows = 0;
	
	// For each row, update the minimap cache and progress bar
	for (int y = 0; y < mapHeight; ++y) {
		for (int x = 0; x < mapWidth; ++x) {
			Tile* tile = editor.map.getTile(x, y, floor);
			// Optionally, update cache here if you have a block-based cache
			// For now, just access the tile to simulate caching
			(void)tile;
		}
		doneRows++;
		int percent = int((doneRows / (double)totalRows) * 100.0);
		g_gui.SetLoadDone(percent, wxString::Format("Caching row %d/%d", doneRows, totalRows));
		wxYield(); // Keep UI responsive
	}
	needs_update = true;
	Refresh();
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	wxPoint pt(event.GetX(), event.GetY());
	int headerHeight = 30;
	if (pt.y < headerHeight) {
		HandleHeaderButtonClick(pt);
		return;
	}

	if (!g_gui.IsEditorOpen())
		return;

	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	
	int windowWidth = GetSize().GetWidth();
	int windowHeight = GetSize().GetHeight();
	
	// CRITICAL FIX: Prevent division by zero in minimap click handling
	if (windowWidth <= 0 || windowHeight <= 0) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! Minimap click dimensions are zero: width=%d, height=%d\n", 
			windowWidth, windowHeight);
		OutputDebugStringA(debug_msg);
		return; // Exit early to prevent division by zero
	}
	
	// Calculate the map position clicked
	int clickX = event.GetX();
	int clickY = event.GetY() - headerHeight; // Adjust for header
	
	int mapX = centerX - (windowWidth / 2) + clickX;
	int mapY = centerY - ((windowHeight - headerHeight) / 2) + clickY;
	
	// Only process clicks below the header
	if (event.GetY() > headerHeight) {
		g_gui.SetScreenCenterPosition(Position(mapX, mapY, g_gui.GetCurrentFloor()));
	Refresh();
	g_gui.RefreshView();
	}
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

MinimapWindow::BlockPtr MinimapWindow::getBlock(int x, int y) {
	std::lock_guard<std::mutex> lock(m_mutex);
	uint32_t index = getBlockIndex(x, y);
	
	auto it = m_blocks.find(index);
	if (it == m_blocks.end()) {
		auto block = std::make_shared<MinimapBlock>();
		m_blocks[index] = block;
		return block;
	}
	return it->second;
}

void MinimapWindow::updateBlock(BlockPtr block, int startX, int startY, int floor) {
	Editor& editor = *g_gui.GetCurrentEditor();
	
	// Always update if the block's floor doesn't match current floor
	if (!block->needsUpdate && block->floor != floor) {
		block->needsUpdate = true;
	}
	
	if (!block->needsUpdate) return;
	
	wxBitmap bitmap(BLOCK_SIZE, BLOCK_SIZE);
	wxMemoryDC dc(bitmap);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	// Store the floor this block was rendered for
	block->floor = floor;
	
	// Batch drawing by color like OTClient
	std::vector<std::vector<wxPoint>> colorPoints(256);
	
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			int mapX = startX + x;
			int mapY = startY + y;
			
			Tile* tile = editor.map.getTile(mapX, mapY, floor);
			if (tile) {
				uint8_t color = tile->getMiniMapColor();
				if (color != 255) {  // Not transparent
					colorPoints[color].push_back(wxPoint(x, y));
				}
			}
		}
	}
	
	// Draw all points of same color at once
	for (int color = 0; color < 256; ++color) {
		if (!colorPoints[color].empty()) {
			dc.SetPen(*pens[color]);
			for (const wxPoint& pt : colorPoints[color]) {
				dc.DrawPoint(pt);
			}
		}
	}
	
	block->bitmap = bitmap;
	block->needsUpdate = false;
	block->wasSeen = true;
}

void MinimapWindow::ClearCache() {
	// Simply refresh the window
	Refresh();
}

void MinimapWindow::UpdateDrawnTiles(const PositionVector& positions) {
	// Just refresh the entire minimap - it's faster than checking which tiles need updating
	Refresh();
}

void MinimapWindow::PreCacheEntireMap() {
	// No longer needed - we'll just draw the visible area on demand
	return;
}

void MinimapWindow::InitialLoad() {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	// Clear any existing cache
	std::lock_guard<std::mutex> lock(m_mutex);
	m_blocks.clear();
	
	// Force an immediate refresh
	needs_update = true;
	Refresh();
}

void MinimapWindow::UpdateMinimapWaypointCombo() {
	if (!minimap_waypoint_combo) return;
	minimap_waypoint_combo->Clear();
	for (const auto& wp : minimap_waypoints) {
		minimap_waypoint_combo->Append(wp.name);
	}
	if (selected_minimap_waypoint_idx >= 0 && selected_minimap_waypoint_idx < (int)minimap_waypoints.size()) {
		minimap_waypoint_combo->SetSelection(selected_minimap_waypoint_idx);
	}
}

void MinimapWindow::OnMinimapWaypointSelected(wxCommandEvent& event) {
	int idx = minimap_waypoint_combo->GetSelection();
	if (idx >= 0 && idx < (int)minimap_waypoints.size()) {
		TeleportToMinimapWaypoint(idx);
	}
}

void MinimapWindow::OnAddMinimapWaypoint(wxCommandEvent& event) {
	// Prompt for name
	wxString name = wxGetTextFromUser("Enter waypoint name:", "Add Minimap Waypoint");
	if (name.IsEmpty()) return;
	// Use current minimap position
	int centerX = last_center_x;
	int centerY = last_center_y;
	int floor = minimap_floor;
	minimap_waypoints.emplace_back(name, Position(centerX, centerY, floor));
	selected_minimap_waypoint_idx = minimap_waypoints.size() - 1;
	UpdateMinimapWaypointCombo();
}

void MinimapWindow::TeleportToMinimapWaypoint(int idx) {
	if (idx < 0 || idx >= (int)minimap_waypoints.size()) return;
	const MinimapWaypoint& wp = minimap_waypoints[idx];
	minimap_floor = wp.pos.z;
	needs_update = true;
	Refresh();
	// Optionally, also move the main view:
	if (g_gui.IsEditorOpen()) {
		g_gui.GetCurrentMapTab()->SetScreenCenterPosition(wp.pos);
	}
}

void MinimapWindow::SaveMinimapWaypointsToXML() {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString filePath = dataDir + wxFileName::GetPathSeparator() + "minimap_waypoints.xml";
	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "minimap_waypoints");
	for (const auto& wp : minimap_waypoints) {
		wxXmlNode* waypoint = new wxXmlNode(wxXML_ELEMENT_NODE, "waypoint");
		waypoint->AddAttribute("name", wp.name);
		waypoint->AddAttribute("x", wxString::Format("%d", wp.pos.x));
		waypoint->AddAttribute("y", wxString::Format("%d", wp.pos.y));
		waypoint->AddAttribute("floor", wxString::Format("%d", wp.pos.z));
		root->AddChild(waypoint);
	}
	doc.SetRoot(root);
	doc.Save(filePath);
}

void MinimapWindow::LoadMinimapWaypointsFromXML() {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString filePath = dataDir + wxFileName::GetPathSeparator() + "minimap_waypoints.xml";
	minimap_waypoints.clear();
	wxXmlDocument doc;
	if (doc.Load(filePath)) {
		wxXmlNode* root = doc.GetRoot();
		if (root->GetName() == "minimap_waypoints") {
			wxXmlNode* waypoint = root->GetChildren();
			while (waypoint) {
				if (waypoint->GetName() == "waypoint") {
					wxString name = waypoint->GetAttribute("name");
					int x = 0, y = 0, floor = 0;
					waypoint->GetAttribute("x").ToInt(&x);
					waypoint->GetAttribute("y").ToInt(&y);
					waypoint->GetAttribute("floor").ToInt(&floor);
					minimap_waypoints.emplace_back(name, Position(x, y, floor));
				}
				waypoint = waypoint->GetNext();
			}
			selected_minimap_waypoint_idx = 0;
			UpdateMinimapWaypointCombo();
		}
	}
}

void MinimapWindow::OnSaveMinimapWaypoints(wxCommandEvent& event) {
	SaveMinimapWaypointsToXML();
}

void MinimapWindow::OnLoadMinimapWaypoints(wxCommandEvent& event) {
	LoadMinimapWaypointsFromXML();
}

void MinimapWindow::CacheFilledBlocksForFloor(int floor) {
	if (!g_gui.IsEditorOpen()) return;
	Editor& editor = *g_gui.GetCurrentEditor();
	int mapWidth = editor.map.getWidth();
	int mapHeight = editor.map.getHeight();
	int numBlocksX = (mapWidth + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int numBlocksY = (mapHeight + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int totalBlocks = numBlocksX * numBlocksY;
	int doneBlocks = 0;
	block_cache.clear();
	for (int by = 0; by < numBlocksY; ++by) {
		for (int bx = 0; bx < numBlocksX; ++bx) {
			if (IsBlockFilled(bx, by, floor)) {
				wxBitmap bmp = RenderBlock(bx, by, floor);
				block_cache[{bx, by, floor}] = bmp;
			}
			doneBlocks++;
			int percent = int((doneBlocks / (double)totalBlocks) * 100.0);
			g_gui.SetLoadDone(percent, wxString::Format("Caching block %d/%d", doneBlocks, totalBlocks));
			wxYield();
		}
	}
}

void MinimapWindow::SaveBlockCacheToDisk(int floor) {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString mapName = GetCurrentMapName();
	wxString cacheDir = dataDir + wxFileName::GetPathSeparator() + "cachedmaps" + wxFileName::GetPathSeparator() + mapName;
	wxFileName::Mkdir(cacheDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	for (const auto& pair : block_cache) {
		if (pair.first.z != floor) continue;
		wxString fileName = wxString::Format("block_%d_%d_%d.bin", pair.first.bx, pair.first.by, pair.first.z);
		wxString filePath = cacheDir + wxFileName::GetPathSeparator() + fileName;
		wxFFile file(filePath, "wb");
		if (!file.IsOpened()) continue;
		wxImage img = pair.second.ConvertToImage();
		for (int y = 0; y < BLOCK_SIZE; ++y) {
			for (int x = 0; x < BLOCK_SIZE; ++x) {
				unsigned char r = img.GetRed(x, y);
				unsigned char g = img.GetGreen(x, y);
				unsigned char b = img.GetBlue(x, y);
				// Find closest minimap color index
				uint8_t best = 0;
				int bestDist = 256*256*3;
				for (int i = 0; i < 256; ++i) {
					int dr = int(minimap_color[i].red) - r;
					int dg = int(minimap_color[i].green) - g;
					int db = int(minimap_color[i].blue) - b;
					int dist = dr*dr + dg*dg + db*db;
					if (dist < bestDist) {
						bestDist = dist;
						best = i;
					}
				}
				file.Write(&best, 1);
			}
		}
		file.Close();
	}
}

void MinimapWindow::LoadBlockCacheFromDisk(int floor) {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString mapName = GetCurrentMapName();
	wxString cacheDir = dataDir + wxFileName::GetPathSeparator() + "cachedmaps" + wxFileName::GetPathSeparator() + mapName;
	wxDir dir(cacheDir);
	wxString filename;
	bool cont = dir.GetFirst(&filename, wxString::Format("block_*_%d.bin", floor), wxDIR_FILES);
	while (cont) {
		wxFileName fn(cacheDir, filename);
		wxFFile file(fn.GetFullPath(), "rb");
		if (!file.IsOpened()) { cont = dir.GetNext(&filename); continue; }
		std::vector<uint8_t> buffer(BLOCK_SIZE * BLOCK_SIZE);
		if (file.Read(buffer.data(), buffer.size()) == buffer.size()) {
			int bx = 0, by = 0, z = 0;
			if (sscanf(filename.mb_str(), "block_%d_%d_%d.bin", &bx, &by, &z) == 3) {
				wxImage img(BLOCK_SIZE, BLOCK_SIZE);
				for (int y = 0; y < BLOCK_SIZE; ++y) {
					for (int x = 0; x < BLOCK_SIZE; ++x) {
						uint8_t idx = buffer[y * BLOCK_SIZE + x];
						img.SetRGB(x, y, minimap_color[idx].red, minimap_color[idx].green, minimap_color[idx].blue);
					}
				}
				block_cache[{bx, by, z}] = wxBitmap(img);
			}
		}
		file.Close();
		cont = dir.GetNext(&filename);
	}
}

bool MinimapWindow::IsBlockFilled(int bx, int by, int floor) {
	if (!g_gui.IsEditorOpen()) return false;
	Editor& editor = *g_gui.GetCurrentEditor();
	int startX = bx * BLOCK_SIZE;
	int startY = by * BLOCK_SIZE;
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			Tile* tile = editor.map.getTile(startX + x, startY + y, floor);
			if (tile && tile->getMiniMapColor()) {
				return true;
			}
		}
	}
	return false;
}

wxBitmap MinimapWindow::RenderBlock(int bx, int by, int floor) {
	if (!g_gui.IsEditorOpen()) return wxBitmap(BLOCK_SIZE, BLOCK_SIZE);
	Editor& editor = *g_gui.GetCurrentEditor();
	wxBitmap bitmap(BLOCK_SIZE, BLOCK_SIZE);
	wxMemoryDC dc(bitmap);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	int startX = bx * BLOCK_SIZE;
	int startY = by * BLOCK_SIZE;
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			Tile* tile = editor.map.getTile(startX + x, startY + y, floor);
			if (tile) {
				uint8_t color = tile->getMiniMapColor();
				if (color) {
					dc.SetPen(*pens[color]);
					dc.DrawPoint(x, y);
				}
			}
		}
	}
	return bitmap;
}

wxString MinimapWindow::GetCurrentMapName() const {
	if (!g_gui.IsEditorOpen()) return "unnamed";
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return "unnamed";
	wxString mapName = editor->map.getName();
	if (mapName.IsEmpty()) mapName = "unnamed";
	return mapName;
}

void MinimapWindow::SetMinimapFloor(int floor) {
	if (minimap_floor != floor) {
		minimap_floor = floor;
		needs_update = true;
		Refresh();
	}
}