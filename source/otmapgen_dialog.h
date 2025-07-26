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

#ifndef RME_OTMAPGEN_DIALOG_H_
#define RME_OTMAPGEN_DIALOG_H_

#include "main.h"
#include "otmapgen.h"
#include <wx/listctrl.h>

// Forward declaration for item preview button
class OTMapGenItemButton;

// Generator types for tab-based interface
enum GeneratorType {
	GENERATOR_ISLAND = 0,
	GENERATOR_MOUNTAIN = 1,
	GENERATOR_DUNGEON = 2,
	GENERATOR_CUSTOM = 3
};

// Item preview button class (similar to ReplaceItemsButton)
class OTMapGenItemButton : public wxButton {
public:
	OTMapGenItemButton(wxWindow* parent, wxWindowID id = wxID_ANY);
	
	void SetItemId(uint16_t id);
	uint16_t GetItemId() const { return m_id; }
	
	void OnPaint(wxPaintEvent& event);
	void OnClick(wxCommandEvent& event);
	
private:
	uint16_t m_id;
	
	DECLARE_EVENT_TABLE()
};

class OTMapGenDialog : public wxDialog {
public:
	OTMapGenDialog(wxWindow* parent);
	~OTMapGenDialog();

	// Event handlers
	void OnGenerate(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnPreview(wxCommandEvent& event);
	void OnSeedChange(wxCommandEvent& event);
	void OnParameterChange(wxSpinEvent& event);
	void OnParameterChangeText(wxCommandEvent& event);
	void OnMountainTypeChange(wxCommandEvent& event);
	void OnFloorUp(wxCommandEvent& event);
	void OnFloorDown(wxCommandEvent& event);
	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	
	// Terrain layer management events
	void OnTerrainLayerSelect(wxListEvent& event);
	void OnTerrainLayerAdd(wxCommandEvent& event);
	void OnTerrainLayerRemove(wxCommandEvent& event);
	void OnTerrainLayerMoveUp(wxCommandEvent& event);
	void OnTerrainLayerMoveDown(wxCommandEvent& event);
	void OnTerrainLayerEdit(wxCommandEvent& event);
	void OnBrushChoice(wxCommandEvent& event);
	void OnItemIdChange(wxCommandEvent& event);
	
	// New island generator events
	void OnIslandParameterChange(wxCommandEvent& event);
	void OnIslandParameterSpin(wxSpinEvent& event);
	void OnRollDice(wxCommandEvent& event);
	void OnTabChanged(wxBookCtrlEvent& event);
	
	// Dungeon generator event handlers
	void OnDungeonParameterChange(wxCommandEvent& event);
	void OnDungeonParameterSpin(wxSpinEvent& event);
	void OnDungeonWallBrushChange(wxCommandEvent& event);
	void OnDungeonRollDice(wxCommandEvent& event);

	// Helper functions
	bool GenerateMap();
	bool GenerateIslandMap();
	bool GenerateIslandMapBatched(const IslandConfig& config, int width, int height, const std::string& seed);
	bool GenerateDungeonMap();
	void UpdatePreview();
	void UpdateDungeonPreview();
	void UpdatePreviewFloor();
	void UpdateFloorLabel();
	void UpdateZoomLabel();
	GenerationConfig BuildGenerationConfig();
	IslandConfig BuildIslandConfig();
	DungeonConfig BuildDungeonConfig();
	void GetTilePreviewColor(uint16_t tileId, unsigned char& r, unsigned char& g, unsigned char& b);
	
	// Tab-specific value getters
	int GetCurrentWidth();
	int GetCurrentHeight();
	std::string GetCurrentSeed();
	std::string GetCurrentVersion();
	
	// Terrain layer management helpers
	void PopulateTerrainLayerList();
	void PopulateBrushChoices();
	void UpdateLayerControls();
	TerrainLayer* GetSelectedLayer();
	void SetSelectedLayer(const TerrainLayer& layer);
	
	// Dice roll helpers
	void RollIslandDice();
	void UpdateIslandControlsFromConfig();
	
	// Brush integration helpers
	uint16_t getActualItemIdFromBrush(const class Brush* brush) const;
	void OnWaterIdButtonClick(wxCommandEvent& event);
	void OnGroundIdButtonClick(wxCommandEvent& event);
	void OnItemIdTextChange(wxCommandEvent& event);

	// Preset management helpers
	void LoadGenerationPresets();
	void SaveGenerationPresets();
	void LoadPreset(const std::string& presetName);
	void SaveCurrentAsPreset(const std::string& presetName);
	void OnPresetLoad(wxCommandEvent& event);
	void OnPresetSave(wxCommandEvent& event);
	void OnPresetDelete(wxCommandEvent& event);

protected:
	// Dialog controls
	wxNotebook* notebook;
	
	// Island tab specific controls
	wxSpinCtrl* island_width_spin_ctrl;
	wxSpinCtrl* island_height_spin_ctrl;
	wxTextCtrl* island_seed_text_ctrl;
	wxChoice* island_version_choice;
	
	// Main tab specific controls (legacy generator)
	wxSpinCtrl* main_width_spin_ctrl;
	wxSpinCtrl* main_height_spin_ctrl;
	wxTextCtrl* main_seed_text_ctrl;
	wxChoice* main_version_choice;
	
	// Shared controls
	wxSpinCtrl* floors_to_generate_spin;
	wxChoice* mountain_type_choice;
	wxCheckBox* terrain_only_checkbox;
	wxCheckBox* sand_biome_checkbox;
	wxCheckBox* smooth_coastline_checkbox;
	wxCheckBox* add_caves_checkbox;
	
	// Preset management controls
	wxChoice* preset_choice;
	wxButton* preset_save_button;
	wxButton* preset_delete_button;
	wxTextCtrl* preset_name_text;
	
	// Basic settings (shared across all generators)
	wxTextCtrl* seed_text_ctrl;
	wxSpinCtrl* width_spin_ctrl;
	wxSpinCtrl* height_spin_ctrl;
	wxChoice* version_choice;
	
	// Advanced settings (legacy)
	wxTextCtrl* noise_increment_text;
	wxTextCtrl* island_distance_text;
	wxTextCtrl* cave_depth_text;
	wxTextCtrl* cave_roughness_text;
	wxTextCtrl* cave_chance_text;
	wxTextCtrl* water_level_text;
	wxTextCtrl* exponent_text;
	wxTextCtrl* linear_text;
	
	// Island Generator Tab Controls
	wxPanel* island_panel;
	wxTextCtrl* island_noise_scale_text;
	wxSpinCtrl* island_noise_octaves_spin;
	wxTextCtrl* island_noise_persistence_text;
	wxTextCtrl* island_noise_lacunarity_text;
	wxTextCtrl* island_size_text;
	wxTextCtrl* island_falloff_text;
	wxTextCtrl* island_threshold_text;
	wxSpinCtrl* island_water_id_spin;
	wxSpinCtrl* island_ground_id_spin;
	wxTextCtrl* island_water_level_text;
	wxButton* island_roll_dice_button;
	
	// Island cleanup controls
	wxCheckBox* island_enable_cleanup_checkbox;
	wxSpinCtrl* island_min_patch_size_spin;
	wxSpinCtrl* island_max_hole_size_spin;
	wxSpinCtrl* island_smoothing_passes_spin;
	
	// Island item ID controls with preview
	OTMapGenItemButton* island_water_id_button;
	wxTextCtrl* island_water_id_text;
	OTMapGenItemButton* island_ground_id_button; 
	wxTextCtrl* island_ground_id_text;
	
	// Mountain Generator Tab Controls (for future use)
	wxPanel* mountain_panel;
	wxSpinCtrl* mountain_fill_id_spin;
	wxSpinCtrl* mountain_ground_id_spin;
	wxTextCtrl* mountain_height_threshold_text;
	wxTextCtrl* mountain_slope_factor_text;
	wxButton* mountain_roll_dice_button;
	
	// Dungeon Generator Tab Controls
	wxPanel* dungeon_panel;
	wxSpinCtrl* dungeon_width_spin_ctrl;
	wxSpinCtrl* dungeon_height_spin_ctrl;
	wxTextCtrl* dungeon_seed_text_ctrl;
	wxChoice* dungeon_version_choice;
	wxChoice* dungeon_wall_brush_choice;
	wxStaticBitmap* dungeon_wall_preview_bitmap;
	wxSpinCtrl* dungeon_corridor_width_spin;
	wxSpinCtrl* dungeon_room_min_size_spin;
	wxSpinCtrl* dungeon_room_max_size_spin;
	wxSpinCtrl* dungeon_room_count_spin;
	wxSpinCtrl* dungeon_corridor_count_spin;
	wxTextCtrl* dungeon_complexity_text;
	wxCheckBox* dungeon_add_dead_ends_checkbox;
	wxCheckBox* dungeon_circular_rooms_checkbox;
	wxButton* dungeon_roll_dice_button;
	wxButton* dungeon_preview_button;
	wxStaticBitmap* dungeon_preview_bitmap;
	
	// Intersection controls (new feature for triple/quadruple corridors)
	wxCheckBox* dungeon_triple_intersections_checkbox;
	wxCheckBox* dungeon_quad_intersections_checkbox;
	wxSpinCtrl* dungeon_intersection_count_spin;
	wxSpinCtrl* dungeon_intersection_size_spin;
	wxTextCtrl* dungeon_intersection_probability_text;
	
	// Corridor length controls (prevent massive tunnels)
	wxSpinCtrl* dungeon_max_corridor_length_spin;
	wxCheckBox* dungeon_smart_pathfinding_checkbox;
	wxCheckBox* dungeon_prefer_intersections_checkbox;
	
	// Layout design controls (new advanced tab)
	wxListCtrl* terrain_layer_list;
	wxButton* add_layer_button;
	wxButton* remove_layer_button;
	wxButton* move_up_button;
	wxButton* move_down_button;
	wxButton* edit_layer_button;
	
	// Layer properties panel
	wxPanel* layer_properties_panel;
	wxTextCtrl* layer_name_text;
	wxChoice* layer_brush_choice;
	wxSpinCtrl* layer_item_id_spin;
	wxTextCtrl* height_min_text;
	wxTextCtrl* height_max_text;
	wxTextCtrl* moisture_min_text;
	wxTextCtrl* moisture_max_text;
	wxTextCtrl* noise_scale_text;
	wxTextCtrl* coverage_text;
	wxCheckBox* use_borders_checkbox;
	wxCheckBox* layer_enabled_checkbox;
	wxSpinCtrl* z_order_spin;
	
	// Cave and water configuration
	wxChoice* cave_brush_choice;
	wxSpinCtrl* cave_item_id_spin;
	wxChoice* water_brush_choice;
	wxSpinCtrl* water_item_id_spin;
	
	// Preview controls - separate for each tab
	wxStaticBitmap* island_preview_bitmap;  // Island tab preview
	wxStaticBitmap* main_preview_bitmap;    // Map Generation tab preview
	wxButton* island_preview_button;        // Island tab preview button
	wxButton* main_preview_button;          // Map Generation tab preview button
	wxButton* floor_up_button;
	wxButton* floor_down_button;
	wxStaticText* floor_label;
	wxButton* zoom_in_button;
	wxButton* zoom_out_button;
	wxStaticText* zoom_label;
	
	// Buttons
	wxButton* generate_button;
	wxButton* cancel_button;
	
	// Preview state
	wxBitmap* current_preview;
	std::vector<std::vector<uint16_t>> current_layers;
	int current_preview_floor;
	double current_zoom;
	int preview_offset_x;
	int preview_offset_y;
	
	// Preview optimization tracking
	int preview_actual_width;
	int preview_actual_height;
	bool preview_is_scaled;
	
	// Terrain layer management
	std::vector<TerrainLayer> working_terrain_layers;
	std::vector<std::string> available_brushes;
	
	// Generator configurations
	IslandConfig island_config;
	MountainConfig mountain_config;
	DungeonConfig dungeon_config;
	GeneratorType current_generator_type;
	
	// Preset management
	struct GenerationPreset {
		std::string name;
		GeneratorType type;
		std::string seed;
		IslandConfig island_config;
		MountainConfig mountain_config;
		DungeonConfig dungeon_config;
		int width;
		int height;
		std::string version;
	};
	
	std::vector<GenerationPreset> generation_presets;
	std::string presets_file_path;

	DECLARE_EVENT_TABLE()
};

enum {
	ID_GENERATE = 1000,
	ID_PREVIEW,
	ID_SEED_TEXT,
	ID_WIDTH_SPIN,
	ID_HEIGHT_SPIN,
	ID_VERSION_CHOICE,
	ID_MOUNTAIN_TYPE_CHOICE,
	ID_NOISE_INCREMENT_TEXT,
	ID_ISLAND_DISTANCE_TEXT,
	ID_CAVE_DEPTH_TEXT,
	ID_CAVE_ROUGHNESS_TEXT,
	ID_CAVE_CHANCE_TEXT,
	ID_WATER_LEVEL_TEXT,
	ID_EXPONENT_TEXT,
	ID_LINEAR_TEXT,
	ID_FLOOR_UP,
	ID_FLOOR_DOWN,
	ID_ZOOM_IN,
	ID_ZOOM_OUT,
	
