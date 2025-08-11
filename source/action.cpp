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

#include "action.h"
#include "settings.h"
#include "map.h"
#include "editor.h"
#include "gui.h"

// Add necessary includes for exception handling and file operations
#include <exception>
#include <fstream>
#include <wx/filename.h>
#include <wx/datetime.h>
#include <wx/stdpaths.h>

Change::Change() :
	type(CHANGE_NONE), data(nullptr) {
	////
}

Change::Change(Tile* t) :
	type(CHANGE_TILE) {
	ASSERT(t);
	data = t;
}

Change* Change::Create(House* house, const Position& where) {
	Change* c = newd Change();
	c->type = CHANGE_MOVE_HOUSE_EXIT;
	std::pair<uint32_t, Position>* p = newd std::pair<uint32_t, Position>;
	p->first = house->getID();
	p->second = where;
	c->data = p;
	return c;
}

Change* Change::Create(Waypoint* wp, const Position& where) {
	Change* c = newd Change();
	c->type = CHANGE_MOVE_WAYPOINT;
	std::pair<std::string, Position>* p = newd std::pair<std::string, Position>;
	p->first = wp->name;
	p->second = where;
	c->data = p;
	return c;
}

Change::~Change() {
	clear();
}

void Change::clear() {
	switch (type) {
		case CHANGE_TILE:
			ASSERT(data);
			delete reinterpret_cast<Tile*>(data);
			break;
		case CHANGE_MOVE_HOUSE_EXIT:
			ASSERT(data);
			delete reinterpret_cast<std::pair<uint32_t, Position>*>(data);
			break;
		case CHANGE_MOVE_WAYPOINT:
			ASSERT(data);
			delete reinterpret_cast<std::pair<std::string, Position>*>(data);
			break;
		case CHANGE_NONE:
			break;
		default:
#ifdef __DEBUG_MODE__
			if (data) {
				printf("UNHANDLED CHANGE TYPE! Leak!");
			}
#endif
			break;
	}
	type = CHANGE_NONE;
	data = nullptr;
}

uint32_t Change::memsize() const {
	uint32_t mem = sizeof(*this);
	switch (type) {
		case CHANGE_TILE:
			ASSERT(data);
			mem += reinterpret_cast<Tile*>(data)->memsize();
			break;
		default:
			break;
	}
	return mem;
}

Action::Action(Editor& editor, ActionIdentifier ident) :
	commited(false),
	editor(editor),
	type(ident) {
}

Action::~Action() {
	ChangeList::const_reverse_iterator it = changes.rbegin();
	while (it != changes.rend()) {
		delete *it;
		++it;
	}
}

size_t Action::approx_memsize() const {
	uint32_t mem = sizeof(*this);
	mem += changes.size() * (sizeof(Change) + sizeof(Tile) + sizeof(Item) + 6 /* approx overhead*/);
	return mem;
}

size_t Action::memsize() const {
	uint32_t mem = sizeof(*this);
	mem += sizeof(Change*) * 3 * changes.size();
	ChangeList::const_iterator it = changes.begin();
	while (it != changes.end()) {
		Change* c = *it;
		switch (c->type) {
			case CHANGE_TILE: {
				ASSERT(c->data);
				mem += reinterpret_cast<Tile*>(c->data)->memsize();
				break;
			}

			default:
				break;
		}
		++it;
	}
	return mem;
}

