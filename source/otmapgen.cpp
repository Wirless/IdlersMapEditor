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
#include "otmapgen.h"
#include "basemap.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "map.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <random>
#include <queue>
#include <map>
#include <numeric>
#include <vector>  // if you're using std::vector
#include <algorithm> // if using other STL algorithms
#include <random>

// SimplexNoise implementation
const double SimplexNoise::F2 = 0.5 * (sqrt(3.0) - 1.0);
const double SimplexNoise::G2 = (3.0 - sqrt(3.0)) / 6.0;



const int SimplexNoise::SIMPLEX[64][4] = {
    {0,1,2,3},{0,1,3,2},{0,0,0,0},{0,2,3,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,2,3,0},
    {0,2,1,3},{0,0,0,0},{0,3,1,2},{0,3,2,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,3,2,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {1,2,0,3},{0,0,0,0},{1,3,0,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,3,0,1},{2,3,1,0},
    {1,0,2,3},{1,0,3,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,0,3,1},{0,0,0,0},{2,1,3,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {2,0,1,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,0,1,2},{3,0,2,1},{0,0,0,0},{3,1,2,0},
    {2,1,0,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,1,0,2},{0,0,0,0},{3,2,0,1},{3,2,1,0}
};

SimplexNoise::SimplexNoise(unsigned int seed) {
    initializePermutation(seed);
}

void SimplexNoise::initializePermutation(unsigned int seed) {
    // Initialize permutation table based on seed
    std::mt19937 rng(seed);
    
    // Initialize with sequential values
    for (int i = 0; i < 256; i++) {
        perm[i] = i;
    }
    
    // Shuffle the permutation table
    for (int i = 255; i > 0; i--) {
        int j = rng() % (i + 1);
        std::swap(perm[i], perm[j]);
    }
    
    // Duplicate the permutation table
    for (int i = 0; i < 256; i++) {
        perm[256 + i] = perm[i];
        permMod12[i] = perm[i] % 12;
        permMod12[256 + i] = perm[i] % 12;
    }
}

int SimplexNoise::fastfloor(double x) {
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

double SimplexNoise::dot(const int g[], double x, double y) {
    return g[0] * x + g[1] * y;
}

double SimplexNoise::noise(double xin, double yin) {
    static const int grad3[12][3] = {
        {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
        {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
        {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
    };
    
    double n0, n1, n2; // Noise contributions from the three corners
    
    // Skew the input space to determine which simplex cell we're in
    double s = (xin + yin) * F2; // Hairy factor for 2D
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);
    double t = (i + j) * G2;
    double X0 = i - t; // Unskew the cell origin back to (x,y) space
    double Y0 = j - t;
    double x0 = xin - X0; // The x,y distances from the cell origin
    double y0 = yin - Y0;
    
    // For the 2D case, the simplex shape is an equilateral triangle
    // Determine which simplex we are in
    int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
    if (x0 > y0) {
        i1 = 1; j1 = 0; // lower triangle, XY order: (0,0)->(1,0)->(1,1)
    } else {
        i1 = 0; j1 = 1; // upper triangle, YX order: (0,0)->(0,1)->(1,1)
    }
    
    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6
    double x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
    double y2 = y0 - 1.0 + 2.0 * G2;
    
    // Work out the hashed gradient indices of the three simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = permMod12[ii + perm[jj]];
    int gi1 = permMod12[ii + i1 + perm[jj + j1]];
    int gi2 = permMod12[ii + 1 + perm[jj + 1]];
    
    // Calculate the contribution from the three corners
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
    }
    
    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
    }
    
    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
    }
    
    // Add contributions from each corner to get the final noise value
    // The result is scaled to return values in the interval [-1,1]
    return 70.0 * (n0 + n1 + n2);
}

double SimplexNoise::fractal(double x, double y, const std::vector<GenerationConfig::FrequencyWeight>& frequencies) {
    double value = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& freq : frequencies) {
        value += noise(x * freq.frequency, y * freq.frequency) * freq.weight;
        totalWeight += freq.weight;
    }
    
    return totalWeight > 0 ? value / totalWeight : 0.0;
}

// OTMapGenerator implementation
OTMapGenerator::OTMapGenerator() : noise_generator(nullptr) {
    seedRandom("default");
}

OTMapGenerator::~OTMapGenerator() {
    if (noise_generator) {
        delete noise_generator;
        noise_generator = nullptr;
    }
}

void OTMapGenerator::seedRandom(const std::string& seed) {
    // Try to parse as 64-bit integer first, like original OTMapGen
    unsigned long long numeric_seed = 0;
    
    try {
        // Try to parse as number
        numeric_seed = std::stoull(seed);
    } catch (...) {
        // If parsing fails, fall back to string hash
        std::hash<std::string> hasher;
        numeric_seed = hasher(seed);
    }
    
    // Initialize noise generator and RNG with 64-bit seed
    delete noise_generator;
    noise_generator = new SimplexNoise(static_cast<unsigned int>(numeric_seed & 0xFFFFFFFF));
    rng.seed(static_cast<unsigned int>(numeric_seed >> 32) ^ static_cast<unsigned int>(numeric_seed & 0xFFFFFFFF));
}

bool OTMapGenerator::generateMap(BaseMap* map, const GenerationConfig& config) {
    if (!map) {
        return false;
    }
    
    // Cast BaseMap to Map to access the editor's action system
    Map* editorMap = static_cast<Map*>(map);
    
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map and moisture map
    auto heightMap = generateHeightMap(config);
    auto moistureMap = generateMoistureMap(config);
    
    // Generate terrain layer
    auto terrainLayer = generateTerrainLayer(heightMap, moistureMap, config);
    
    // Apply terrain to map using Actions like the editor does
    std::vector<Position> tilesToGenerate;
    
    // First pass: collect all positions that need tiles
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            uint16_t tileId = terrainLayer[y][x];
            if (tileId != 0) {
                tilesToGenerate.push_back(Position(x, y, config.base_floor));
            }
        }
    }
    
    // Process tiles in batches to avoid memory issues
    const int BATCH_SIZE = 1000;
    
    for (size_t start = 0; start < tilesToGenerate.size(); start += BATCH_SIZE) {
        size_t end = std::min(start + BATCH_SIZE, tilesToGenerate.size());
        
        // Create a batch of tiles
        for (size_t i = start; i < end; ++i) {
            Position pos = tilesToGenerate[i];
            uint16_t tileId = terrainLayer[pos.y][pos.x];
            
            // Create tile location and allocate tile properly
            TileLocation* location = editorMap->createTileL(pos);
            Tile* existing_tile = location->get();
            Tile* new_tile = nullptr;
            
            if (existing_tile) {
                // Copy existing tile and modify it
                new_tile = existing_tile->deepCopy(*editorMap);
            } else {
                // Create new tile
                new_tile = editorMap->allocator(location);
            }
            
            // Set the ground using proper API
            if (new_tile) {
                // Remove existing ground if any
                if (new_tile->ground) {
                    delete new_tile->ground;
                    new_tile->ground = nullptr;
                }
                
                // Create new ground item
                Item* groundItem = Item::Create(tileId);
                if (groundItem) {
                    new_tile->ground = groundItem;
                }
                
                // Set the tile back to the map
                editorMap->setTile(pos, new_tile);
            }
        }
    }
    
    // Generate caves if enabled
    if (config.add_caves) {
        auto caveLayer = generateCaveLayer(config);
        
        std::vector<Position> caveTilesToGenerate;
        
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                uint16_t caveId = caveLayer[y][x];
                if (caveId != 0) {
                    // Place cave tiles below the surface using base_floor as reference
                    for (int z = config.base_floor + 1; z <= config.base_floor + config.cave_depth && z <= 15; ++z) {
                        caveTilesToGenerate.push_back(Position(x, y, z));
                    }
                }
            }
        }
        
        // Process cave tiles in batches
        for (size_t start = 0; start < caveTilesToGenerate.size(); start += BATCH_SIZE) {
            size_t end = std::min(start + BATCH_SIZE, caveTilesToGenerate.size());
            
            for (size_t i = start; i < end; ++i) {
                Position pos = caveTilesToGenerate[i];
                uint16_t caveId = caveLayer[pos.y][pos.x];
                
                TileLocation* location = editorMap->createTileL(pos);
                Tile* existing_tile = location->get();
                Tile* new_tile = nullptr;
                
                if (existing_tile) {
                    new_tile = existing_tile->deepCopy(*editorMap);
                } else {
                    new_tile = editorMap->allocator(location);
                }
                
                if (new_tile) {
                    // Remove existing ground if any
                    if (new_tile->ground) {
                        delete new_tile->ground;
                        new_tile->ground = nullptr;
                    }
                    
                    // Create new ground item
                    Item* groundItem = Item::Create(caveId);
                    if (groundItem) {
                        new_tile->ground = groundItem;
                    }
                    
                    // Set the tile back to the map
                    editorMap->setTile(pos, new_tile);
                }
            }
        }
    }
    
    // Add decorations if not terrain only (simplified for now)
    if (!config.terrain_only) {
        // Find grass layer for decoration placement
        const TerrainLayer* grassLayer = nullptr;
        for (const auto& layer : config.terrain_layers) {
            if (layer.name == "Grass" && layer.enabled) {
                grassLayer = &layer;
                break;
            }
        }
        
        if (grassLayer) {
            for (int y = 0; y < config.height; ++y) {
                for (int x = 0; x < config.width; ++x) {
                    Tile* tile = editorMap->getTile(x, y, config.water_level);
                    if (tile && tile->ground) {
                        uint16_t groundId = tile->ground->getID();
                        
                        // Add vegetation to grass tiles
                        if (groundId == grassLayer->item_id) {
                            std::uniform_real_distribution<double> dist(0.0, 1.0);
                            if (dist(rng) < 0.05) { // 5% chance
                                Tile* new_tile = tile->deepCopy(*editorMap);
                                
                                // Use configurable decoration items (could be made configurable too)
                                uint16_t decorationId = 2700; // Default tree ID
                                double rand_val = dist(rng);
                                if (rand_val < 0.6) {
                                    decorationId = 2700; // Tree
                                } else if (rand_val < 0.8) {
                                    decorationId = 2785; // Bush
                                } else {
                                    decorationId = 2782; // Flower
                                }
                                
                                Item* decoration = Item::Create(decorationId);
                                if (decoration) {
                                    new_tile->addItem(decoration);
                                    editorMap->setTile(Position(x, y, config.water_level), new_tile);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

std::vector<std::vector<double>> OTMapGenerator::generateHeightMap(const GenerationConfig& config) {
    std::vector<std::vector<double>> heightMap(config.height, std::vector<double>(config.width, 0.0));
    
    double centerX = config.width / 2.0;
    double centerY = config.height / 2.0;
    double maxDistance = std::min(config.width, config.height) / 2.0;
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            // Enhanced noise for better island shapes
            double nx = x * config.noise_increment / config.width;
            double ny = y * config.noise_increment / config.height;
            
            // Multi-octave noise for more interesting terrain
            double noiseValue = 0.0;
            double amplitude = 1.0;
            double frequency = 1.0;
            double maxValue = 0.0;
            
            // Generate 4 octaves of noise for complex island shapes
            for (int i = 0; i < 4; i++) {
                noiseValue += noise_generator->noise(nx * frequency, ny * frequency) * amplitude;
                maxValue += amplitude;
                amplitude *= 0.5;
                frequency *= 2.0;
            }
            noiseValue /= maxValue; // Normalize
            
            // Apply island distance effect with better shaping
            double distance = getDistance(x, y, (int)centerX, (int)centerY, config.euclidean);
            double normalizedDistance = distance / maxDistance;
            
            // Create more interesting island shapes with noise-based distortion
            double distortionNoise = noise_generator->noise(x * 0.01, y * 0.01) * 0.3;
            normalizedDistance += distortionNoise;
            
            // Improved island falloff for organic shapes
            double distanceEffect = 1.0 - pow(std::max(0.0, normalizedDistance), config.island_distance_exponent);
            distanceEffect = std::max(0.0, distanceEffect * config.island_distance_decrement);
            
            // Apply sharper transitions for distinct land/water boundaries
            if (distanceEffect < 0.3) {
                distanceEffect *= distanceEffect; // Square it to create sharper dropoff
            }
            
            // Combine noise with distance effect
            double height = (noiseValue + 1.0) * 0.5; // Normalize to [0,1]
            height = pow(height, config.exponent) * config.linear;
            height *= distanceEffect;
            
            // Add some randomness to break up regular patterns
            height += (noise_generator->noise(x * 0.1, y * 0.1) * 0.05);
            height = std::max(0.0, std::min(1.0, height));
            
            heightMap[y][x] = height;
        }
    }
    
    return heightMap;
}

std::vector<std::vector<double>> OTMapGenerator::generateMoistureMap(const GenerationConfig& config) {
    std::vector<std::vector<double>> moistureMap(config.height, std::vector<double>(config.width, 0.0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            // Generate moisture noise with different scale than height
            double nx = x * 0.01; // Moisture varies more slowly
            double ny = y * 0.01;
            double moistureValue = noise_generator->noise(nx, ny);
            
            moistureMap[y][x] = moistureValue;
        }
    }
    
    return moistureMap;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, 
                                                      const std::vector<std::vector<double>>& moistureMap,
                                                      const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> terrainLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            double moisture = moistureMap[y][x];
            
            // Reduce moisture influence for more consistent terrain generation
            // This helps prevent strange terrain combinations on upper floors
            uint16_t tileId = getTerrainTileId(height, moisture * 0.7, config);
            terrainLayer[y][x] = tileId;
        }
    }
    
    return terrainLayer;
}

uint16_t OTMapGenerator::getTerrainTileId(double height, double moisture, const GenerationConfig& config) {
    // Select terrain layer based on height and reduced moisture influence
    const TerrainLayer* selectedLayer = selectTerrainLayer(height, moisture, config);
    
    if (selectedLayer) {
        return selectedLayer->item_id;
    }
    
    // Fallback to water if no layer matches
    return config.water_item_id;
}

const TerrainLayer* OTMapGenerator::selectTerrainLayer(double height, double moisture, const GenerationConfig& config) {
    // Sort layers by z-order (higher z-order = higher priority)
    std::vector<const TerrainLayer*> sortedLayers;
    for (const auto& layer : config.terrain_layers) {
        if (layer.enabled) {
            sortedLayers.push_back(&layer);
        }
    }
    
    std::sort(sortedLayers.begin(), sortedLayers.end(), 
        [](const TerrainLayer* a, const TerrainLayer* b) {
            return a->z_order > b->z_order; // Higher z-order first
        });
    
    // Find the first layer that matches the height and moisture criteria
    for (const TerrainLayer* layer : sortedLayers) {
        if (height >= layer->height_min && height <= layer->height_max &&
            moisture >= layer->moisture_min && moisture <= layer->moisture_max) {
            
            // Check coverage probability
            if (layer->coverage >= 1.0) {
                return layer;
            } else {
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < layer->coverage) {
                    return layer;
                }
            }
        }
    }
    
    return nullptr; // No matching layer found
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateCaveLayer(const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> caveLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double caveNoise = noise_generator->noise(x * config.cave_roughness, y * config.cave_roughness);
            
            // Random chance for cave generation
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < config.cave_chance && caveNoise > 0.1) {
                caveLayer[y][x] = config.cave_item_id; // Use configurable cave item ID
            }
        }
    }
    
    return caveLayer;
}

double OTMapGenerator::getDistance(int x, int y, int centerX, int centerY, bool euclidean) {
    if (euclidean) {
        double dx = x - centerX;
        double dy = y - centerY;
        return sqrt(dx * dx + dy * dy);
    } else {
        // Manhattan distance
        return abs(x - centerX) + abs(y - centerY);
    }
}

double OTMapGenerator::smoothstep(double edge0, double edge1, double x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * (3 - 2 * x);
}

void OTMapGenerator::generateBorders(BaseMap* map, const GenerationConfig& config) {
    // This would integrate with your existing border system
    // For now, we'll leave this as a placeholder since you already have border generation
}

void OTMapGenerator::addBordersToTile(BaseMap* map, Tile* tile, int x, int y, int z) {
    // Placeholder - integrate with your existing border system
}

void OTMapGenerator::addClutter(BaseMap* map, const GenerationConfig& config) {
    // Add decorative items to terrain
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            Tile* tile = map->getTile(x, y, config.water_level);
            if (tile && tile->ground) {
                uint16_t groundId = tile->ground->getID();
                
                // Find the terrain layer this ground belongs to
                const TerrainLayer* terrainLayer = nullptr;
                for (const auto& layer : config.terrain_layers) {
                    if (layer.item_id == groundId && layer.enabled) {
                        terrainLayer = &layer;
                        break;
                    }
                }
                
                if (terrainLayer) {
                    // Add decorations based on terrain type
                    if (terrainLayer->name == "Grass") {
                        placeTreesAndVegetation(map, tile, groundId);
                    } else if (terrainLayer->name == "Mountain" || terrainLayer->brush_name.find("stone") != std::string::npos) {
                        placeStones(map, tile, groundId);
                    }
                }
            }
        }
    }
}

