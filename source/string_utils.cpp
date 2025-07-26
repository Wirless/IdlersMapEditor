#include "main.h"
#include "string_utils.h"
#include <algorithm>
#include <cctype>
#include <sstream>


// Helper function to split a string by a delimiter
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        // Optionally trim whitespace from each token
        token.erase(token.find_last_not_of(" \n\r\t")+1);
        token.erase(0, token.find_first_not_of(" \n\r\t"));
        tokens.push_back(token);
    }
    return tokens;
}

// Helper function to check if a string represents a valid integer
bool isInteger(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), ::isdigit);
}

std::vector<std::pair<uint16_t, uint16_t>> parseIdRangesString(const std::string& input) {
    std::vector<std::pair<uint16_t, uint16_t>> ranges;
    std::vector<std::string> parts = splitString(input, ',');
    for(const auto& part : parts) {
        if(part.find('-') != std::string::npos) {
            std::vector<std::string> range = splitString(part, '-');
            if(range.size() == 2 && isInteger(range[0]) && isInteger(range[1])) {
                uint16_t from = static_cast<uint16_t>(std::stoi(range[0]));
                uint16_t to = static_cast<uint16_t>(std::stoi(range[1]));
                if(from <= to) {
                    ranges.emplace_back(from, to);
                }
            }
        } else if(isInteger(part)) {
            uint16_t id = static_cast<uint16_t>(std::stoi(part));
            ranges.emplace_back(id, id);
        }
    }
    return ranges;
}

bool isIdInRanges(uint16_t id, const std::vector<std::pair<uint16_t, uint16_t>>& ranges) {
    for(const auto& range : ranges) {
        if(id >= range.first && id <= range.second) {
            return true;
        }
    }
    return false;
} 