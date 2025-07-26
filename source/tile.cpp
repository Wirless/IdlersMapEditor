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

#include "brush.h"

#include "tile.h"
#include "creature.h"
#include "house.h"
#include "basemap.h"
#include "spawn.h"
#include "ground_brush.h"
#include "wall_brush.h"
#include "carpet_brush.h"
#include "table_brush.h"
#include "town.h"
#include "map.h"

static thread_local std::set<Position> wallize_processing_tiles;

Tile::Tile(int x, int y, int z) :
	location(nullptr),
	ground(nullptr),
	creature(nullptr),
	spawn(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::Tile(TileLocation& loc) :
	location(&loc),
	ground(nullptr),
	creature(nullptr),
	spawn(nullptr),
	house_id(0),
	mapflags(0),
	statflags(0),
	minimapColor(INVALID_MINIMAP_COLOR) {
	////
}

Tile::~Tile() {
	bool had_items = !items.empty();
	bool had_ground = ground != nullptr;
	
#ifdef __WXDEBUG__
	if (had_ground) {
		// Store ground info before deleting it
		uint16_t ground_id = ground->getID();
		void* ground_ptr = ground;
		printf("DEBUG: Tile destructor for %p with ground %p (ID:%d)\n", this, ground_ptr, ground_id);
		
		// Get call stack info by adding a breakpoint variable
		int debug_breakpoint_for_ground_deletion = 1;
	}
#endif

	while (!items.empty()) {
		delete items.back();
		items.pop_back();
	}
	delete creature;
	delete ground;
	delete spawn;
	
#ifdef __WXDEBUG__
	if (had_ground) {
		printf("DEBUG: Ground %p deleted\n", ground);
	}
#endif
}

Tile* Tile::deepCopy(BaseMap& map) {
	Tile* copy = map.allocator.allocateTile(location);
	copy->flags = flags;
	copy->house_id = house_id;
	
#ifdef __WXDEBUG__
	printf("DEBUG: deepCopy - Creating copy of tile %p (with ground %p)\n", 
		this, ground);
#endif
	
	if (spawn) {
		copy->spawn = spawn->deepCopy();
	}
	if (creature) {
		copy->creature = creature->deepCopy();
	}
	// Spawncount & exits are not transferred on copy!
	if (ground) {
#ifdef __WXDEBUG__
		printf("DEBUG: deepCopy - Copying ground %p with ID %d\n", 
			ground, ground->getID());
#endif
		copy->ground = ground->deepCopy();
#ifdef __WXDEBUG__
		printf("DEBUG: deepCopy - Ground copied to %p with ID %d\n", 
			copy->ground, copy->ground->getID());
#endif
	}

	copy->setZoneIds(this);

	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		copy->items.push_back((*it)->deepCopy());
		++it;
	}

#ifdef __WXDEBUG__
	printf("DEBUG: deepCopy - Created tile copy %p (with ground %p)\n", 
		copy, copy->ground);
#endif

	return copy;
}

uint32_t Tile::memsize() const {
	uint32_t mem = sizeof(*this);
	if (ground) {
		mem += ground->memsize();
	}

	ItemVector::const_iterator it;

	it = items.begin();
	while (it != items.end()) {
		mem += (*it)->memsize();
		++it;
	}

	mem += sizeof(Item*) * items.capacity();

	return mem;
}

int Tile::size() const {
	int sz = 0;
	if (ground) {
		++sz;
	}
	sz += items.size();
	if (creature) {
		++sz;
	}
	if (spawn) {
		++sz;
	}
	if (location) {
		if (location->getHouseExits()) {
			++sz;
		}
		if (location->getSpawnCount()) {
			++sz;
		}
		if (location->getWaypointCount()) {
			++sz;
		}
	}
	return sz;
}

