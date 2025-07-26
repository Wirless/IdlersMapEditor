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

#ifndef RME_OTMAPGEN_H_
#define RME_OTMAPGEN_H_

#include "main.h"
#include <vector>
#include <string>
#include <map>
#include <random>

// Forward declarations
class BaseMap;
class Tile;

// Island-specific configuration
struct IslandConfig {
	// Noise parameters for island generation
	double noise_scale = 0.01;
	double noise_octaves = 4;
	double noise_persistence = 0.5;
	double noise_lacunarity = 2.0;
	
	// Island shape parameters
	double island_size = 0.8;
	double island_falloff = 2.0;
	double island_threshold = 0.3;
	
	// Tile IDs for island generation
	uint16_t water_id = 4608;  // Water tile
	uint16_t ground_id = 4526; // Grass tile
	
	// Water/ground ratio control
	double water_level = 0.5;
	
	// Post-processing cleanup parameters
	bool enable_cleanup = true;          // Enable post-processing cleanup
	int min_land_patch_size = 4;        // Minimum size for land patches (in tiles)
	int max_water_hole_size = 3;        // Maximum size for water holes to fill (in tiles)
	int smoothing_passes = 2;           // Number of smoothing passes to apply
	
	// Target floor (fixed to 7 for islands)
	int target_floor = 7;
};

// Mountain-specific configuration (for future use)
struct MountainConfig {
	uint16_t fill_id = 4608;    // Fill material (water, lava, etc.)
	uint16_t ground_id = 919;   // Ground material (stone, grass, etc.)
	double height_threshold = 0.6;
	double slope_factor = 1.5;
	int target_floor = 7;
};


struct DungeonConfig {
	// Wall brush configuration
	std::string wall_brush = "brick wall";  // Default wall brush from walls.xml
	uint16_t wall_id = 1026;                // Default brick wall horizontal ID
	
	// Ground/floor configuration
	std::string ground_brush = "dirt";    // Ground brush for corridors and rooms
	uint16_t ground_id = 351;               // Default floor tile ID
	
	// Fill material for solid areas (outside walls)
	std::string fill_brush = "earth (soft)"; // Fill brush for solid areas
	uint16_t fill_id = 101;                   // Default earth/stone fill
	
	// Layout parameters
	int corridor_width = 1;      // Gap between walls (1-4 sqm)
	int room_min_size = 3;       // Minimum room radius (3-8)
	int room_max_size = 6;       // Maximum room radius (4-12)
	int room_count = 8;          // Number of rooms to generate (4-20)
	int corridor_count = 12;     // Number of corridor segments (8-30)
	double complexity = 0.3;     // Maze complexity factor (0.1-0.8)
	
	// Generation options
	bool add_dead_ends = true;     // Add dead-end corridors for complexity
	bool circular_rooms = false;   // Generate circular vs rectangular rooms
	bool connect_all_rooms = true; // Ensure all rooms are connected
	
	// Multi-way intersection options (crossroads)
	bool add_triple_intersections = true;   // Add 3-way intersections (T-junctions)
	bool add_quad_intersections = true;     // Add 4-way intersections (crossroads) 
	int intersection_count = 4;             // Number of intersection hubs to create (2-8)
	int intersection_size = 2;              // Size of intersection areas (1-3 sqm radius)
	double intersection_probability = 0.3;  // Chance for each potential intersection (0.1-0.8)
	
	// Corridor length controls (prevent massive tunnels)
	int max_corridor_length = 50;          // Maximum direct corridor length (10-200)
	bool use_smart_pathfinding = true;     // Use A* pathfinding for shorter routes
	bool prefer_intersections = true;      // Route through intersections when possible
	int corridor_segments = 3;             // Break long corridors into segments (2-6)
	
	// Target floor
	int target_floor = 7;
};


struct borderDooDads_config {
// there we will generate small patches on ground near walls enclosing small areas with "dirt patch"
// or grass patch or sand or anything like that would add more variety to the dungeon.

};

