#ifndef RME_NPC_MANAGER_H_
#define RME_NPC_MANAGER_H_

#include "main.h"
#include <map>
#include <string>
#include <vector>

struct ParameterEntry {
    std::string key;
    std::string value;
    
    ParameterEntry() {}
    ParameterEntry(const std::string& k, const std::string& v) : key(k), value(v) {}
};

struct NPCEntry {
    std::string name;
    std::string filename;           // XML filename (e.g., "Xodet.xml")
    std::string full_path;          // Full path to XML file
    std::string script_path;        // Relative script path from XML (e.g., "runes.lua" or "somefolder/runes.lua")
    std::string script_full_path;   // Full path to Lua script file
    bool loaded;
    
    // Basic NPC properties from XML
    int walkinterval;
    int floorchange;
    
    // Health properties
    int health_now;
    int health_max;
    
    // Look properties
    int look_type;
    int look_head;
    int look_body;
    int look_legs;
    int look_feet;
    int look_addons;
    int look_mount;
    
    // Parameters
    std::vector<ParameterEntry> parameters;
    
    // Script support
    bool hasScript;
    
    NPCEntry() : loaded(false), walkinterval(2000), floorchange(0),
                 health_now(100), health_max(100), look_type(0), look_head(0), 
                 look_body(0), look_legs(0), look_feet(0), look_addons(0), 
                 look_mount(0), hasScript(false) {}
};

class NPCManager {
public:
    NPCManager();
    ~NPCManager();
    
    // Scan the data/npc directory for NPC XML files
    bool scanNPCDirectory(const std::string& data_directory);
    
    // Find NPC entry by name
    NPCEntry* findByName(const std::string& name) const;
    
    // Get all loaded NPCs
    const std::map<std::string, NPCEntry>& getAllNPCs() const { return npc_entries; }
    
    // Load detailed NPC data from XML file
    bool loadNPCDetails(NPCEntry& entry) const;
    
    // Create a new NPC XML file
    bool createNPCXML(const NPCEntry& entry, const std::string& output_path) const;
    
    // Open NPC XML file in external editor
    bool openNPCXML(const NPCEntry& entry) const;
    
    // Open NPC Lua script file in external editor
    bool openNPCScript(const NPCEntry& entry) const;
    
    // Check if NPCs are loaded
    bool isLoaded() const { return loaded; }
    
    // Get the base data directory
    const std::string& getDataDirectory() const { return data_directory; }
    
    // Clear all loaded NPC data
    void clear();
    
    // Get statistics for debugging
    std::string getScanStatistics() const;

private:
    std::map<std::string, NPCEntry> npc_entries;
    std::string data_directory;
    bool loaded;
    
    // Helper methods
    bool scanNPCDirectoryRecursive(const std::string& npc_directory);
    bool parseNPCXML(const std::string& npc_xml_path, NPCEntry& entry) const;
    void parseParameters(pugi::xml_node npc_node, NPCEntry& entry) const;
    void generateParametersXML(pugi::xml_node npc_node, const NPCEntry& entry) const;
};

#endif // RME_NPC_MANAGER_H_ 