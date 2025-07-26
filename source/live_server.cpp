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

#include "live_server.h"
#include "live_peer.h"
#include "live_tab.h"
#include "live_action.h"

#include "editor.h"

#include <fstream>
#include <wx/filename.h>

LiveServer::LiveServer(Editor& editor) :
	LiveSocket(),
	clients(), acceptor(nullptr), socket(nullptr), editor(&editor),
	clientIds(0), port(0), stopped(false), drawingReady(false) {
	// Initialize with a safe color
	usedColor = wxColor(255, 0, 0); // Red for host
	
	// Log server initialization to file
	std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_init.log").ToStdString(), std::ios::app);
	if (logFile.is_open()) {
		wxDateTime now = wxDateTime::Now();
		logFile << now.FormatISOCombined() << ": LiveServer initialized\n";
		logFile.close();
	}
	
	// Set the drawing ready flag after a short delay to ensure all initialization is complete
	wxTheApp->CallAfter([this]() {
		drawingReady = true;
		
		// Log to file
		std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_status.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Server drawing ready flag set\n";
			logFile.close();
		}
	});
}

LiveServer::~LiveServer() {
	//
}

bool LiveServer::bind() {
	// Ensure we're on the main thread for initialization
	if (!wxThread::IsMain()) {
		bool success = false;
		wxTheApp->CallAfter([this, &success]() {
			success = this->bind();
		});
		return success;
	}

	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(service);

	// Try to bind to the specified port, if that fails, try the next port
	// This allows multiple instances to host simultaneously
	uint16_t originalPort = port;
	uint16_t maxPortRetries = 10; // Try up to 10 ports in sequence
	boost::system::error_code bindError;
	
	for (uint16_t retry = 0; retry < maxPortRetries; ++retry) {
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
		
		// Close acceptor if it was previously opened
		if (acceptor->is_open()) {
			acceptor->close();
		}
		
		// Try to open and bind
		acceptor->open(endpoint.protocol());
		
		boost::system::error_code error;
		acceptor->set_option(boost::asio::ip::tcp::no_delay(true), error);
		if (error) {
			logMessage(wxString::Format("Warning: Could not set no_delay option: %s", error.message()));
		}
		
		// Try binding to endpoint
		bindError.clear();
		acceptor->bind(endpoint, bindError);
		
		if (!bindError) {
			// Binding successful
			if (port != originalPort) {
				// Log that we're using a different port
				logMessage(wxString::Format("Port %d was in use, using port %d instead", originalPort, port));
			}
			break;
		}
		
		// If binding failed, try next port
		port++;
	}
	
	if (bindError) {
		setLastError("Error binding socket: " + bindError.message() + 
			"\nTried ports " + std::to_string(originalPort) + " to " + 
			std::to_string(originalPort + maxPortRetries - 1));
		return false;
	}

	acceptor->listen();
	acceptClient();
	return true;
}

void LiveServer::close() {
	// Set stopped flag first to prevent any new operations
	stopped = true;
	
	// Also disable drawing operations
	drawingReady = false;
	
	// Log server shutdown to file
	std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_status.log").ToStdString(), std::ios::app);
	if (logFile.is_open()) {
		wxDateTime now = wxDateTime::Now();
		logFile << now.FormatISOCombined() << ": Server shutting down\n";
		logFile.close();
	}
	
	// Then proceed with normal shutdown
	for (auto& clientEntry : clients) {
		delete clientEntry.second;
	}
	clients.clear();

	if (log) {
		log->Message("Server was shutdown.");
		log->Disconnect();
		log = nullptr;
	}

	if (acceptor) {
		acceptor->close();
	}

	if (socket) {
		socket->close();
	}
}

