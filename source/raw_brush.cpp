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
		
		// Calculate distance (using Euclidean distance)
		double dx = static_cast<double>(depotPos.x - templePos.x);
		double dy = static_cast<double>(depotPos.y - templePos.y);
		double dz = static_cast<double>(depotPos.z - templePos.z);
		double distance = sqrt(dx * dx + dy * dy + dz * dz);
		
		if (distance < minDistance) {
			minDistance = distance;
			closestTownId = town->getID();
		}
	}
	
	return closestTownId;
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
		if (g_settings.getBoolean(Config::AUTO_ASSIGN_DEPOT_TO_CLOSEST_TEMPLE) && 
			itemtype->isDepot()) {
			Depot* depot = dynamic_cast<Depot*>(new_item);
			if (depot) {
				uint32_t closestTownId = FindClosestTemple(map, tile->getPosition());
				if (closestTownId > 0) {
					depot->setDepotID(static_cast<uint8_t>(closestTownId));
				}
			}
		}
		
		tile->addItem(new_item);
	}
}
