#include "main.h"
#include "monster_manager.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/utils.h>
#include <pugixml.hpp>
#include <regex>
#include <fstream>

MonsterManager::MonsterManager() : loaded(false) {
}

MonsterManager::~MonsterManager() {
    clear();
}

bool MonsterManager::scanMonstersDirectory(const std::string& data_dir) {
    clear();
    data_directory = data_dir;
    
    if (!wxDir::Exists(wxstr(data_dir))) {
        return false;
    }
    
    // Look for monsters.xml in the data directory
    std::string monsters_xml_path = data_dir + "/monsters.xml";
    if (!parseMonstersXML(monsters_xml_path)) {
        // Try alternative path /monster/monsters.xml (correct TFS path)
        monsters_xml_path = data_dir + "/monster/monsters.xml";
        if (!parseMonstersXML(monsters_xml_path)) {
            return false;
        }
    }
    
    loaded = true;
    
    // Debug output
    OutputDebugStringA(wxString::Format("Monster scan complete:\n").mb_str());
    OutputDebugStringA(wxString::Format("  Data directory: %s\n", wxstr(data_dir)).mb_str());
    OutputDebugStringA(wxString::Format("  Monsters XML loaded: %s\n", monsters_xml_path.c_str()).mb_str());
    OutputDebugStringA(wxString::Format("  Total monster entries: %d\n", 
           (int)monster_entries.size()).mb_str());
    
    return true;
}

bool MonsterManager::parseMonstersXML(const std::string& monsters_xml_path) {
    if (!wxFile::Exists(wxstr(monsters_xml_path))) {
        OutputDebugStringA(wxString::Format("Monsters XML file not found: %s\n", monsters_xml_path.c_str()).mb_str());
        return false;
    }
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(monsters_xml_path.c_str());
    
    if (!result) {
        OutputDebugStringA(wxString::Format("Failed to parse monsters XML file: %s - %s\n", 
                          monsters_xml_path.c_str(), result.description()).mb_str());
        return false;
    }
    
    pugi::xml_node root = doc.first_child();
    if (!root) {
        OutputDebugStringA(wxString::Format("No root node in monsters XML file: %s\n", monsters_xml_path.c_str()).mb_str());
        return false;
    }
    
    int entries_found = 0;
    
    // Find all monster nodes
    for (pugi::xml_node node = root.child("monster"); node; node = node.next_sibling("monster")) {
        std::string name = node.attribute("name").as_string();
        std::string file = node.attribute("file").as_string();
        
        if (name.empty() || file.empty()) {
            continue; // Skip entries without name or file attribute
        }
        
        MonsterEntry entry;
        entry.name = name;
        entry.filename = file;
        
        // Build full path - the file path in monsters.xml is relative to the data/monster directory
        wxFileName monsterFile(wxstr(file));
        if (monsterFile.IsAbsolute()) {
            entry.full_path = file;
        } else {
            // Relative path - append to data/monster directory
            wxFileName relativePath(wxstr(data_directory));
            relativePath.AppendDir("monster");
            relativePath.SetFullName(wxstr(file));
            entry.full_path = nstr(relativePath.GetFullPath());
        }
        
        entries_found++;
        
        OutputDebugStringA(wxString::Format("Found monster entry: %s -> %s\n", 
            name.c_str(), entry.full_path.c_str()).mb_str());
        
        // Store in map
        monster_entries[name] = entry;
    }
    
    OutputDebugStringA(wxString::Format("Parsed %s: found %d entries\n", monsters_xml_path.c_str(), entries_found).mb_str());
    return entries_found > 0;
}

MonsterEntry* MonsterManager::findByName(const std::string& name) const {
    auto it = monster_entries.find(name);
    if (it != monster_entries.end()) {
        return const_cast<MonsterEntry*>(&it->second);
    }
    return nullptr;
}

bool MonsterManager::loadMonsterDetails(MonsterEntry& entry) const {
    if (entry.loaded) {
        return true; // Already loaded
    }
    
    return parseMonsterXML(entry.full_path, entry);
}