void OTMapGenerator::placeTreesAndVegetation(BaseMap* map, Tile* tile, uint16_t groundId) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for vegetation
    if (dist(rng) < 0.1) { // 10% chance
        uint16_t decorationId;
        double rand_val = dist(rng);
        
        if (rand_val < 0.6) {
            decorationId = 2700; // Tree
        } else if (rand_val < 0.8) {
            decorationId = 2785; // Bush
        } else {
            decorationId = 2782; // Flower
        }
        
        OTMapGenUtils::addDecoration(tile, decorationId);
    }
}

void OTMapGenerator::placeStones(BaseMap* map, Tile* tile, uint16_t groundId) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for stones
    if (dist(rng) < 0.05) { // 5% chance
        uint16_t stoneId = (dist(rng) < 0.7) ? 1770 : 1771; // Small or large stone
        OTMapGenUtils::addDecoration(tile, stoneId);
    }
}

void OTMapGenerator::placeCaveDecorations(BaseMap* map, Tile* tile) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for cave decorations
    if (dist(rng) < 0.15) { // 15% chance
        OTMapGenUtils::addDecoration(tile, 1785); // Stalagmite
    }
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateLayers(const GenerationConfig& config) {
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map and moisture map
    auto heightMap = generateHeightMap(config);
    auto moistureMap = generateMoistureMap(config);
    
    // Create layers for configurable number of floors instead of hardcoded 8
    int floors = std::max(1, std::min(15, config.floors_to_generate)); // Clamp to valid range
    std::vector<std::vector<std::vector<uint16_t>>> layers(floors);
    for (int z = 0; z < floors; ++z) {
        layers[z] = std::vector<std::vector<uint16_t>>(config.height, std::vector<uint16_t>(config.width, 0));
    }
    
    // Fill terrain using the fillColumn approach like the original
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            double moisture = moistureMap[y][x];
            
            // Get terrain tile ID using the new configurable system
            uint16_t tileId = getTerrainTileId(height, moisture, config);
            
            // Calculate elevation for determining elevated content
            // Scale height to 0-7 range for determining what gets elevated content
            int elevation = static_cast<int>(std::max(0.0, std::min(7.0, height * 8.0)));
            
            // Fill column - this will put main terrain on layers[0] (Floor 7)
            // and add elevated content based on elevation value
            fillColumn(layers, x, y, elevation, tileId, config);
        }
    }
    
    // Add caves if enabled (now using configurable cave generation)
    if (config.add_caves && floors > 1) {
        auto caveLayer = generateCaveLayer(config);
        
        // Place caves on underground floors with decreasing probability
        // Skip surface floor (index 0 = Tibia floor 7), start from floor below surface
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                uint16_t caveId = caveLayer[y][x];
                if (caveId != 0) {
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    
                    // Place caves on available floors below surface
                    for (int floor_idx = 1; floor_idx < floors && floor_idx <= 3; ++floor_idx) {
                        double cave_probability = 0.8 * std::pow(0.6, floor_idx - 1); // Decreasing probability
                        if (dist(rng) < cave_probability) {
                            layers[floor_idx][y][x] = caveId;
                        }
                    }
                }
            }
        }
    }
    
    // Convert 3D layers to the format expected by the dialog (single layer for each floor)
    // Return layers in the order they map to Tibia floors:
    // layers[0] → Floor 7 (ground level)
    // layers[1] → Floor 6 (+1 above)
    // layers[7] → Floor 0 (+7 above)
    std::vector<std::vector<uint16_t>> result;
    for (int z = 0; z < floors; ++z) {
        std::vector<uint16_t> floorData;
        floorData.reserve(config.width * config.height);
        
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                floorData.push_back(layers[z][y][x]);
            }
        }
        result.push_back(floorData);
    }
    
    return result;
}

