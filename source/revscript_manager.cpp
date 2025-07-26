#include "main.h"
#include "revscript_manager.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/process.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>
#include <regex>
#include <fstream>

RevScriptManager::RevScriptManager() : loaded(false) {
}

RevScriptManager::~RevScriptManager() {
    clear();
}

bool RevScriptManager::scanRevScriptsDirectory(const std::string& data_dir) {
    clear();
    data_directory = data_dir;
    
    if (!wxDir::Exists(wxstr(data_dir))) {
        return false;
    }
    
    // Scan scripts directory (for RevScript .lua files)
    std::string scripts_dir = data_dir + "/scripts";
    if (wxDir::Exists(wxstr(scripts_dir))) {
        scanDirectoryRecursive(scripts_dir);
    }
    
    // Try to find and scan actions.xml and movements.xml in their respective directories
    bool actions_loaded = scanActionsXml(data_dir);
    bool movements_loaded = scanMovementsXml(data_dir);
    
    loaded = true;
    
    // Debug output
    OutputDebugStringA(wxString::Format("RevScript scan complete:\n").mb_str());
    OutputDebugStringA(wxString::Format("  Data directory: %s\n", wxstr(data_dir)).mb_str());
    OutputDebugStringA(wxString::Format("  Scripts directory exists: %s\n", wxDir::Exists(wxstr(scripts_dir)) ? "yes" : "no").mb_str());
    OutputDebugStringA(wxString::Format("  Actions XML loaded: %s\n", actions_loaded ? "yes" : "no").mb_str());
    OutputDebugStringA(wxString::Format("  Movements XML loaded: %s\n", movements_loaded ? "yes" : "no").mb_str());
    OutputDebugStringA(wxString::Format("  Total RevScript entries: %d AID, %d UID, %d ItemID\n", 
           (int)action_id_entries.size(), (int)unique_id_entries.size(), (int)item_id_entries.size()).mb_str());
    OutputDebugStringA(wxString::Format("  Total XML entries: %d actions, %d movements\n", 
           (int)xml_action_entries.size(), (int)xml_movement_entries.size()).mb_str());
    
    return true;
}

void RevScriptManager::scanDirectoryRecursive(const std::string& dir_path) {
    wxDir dir(wxstr(dir_path));
    if (!dir.IsOpened()) {
        return;
    }
    
    wxString filename;
    
    // First, scan all .lua files in current directory
    bool found = dir.GetFirst(&filename, "*.lua", wxDIR_FILES);
    while (found) {
        wxString fullpath = wxstr(dir_path) + wxFileName::GetPathSeparator() + filename;
        scanLuaFile(nstr(fullpath));
        found = dir.GetNext(&filename);
    }
    
    // Then recursively scan all subdirectories
    found = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (found) {
        // Skip hidden directories and system directories
        if (!filename.StartsWith(".")) {
            wxString subdirpath = wxstr(dir_path) + wxFileName::GetPathSeparator() + filename;
            scanDirectoryRecursive(nstr(subdirpath));
        }
        found = dir.GetNext(&filename);
    }
}

