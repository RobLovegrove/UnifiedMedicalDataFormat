#include "moduleType.hpp"

#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

string module_type_to_string(ModuleType type) {
    switch (type) {
        case ModuleType::Tabular:  return "tabular";
        case ModuleType::Image:    return "image";
        case ModuleType::Frame:    return "frame";
        default:                     return "unknown";
    }
}

ModuleType module_type_from_string(const string& str) {
    string s = str;
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "unknown")   return ModuleType::Unknown;
    if (s == "tabular")   return ModuleType::Tabular;
    if (s == "image")     return ModuleType::Image;
    if (s == "frame")     return ModuleType::Frame;
    return ModuleType::Unknown;
}

bool isValidModuleType(const string& str) {
     return (str == "fileHeader" || str == "xreftable" || str == "tabular" || str == "image" || str == "frame");
}

ostream& operator<<(ostream& os, ModuleType type) {
    return os << module_type_to_string(type);
}