	// Terrain layer management IDs
	ID_TERRAIN_LAYER_LIST,
	ID_ADD_LAYER,
	ID_REMOVE_LAYER,
	ID_MOVE_UP_LAYER,
	ID_MOVE_DOWN_LAYER,
	ID_EDIT_LAYER,
	ID_LAYER_BRUSH_CHOICE,
	ID_LAYER_ITEM_ID_SPIN,
	ID_CAVE_BRUSH_CHOICE,
	ID_CAVE_ITEM_ID_SPIN,
	ID_WATER_BRUSH_CHOICE,
	ID_WATER_ITEM_ID_SPIN,
	
	// Island generator IDs
	ID_ISLAND_NOISE_SCALE,
	ID_ISLAND_NOISE_OCTAVES,
	ID_ISLAND_NOISE_PERSISTENCE,
	ID_ISLAND_NOISE_LACUNARITY,
	ID_ISLAND_SIZE,
	ID_ISLAND_FALLOFF,
	ID_ISLAND_THRESHOLD,
	ID_ISLAND_WATER_ID,
	ID_ISLAND_GROUND_ID,
	ID_ISLAND_WATER_LEVEL,
	ID_ISLAND_ROLL_DICE,
	
	// Island cleanup IDs
	ID_ISLAND_ENABLE_CLEANUP,
	ID_ISLAND_MIN_PATCH_SIZE,
	ID_ISLAND_MAX_HOLE_SIZE,
	ID_ISLAND_SMOOTHING_PASSES,
	