void OTMapGenerator::fillColumn(std::vector<std::vector<std::vector<uint16_t>>>& layers, 
                               int x, int y, int elevation, uint16_t surfaceTileId, 
                               const GenerationConfig& config) {
    int baseFloorIndex = 0;
    layers[baseFloorIndex][y][x] = surfaceTileId;
    
    if (elevation > 4) {
        int available_floors = static_cast<int>(layers.size());
        bool isMountainTerrain = (surfaceTileId == 919);
        
        if (isMountainTerrain) {
            int max_mountain_floors = std::min(available_floors - 1, 3);
            
            // Calculate mountain shape
            double mountainFactor = (elevation - 4) / 4.0;
            mountainFactor = std::pow(mountainFactor, 1.5);
            
            // Track edge detection
            std::vector<bool> isEdgeFloor(max_mountain_floors + 1, false);
            
            // Fill mountain floors from bottom up
            for (int floor_idx = 1; floor_idx <= max_mountain_floors; ++floor_idx) {
                double floorProbability = mountainFactor * (1.0 - (floor_idx - 1) * 0.25);
                
                // Check support and neighbors
                bool hasSupport = false;
                int supportCount = 0;
                std::vector<std::pair<int,int>> rockNeighbors;
                
                if (floor_idx > 1) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int checkY = y + dy;
                            int checkX = x + dx;
                            if (checkY >= 0 && checkY < config.height && 
                                checkX >= 0 && checkX < config.width) {
                                if (layers[floor_idx-1][checkY][checkX] == 919) {
                                    supportCount++;
                                    rockNeighbors.push_back({checkX, checkY});
                                    if (dx == 0 && dy == 0) hasSupport = true;
                                }
                            }
                        }
                    }
                } else {
                    hasSupport = true;
                    supportCount = 9;
                }
                
                // Determine if this should be an edge tile
                bool isEdge = (supportCount < 7);
                isEdgeFloor[floor_idx] = isEdge;
                
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (hasSupport && (floor_idx == 1 || dist(rng) < floorProbability)) {
                    // For edges, create grass platforms
                    if (isEdge) {
                        // Calculate grass platform size (1-5 tiles)
                        int platformSize = 1 + (int)(dist(rng) * 4);
                        bool canPlaceGrass = true;
                        
                        // Check if we have enough rock support for grass
                        for (const auto& neighbor : rockNeighbors) {
                            int dx = neighbor.first - x;
                            int dy = neighbor.second - y;
                            if (abs(dx) + abs(dy) <= platformSize) {
                                canPlaceGrass = true;
                                break;
                            }
                        }
                        
                        layers[floor_idx][y][x] = canPlaceGrass ? 4526 : 919;
                    } else {
                        layers[floor_idx][y][x] = 919;
                    }
                }
            }
        }
    }
}

