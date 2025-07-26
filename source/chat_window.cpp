#include "main.h"
#include "chat_window.h"
#include "settings.h"
#include "gui.h"
#include "json.h"

#include <wx/url.h>
#include <wx/stream.h>
#include <wx/sstream.h>
#include <wx/protocol/http.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>



// Custom event definitions
wxDEFINE_EVENT(wxEVT_CHAT_MESSAGE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHAT_USER_JOINED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHAT_USER_LEFT, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHAT_PRIVATE_MESSAGE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHAT_ERROR, wxCommandEvent);

// Event IDs
enum {
    ID_CHAT_SEND = 1000,
    ID_CHAT_MESSAGE_INPUT,
    ID_CHAT_USER_LIST,
    ID_CHAT_CONNECT,
    ID_CHAT_REGISTER,
    ID_CHAT_PRIVATE_MESSAGE,
    ID_REGISTER_OK,
    ID_REGISTER_CANCEL,
    ID_PM_SEND,
    ID_PM_MESSAGE_INPUT,
    ID_TYPING_TIMER,
    ID_RECONNECT_TIMER,
    ID_HEARTBEAT_TIMER
};

// Global chat window instance
static ChatWindow* g_chatWindow = nullptr;

//=============================================================================
// ChatService Implementation
//=============================================================================

wxBEGIN_EVENT_TABLE(ChatService, wxEvtHandler)
    EVT_SOCKET(wxID_ANY, ChatService::OnSocketEvent)
    EVT_TIMER(ID_RECONNECT_TIMER, ChatService::OnTimer)
    EVT_TIMER(ID_HEARTBEAT_TIMER, ChatService::OnTimer)
wxEND_EVENT_TABLE()

ChatService::ChatService() :
    connected(false),
    socket(nullptr),
    reconnectTimer(nullptr),
    heartbeatTimer(nullptr) {
    
    socket = new wxSocketClient();
    socket->SetEventHandler(*this);
    socket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    socket->Notify(true);
    
    reconnectTimer = new wxTimer(this, ID_RECONNECT_TIMER);
    heartbeatTimer = new wxTimer(this, ID_HEARTBEAT_TIMER);
}

ChatService::~ChatService() {
    if (socket) {
        socket->Close();
        delete socket;
    }
    if (reconnectTimer) {
        reconnectTimer->Stop();
        delete reconnectTimer;
    }
    if (heartbeatTimer) {
        heartbeatTimer->Stop();
        delete heartbeatTimer;
    }
}

bool ChatService::RegisterUser(const wxString& username, const wxString& password, const wxString& discordName) {
    // Basic validation
    if (username.length() < 3 || username.length() > 20) {
        return false;
    }
    if (password.length() < 6) {
        return false;
    }
    
    // Use a simple socket-based approach since wxHTTP doesn't handle POST well
    wxSocketClient socket;
    socket.SetTimeout(10);
    
    wxIPV4address addr;
    addr.Hostname("116.203.17.184");
    addr.Service(3678);
    
    if (!socket.Connect(addr, true)) {
        return false;
    }
    
    // Prepare JSON payload
    wxString jsonString = wxString::Format(
        "{\"username\":\"%s\",\"password\":\"%s\",\"discordName\":\"%s\"}", 
        username.c_str(), password.c_str(), discordName.c_str()
    );
    
    // Prepare HTTP POST request
    wxString httpRequest = wxString::Format(
        "POST /api/register HTTP/1.1\r\n"
        "Host: 116.203.17.184:3678\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        jsonString.length(),
        jsonString.c_str()
    );
    
    // Send request
    socket.Write(httpRequest.mb_str(), httpRequest.length());
    
    // Read response
    wxString response;
    char buffer[4096];
    while (socket.IsConnected()) {
        socket.Read(buffer, sizeof(buffer));
        size_t bytesRead = socket.LastCount();
        if (bytesRead > 0) {
            response += wxString(buffer, wxConvUTF8, bytesRead);
        } else {
            break;
        }
    }
    
    socket.Close();
    
    // Simple response parsing - look for "success":true
    return response.Contains("\"success\":true");
}

