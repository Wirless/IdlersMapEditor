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

#ifndef RME_WALLIZE_WINDOW_H_
#define RME_WALLIZE_WINDOW_H_

#include "main.h"

class Editor;

class WallizeWindow : public wxDialog {
public:
    WallizeWindow(wxWindow* parent, Editor& editor);
    virtual ~WallizeWindow();

    void Start();
    
private:
    void UpdateProgress(int current, int total);
    
    // Event handlers
    void OnClickNext(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnChunkSizeChange(wxSpinEvent& event);
    
    Editor& editor;
    
    // Progress tracking
    std::list<Tile*> remaining_tiles;
    int current_chunk;
    int total_chunks;
    bool processing_whole_map;
    
    // UI elements
    wxGauge* progress;
    wxStaticText* status_text;
    wxButton* next_button;
    wxButton* cancel_button;
    wxSpinCtrl* chunk_size_spin;
    
    DECLARE_EVENT_TABLE()
};

enum {
    WALLIZE_NEXT_BUTTON = 1000,
    WALLIZE_CANCEL_BUTTON,
    WALLIZE_CHUNK_SIZE_SPIN
};

#endif 