bool RevScriptManager::scanLuaFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    int line_number = 0;
    std::string current_function = "";
    
    // Regex patterns for different ID types
    std::regex aid_pattern(R"((\w+):aid\s*\(\s*(\d+)\s*\))");
    std::regex uid_pattern(R"((\w+):uid\s*\(\s*(\d+)\s*\))");
    std::regex id_pattern(R"((\w+):id\s*\(\s*(\d+)\s*\))");
    std::regex function_pattern(R"(function\s+(\w+[\w\.]*)\s*\()");
    
    while (std::getline(file, line)) {
        line_number++;
        
        // Check for function definitions to track context
        std::smatch function_match;
        if (std::regex_search(line, function_match, function_pattern)) {
            current_function = function_match[1].str();
        }
        
        // Check for action ID patterns
        std::smatch aid_match;
        if (std::regex_search(line, aid_match, aid_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(aid_match[2].str());
            entry.id_type = "aid";
            entry.script_type = "revscript";
            addEntry(entry);
        }
        
        // Check for unique ID patterns
        std::smatch uid_match;
        if (std::regex_search(line, uid_match, uid_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(uid_match[2].str());
            entry.id_type = "uid";
            entry.script_type = "revscript";
            addEntry(entry);
        }
        
        // Check for item ID patterns
        std::smatch id_match;
        if (std::regex_search(line, id_match, id_pattern)) {
            RevScriptEntry entry;
            entry.filename = filepath;
            entry.function_name = current_function;
            entry.line_number = line_number;
            entry.id_value = std::stoi(id_match[2].str());
            entry.id_type = "id";
            entry.script_type = "revscript";
            addEntry(entry);
        }
    }
    
    return true;
}

bool RevScriptManager::scanActionsXml(const std::string& base_directory) {
    std::string actions_xml_path = base_directory + "/actions/actions.xml";
    std::string actions_script_dir = base_directory + "/actions/scripts/";
    
    return parseXmlFile(actions_xml_path, "action", actions_script_dir);
}

bool RevScriptManager::scanMovementsXml(const std::string& base_directory) {
    std::string movements_xml_path = base_directory + "/movements/movements.xml";
    std::string movements_script_dir = base_directory + "/movements/scripts/";
    
    return parseXmlFile(movements_xml_path, "movement", movements_script_dir);
}

bool RevScriptManager::parseXmlFile(const std::string& xml_path, const std::string& script_type, const std::string& base_script_dir) {
    if (!wxFile::Exists(wxstr(xml_path))) {
        OutputDebugStringA(wxString::Format("XML file not found: %s\n", xml_path.c_str()).mb_str());
        return false;
    }
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xml_path.c_str());
    
    if (!result) {
        OutputDebugStringA(wxString::Format("Failed to parse XML file: %s - %s\n", xml_path.c_str(), result.description()).mb_str());
        return false;
    }
    
    pugi::xml_node root = doc.first_child();
    if (!root) {
        OutputDebugStringA(wxString::Format("No root node in XML file: %s\n", xml_path.c_str()).mb_str());
        return false;
    }
    
    int entries_found = 0;
    
    // For movements, we need to look for "movevent" nodes, for actions we look for "action" nodes
    const char* node_name = (script_type == "movement") ? "movevent" : script_type.c_str();
    
    // Find all action/movement nodes
    for (pugi::xml_node node = root.child(node_name); node; node = node.next_sibling(node_name)) {
        // For movements: only process entries that have a "script" attribute (ignore "function" ones)
        std::string script_path = node.attribute("script").as_string();
        if (script_path.empty()) {
            continue; // Skip entries without script attribute (e.g., those with "function" attribute)
        }
        
        XmlScriptEntry entry;
        entry.script_type = script_type;
        entry.script_path = script_path;
        entry.full_path = base_script_dir + script_path;
        
        // Parse different ID attributes
        if (node.attribute("itemid")) {
            entry.id_attribute = "itemid";
            entry.from_id = node.attribute("itemid").as_int();
            entry.to_id = entry.from_id;
        } else if (node.attribute("fromid") && node.attribute("toid")) {
            entry.id_attribute = "itemid";
            entry.from_id = node.attribute("fromid").as_int();
            entry.to_id = node.attribute("toid").as_int();
        } else if (node.attribute("actionid")) {
            entry.id_attribute = "actionid";
            entry.from_id = node.attribute("actionid").as_int();
            entry.to_id = entry.from_id;
        } else if (node.attribute("fromaid") && node.attribute("toaid")) {
            entry.id_attribute = "actionid";
            entry.from_id = node.attribute("fromaid").as_int();
            entry.to_id = node.attribute("toaid").as_int();
        } else if (node.attribute("uniqueid")) {
            entry.id_attribute = "uniqueid";
            entry.from_id = node.attribute("uniqueid").as_int();
            entry.to_id = entry.from_id;
            OutputDebugStringA(wxString::Format("Found uniqueid entry: %d -> %s\n", entry.from_id, entry.full_path.c_str()).mb_str());
        } else if (node.attribute("fromuid") && node.attribute("touid")) {
            entry.id_attribute = "uniqueid";
            entry.from_id = node.attribute("fromuid").as_int();
            entry.to_id = node.attribute("touid").as_int();
            OutputDebugStringA(wxString::Format("Found uniqueid range entry: %d-%d -> %s\n", entry.from_id, entry.to_id, entry.full_path.c_str()).mb_str());
        } else {
            continue; // Skip entries without valid ID attributes
        }
        
        entries_found++;
        
        // Debug output for movements with event info
        if (script_type == "movement") {
            std::string event = node.attribute("event").as_string();
            OutputDebugStringA(wxString::Format("Found %s movement event '%s': %s %d -> %s\n", 
                entry.id_attribute.c_str(), event.c_str(), entry.id_attribute.c_str(), entry.from_id, entry.full_path.c_str()).mb_str());
        }
        
        // Store in appropriate vector
        if (script_type == "action") {
            xml_action_entries.push_back(entry);
        } else if (script_type == "movement") {
            xml_movement_entries.push_back(entry);
        }
    }
    
    OutputDebugStringA(wxString::Format("Parsed %s: found %d entries\n", xml_path.c_str(), entries_found).mb_str());
    return true;
}