bool ChatService::LoginUser(const wxString& username, const wxString& password, wxString& token) {
    // Basic validation
    if (username.IsEmpty() || password.IsEmpty()) {
        return false;
    }
    
    // Use a simple socket-based approach since wxHTTP doesn't handle POST well
    wxSocketClient socket;
    socket.SetTimeout(10);
    
    wxIPV4address addr;
    addr.Hostname("116.203.17.184");
    addr.Service(3678);
    
    if (!socket.Connect(addr, true)) {
        return false;
    }
    
    // Prepare JSON payload
    wxString jsonString = wxString::Format(
        "{\"username\":\"%s\",\"password\":\"%s\"}", 
        username.c_str(), password.c_str()
    );
    
    // Prepare HTTP POST request
    wxString httpRequest = wxString::Format(
        "POST /api/login HTTP/1.1\r\n"
        "Host: 116.203.17.184:3678\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        jsonString.length(),
        jsonString.c_str()
    );
    
    // Send request
    socket.Write(httpRequest.mb_str(), httpRequest.length());
    
    // Read response
    wxString response;
    char buffer[4096];
    while (socket.IsConnected()) {
        socket.Read(buffer, sizeof(buffer));
        size_t bytesRead = socket.LastCount();
        if (bytesRead > 0) {
            response += wxString(buffer, wxConvUTF8, bytesRead);
        } else {
            break;
        }
    }
    
    socket.Close();
    
    // Simple response parsing - look for token
    if (response.Contains("\"success\":true")) {
        // Extract token from response (simple parsing)
        size_t tokenStart = response.find("\"token\":\"");
        if (tokenStart != wxString::npos) {
            tokenStart += 9; // Length of "token":"
            size_t tokenEnd = response.find("\"", tokenStart);
            if (tokenEnd != wxString::npos) {
                token = response.substr(tokenStart, tokenEnd - tokenStart);
                return true;
            }
        }
    }
    
    return false;
}

bool ChatService::Connect(const wxString& username, const wxString& token) {
    if (connected) {
        return true;
    }
    
    currentUsername = username;
    authToken = token;
    
    // For HTTP-based chat, we just mark as connected and start polling
    connected = true;
    heartbeatTimer->Start(2000); // Poll every 2 seconds
    
    return true;
}

void ChatService::Disconnect() {
    connected = false;
    heartbeatTimer->Stop();
}

void ChatService::SendMessage(const wxString& message) {
    if (!connected) {
        return;
    }
    
    // Send message via HTTP POST
    wxSocketClient socket;
    socket.SetTimeout(10);
    
    wxIPV4address addr;
    addr.Hostname("116.203.17.184");
    addr.Service(3678);
    
    if (!socket.Connect(addr, true)) {
        return;
    }
    
    // Prepare JSON payload
    wxString jsonString = wxString::Format(
        "{\"content\":\"%s\",\"token\":\"%s\"}", 
        message.c_str(), authToken.c_str()
    );
    
    // Prepare HTTP POST request
    wxString httpRequest = wxString::Format(
        "POST /api/messages/send HTTP/1.1\r\n"
        "Host: 116.203.17.184:3678\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        jsonString.length(),
        jsonString.c_str()
    );
    
    // Send request
    socket.Write(httpRequest.mb_str(), httpRequest.length());
    socket.Close();
}

void ChatService::SendPrivateMessage(const wxString& recipient, const wxString& message) {
    if (!connected || !socket) {
        return;
    }
    
    json::mObject jsonData;
    jsonData["type"] = json::mValue("privateMessage");
    jsonData["recipient"] = json::mValue(std::string(recipient.mb_str()));
    jsonData["content"] = json::mValue(std::string(message.mb_str()));
    
    std::string jsonString = json::write(jsonData);
    
    socket->Write(jsonString.c_str(), jsonString.length());
}

void ChatService::StartTyping() {
    // Not implemented for HTTP polling
}

void ChatService::StopTyping() {
    // Not implemented for HTTP polling
}

void ChatService::OnSocketEvent(wxSocketEvent& event) {
    // No longer used - we use HTTP polling instead of persistent sockets
}

void ChatService::OnTimer(wxTimerEvent& event) {
    if (event.GetId() == ID_RECONNECT_TIMER) {
        // Attempt to reconnect
        Connect(currentUsername, authToken);
    } else if (event.GetId() == ID_HEARTBEAT_TIMER) {
        // Poll for new messages
        PollMessages();
    }
}

