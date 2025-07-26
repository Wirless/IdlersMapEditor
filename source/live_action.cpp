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

#include "live_action.h"
#include "editor.h"

NetworkedAction::NetworkedAction(Editor& editor, ActionIdentifier ident) :
	Action(editor, ident),
	owner(0) {
	;
}

NetworkedAction::~NetworkedAction() {
	try {
		// Nothing specific to clean up here, base class will handle the changes
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedAction destructor: " << e.what() << "\n";
			logFile.close();
		}
	}
}

NetworkedBatchAction::NetworkedBatchAction(Editor& editor, NetworkedActionQueue& queue, ActionIdentifier ident) :
	BatchAction(editor, ident),
	queue(queue) {
	;
}

NetworkedBatchAction::~NetworkedBatchAction() {
	// The base class BatchAction destructor will handle the actual cleanup
	// This is just a safety measure to ensure we don't have any issues specific to NetworkedBatchAction
	try {
		// Nothing specific to clean up here, base class will handle the batch
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedBatchAction destructor: " << e.what() << "\n";
			logFile.close();
		}
	}
}

void NetworkedBatchAction::addAndCommitAction(Action* action) {
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

		// Track changed nodes...
		DirtyList dirty_list;
		NetworkedAction* netact = dynamic_cast<NetworkedAction*>(action);
		if (netact) {
			dirty_list.owner = netact->owner;
		}

		// Add it!
		try {
			action->commit(type != ACTION_SELECT ? &dirty_list : nullptr);
			batch.push_back(action);
			timestamp = time(nullptr);
		}
		catch (const std::exception& e) {
			// Log error and clean up
			std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
			if (logFile.is_open()) {
				wxDateTime now = wxDateTime::Now();
				logFile << now.FormatISOCombined() << ": Error committing action: " << e.what() << "\n";
				logFile.close();
			}
			
			// Clean up action if it wasn't added to batch
			delete action;
			return;
		}

		// Broadcast changes to all clients!
		try {
			// Log that we're broadcasting changes
			std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_broadcast.log").ToStdString(), std::ios::app);
			if (logFile.is_open()) {
				wxDateTime now = wxDateTime::Now();
				logFile << now.FormatISOCombined() << ": Broadcasting changes from owner " << dirty_list.owner << "\n";
				logFile.close();
			}
			
			queue.broadcast(dirty_list);
		}
		catch (const std::exception& e) {
			// Log error but continue
			std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
			if (logFile.is_open()) {
				wxDateTime now = wxDateTime::Now();
				logFile << now.FormatISOCombined() << ": Error broadcasting changes: " << e.what() << "\n";
				logFile.close();
			}
		}
	}
	catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Critical error in addAndCommitAction: " << e.what() << "\n";
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

void NetworkedBatchAction::commit() {
	try {
		// Track changed nodes...
		DirtyList dirty_list;

		for (ActionVector::iterator it = batch.begin(); it != batch.end(); ++it) {
			NetworkedAction* action = static_cast<NetworkedAction*>(*it);
			if (action && !action->isCommited()) {
				action->commit(type != ACTION_SELECT ? &dirty_list : nullptr);
				if (action->owner != 0) {
					dirty_list.owner = action->owner;
				}
			}
		}
		
		// Log that we're broadcasting changes from commit
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_broadcast.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Broadcasting changes from commit, owner: " << dirty_list.owner << "\n";
			logFile.close();
		}
		
		// Broadcast changes to all clients!
		queue.broadcast(dirty_list);
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedBatchAction::commit(): " << e.what() << "\n";
			logFile.close();
		}
	}
}

void NetworkedBatchAction::undo() {
	try {
		// Track changed nodes...
		DirtyList dirty_list;

		for (ActionVector::reverse_iterator it = batch.rbegin(); it != batch.rend(); ++it) {
			Action* action = *it;
			if (action) {
				action->undo(type != ACTION_SELECT ? &dirty_list : nullptr);
			}
		}
		
		// Log that we're broadcasting changes from undo
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_broadcast.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Broadcasting changes from undo, owner: " << dirty_list.owner << "\n";
			logFile.close();
		}
		
		// Broadcast changes to all clients!
		queue.broadcast(dirty_list);
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedBatchAction::undo(): " << e.what() << "\n";
			logFile.close();
		}
	}
}

void NetworkedBatchAction::redo() {
	try {
		commit();
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedBatchAction::redo(): " << e.what() << "\n";
			logFile.close();
		}
	}
}

void NetworkedBatchAction::commitChanges(Action* action) {
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

		// Track changed nodes...
		DirtyList dirty_list;
		NetworkedAction* netact = dynamic_cast<NetworkedAction*>(action);
		if (netact) {
			dirty_list.owner = netact->owner;
		}

		// Commit the action
		action->commit(type != ACTION_SELECT ? &dirty_list : nullptr);
		
		// Log that we're broadcasting changes from commitChanges
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_broadcast.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Broadcasting changes from commitChanges, owner: " << dirty_list.owner << "\n";
			logFile.close();
		}
		
		// Broadcast changes to all clients!
		queue.broadcast(dirty_list);
		
		// Clean up the action - we don't store it
		delete action;
	}
	catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in commitChanges: " << e.what() << "\n";
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

//===================
// Action queue

NetworkedActionQueue::NetworkedActionQueue(Editor& editor) :
	ActionQueue(editor) {
}

NetworkedActionQueue::~NetworkedActionQueue() {
}

Action* NetworkedActionQueue::createAction(ActionIdentifier ident) {
	try {
		NetworkedAction* action = newd NetworkedAction(editor, ident);
		return action;
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error creating NetworkedAction: " << e.what() << "\n";
			logFile.close();
		}
		return nullptr;
	}
}

BatchAction* NetworkedActionQueue::createBatch(ActionIdentifier ident) {
	try {
		NetworkedBatchAction* batch = newd NetworkedBatchAction(editor, *this, ident);
		return batch;
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error creating NetworkedBatchAction: " << e.what() << "\n";
			logFile.close();
		}
		return nullptr;
	}
}

void NetworkedActionQueue::broadcast(DirtyList& dirty_list) {
	editor.BroadcastNodes(dirty_list);
}

void NetworkedActionQueue::commitChanges(Action* action) {
	try {
		// Create a temporary batch to handle the action
		NetworkedBatchAction* batch = static_cast<NetworkedBatchAction*>(createBatch(ACTION_REMOTE));
		if (!batch) {
			// If batch creation failed, clean up and return
			delete action;
			return;
		}
		
		// Use the batch to commit the changes
		batch->commitChanges(action);
		
		// The batch has already deleted the action, so we just need to delete the batch
		delete batch;
	} catch (const std::exception& e) {
		// Log error but don't crash
		std::ofstream logFile((wxStandardPaths::Get().GetUserDataDir() + wxFileName::GetPathSeparator() + "action_error.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Error in NetworkedActionQueue::commitChanges: " << e.what() << "\n";
			logFile.close();
		}
		
		// Try to clean up action if possible
		try {
			delete action;
		} catch (...) {
			// Ignore cleanup errors
		}
	}
}
