#include "main.h"
#include "npc_manager.h"
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/utils.h>
#include <pugixml.hpp>
#include <regex>
#include <fstream>

NPCManager::NPCManager() : loaded(false) {
}

NPCManager::~NPCManager() {
    clear();
}

bool NPCManager::scanNPCDirectory(const std::string& data_dir) {
    clear();
    data_directory = data_dir;
    
    if (!wxDir::Exists(wxstr(data_dir))) {
        return false;
    }
    
    // Look for npc directory
    std::string npc_directory = data_dir + "/npc";
    if (!wxDir::Exists(wxstr(npc_directory))) {
        OutputDebugStringA(wxString::Format("NPC directory not found: %s\n", npc_directory.c_str()).mb_str());
        return false;
    }
    
    // Scan all XML files in the npc directory and subdirectories
    if (!scanNPCDirectoryRecursive(npc_directory)) {
        return false;
    }
    
    loaded = true;
    
    // Debug output
    OutputDebugStringA(wxString::Format("NPC scan complete:\n").mb_str());
    OutputDebugStringA(wxString::Format("  Data directory: %s\n", wxstr(data_dir)).mb_str());
    OutputDebugStringA(wxString::Format("  NPC directory: %s\n", npc_directory.c_str()).mb_str());
    OutputDebugStringA(wxString::Format("  Total NPC entries: %d\n", 
           (int)npc_entries.size()).mb_str());
    
    return true;
}

bool NPCManager::scanNPCDirectoryRecursive(const std::string& npc_directory) {
    wxDir dir(wxstr(npc_directory));
    if (!dir.IsOpened()) {
        return false;
    }
    
    wxString filename;
    int entries_found = 0;
    
    // First, scan all .xml files in current directory
    bool found = dir.GetFirst(&filename, "*.xml", wxDIR_FILES);
    while (found) {
        wxString fullpath = wxstr(npc_directory) + wxFileName::GetPathSeparator() + filename;
        
        NPCEntry entry;
        entry.filename = nstr(filename);
        entry.full_path = nstr(fullpath);
        
        if (parseNPCXML(nstr(fullpath), entry)) {
            // Only add if parsing was successful
            npc_entries[entry.name] = entry;
            entries_found++;
            
            OutputDebugStringA(wxString::Format("Found NPC: %s -> %s\n", 
                entry.name.c_str(), entry.full_path.c_str()).mb_str());
        }
        
        found = dir.GetNext(&filename);
    }
    
    // Then recursively scan all subdirectories (skip scripts directory to avoid confusion)
    found = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (found) {
        // Skip hidden directories, system directories, and scripts directory
        if (!filename.StartsWith(".") && filename != "scripts") {
            wxString subdirpath = wxstr(npc_directory) + wxFileName::GetPathSeparator() + filename;
            scanNPCDirectoryRecursive(nstr(subdirpath));
        }
        found = dir.GetNext(&filename);
    }
    
    return entries_found > 0;
}

bool NPCManager::parseNPCXML(const std::string& npc_xml_path, NPCEntry& entry) const {
    if (!wxFile::Exists(wxstr(npc_xml_path))) {
        OutputDebugStringA(wxString::Format("NPC XML file not found: %s\n", npc_xml_path.c_str()).mb_str());
        return false;
    }
    
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(npc_xml_path.c_str());
    
    if (!result) {
        OutputDebugStringA(wxString::Format("Failed to parse NPC XML file: %s - %s\n", 
                          npc_xml_path.c_str(), result.description()).mb_str());
        return false;
    }
    
    pugi::xml_node npc_node = doc.child("npc");
    if (!npc_node) {
        OutputDebugStringA(wxString::Format("No npc node in XML file: %s\n", npc_xml_path.c_str()).mb_str());
        return false;
    }
    
    // Parse basic NPC attributes
    if (auto attr = npc_node.attribute("name")) {
        entry.name = attr.as_string();
    } else {
        // If no name attribute, use filename without extension
        wxFileName fn(wxstr(npc_xml_path));
        entry.name = nstr(fn.GetName());
    }
    
    if (auto attr = npc_node.attribute("script")) {
        entry.script_path = attr.as_string();
        entry.hasScript = true;
        
        // Build full script path
        std::string scripts_dir = data_directory + "/npc/scripts/";
        entry.script_full_path = scripts_dir + entry.script_path;
        
        OutputDebugStringA(wxString::Format("NPC %s has script: %s -> %s\n", 
            entry.name.c_str(), entry.script_path.c_str(), entry.script_full_path.c_str()).mb_str());
    }
    
    if (auto attr = npc_node.attribute("walkinterval")) {
        entry.walkinterval = attr.as_int();
    }
    
    if (auto attr = npc_node.attribute("floorchange")) {
        entry.floorchange = attr.as_int();
    }
    
    // Parse health node
    if (auto health_node = npc_node.child("health")) {
        if (auto attr = health_node.attribute("now")) {
            entry.health_now = attr.as_int();
        }
        if (auto attr = health_node.attribute("max")) {
            entry.health_max = attr.as_int();
        }
    }
    
    // Parse look node
    if (auto look_node = npc_node.child("look")) {
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
    }
    
    // Parse parameters
    parseParameters(npc_node, entry);
    
    entry.loaded = true;
    return true;
}