void Tile::merge(Tile* other) {
	if (other->isPZ()) {
		setPZ(true);
	}
	if (other->house_id) {
		house_id = other->house_id;
	}

	if (other->ground) {
		delete ground;
		ground = other->ground;
		other->ground = nullptr;
	}

	if (other->creature) {
		delete creature;
		creature = other->creature;
		other->creature = nullptr;
	}

	if (other->spawn) {
		delete spawn;
		spawn = other->spawn;
		other->spawn = nullptr;
	}

	if (other->creature) {
		delete creature;
		creature = other->creature;
		other->creature = nullptr;
	}

	ItemVector::iterator it;

	it = other->items.begin();
	while (it != other->items.end()) {
		addItem(*it);
		++it;
	}
	other->items.clear();
}

bool Tile::hasProperty(enum ITEMPROPERTY prop) const {
	if (prop == PROTECTIONZONE && isPZ()) {
		return true;
	}

	if (ground && ground->hasProperty(prop)) {
		return true;
	}

	ItemVector::const_iterator iit;
	for (iit = items.begin(); iit != items.end(); ++iit) {
		if ((*iit)->hasProperty(prop)) {
			return true;
		}
	}

	return false;
}

int Tile::getIndexOf(Item* item) const {
	if (!item) {
		return wxNOT_FOUND;
	}

	int index = 0;
	if (ground) {
		if (ground == item) {
			return index;
		}
		index++;
	}

	if (!items.empty()) {
		auto it = std::find(items.begin(), items.end(), item);
		if (it != items.end()) {
			index += (it - items.begin());
			return index;
		}
	}
	return wxNOT_FOUND;
}

Item* Tile::getTopItem() const {
	if (!items.empty() && !items.back()->isMetaItem()) {
		return items.back();
	}
	if (ground && !ground->isMetaItem()) {
		return ground;
	}
	return nullptr;
}

Item* Tile::getItemAt(int index) const {
	if (index < 0) {
		return nullptr;
	}
	if (ground) {
		if (index == 0) {
			return ground;
		}
		index--;
	}
	if (index >= 0 && index < items.size()) {
		return items.at(index);
	}
	return nullptr;
}

void Tile::addItem(Item* item) {
	if (!item) {
		return;
	}
	
	// Handle ground tiles
	if (item->isGroundTile()) {
#ifdef __WXDEBUG__
		printf("DEBUG: Adding ground tile ID %d to position %d,%d,%d\n", 
			item->getID(), getPosition().x, getPosition().y, getPosition().z);
#endif
		// Always delete the existing ground first
		delete ground;
		ground = item;
		
		// Also check for any ground items that might be in the items list
		// and remove them to prevent stacking issues
		ItemVector::iterator it = items.begin();
		while (it != items.end()) {
			if ((*it)->isGroundTile() || (*it)->getGroundEquivalent() != 0) {
#ifdef __WXDEBUG__
				printf("DEBUG: Removing misplaced ground item with ID %d from tile items\n", (*it)->getID());
#endif
				delete *it;
				it = items.erase(it);
			} else {
				++it;
			}
		}
		return;
	}

	// Handle items with ground equivalents
	uint16_t gid = item->getGroundEquivalent();
	if (gid != 0) {
		// If item has a ground equivalent, replace the ground
		delete ground;
		ground = Item::Create(gid);
		
		// Insert at the very bottom of the stack
		items.insert(items.begin(), item);
		
		if (item->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		return;
	}
	
	// Handle normal items
	ItemVector::iterator it;
	if (item->isAlwaysOnBottom()) {
		it = items.begin();
		while (true) {
			if (it == items.end()) {
				break;
			} else if ((*it)->isAlwaysOnBottom()) {
				if (item->getTopOrder() < (*it)->getTopOrder()) {
					break;
				}
			} else { // Always on top
				break;
			}
			++it;
		}
	} else {
		it = items.end();
	}

	items.insert(it, item);

	if (item->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
}

void Tile::select() {
	if (size() == 0) {
		return;
	}
	if (ground) {
		ground->select();
	}
	if (spawn) {
		spawn->select();
	}
	if (creature) {
		creature->select();
	}

	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		(*it)->select();
		++it;
	}

	statflags |= TILESTATE_SELECTED;
}

void Tile::deselect() {
	if (ground) {
		ground->deselect();
	}
	if (spawn) {
		spawn->deselect();
	}
	if (creature) {
		creature->deselect();
	}

	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		(*it)->deselect();
		++it;
	}

	statflags &= ~TILESTATE_SELECTED;
}