void RevScriptManager::addEntry(const RevScriptEntry& entry) {
    if (entry.id_type == "aid") {
        action_id_entries.emplace(entry.id_value, entry);
    } else if (entry.id_type == "uid") {
        unique_id_entries.emplace(entry.id_value, entry);
    } else if (entry.id_type == "id") {
        item_id_entries.emplace(entry.id_value, entry);
    }
}

std::vector<RevScriptEntry> RevScriptManager::findByActionID(int action_id) const {
    std::vector<RevScriptEntry> results;
    
    // First check RevScript entries
    auto range = action_id_entries.equal_range(action_id);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    
    // Then check XML action entries
    auto xml_entries = findXmlEntriesByActionID(action_id);
    for (const auto& xml_entry : xml_entries) {
        RevScriptEntry entry;
        entry.filename = xml_entry.full_path;
        entry.function_name = "";
        entry.line_number = 1;
        entry.id_value = action_id;
        entry.id_type = "aid";
        entry.script_type = xml_entry.script_type;
        results.push_back(entry);
    }
    
    return results;
}

std::vector<RevScriptEntry> RevScriptManager::findByUniqueID(int unique_id) const {
    std::vector<RevScriptEntry> results;
    
    // First check RevScript entries
    auto range = unique_id_entries.equal_range(unique_id);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    
    // Then check XML entries
    auto xml_entries = findXmlEntriesByUniqueID(unique_id);
    for (const auto& xml_entry : xml_entries) {
        RevScriptEntry entry;
        entry.filename = xml_entry.full_path;
        entry.function_name = "";
        entry.line_number = 1;
        entry.id_value = unique_id;
        entry.id_type = "uid";
        entry.script_type = xml_entry.script_type;
        results.push_back(entry);
    }
    
    return results;
}

std::vector<RevScriptEntry> RevScriptManager::findByItemID(int item_id) const {
    std::vector<RevScriptEntry> results;
    
    // First check RevScript entries
    auto range = item_id_entries.equal_range(item_id);
    for (auto it = range.first; it != range.second; ++it) {
        results.push_back(it->second);
    }
    
    // Then check XML action entries for item IDs
    auto xml_action_entries = findXmlEntriesByItemID(item_id, "action");
    for (const auto& xml_entry : xml_action_entries) {
        RevScriptEntry entry;
        entry.filename = xml_entry.full_path;
        entry.function_name = "";
        entry.line_number = 1;
        entry.id_value = item_id;
        entry.id_type = "id";
        entry.script_type = xml_entry.script_type;
        results.push_back(entry);
    }
    
    // Check XML movement entries for item IDs
    auto xml_movement_entries = findXmlEntriesByItemID(item_id, "movement");
    for (const auto& xml_entry : xml_movement_entries) {
        RevScriptEntry entry;
        entry.filename = xml_entry.full_path;
        entry.function_name = "";
        entry.line_number = 1;
        entry.id_value = item_id;
        entry.id_type = "id";
        entry.script_type = xml_entry.script_type;
        results.push_back(entry);
    }
    
    return results;
}

