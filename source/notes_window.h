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

#ifndef RME_NOTES_WINDOW_H_
#define RME_NOTES_WINDOW_H_

#include "main.h"
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/statbmp.h>
#include <wx/imaglist.h>
#include "editor.h"

struct MapNote {
    wxString title;
    wxString content;
    bool completed;
    
    MapNote() : completed(false) {}
    MapNote(const wxString& title, const wxString& content, bool completed = false) :
        title(title), content(content), completed(completed) {}
};

class NotesWindow : public wxDialog {
public:
    enum {
        ID_POPUP_DELETE = wxID_HIGHEST + 1,
        ID_POPUP_RENAME,
        ID_POPUP_MARK_COMPLETED,
        ID_POPUP_MARK_PENDING
    };

    NotesWindow(wxWindow* parent, Editor* editor);
    ~NotesWindow();

    void OnSave(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnStatusChange(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnTitleChanged(wxCommandEvent& event);
    void OnAutoSave(wxTimerEvent& event);
    void OnNoteSelected(wxListEvent& event);
    void OnAddNote(wxCommandEvent& event);
    void OnDeleteNote(wxCommandEvent& event);
    void OnRenameNote(wxCommandEvent& event);
    void OnListItemRightClick(wxListEvent& event);
    void OnPopupClick(wxCommandEvent& event);

    bool LoadNotes();
    bool LoadOldFormatNotes(const wxString& filePath);
    bool SaveNotes();
    wxString GetNotesDirectory() const;
    wxString GetNotesFilePath() const;
    
    void UpdateNotesList();
    void UpdateStatusIcon();
    long GetSelectedNoteIndex() const;
    void InitImageList();
    void SaveSelectedNote();

private:
    void ShowPopupMenu(wxPoint pos);
    
    Editor* editor;
    wxTextCtrl* notesText;
    wxTextCtrl* titleText;
    wxCheckBox* statusCheckbox;
    wxStaticBitmap* statusIcon;
    wxListCtrl* notesList;
    wxSplitterWindow* splitter;
    wxTimer autoSaveTimer;
    bool modified;
    wxImageList* imageList;
    
    std::vector<MapNote> notes;

    DECLARE_EVENT_TABLE()
};

#endif // RME_NOTES_WINDOW_H_ 