void Action::commit(DirtyList* dirty_list) {
	editor.selection.start(Selection::INTERNAL);
	ChangeList::const_iterator it = changes.begin();
	while (it != changes.end()) {
		Change* c = *it;
		switch (c->type) {
			case CHANGE_TILE: {
				void** data = &c->data;
				Tile* newtile = reinterpret_cast<Tile*>(*data);
				ASSERT(newtile);
				Position pos = newtile->getPosition();

				if (editor.IsLiveClient()) {
					QTreeNode* nd = editor.map.getLeaf(pos.x, pos.y);
					if (!nd || !nd->isVisible(pos.z > GROUND_LAYER)) {
						// Delete all changes that affect tiles outside our view
						c->clear();
						++it;
						continue;
					}
				}

				Tile* oldtile = editor.map.swapTile(pos, newtile);
				TileLocation* location = newtile->getLocation();

				// Update other nodes in the network
				if (editor.IsLiveServer() && dirty_list) {
					dirty_list->AddPosition(pos.x, pos.y, pos.z);
				}

				newtile->update();

				// std::cout << "\tSwitched tile at " << pos.x << ";" << pos.y << ";" << pos.z << " from " << (void*)oldtile << " to " << *data <<  std::endl;
				if (newtile->isSelected()) {
					editor.selection.addInternal(newtile);
				}

				if (oldtile) {
					if (newtile->getHouseID() != oldtile->getHouseID()) {
						// oooooomggzzz we need to add it to the appropriate house!
						House* house = editor.map.houses.getHouse(oldtile->getHouseID());
						if (house) {
							house->removeTile(oldtile);
						}

						house = editor.map.houses.getHouse(newtile->getHouseID());
						if (house) {
							house->addTile(newtile);
						}
					}
					if (oldtile->spawn) {
						if (newtile->spawn) {
							if (*oldtile->spawn != *newtile->spawn) {
								editor.map.removeSpawn(oldtile);
								editor.map.addSpawn(newtile);
							}
						} else {
							// Spawn has been removed
							editor.map.removeSpawn(oldtile);
						}
					} else if (newtile->spawn) {
						editor.map.addSpawn(newtile);
					}

					// oldtile->update();
					if (oldtile->isSelected()) {
						editor.selection.removeInternal(oldtile);
					}

					*data = oldtile;
				} else {
					*data = editor.map.allocator(location);
					if (newtile->getHouseID() != 0) {
						// oooooomggzzz we need to add it to the appropriate house!
						House* house = editor.map.houses.getHouse(newtile->getHouseID());
						if (house) {
							house->addTile(newtile);
						}
					}

					if (newtile->spawn) {
						editor.map.addSpawn(newtile);
					}
				}
				// Mark the tile as modified
				newtile->modify();

				// Update client dirty list
				if (editor.IsLiveClient() && dirty_list && type != ACTION_REMOTE) {
					// Local action, assemble changes
					dirty_list->AddChange(c);
				}
				break;
			}

			case CHANGE_MOVE_HOUSE_EXIT: {
				std::pair<uint32_t, Position>* p = reinterpret_cast<std::pair<uint32_t, Position>*>(c->data);
				ASSERT(p);
				House* whathouse = editor.map.houses.getHouse(p->first);

				if (whathouse) {
					Position oldpos = whathouse->getExit();
					whathouse->setExit(p->second);
					p->second = oldpos;
				}
				break;
			}

			case CHANGE_MOVE_WAYPOINT: {
				std::pair<std::string, Position>* p = reinterpret_cast<std::pair<std::string, Position>*>(c->data);
				ASSERT(p);
				Waypoint* wp = editor.map.waypoints.getWaypoint(p->first);

				if (wp) {
					// Change the tiles
					TileLocation* oldtile = editor.map.getTileL(wp->pos);
					TileLocation* newtile = editor.map.getTileL(p->second);

					// Only need to remove from old if it actually exists
					if (p->second != Position()) {
						if (oldtile && oldtile->getWaypointCount() > 0) {
							oldtile->decreaseWaypointCount();
						}
					}

					newtile->increaseWaypointCount();

					// Update shit
					Position oldpos = wp->pos;
					wp->pos = p->second;
					p->second = oldpos;
				}
				break;
			}

			default:
				break;
		}
		++it;
	}
	editor.selection.finish(Selection::INTERNAL);
	commited = true;
}

