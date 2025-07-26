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
#include "notes_window.h"
#include "editor.h"
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dcmemory.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include "json/json_spirit_reader.h"
#include "json/json_spirit_writer.h"
#include <wx/wfstream.h>
#include <fstream>
#include <sstream>

// Define the ID for rename button
#define wxID_RENAME 9000

// Event table for the notes window
BEGIN_EVENT_TABLE(NotesWindow, wxDialog)
    EVT_BUTTON(wxID_SAVE, NotesWindow::OnSave)
    EVT_BUTTON(wxID_CLOSE, NotesWindow::OnClose)
    EVT_CHECKBOX(wxID_ANY, NotesWindow::OnStatusChange)
    EVT_TEXT(wxID_ANY, NotesWindow::OnTextChanged)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, NotesWindow::OnNoteSelected)
    EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, NotesWindow::OnListItemRightClick)
    EVT_TIMER(wxID_ANY, NotesWindow::OnAutoSave)
    EVT_BUTTON(wxID_ADD, NotesWindow::OnAddNote)
    EVT_BUTTON(wxID_DELETE, NotesWindow::OnDeleteNote)
    EVT_BUTTON(wxID_RENAME, NotesWindow::OnRenameNote)
    EVT_MENU(ID_POPUP_DELETE, NotesWindow::OnPopupClick)
    EVT_MENU(ID_POPUP_RENAME, NotesWindow::OnPopupClick)
    EVT_MENU(ID_POPUP_MARK_COMPLETED, NotesWindow::OnPopupClick)
    EVT_MENU(ID_POPUP_MARK_PENDING, NotesWindow::OnPopupClick)
END_EVENT_TABLE()

// Helper function to draw status indicators
wxBitmap CreateStatusBitmap(bool completed) {
    wxBitmap bmp(16, 16, 32);
    wxMemoryDC dc;
    dc.SelectObject(bmp);
    
    // Draw background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    // Draw indicator (green checkmark or red X)
    if (completed) {
        // Green checkmark
        dc.SetPen(wxPen(wxColor(0, 150, 0), 2));
        dc.DrawLine(3, 8, 7, 12);
        dc.DrawLine(7, 12, 13, 4);
    } else {
        // Red X
        dc.SetPen(wxPen(wxColor(200, 0, 0), 2));
        dc.DrawLine(4, 4, 12, 12);
        dc.DrawLine(4, 12, 12, 4);
    }
    
    dc.SelectObject(wxNullBitmap);
    return bmp;
}