void LiveServer::acceptClient() {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this]() {
			this->acceptClient();
		});
		return;
	}

	static uint32_t id = 0;
	if (stopped) {
		return;
	}

	if (!socket) {
		socket = std::make_shared<boost::asio::ip::tcp::socket>(
			NetworkConnection::getInstance().get_service()
		);
	}

	acceptor->async_accept(*socket, [this](const boost::system::error_code& error) -> void {
		// Queue the client handling on the main thread
		wxTheApp->CallAfter([this, error]() {
			if (!error) {
				static uint32_t nextId = 0;
				LivePeer* peer = new LivePeer(this, std::move(*socket));
				peer->log = log;
				peer->receiveHeader();

				clients.insert(std::make_pair(nextId++, peer));
				
				// Make sure the host's cursor exists
				if (cursors.find(0) == cursors.end()) {
					LiveCursor hostCursor;
					hostCursor.id = 0;
					hostCursor.color = usedColor;
					hostCursor.pos = Position();
					cursors[0] = hostCursor;
				}
				
				updateClientList();
			}
			// Queue next accept regardless of error
			acceptClient();
		});
	});
}

void LiveServer::removeClient(uint32_t id) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, id]() {
			this->removeClient(id);
		});
		return;
	}

	auto it = clients.find(id);
	if (it == clients.end()) {
		return;
	}

	const uint32_t clientId = it->second->getClientId();
	if (clientId != 0) {
		clientIds &= ~clientId;
		editor->map.clearVisible(clientIds);
	}

	clients.erase(it);
	updateClientList();
}

void LiveServer::updateCursor(const Position& position) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, position]() {
			this->updateCursor(position);
		});
		return;
	}

	LiveCursor cursor;
	cursor.id = 0;
	cursor.pos = position;
	cursor.color = usedColor;
	
	this->cursors[cursor.id] = cursor;
	this->broadcastCursor(cursor);
	
	g_gui.RefreshView();
}

void LiveServer::updateClientList() const {
	log->UpdateClientList(clients);
}

uint16_t LiveServer::getPort() const {
	return port;
}

bool LiveServer::setPort(int32_t newPort) {
	if (newPort < 1 || newPort > 65535) {
		setLastError("Port must be a number in the range 1-65535.");
		return false;
	}
	port = newPort;
	return true;
}

uint32_t LiveServer::getFreeClientId() {
	for (int32_t bit = 1; bit < (1 << 16); bit <<= 1) {
		if (!testFlags(clientIds, bit)) {
			clientIds |= bit;
			return bit;
		}
	}
	return 0;
}

std::string LiveServer::getHostName() const {
	if (acceptor) {
		auto endpoint = acceptor->local_endpoint();
		return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
	}
	return "localhost";
}