Item* Tile::getTopSelectedItem() {
	for (ItemVector::reverse_iterator iter = items.rbegin(); iter != items.rend(); ++iter) {
		if ((*iter)->isSelected() && !(*iter)->isMetaItem()) {
			return *iter;
		}
	}
	if (ground && ground->isSelected() && !ground->isMetaItem()) {
		return ground;
	}
	return nullptr;
}

ItemVector Tile::popSelectedItems(bool ignoreTileSelected) {
	ItemVector pop_items;

	if (!ignoreTileSelected && !isSelected()) {
		return pop_items;
	}

	if (ground && ground->isSelected()) {
		pop_items.push_back(ground);
		ground = nullptr;
	}

	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isSelected()) {
			pop_items.push_back(*it);
			it = items.erase(it);
		} else {
			++it;
		}
	}

	statflags &= ~TILESTATE_SELECTED;
	return pop_items;
}

ItemVector Tile::getSelectedItems(bool unzoomed) {
	ItemVector selected_items;

	if (!isSelected()) {
		return selected_items;
	}

	if (ground && ground->isSelected()) {
		selected_items.push_back(ground);
	}

	// save performance when zoomed out
	if (!unzoomed) {
		ItemVector::iterator it;

		it = items.begin();
		while (it != items.end()) {
			if ((*it)->isSelected()) {
				selected_items.push_back(*it);
			}
			it++;
		}
	}

	return selected_items;
}

uint8_t Tile::getMiniMapColor() const {
	if (minimapColor != INVALID_MINIMAP_COLOR) {
		return minimapColor;
	}

	for (ItemVector::const_reverse_iterator item_iter = items.rbegin(); item_iter != items.rend(); ++item_iter) {
		if ((*item_iter)->getMiniMapColor()) {
			return (*item_iter)->getMiniMapColor();
			break;
		}
	}

	// check ground too
	if (hasGround()) {
		return ground->getMiniMapColor();
	}

	return 0;
}

bool tilePositionLessThan(const Tile* a, const Tile* b) {
	return a->getPosition() < b->getPosition();
}

bool tilePositionVisualLessThan(const Tile* a, const Tile* b) {
	Position pa = a->getPosition();
	Position pb = b->getPosition();

	if (pa.z > pb.z) {
		return true;
	}
	if (pa.z < pb.z) {
		return false;
	}

	if (pa.y < pb.y) {
		return true;
	}
	if (pa.y > pb.y) {
		return false;
	}

	if (pa.x < pb.x) {
		return true;
	}

	return false;
}

void Tile::validateZoneConsistency() {
	bool hasZoneFlag = (mapflags & TILESTATE_ZONE_BRUSH) != 0;
	bool hasZoneData = !zoneIds.empty();
	
	// ENHANCED FIX: Handle more edge cases to prevent division by zero
	// Remove invalid zone IDs (0 values)
	zoneIds.erase(std::remove(zoneIds.begin(), zoneIds.end(), 0), zoneIds.end());
	
	// Update hasZoneData after cleaning
	hasZoneData = !zoneIds.empty();
	
	if (hasZoneFlag && !hasZoneData) {
		// Clear invalid zone flag if no zone data exists
		mapflags &= ~TILESTATE_ZONE_BRUSH;
	} else if (!hasZoneFlag && hasZoneData) {
		// Set zone flag if zone data exists but flag is missing
		mapflags |= TILESTATE_ZONE_BRUSH;
	}
	
	// CRITICAL FIX: Additional safety check for empty tiles with zones
	// If the tile is completely empty but has zone data, clear zones to prevent crashes
	if (zoneIds.size() > 0 && empty() && !ground) {
		char debug_msg[256];
		sprintf(debug_msg, "DEBUG DRAG: WARNING! Empty tile at (%d,%d,%d) has %zu zones - clearing to prevent crashes\n", 
			getPosition().x, getPosition().y, getPosition().z, zoneIds.size());
		OutputDebugStringA(debug_msg);
		
		clearZoneId();
	}
}

