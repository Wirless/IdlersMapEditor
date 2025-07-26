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

#include "gui.h" // loadbar

#include "map.h"

#include <sstream>
#include "string_utils.h"

Map::Map() :
	BaseMap(),
	width(512),
	height(512),
	houses(*this),
	has_changed(false),
	unnamed(false),
	waypoints(*this) {
	// Earliest version possible
	// Caller is responsible for converting us to proper version
	mapVersion.otbm = MAP_OTBM_1;
	mapVersion.client = CLIENT_VERSION_NONE;
}

Map::~Map() {
	////
}

bool Map::open(const std::string file) {
	if (file == filename) {
		return true; // Do not reopen ourselves!
	}

	tilecount = 0;

	IOMapOTBM maploader(getVersion());

	bool success = maploader.loadMap(*this, wxstr(file));

	mapVersion = maploader.version;

	warnings = maploader.getWarnings();

	if (!success) {
		error = maploader.getError();
		return false;
	}

	has_changed = false;

	wxFileName fn = wxstr(file);
	filename = fn.GetFullPath().mb_str(wxConvUTF8);
	name = fn.GetFullName().mb_str(wxConvUTF8);

	// convert(getReplacementMapClassic(), true);

#if 0 // This will create a replacement map out of one of SO's template files
	std::ofstream out("templateshit.cpp");
	out << "\tConversionMap replacement_map;\n";
	int count = 0;
	out << "\tstd::vector<uint16_t> veckey, vecval;\n\n";

	for(int x = 20; ; x += 2) {
		int y = 22;
		Tile* old = getTile(x, y, GROUND_LAYER);
		if(old) {
			y -= 2;
			Tile* new_ = getTile(x, y, GROUND_LAYER);
			if(new_) {
				if(old->ground || old->items.size()) {
					out << "\tvecval.clear();\n";
					if(new_->ground)
						out << "\tvecval.push_back(" << new_->ground->getID() << ");\n";
					for(ItemVector::iterator iter = new_->items.begin(); iter != new_->items.end(); ++iter)
						out << "\tvecval.push_back(" << (*iter)->getID() << ");\n";

					if(old->ground && old->items.empty()) // Single item
						out << "\treplacement_map.stm[" << old->ground->getID() << "] = vecval;\n\n";
					else if(old->ground == nullptr && old->items.size() == 1) // Single item
						out << "\treplacement_map.stm[" << old->items.front()->getID() << "] = vecval;\n\n";
					else {
						// Many items
						out << "\tveckey.clear();\n";
						if(old->ground)
							out << "\tveckey.push_back(" << old->ground->getID() << ");\n";
						for(ItemVector::iterator iter = old->items.begin(); iter != old->items.end(); ++iter)
							out << "\tveckey.push_back(" << (*iter)->getID() << ");\n";
						out << "\tstd::sort(veckey.begin(), veckey.end());\n";
						out << "\treplacement_map.mtm[veckey] = vecval;\n\n";
					}
				}
			}
		} else {
			break;
		}
	}
	out.close();
#endif
	return true;
}

bool Map::convert(MapVersion to, bool showdialog) {
	if (mapVersion.client == to.client) {
		// Only OTBM version differs
		// No changes necessary
		mapVersion = to;
		return true;
	}

	/* TODO

	if(to.otbm == MAP_OTBM_4 && to.client < CLIENT_VERSION_850)
		return false;

	if(mapVersion.client >= CLIENT_VERSION_760 && to.client < CLIENT_VERSION_760)
		convert(getReplacementMapFrom760To740(), showdialog);

	if(mapVersion.client < CLIENT_VERSION_810 && to.client >= CLIENT_VERSION_810)
		convert(getReplacementMapFrom800To810(), showdialog);

	if(mapVersion.client == CLIENT_VERSION_854_BAD && to.client >= CLIENT_VERSION_854)
		convert(getReplacementMapFrom854To854(), showdialog);
	*/
	mapVersion = to;

	return true;
}

