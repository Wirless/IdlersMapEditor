#ifndef RME_MONSTER_MANAGER_H_
#define RME_MONSTER_MANAGER_H_

#include "main.h"
#include <map>
#include <string>
#include <vector>

// Forward declarations for complex entry types
struct AttackEntry {
    std::string name;           // melee, fire, lifedrain, speed, etc.
    int minDamage;              // min damage (can be negative)
    int maxDamage;              // max damage (can be negative)
    int interval;               // attack interval in ms
    int chance;                 // chance percentage (0-100)
    
    // Advanced properties
    int range;                  // attack range in tiles (0 = melee)
    int radius;                 // area effect radius (0 = single target)
    int target;                 // 0 = area around target, 1 = target position
    int length;                 // beam length (for wave attacks)
    int spread;                 // beam spread (for wave attacks)
    
    // Status effect properties
    int speedChange;            // speed modification
    int duration;               // effect duration in ms
    
    // Visual effects
    std::string shootEffect;    // projectile effect
    std::string areaEffect;     // area effect
    
    // Condition properties
    std::string conditionType;  // poison, burn, freeze, etc.
    
    AttackEntry() : minDamage(0), maxDamage(0), interval(2000), chance(100),
                    range(0), radius(0), target(0), length(0), spread(0),
                    speedChange(0), duration(0) {}
};

struct DefenseEntry {
    std::string name;
    std::string type;           // armor, healing, shielding
    int interval;
    int chance;
    int minValue;
    int maxValue;
    
    DefenseEntry() : interval(2000), chance(100), minValue(1), maxValue(1) {}
};

struct ElementEntry {
    std::string element;        // physical, holy, death, fire, energy, earth, ice, drown
    int percentage;             // damage/healing percentage (-100 to 100)
    
    ElementEntry() : percentage(0) {}
};

struct ImmunityEntry {
    std::string type;           // physical, holy, death, fire, energy, earth, ice, drown, lifedrain, manadrain, healing, speed, outfit, drunk, invisible, paralyze
    
    ImmunityEntry() {}
};

struct SummonEntry {
    std::string name;           // creature name to summon
    int interval;
    int chance;
    int max;                    // maximum summons
    
    SummonEntry() : interval(2000), chance(100), max(1) {}
};

struct VoiceEntry {
    std::string text;           // what the monster says
    bool yell;                  // true for yell, false for say
    int interval;
    int chance;
    
    VoiceEntry() : yell(false), interval(5000), chance(100) {}
};

struct LootEntry {
    int itemId;
    std::string itemName;
    int chance;                 // 1-100000 (100000 = 100%)
    int countMax;              // maximum count for stackable items
    int subType;               // fluid type for containers
    int actionId;
    int uniqueId;
    bool useItemName;          // true to use name instead of id
    
    LootEntry() : itemId(0), chance(100000), countMax(1), subType(0), actionId(0), uniqueId(0), useItemName(false) {}
};

struct MonsterEntry {
    std::string name;
    std::string nameDescription;
    std::string filename;
    std::string full_path;
    bool loaded;
    
    // Basic monster properties
    std::string race;
    std::string skull;          // none, yellow, black, red, white, orange, green
    int experience;
    int speed;
    int manacost;
    int health_max;
    int health_now;
    
    // Look properties
    int look_type;
    int look_head;
    int look_body;
    int look_legs;
    int look_feet;
    int look_addons;
    int look_mount;
    int corpse;
    
    // Combat properties
    int interval;
    int chance;
    int strategy;               // 0-100 (0=attack, 100=defense)
    int targetDistance;
    int staticAttack;          // 0-100
    bool hideHealth;
    int lightColor;
    int lightLevel;
    int runOnHealth;           // 0-100 percentage
    
    // Behavior flags
    bool summonable;
    bool attackable;
    bool hostile;
    bool illusionable;
    bool convinceable;
    bool pushable;
    bool canPushItems;
    bool canPushCreatures;
    
    // Complex systems
    std::vector<AttackEntry> attacks;
    std::vector<DefenseEntry> defenses;
    std::vector<ElementEntry> elements;
    std::vector<ImmunityEntry> immunities;
    std::vector<SummonEntry> summons;
    std::vector<VoiceEntry> voices;
    std::vector<LootEntry> loot;
    
    // Script support
    bool useScript;
    std::string scriptFile;
    
    MonsterEntry() : loaded(false), experience(0), speed(0), manacost(0), 
                     health_max(0), health_now(0), look_type(0), look_head(0), look_body(0), 
                     look_legs(0), look_feet(0), look_addons(0), look_mount(0), corpse(0),
                     interval(2000), chance(5), strategy(50), targetDistance(0), staticAttack(90),
                     hideHealth(false), lightColor(0), lightLevel(0), runOnHealth(0),
                     summonable(false), attackable(true), hostile(true), illusionable(false),
                     convinceable(false), pushable(false), canPushItems(false), canPushCreatures(false),
                     useScript(false) {}
};

class MonsterManager {
public:
    MonsterManager();
    ~MonsterManager();
    
    // Scan the data directory for monsters.xml and load monster entries
    bool scanMonstersDirectory(const std::string& data_directory);
    
    // Find monster entry by name
    MonsterEntry* findByName(const std::string& name) const;
    
    // Get all loaded monsters
    const std::map<std::string, MonsterEntry>& getAllMonsters() const { return monster_entries; }
    
    // Load detailed monster data from individual XML file
    bool loadMonsterDetails(MonsterEntry& entry) const;
    
    // Create a new monster XML file
    bool createMonsterXML(const MonsterEntry& entry, const std::string& output_path) const;
    
    // Open monster XML file in external editor
    bool openMonsterXML(const MonsterEntry& entry) const;
    
    // Check if monsters are loaded
    bool isLoaded() const { return loaded; }
    
    // Get the base data directory
    const std::string& getDataDirectory() const { return data_directory; }
    
    // Clear all loaded monster data
    void clear();
    
    // Get statistics for debugging
    std::string getScanStatistics() const;

private:
    std::map<std::string, MonsterEntry> monster_entries;
    std::string data_directory;
    bool loaded;
    
    // Helper methods
    bool parseMonstersXML(const std::string& monsters_xml_path);
    bool parseMonsterXML(const std::string& monster_xml_path, MonsterEntry& entry) const;
    
    // Enhanced parsing methods
    void parseAttacks(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseDefenses(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseElements(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseImmunities(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseSummons(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseVoices(pugi::xml_node monster_node, MonsterEntry& entry) const;
    void parseLoot(pugi::xml_node monster_node, MonsterEntry& entry) const;
    
    // Enhanced XML generation methods
    void generateAttacksXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateDefensesXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateElementsXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateImmunitiesXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateSummonsXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateVoicesXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
    void generateLootXML(pugi::xml_node monster_node, const MonsterEntry& entry) const;
};

#endif // RME_MONSTER_MANAGER_H_ 