bool Tile::hasValidZones() const {
	bool hasZoneFlag = (mapflags & TILESTATE_ZONE_BRUSH) != 0;
	bool hasZoneData = !zoneIds.empty();
	
	// Zones are valid if both flag and data are consistent
	return (hasZoneFlag && hasZoneData) || (!hasZoneFlag && !hasZoneData);
}

void Tile::update() {
	statflags &= TILESTATE_MODIFIED;

	if (spawn && spawn->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}
	if (creature && creature->isSelected()) {
		statflags |= TILESTATE_SELECTED;
	}

	if (ground) {
		if (ground->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (ground->isBlocking()) {
			statflags |= TILESTATE_BLOCKING;
		}
		if (ground->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (ground->getMiniMapColor() != 0) {
			minimapColor = ground->getMiniMapColor();
		}
	}

	ItemVector::const_iterator iter = items.begin();
	while (iter != items.end()) {
		Item* i = *iter;
		if (i->isSelected()) {
			statflags |= TILESTATE_SELECTED;
		}
		if (i->getUniqueID() != 0) {
			statflags |= TILESTATE_UNIQUE;
		}
		if (i->getMiniMapColor() != 0) {
			minimapColor = i->getMiniMapColor();
		}

		ItemType& it = g_items[i->getID()];
		if (it.unpassable) {
			statflags |= TILESTATE_BLOCKING;
		}
		if (it.isOptionalBorder) {
			statflags |= TILESTATE_OP_BORDER;
		}
		if (it.isTable) {
			statflags |= TILESTATE_HAS_TABLE;
		}
		if (it.isCarpet) {
			statflags |= TILESTATE_HAS_CARPET;
		}
		++iter;
	}

	if ((statflags & TILESTATE_BLOCKING) == 0) {
		if (ground == nullptr && items.size() == 0) {
			statflags |= TILESTATE_BLOCKING;
		}
	}
	
	// Validate zone consistency to prevent crashes
	validateZoneConsistency();
}

void Tile::borderize(BaseMap* parent) {
	if (!ground) {
		OutputDebugStringA("DEBUG DRAG: borderize called on tile with no ground, skipping\n");
		return;
	}
	// Add debugging output for borderize operation
	char debug_msg[512];
	sprintf(debug_msg, "DEBUG DRAG: borderize called on tile at pos=(%d,%d,%d), SAME_GROUND_TYPE_BORDER=%d\n", 
		getPosition().x, getPosition().y, getPosition().z, g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER));
	OutputDebugStringA(debug_msg);
	
	if (g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER)) {
		// Use the custom reborderize method for better border placement
		sprintf(debug_msg, "DEBUG DRAG: Calling GroundBrush::reborderizeTile for tile at pos=(%d,%d,%d)\n", 
			getPosition().x, getPosition().y, getPosition().z);
		OutputDebugStringA(debug_msg);
		GroundBrush::reborderizeTile(parent, this);
	} else {
		// Standard border handling
		sprintf(debug_msg, "DEBUG DRAG: Calling GroundBrush::doBorders for tile at pos=(%d,%d,%d)\n", 
			getPosition().x, getPosition().y, getPosition().z);
		OutputDebugStringA(debug_msg);
		GroundBrush::doBorders(parent, this);
	}
}

void Tile::addBorderItem(Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isBorder());
	
	if (g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER)) {
		// When Same Ground Type Border is enabled, add borders at the end (top) of the stack
		// This ensures borders appear on top of existing items
		items.push_back(item);
	} else {
		// Standard behavior (borders at the bottom of the stack)
		items.insert(items.begin(), item);
	}
}

GroundBrush* Tile::getGroundBrush() const {
	if (ground) {
		if (ground->getGroundBrush()) {
			return ground->getGroundBrush();
		}
	}
	return nullptr;
}