void ChatService::PollMessages() {
    if (!connected) {
        return;
    }
    
    wxSocketClient socket;
    socket.SetTimeout(5);
    
    wxIPV4address addr;
    addr.Hostname("116.203.17.184");
    addr.Service(3678);
    
    if (!socket.Connect(addr, true)) {
        return;
    }
    
    // Prepare HTTP GET request for polling with timestamp filter
    wxString url = wxString::Format("/api/messages/poll?token=%s", authToken.c_str());
    if (!lastMessageTimestamp.IsEmpty()) {
        url += wxString::Format("&since=%s", lastMessageTimestamp.c_str());
    }
    
    wxString httpRequest = wxString::Format(
        "GET %s HTTP/1.1\r\n"
        "Host: 116.203.17.184:3678\r\n"
        "Connection: close\r\n"
        "\r\n",
        url.c_str()
    );
    
    // Send request
    socket.Write(httpRequest.mb_str(), httpRequest.length());
    
    // Read response
    wxString response;
    char buffer[4096];
    while (socket.IsConnected()) {
        socket.Read(buffer, sizeof(buffer));
        size_t bytesRead = socket.LastCount();
        if (bytesRead > 0) {
            response += wxString(buffer, wxConvUTF8, bytesRead);
        } else {
            break;
        }
    }
    
    socket.Close();
    
    // Parse HTTP response to get JSON body
    size_t bodyStart = response.find("\r\n\r\n");
    if (bodyStart != wxString::npos) {
        wxString jsonBody = response.substr(bodyStart + 4);
        ProcessPollResponse(jsonBody);
    }
}

void ChatService::ProcessPollResponse(const wxString& jsonResponse) {
    // Simple JSON parsing to extract messages
    if (jsonResponse.Contains("\"success\":true")) {
        // Update timestamp from response
        size_t timestampStart = jsonResponse.find("\"timestamp\":\"");
        if (timestampStart != wxString::npos) {
            timestampStart += 13; // Length of "timestamp":"
            size_t timestampEnd = jsonResponse.find("\"", timestampStart);
            if (timestampEnd != wxString::npos) {
                lastMessageTimestamp = jsonResponse.substr(timestampStart, timestampEnd - timestampStart);
            }
        }
        
        // Extract and process messages
        size_t messagesStart = jsonResponse.find("\"messages\":[");
        if (messagesStart != wxString::npos) {
            // Only send event if there are actual messages (to avoid empty updates)
            if (jsonResponse.Contains("\"username\"")) {
                if (g_chatWindow) {
                    wxCommandEvent evt(wxEVT_CHAT_MESSAGE);
                    evt.SetString(jsonResponse);
                    g_chatWindow->GetEventHandler()->AddPendingEvent(evt);
                }
            }
        }
        
        // Always update online users even if no new messages
        if (jsonResponse.Contains("\"onlineUsers\"")) {
            if (g_chatWindow) {
                wxCommandEvent evt(wxEVT_CHAT_USER_JOINED); // Reuse this event for user list updates
                evt.SetString(jsonResponse);
                g_chatWindow->GetEventHandler()->AddPendingEvent(evt);
            }
        }
    }
}

//=============================================================================
// ChatRegisterDialog Implementation
//=============================================================================

wxBEGIN_EVENT_TABLE(ChatRegisterDialog, wxDialog)
    EVT_BUTTON(ID_REGISTER_OK, ChatRegisterDialog::OnRegister)
    EVT_BUTTON(ID_REGISTER_CANCEL, ChatRegisterDialog::OnCancel)
wxEND_EVENT_TABLE()

ChatRegisterDialog::ChatRegisterDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Register Chat Account", wxDefaultPosition, wxSize(400, 300)) {
    
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Create New Chat Account");
    wxFont titleFont = title->GetFont();
    titleFont.SetPointSize(titleFont.GetPointSize() + 2);
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    mainSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
    
    // Form fields
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(4, 2, 10, 10);
    formSizer->AddGrowableCol(1, 1);
    
    // Username
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Username:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    usernameCtrl = new wxTextCtrl(this, wxID_ANY);
    formSizer->Add(usernameCtrl, 1, wxEXPAND);
    
    // Password
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Password:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    passwordCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    formSizer->Add(passwordCtrl, 1, wxEXPAND);
    
    // Confirm Password
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Confirm Password:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    confirmPasswordCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    formSizer->Add(confirmPasswordCtrl, 1, wxEXPAND);
    
    // Discord Name (optional)
    formSizer->Add(new wxStaticText(this, wxID_ANY, "Discord Name (optional):"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    discordNameCtrl = new wxTextCtrl(this, wxID_ANY);
    formSizer->Add(discordNameCtrl, 1, wxEXPAND);
    
    mainSizer->Add(formSizer, 1, wxALL | wxEXPAND, 20);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, ID_REGISTER_OK, "Register"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, ID_REGISTER_CANCEL, "Cancel"));
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxALIGN_CENTER, 10);
    
    SetSizer(mainSizer);
    Center();
    
    usernameCtrl->SetFocus();
}

