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

#ifndef _RME_LIVE_CLIENT_H_
#define _RME_LIVE_CLIENT_H_

#include "live_socket.h"
#include "net_connection.h"

#include <set>

class DirtyList;
class MapTab;

class LiveClient : public LiveSocket {
public:
	LiveClient();
	~LiveClient();

	//
	bool connect(const std::string& address, uint16_t port);
	void tryConnect(boost::asio::ip::tcp::resolver::results_type::iterator endpoint);

	void close();
	bool handleError(const boost::system::error_code& error);

	//
	std::string getHostName() const;

	//
	void receiveHeader() override;
	void receive(uint32_t packetSize) override;
	void send(NetworkMessage& message) override;
	void sendWithoutLogging(NetworkMessage& message); // For cursor updates and other high-frequency events

	//
	void updateCursor(const Position& position) override;

	LiveLogTab* createLogWindow(wxWindow* parent);

	
	MapTab* createEditorWindow();

	// Override socket type check
	bool IsClient() const override { return true; }


	// send packets
	void sendHello();
	void sendNodeRequests();
	void sendChanges(DirtyList& dirtyList);
	void sendChat(const wxString& chatMessage) override;
	void sendReady();
	void sendColorUpdate(uint32_t targetClientId, const wxColor& color);
	
	// Request a refresh of the visible area from the server
	void requestVisibleRefresh();

	// Flags a node as queried and stores it, need to call SendNodeRequest to send it to server
	void queryNode(int32_t ndx, int32_t ndy, bool underground);

protected:
	void parsePacket(NetworkMessage message);

	// parse packets
	void parseHello(NetworkMessage& message);
	void parseKick(NetworkMessage& message);
	void parseClientAccepted(NetworkMessage& message);
	void parseChangeClientVersion(NetworkMessage& message);
	void parseServerTalk(NetworkMessage& message);
	void parseNode(NetworkMessage& message);
	void parseCursorUpdate(NetworkMessage& message);
	void parseStartOperation(NetworkMessage& message);
	void parseUpdateOperation(NetworkMessage& message);
	void parseColorUpdate(NetworkMessage& message);

	//
	NetworkMessage readMessage;

	std::set<uint32_t> queryNodeList;
	wxString currentOperation;

	std::shared_ptr<boost::asio::ip::tcp::resolver> resolver;
	std::shared_ptr<boost::asio::ip::tcp::socket> socket;

	Editor* editor;

	bool stopped;
	bool isDrawingReady; // Flag indicating client is ready to handle drawing operations
};

#endif