void OTMapGenerator::generateCaves(std::vector<std::vector<std::vector<uint16_t>>>& layers, const GenerationConfig& config) {
    struct CaveNode {
        int x, y, floor;
        bool isRoom;
        int width, height;
        CaveNode* parent;
        std::vector<CaveNode*> children;
        
        CaveNode(int _x, int _y, int _f, bool _isRoom = false) 
            : x(_x), y(_y), floor(_f), isRoom(_isRoom), width(0), height(0), parent(nullptr) {}
    };
    
    // Find valid cave entrance points
    std::vector<CaveNode*> entrances;
    for (int floor = 1; floor < static_cast<int>(layers.size()); floor++) {
        for (int y = 2; y < config.height - 2; y++) {
            for (int x = 2; x < config.width - 2; x++) {
                if (layers[floor][y][x] == 919) {
                    // Check for good entrance location (edge of mountain)
                    int rockCount = 0;
                    bool hasOpen = false;
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (layers[floor][y+dy][x+dx] == 919) rockCount++;
                            else hasOpen = true;
                        }
                    }
                    
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    if (rockCount >= 6 && hasOpen && dist(rng) < 0.15) {
                        entrances.push_back(new CaveNode(x, y, floor));
                    }
                }
            }
        }
    }
    
    // Generate cave system from each entrance
    for (auto entrance : entrances) {
        std::queue<CaveNode*> nodeQueue;
        nodeQueue.push(entrance);
        
        while (!nodeQueue.empty()) {
            CaveNode* current = nodeQueue.front();
            nodeQueue.pop();
            
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            
            // Decide if we should create a room here
            if (dist(rng) < 0.3 && !current->isRoom) {
                current->isRoom = true;
                current->width = 3 + (int)(dist(rng) * 4);
                current->height = 4 + (int)(dist(rng) * 4);
                
                // Create the room
                for (int dy = -current->height/2; dy <= current->height/2; dy++) {
                    for (int dx = -current->width/2; dx <= current->width/2; dx++) {
                        int roomX = current->x + dx;
                        int roomY = current->y + dy;
                        
                        if (roomX > 1 && roomX < config.width-2 && 
                            roomY > 1 && roomY < config.height-2 && 
                            layers[current->floor][roomY][roomX] == 919) {
                            // Oval shape check
                            double normalizedX = dx / (double)(current->width/2);
                            double normalizedY = dy / (double)(current->height/2);
                            if ((normalizedX*normalizedX + normalizedY*normalizedY) <= 1.2) {
                                layers[current->floor][roomY][roomX] = 4405;
                            }
                        }
                    }
                }
                
                // Create 2-3 tunnel exits from room
                int numExits = 2 + (dist(rng) < 0.5 ? 1 : 0);
                for (int i = 0; i < numExits; i++) {
                    // Pick exit direction
                    double angle = dist(rng) * 2 * M_PI;
                    int exitX = current->x + (int)(cos(angle) * current->width);
                    int exitY = current->y + (int)(sin(angle) * current->height);
                    int exitFloor = current->floor + (dist(rng) < 0.3 ? ((dist(rng) < 0.5) ? 1 : -1) : 0);
                    
                    if (exitFloor >= 1 && exitFloor < static_cast<int>(layers.size()) &&
                        exitX > 1 && exitX < config.width-2 && 
                        exitY > 1 && exitY < config.height-2) {
                        auto newNode = new CaveNode(exitX, exitY, exitFloor);
                        current->children.push_back(newNode);
                        newNode->parent = current;
                        nodeQueue.push(newNode);
                    }
                }
            } else {
                // Create tunnel
                CaveNode* target = nullptr;
                if (!current->children.empty()) {
                    target = current->children[0];
                } else if (current->parent) {
                    target = current->parent;
                }
                
                if (target) {
                    // Generate tunnel path
                    int x = current->x, y = current->y, floor = current->floor;
                    while ((x != target->x || y != target->y || floor != target->floor) && 
                           x > 1 && x < config.width-2 && y > 1 && y < config.height-2) {
                        // Decide primary movement direction
                        if (dist(rng) < 0.8) { // 80% chance for cardinal movement
                            if (abs(target->x - x) > abs(target->y - y)) {
                                x += (target->x > x) ? 1 : -1;
                            } else {
                                y += (target->y > y) ? 1 : -1;
                            }
                        } else { // 20% chance for diagonal or floor change
                            if (floor != target->floor && dist(rng) < 0.5) {
                                floor += (target->floor > floor) ? 1 : -1;
                            } else {
                                x += (dist(rng) < 0.5) ? 1 : -1;
                                y += (dist(rng) < 0.5) ? 1 : -1;
                            }
                        }
                        
                        // Create tunnel tile if in rock
                        if (layers[floor][y][x] == 919) {
                            layers[floor][y][x] = 4405;
                            
                            // Sometimes widen the tunnel
                            if (dist(rng) < 0.3) {
                                for (int dy = -1; dy <= 1; dy++) {
                                    for (int dx = -1; dx <= 1; dx++) {
                                        if (dx == 0 && dy == 0) continue;
                                        int nx = x + dx, ny = y + dy;
                                        if (nx > 1 && nx < config.width-2 && 
                                            ny > 1 && ny < config.height-2 && 
                                            layers[floor][ny][nx] == 919) {
                                            layers[floor][ny][nx] = 4405;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Cleanup
    for (auto entrance : entrances) {
        delete entrance;
    }
}

// Utility functions
namespace OTMapGenUtils {
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z) {
        Position pos(x, y, z);
        Tile* tile = map->getTile(pos);
        if (!tile) {
            tile = map->allocator(map->createTileL(pos));
            map->setTile(pos, tile);
        }
        return tile;
    }
    
    void setGroundTile(Tile* tile, uint16_t itemId) {
        if (!tile) return;
        
        // Remove existing ground
        if (tile->ground) {
            delete tile->ground;
            tile->ground = nullptr;
        }
        
        // Create new ground item
        Item* groundItem = Item::Create(itemId);
        if (groundItem) {
            tile->ground = groundItem;
        }
    }
    
    void addDecoration(Tile* tile, uint16_t itemId) {
        if (!tile) return;
        
        Item* decoration = Item::Create(itemId);
        if (decoration) {
            tile->addItem(decoration);
        }
    }
    
    bool isWaterTile(uint16_t itemId) {
        // This could be made configurable by checking against water item IDs
        return itemId == 4608 || itemId == 4609 || itemId == 4610 || itemId == 4611;
    }
    
    bool isLandTile(uint16_t itemId) {
        // This could be made configurable by checking against land item IDs  
        return itemId == 4526 || itemId == 231 || itemId == 1284 || itemId == 4597;
    }
    
    bool isMountainTile(uint16_t itemId) {
        // This could be made configurable by checking against mountain item IDs
        return itemId == 919 || itemId == 4611 || itemId == 4621 || itemId == 4616;
    }
    
    std::vector<std::string> getAvailableBrushes() {
        // This should parse the actual grounds.xml files
        // For now, return a basic list
        return {
            "grass", "sea", "sand", "mountain", "cave", "snow", 
            "stone floor", "wooden floor", "lawn", "ice"
        };
    }
    
    uint16_t getPrimaryItemFromBrush(const std::string& brushName) {
        // This should parse the grounds.xml to get the primary item ID
        // For now, use basic mappings
        if (brushName == "grass") return 4526;
        else if (brushName == "sea") return 4608;
        else if (brushName == "sand") return 231;
        else if (brushName == "mountain") return 919;
        else if (brushName == "cave") return 351;
        else if (brushName == "snow") return 670;
        else if (brushName == "stone floor") return 431;
        else if (brushName == "wooden floor") return 405;
        else if (brushName == "lawn") return 106;
        else if (brushName == "ice") return 671;
        return 100; // Default
    }
    
    bool applyBrushToTile(BaseMap* map, Tile* tile, const std::string& brushName, int x, int y, int z) {
        // This should integrate with the actual brush system
        // For now, just set the ground tile
        uint16_t itemId = getPrimaryItemFromBrush(brushName);
        setGroundTile(tile, itemId);
        return true;
    }
}

// Island generation implementation
bool OTMapGenerator::generateIslandMap(BaseMap* map, const IslandConfig& config, int width, int height, const std::string& seed) {
    if (!map) {
        return false;
    }
    
    // Cast BaseMap to Map to access the editor's action system
    Map* editorMap = static_cast<Map*>(map);
    
    // Initialize random seed
    seedRandom(seed);
    
    // Generate island layer for floor 7 only
    auto islandLayer = generateIslandLayer(config, width, height, seed);
    
    // Apply terrain to map on floor 7 only
    std::vector<Position> tilesToGenerate;
    
    // Collect all positions that need tiles (floor 7 only)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint16_t tileId = islandLayer[y][x];  // Now accessing 2D vector correctly
            if (tileId != 0) {
                tilesToGenerate.push_back(Position(x, y, config.target_floor));
            }
        }
    }
    
    // Process tiles in batches to avoid memory issues
    const int BATCH_SIZE = 1000;
    
    for (size_t start = 0; start < tilesToGenerate.size(); start += BATCH_SIZE) {
        size_t end = std::min(start + BATCH_SIZE, tilesToGenerate.size());
        
        // Create a batch of tiles
        for (size_t i = start; i < end; ++i) {
            Position pos = tilesToGenerate[i];
            uint16_t tileId = islandLayer[pos.y][pos.x];
            
            // Create tile location and allocate tile properly
            TileLocation* location = editorMap->createTileL(pos);
            Tile* existing_tile = location->get();
            Tile* new_tile = nullptr;
            
            if (existing_tile) {
                // Copy existing tile and modify it
                new_tile = existing_tile->deepCopy(*editorMap);
            } else {
                // Create new tile
                new_tile = editorMap->allocator(location);
            }
            
            // Set the ground using proper API
            if (new_tile) {
                // Remove existing ground if any
                if (new_tile->ground) {
                    delete new_tile->ground;
                    new_tile->ground = nullptr;
                }
                
                // Create new ground item
                Item* groundItem = Item::Create(tileId);
                if (groundItem) {
                    new_tile->ground = groundItem;
                }
                
                // Set the tile back to the map
                editorMap->setTile(pos, new_tile);
            }
        }
    }
    
    return true;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateIslandLayer(const IslandConfig& config, int width, int height, const std::string& seed) {
    // Initialize random seed
    seedRandom(seed);
    
    // Generate height map for island
    auto heightMap = generateIslandHeightMap(config, width, height, seed);
    
    // Convert height map to tile IDs - return as 2D vector
    std::vector<std::vector<uint16_t>> terrainLayer(height, std::vector<uint16_t>(width, 0));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double height_value = heightMap[y][x];
            
            // Simple island logic: above threshold = ground, below = water
            if (height_value > config.island_threshold) {
                terrainLayer[y][x] = config.ground_id;  // Grass
            } else {
                terrainLayer[y][x] = config.water_id;   // Water
            }
        }
    }
    
    // Apply post-processing cleanup to improve border generation
    if (config.enable_cleanup) {
        cleanupIslandTerrain(terrainLayer, config, width, height);
    }
    
    return terrainLayer;
}

std::vector<std::vector<double>> OTMapGenerator::generateIslandHeightMap(const IslandConfig& config, int width, int height, const std::string& seed) {
    std::vector<std::vector<double>> heightMap(height, std::vector<double>(width, 0.0));
    
    double centerX = width / 2.0;
    double centerY = height / 2.0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Generate multi-octave noise for island shape
            double noiseValue = generateIslandNoise(x, y, config);
            
            // Calculate distance from center for island shape
            double distance = getIslandDistance(x, y, (int)centerX, (int)centerY, config.island_size);
            
            // Apply island falloff
            double falloff = applyIslandFalloff(distance, config.island_falloff);
            
            // Combine noise with island falloff
            double height = noiseValue * falloff;
            
            // Apply water level adjustment
            height = (height + config.water_level) / (1.0 + config.water_level);
            
            // Clamp to [0, 1]
            height = std::max(0.0, std::min(1.0, height));
            
            heightMap[y][x] = height;
        }
    }
    
    return heightMap;
}

double OTMapGenerator::getIslandDistance(int x, int y, int centerX, int centerY, double island_size) {
    double dx = (x - centerX) / static_cast<double>(centerX);
    double dy = (y - centerY) / static_cast<double>(centerY);
    double distance = sqrt(dx * dx + dy * dy) / island_size;
    return distance;
}

double OTMapGenerator::applyIslandFalloff(double distance, double falloff) {
    if (distance >= 1.0) {
        return 0.0;
    }
    return pow(1.0 - distance, falloff);
}

double OTMapGenerator::generateIslandNoise(double x, double y, const IslandConfig& config) {
    double value = 0.0;
    double amplitude = 1.0;
    double frequency = config.noise_scale;
    double maxValue = 0.0;
    
    // Generate multiple octaves of noise
    for (int i = 0; i < static_cast<int>(config.noise_octaves); ++i) {
        value += noise_generator->noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= config.noise_persistence;
        frequency *= config.noise_lacunarity;
    }
    
    // Normalize to [-1, 1] then shift to [0, 1]
    return (value / maxValue + 1.0) * 0.5;
}

void OTMapGenerator::cleanupIslandTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, const IslandConfig& config, int width, int height) {
    // Step 1: Remove isolated single pixels of land
    removeIsolatedPixels(terrainLayer, width, height, config, config.ground_id);
    
    // Step 2: Remove small land patches below minimum size
    removeSmallPatches(terrainLayer, width, height, config, config.ground_id, config.min_land_patch_size);
    
    // Step 3: Fill small water holes within land areas
    fillSmallHoles(terrainLayer, width, height, config.water_id, config.ground_id, config.max_water_hole_size);
    
    // Step 4: Apply smoothing passes to reduce noise
    for (int pass = 0; pass < config.smoothing_passes; ++pass) {
        smoothTerrain(terrainLayer, width, height, config);
    }
    
    // Step 5: Final cleanup - remove any remaining isolated pixels
    removeIsolatedPixels(terrainLayer, width, height, config, config.ground_id);
}

void OTMapGenerator::removeIsolatedPixels(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id) {
    // Remove pixels that have no neighbors of the same type
    std::vector<std::vector<uint16_t>> temp = terrainLayer;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (terrainLayer[y][x] == target_id) {
                // Count neighbors of the same type
                int neighbors = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue; // Skip center
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            if (terrainLayer[ny][nx] == target_id) {
                                neighbors++;
                            }
                        }
                    }
                }
                
                // If completely isolated, convert to water
                if (neighbors == 0) {
                    temp[y][x] = target_id == config.ground_id ? config.water_id : config.ground_id;
                }
            }
        }
    }
    
    terrainLayer = temp;
}