void NPCManager::parseParameters(pugi::xml_node npc_node, NPCEntry& entry) const {
    // Find parameters node
    pugi::xml_node parameters_node = npc_node.child("parameters");
    if (!parameters_node) {
        return;
    }
    
    // Parse all parameter nodes
    for (pugi::xml_node param_node = parameters_node.child("parameter"); param_node; param_node = param_node.next_sibling("parameter")) {
        std::string key = param_node.attribute("key").as_string();
        std::string value = param_node.attribute("value").as_string();
        
        if (!key.empty()) {
            entry.parameters.emplace_back(key, value);
        }
    }
}

NPCEntry* NPCManager::findByName(const std::string& name) const {
    auto it = npc_entries.find(name);
    if (it != npc_entries.end()) {
        return const_cast<NPCEntry*>(&it->second);
    }
    return nullptr;
}

bool NPCManager::loadNPCDetails(NPCEntry& entry) const {
    if (entry.loaded) {
        return true; // Already loaded
    }
    
    return parseNPCXML(entry.full_path, entry);
}

bool NPCManager::createNPCXML(const NPCEntry& entry, const std::string& output_path) const {
    pugi::xml_document doc;
    
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    
    pugi::xml_node npc_node = doc.append_child("npc");
    npc_node.append_attribute("name") = entry.name.c_str();
    
    if (entry.hasScript && !entry.script_path.empty()) {
        npc_node.append_attribute("script") = entry.script_path.c_str();
    }
    
    if (entry.walkinterval != 2000) {
        npc_node.append_attribute("walkinterval") = entry.walkinterval;
    }
    
    if (entry.floorchange != 0) {
        npc_node.append_attribute("floorchange") = entry.floorchange;
    }
    
    // Add health node
    if (entry.health_max > 0) {
        pugi::xml_node health_node = npc_node.append_child("health");
        health_node.append_attribute("now") = entry.health_now;
        health_node.append_attribute("max") = entry.health_max;
    }
    
    // Add look node
    if (entry.look_type > 0) {
        pugi::xml_node look_node = npc_node.append_child("look");
        look_node.append_attribute("type") = entry.look_type;
        
        if (entry.look_head > 0) look_node.append_attribute("head") = entry.look_head;
        if (entry.look_body > 0) look_node.append_attribute("body") = entry.look_body;
        if (entry.look_legs > 0) look_node.append_attribute("legs") = entry.look_legs;
        if (entry.look_feet > 0) look_node.append_attribute("feet") = entry.look_feet;
        if (entry.look_addons > 0) look_node.append_attribute("addons") = entry.look_addons;
    }
    
    // Add parameters
    if (!entry.parameters.empty()) {
        generateParametersXML(npc_node, entry);
    }
    
    return doc.save_file(output_path.c_str(), "\t", pugi::format_default, pugi::encoding_utf8);
}

void NPCManager::generateParametersXML(pugi::xml_node npc_node, const NPCEntry& entry) const {
    pugi::xml_node parameters_node = npc_node.append_child("parameters");
    
    for (const auto& param : entry.parameters) {
        pugi::xml_node param_node = parameters_node.append_child("parameter");
        param_node.append_attribute("key") = param.key.c_str();
        param_node.append_attribute("value") = param.value.c_str();
    }
}

bool NPCManager::openNPCXML(const NPCEntry& entry) const {
    // Use the default Windows application to open the XML file
    wxString filename = wxstr(entry.full_path);
    return wxLaunchDefaultApplication(filename);
}

bool NPCManager::openNPCScript(const NPCEntry& entry) const {
    if (!entry.hasScript || entry.script_full_path.empty()) {
        return false;
    }
    
    // Check if script file exists
    if (!wxFile::Exists(wxstr(entry.script_full_path))) {
        OutputDebugStringA(wxString::Format("NPC script file not found: %s\n", entry.script_full_path.c_str()).mb_str());
        return false;
    }
    
    // Use the default Windows application to open the Lua file
    wxString filename = wxstr(entry.script_full_path);
    return wxLaunchDefaultApplication(filename);
}

void NPCManager::clear() {
    npc_entries.clear();
    loaded = false;
    data_directory.clear();
}

std::string NPCManager::getScanStatistics() const {
    std::string result = "NPC Statistics:\n";
    result += "Directory: " + data_directory + "\n";
    result += "Total NPC entries: " + std::to_string(npc_entries.size()) + "\n";
    
    int with_scripts = 0;
    int loaded_count = 0;
    for (const auto& entry : npc_entries) {
        if (entry.second.loaded) {
            loaded_count++;
        }
        if (entry.second.hasScript) {
            with_scripts++;
        }
    }
    result += "Loaded with details: " + std::to_string(loaded_count) + "\n";
    result += "NPCs with scripts: " + std::to_string(with_scripts);
    
    return result;
} 