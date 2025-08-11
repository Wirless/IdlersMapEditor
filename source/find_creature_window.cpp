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
#include "find_creature_window.h"
#include "common_windows.h"
#include "gui.h"
#include "creatures.h"
#include "creature_brush.h"
#include "spawn_brush.h"
#include "spawn.h"
#include "map.h"
#include "editor.h"
#include "result_window.h"

// ============================================================================
// FindCreatureListBox

class FindCreatureListBox : public wxListBox {
public:
    FindCreatureListBox(wxWindow* parent, wxWindowID id) : wxListBox(parent, id, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB) {
        // ...
    }
    
    ~FindCreatureListBox() {
        // ...
    }
};

// ============================================================================
// FindCreatureDialog

BEGIN_EVENT_TABLE(FindCreatureDialog, wxDialog)
    EVT_TEXT(wxID_ANY, FindCreatureDialog::OnText)
    EVT_TIMER(wxID_ANY, FindCreatureDialog::OnInputTimer)
    EVT_BUTTON(wxID_OK, FindCreatureDialog::OnClickOK)
    EVT_BUTTON(wxID_CANCEL, FindCreatureDialog::OnClickCancel)
    EVT_BUTTON(wxID_REFRESH, FindCreatureDialog::OnRefreshClick)
END_EVENT_TABLE()

FindCreatureDialog::FindCreatureDialog(wxWindow* parent, const wxString& title) :
    wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(600, 500), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    input_timer(this) {
    
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);
    
    wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* options_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Name search controls
    wxStaticBoxSizer* name_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Creature Name"), wxVERTICAL);
    name_text_input = new wxTextCtrl(name_box_sizer->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    name_box_sizer->Add(name_text_input, 0, wxALL | wxEXPAND, 5);
    options_sizer->Add(name_box_sizer, 0, wxALL | wxEXPAND, 5);
    
    // Type selection
    wxStaticBoxSizer* type_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Type"), wxVERTICAL);
    search_monsters = new wxCheckBox(type_box_sizer->GetStaticBox(), wxID_ANY, "Monsters", wxDefaultPosition, wxDefaultSize, 0);
    search_monsters->SetValue(true);
    type_box_sizer->Add(search_monsters, 0, wxALL, 5);
    
    search_npcs = new wxCheckBox(type_box_sizer->GetStaticBox(), wxID_ANY, "NPCs", wxDefaultPosition, wxDefaultSize, 0);
    search_npcs->SetValue(true);
    type_box_sizer->Add(search_npcs, 0, wxALL, 5);
    
    options_sizer->Add(type_box_sizer, 0, wxALL | wxEXPAND, 5);
    
    // Auto-refresh option
    wxStaticBoxSizer* refresh_box_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Options"), wxVERTICAL);
    auto_refresh = new wxCheckBox(refresh_box_sizer->GetStaticBox(), wxID_ANY, "Auto Refresh", wxDefaultPosition, wxDefaultSize, 0);
    auto_refresh->SetValue(true);
    refresh_box_sizer->Add(auto_refresh, 0, wxALL, 5);
    
    options_sizer->Add(refresh_box_sizer, 0, wxALL | wxEXPAND, 5);
    
    // Add spacer
    options_sizer->Add(0, 0, 1, wxEXPAND, 5);
    
    // Add buttons at the bottom
    buttons_box_sizer = new wxStdDialogButtonSizer();
    ok_button = new wxButton(this, wxID_OK);
    buttons_box_sizer->AddButton(ok_button);
    cancel_button = new wxButton(this, wxID_CANCEL);
    buttons_box_sizer->AddButton(cancel_button);
    refresh_button = new wxButton(this, wxID_REFRESH, "Refresh");
    buttons_box_sizer->Add(refresh_button);
    buttons_box_sizer->Realize();
    options_sizer->Add(buttons_box_sizer, 0, wxALIGN_CENTER | wxALL, 5);
    
    main_sizer->Add(options_sizer, 1, wxALL | wxEXPAND, 5);
    
    // Creature list
    wxStaticBoxSizer* creature_list_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, "Creatures"), wxVERTICAL);
    creatures_list = new FindCreatureListBox(this, wxID_ANY);
    creature_list_sizer->Add(creatures_list, 1, wxALL | wxEXPAND, 5);
    
    main_sizer->Add(creature_list_sizer, 1, wxALL | wxEXPAND, 5);
    
    SetSizer(main_sizer);
    Layout();
    Centre(wxBOTH);
    
    RefreshContentsInternal();
}

FindCreatureDialog::~FindCreatureDialog() {
    // Nothing to do
}

void FindCreatureDialog::OnText(wxCommandEvent& event) {
    input_timer.Start(300, true);
}

void FindCreatureDialog::OnInputTimer(wxTimerEvent& event) {
    if (auto_refresh->GetValue()) {
        RefreshContentsInternal();
    }
}