struct dungeonItems_config {
    //Urns tables chairs just general doodads fill to our generated map this is step 3-4 like finishing so we generate each part piece by piece so we can randomize it visually
    //those wont show preview but rather place and replace within our generated map so that we can easily see 
    //what we generated and re-generate by removing and placing again. roll dice for each item and position :)

};


// Terrain layer configuration for multiple ground types
struct TerrainLayer {
    std::string name;
    std::string brush_name;        // Name of brush from grounds.xml
    uint16_t item_id;             // Primary item ID for this layer
    double height_min = 0.0;      // Minimum height for this terrain
    double height_max = 1.0;      // Maximum height for this terrain
    double moisture_min = -1.0;   // Minimum moisture for this terrain
    double moisture_max = 1.0;    // Maximum moisture for this terrain
    double noise_scale = 1.0;     // Scale factor for noise sampling
    double coverage = 1.0;        // Coverage probability (0.0 to 1.0)
    bool use_borders = true;      // Whether to apply borders from brush
    bool enabled = true;          // Whether this layer is active
    int z_order = 1000;          // Rendering order (higher = on top)
    int min_floor = 7;  // Minimum floor where this layer can appear (7 = ground level)
    int max_floor = 7;  // Maximum floor where this layer can appear
};

struct GenerationConfig {
    std::string seed;
    int width = 256;
    int height = 256;
    std::string version = "10.98";
    bool terrain_only = false;
    
    // Floor generation control
    int floors_to_generate = 8;  // Number of floors to generate (1-15, default 8 for compatibility)
    int base_floor = 7;          // Base floor level (Tibia coordinates, 7 = ground level)
    
    // Generation parameters
    double noise_increment = 1.0;
    double island_distance_decrement = 0.92;
    double island_distance_exponent = 0.25;
    int cave_depth = 20;
    double cave_roughness = 0.45;
    double cave_chance = 0.09;
    bool sand_biome = true;
    bool euclidean = false;
    bool smooth_coastline = true;
    bool add_caves = true;
    int water_level = 7;  // Changed to match Tibia coordinates (7 = ground level)
    double exponent = 1.4;
    double linear = 6.0;
    std::string mountain_type = "MOUNTAIN";
    
    // Configurable terrain layers instead of hardcoded items
    std::vector<TerrainLayer> terrain_layers;
    
    // Cave configuration
    std::string cave_brush_name = "cave";
    uint16_t cave_item_id = 351;  // Default cave floor from grounds.xml
    
    // Water configuration  
    std::string water_brush_name = "sea";
    uint16_t water_item_id = 4608;  // Default water from grounds.xml
    
    // Frequency weights for noise generation
    struct FrequencyWeight {
        double frequency;
        double weight;
    };
    
    std::vector<FrequencyWeight> frequencies = {
        {1.0, 0.3}, {2.0, 0.2}, {4.0, 0.2}, {8.0, 0.125},
        {16.0, 0.1}, {32.0, 0.05}, {64.0, 0.0025}
    };
    