bool Map::convert(const ConversionMap& rm, bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Converting map ...");
	}

	uint64_t tiles_done = 0;
	std::vector<uint16_t> id_list;

	// std::ofstream conversions("converted_items.txt");

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		if (tile->size() == 0) {
			continue;
		}

		// id_list try MTM conversion
		id_list.clear();

		if (tile->ground) {
			id_list.push_back(tile->ground->getID());
		}
		for (ItemVector::const_iterator item_iter = tile->items.begin(); item_iter != tile->items.end(); ++item_iter) {
			if ((*item_iter)->isBorder()) {
				id_list.push_back((*item_iter)->getID());
			}
		}

		std::sort(id_list.begin(), id_list.end());

		ConversionMap::MTM::const_iterator cfmtm = rm.mtm.end();

		while (id_list.size()) {
			cfmtm = rm.mtm.find(id_list);
			if (cfmtm != rm.mtm.end()) {
				break;
			}
			id_list.pop_back();
		}

		// Keep track of how many items have been inserted at the bottom
		size_t inserted_items = 0;

		if (cfmtm != rm.mtm.end()) {
			const std::vector<uint16_t>& v = cfmtm->first;

			if (tile->ground && std::find(v.begin(), v.end(), tile->ground->getID()) != v.end()) {
				delete tile->ground;
				tile->ground = nullptr;
			}

			for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end();) {
				if (std::find(v.begin(), v.end(), (*item_iter)->getID()) != v.end()) {
					delete *item_iter;
					item_iter = tile->items.erase(item_iter);
				} else {
					++item_iter;
				}
			}

			const std::vector<uint16_t>& new_items = cfmtm->second;
			for (std::vector<uint16_t>::const_iterator iit = new_items.begin(); iit != new_items.end(); ++iit) {
				Item* item = Item::Create(*iit);
				if (item->isGroundTile()) {
					tile->ground = item;
				} else {
					tile->items.insert(tile->items.begin(), item);
					++inserted_items;
				}
			}
		}

		if (tile->ground) {
			ConversionMap::STM::const_iterator cfstm = rm.stm.find(tile->ground->getID());
			if (cfstm != rm.stm.end()) {
				uint16_t aid = tile->ground->getActionID();
				uint16_t uid = tile->ground->getUniqueID();
				delete tile->ground;
				tile->ground = nullptr;

				const std::vector<uint16_t>& v = cfstm->second;
				// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> ";
				for (std::vector<uint16_t>::const_iterator iit = v.begin(); iit != v.end(); ++iit) {
					Item* item = Item::Create(*iit);
					// conversions << *iit << " ";
					if (item->isGroundTile()) {
						item->setActionID(aid);
						item->setUniqueID(uid);
						tile->addItem(item);
					} else {
						tile->items.insert(tile->items.begin(), item);
						++inserted_items;
					}
				}
				// conversions << std::endl;
			}
		}

		for (ItemVector::iterator replace_item_iter = tile->items.begin() + inserted_items; replace_item_iter != tile->items.end();) {
			uint16_t id = (*replace_item_iter)->getID();
			ConversionMap::STM::const_iterator cf = rm.stm.find(id);
			if (cf != rm.stm.end()) {
				// uint16_t aid = (*replace_item_iter)->getActionID();
				// uint16_t uid = (*replace_item_iter)->getUniqueID();
				delete *replace_item_iter;

				replace_item_iter = tile->items.erase(replace_item_iter);
				const std::vector<uint16_t>& v = cf->second;
				for (std::vector<uint16_t>::const_iterator iit = v.begin(); iit != v.end(); ++iit) {
					replace_item_iter = tile->items.insert(replace_item_iter, Item::Create(*iit));
					// conversions << "Converted " << tile->getX() << ":" << tile->getY() << ":" << tile->getZ() << " " << id << " -> " << *iit << std::endl;
					++replace_item_iter;
				}
			} else {
				++replace_item_iter;
			}
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}

	return true;
}