	// Island item preview button IDs
	ID_ISLAND_WATER_ID_BUTTON,
	ID_ISLAND_GROUND_ID_BUTTON,
	ID_ISLAND_WATER_ID_TEXT,
	ID_ISLAND_GROUND_ID_TEXT,
	
	// Mountain generator IDs
	ID_MOUNTAIN_FILL_ID,
	ID_MOUNTAIN_GROUND_ID,
	ID_MOUNTAIN_HEIGHT_THRESHOLD,
	ID_MOUNTAIN_SLOPE_FACTOR,
	ID_MOUNTAIN_ROLL_DICE,
	
	// Dungeon generator IDs
	ID_DUNGEON_WIDTH_SPIN,
	ID_DUNGEON_HEIGHT_SPIN,
	ID_DUNGEON_SEED_TEXT,
	ID_DUNGEON_VERSION_CHOICE,
	ID_DUNGEON_WALL_BRUSH_CHOICE,
	ID_DUNGEON_CORRIDOR_WIDTH_SPIN,
	ID_DUNGEON_ROOM_MIN_SIZE_SPIN,
	ID_DUNGEON_ROOM_MAX_SIZE_SPIN,
	ID_DUNGEON_ROOM_COUNT_SPIN,
	ID_DUNGEON_CORRIDOR_COUNT_SPIN,
	ID_DUNGEON_COMPLEXITY_TEXT,
	ID_DUNGEON_ADD_DEAD_ENDS,
	ID_DUNGEON_CIRCULAR_ROOMS,
	ID_DUNGEON_ROLL_DICE,
	ID_DUNGEON_PREVIEW,
	
	// Intersection IDs (new feature for triple/quadruple corridors)
	ID_DUNGEON_TRIPLE_INTERSECTIONS,
	ID_DUNGEON_QUAD_INTERSECTIONS,
	ID_DUNGEON_INTERSECTION_COUNT,
	ID_DUNGEON_INTERSECTION_SIZE,
	ID_DUNGEON_INTERSECTION_PROBABILITY,
	
	// Corridor length control IDs
	ID_DUNGEON_MAX_CORRIDOR_LENGTH,
	ID_DUNGEON_SMART_PATHFINDING,
	ID_DUNGEON_PREFER_INTERSECTIONS,
	
	// Preset management IDs
	ID_PRESET_CHOICE,
	ID_PRESET_SAVE,
	ID_PRESET_DELETE,
	ID_PRESET_NAME_TEXT,
	
	// Tab change event
	ID_NOTEBOOK_TAB_CHANGED
};

#endif 