void OTMapGenerator::removeSmallPatches(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id, int min_size) {
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!visited[y][x] && terrainLayer[y][x] == target_id) {
                // Found an unvisited patch, measure its size
                uint16_t temp_id = 9999; // Temporary ID for flood fill
                int patch_size = floodFillCount(terrainLayer, x, y, width, height, target_id, temp_id);
                
                if (patch_size < min_size) {
                    // Patch is too small, convert it to water
                    for (int py = 0; py < height; ++py) {
                        for (int px = 0; px < width; ++px) {
                            if (terrainLayer[py][px] == temp_id) {
                                terrainLayer[py][px] = target_id == config.ground_id ? config.water_id : config.ground_id;
                                visited[py][px] = true;
                            }
                        }
                    }
                } else {
                    // Patch is large enough, restore original ID
                    for (int py = 0; py < height; ++py) {
                        for (int px = 0; px < width; ++px) {
                            if (terrainLayer[py][px] == temp_id) {
                                terrainLayer[py][px] = target_id;
                                visited[py][px] = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

void OTMapGenerator::fillSmallHoles(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, uint16_t target_id, uint16_t fill_id, int max_hole_size) {
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!visited[y][x] && terrainLayer[y][x] == target_id) {
                // Found an unvisited hole, measure its size
                uint16_t temp_id = 9998; // Temporary ID for flood fill
                int hole_size = floodFillCount(terrainLayer, x, y, width, height, target_id, temp_id);
                
                if (hole_size <= max_hole_size) {
                    // Hole is small enough to fill
                    for (int py = 0; py < height; ++py) {
                        for (int px = 0; px < width; ++px) {
                            if (terrainLayer[py][px] == temp_id) {
                                terrainLayer[py][px] = fill_id;
                                visited[py][px] = true;
                            }
                        }
                    }
                } else {
                    // Hole is too large, restore original ID
                    for (int py = 0; py < height; ++py) {
                        for (int px = 0; px < width; ++px) {
                            if (terrainLayer[py][px] == temp_id) {
                                terrainLayer[py][px] = target_id;
                                visited[py][px] = true;
                            }
                        }
                    }
                }
            }
        }
    }
}

void OTMapGenerator::smoothTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config) {
    std::vector<std::vector<uint16_t>> temp = terrainLayer;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // Count neighbors
            int land_neighbors = 0;
            int total_neighbors = 0;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        total_neighbors++;
                        if (terrainLayer[ny][nx] == config.ground_id) {
                            land_neighbors++;
                        }
                    }
                }
            }
            
            // Apply smoothing rule: if majority of neighbors are land, make this land
            // This helps create more coherent landmasses
            if (land_neighbors > total_neighbors / 2) {
                temp[y][x] = config.ground_id;
            } else {
                temp[y][x] = config.water_id;
            }
        }
    }
    
    terrainLayer = temp;
}

int OTMapGenerator::floodFillCount(std::vector<std::vector<uint16_t>>& terrainLayer, int x, int y, int width, int height, uint16_t target_id, uint16_t replacement_id) {
    if (x < 0 || x >= width || y < 0 || y >= height) return 0;
    if (terrainLayer[y][x] != target_id) return 0;
    
    // Use iterative flood fill to avoid stack overflow
    std::vector<std::pair<int, int>> stack;
    stack.push_back({x, y});
    int count = 0;
    
    while (!stack.empty()) {
        auto [cx, cy] = stack.back();
        stack.pop_back();
        
        if (cx < 0 || cx >= width || cy < 0 || cy >= height) continue;
        if (terrainLayer[cy][cx] != target_id) continue;
        
        terrainLayer[cy][cx] = replacement_id;
        count++;
        
        // Add neighbors to stack
        stack.push_back({cx + 1, cy});
        stack.push_back({cx - 1, cy});
        stack.push_back({cx, cy + 1});
        stack.push_back({cx, cy - 1});
    }
    
    return count;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateIslandLayerBatch(const IslandConfig& config, int width, int height, const std::string& seed, int offsetX, int offsetY, int totalWidth, int totalHeight) {
	// Seed the random generator with the provided seed
	seedRandom(seed);
	
	// Create result layer
	std::vector<std::vector<uint16_t>> islandLayer(height, std::vector<uint16_t>(width, config.water_id));
	
	// Calculate center point for the entire island (not this batch)
	int totalCenterX = totalWidth / 2;
	int totalCenterY = totalHeight / 2;
	
	// Generate height map for this batch with global coordinates
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			// Calculate global coordinates for noise sampling
			int globalX = offsetX + x;
			int globalY = offsetY + y;
			
			// Generate noise using global coordinates for consistency
			double noiseValue = generateIslandNoise(globalX, globalY, config);
			
			// Calculate distance from center using global coordinates
			double distance = getIslandDistance(globalX, globalY, totalCenterX, totalCenterY, config.island_size);
			
			// Apply island shape using distance and noise
			double falloffValue = applyIslandFalloff(distance, config.island_falloff);
			double heightValue = (noiseValue * 0.5 + 0.5) * falloffValue + config.water_level - 0.5;
			
			// Determine tile type based on height
			if (heightValue > config.island_threshold) {
				islandLayer[y][x] = config.ground_id;
			} else {
				islandLayer[y][x] = config.water_id;
			}
		}
	}
	
	// Apply cleanup if enabled (only on batch level for performance)
	if (config.enable_cleanup) {
		cleanupIslandTerrain(islandLayer, config, width, height);
	}
	
	return islandLayer;
}

