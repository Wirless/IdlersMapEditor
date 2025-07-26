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
#include "wallize_window.h"
#include "editor.h"
#include "gui.h"

wxBEGIN_EVENT_TABLE(WallizeWindow, wxDialog)
    EVT_BUTTON(WALLIZE_NEXT_BUTTON, WallizeWindow::OnClickNext)
    EVT_BUTTON(WALLIZE_CANCEL_BUTTON, WallizeWindow::OnCancel)
    EVT_SPINCTRL(WALLIZE_CHUNK_SIZE_SPIN, WallizeWindow::OnChunkSizeChange)
wxEND_EVENT_TABLE()

WallizeWindow::WallizeWindow(wxWindow* parent, Editor& editor) :
    wxDialog(parent, wxID_ANY, "Wallize Progress", wxDefaultPosition, wxSize(400, 200)),
    editor(editor),
    current_chunk(0),
    total_chunks(0),
    processing_whole_map(false)
{
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(this, wxID_ANY, "Wallizing Map");
    wxFont title_font = title->GetFont();
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(title_font);
    main_sizer->Add(title, 0, wxALL | wxCENTER, 10);
    
    // Progress bar
    progress = new wxGauge(this, wxID_ANY, 100);
    main_sizer->Add(progress, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Status text
    status_text = new wxStaticText(this, wxID_ANY, "Preparing to wallize...");
    main_sizer->Add(status_text, 0, wxALL | wxCENTER, 5);
    
    // Chunk size setting
    wxBoxSizer* chunk_sizer = new wxBoxSizer(wxHORIZONTAL);
    chunk_sizer->Add(new wxStaticText(this, wxID_ANY, "Tiles per chunk:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    chunk_size_spin = new wxSpinCtrl(this, WALLIZE_CHUNK_SIZE_SPIN, "500", wxDefaultPosition, wxSize(80, -1));
    chunk_size_spin->SetRange(100, 5000);
    chunk_size_spin->SetValue(500);
    chunk_size_spin->SetToolTip("Number of tiles to process per chunk. Lower values use less memory but are slower.");
    chunk_sizer->Add(chunk_size_spin, 0, wxALIGN_CENTER_VERTICAL);
    main_sizer->Add(chunk_sizer, 0, wxALL | wxCENTER, 5);
    
    // Buttons
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    next_button = new wxButton(this, WALLIZE_NEXT_BUTTON, "Next Chunk");
    cancel_button = new wxButton(this, WALLIZE_CANCEL_BUTTON, "Cancel");
    
    button_sizer->Add(next_button, 0, wxRIGHT, 5);
    button_sizer->Add(cancel_button, 0, wxLEFT, 5);
    main_sizer->Add(button_sizer, 0, wxALL | wxCENTER, 10);
    
    SetSizer(main_sizer);
    Center();
}

WallizeWindow::~WallizeWindow() {
    // Nothing to clean up
}

void WallizeWindow::Start() {
    // Initialize the tile list based on whether we're processing selection or whole map
    if (editor.selection.size() > 0) {
        processing_whole_map = false;
        for (Tile* tile : editor.selection) {
            remaining_tiles.push_back(tile);
        }
    } else {
        processing_whole_map = true;
        for (TileLocation* tileLocation : editor.map) {
            if (Tile* tile = tileLocation->get()) {
                remaining_tiles.push_back(tile);
            }
        }
    }
    
    total_chunks = (remaining_tiles.size() + 499) / 500; // Process 500 tiles per chunk
    UpdateProgress(0, total_chunks);
    ShowModal();
}

void WallizeWindow::UpdateProgress(int current, int total) {
    progress->SetValue((current * 100) / total);
    status_text->SetLabel(wxString::Format("Processing chunk %d of %d (%d tiles remaining)",
        current + 1, total, (int)remaining_tiles.size()));
}

void WallizeWindow::OnChunkSizeChange(wxSpinEvent& event) {
    // Recalculate total chunks based on new chunk size
    if (!remaining_tiles.empty()) {
        total_chunks = (remaining_tiles.size() + chunk_size_spin->GetValue() - 1) / chunk_size_spin->GetValue();
        UpdateProgress(current_chunk, total_chunks);
    }
}

void WallizeWindow::OnClickNext(wxCommandEvent& WXUNUSED(event)) {
    if (remaining_tiles.empty()) {
        EndModal(1);
        return;
    }
    
    // Process next chunk of tiles using the user-defined chunk size
    Action* action = editor.actionQueue->createAction(ACTION_WALLIZE);
    
    size_t chunk_size = chunk_size_spin->GetValue();
    size_t tiles_to_process = std::min<size_t>(chunk_size, remaining_tiles.size());
    
    for (size_t i = 0; i < tiles_to_process; ++i) {
        Tile* tile = remaining_tiles.front();
        remaining_tiles.pop_front();
        
        Tile* newTile = tile->deepCopy(editor.map);
        newTile->wallize(&editor.map);
        if (!processing_whole_map) {
            newTile->select();
        }
        action->addChange(newd Change(newTile));
    }
    
    editor.addAction(action);
    current_chunk++;
    UpdateProgress(current_chunk, total_chunks);
    
    if (remaining_tiles.empty()) {
        next_button->SetLabel("Finish");
    }
    
    editor.map.doChange();
    // Refresh the view after each chunk
    g_gui.RefreshView();
}

void WallizeWindow::OnCancel(wxCommandEvent& WXUNUSED(event)) {
    EndModal(0);
} 