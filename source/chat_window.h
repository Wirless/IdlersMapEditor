#ifndef RME_CHAT_WINDOW_H_
#define RME_CHAT_WINDOW_H_

#include "main.h"
#include <wx/socket.h>
#include <wx/listctrl.h>
#include <wx/timer.h>

class ChatRegisterDialog;
class ChatWindow;
class PrivateMessageWindow;

// Event declarations for custom chat events
wxDECLARE_EVENT(wxEVT_CHAT_MESSAGE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHAT_USER_JOINED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHAT_USER_LEFT, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHAT_PRIVATE_MESSAGE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_CHAT_ERROR, wxCommandEvent);

// Chat service manager - handles connection to Node.js server
class ChatService : public wxEvtHandler {
public:
    static ChatService& Instance() {
        static ChatService instance;
        return instance;
    }
    
    bool IsConnected() const { return connected; }
    bool Connect(const wxString& username, const wxString& token);
    void Disconnect();
    
    void SendMessage(const wxString& message);
    void SendPrivateMessage(const wxString& recipient, const wxString& message);
    void StartTyping();
    void StopTyping();
    
    const wxString& GetUsername() const { return currentUsername; }
    const wxArrayString& GetOnlineUsers() const { return onlineUsers; }
    
    // Registration
    static bool RegisterUser(const wxString& username, const wxString& password, const wxString& discordName = "");
    static bool LoginUser(const wxString& username, const wxString& password, wxString& token);
    
private:
    ChatService();
    ~ChatService();
    
    void OnSocketEvent(wxSocketEvent& event);
    void OnTimer(wxTimerEvent& event);
    void PollMessages();
    void ProcessPollResponse(const wxString& jsonResponse);
    
    bool connected;
    wxString currentUsername;
    wxString authToken;
    wxArrayString onlineUsers;
    wxString lastMessageTimestamp;
    
    wxSocketClient* socket;
    wxTimer* reconnectTimer;
    wxTimer* heartbeatTimer;
    
    DECLARE_EVENT_TABLE()
};

// Registration dialog
class ChatRegisterDialog : public wxDialog {
public:
    ChatRegisterDialog(wxWindow* parent);
    virtual ~ChatRegisterDialog() {}
    
private:
    void OnRegister(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    
    wxTextCtrl* usernameCtrl;
    wxTextCtrl* passwordCtrl;
    wxTextCtrl* confirmPasswordCtrl;
    wxTextCtrl* discordNameCtrl;
    
    DECLARE_EVENT_TABLE()
};

// Main chat window - detached like minimap
class ChatWindow : public wxFrame {
public:
    ChatWindow(wxWindow* parent);
    virtual ~ChatWindow();
    
    bool Show(bool show = true) override;
    
private:
    void OnClose(wxCloseEvent& event);
    void OnSendMessage(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent& event);
    void OnUserListRightClick(wxListEvent& event);
    void OnPrivateMessage(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnRegister(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    
    // Chat service event handlers
    void OnChatMessage(wxCommandEvent& event);
    void OnUserJoined(wxCommandEvent& event);
    void OnUserLeft(wxCommandEvent& event);
    void OnPrivateMessageReceived(wxCommandEvent& event);
    void OnChatError(wxCommandEvent& event);
    
    void UpdateUserList();
    void AddMessage(const wxString& username, const wxString& message, const wxString& timestamp = "");
    void AddSystemMessage(const wxString& message);
    void ConnectToServer();
    
    wxTextCtrl* chatDisplay;
    wxTextCtrl* messageInput;
    wxButton* sendButton;
    wxListCtrl* userList;
    wxStaticText* onlineCountLabel;
    wxButton* connectButton;
    wxButton* registerButton;
    
public:
    wxString selectedUser;
    std::map<wxString, PrivateMessageWindow*> privateWindows;
    
private:
    
    wxTimer* typingTimer;
    bool isTyping;
    
    DECLARE_EVENT_TABLE()
};

// Private message window
class PrivateMessageWindow : public wxDialog {
public:
    PrivateMessageWindow(wxWindow* parent, const wxString& username);
    virtual ~PrivateMessageWindow() {}
    
    void AddMessage(const wxString& from, const wxString& message, const wxString& timestamp = "");
    void FocusInput();
    
private:
    void OnClose(wxCloseEvent& event);
    void OnSendMessage(wxCommandEvent& event);
    void OnTextEnter(wxCommandEvent& event);
    
    wxString targetUser;
    wxTextCtrl* chatDisplay;
    wxTextCtrl* messageInput;
    wxButton* sendButton;
    
    DECLARE_EVENT_TABLE()
};

// Global chat window instance management
namespace ChatManager {
    void ShowChatWindow();
    void CloseChatWindow();
    bool IsChatWindowOpen();
    ChatWindow* GetChatWindow();
    
    void ShowRegisterDialog();
    void ShowPrivateMessage(const wxString& username);
}

#endif // RME_CHAT_WINDOW_H_ 