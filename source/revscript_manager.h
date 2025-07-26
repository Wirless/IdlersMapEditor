#ifndef RME_REVSCRIPT_MANAGER_H_
#define RME_REVSCRIPT_MANAGER_H_

#include "main.h"
#include <map>
#include <string>
#include <vector>

struct RevScriptEntry {
    std::string filename;
    std::string function_name;
    int line_number;
    int id_value;
    std::string id_type; // "aid", "uid", or "id"
    std::string script_type; // "revscript", "action", "movement"
};

struct XmlScriptEntry {
    std::string script_path; // Relative path to script file
    std::string full_path;   // Full absolute path to script file
    std::string script_type; // "action" or "movement" 
    
    // ID ranges
    int from_id;
    int to_id;
    std::string id_attribute; // "itemid", "actionid", "uniqueid"
    
    bool isInRange(int id) const {
        return id >= from_id && id <= to_id;
    }
};

class RevScriptManager {
public:
    RevScriptManager();
    ~RevScriptManager();
    
    // Scan the revscripts directory for files
    bool scanRevScriptsDirectory(const std::string& scripts_directory);
    
    // Find revscript entries by action ID
    std::vector<RevScriptEntry> findByActionID(int action_id) const;
    
    // Find revscript entries by unique ID  
    std::vector<RevScriptEntry> findByUniqueID(int unique_id) const;
    
    // Find revscript entries by item ID
    std::vector<RevScriptEntry> findByItemID(int item_id) const;
    
    // Check if any script exists for given IDs
    bool hasScriptForActionID(int action_id) const;
    bool hasScriptForUniqueID(int unique_id) const;
    bool hasScriptForItemID(int item_id) const;
    
    // Open a revscript file in external editor
    bool openRevScript(const RevScriptEntry& entry) const;
    
    // Clear all loaded revscript data
    void clear();
    
    // Check if revscripts are loaded
    bool isLoaded() const { return loaded; }
    
    // Get the base scripts directory
    const std::string& getScriptsDirectory() const { return data_directory; }
    
    // Get statistics for debugging
    std::string getScanStatistics() const;

private:
    void scanDirectoryRecursive(const std::string& directory);
    bool scanActionsXml(const std::string& data_directory);
    bool scanMovementsXml(const std::string& data_directory);
    
    // RevScript entries (from Lua files)
    std::multimap<int, RevScriptEntry> action_id_entries;
    std::multimap<int, RevScriptEntry> unique_id_entries;
    std::multimap<int, RevScriptEntry> item_id_entries;
    
    // XML script entries (from actions.xml and movements.xml)
    std::vector<XmlScriptEntry> xml_action_entries;
    std::vector<XmlScriptEntry> xml_movement_entries;
    
    std::string data_directory;
    bool loaded;
    
    // Helper methods for RevScript scanning
    bool scanLuaFile(const std::string& filepath);
    void addEntry(const RevScriptEntry& entry);
    
    // Helper methods for XML scanning
    bool parseXmlFile(const std::string& xml_path, const std::string& script_type, const std::string& base_script_dir);
    std::vector<XmlScriptEntry> findXmlEntriesByItemID(int item_id, const std::string& script_type) const;
    std::vector<XmlScriptEntry> findXmlEntriesByActionID(int action_id) const;
    std::vector<XmlScriptEntry> findXmlEntriesByUniqueID(int unique_id) const;
    
    std::string getDefaultEditor() const;
};

#endif // RME_REVSCRIPT_MANAGER_H_ 