bool RevScriptManager::hasScriptForActionID(int action_id) const {
    // Check RevScript entries
    if (action_id_entries.find(action_id) != action_id_entries.end()) {
        return true;
    }
    
    // Check XML entries
    return !findXmlEntriesByActionID(action_id).empty();
}

bool RevScriptManager::hasScriptForUniqueID(int unique_id) const {
    // Check RevScript entries  
    if (unique_id_entries.find(unique_id) != unique_id_entries.end()) {
        return true;
    }
    
    // Check XML entries
    return !findXmlEntriesByUniqueID(unique_id).empty();
}

bool RevScriptManager::hasScriptForItemID(int item_id) const {
    // Check RevScript entries
    if (item_id_entries.find(item_id) != item_id_entries.end()) {
        return true;
    }
    
    // Check XML entries for both actions and movements
    return !findXmlEntriesByItemID(item_id, "action").empty() || 
           !findXmlEntriesByItemID(item_id, "movement").empty();
}

std::vector<XmlScriptEntry> RevScriptManager::findXmlEntriesByItemID(int item_id, const std::string& script_type) const {
    std::vector<XmlScriptEntry> results;
    
    const std::vector<XmlScriptEntry>* entries;
    if (script_type == "action") {
        entries = &xml_action_entries;
    } else if (script_type == "movement") {
        entries = &xml_movement_entries;
    } else {
        return results;
    }
    
    for (const auto& entry : *entries) {
        if (entry.id_attribute == "itemid" && entry.isInRange(item_id)) {
            results.push_back(entry);
        }
    }
    
    return results;
}

std::vector<XmlScriptEntry> RevScriptManager::findXmlEntriesByActionID(int action_id) const {
    std::vector<XmlScriptEntry> results;
    
    for (const auto& entry : xml_action_entries) {
        if (entry.id_attribute == "actionid" && entry.isInRange(action_id)) {
            results.push_back(entry);
        }
    }
    
    return results;
}

std::vector<XmlScriptEntry> RevScriptManager::findXmlEntriesByUniqueID(int unique_id) const {
    std::vector<XmlScriptEntry> results;
    
    // Check both action and movement entries for unique IDs
    for (const auto& entry : xml_action_entries) {
        if (entry.id_attribute == "uniqueid" && entry.isInRange(unique_id)) {
            results.push_back(entry);
        }
    }
    
    for (const auto& entry : xml_movement_entries) {
        if (entry.id_attribute == "uniqueid" && entry.isInRange(unique_id)) {
            results.push_back(entry);
        }
    }
    
    return results;
}

bool RevScriptManager::openRevScript(const RevScriptEntry& entry) const {
    // Use the default Windows application to open the file
    wxString filename = wxstr(entry.filename);
    return wxLaunchDefaultApplication(filename);
}

std::string RevScriptManager::getDefaultEditor() const {
    // This method is no longer needed since we use default app
    // but keeping it for potential future use
    return "";
}

void RevScriptManager::clear() {
    action_id_entries.clear();
    unique_id_entries.clear();
    item_id_entries.clear();
    xml_action_entries.clear();
    xml_movement_entries.clear();
    loaded = false;
    data_directory.clear();
}

std::string RevScriptManager::getScanStatistics() const {
    int action_ids = 0, unique_ids = 0, item_ids = 0;
    
    // Count RevScript entries
    action_ids = action_id_entries.size();
    unique_ids = unique_id_entries.size();
    item_ids = item_id_entries.size();
    
    // Count XML entries
    int xml_action_count = xml_action_entries.size();
    int xml_movement_count = xml_movement_entries.size();
    
    std::string result = "RevScript Statistics:\n";
    result += "Directory: " + data_directory + "\n";
    result += "RevScript Action IDs: " + std::to_string(action_ids) + "\n";
    result += "RevScript Unique IDs: " + std::to_string(unique_ids) + "\n";
    result += "RevScript Item IDs: " + std::to_string(item_ids) + "\n";
    result += "XML Action entries: " + std::to_string(xml_action_count) + "\n";
    result += "XML Movement entries: " + std::to_string(xml_movement_count) + "\n";
    result += "Total entries: " + std::to_string(action_ids + unique_ids + item_ids + xml_action_count + xml_movement_count);
    
    return result;
} 