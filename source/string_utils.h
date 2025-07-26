#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include <vector>
#include <string>

std::vector<std::string> splitString(const std::string& str, char delimiter);
bool isInteger(const std::string& s);
std::vector<std::pair<uint16_t, uint16_t>> parseIdRangesString(const std::string& input);
bool isIdInRanges(uint16_t id, const std::vector<std::pair<uint16_t, uint16_t>>& ranges);

#endif // STRING_UTILS_H_ 