bool MonsterManager::parseMonsterXML(const std::string& monster_xml_path, MonsterEntry& entry) const {
    if (!wxFile::Exists(wxstr(monster_xml_path))) {
        OutputDebugStringA(wxString::Format("Monster XML file not found: %s\n", monster_xml_path.c_str()).mb_str());
        return false;
    }
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(monster_xml_path.c_str());
    
    if (!result) {
        OutputDebugStringA(wxString::Format("Failed to parse monster XML file: %s - %s\n", 
                          monster_xml_path.c_str(), result.description()).mb_str());
        return false;
    }
    
    pugi::xml_node monster_node = doc.child("monster");
    if (!monster_node) {
        OutputDebugStringA(wxString::Format("No monster node in XML file: %s\n", monster_xml_path.c_str()).mb_str());
        return false;
    }
    
    // Parse basic monster attributes
    if (auto attr = monster_node.attribute("name")) {
        entry.name = attr.as_string();
    }
    
    if (auto attr = monster_node.attribute("race")) {
        entry.race = attr.as_string();
    }
    
    if (auto attr = monster_node.attribute("experience")) {
        entry.experience = attr.as_int();
    }
    
    if (auto attr = monster_node.attribute("speed")) {
        entry.speed = attr.as_int();
    }
    
    if (auto attr = monster_node.attribute("manacost")) {
        entry.manacost = attr.as_int();
    }
    
    // Parse health node
    if (auto health_node = monster_node.child("health")) {
        if (auto attr = health_node.attribute("max")) {
            entry.health_max = attr.as_int();
        }
    }
    
    // Parse look node
    if (auto look_node = monster_node.child("look")) {
        if (auto attr = look_node.attribute("type")) {
            entry.look_type = attr.as_int();
        }
        if (auto attr = look_node.attribute("head")) {
            entry.look_head = attr.as_int();
        }
        if (auto attr = look_node.attribute("body")) {
            entry.look_body = attr.as_int();
        }
        if (auto attr = look_node.attribute("legs")) {
            entry.look_legs = attr.as_int();
        }
        if (auto attr = look_node.attribute("feet")) {
            entry.look_feet = attr.as_int();
        }
        if (auto attr = look_node.attribute("addons")) {
            entry.look_addons = attr.as_int();
        }
        if (auto attr = look_node.attribute("mount")) {
            entry.look_mount = attr.as_int();
        }
    }
    
    entry.loaded = true;
    return true;
}

bool MonsterManager::createMonsterXML(const MonsterEntry& entry, const std::string& output_path) const {
    pugi::xml_document doc;
    
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    
    pugi::xml_node monster_node = doc.append_child("monster");
    monster_node.append_attribute("name") = entry.name.c_str();
    
    if (!entry.race.empty()) {
        monster_node.append_attribute("race") = entry.race.c_str();
    }
    
    if (entry.experience > 0) {
        monster_node.append_attribute("experience") = entry.experience;
    }
    
    if (entry.speed > 0) {
        monster_node.append_attribute("speed") = entry.speed;
    }
    
    if (entry.manacost > 0) {
        monster_node.append_attribute("manacost") = entry.manacost;
    }
    
    // Add health node
    if (entry.health_max > 0) {
        pugi::xml_node health_node = monster_node.append_child("health");
        health_node.append_attribute("max") = entry.health_max;
        health_node.append_attribute("now") = entry.health_max; // Default to max
    }
    
    // Add look node
    if (entry.look_type > 0) {
        pugi::xml_node look_node = monster_node.append_child("look");
        look_node.append_attribute("type") = entry.look_type;
        
        if (entry.look_head > 0) look_node.append_attribute("head") = entry.look_head;
        if (entry.look_body > 0) look_node.append_attribute("body") = entry.look_body;
        if (entry.look_legs > 0) look_node.append_attribute("legs") = entry.look_legs;
        if (entry.look_feet > 0) look_node.append_attribute("feet") = entry.look_feet;
        if (entry.look_addons > 0) look_node.append_attribute("addons") = entry.look_addons;
        if (entry.look_mount > 0) look_node.append_attribute("mount") = entry.look_mount;
    }
    
    return doc.save_file(output_path.c_str(), "\t", pugi::format_default, pugi::encoding_utf8);
}

bool MonsterManager::openMonsterXML(const MonsterEntry& entry) const {
    // Use the default Windows application to open the XML file
    wxString filename = wxstr(entry.full_path);
    return wxLaunchDefaultApplication(filename);
}

void MonsterManager::clear() {
    monster_entries.clear();
    loaded = false;
    data_directory.clear();
}

std::string MonsterManager::getScanStatistics() const {
    std::string result = "Monster Statistics:\n";
    result += "Directory: " + data_directory + "\n";
    result += "Total monster entries: " + std::to_string(monster_entries.size()) + "\n";
    
    int loaded_count = 0;
    for (const auto& entry : monster_entries) {
        if (entry.second.loaded) {
            loaded_count++;
        }
    }
    result += "Loaded with details: " + std::to_string(loaded_count);
    
    return result;
} 