void ChatRegisterDialog::OnRegister(wxCommandEvent& event) {
    wxString username = usernameCtrl->GetValue().Trim();
    wxString password = passwordCtrl->GetValue();
    wxString confirmPassword = confirmPasswordCtrl->GetValue();
    wxString discordName = discordNameCtrl->GetValue().Trim();
    
    // Validation
    if (username.IsEmpty()) {
        wxMessageBox("Username is required.", "Error", wxOK | wxICON_ERROR);
        usernameCtrl->SetFocus();
        return;
    }
    
    if (username.length() < 3 || username.length() > 20) {
        wxMessageBox("Username must be between 3 and 20 characters.", "Error", wxOK | wxICON_ERROR);
        usernameCtrl->SetFocus();
        return;
    }
    
    if (password.length() < 6) {
        wxMessageBox("Password must be at least 6 characters.", "Error", wxOK | wxICON_ERROR);
        passwordCtrl->SetFocus();
        return;
    }
    
    if (password != confirmPassword) {
        wxMessageBox("Passwords do not match.", "Error", wxOK | wxICON_ERROR);
        confirmPasswordCtrl->SetFocus();
        return;
    }
    
    // Attempt registration
    if (ChatService::RegisterUser(username, password, discordName)) {
        wxMessageBox("Registration successful! You can now connect to the chat.", "Success", wxOK | wxICON_INFORMATION);
        EndModal(wxID_OK);
    } else {
        wxMessageBox("Registration failed. The username might already be taken or the server is unavailable.", "Error", wxOK | wxICON_ERROR);
    }
}

void ChatRegisterDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

//=============================================================================
// ChatWindow Implementation
//=============================================================================

wxBEGIN_EVENT_TABLE(ChatWindow, wxFrame)
    EVT_CLOSE(ChatWindow::OnClose)
    EVT_BUTTON(ID_CHAT_SEND, ChatWindow::OnSendMessage)
    EVT_TEXT_ENTER(ID_CHAT_MESSAGE_INPUT, ChatWindow::OnTextEnter)
    EVT_LIST_ITEM_RIGHT_CLICK(ID_CHAT_USER_LIST, ChatWindow::OnUserListRightClick)
    EVT_MENU(ID_CHAT_PRIVATE_MESSAGE, ChatWindow::OnPrivateMessage)
    EVT_TEXT(ID_CHAT_MESSAGE_INPUT, ChatWindow::OnTextChanged)
    EVT_BUTTON(ID_CHAT_CONNECT, ChatWindow::OnConnect)
    EVT_BUTTON(ID_CHAT_REGISTER, ChatWindow::OnRegister)
    
    // Custom events
    EVT_COMMAND(wxID_ANY, wxEVT_CHAT_MESSAGE, ChatWindow::OnChatMessage)
    EVT_COMMAND(wxID_ANY, wxEVT_CHAT_USER_JOINED, ChatWindow::OnUserJoined)
    EVT_COMMAND(wxID_ANY, wxEVT_CHAT_USER_LEFT, ChatWindow::OnUserLeft)
    EVT_COMMAND(wxID_ANY, wxEVT_CHAT_PRIVATE_MESSAGE, ChatWindow::OnPrivateMessageReceived)
    EVT_COMMAND(wxID_ANY, wxEVT_CHAT_ERROR, ChatWindow::OnChatError)
wxEND_EVENT_TABLE()

