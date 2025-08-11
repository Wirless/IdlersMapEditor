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
#include "gui.h"

#include "raw_brush.h"
#include "settings.h"
#include "items.h"
#include "basemap.h"
#include "map.h"
#include "town.h"
#include "complexitem.h"

//=============================================================================
// Helper function to find closest temple

uint32_t FindClosestTemple(BaseMap* map, const Position& depotPos) {
	if (!map) return 0;
	
	// Cast to Map to access towns
	Map* fullMap = dynamic_cast<Map*>(map);
	if (!fullMap) return 0;
	
	const Towns& towns = fullMap->towns;
	if (towns.count() == 0) return 0;
	
	uint32_t closestTownId = 0;
	double minDistance = std::numeric_limits<double>::max();
	
	for (TownMap::const_iterator town_iter = towns.begin(); town_iter != towns.end(); ++town_iter) {
		const Town* town = town_iter->second;
		if (!town) continue;
		
		const Position& templePos = town->getTemplePosition();
		if (!templePos.isValid()) continue;
		
		// Optimize distance calculation for large maps
		int64_t dx = static_cast<int64_t>(depotPos.x) - static_cast<int64_t>(templePos.x);
		int64_t dy = static_cast<int64_t>(depotPos.y) - static_cast<int64_t>(templePos.y);
		int64_t dz = static_cast<int64_t>(depotPos.z) - static_cast<int64_t>(templePos.z);
		
		// Use squared distance to avoid expensive sqrt operation
		double distanceSquared = static_cast<double>(dx * dx + dy * dy + dz * dz);
		
		if (distanceSquared < minDistance) {
			minDistance = distanceSquared;
			closestTownId = town->getID();
		}
	}
	
	return closestTownId;
}

// Universal helper function to apply depot assignment for any newly created item
void ApplyDepotAssignment(Item* item, BaseMap* map, const Position& position) {
	if (!item || !map || !g_settings.getBoolean(Config::AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE)) {
		return;
	}
	
	const ItemType& itemType = g_items[item->getID()];
	if (itemType.isDepot()) {
		Depot* depot = dynamic_cast<Depot*>(item);
		if (depot) {
			uint32_t closestTownId = FindClosestTemple(map, position);
			if (closestTownId > 0) {
				depot->setDepotID(static_cast<uint8_t>(closestTownId));
			}
		}
	}
}

//=============================================================================
// RAW brush

RAWBrush::RAWBrush(uint16_t itemid) :
	Brush() {
	ItemType& it = g_items[itemid];
	if (it.id == 0) {
		itemtype = nullptr;
	} else {
		itemtype = &it;
	}
}

RAWBrush::~RAWBrush() {
	////
}

int RAWBrush::getLookID() const {
	if (itemtype) {
		return itemtype->clientID;
	}
	return 0;
}

uint16_t RAWBrush::getItemID() const {
	return itemtype->id;
}

std::string RAWBrush::getName() const {
	if (!itemtype) {
		return "RAWBrush";
	}

	if (itemtype->hookSouth) {
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook South)";
	} else if (itemtype->hookEast) {
		return i2s(itemtype->id) + " - " + itemtype->name + " (Hook East)";
	}

	return i2s(itemtype->id) + " - " + itemtype->name + itemtype->editorsuffix;
}

void RAWBrush::undraw(BaseMap* map, Tile* tile) {
	if (tile->ground && tile->ground->getID() == itemtype->id) {
		delete tile->ground;
		tile->ground = nullptr;
	}
	for (ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
		Item* item = *iter;
		if (item->getID() == itemtype->id) {
			delete item;
			iter = tile->items.erase(iter);
		} else {
			++iter;
		}
	}
}

void RAWBrush::draw(BaseMap* map, Tile* tile, void* parameter) {
	if (!itemtype) {
		return;
	}

	bool b = parameter ? *reinterpret_cast<bool*>(parameter) : false;
	if ((g_settings.getInteger(Config::RAW_LIKE_SIMONE) && !b) && itemtype->alwaysOnBottom && itemtype->alwaysOnTopOrder == 2) {
		for (ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
			Item* item = *iter;
			if (item->getTopOrder() == itemtype->alwaysOnTopOrder) {
				delete item;
				iter = tile->items.erase(iter);
			} else {
				++iter;
			}
		}
	}
	Item* new_item = Item::Create(itemtype->id);
	if (new_item) {
		if (g_gui.IsCurrentActionIDEnabled()) {
			new_item->setActionID(g_gui.GetCurrentActionID());
		}
		
		// Auto-assign depot to closest temple if enabled and this is a depot item
		ApplyDepotAssignment(new_item, map, tile->getPosition());
		
		tile->addItem(new_item);
	}
}