// Dungeon Generation Functions
bool OTMapGenerator::generateDungeonMap(BaseMap* map, const DungeonConfig& config, int width, int height, const std::string& seed) {
	if (!map) {
		return false;
	}
	
	// Generate dungeon layer
	auto dungeonLayer = generateDungeonLayer(config, width, height, seed);
	
	if (dungeonLayer.empty()) {
		return false;
	}
	
	// Apply the dungeon layer to the map (floor 7 only)
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			uint16_t tileId = dungeonLayer[y][x];
			if (tileId != 0) {
				Position pos(x, y, 7); // Dungeons are always on floor 7
				
				// Get or create tile
				Tile* tile = OTMapGenUtils::getOrCreateTile(map, x, y, 7);
				if (tile) {
					// Set ground tile
					OTMapGenUtils::setGroundTile(tile, tileId);
				}
			}
		}
	}
	
	return true;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateDungeonLayer(const DungeonConfig& config, int width, int height, const std::string& seed) {
	// Seed the random generator
	seedRandom(seed);
	
	// Initialize dungeon grid - start with all walls (fill_id)
	std::vector<std::vector<uint16_t>> dungeonLayer(height, std::vector<uint16_t>(width, config.fill_id));
	
	// Create a working grid for algorithm (0 = wall, 1 = corridor/room)
	std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0));
	
	// Generate rooms first
	std::vector<Room> rooms = generateRooms(config, width, height);
	
	// Place rooms in the grid
	for (const auto& room : rooms) {
		placeRoom(grid, room, config);
	}
	
	// Generate intersection hubs if enabled
	std::vector<Intersection> intersections;
	if (config.add_triple_intersections || config.add_quad_intersections) {
		intersections = generateIntersections(config, rooms, width, height);
		
		// Place intersections in the grid
		for (const auto& intersection : intersections) {
			placeIntersection(grid, intersection, config);
		}
		
		// Connect rooms via intersections
		connectRoomsViaIntersections(grid, rooms, intersections, config, width, height);
	}
	
	// Generate traditional corridors to connect rooms (this will complement the intersections)
	generateCorridors(grid, rooms, config, width, height);
	
	// Add dead ends if requested
	if (config.add_dead_ends) {
		addDeadEnds(grid, config, width, height);
	}
	
	// Convert working grid to tile IDs
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (grid[y][x] == 1) {
				// Corridor or room floor
				dungeonLayer[y][x] = config.ground_id;
			} else if (isWallPosition(grid, x, y, width, height)) {
				// Wall position (adjacent to corridor/room)
				dungeonLayer[y][x] = config.wall_id;
			} else {
				// Solid fill
				dungeonLayer[y][x] = config.fill_id;
			}
		}
	}
	
	return dungeonLayer;
}

std::vector<OTMapGenerator::Room> OTMapGenerator::generateRooms(const DungeonConfig& config, int width, int height) {
	std::vector<Room> rooms;
	
	// Calculate minimum and actual room counts
	int min_rooms = config.room_count;
	int max_rooms = std::min(min_rooms * 2, (width * height) / 200); // Prevent overcrowding
	
	// Calculate area-based room count for better distribution
	int area = width * height;
	int recommended_rooms = std::max(min_rooms, area / 1000); // 1 room per 1000 sqm roughly
	int actual_room_count = std::min(recommended_rooms, max_rooms);
	
	std::uniform_int_distribution<> x_dist(config.room_max_size + 2, width - config.room_max_size - 2);
	std::uniform_int_distribution<> y_dist(config.room_max_size + 2, height - config.room_max_size - 2);
		std::uniform_int_distribution<> size_dist(config.room_min_size, config.room_max_size);
	std::uniform_real_distribution<> shape_dist(0.0, 1.0);
	
	int attempts = 0;
	const int max_attempts = actual_room_count * 20; // Allow more attempts for better placement
	
	while (rooms.size() < static_cast<size_t>(actual_room_count) && attempts < max_attempts) {
		attempts++;
		
		Room newRoom;
		newRoom.centerX = x_dist(rng);
		newRoom.centerY = y_dist(rng);
		newRoom.radius = size_dist(rng);
		newRoom.isCircular = config.circular_rooms && (shape_dist(rng) < 0.5);
		
		// Check for overlap with existing rooms (with minimum separation)
		bool overlaps = false;
		int min_separation = config.room_max_size + 3; // Ensure rooms have space between them
		
		for (const Room& existing : rooms) {
			int distance = abs(newRoom.centerX - existing.centerX) + abs(newRoom.centerY - existing.centerY);
			int min_distance = newRoom.radius + existing.radius + min_separation;
			
			if (distance < min_distance) {
				overlaps = true;
				break;
			}
		}
		
		if (!overlaps) {
			rooms.push_back(newRoom);
		}
	}
	
	// Ensure we have at least the minimum number of rooms
	if (rooms.size() < static_cast<size_t>(min_rooms)) {
		// If we couldn't place enough rooms, reduce size requirements and try again
		while (rooms.size() < static_cast<size_t>(min_rooms) && attempts < max_attempts * 2) {
			attempts++;
			
			Room newRoom;
			newRoom.centerX = x_dist(rng);
			newRoom.centerY = y_dist(rng);
			newRoom.radius = std::max(2, config.room_min_size - 1); // Smaller rooms
			newRoom.isCircular = config.circular_rooms && (shape_dist(rng) < 0.5);
			
			// Check for overlap with reduced separation
		bool overlaps = false;
			int min_separation = 2; // Reduced separation for tight spaces
			
			for (const Room& existing : rooms) {
				int distance = abs(newRoom.centerX - existing.centerX) + abs(newRoom.centerY - existing.centerY);
				int min_distance = newRoom.radius + existing.radius + min_separation;
				
				if (distance < min_distance) {
				overlaps = true;
				break;
			}
		}
		
		if (!overlaps) {
				rooms.push_back(newRoom);
			}
		}
	}
	
	return rooms;
}

void OTMapGenerator::placeRoom(std::vector<std::vector<int>>& grid, const Room& room, const DungeonConfig& config) {
	int width = grid[0].size();
	int height = grid.size();
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int dx = x - room.centerX;
			int dy = y - room.centerY;
			
			bool inRoom = false;
			if (room.isCircular) {
				// Circular room
				double distance = std::sqrt(dx * dx + dy * dy);
				inRoom = distance <= room.radius;
			} else {
				// Rectangular room
				inRoom = (std::abs(dx) <= room.radius && std::abs(dy) <= room.radius);
			}
			
			if (inRoom) {
				grid[y][x] = 1; // Mark as room floor
			}
		}
	}
}