void Map::cleanInvalidTiles(bool showdialog) {
	uint64_t tiles_done = 0;
	uint64_t removed_count = 0;
	bool has_invalid_tiles = false;

	// First check if there are any invalid tiles to remove
	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		if (!tile || tile->size() == 0) {
			continue;
		}

		for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end(); ++item_iter) {
			if (!g_items.typeExists((*item_iter)->getID())) {
				has_invalid_tiles = true;
				break;
			}
		}

		if (has_invalid_tiles) {
			break;
		}
	}

	if (!has_invalid_tiles) {
		if (showdialog) {
			g_gui.SetLoadDone(100);
			g_gui.PopupDialog("Cleanup Complete", "No invalid tiles found.", wxOK);
		}
		return;
	}

	// Note: We don't create a loading bar here anymore, it should be created by the caller

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		if (tile->size() == 0) {
			continue;
		}

		for (ItemVector::iterator item_iter = tile->items.begin(); item_iter != tile->items.end();) {
			if (g_items.typeExists((*item_iter)->getID())) {
				++item_iter;
			} else {
				delete *item_iter;
				item_iter = tile->items.erase(item_iter);
				++removed_count;
			}
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	if (showdialog) {
		g_gui.SetLoadDone(100);
		// Note: We don't destroy the loading bar here anymore, it should be destroyed by the caller
		g_gui.PopupDialog("Cleanup Complete", "Removed " + i2ws(removed_count) + " invalid tiles.", wxOK);
	}
}

void Map::convertHouseTiles(uint32_t fromId, uint32_t toId) {
	g_gui.CreateLoadBar("Converting house tiles...");
	uint64_t tiles_done = 0;

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		uint32_t houseId = tile->getHouseID();
		if (houseId == 0 || houseId != fromId) {
			continue;
		}

		tile->setHouseID(toId);
		++tiles_done;
		if (tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(getTileCount()) * 100.0));
		}
	}

	g_gui.DestroyLoadBar();
}

uint32_t Map::validateZoneConsistency(bool showdialog) {
	if (showdialog) {
		g_gui.CreateLoadBar("Validating zone consistency...");
	}
	
	uint32_t fixed_tiles = 0;
	uint64_t tiles_done = 0;
	uint64_t total_tiles = getTileCount();

	for (MapIterator miter = begin(); miter != end(); ++miter) {
		Tile* tile = (*miter)->get();
		ASSERT(tile);

		if (!tile->hasValidZones()) {
			tile->validateZoneConsistency();
			fixed_tiles++;
		}

		++tiles_done;
		if (showdialog && tiles_done % 0x10000 == 0) {
			g_gui.SetLoadDone(int(tiles_done / double(total_tiles) * 100.0));
		}
	}

	if (showdialog) {
		g_gui.DestroyLoadBar();
	}
	
	return fixed_tiles;
}

void Map::fixInconsistentZones() {
	// This is a more aggressive cleanup that removes all inconsistent zone data
	for (MapIterator it = begin(); it != end(); ++it) {
		Tile* tile = (*it)->get();
		if (!tile) continue;
		
		// ENHANCED FIX: Check for more zone inconsistency types
		bool hasZoneFlag = (tile->getMapFlags() & TILESTATE_ZONE_BRUSH) != 0;
		bool hasZoneData = !tile->getZoneIds().empty();
		
		// Case 1: Flag set but no zone data  
		// Case 2: Zone data exists but no flag
		// Case 3: Empty tile with zone data (dangerous for division by zero)
		if ((hasZoneFlag && !hasZoneData) || 
			(!hasZoneFlag && hasZoneData) ||
			(hasZoneData && tile->empty() && !tile->ground)) {
			
			char debug_msg[256];
			sprintf(debug_msg, "DEBUG DRAG: Map cleanup - fixing zone inconsistency at (%d,%d,%d) - flag=%d, data=%zu, empty=%d\n", 
				tile->getPosition().x, tile->getPosition().y, tile->getPosition().z,
				hasZoneFlag, tile->getZoneIds().size(), tile->empty());
			OutputDebugStringA(debug_msg);
			
			tile->clearZoneId(); // This also clears the flag
			
			// Additional validation after clearing
			tile->validateZoneConsistency();
		}
	}
}

MapVersion Map::getVersion() const {
	return mapVersion;
}

bool Map::hasChanged() const {
	return has_changed;
}

bool Map::doChange() {
	bool doupdate = !has_changed;
	has_changed = true;
	return doupdate;
}

bool Map::clearChanges() {
	bool doupdate = has_changed;
	has_changed = false;
	return doupdate;
}

bool Map::hasFile() const {
	return filename != "";
}