NotesWindow::NotesWindow(wxWindow* parent, Editor* editor) :
    wxDialog(parent, wxID_ANY, "Map Notes", wxDefaultPosition, wxSize(700, 500), 
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    editor(editor),
    autoSaveTimer(this),
    modified(false),
    imageList(NULL)
{
    // Create controls
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Split window - notes list on left, content on right
    splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                   wxSP_3D | wxSP_LIVE_UPDATE);
    
    // Create left panel with note list
    wxPanel* leftPanel = new wxPanel(splitter, wxID_ANY);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    
    // Initialize image list for status icons
    InitImageList();
    
    // Notes list with columns
    notesList = new wxListCtrl(leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                              wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    
    // Add columns
    notesList->InsertColumn(0, "Status", wxLIST_FORMAT_CENTER, 60);
    notesList->InsertColumn(1, "Note Title", wxLIST_FORMAT_LEFT, 120);
    
    // Set image list
    notesList->SetImageList(imageList, wxIMAGE_LIST_SMALL);
    
    // Buttons for managing notes
    wxBoxSizer* notesButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* addButton = new wxButton(leftPanel, wxID_ADD, "+", wxDefaultPosition, wxSize(30, -1));
    wxButton* deleteButton = new wxButton(leftPanel, wxID_DELETE, "-", wxDefaultPosition, wxSize(30, -1));
    wxButton* renameButton = new wxButton(leftPanel, wxID_RENAME, "Rename", wxDefaultPosition, wxDefaultSize);
    
    notesButtonSizer->Add(addButton, 0, wxALL, 2);
    notesButtonSizer->Add(deleteButton, 0, wxALL, 2);
    notesButtonSizer->Add(renameButton, 1, wxALL | wxEXPAND, 2);
    
    leftSizer->Add(new wxStaticText(leftPanel, wxID_ANY, "Notes:"), 0, wxALL, 5);
    leftSizer->Add(notesList, 1, wxEXPAND | wxALL, 5);
    leftSizer->Add(notesButtonSizer, 0, wxEXPAND | wxALL, 5);
    
    leftPanel->SetSizer(leftSizer);
    
    // Create right panel with note content
    wxPanel* rightPanel = new wxPanel(splitter, wxID_ANY);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    // Title field
    wxBoxSizer* titleSizer = new wxBoxSizer(wxHORIZONTAL);
    titleSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "Title:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    titleText = new wxTextCtrl(rightPanel, wxID_ANY, "");
    titleText->Bind(wxEVT_TEXT, &NotesWindow::OnTitleChanged, this);
    titleSizer->Add(titleText, 1, wxEXPAND);
    
    // Status indicator area
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    statusCheckbox = new wxCheckBox(rightPanel, wxID_ANY, "Completed");
    statusIcon = new wxStaticBitmap(rightPanel, wxID_ANY, CreateStatusBitmap(false), wxDefaultPosition, wxSize(16, 16));
    
    statusSizer->Add(statusCheckbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    statusSizer->Add(statusIcon, 0, wxALIGN_CENTER_VERTICAL, 0);
    statusSizer->Add(new wxStaticText(rightPanel, wxID_ANY, "Mark task as completed"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    
    // Notes text area
    notesText = new wxTextCtrl(rightPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 
                              wxTE_MULTILINE | wxTE_RICH);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* saveButton = new wxButton(rightPanel, wxID_SAVE, "Save");
    wxButton* closeButton = new wxButton(rightPanel, wxID_CLOSE, "Close");
    
    buttonSizer->Add(saveButton, 0, wxALL, 5);
    buttonSizer->Add(closeButton, 0, wxALL, 5);
    
    // Add elements to right panel
    rightSizer->Add(titleSizer, 0, wxEXPAND | wxALL, 10);
    rightSizer->Add(statusSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    rightSizer->Add(notesText, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);
    rightSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    rightPanel->SetSizer(rightSizer);
    
    // Set up splitter
    splitter->SplitVertically(leftPanel, rightPanel, 200);
    splitter->SetMinimumPaneSize(100);
    
    mainSizer->Add(splitter, 1, wxEXPAND | wxALL, 5);
    
    SetSizer(mainSizer);
    
    // Load existing notes
    LoadNotes();
    
    // Select first note if available
    if (!notes.empty()) {
        notesList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    } else {
        // Disable note content area if no notes
        titleText->Enable(false);
        notesText->Enable(false);
        statusCheckbox->Enable(false);
    }
    
    // Set up auto-save timer (every 30 seconds)
    autoSaveTimer.Start(30000);
}

NotesWindow::~NotesWindow() {
    // Save notes when window is closed
    if (modified) {
        SaveNotes();
    }
    
    // Stop the timer
    autoSaveTimer.Stop();
    
    // Clean up image list
    if (imageList) {
        delete imageList;
        imageList = NULL;
    }
}

void NotesWindow::InitImageList() {
    // Create image list for status icons
    imageList = new wxImageList(16, 16, true);
    imageList->Add(CreateStatusBitmap(false));  // Index 0: Incomplete
    imageList->Add(CreateStatusBitmap(true));   // Index 1: Complete
}

wxString NotesWindow::GetNotesDirectory() const {
    wxString dir = "data/notes";
    
    // Create directory if it doesn't exist
    if (!wxDir::Exists(dir)) {
        wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    
    return dir;
}

wxString NotesWindow::GetNotesFilePath() const {
    if (!editor || !editor->map.hasFile()) {
        return wxString();
    }
    
    // Get the map name without extension
    wxString mapName = wxstr(editor->map.getName());
    
    // Create the notes filename
    wxString notesDir = GetNotesDirectory();
    return wxString::Format("%s/%s_notes.json", notesDir, mapName);
}

bool NotesWindow::LoadNotes() {
    wxString filePath = GetNotesFilePath();
    if (filePath.empty()) {
        return false;
    }
    
    // Clear existing notes
    notes.clear();
    
    if (!wxFileExists(filePath)) {
        // No notes file exists yet - create default note
        notes.push_back(MapNote("Default Note", "", false));
        UpdateNotesList();
        return true;
    }
    
    try {
        // Open the file
        std::ifstream file(filePath.mb_str(wxConvUTF8).data());
        if (!file.is_open()) {
            return false;
        }
        
        // Parse JSON with json_spirit
        json_spirit::Value value;
        if (!json_spirit::read(file, value) || value.type() != json_spirit::array_type) {
            file.close();
            // Try to parse as old format for backward compatibility
            return LoadOldFormatNotes(filePath);
        }
        
        // Read notes from JSON array
        json_spirit::Array arr = value.get_array();
        for (size_t i = 0; i < arr.size(); i++) {
            if (arr[i].type() == json_spirit::obj_type) {
                json_spirit::Object obj = arr[i].get_obj();
                
                // Find title, content, and completed status
                wxString title, content;
                bool completed = false;
                
                for (json_spirit::Object::const_iterator it = obj.begin(); it != obj.end(); ++it) {
                    if (it->name_ == "title" && it->value_.type() == json_spirit::str_type) {
                        title = wxString::FromUTF8(it->value_.get_str().c_str());
                    }
                    else if (it->name_ == "content" && it->value_.type() == json_spirit::str_type) {
                        content = wxString::FromUTF8(it->value_.get_str().c_str());
                    }
                    else if (it->name_ == "completed" && it->value_.type() == json_spirit::bool_type) {
                        completed = it->value_.get_bool();
                    }
                }
                
                if (!title.IsEmpty()) {
                    notes.push_back(MapNote(title, content, completed));
                }
            }
        }
        
        file.close();
    } catch (...) {
        // If any exception occurs, try to load old format
        return LoadOldFormatNotes(filePath);
    }
    
    // If no valid notes were found, create a default one
    if (notes.empty()) {
        notes.push_back(MapNote("Default Note", "", false));
    }
    
    UpdateNotesList();
    modified = false;
    return true;
}

bool NotesWindow::LoadOldFormatNotes(const wxString& filePath) {
    // Try to load in the old format (single note)
    wxFile file;
    if (!file.Open(filePath, wxFile::read)) {
        return false;
    }
    
    // Read content
    wxString content;
    file.ReadAll(&content);
    file.Close();
    
    // Parse content - first line contains status info
    wxString statusLine;
    wxString noteContent;
    
    int pos = content.Find('\n');
    if (pos != wxNOT_FOUND) {
        statusLine = content.Left(pos);
        noteContent = content.Mid(pos + 1);
    } else {
        statusLine = content;
        noteContent = "";
    }
    
    // Create a single note from old format
    bool isCompleted = (statusLine.Trim() == "STATUS:COMPLETED");
    notes.push_back(MapNote("Imported Note", noteContent, isCompleted));
    
    UpdateNotesList();
    modified = true; // Mark as modified to save in new format
    return true;
}

void NotesWindow::SaveSelectedNote() {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        wxString currentTitle = titleText->GetValue();
        wxString currentContent = notesText->GetValue();
        bool currentCompleted = statusCheckbox->GetValue();
        
        // Check if this note already exists
        bool foundDuplicate = false;
        
        // If the selected note doesn't match the UI, look for a match elsewhere
        if (notes[selected].title != currentTitle || 
            notes[selected].content != currentContent ||
            notes[selected].completed != currentCompleted) {
            
            // Try to find a note that matches our current UI values
            for (size_t i = 0; i < notes.size(); i++) {
                if (i != selected && 
                    notes[i].title == currentTitle &&
                    notes[i].content == currentContent &&
                    notes[i].completed == currentCompleted) {
                    // We found an existing duplicate, don't update
                    foundDuplicate = true;
                    break;
                }
            }
        }
        
        // Only update if not a duplicate
        if (!foundDuplicate) {
            // Update the selected note with current UI values
            notes[selected].title = currentTitle;
            notes[selected].content = currentContent;
            notes[selected].completed = currentCompleted;
            
            // Update the list display
            notesList->SetItem(selected, 1, currentTitle);
            notesList->SetItem(selected, 0, "", currentCompleted ? 1 : 0);
        }
    }
}

bool NotesWindow::SaveNotes() {
    wxString filePath = GetNotesFilePath();
    if (filePath.empty()) {
        return false;
    }
    
    // Save current note content if a note is selected
    SaveSelectedNote();
    
    try {
        // Create JSON array using json_spirit
        json_spirit::Array root;
        
        for (size_t i = 0; i < notes.size(); i++) {
            json_spirit::Object noteObj;
            noteObj.push_back(json_spirit::Pair("title", std::string(notes[i].title.mb_str(wxConvUTF8))));
            noteObj.push_back(json_spirit::Pair("content", std::string(notes[i].content.mb_str(wxConvUTF8))));
            noteObj.push_back(json_spirit::Pair("completed", notes[i].completed));
            
            root.push_back(noteObj);
        }
        
        // Write JSON to file
        std::ofstream file(filePath.mb_str(wxConvUTF8).data());
        if (!file.is_open()) {
            return false;
        }
        
        json_spirit::write_formatted(root, file);
        file.close();
        
        modified = false;
        return true;
    } catch (...) {
        return false;
    }
}

void NotesWindow::UpdateNotesList() {
    // Clear existing items
    notesList->DeleteAllItems();
    
    // Add notes to the list control
    for (size_t i = 0; i < notes.size(); i++) {
        long idx = notesList->InsertItem(i, "", notes[i].completed ? 1 : 0);
        notesList->SetItem(idx, 1, notes[i].title);
    }
}

void NotesWindow::UpdateStatusIcon() {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        bool isCompleted = statusCheckbox->GetValue();
        statusIcon->SetBitmap(CreateStatusBitmap(isCompleted));
        
        // Update list item status icon
        notesList->SetItem(selected, 0, "", isCompleted ? 1 : 0);
    }
}

long NotesWindow::GetSelectedNoteIndex() const {
    return notesList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

void NotesWindow::OnSave(wxCommandEvent& WXUNUSED(event)) {
    SaveNotes();
}

void NotesWindow::OnClose(wxCommandEvent& WXUNUSED(event)) {
    // Save notes before closing if modified
    if (modified) {
        SaveNotes();
    }
    
    // Close the dialog
    EndModal(wxID_CLOSE);
}

void NotesWindow::OnStatusChange(wxCommandEvent& WXUNUSED(event)) {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        // Update status icon
        bool isCompleted = statusCheckbox->GetValue();
        notes[selected].completed = isCompleted;
        statusIcon->SetBitmap(CreateStatusBitmap(isCompleted));
        
        // Update list item status icon
        notesList->SetItem(selected, 0, "", isCompleted ? 1 : 0);
        
        modified = true;
        
        // Save immediately on status change
        SaveNotes();
    }
}

void NotesWindow::OnTextChanged(wxCommandEvent& WXUNUSED(event)) {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        // Update the current note's content
        notes[selected].content = notesText->GetValue();
        modified = true;
        
        // Trigger auto-save after a short delay to avoid 
        // excessive saves during typing
        if (!autoSaveTimer.IsRunning()) {
            autoSaveTimer.StartOnce(1500); // 1.5 seconds delay
        }
    }
}

void NotesWindow::OnTitleChanged(wxCommandEvent& WXUNUSED(event)) {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        wxString newTitle = titleText->GetValue();
        notes[selected].title = newTitle;
        
        // Update the list item title
        notesList->SetItem(selected, 1, newTitle);
        
        modified = true;
        
        // Trigger auto-save on title change after a short delay
        // to avoid excessive saves while typing
        if (!autoSaveTimer.IsRunning()) {
            autoSaveTimer.StartOnce(1500); // 1.5 seconds delay
        }
    }
}

void NotesWindow::OnNoteSelected(wxListEvent& WXUNUSED(event)) {
    // Get the newly selected note index
    long newSelected = GetSelectedNoteIndex();
    
    // Ensure it's valid
    if (newSelected < 0 || newSelected >= (long)notes.size())
        return;
    
    // We don't want to trigger recursive selection events
    static bool isSelecting = false;
    if (isSelecting)
        return;
    
    isSelecting = true;
    
    // Always save the currently displayed note data first before switching
    // regardless of whether 'modified' flag is set
    long oldSelected = -1;
    
    // Find the currently loaded note (which may not be the same as the newly selected one)
    // We'll save it based on the current UI values to ensure no data loss
    for (size_t i = 0; i < notes.size(); i++) {
        if (notes[i].title == titleText->GetValue() && 
            notes[i].content == notesText->GetValue() &&
            notes[i].completed == statusCheckbox->GetValue()) {
            oldSelected = i;
            break;
        }
    }
    
    // If we couldn't find an exact match, use the current selection
    if (oldSelected == -1) {
        // Update the note that was being edited before the selection changed
        // Get all currently displayed items
        wxString currentTitle = titleText->GetValue();
        wxString currentContent = notesText->GetValue();
        bool currentCompleted = statusCheckbox->GetValue();
        
        // Check which notes in our list might match based on title
        for (size_t i = 0; i < notes.size(); i++) {
            if (i != newSelected && notes[i].title == currentTitle) {
                oldSelected = i;
                break;
            }
        }
        
        // If we found a possible match, update it
        if (oldSelected >= 0 && oldSelected < (long)notes.size()) {
            notes[oldSelected].title = currentTitle;
            notes[oldSelected].content = currentContent;
            notes[oldSelected].completed = currentCompleted;
            
            // Update the list display
            notesList->SetItem(oldSelected, 1, currentTitle);
            notesList->SetItem(oldSelected, 0, "", currentCompleted ? 1 : 0);
            
            // Mark as modified to trigger auto-save
            modified = true;
        }
    }
    
    // Now load the newly selected note
    titleText->SetValue(notes[newSelected].title);
    notesText->SetValue(notes[newSelected].content);
    statusCheckbox->SetValue(notes[newSelected].completed);
    statusIcon->SetBitmap(CreateStatusBitmap(notes[newSelected].completed));
    
    // Enable note content area
    titleText->Enable(true);
    notesText->Enable(true);
    statusCheckbox->Enable(true);
    
    // Save to disk if any changes were made
    if (modified) {
        SaveNotes();
    }
    
    isSelecting = false;
}

void NotesWindow::OnAddNote(wxCommandEvent& WXUNUSED(event)) {
    wxTextEntryDialog dialog(this, "Enter a title for the new note:", "New Note", "New Note");
    
    if (dialog.ShowModal() == wxID_OK) {
        wxString title = dialog.GetValue();
        if (!title.IsEmpty()) {
            // Check if a note with this title already exists
            bool titleExists = false;
            for (size_t i = 0; i < notes.size(); i++) {
                if (notes[i].title == title) {
                    titleExists = true;
                    
                    // Select the existing note with this title
                    notesList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    notesList->EnsureVisible(i);
                    
                    // Show a message
                    wxMessageBox("A note with this title already exists. It has been selected.",
                                "Note Exists", wxOK | wxICON_INFORMATION);
                    break;
                }
            }
            
            // Only add a new note if the title is unique
            if (!titleExists) {
                // Save current note
                SaveSelectedNote();
                
                // Add new note
                notes.push_back(MapNote(title, "", false));
                
                // Update list and select new note
                long newIndex = notesList->GetItemCount();
                notesList->InsertItem(newIndex, "", 0);
                notesList->SetItem(newIndex, 1, title);
                
                // Select the new item
                notesList->SetItemState(newIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                notesList->EnsureVisible(newIndex);
                
                modified = true;
            }
        }
    }
}

void NotesWindow::OnDeleteNote(wxCommandEvent& WXUNUSED(event)) {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        // Confirm deletion
        wxMessageDialog dialog(this, 
            wxString::Format("Are you sure you want to delete the note '%s'?", notes[selected].title),
            "Confirm Deletion", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        
        if (dialog.ShowModal() == wxID_YES) {
            // Remove the note
            notes.erase(notes.begin() + selected);
            
            // If no notes left, create a default one
            if (notes.empty()) {
                notes.push_back(MapNote("Default Note", "", false));
            }
            
            // Update list
            UpdateNotesList();
            
            // Select another note
            long newSelected = (selected >= (long)notes.size()) ? notes.size() - 1 : selected;
            notesList->SetItemState(newSelected, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            
            modified = true;
        }
    }
}

void NotesWindow::OnRenameNote(wxCommandEvent& WXUNUSED(event)) {
    long selected = GetSelectedNoteIndex();
    if (selected >= 0 && selected < (long)notes.size()) {
        wxTextEntryDialog dialog(this, "Enter a new title for the note:", "Rename Note", 
                               notes[selected].title);
        
        if (dialog.ShowModal() == wxID_OK) {
            wxString newTitle = dialog.GetValue();
            if (!newTitle.IsEmpty()) {
                // Update note title
                notes[selected].title = newTitle;
                titleText->SetValue(newTitle);
                
                // Update list item
                notesList->SetItem(selected, 1, newTitle);
                
                modified = true;
            }
        }
    }
}

void NotesWindow::OnListItemRightClick(wxListEvent& event) {
    // Show context menu on right click
    ShowPopupMenu(event.GetPoint());
}

void NotesWindow::ShowPopupMenu(wxPoint pos) {
    long selected = GetSelectedNoteIndex();
    if (selected == -1)
        return;
    
    // Create popup menu
    wxMenu menu;
    
    // Add menu items
    menu.Append(ID_POPUP_RENAME, "Rename Note");
    menu.Append(ID_POPUP_DELETE, "Delete Note");
    menu.AppendSeparator();
    
    // Add completion status items
    if (notes[selected].completed) {
        menu.Append(ID_POPUP_MARK_PENDING, "Mark as Pending");
    } else {
        menu.Append(ID_POPUP_MARK_COMPLETED, "Mark as Completed");
    }
    
    // Show popup menu
    wxDialog::PopupMenu(&menu, pos);
}

void NotesWindow::OnPopupClick(wxCommandEvent& event) {
    long selected = GetSelectedNoteIndex();
    if (selected == -1)
        return;
    
    // Handle popup menu items
    switch (event.GetId()) {
        case ID_POPUP_DELETE:
            OnDeleteNote(event);
            break;
        
        case ID_POPUP_RENAME:
            OnRenameNote(event);
            break;
        
        case ID_POPUP_MARK_COMPLETED:
            notes[selected].completed = true;
            statusCheckbox->SetValue(true);
            statusIcon->SetBitmap(CreateStatusBitmap(true));
            notesList->SetItem(selected, 0, "", 1);
            modified = true;
            break;
        
        case ID_POPUP_MARK_PENDING:
            notes[selected].completed = false;
            statusCheckbox->SetValue(false);
            statusIcon->SetBitmap(CreateStatusBitmap(false));
            notesList->SetItem(selected, 0, "", 0);
            modified = true;
            break;
    }
}

void NotesWindow::OnAutoSave(wxTimerEvent& WXUNUSED(event)) {
    // Auto-save if modified
    if (modified) {
        SaveNotes();
    }
} 