void Action::undo(DirtyList* dirty_list) {
	if (changes.empty()) {
		return;
	}

	editor.selection.start(Selection::INTERNAL);
	ChangeList::reverse_iterator it = changes.rbegin();

	while (it != changes.rend()) {
		Change* c = *it;
		switch (c->type) {
			case CHANGE_TILE: {
				void** data = &c->data;
				Tile* oldtile = reinterpret_cast<Tile*>(*data);
				ASSERT(oldtile);
				Position pos = oldtile->getPosition();

				if (editor.IsLiveClient()) {
					QTreeNode* nd = editor.map.getLeaf(pos.x, pos.y);
					if (!nd || !nd->isVisible(pos.z > GROUND_LAYER)) {
						// Delete all changes that affect tiles outside our view
						c->clear();
						++it;
						continue;
					}
				}

				Tile* newtile = editor.map.swapTile(pos, oldtile);

				// Update server side change list (for broadcast)
				if (editor.IsLiveServer() && dirty_list) {
					dirty_list->AddPosition(pos.x, pos.y, pos.z);
				}

				if (oldtile->isSelected()) {
					editor.selection.addInternal(oldtile);
				}
				if (newtile->isSelected()) {
					editor.selection.removeInternal(newtile);
				}

				if (newtile->getHouseID() != oldtile->getHouseID()) {
					// oooooomggzzz we need to remove it from the appropriate house!
					House* house = editor.map.houses.getHouse(newtile->getHouseID());
					if (house) {
						house->removeTile(newtile);
					} else {
						// Set tile house to 0, house has been removed
						newtile->setHouse(nullptr);
					}

					house = editor.map.houses.getHouse(oldtile->getHouseID());
					if (house) {
						house->addTile(oldtile);
					}
				}

				if (oldtile->spawn) {
					if (newtile->spawn) {
						if (*oldtile->spawn != *newtile->spawn) {
							editor.map.removeSpawn(newtile);
							editor.map.addSpawn(oldtile);
						}
					} else {
						editor.map.addSpawn(oldtile);
					}
				} else if (newtile->spawn) {
					editor.map.removeSpawn(newtile);
				}
				*data = newtile;

				// Update client dirty list
				if (editor.IsLiveClient() && dirty_list && type != ACTION_REMOTE) {
					// Local action, assemble changes
					dirty_list->AddChange(c);
				}
				break;
			}

			case CHANGE_MOVE_HOUSE_EXIT: {
				std::pair<uint32_t, Position>* p = reinterpret_cast<std::pair<uint32_t, Position>*>(c->data);
				ASSERT(p);
				House* whathouse = editor.map.houses.getHouse(p->first);
				if (whathouse) {
					Position oldpos = whathouse->getExit();
					whathouse->setExit(p->second);
					p->second = oldpos;
				}
				break;
			}

			case CHANGE_MOVE_WAYPOINT: {
				std::pair<std::string, Position>* p = reinterpret_cast<std::pair<std::string, Position>*>(c->data);
				ASSERT(p);
				Waypoint* wp = editor.map.waypoints.getWaypoint(p->first);

				if (wp) {
					// Change the tiles
					TileLocation* oldtile = editor.map.getTileL(wp->pos);
					TileLocation* newtile = editor.map.getTileL(p->second);

					// Only need to remove from old if it actually exists
					if (p->second != Position()) {
						if (oldtile && oldtile->getWaypointCount() > 0) {
							oldtile->decreaseWaypointCount();
						}
					}

					if (newtile) {
						newtile->increaseWaypointCount();
					}

					// Update shit
					Position oldpos = wp->pos;
					wp->pos = p->second;
					p->second = oldpos;
				}
				break;
			}

			default:
				break;
		}
		++it;
	}
	editor.selection.finish(Selection::INTERNAL);
	commited = false;
}

BatchAction::BatchAction(Editor& editor, ActionIdentifier ident) :
	editor(editor),
	timestamp(0),
	memory_size(0),
	type(ident) {
	////
}

BatchAction::~BatchAction() {
	try {
		// Make a copy of the batch to avoid modifying while iterating
		ActionVector batchCopy;
		batchCopy.swap(batch); // Swap is safer than copying
		
		// Now safely delete each action in the copy
		for (ActionVector::iterator it = batchCopy.begin(); it != batchCopy.end(); ++it) {
			try {
				Action* action = *it;
				if (action) {
					delete action;
				}
			} catch (const std::exception& e) {
				// Log error but continue cleanup
				std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error in BatchAction destructor: " << e.what() << "\n";
					logFile.close();
				}
			}
		}
		
		// Ensure batch is empty (should already be from the swap)
		batch.clear();
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Critical error in BatchAction destructor: " << e.what() << "\n";
			logFile.close();
		}
	}
}