void Map::setWidth(int new_width) {
	if (new_width > 65000) {
		width = 65000;
	} else if (new_width < 64) {
		width = 64;
	} else {
		width = new_width;
	}
}

void Map::setHeight(int new_height) {
	if (new_height > 65000) {
		height = 65000;
	} else if (new_height < 64) {
		height = 64;
	} else {
		height = new_height;
	}
}
void Map::setMapDescription(const std::string& new_description) {
	description = new_description;
}

void Map::setHouseFilename(const std::string& new_housefile) {
	housefile = new_housefile;
	unnamed = false;
}

void Map::setSpawnFilename(const std::string& new_spawnfile) {
	spawnfile = new_spawnfile;
	unnamed = false;
}

bool Map::addSpawn(Tile* tile) {
	Spawn* spawn = tile->spawn;
	if (spawn) {
		int z = tile->getZ();
		int start_x = tile->getX() - spawn->getSize();
		int start_y = tile->getY() - spawn->getSize();
		int end_x = tile->getX() + spawn->getSize();
		int end_y = tile->getY() + spawn->getSize();

		for (int y = start_y; y <= end_y; ++y) {
			for (int x = start_x; x <= end_x; ++x) {
				TileLocation* ctile_loc = createTileL(x, y, z);
				ctile_loc->increaseSpawnCount();
			}
		}
		spawns.addSpawn(tile);
		return true;
	}
	return false;
}

void Map::removeSpawnInternal(Tile* tile) {
	Spawn* spawn = tile->spawn;
	ASSERT(spawn);

	int z = tile->getZ();
	int start_x = tile->getX() - spawn->getSize();
	int start_y = tile->getY() - spawn->getSize();
	int end_x = tile->getX() + spawn->getSize();
	int end_y = tile->getY() + spawn->getSize();

	for (int y = start_y; y <= end_y; ++y) {
		for (int x = start_x; x <= end_x; ++x) {
			TileLocation* ctile_loc = getTileL(x, y, z);
			if (ctile_loc != nullptr && ctile_loc->getSpawnCount() > 0) {
				ctile_loc->decreaseSpawnCount();
			}
		}
	}
}

void Map::removeSpawn(Tile* tile) {
	if (tile->spawn) {
		removeSpawnInternal(tile);
		spawns.removeSpawn(tile);
	}
}