    // Initialize with default terrain layers
    void initializeDefaultLayers() {
        terrain_layers.clear();
        
        // Water layer (lowest)
        TerrainLayer water;
        water.name = "Water";
        water.brush_name = "sea";
        water.item_id = 4608;
        water.height_min = -1.0;
        water.height_max = 0.0;
        water.moisture_min = -1.0;
        water.moisture_max = 1.0;
        water.noise_scale = 1.0;
        water.coverage = 1.0;
        water.use_borders = true;
        water.z_order = 6000;
        terrain_layers.push_back(water);
        
        // Grass layer (main land)
        TerrainLayer grass;
        grass.name = "Grass";
        grass.brush_name = "grass";
        grass.item_id = 4526;
        grass.height_min = 0.0;
        grass.height_max = 0.7;
        grass.moisture_min = -0.5;
        grass.moisture_max = 1.0;
        grass.noise_scale = 1.0;
        grass.coverage = 1.0;
        grass.use_borders = true;
        grass.z_order = 3500;
        terrain_layers.push_back(grass);
        
        // Sand layer (dry areas)
        TerrainLayer sand;
        sand.name = "Sand";
        sand.brush_name = "sand";
        sand.item_id = 231;
        sand.height_min = 0.0;
        sand.height_max = 0.6;
        sand.moisture_min = -1.0;
        sand.moisture_max = -0.6;
        sand.noise_scale = 1.5;
        sand.coverage = 1.0;
        sand.use_borders = true;
        sand.z_order = 3400;
        sand.enabled = sand_biome;
        terrain_layers.push_back(sand);
        
        // Mountain layer (high elevation)
        TerrainLayer mountain;
        mountain.name = "Mountain";
        mountain.brush_name = "mountain";
        mountain.item_id = 919;  // From grounds.xml mountain brush
        mountain.height_min = 0.7;
        mountain.height_max = 1.0;
        mountain.moisture_min = -1.0;
        mountain.moisture_max = 1.0;
        mountain.noise_scale = 0.8;
        mountain.coverage = 1.0;
        mountain.use_borders = true;
        mountain.z_order = 9900;
        terrain_layers.push_back(mountain);
    }
};

class SimplexNoise {
public:
    SimplexNoise(unsigned int seed = 0);
    
    double noise(double x, double y);
    double fractal(double x, double y, const std::vector<GenerationConfig::FrequencyWeight>& frequencies);
    
private:
    static const int SIMPLEX[64][4];
    static const double F2, G2;
    
    int perm[512];
    int permMod12[512];
    
    void initializePermutation(unsigned int seed);
    double dot(const int g[], double x, double y);
    int fastfloor(double x);
};

class OTMapGenerator {
public:
    OTMapGenerator();
    ~OTMapGenerator();
    
    // Main generation function
    bool generateMap(BaseMap* map, const GenerationConfig& config);
    
    // Preview generation
    std::vector<std::vector<uint16_t>> generateLayers(const GenerationConfig& config);
    
    // Island-specific generation (floor 7 only)
    bool generateIslandMap(BaseMap* map, const struct IslandConfig& config, int width, int height, const std::string& seed);
    std::vector<std::vector<uint16_t>> generateIslandLayer(const struct IslandConfig& config, int width, int height, const std::string& seed);
    std::vector<std::vector<uint16_t>> generateIslandLayerBatch(const IslandConfig& config, int width, int height, const std::string& seed, int offsetX, int offsetY, int totalWidth, int totalHeight);
    std::vector<std::vector<double>> generateIslandHeightMap(const IslandConfig& config, int width, int height, const std::string& seed);
    
    // Dungeon generation functions
    bool generateDungeonMap(BaseMap* map, const struct DungeonConfig& config, int width, int height, const std::string& seed);
    