size_t BatchAction::memsize(bool recalc) const {
	// Expensive operation, only evaluate once (won't change anyways)
	if (!recalc && memory_size > 0) {
		return memory_size;
	}

	uint32_t mem = sizeof(*this);
	mem += sizeof(Action*) * 3 * batch.size();

	for (Action* action : batch) {
#ifdef __USE_EXACT_MEMSIZE__
		mem += action->memsize();
#else
		// Less exact but MUCH faster
		mem += action->approx_memsize();
#endif
	}

	const_cast<BatchAction*>(this)->memory_size = mem;
	return mem;
}

void BatchAction::addAction(Action* action) {
	try {
		// Safety check for null action
		if (!action) {
			return;
		}
		
		// If empty, do nothing.
		if (action->size() == 0) {
			delete action;
			return;
		}

		ASSERT(action->getType() == type);

		if (!editor.CanEdit()) {
			delete action;
			return;
		}

		// Add it!
		batch.push_back(action);
		timestamp = time(nullptr);
	}
	catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in BatchAction::addAction: " << e.what() << "\n";
			logFile.close();
		}
		
		// Try to clean up action if possible
		try {
			delete action;
		}
		catch (...) {
			// Ignore cleanup errors
		}
	}
}

void BatchAction::addAndCommitAction(Action* action) {
	// If empty, do nothing.
	if (action->size() == 0) {
		delete action;
		return;
	}

	if (!editor.CanEdit()) {
		delete action;
		return;
	}

	// Add it!
	action->commit(nullptr);
	batch.push_back(action);
	timestamp = time(nullptr);
}

void BatchAction::commit() {
	for (Action* action : batch) {
		if (!action->isCommited()) {
			action->commit(nullptr);
		}
	}
}

void BatchAction::undo() {
	for (Action* action : boost::adaptors::reverse(batch)) {
		action->undo(nullptr);
	}
}

void BatchAction::redo() {
	for (Action* action : batch) {
		action->redo(nullptr);
	}
}

void BatchAction::merge(BatchAction* other) {
	try {
		// Safety check for null pointer
		if (!other) {
			return;
		}
		
		// Reserve space to avoid multiple reallocations
		batch.reserve(batch.size() + other->batch.size());
		
		// Move actions from other batch to this one
		for (ActionVector::iterator it = other->batch.begin(); it != other->batch.end(); /* no increment here */) {
			Action* action = *it;
			it = other->batch.erase(it); // Remove from source vector before adding to destination
			if (action) {
				batch.push_back(action);
			}
		}
		
		// Clear the other batch's vector (should already be empty)
		other->batch.clear();
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error merging batch actions: " << e.what() << "\n";
			logFile.close();
		}
	}
}

ActionQueue::ActionQueue(Editor& editor) :
	current(0), memory_size(0), editor(editor) {
	////
}

ActionQueue::~ActionQueue() {
	try {
		for (auto it = actions.begin(); it != actions.end(); /* no increment here */) {
			BatchAction* action = *it;
			it = actions.erase(it); // Remove from container before deleting
			
			try {
				delete action;
			} catch (const std::exception& e) {
				// Log error but continue cleanup
				std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error deleting action in queue destructor: " << e.what() << "\n";
					logFile.close();
				}
			}
		}
		actions.clear(); // Ensure container is empty
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in ActionQueue destructor: " << e.what() << "\n";
			logFile.close();
		}
	}
}

Action* ActionQueue::createAction(ActionIdentifier ident) {
	return newd Action(editor, ident);
}

Action* ActionQueue::createAction(BatchAction* batch) {
	return newd Action(editor, batch->getType());
}

BatchAction* ActionQueue::createBatch(ActionIdentifier ident) {
	return newd BatchAction(editor, ident);
}

void ActionQueue::resetTimer() {
	if (!actions.empty()) {
		actions.back()->resetTimer();
	}
}