void LiveServer::broadcastNodes(DirtyList& dirtyList) {
	// Skip if we're not ready for drawing operations
	if (!drawingReady || stopped) {
		// Log to file instead of UI
		std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_status.log").ToStdString(), std::ios::app);
		if (logFile.is_open()) {
			wxDateTime now = wxDateTime::Now();
			logFile << now.FormatISOCombined() << ": Skipped broadcast, drawing not ready\n";
			logFile.close();
		}
		return;
	}

	// If there are no clients or no changes, there's nothing to do
	if (clients.empty() || dirtyList.Empty()) {
		return;
	}

	// Log to file rather than UI
	std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_ops.log").ToStdString(), std::ios::app);
	if (logFile.is_open()) {
		wxDateTime now = wxDateTime::Now();
		logFile << now.FormatISOCombined() << ": Broadcasting changes to " << clients.size() << " clients\n";
	}

	// Extract the change information to a struct we can capture in our lambda
	struct BroadcastData {
		uint32_t owner;
		std::vector<Change*> changes;  // Changed from ChangeList to std::vector<Change*>
		std::vector<DirtyList::ValueType> positions;  // Use the existing type from DirtyList
	};

	std::shared_ptr<BroadcastData> broadcastData = std::make_shared<BroadcastData>();
	broadcastData->owner = dirtyList.owner; // Set the correct owner
	
	try {
		// Get the changes from the dirty list, converting them to a format we can save and broadcast
		// Note: We need to create deep copies of the changes as the dirty list will be cleared
		ChangeList& changeList = dirtyList.GetChanges();
		broadcastData->changes.reserve(changeList.size());
		
		for (Change* change : changeList) {
			if (!change) continue;
			
			// Store a pointer - no unique_ptr here
			if (change->getType() == CHANGE_TILE) {
				// Create a deep copy of the change
				Tile* oldTile = static_cast<Tile*>(change->getData());
				if (oldTile) {
					Tile* newTile = oldTile->deepCopy(editor->map);
					Change* newChange = new Change(newTile);
					broadcastData->changes.push_back(newChange);
				}
			}
		}
		
		// Get position list - need to copy the positions manually
		DirtyList::SetType& positionList = dirtyList.GetPosList();
		for (const auto& pos : positionList) {
			broadcastData->positions.push_back(pos);
		}
		
		// Safe logging of the node count
		if (logFile.is_open()) {
			logFile << "Changes: " << broadcastData->changes.size() << ", Positions: " << broadcastData->positions.size() << "\n";
			logFile << "Change owner: " << broadcastData->owner << "\n";
		}
		
		// Use a safer approach with CallAfter
		wxTheApp->CallAfter([this, broadcastData]() {
			try {
				if (!editor || !drawingReady) return;

				// Apply changes to the host if the owner is not the host
				if (broadcastData->owner != 0 && !broadcastData->changes.empty()) {
					NetworkedAction* action = static_cast<NetworkedAction*>(editor->actionQueue->createAction(ACTION_REMOTE));
					if (action) {
						action->owner = broadcastData->owner;
						
						// Add the changes to the action - action takes ownership
						for (Change* change : broadcastData->changes) {
							if (change) {
								action->addChange(change);
							}
						}
						
						// Add the action to the queue
						editor->actionQueue->addAction(action, 0);
					}
				}

				// Now handle the network broadcasting
				// Create a vector of all the work we need to do
				struct BroadcastWork {
					LivePeer* peer;
					QTreeNode* node;
					int32_t ndx;
					int32_t ndy;
					uint32_t floors;
					uint32_t clientId;
				};
				
				std::vector<BroadcastWork> workItems;

				// First gather all the work without doing any actual sending
				for (const auto& ind : broadcastData->positions) {
					int32_t ndx = ind.pos >> 18;
					int32_t ndy = (ind.pos >> 4) & 0x3FFF;
					uint32_t floors = ind.floors;

					QTreeNode* node = editor->map.getLeaf(ndx * 4, ndy * 4);
					if (!node) continue;

					for (auto& clientEntry : clients) {
						LivePeer* peer = clientEntry.second;
						if (!peer) continue;

						const uint32_t clientId = peer->getClientId();
						
						// Skip sending changes back to the client that made them
						if (broadcastData->owner != 0 && clientId == broadcastData->owner) {
							continue;
						}
						
						// Always broadcast to all clients, regardless of visibility
						// This ensures all clients see all changes
						workItems.push_back({peer, node, ndx, ndy, floors, clientId});
						
						// Mark the node as visible to this client to ensure future updates are sent
						node->setVisible(clientId, true, true);
						node->setVisible(clientId, false, true);
					}
				}

				// If we have work to do, process it all in a single batch
				if (!workItems.empty()) {
					// Process work items in batches to avoid overwhelming the network
					const size_t batchSize = 100; // Process 100 work items at a time
					
					for (size_t startIdx = 0; startIdx < workItems.size(); startIdx += batchSize) {
						size_t endIdx = std::min(startIdx + batchSize, workItems.size());
						
						for (size_t i = startIdx; i < endIdx; ++i) {
							const auto& work = workItems[i];
							if (work.peer && work.node) {
								sendNode(work.clientId, work.node, work.ndx, work.ndy, work.floors);
							}
						}
					}
					
					// Log completion to file
					std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_ops.log").ToStdString(), std::ios::app);
					if (logFile.is_open()) {
						wxDateTime now = wxDateTime::Now();
						logFile << now.FormatISOCombined() << ": Broadcast completed, sent " << workItems.size() << " node updates\n";
						logFile.close();
					}
				}
			} catch (std::exception& e) {
				// Log error to file
				std::ofstream logFile((GetAppDir() + wxFileName::GetPathSeparator() + "server_error.log").ToStdString(), std::ios::app);
				if (logFile.is_open()) {
					wxDateTime now = wxDateTime::Now();
					logFile << now.FormatISOCombined() << ": Error broadcasting nodes: " << e.what() << "\n";
					logFile.close();
				}
			}
		});
		
		if (logFile.is_open()) {
			logFile << "Broadcast queued to main thread\n";
			logFile.close();
		}
	} catch (std::exception& e) {
		// Log error to file
		if (logFile.is_open()) {
			logFile << "Error preparing broadcast: " << e.what() << "\n";
			logFile.close();
		} else {
			std::ofstream errorLog((GetAppDir() + wxFileName::GetPathSeparator() + "server_error.log").ToStdString(), std::ios::app);
			if (errorLog.is_open()) {
				wxDateTime now = wxDateTime::Now();
				errorLog << now.FormatISOCombined() << ": Error preparing broadcast: " << e.what() << "\n";
				errorLog.close();
			}
		}
	}
}