    // Core generation methods
    std::vector<std::vector<double>> generateHeightMap(const GenerationConfig& config);
    std::vector<std::vector<double>> generateMoistureMap(const GenerationConfig& config);
    std::vector<std::vector<uint16_t>> generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, 
                                                          const std::vector<std::vector<double>>& moistureMap,
                                                          const GenerationConfig& config);
    
    // Island post-processing methods
    void cleanupIslandTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, const IslandConfig& config, int width, int height);
    void removeIsolatedPixels(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id);
    void removeSmallPatches(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id, int min_size);
    void fillSmallHoles(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, uint16_t target_id, uint16_t fill_id, int max_hole_size);
    void smoothTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config);
    int floodFillCount(std::vector<std::vector<uint16_t>>& terrainLayer, int x, int y, int width, int height, uint16_t target_id, uint16_t replacement_id);
    
    // Helper methods for multi-floor generation
    void fillColumn(std::vector<std::vector<std::vector<uint16_t>>>& layers, 
                   int x, int y, int elevation, uint16_t surfaceTileId, 
                   const GenerationConfig& config);
	void generateCaves(std::vector<std::vector<std::vector<uint16_t>>>& layers, const GenerationConfig& config);

    uint16_t getTerrainTileId(double height, double moisture, const GenerationConfig& config);
    const TerrainLayer* selectTerrainLayer(double height, double moisture, const GenerationConfig& config);
    
    // Island generation helper functions
    double getIslandDistance(int x, int y, int centerX, int centerY, double island_size);
    double applyIslandFalloff(double distance, double falloff);
    double generateIslandNoise(double x, double y, const IslandConfig& config);
    
    // Border generation (integrate with brush system)
    void generateBorders(BaseMap* map, const GenerationConfig& config);
    void addBordersToTile(BaseMap* map, Tile* tile, int x, int y, int z);
    
    // Decoration placement
    void addClutter(BaseMap* map, const GenerationConfig& config);
    void placeTreesAndVegetation(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeStones(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeCaveDecorations(BaseMap* map, Tile* tile);
    
    // Dungeon generation functions
    std::vector<std::vector<uint16_t>> generateDungeonLayer(const DungeonConfig& config, int width, int height, const std::string& seed);
    
    // Room structure for dungeon generation
    struct Room {
        int centerX, centerY;
        int radius;
        bool isCircular;
    };
    
    // Intersection structure for multi-way corridor hubs
    struct Intersection {
        int centerX, centerY;
        int size;                    // Radius of the intersection area
        int connectionCount;         // Number of corridors connecting (3 or 4)
        std::vector<std::pair<int, int>> connections; // Connected room/intersection positions
    };
    
    // Dungeon generation helper functions
    std::vector<Room> generateRooms(const DungeonConfig& config, int width, int height);
    void placeRoom(std::vector<std::vector<int>>& grid, const Room& room, const DungeonConfig& config);
    void generateCorridors(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const DungeonConfig& config, int width, int height);
    void createCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height);
    void createImprovedCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height);
    void createCorridorTile(std::vector<std::vector<int>>& grid, int centerX, int centerY, const DungeonConfig& config, int width, int height);
    void addDeadEnds(std::vector<std::vector<int>>& grid, const DungeonConfig& config, int width, int height);
    bool isWallPosition(const std::vector<std::vector<int>>& grid, int x, int y, int width, int height);
    
    // Multi-way intersection generation functions
    std::vector<Intersection> generateIntersections(const DungeonConfig& config, const std::vector<Room>& rooms, int width, int height);
    void placeIntersection(std::vector<std::vector<int>>& grid, const Intersection& intersection, const DungeonConfig& config);
    void connectRoomsViaIntersections(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const std::vector<Intersection>& intersections, const DungeonConfig& config, int width, int height);
    bool findIntersectionPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height);
    
    // Smart corridor generation (prevent massive tunnels)
    void createSmartCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height);
    std::vector<std::pair<int, int>> findShortestPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height, int maxLength);
    void createCorridorSegments(std::vector<std::vector<int>>& grid, const std::vector<std::pair<int, int>>& path, const DungeonConfig& config, int width, int height);
    std::pair<int, int> findNearestIntersection(const std::vector<Intersection>& intersections, int x, int y);

private:
    SimplexNoise* noise_generator;
    std::mt19937 rng;
    
    // Cave generation
    std::vector<std::vector<uint16_t>> generateCaveLayer(const GenerationConfig& config);
    
    // Utility functions
    double getDistance(int x, int y, int centerX, int centerY, bool euclidean = false);
    double smoothstep(double edge0, double edge1, double x);
    void seedRandom(const std::string& seed);
};

// Helper functions for tile creation and manipulation
namespace OTMapGenUtils {
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z);
    void setGroundTile(Tile* tile, uint16_t itemId);
    void addDecoration(Tile* tile, uint16_t itemId);
    bool isWaterTile(uint16_t itemId);
    bool isLandTile(uint16_t itemId);
    bool isMountainTile(uint16_t itemId);
    
    // Brush system integration
    std::vector<std::string> getAvailableBrushes();
    uint16_t getPrimaryItemFromBrush(const std::string& brushName);
    bool applyBrushToTile(BaseMap* map, Tile* tile, const std::string& brushName, int x, int y, int z);
}

#endif 