void ActionQueue::addAction(Action* action, int stacking_delay) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		// Use CallAfter with a copy of parameters to prevent race conditions
		Action* actionPtr = action;
		int delay = stacking_delay;
		wxTheApp->CallAfter([this, actionPtr, delay]() {
			this->addAction(actionPtr, delay);
		});
		return;
	}

	// Safety check for null action
	if (!action) {
		return;
	}

	try {
		// If empty, do nothing.
		if (action->size() == 0) {
			delete action;
			return;
		}

		BatchAction* batch = createBatch(action->getType());
		if (!batch) {
			delete action;
			return;
		}

		try {
			batch->addAndCommitAction(action);
			
			// If batch is empty after adding action, clean up and return
			if (batch->size() == 0) {
				delete batch;
				return;
			}

			addBatch(batch, stacking_delay);
		}
		catch (const std::exception& e) {
			// Log error but don't crash
			std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
			if (logFile.is_open()) {
				wxDateTime now = wxDateTime::Now();
				logFile << now.FormatISOCombined() << ": Error in addAction: " << e.what() << "\n";
				logFile.close();
			}
			
			// Clean up if exception occurred
			delete batch;
		}
	}
	catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Critical error in addAction: " << e.what() << "\n";
			logFile.close();
		}
		
		// Try to clean up action if possible
		try {
			delete action;
		}
		catch (...) {
			// Ignore cleanup errors
		}
	}
}

void ActionQueue::addBatch(BatchAction* batch, int stacking_delay) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		// Use CallAfter but with a 'copy' of needed parameters
		// to prevent race conditions
		ActionQueue* self = this;
		int delay = stacking_delay;
		wxTheApp->CallAfter([self, batch, delay]() {
			self->addBatch(batch, delay);
		});
		return;
	}

	// Safety check - if batch is null, just return
	if (!batch) {
		return;
	}

	// Additional safety check to catch any potential crashes
	try {
		ASSERT(current <= actions.size());

		if (batch->size() == 0) {
			delete batch;
			return;
		}

		// Commit any uncommited actions...
		batch->commit();

		// Update title
		if (editor.map.doChange()) {
			// Use a safer version that doesn't trigger UI updates
			// during the first drawing operation
			static bool isFirstOperation = true;
			if (!isFirstOperation) {
				g_gui.UpdateTitle();
			} else {
				isFirstOperation = false;
			}
		}

		// Special handling for remote actions
		if (batch->type == ACTION_REMOTE) {
			try {
				// For remote actions, we need to be extra careful
				// First clear the batch to ensure no dangling pointers
				ActionVector batchCopy;
				
				// Swap the batch contents to avoid issues during deletion
				batchCopy.swap(batch->batch);
				
				// Now safely delete the batch (which should be empty)
				delete batch;
				
				// Then safely delete each action in the copy
				for (Action* action : batchCopy) {
					try {
						if (action) {
							delete action;
						}
					} catch (const std::exception& e) {
						// Log error but continue cleanup
						std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
						if (logFile.is_open()) {
							wxDateTime now = wxDateTime::Now();
							logFile << now.FormatISOCombined() << ": Error deleting remote action: " << e.what() << "\n";
							logFile.close();
						}
					}
				}
			} catch (const std::exception& e) {
				// Log error but don't crash
				std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error handling remote batch: " << e.what() << "\n";
					logFile.close();
				}
			}
			return;
		}

		// Protect against memory corruption when clearing redo history
		while (current < actions.size()) {
			try {
				if (actions.empty()) break;
				
				BatchAction* todelete = actions.back();
				actions.pop_back(); // Remove from container before deleting to avoid invalid references
				
				if (todelete) {
					memory_size -= todelete->memsize();
					delete todelete;
				}
			} catch (const std::exception& e) {
				// Log error but continue processing
				std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error clearing action: " << e.what() << "\n";
					logFile.close();
				}
			}
		}

		// Safely manage memory for undo size limits
		try {
			// Limit by memory size
			while (memory_size > size_t(1024 * 1024 * g_settings.getInteger(Config::UNDO_MEM_SIZE)) && !actions.empty()) {
				BatchAction* todelete = actions.front();
				actions.pop_front(); // Remove from container before accessing or deleting
				
				if (todelete) {
					memory_size -= todelete->memsize();
					delete todelete;
				}
				
				if (current > 0) {
					current--;
				}
			}

			// Limit by action count
			while (actions.size() > size_t(g_settings.getInteger(Config::UNDO_SIZE)) && !actions.empty()) {
				BatchAction* todelete = actions.front();
				actions.pop_front(); // Remove from container before accessing or deleting
				
				if (todelete) {
					memory_size -= todelete->memsize();
					delete todelete;
				}
				
				if (current > 0) {
					current--;
				}
			}

			// Process action with additional safety
			bool actionMerged = false;
			if (!actions.empty()) {
				BatchAction* lastAction = actions.back();
				if (lastAction && batch && 
					lastAction->type == batch->type && 
					g_settings.getInteger(Config::GROUP_ACTIONS) && 
					time(nullptr) - stacking_delay < lastAction->timestamp) {
					
					// Save current memory size before merge
					memory_size -= lastAction->memsize();
					
					// Perform the merge
					lastAction->merge(batch);
					lastAction->timestamp = time(nullptr);
					
					// Update memory size after merge
					memory_size += lastAction->memsize(true);
					delete batch;
					actionMerged = true;
				}
			}
			
			// Add as new action if not merged
			if (!actionMerged) {
				memory_size += batch->memsize();
				actions.push_back(batch);
				batch->timestamp = time(nullptr);
				current++;
			}
		} catch (const std::exception& e) {
			// Log error but don't crash
			std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
			if (logFile.is_open()) {
				wxDateTime now = wxDateTime::Now();
				logFile << now.FormatISOCombined() << ": Error processing batch: " << e.what() << "\n";
				logFile.close();
			}
			
			// If we caught an exception and haven't added or deleted the batch yet, delete it now
			if (batch) {
				delete batch;
			}
		}
	} catch (const std::exception& e) {
		// Last resort error handler for entire function
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Critical error in addBatch: " << e.what() << "\n";
			logFile.close();
		}
		
		// Clean up batch if we haven't processed it yet
		if (batch) {
			delete batch;
		}
	}
}