void LiveServer::broadcastCursor(const LiveCursor& cursor) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, cursor]() {
			this->broadcastCursor(cursor);
		});
		return;
	}

	if (clients.empty()) {
		return;
	}

	// Update the cursor in our storage without logging each movement
	cursors[cursor.id] = cursor;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CURSOR_UPDATE);
	writeCursor(message, cursor);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		peer->send(message);
	}
	
	g_gui.RefreshView();
}

void LiveServer::broadcastChat(const wxString& speaker, const wxString& chatMessage) {
	// Ensure we're on the main thread
	if (!wxThread::IsMain()) {
		wxTheApp->CallAfter([this, speaker, chatMessage]() {
			this->broadcastChat(speaker, chatMessage);
		});
		return;
	}

	if (clients.empty()) {
		return;
	}

	wxString displayName = (speaker == "HOST") ? name : speaker;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_SERVER_TALK);
	message.write<std::string>(nstr(displayName));
	message.write<std::string>(nstr(chatMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}

	if (log) {
		log->Chat(displayName, chatMessage);
	}
}

void LiveServer::sendChat(const wxString& chatMessage) {
	// For server, sending a chat message means broadcasting it from HOST
	broadcastChat("HOST", chatMessage);
}

void LiveServer::startOperation(const wxString& operationMessage) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_START_OPERATION);
	message.write<std::string>(nstr(operationMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

void LiveServer::updateOperation(int32_t percent) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_UPDATE_OPERATION);
	message.write<uint32_t>(percent);

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

LiveLogTab* LiveServer::createLogWindow(wxWindow* parent) {
	MapTabbook* mapTabBook = dynamic_cast<MapTabbook*>(parent);
	ASSERT(mapTabBook);

	log = newd LiveLogTab(mapTabBook, this);
	log->Message("New Live mapping session started.");
	log->Message("Hosted on server " + getHostName() + ".");

	updateClientList();
	return log;
}

void LiveServer::broadcastColorChange(uint32_t clientId, const wxColor& color) {
	if (clients.empty()) {
		return;
	}

	// Prepare the color update packet
	NetworkMessage message;
	message.write<uint8_t>(PACKET_COLOR_UPDATE);
	message.write<uint32_t>(clientId);
	message.write<uint8_t>(color.Red());
	message.write<uint8_t>(color.Green());
	message.write<uint8_t>(color.Blue());
	message.write<uint8_t>(color.Alpha());

	// Log the color change for debugging
	logMessage(wxString::Format("[Server]: Broadcasting color change for client %u: RGB(%d,%d,%d)", 
		clientId, color.Red(), color.Green(), color.Blue()));

	// Send to all clients
	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}

	// Update client list in all open log tabs
	updateClientList();
}

void LiveServer::setUsedColor(const wxColor& color) {
	usedColor = color;
	
	// Broadcast the host's color change (host always has clientId 0)
	broadcastColorChange(0, color);
}