SpawnList Map::getSpawnList(Tile* where) {
	SpawnList list;
	TileLocation* tile_loc = where->getLocation();
	if (tile_loc) {
		if (tile_loc->getSpawnCount() > 0) {
			uint32_t found = 0;
			if (where->spawn) {
				++found;
				list.push_back(where->spawn);
			}

			// Scans the border tiles in an expanding square around the original spawn
			int z = where->getZ();
			int start_x = where->getX() - 1, end_x = where->getX() + 1;
			int start_y = where->getY() - 1, end_y = where->getY() + 1;
			while (found != tile_loc->getSpawnCount()) {
				for (int x = start_x; x <= end_x; ++x) {
					Tile* tile = getTile(x, start_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
					tile = getTile(x, end_y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
				}

				for (int y = start_y + 1; y < end_y; ++y) {
					Tile* tile = getTile(start_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
					tile = getTile(end_x, y, z);
					if (tile && tile->spawn) {
						list.push_back(tile->spawn);
						++found;
					}
				}
				--start_x, --start_y;
				++end_x, ++end_y;
			}
		}
	}
	return list;
}

bool Map::exportMinimap(FileName filename, int floor, bool displaydialog) {
	uint8_t* pic = nullptr;

	try {
		const int MAX_AREA_SIZE = 2500;
		std::vector<std::pair<Position, Position>> initial_areas;
		std::map<uint32_t, bool> processed_tiles;

		// Helper function to get tile key
		auto getTileKey = [](int x, int y) -> uint32_t {
			return (uint32_t(x) << 16) | uint32_t(y);
		};

		// Helper function to check if two areas can be merged within size limits
		auto canMergeAreas = [MAX_AREA_SIZE](const Position& min1, const Position& max1,
											const Position& min2, const Position& max2) -> bool {
			// Calculate combined bounds
			Position combined_min(
				std::min(min1.x, min2.x),
				std::min(min1.y, min2.y),
				min1.z
			);
			Position combined_max(
				std::max(max1.x, max2.x),
				std::max(max1.y, max2.y),
				max1.z
			);

			// Check if combined area is within limits
			return (combined_max.x - combined_min.x + 1 <= MAX_AREA_SIZE) &&
				   (combined_max.y - combined_min.y + 1 <= MAX_AREA_SIZE);
		};

		// Helper function to check if areas are adjacent or close enough to merge
		auto areAreasNearby = [](const Position& max1, const Position& min2,
								const Position& min1, const Position& max2) -> bool {
			const int MERGE_DISTANCE = 1000; // Maximum distance to consider areas for merging
			
			// Check if areas are within merge distance of each other
			bool x_overlap = (min1.x <= max2.x + MERGE_DISTANCE) && (max1.x + MERGE_DISTANCE >= min2.x);
			bool y_overlap = (min1.y <= max2.y + MERGE_DISTANCE) && (max1.y + MERGE_DISTANCE >= min2.y);
			
			return x_overlap && y_overlap;
		};

		// First pass - collect all initial areas using the existing flood fill
		std::vector<Position> tiles_on_floor;
		for (MapIterator mit = begin(); mit != end(); ++mit) {
			if ((*mit)->get() == nullptr || (*mit)->empty() || (*mit)->getZ() != floor) {
				continue;
			}
			tiles_on_floor.push_back((*mit)->getPosition());
		}

		// Sort tiles for efficient processing
		std::sort(tiles_on_floor.begin(), tiles_on_floor.end(),
			[](const Position& a, const Position& b) {
				return a.x < b.x || (a.x == b.x && a.y < b.y);
			});

		// Collect initial areas using flood fill
		for (const Position& pos : tiles_on_floor) {
			uint32_t tile_key = getTileKey(pos.x, pos.y);
			if (processed_tiles[tile_key]) {
				continue;
			}

			Position area_start = pos;
			Position area_end = pos;
			std::queue<Position> to_process;
			to_process.push(pos);
			processed_tiles[tile_key] = true;

			while (!to_process.empty()) {
				Position current = to_process.front();
				to_process.pop();

				if (area_end.x - area_start.x >= MAX_AREA_SIZE ||
					area_end.y - area_start.y >= MAX_AREA_SIZE) {
					continue;
				}

				area_start.x = std::min(area_start.x, current.x);
				area_start.y = std::min(area_start.y, current.y);
				area_end.x = std::max(area_end.x, current.x);
				area_end.y = std::max(area_end.y, current.y);

				for (int dx = -1; dx <= 1; dx++) {
					for (int dy = -1; dy <= 1; dy++) {
						if (dx == 0 && dy == 0) continue;

						Position next(current.x + dx, current.y + dy, floor);
						uint32_t next_key = getTileKey(next.x, next.y);

						if (!processed_tiles[next_key]) {
							Tile* next_tile = getTile(next);
							if (next_tile && !next_tile->empty()) {
								to_process.push(next);
								processed_tiles[next_key] = true;
							}
						}
					}
				}
			}

			// Add the area if it contains content
			bool has_content = false;
			for (int y = area_start.y; y <= area_end.y && !has_content; y++) {
				for (int x = area_start.x; x <= area_end.x && !has_content; x++) {
					Tile* tile = getTile(x, y, floor);
					if (tile && !tile->empty()) {
						has_content = true;
					}
				}
			}
			if (has_content) {
				initial_areas.push_back(std::make_pair(area_start, area_end));
			}
		}

		// Second pass - merge nearby areas into larger areas while respecting size limits
		std::vector<std::pair<Position, Position>> merged_areas;
		std::vector<bool> area_processed(initial_areas.size(), false);

		for (size_t i = 0; i < initial_areas.size(); i++) {
			if (area_processed[i]) continue;

			Position current_min = initial_areas[i].first;
			Position current_max = initial_areas[i].second;
			area_processed[i] = true;

			bool merged;
			do {
				merged = false;
				for (size_t j = 0; j < initial_areas.size(); j++) {
					if (area_processed[j]) continue;

					const auto& area2 = initial_areas[j];
					if (canMergeAreas(current_min, current_max, area2.first, area2.second) &&
						areAreasNearby(current_max, area2.first, current_min, area2.second)) {
						
						// Merge the areas
						current_min.x = std::min(current_min.x, area2.first.x);
						current_min.y = std::min(current_min.y, area2.first.y);
						current_max.x = std::max(current_max.x, area2.second.x);
						current_max.y = std::max(current_max.y, area2.second.y);
						
						area_processed[j] = true;
						merged = true;
					}
				}
			} while (merged);

			merged_areas.push_back(std::make_pair(current_min, current_max));
		}

		// No areas found
		if (merged_areas.empty()) {
			return true;
		}

		// Export each merged area
		int area_count = 0;
		for (const auto& area : merged_areas) {
			Position min_pos = area.first;
			Position max_pos = area.second;

			// Add small padding but respect MAX_AREA_SIZE
			min_pos.x = std::max(0, min_pos.x - 5);
			min_pos.y = std::max(0, min_pos.y - 5);
			max_pos.x = std::min(65535, max_pos.x + 5);
			max_pos.y = std::min(65535, max_pos.y + 5);

			// Ensure we don't exceed MAX_AREA_SIZE
			if (max_pos.x - min_pos.x + 1 > MAX_AREA_SIZE) {
				max_pos.x = min_pos.x + MAX_AREA_SIZE - 1;
			}
			if (max_pos.y - min_pos.y + 1 > MAX_AREA_SIZE) {
				max_pos.y = min_pos.y + MAX_AREA_SIZE - 1;
			}

			// Calculate dimensions
			int minimap_width = max_pos.x - min_pos.x + 1;
			int minimap_height = max_pos.y - min_pos.y + 1;

			// Allocate memory for the bitmap
			pic = newd uint8_t[minimap_width * minimap_height];
			memset(pic, 0, minimap_width * minimap_height);

			// Fill the bitmap
			for (int y = min_pos.y; y <= max_pos.y; y++) {
				for (int x = min_pos.x; x <= max_pos.x; x++) {
					Tile* tile = getTile(x, y, floor);
					if (!tile || tile->empty()) {
						continue;
					}

					uint32_t pixelpos = (y - min_pos.y) * minimap_width + (x - min_pos.x);
					uint8_t& pixel = pic[pixelpos];

					// Get color from items
					for (ItemVector::const_reverse_iterator item_iter = tile->items.rbegin(); 
						 item_iter != tile->items.rend(); ++item_iter) {
						if ((*item_iter)->getMiniMapColor()) {
							pixel = (*item_iter)->getMiniMapColor();
							break;
						}
					}

					// If no item color, check ground
					if (pixel == 0 && tile->hasGround()) {
						pixel = tile->ground->getMiniMapColor();
					}
				}
			}

			// Generate filename for this area
			wxString base_name = filename.GetFullPath();
			base_name = base_name.BeforeLast('.');
			wxString area_filename = wxString::Format("%s_area%d.bmp", base_name, area_count++);

			// Write to file
			FileWriteHandle fh(nstr(area_filename));
			if (!fh.isOpen()) {
				delete[] pic;
				return false;
			}

			// Store the magic number
			fh.addRAW("BM");

			// Store the file size
			uint32_t file_size = 14 // header
				+ 40 // image data header
				+ 256 * 4 // color palette
				+ ((minimap_width + 3) / 4 * 4) * minimap_height; // pixels
			fh.addU32(file_size);

			// Two values reserved, must always be 0.
			fh.addU16(0);
			fh.addU16(0);

			// Bitmapdata offset
			fh.addU32(14 + 40 + 256 * 4);

			// Header size
			fh.addU32(40);

			// Header width/height
			fh.addU32(minimap_width);
			fh.addU32(minimap_height);

			// Color planes
			fh.addU16(1);

			// bits per pixel, OT map format is 8
			fh.addU16(8);

			// compression type, 0 is no compression
			fh.addU32(0);

			// image size, 0 is valid if we use no compression
			fh.addU32(0);

			// horizontal/vertical resolution in pixels / meter
			fh.addU32(4000);
			fh.addU32(4000);

			// Number of colors
			fh.addU32(256);
			// Important colors, 0 is all
			fh.addU32(0);

			// Write the color palette
			for (int i = 0; i < 256; ++i) {
				fh.addU32(uint32_t(minimap_color[i]));
			}

			// Bitmap width must be divisible by four, calculate how much padding we need
			int padding = ((minimap_width & 3) != 0 ? 4 - (minimap_width & 3) : 0);
			// Bitmap rows are saved in reverse order
			for (int y = minimap_height - 1; y >= 0; --y) {
				fh.addRAW(pic + y * minimap_width, minimap_width);
				for (int i = 0; i < padding; ++i) {
					fh.addU8(0);
				}
				if (displaydialog && y % 100 == 0) {
					g_gui.SetLoadDone(90 + int((minimap_height - y) / double(minimap_height) * 10.0));
				}
			}

			delete[] pic;
			pic = nullptr;
		}

		return true;
	} catch (...) {
		delete[] pic;
		throw;
	}
}

uint32_t Map::cleanDuplicateItems(const std::vector<std::pair<uint16_t, uint16_t>>& ranges, const PropertyFlags& flags) {
	uint32_t duplicates_removed = 0;
	uint32_t tiles_affected = 0;

	// Helper function to check if item is in ranges
	auto isInRanges = [&ranges](uint16_t id) -> bool {
		if (ranges.empty()) return true; // If no ranges, check all items
		for (const auto& range : ranges) {
			if (id >= range.first && id <= range.second) return true;
		}
		return false;
	};

	// Helper function to compare items considering property flags
	auto compareItems = [&flags](Item* item1, Item* item2) -> bool {
		if (item1->getID() != item2->getID()) return false;

		const ItemType& type1 = g_items[item1->getID()];
		const ItemType& type2 = g_items[item2->getID()];

		// If a flag is true (checked), we treat items with that property as different
		// even if they have the same value for that property
		if (flags.ignore_unpassable && (type1.unpassable || type2.unpassable)) return false;
		if (flags.ignore_unmovable && (type1.moveable || type2.moveable)) return false;
		if (flags.ignore_block_missiles && (type1.blockMissiles || type2.blockMissiles)) return false;
		if (flags.ignore_block_pathfinder && (type1.blockPathfinder || type2.blockPathfinder)) return false;
		if (flags.ignore_readable && (type1.canReadText || type2.canReadText)) return false;
		if (flags.ignore_writeable && (type1.canWriteText || type2.canWriteText)) return false;
		if (flags.ignore_pickupable && (type1.pickupable || type2.pickupable)) return false;
		if (flags.ignore_stackable && (type1.stackable || type2.stackable)) return false;
		if (flags.ignore_rotatable && (type1.rotable || type2.rotable)) return false;
		if (flags.ignore_hangable && (type1.isHangable || type2.isHangable)) return false;
		if (flags.ignore_hook_east && (type1.hookEast || type2.hookEast)) return false;
		if (flags.ignore_hook_south && (type1.hookSouth || type2.hookSouth)) return false;
		if (flags.ignore_elevation && (type1.hasElevation || type2.hasElevation)) return false;

		return true;
	};

	for (MapIterator mit = begin(); mit != end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (!tile) continue;

		bool tile_modified = false;
		std::set<Item*> kept_items; // Track first instances

		// First pass: identify items to keep
		for (Item* item : tile->items) {
			if (!isInRanges(item->getID())) continue;

			bool is_duplicate = false;
			for (Item* kept : kept_items) {
				if (compareItems(item, kept)) {
					is_duplicate = true;
					break;
				}
			}

			if (!is_duplicate) {
				kept_items.insert(item);
			}
		}

		// Second pass: remove duplicates
		auto iit = tile->items.begin();
		while (iit != tile->items.end()) {
			Item* item = *iit;
			if (!isInRanges(item->getID())) {
				++iit;
				continue;
			}

			bool should_remove = true;
			for (Item* kept : kept_items) {
				if (item == kept) {
					should_remove = false;
					break;
				}
			}

			if (should_remove) {
				delete item;
				iit = tile->items.erase(iit);
				duplicates_removed++;
				tile_modified = true;
			} else {
				++iit;
			}
		}

		if (tile_modified) {
			tiles_affected++;
		}
	}

	return duplicates_removed;
}