void ActionQueue::undo() {
	try {
		if (current > 0 && current <= actions.size()) {
			current--;
			BatchAction* batch = actions[current];
			if (batch) {
				batch->undo();
			}
		}
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in undo(): " << e.what() << "\n";
			logFile.close();
		}
	}
}

void ActionQueue::redo() {
	try {
		if (current < actions.size()) {
			BatchAction* batch = actions[current];
			if (batch) {
				batch->redo();
			}
			current++;
		}
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in redo(): " << e.what() << "\n";
			logFile.close();
		}
	}
}

void ActionQueue::clear() {
	try {
		// Make a copy of the actions to avoid modifying the container while iterating
		ActionList actionsCopy = actions;
		
		// Clear the original container first to prevent double deletions
		actions.clear();
		current = 0;
		memory_size = 0;
		
		// Now safely delete each action
		for (BatchAction* action : actionsCopy) {
			try {
				if (action) {
					delete action;
				}
			} catch (const std::exception& e) {
				// Log error but continue cleanup
				std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error in clear() deleting action: " << e.what() << "\n";
					logFile.close();
				}
			}
		}
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Critical error in clear(): " << e.what() << "\n";
			logFile.close();
		}
		
		// Reset state as much as possible
		actions.clear();
		current = 0;
		memory_size = 0;
	}
}

DirtyList::DirtyList() :
	owner(0) {
	;
}

DirtyList::~DirtyList() {
	;
}

void DirtyList::AddPosition(int x, int y, int z) {
	uint32_t m = ((x >> 2) << 18) | ((y >> 2) << 4);
	ValueType fi = { m, 0 };
	SetType::iterator s = iset.find(fi);
	if (s != iset.end()) {
		ValueType v = *s;
		iset.erase(s);
		v.floors = (1 << z) | v.floors;
		iset.insert(v);
	} else {
		ValueType v = { m, (uint32_t)(1 << z) };
		iset.insert(v);
	}
}

void DirtyList::AddChange(Change* c) {
	ichanges.push_back(c);
}

DirtyList::SetType& DirtyList::GetPosList() {
	return iset;
}

ChangeList& DirtyList::GetChanges() {
	return ichanges;
}