void OTMapGenerator::generateCorridors(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const DungeonConfig& config, int width, int height) {
	if (rooms.size() < 2) {
		return; // Need at least 2 rooms to connect
	}
	
	// Calculate minimum corridors needed for connectivity
	int min_corridors_for_connectivity = static_cast<int>(rooms.size()) - 1;
	int min_corridors_user = config.corridor_count;
	int actual_min_corridors = std::max(min_corridors_for_connectivity, min_corridors_user);
	
	// Calculate maximum reasonable corridors (prevent over-connection)
	int max_corridors = std::min(actual_min_corridors * 3, static_cast<int>(rooms.size() * (rooms.size() - 1) / 2));
	
	// Use Minimum Spanning Tree (MST) algorithm to ensure all rooms are connected
	// This guarantees connectivity while minimizing total corridor length
	
	std::vector<bool> connected(rooms.size(), false);
	std::vector<std::tuple<double, int, int>> edges; // distance, room1, room2
	
	// Calculate all possible room-to-room distances
	for (size_t i = 0; i < rooms.size(); ++i) {
		for (size_t j = i + 1; j < rooms.size(); ++j) {
			int dx = rooms[i].centerX - rooms[j].centerX;
			int dy = rooms[i].centerY - rooms[j].centerY;
			double distance = std::sqrt(dx * dx + dy * dy);
			edges.push_back(std::make_tuple(distance, static_cast<int>(i), static_cast<int>(j)));
		}
	}
	
	// Sort edges by distance (shortest first)
	std::sort(edges.begin(), edges.end());
	
	// Build MST using Kruskal's algorithm
	std::vector<int> parent(rooms.size());
	// Initialize union-find manually (replaced std::iota for compatibility)
	for (size_t i = 0; i < parent.size(); ++i) {
		parent[i] = i;
	}
	
	std::function<int(int)> find = [&](int x) {
		if (parent[x] != x) {
			parent[x] = find(parent[x]);
		}
		return parent[x];
	};
	
	auto unite = [&](int x, int y) {
		int px = find(x);
		int py = find(y);
		if (px != py) {
			parent[px] = py;
			return true;
		}
		return false;
	};
	
	// Phase 1: Connect all rooms using MST (guarantees connectivity)
	std::vector<std::pair<int, int>> essential_connections;
	int connections_made = 0;
	
	for (const auto& edge : edges) {
		int room1 = std::get<1>(edge);
		int room2 = std::get<2>(edge);
		
		if (unite(room1, room2)) {
			essential_connections.push_back({room1, room2});
			connections_made++;
			
			// If we've connected all rooms, we have a spanning tree
			if (connections_made == static_cast<int>(rooms.size()) - 1) {
                        break;
                    }
                }
	}
	
	// Phase 2: Add additional corridors for redundancy and meeting minimum requirements
	std::vector<std::pair<int, int>> additional_connections;
	int additional_needed = actual_min_corridors - connections_made;
	
	if (additional_needed > 0) {
		// Add the shortest remaining edges
		for (const auto& edge : edges) {
			if (additional_connections.size() >= static_cast<size_t>(additional_needed)) {
				break;
			}
			
			int room1 = std::get<1>(edge);
			int room2 = std::get<2>(edge);
			
			// Check if this connection already exists
			bool already_connected = false;
			for (const auto& existing : essential_connections) {
				if ((existing.first == room1 && existing.second == room2) ||
					(existing.first == room2 && existing.second == room1)) {
					already_connected = true;
					break;
				}
			}
			
			if (!already_connected) {
				additional_connections.push_back({room1, room2});
			}
		}
	}
	
	// Phase 3: Create all the corridor connections
	auto all_connections = essential_connections;
	all_connections.insert(all_connections.end(), additional_connections.begin(), additional_connections.end());
	
	for (const auto& connection : all_connections) {
		int room1 = connection.first;
		int room2 = connection.second;
		
		int x1 = rooms[room1].centerX;
		int y1 = rooms[room1].centerY;
		int x2 = rooms[room2].centerX;
		int y2 = rooms[room2].centerY;
		
		// Use smart corridor creation if enabled
		if (config.use_smart_pathfinding) {
			createSmartCorridor(grid, x1, y1, x2, y2, config, width, height);
		} else {
			createImprovedCorridor(grid, x1, y1, x2, y2, config, width, height);
		}
	}
	
	// Phase 4: Add intersections if requested
	if ((config.add_triple_intersections || config.add_quad_intersections) && config.intersection_count > 0) {
		auto intersections = generateIntersections(config, rooms, width, height);
		
		// Place intersection areas
		for (const auto& intersection : intersections) {
			placeIntersection(grid, intersection, config);
		}
		
		// Connect some rooms via intersections for variety
		if (config.prefer_intersections && !intersections.empty()) {
			connectRoomsViaIntersections(grid, rooms, intersections, config, width, height);
		}
	}
}

void OTMapGenerator::createCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height) {
	// This is the old simple corridor - replace with improved version
	createImprovedCorridor(grid, x1, y1, x2, y2, config, width, height);
}

void OTMapGenerator::createImprovedCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height) {
	// Create L-shaped corridor (horizontal then vertical) for better layout
	int currentX = x1;
	int currentY = y1;
	
	// Horizontal segment first
	while (currentX != x2) {
		createCorridorTile(grid, currentX, currentY, config, width, height);
		currentX += (x2 > x1) ? 1 : -1;
	}
	
	// Vertical segment
	while (currentY != y2) {
		createCorridorTile(grid, currentX, currentY, config, width, height);
		currentY += (y2 > y1) ? 1 : -1;
	}
	
	// Final destination tile
	createCorridorTile(grid, x2, y2, config, width, height);
}

void OTMapGenerator::createCorridorTile(std::vector<std::vector<int>>& grid, int centerX, int centerY, const DungeonConfig& config, int width, int height) {
	// Create corridor with specified width centered on the position
	int half_width = config.corridor_width / 2;
	
	for (int dy = -half_width; dy <= half_width; ++dy) {
		for (int dx = -half_width; dx <= half_width; ++dx) {
			int x = centerX + dx;
			int y = centerY + dy;
			
			if (x >= 1 && x < width - 1 && y >= 1 && y < height - 1) {
				grid[y][x] = 1; // Mark as corridor
			}
		}
	}
}

void OTMapGenerator::addDeadEnds(std::vector<std::vector<int>>& grid, const DungeonConfig& config, int width, int height) {
	// Add some random dead-end corridors for complexity
	int deadEndCount = config.corridor_count / 3; // About 1/3 of corridor count
	
	for (int i = 0; i < deadEndCount; ++i) {
		// Find a random corridor tile to start from
		std::vector<std::pair<int, int>> corridorTiles;
		for (int y = 1; y < height - 1; ++y) {
			for (int x = 1; x < width - 1; ++x) {
				if (grid[y][x] == 1) {
					corridorTiles.push_back({x, y});
				}
			}
		}
		
		if (corridorTiles.empty()) {
			continue;
		}
		
		// Pick random starting point
		std::uniform_int_distribution<> tile_dist(0, corridorTiles.size() - 1);
		auto [startX, startY] = corridorTiles[tile_dist(rng)];
		
		// Create dead end in random direction
		std::uniform_int_distribution<> dir_dist(0, 3);
		int direction = dir_dist(rng);
		
		int dx = 0, dy = 0;
		switch (direction) {
			case 0: dx = 1; break;  // Right
			case 1: dx = -1; break; // Left
			case 2: dy = 1; break;  // Down
			case 3: dy = -1; break; // Up
		}
		
		// Create dead end corridor
		std::uniform_int_distribution<> length_dist(3, 8);
		int length = length_dist(rng);
		
		int currentX = startX;
		int currentY = startY;
		
		for (int step = 0; step < length; ++step) {
			currentX += dx;
			currentY += dy;
			
			if (currentX <= 0 || currentX >= width - 1 || currentY <= 0 || currentY >= height - 1) {
				break; // Hit boundary
			}
			
			if (grid[currentY][currentX] == 1) {
				break; // Hit existing corridor
			}
			
			grid[currentY][currentX] = 1; // Mark as corridor
		}
	}
}

bool OTMapGenerator::isWallPosition(const std::vector<std::vector<int>>& grid, int x, int y, int width, int height) {
	// A position is a wall if it's not a corridor/room but is adjacent to one
	if (grid[y][x] == 1) {
		return false; // This is a corridor/room, not a wall
	}
	
	// Check if adjacent to any corridor/room
	for (int dy = -1; dy <= 1; ++dy) {
		for (int dx = -1; dx <= 1; ++dx) {
			if (dx == 0 && dy == 0) continue;
			
			int nx = x + dx;
			int ny = y + dy;
			
			if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
				if (grid[ny][nx] == 1) {
					return true; // Adjacent to corridor/room
				}
			}
		}
	}
	
	return false; // Not adjacent to any corridor/room
}

std::vector<OTMapGenerator::Intersection> OTMapGenerator::generateIntersections(const DungeonConfig& config, const std::vector<Room>& rooms, int width, int height) {
    std::vector<Intersection> intersections;
    
    if (!config.add_triple_intersections && !config.add_quad_intersections) {
        return intersections; // No intersections requested
    }
    
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<int> x_dist(config.intersection_size + 2, width - config.intersection_size - 3);
    std::uniform_int_distribution<int> y_dist(config.intersection_size + 2, height - config.intersection_size - 3);
    std::uniform_int_distribution<int> connection_dist(3, 4);
    
    int attempts = 0;
    const int max_attempts = config.intersection_count * 10; // Prevent infinite loops
    
    while (intersections.size() < static_cast<size_t>(config.intersection_count) && attempts < max_attempts) {
        attempts++;
        
        // Only proceed if probability check passes
        if (prob_dist(rng) > config.intersection_probability) {
            continue;
        }
        
        Intersection intersection;
        intersection.centerX = x_dist(rng);
        intersection.centerY = y_dist(rng);
        intersection.size = config.intersection_size;
        
        // Determine number of connections based on config
        if (config.add_triple_intersections && config.add_quad_intersections) {
            intersection.connectionCount = connection_dist(rng);
        } else if (config.add_quad_intersections) {
            intersection.connectionCount = 4;
        } else {
            intersection.connectionCount = 3;
        }
        
        // Check for minimum distance from rooms and other intersections
        bool valid = true;
        int min_distance = config.intersection_size + 5; // Minimum spacing
        
        // Check distance from rooms
        for (const auto& room : rooms) {
            int dx = intersection.centerX - room.centerX;
            int dy = intersection.centerY - room.centerY;
            double distance = sqrt(dx * dx + dy * dy);
            if (distance < min_distance + room.radius) {
                valid = false;
                break;
            }
        }
        
        // Check distance from other intersections
        if (valid) {
            for (const auto& other : intersections) {
                int dx = intersection.centerX - other.centerX;
                int dy = intersection.centerY - other.centerY;
                double distance = sqrt(dx * dx + dy * dy);
                if (distance < min_distance * 2) {
                    valid = false;
                    break;
                }
            }
        }
        
        if (valid) {
            intersections.push_back(intersection);
        }
    }
    
    return intersections;
}