ChatWindow::ChatWindow(wxWindow* parent) :
    wxFrame(parent, wxID_ANY, "RME Chat", wxDefaultPosition, wxSize(700, 500)),
    isTyping(false) {
    
    // Create main panel
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side - chat area
    wxBoxSizer* chatSizer = new wxBoxSizer(wxVERTICAL);
    
    // Chat display
    chatDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    chatDisplay->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    chatSizer->Add(chatDisplay, 1, wxEXPAND | wxALL, 5);
    
    // Message input area
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    messageInput = new wxTextCtrl(panel, ID_CHAT_MESSAGE_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    sendButton = new wxButton(panel, ID_CHAT_SEND, "Send");
    
    inputSizer->Add(messageInput, 1, wxEXPAND | wxRIGHT, 5);
    inputSizer->Add(sendButton, 0);
    
    chatSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 5);
    
    // Connection controls
    wxBoxSizer* connectionSizer = new wxBoxSizer(wxHORIZONTAL);
    connectButton = new wxButton(panel, ID_CHAT_CONNECT, "Connect");
    registerButton = new wxButton(panel, ID_CHAT_REGISTER, "Register");
    
    connectionSizer->Add(connectButton, 0, wxRIGHT, 5);
    connectionSizer->Add(registerButton, 0);
    
    chatSizer->Add(connectionSizer, 0, wxALL, 5);
    
    mainSizer->Add(chatSizer, 2, wxEXPAND);
    
    // Right side - user list
    wxBoxSizer* userSizer = new wxBoxSizer(wxVERTICAL);
    
    onlineCountLabel = new wxStaticText(panel, wxID_ANY, "Online: 0");
    userSizer->Add(onlineCountLabel, 0, wxALL, 5);
    
    userList = new wxListCtrl(panel, ID_CHAT_USER_LIST, wxDefaultPosition, wxSize(150, -1), wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER);
    userList->AppendColumn("Users");
    userList->SetColumnWidth(0, 140);
    userSizer->Add(userList, 1, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(userSizer, 0, wxEXPAND);
    
    panel->SetSizer(mainSizer);
    
    // Set initial state
    messageInput->Enable(false);
    sendButton->Enable(false);
    
    // Initialize typing timer
    typingTimer = new wxTimer(this, ID_TYPING_TIMER);
    
    // Add some initial text
    AddSystemMessage("Welcome to RME Chat! Register an account or connect with existing credentials.");
    AddSystemMessage("Server: RMECHAT");
    
    // Set minimum size
    SetMinSize(wxSize(500, 400));
}

ChatWindow::~ChatWindow() {
    if (typingTimer) {
        typingTimer->Stop();
        delete typingTimer;
    }
    
    // Close all private message windows
    for (auto& pair : privateWindows) {
        if (pair.second) {
            pair.second->Destroy();
        }
    }
    privateWindows.clear();
}

bool ChatWindow::Show(bool show) {
    wxFrame::Show(show);
    if (show) {
        messageInput->SetFocus();
    }
    return true;
}

void ChatWindow::OnClose(wxCloseEvent& event) {
    if (event.CanVeto()) {
        Show(false);
        event.Veto();
    } else {
        ChatService::Instance().Disconnect();
        Destroy();
    }
}

void ChatWindow::OnSendMessage(wxCommandEvent& event) {
    wxString message = messageInput->GetValue().Trim();
    if (message.IsEmpty() || !ChatService::Instance().IsConnected()) {
        return;
    }
    
    ChatService::Instance().SendMessage(message);
    messageInput->Clear();
    messageInput->SetFocus();
}

void ChatWindow::OnTextEnter(wxCommandEvent& event) {
    OnSendMessage(event);
}

void ChatWindow::OnTextChanged(wxCommandEvent& event) {
    // Implement typing indicator logic here
    // For now, just keep focus
    event.Skip();
}

void ChatWindow::OnUserListRightClick(wxListEvent& event) {
    long index = event.GetIndex();
    if (index == wxNOT_FOUND) {
        return;
    }
    
    selectedUser = userList->GetItemText(index);
    if (selectedUser == ChatService::Instance().GetUsername()) {
        return; // Can't send message to self
    }
    
    wxMenu menu;
    menu.Append(ID_CHAT_PRIVATE_MESSAGE, "Send Private Message");
    
    PopupMenu(&menu);
}

void ChatWindow::OnPrivateMessage(wxCommandEvent& event) {
    if (selectedUser.IsEmpty()) {
        return;
    }
    
    // Check if window already exists
    auto it = privateWindows.find(selectedUser);
    if (it != privateWindows.end() && it->second) {
        it->second->Raise();
        it->second->FocusInput();
    } else {
        PrivateMessageWindow* pmWindow = new PrivateMessageWindow(this, selectedUser);
        privateWindows[selectedUser] = pmWindow;
        pmWindow->Show();
    }
}

void ChatWindow::OnChatMessage(wxCommandEvent& event) {
    // Parse polling response data
    std::string jsonString = std::string(event.GetString().mb_str());
    json::mValue jsonData;
    if (json::read(jsonString, jsonData)) {
        json::mObject obj = jsonData.get_obj();
        
        // Check if this is a polling response with messages array
        if (obj.find("messages") != obj.end()) {
            json::mArray messages = obj["messages"].get_array();
            
            for (const auto& msgValue : messages) {
                json::mObject msg = msgValue.get_obj();
                wxString username = wxString(msg["username"].get_str().c_str(), wxConvUTF8);
                wxString content = wxString(msg["content"].get_str().c_str(), wxConvUTF8);
                wxString timestamp = wxString(msg["timestamp"].get_str().c_str(), wxConvUTF8);
                
                AddMessage(username, content, timestamp);
            }
            
            // Online users are handled by the separate user joined event
        } else {
            // Handle single message format (for backward compatibility)
            wxString username = wxString(obj["username"].get_str().c_str(), wxConvUTF8);
            wxString content = wxString(obj["content"].get_str().c_str(), wxConvUTF8);
            wxString timestamp = wxString(obj["timestamp"].get_str().c_str(), wxConvUTF8);
            
            AddMessage(username, content, timestamp);
        }
    }
}

void ChatWindow::OnUserJoined(wxCommandEvent& event) {
    // Parse data and update user list
    std::string jsonString = std::string(event.GetString().mb_str());
    json::mValue jsonData;
    if (json::read(jsonString, jsonData)) {
        json::mObject obj = jsonData.get_obj();
        
        // Check if this is a polling response with online users
        if (obj.find("onlineUsers") != obj.end()) {
            json::mArray users = obj["onlineUsers"].get_array();
            userList->DeleteAllItems();
            
            for (const auto& userValue : users) {
                json::mObject user = userValue.get_obj();
                wxString username = wxString(user["username"].get_str().c_str(), wxConvUTF8);
                userList->InsertItem(userList->GetItemCount(), username);
            }
            
            if (obj.find("onlineCount") != obj.end()) {
                long onlineCount = (long)obj["onlineCount"].get_int();
                onlineCountLabel->SetLabel(wxString::Format("Online: %ld", onlineCount));
            }
        } else if (obj.find("username") != obj.end()) {
            // Handle single user joined event (for backward compatibility)
            wxString username = wxString(obj["username"].get_str().c_str(), wxConvUTF8);
            long onlineCount = (long)obj["onlineCount"].get_int();
            
            AddSystemMessage(username + " joined the chat");
            onlineCountLabel->SetLabel(wxString::Format("Online: %ld", onlineCount));
            
            // Add to user list if not already there
            long index = userList->FindItem(-1, username);
            if (index == wxNOT_FOUND) {
                userList->InsertItem(userList->GetItemCount(), username);
            }
        }
    }
}

void ChatWindow::OnUserLeft(wxCommandEvent& event) {
    // Parse data and update user list
    std::string jsonString = std::string(event.GetString().mb_str());
    json::mValue jsonData;
    if (json::read(jsonString, jsonData)) {
        json::mObject obj = jsonData.get_obj();
        wxString username = wxString(obj["username"].get_str().c_str(), wxConvUTF8);
        long onlineCount = (long)obj["onlineCount"].get_int();
        
        AddSystemMessage(username + " left the chat");
        onlineCountLabel->SetLabel(wxString::Format("Online: %ld", onlineCount));
        
        // Remove from user list
        long index = userList->FindItem(-1, username);
        if (index != wxNOT_FOUND) {
            userList->DeleteItem(index);
        }
    }
}

void ChatWindow::OnPrivateMessageReceived(wxCommandEvent& event) {
    // Parse private message data
    std::string jsonString = std::string(event.GetString().mb_str());
    json::mValue jsonData;
    if (json::read(jsonString, jsonData)) {
        json::mObject obj = jsonData.get_obj();
        wxString from = wxString(obj["from"].get_str().c_str(), wxConvUTF8);
        wxString content = wxString(obj["content"].get_str().c_str(), wxConvUTF8);
        wxString timestamp = wxString(obj["timestamp"].get_str().c_str(), wxConvUTF8);
        
        // Open or focus private message window
        auto it = privateWindows.find(from);
        if (it != privateWindows.end() && it->second) {
            it->second->AddMessage(from, content, timestamp);
            it->second->Raise();
        } else {
            PrivateMessageWindow* pmWindow = new PrivateMessageWindow(this, from);
            privateWindows[from] = pmWindow;
            pmWindow->AddMessage(from, content, timestamp);
            pmWindow->Show();
        }
    }
}

void ChatWindow::OnChatError(wxCommandEvent& event) {
    AddSystemMessage("ERROR: " + event.GetString());
}

void ChatWindow::AddMessage(const wxString& username, const wxString& message, const wxString& timestamp) {
    wxString timeStr = timestamp;
    if (timeStr.IsEmpty()) {
        wxDateTime now = wxDateTime::Now();
        timeStr = now.Format("%H:%M:%S");
    }
    
    wxString fullMessage = wxString::Format("[%s] %s: %s\n", timeStr, username, message);
    
    chatDisplay->SetDefaultStyle(wxTextAttr(*wxBLACK));
    chatDisplay->AppendText(fullMessage);
    
    // Auto-scroll to bottom
    chatDisplay->ShowPosition(chatDisplay->GetLastPosition());
}

void ChatWindow::AddSystemMessage(const wxString& message) {
    wxDateTime now = wxDateTime::Now();
    wxString timeStr = now.Format("%H:%M:%S");
    wxString fullMessage = wxString::Format("[%s] * %s\n", timeStr, message);
    
    chatDisplay->SetDefaultStyle(wxTextAttr(*wxBLUE));
    chatDisplay->AppendText(fullMessage);
    chatDisplay->SetDefaultStyle(wxTextAttr(*wxBLACK));
    
    // Auto-scroll to bottom
    chatDisplay->ShowPosition(chatDisplay->GetLastPosition());
}

void ChatWindow::ConnectToServer() {
    // Show login dialog
    wxDialog loginDialog(this, wxID_ANY, "Connect to Chat", wxDefaultPosition, wxSize(300, 200));
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(2, 2, 10, 10);
    formSizer->AddGrowableCol(1, 1);
    
    formSizer->Add(new wxStaticText(&loginDialog, wxID_ANY, "Username:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxTextCtrl* usernameCtrl = new wxTextCtrl(&loginDialog, wxID_ANY);
    formSizer->Add(usernameCtrl, 1, wxEXPAND);
    
    formSizer->Add(new wxStaticText(&loginDialog, wxID_ANY, "Password:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    wxTextCtrl* passwordCtrl = new wxTextCtrl(&loginDialog, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    formSizer->Add(passwordCtrl, 1, wxEXPAND);
    
    sizer->Add(formSizer, 1, wxALL | wxEXPAND, 20);
    
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(&loginDialog, wxID_OK, "Connect"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(&loginDialog, wxID_CANCEL, "Cancel"));
    
    sizer->Add(buttonSizer, 0, wxALL | wxALIGN_CENTER, 10);
    
    loginDialog.SetSizer(sizer);
    loginDialog.Center();
    
    if (loginDialog.ShowModal() == wxID_OK) {
        wxString username = usernameCtrl->GetValue().Trim();
        wxString password = passwordCtrl->GetValue();
        
        if (username.IsEmpty() || password.IsEmpty()) {
            wxMessageBox("Username and password are required.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        
        wxString token;
        if (ChatService::LoginUser(username, password, token)) {
            if (ChatService::Instance().Connect(username, token)) {
                AddSystemMessage("Connecting to chat server...");
                messageInput->Enable(true);
                sendButton->Enable(true);
                connectButton->SetLabel("Disconnect");
                
                // Update user list with current online users
                UpdateUserList();
            } else {
                wxMessageBox("Failed to connect to chat server.", "Error", wxOK | wxICON_ERROR);
            }
        } else {
            wxMessageBox("Login failed. Check your username and password.", "Error", wxOK | wxICON_ERROR);
        }
    }
}

void ChatWindow::OnConnect(wxCommandEvent& event) {
    ConnectToServer();
}

void ChatWindow::OnRegister(wxCommandEvent& event) {
    ChatManager::ShowRegisterDialog();
}

void ChatWindow::UpdateUserList() {
    userList->DeleteAllItems();
    
    const wxArrayString& users = ChatService::Instance().GetOnlineUsers();
    for (const wxString& user : users) {
        userList->InsertItem(userList->GetItemCount(), user);
    }
    
    onlineCountLabel->SetLabel(wxString::Format("Online: %zu", users.size()));
}

//=============================================================================
// PrivateMessageWindow Implementation
//=============================================================================

wxBEGIN_EVENT_TABLE(PrivateMessageWindow, wxDialog)
    EVT_CLOSE(PrivateMessageWindow::OnClose)
    EVT_BUTTON(ID_PM_SEND, PrivateMessageWindow::OnSendMessage)
    EVT_TEXT_ENTER(ID_PM_MESSAGE_INPUT, PrivateMessageWindow::OnTextEnter)
wxEND_EVENT_TABLE()

PrivateMessageWindow::PrivateMessageWindow(wxWindow* parent, const wxString& username) :
    wxDialog(parent, wxID_ANY, "Private Message - " + username, wxDefaultPosition, wxSize(500, 400)),
    targetUser(username) {
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Chat display
    chatDisplay = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 
                                wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    chatDisplay->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    sizer->Add(chatDisplay, 1, wxEXPAND | wxALL, 5);
    
    // Message input
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    messageInput = new wxTextCtrl(this, ID_PM_MESSAGE_INPUT, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    sendButton = new wxButton(this, ID_PM_SEND, "Send");
    
    inputSizer->Add(messageInput, 1, wxEXPAND | wxRIGHT, 5);
    inputSizer->Add(sendButton, 0);
    
    sizer->Add(inputSizer, 0, wxEXPAND | wxALL, 5);
    
    SetSizer(sizer);
    SetMinSize(wxSize(400, 300));
    
    messageInput->SetFocus();
}

void PrivateMessageWindow::OnClose(wxCloseEvent& event) {
    // Remove from parent's private windows map
    if (g_chatWindow) {
        auto& privateWindows = g_chatWindow->privateWindows;
        auto it = privateWindows.find(targetUser);
        if (it != privateWindows.end()) {
            privateWindows.erase(it);
        }
    }
    
    Destroy();
}

void PrivateMessageWindow::OnSendMessage(wxCommandEvent& event) {
    wxString message = messageInput->GetValue().Trim();
    if (message.IsEmpty() || !ChatService::Instance().IsConnected()) {
        return;
    }
    
    ChatService::Instance().SendPrivateMessage(targetUser, message);
    
    // Add to our own display
    AddMessage(ChatService::Instance().GetUsername(), message);
    
    messageInput->Clear();
    messageInput->SetFocus();
}

void PrivateMessageWindow::OnTextEnter(wxCommandEvent& event) {
    OnSendMessage(event);
}

void PrivateMessageWindow::AddMessage(const wxString& from, const wxString& message, const wxString& timestamp) {
    wxString timeStr = timestamp;
    if (timeStr.IsEmpty()) {
        wxDateTime now = wxDateTime::Now();
        timeStr = now.Format("%H:%M:%S");
    }
    
    wxString fullMessage = wxString::Format("[%s] %s: %s\n", timeStr, from, message);
    
    chatDisplay->SetDefaultStyle(wxTextAttr(*wxBLACK));
    chatDisplay->AppendText(fullMessage);
    
    // Auto-scroll to bottom
    chatDisplay->ShowPosition(chatDisplay->GetLastPosition());
}

void PrivateMessageWindow::FocusInput() {
    messageInput->SetFocus();
}

//=============================================================================
// ChatManager Implementation
//=============================================================================

namespace ChatManager {
    void ShowChatWindow() {
        if (!g_chatWindow) {
            g_chatWindow = new ChatWindow(g_gui.root);
        }
        g_chatWindow->Show(true);
        g_chatWindow->Raise();
    }
    
    void CloseChatWindow() {
        if (g_chatWindow) {
            g_chatWindow->Close();
            g_chatWindow = nullptr;
        }
    }
    
    bool IsChatWindowOpen() {
        return g_chatWindow && g_chatWindow->IsShown();
    }
    
    ChatWindow* GetChatWindow() {
        return g_chatWindow;
    }
    
    void ShowRegisterDialog() {
        ChatRegisterDialog dialog(g_gui.root);
        dialog.ShowModal();
    }
    
    void ShowPrivateMessage(const wxString& username) {
        ShowChatWindow();
        if (g_chatWindow) {
            g_chatWindow->selectedUser = username;
            wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, ID_CHAT_PRIVATE_MESSAGE);
            g_chatWindow->GetEventHandler()->ProcessEvent(evt);
        }
    }
} 