void FindCreatureDialog::RefreshContentsInternal() {
    creatures_list->Clear();
    
    bool include_monsters = search_monsters->GetValue();
    bool include_npcs = search_npcs->GetValue();
    wxString search_text = name_text_input->GetValue().Lower();
    
    // Add matching creatures from the database to the list
    for (CreatureMap::iterator iter = g_creatures.begin(); iter != g_creatures.end(); ++iter) {
        CreatureType* creature_type = iter->second;
        if (!creature_type) continue;
        
        // Filter by type
        if ((creature_type->isNpc && !include_npcs) || 
            (!creature_type->isNpc && !include_monsters)) {
            continue;
        }
        
        // Filter by name
        wxString creature_name = wxString(creature_type->name.c_str(), wxConvUTF8).Lower();
        if (!search_text.IsEmpty() && creature_name.Find(search_text) == wxNOT_FOUND) {
            continue;
        }
        
        // Add to list
        int index = creatures_list->Append(wxString(creature_type->name.c_str(), wxConvUTF8));
        creatures_list->SetClientData(index, creature_type);
    }
    
    if (creatures_list->GetCount() > 0) {
        creatures_list->SetSelection(0);
    }
}

void FindCreatureDialog::OnClickOK(wxCommandEvent& event) {
    int selection = creatures_list->GetSelection();

	int creature_found_count; //
	std::string creature_name; // 

    if (selection != wxNOT_FOUND) {
        CreatureType* creature_type = reinterpret_cast<CreatureType*>(creatures_list->GetClientData(selection));
        if (creature_type) {
            result_name = wxString(creature_type->name.c_str(), wxConvUTF8);
            
            // Find all instances of this creature in spawns across the map
            Editor* editor = g_gui.GetCurrentEditor();
            if (editor) {
                Map& map = editor->getMap();
                // Create a results window to display the found spawns
                SearchResultWindow* result_window = g_gui.ShowSearchWindow();
                result_window->Clear();
                
                // Iterate through all tiles to find spawns with matching creatures
                size_t creature_found_count = 0;
                
                g_gui.CreateLoadBar("Searching for creatures...");
                
                const std::string creature_name = creature_type->name;
                size_t spawn_count = 0;
                size_t total_spawns = 0;
                
                // Count total spawns for progress reporting
                for (auto spawn_iter = map.spawns.begin(); spawn_iter != map.spawns.end(); ++spawn_iter) {
                    ++total_spawns;
                }
                
                // Much more efficient: iterate through spawns directly instead of all map tiles
                for (auto spawn_iter = map.spawns.begin(); spawn_iter != map.spawns.end(); ++spawn_iter) {
                    ++spawn_count;
                    if (spawn_count % 10 == 0) {
                        g_gui.SetLoadDone(int(spawn_count * 100.0 / total_spawns));
                        
                        // Allow UI updates to prevent freezing
                        wxSafeYield();
                    }
                    
                    const Position& spawn_pos = *spawn_iter;
                    Tile* spawn_tile = map.getTile(spawn_pos);
                    if (!spawn_tile || !spawn_tile->spawn) continue;
                    
                    int spawn_radius = spawn_tile->spawn->getSize();
                    bool found_matching_creature = false;
                    
                    // Check all tiles within spawn radius for creatures
                    for (int sx = -spawn_radius; sx <= spawn_radius && !found_matching_creature; ++sx) {
                        for (int sy = -spawn_radius; sy <= spawn_radius && !found_matching_creature; ++sy) {
                            // Skip if outside circular spawn radius
                            if (sx * sx + sy * sy > spawn_radius * spawn_radius) continue;
                            
                            Tile* creature_tile = map.getTile(spawn_pos.x + sx, spawn_pos.y + sy, spawn_pos.z);
                            if (!creature_tile || !creature_tile->creature) continue;
                            
                            // Check if this creature matches our search
                            if (creature_tile->creature->getName() == creature_name) {
                                found_matching_creature = true;
                                wxString description = wxString::Format("%s at (%d,%d,%d)", 
                                    wxString(creature_name.c_str(), wxConvUTF8), 
                                    creature_tile->getPosition().x, 
                                    creature_tile->getPosition().y, 
                                    creature_tile->getPosition().z);
                                result_window->AddPosition(description, creature_tile->getPosition());
                                ++creature_found_count;
                            }
                        }
                    }
                    
                    // If no creature of this type found, still show the spawn position
                    if (!found_matching_creature) {
                        wxString description = wxString::Format("Spawn for %s at (%d,%d,%d)", 
                            wxString(creature_name.c_str(), wxConvUTF8), 
                            spawn_pos.x, spawn_pos.y, spawn_pos.z);
                        result_window->AddPosition(description, spawn_pos);
                        ++creature_found_count;
                    }
                }
                
                g_gui.DestroyLoadBar();

				

                wxString result_message;
                if (creature_found_count == 0) {
                    result_message = wxString::Format("No %s found on the map.", wxString(creature_name.c_str(), wxConvUTF8));
                    g_gui.PopupDialog("Search completed", result_message, wxOK);
                } else {
                    result_message = wxString::Format("Found %d instances of %s on the map.", 
                        creature_found_count, wxString(creature_name.c_str(), wxConvUTF8));
                    g_gui.SetStatusText(result_message);
                }
            }
            
            EndModal(wxID_OK);
        }
    }
}

void FindCreatureDialog::OnClickCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void FindCreatureDialog::OnRefreshClick(wxCommandEvent& event) {
    RefreshContentsInternal();
}

void FindCreatureDialog::OnClose(wxCloseEvent& event) {
    EndModal(wxID_CANCEL);
} 