void OTMapGenerator::placeIntersection(std::vector<std::vector<int>>& grid, const Intersection& intersection, const DungeonConfig& config) {
    // Create a small open area at the intersection center
    int size = intersection.size;
    
    for (int dy = -size; dy <= size; ++dy) {
        for (int dx = -size; dx <= size; ++dx) {
            int x = intersection.centerX + dx;
            int y = intersection.centerY + dy;
            
            if (x >= 0 && x < static_cast<int>(grid[0].size()) && 
                y >= 0 && y < static_cast<int>(grid.size())) {
                
                // Create circular intersection area
                if (dx * dx + dy * dy <= size * size) {
                    grid[y][x] = 1; // Mark as corridor/open space
                }
            }
        }
    }
}

void OTMapGenerator::connectRoomsViaIntersections(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const std::vector<Intersection>& intersections, const DungeonConfig& config, int width, int height) {
    // First, connect some rooms to intersections
    for (const auto& intersection : intersections) {
        std::vector<size_t> nearbyRooms;
        
        // Find nearby rooms
        for (size_t i = 0; i < rooms.size(); ++i) {
            int dx = intersection.centerX - rooms[i].centerX;
            int dy = intersection.centerY - rooms[i].centerY;
            double distance = sqrt(dx * dx + dy * dy);
            
            if (distance < width * 0.4) { // Within reasonable connection distance
                nearbyRooms.push_back(i);
            }
        }
        
        // Shuffle and connect up to connectionCount rooms
        std::shuffle(nearbyRooms.begin(), nearbyRooms.end(), rng);
        
        int connectionsToMake = std::min(intersection.connectionCount, static_cast<int>(nearbyRooms.size()));
        for (int i = 0; i < connectionsToMake; ++i) {
            const Room& room = rooms[nearbyRooms[i]];
            createSmartCorridor(grid, room.centerX, room.centerY, intersection.centerX, intersection.centerY, config, width, height);
        }
        
        // If we still need more connections, try connecting to other intersections
        if (connectionsToMake < intersection.connectionCount) {
            for (const auto& otherIntersection : intersections) {
                if (&otherIntersection == &intersection) continue;
                
                int dx = intersection.centerX - otherIntersection.centerX;
                int dy = intersection.centerY - otherIntersection.centerY;
                double distance = sqrt(dx * dx + dy * dy);
                
                if (distance < width * 0.6 && connectionsToMake < intersection.connectionCount) {
                    createSmartCorridor(grid, intersection.centerX, intersection.centerY, 
                                     otherIntersection.centerX, otherIntersection.centerY, config, width, height);
                    connectionsToMake++;
                }
            }
        }
    }
}

bool OTMapGenerator::findIntersectionPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height) {
    // Simple pathfinding to check if two points can be connected
    // This is a basic implementation - could be enhanced with A* if needed
    
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::queue<std::pair<int, int>> queue;
    
    queue.push({x1, y1});
    visited[y1][x1] = true;
    
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};
    
    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();
        
        if (x == x2 && y == y2) {
            return true; // Path found
        }
        
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && 
                !visited[ny][nx] && grid[ny][nx] == 1) {
                visited[ny][nx] = true;
                queue.push({nx, ny});
            }
        }
    }
    
    return false; // No path found
}

void OTMapGenerator::createSmartCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height) {
	// Use smart pathfinding to create shorter, more logical corridors
	if (config.use_smart_pathfinding) {
		// Try to find shortest path with length limit
		auto path = findShortestPath(grid, x1, y1, x2, y2, width, height, config.max_corridor_length);
		
		if (!path.empty()) {
			// Create corridor using the found path
			createCorridorSegments(grid, path, config, width, height);
			return;
		}
	}
	
	// Fallback to improved corridor if smart pathfinding fails
	createImprovedCorridor(grid, x1, y1, x2, y2, config, width, height);
}

std::vector<std::pair<int, int>> OTMapGenerator::findShortestPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height, int maxLength) {
	// Simple A* pathfinding implementation
	struct Node {
		int x, y;
		int gCost, hCost;
		std::pair<int, int> parent;
		
		int fCost() const { return gCost + hCost; }
	};
	
	// Priority queue for open set (min heap by fCost)
	auto cmp = [](const Node& a, const Node& b) { return a.fCost() > b.fCost(); };
	std::priority_queue<Node, std::vector<Node>, decltype(cmp)> openSet(cmp);
	
	std::vector<std::vector<bool>> closedSet(height, std::vector<bool>(width, false));
	std::vector<std::vector<Node>> nodeGrid(height, std::vector<Node>(width));
	
	// Initialize start node
	Node startNode = {x1, y1, 0, abs(x2 - x1) + abs(y2 - y1), {-1, -1}};
	openSet.push(startNode);
	nodeGrid[y1][x1] = startNode;
	
	std::vector<std::pair<int, int>> directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
	
	while (!openSet.empty()) {
		Node current = openSet.top();
		openSet.pop();
		
		// Check if we reached the target
		if (current.x == x2 && current.y == y2) {
			// Reconstruct path
			std::vector<std::pair<int, int>> path;
			Node* node = &nodeGrid[current.y][current.x];
			
			while (node->parent.first != -1) {
				path.push_back({node->x, node->y});
				node = &nodeGrid[node->parent.second][node->parent.first];
			}
			path.push_back({x1, y1});
			
			std::reverse(path.begin(), path.end());
			
			// Check if path is within length limit
			if (static_cast<int>(path.size()) <= maxLength) {
				return path;
			} else {
				return {}; // Path too long
			}
		}
		
		closedSet[current.y][current.x] = true;
		
		// Check neighbors
		for (const auto& dir : directions) {
			int newX = current.x + dir.first;
			int newY = current.y + dir.second;
			
			// Check bounds
			if (newX < 0 || newX >= width || newY < 0 || newY >= height) {
				continue;
			}
			
			// Skip if already in closed set
			if (closedSet[newY][newX]) {
				continue;
			}
			
			// Skip if wall (but allow if it's the target)
			if (grid[newY][newX] == 1 && !(newX == x2 && newY == y2)) {
				continue;
			}
			
			int newGCost = current.gCost + 1;
			int newHCost = abs(x2 - newX) + abs(y2 - newY);
			
			Node newNode = {newX, newY, newGCost, newHCost, {current.x, current.y}};
			
			// Check if this path to newNode is better
			if (nodeGrid[newY][newX].fCost() == 0 || newGCost < nodeGrid[newY][newX].gCost) {
				nodeGrid[newY][newX] = newNode;
				openSet.push(newNode);
			}
		}
	}
	
	return {}; // No path found
}

void OTMapGenerator::createCorridorSegments(std::vector<std::vector<int>>& grid, const std::vector<std::pair<int, int>>& path, const DungeonConfig& config, int width, int height) {
	// Create corridor along the path with specified width
	for (const auto& point : path) {
		createCorridorTile(grid, point.first, point.second, config, width, height);
	}
}

std::pair<int, int> OTMapGenerator::findNearestIntersection(const std::vector<Intersection>& intersections, int x, int y) {
	if (intersections.empty()) {
		return {-1, -1}; // No intersections available
	}
	
	int nearestX = intersections[0].centerX;
	int nearestY = intersections[0].centerY;
	int minDistance = abs(intersections[0].centerX - x) + abs(intersections[0].centerY - y);
	
	for (const auto& intersection : intersections) {
		int distance = abs(intersection.centerX - x) + abs(intersection.centerY - y);
		if (distance < minDistance) {
			minDistance = distance;
			nearestX = intersection.centerX;
			nearestY = intersection.centerY;
		}
	}
	
	return {nearestX, nearestY};
}