void Tile::cleanBorders() {
	// If Same Ground Type Border is enabled, we don't clean all borders
	// This will be handled in the GroundBrush::doBorders method
	if (g_settings.getBoolean(Config::SAME_GROUND_TYPE_BORDER)) {
		return;
	}

	// When Same Ground Type Border is disabled, remove all border items
	for (ItemVector::iterator it = items.begin(); it != items.end();) {
		if ((*it)->isBorder()) {
			delete *it;
			it = items.erase(it);
		} else {
			++it;
		}
	}
}

void Tile::wallize(BaseMap* parent) {
	// Add recursion guard
	if (wallize_processing_tiles.find(getPosition()) != wallize_processing_tiles.end()) {
		return;
	}
	wallize_processing_tiles.insert(getPosition());

	WallBrush::doWalls(parent, this);

	// Remove from processing set when done
	wallize_processing_tiles.erase(getPosition());
}

Item* Tile::getWall() const {
	ItemVector::const_iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isWall()) {
			return *it;
		}
		++it;
	}
	return nullptr;
}

Item* Tile::getCarpet() const {
	ItemVector::const_iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isCarpet()) {
			return *it;
		}
		++it;
	}
	return nullptr;
}

Item* Tile::getTable() const {
	ItemVector::const_iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isTable()) {
			return *it;
		}
		++it;
	}
	return nullptr;
}

void Tile::addWallItem(Item* item) {
	if (!item) {
		return;
	}
	ASSERT(item->isWall());

	addItem(item);
}

void Tile::cleanWalls(bool dontdelete) {
	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isWall()) {
			if (!dontdelete) {
				delete *it;
			}
			it = items.erase(it);
		} else {
			++it;
		}
	}
}

void Tile::cleanWalls(WallBrush* wb) {
	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isWall() && wb->hasWall(*it)) {
			delete *it;
			it = items.erase(it);
		} else {
			++it;
		}
	}
}

void Tile::cleanTables(bool dontdelete) {
	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isTable()) {
			if (!dontdelete) {
				delete *it;
			}
			it = items.erase(it);
		} else {
			++it;
		}
	}
}

void Tile::tableize(BaseMap* parent) {
	TableBrush::doTables(parent, this);
}

void Tile::carpetize(BaseMap* parent) {
	CarpetBrush::doCarpets(parent, this);
}

void Tile::selectGround() {
	bool selected_ = false;
	if (ground) {
		ground->select();
		selected_ = true;
	}
	ItemVector::iterator it;

	it = items.begin();
	while (it != items.end()) {
		if ((*it)->isBorder()) {
			(*it)->select();
			selected_ = true;
		} else {
			break;
		}
		++it;
	}
	if (selected_) {
		statflags |= TILESTATE_SELECTED;
	}
}

void Tile::deselectGround() {
	if (ground) {
		ground->deselect();
	}
	ItemVector::iterator it = items.begin();
	while (it != items.end()) {
		if ((*it)->isBorder()) {
			(*it)->deselect();
		} else {
			break;
		}
		++it;
	}
}

void Tile::setHouse(House* _house) {
	house_id = (_house ? _house->getID() : 0);
}

void Tile::setHouseID(uint32_t newHouseId) {
	house_id = newHouseId;
}

bool Tile::isTownExit(Map& map) const {
	return location->getTownCount() > 0;
}

void Tile::addHouseExit(House* h) {
	if (!h) {
		return;
	}
	HouseExitList* house_exits = location->createHouseExits();
	house_exits->push_back(h->getID());
}

void Tile::removeHouseExit(House* h) {
	if (!h) {
		return;
	}

	HouseExitList* house_exits = location->getHouseExits();
	if (!house_exits) {
		return;
	}

	for (std::vector<uint32_t>::iterator it = house_exits->begin(); it != house_exits->end(); ++it) {
		if (*it == h->getID()) {
			house_exits->erase(it);
			return;
		}
	}
}
