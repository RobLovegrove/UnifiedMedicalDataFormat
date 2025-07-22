#include "moduleType.hpp"

#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

string module_type_to_string(ModuleType type) {
    switch (type) {
        case ModuleType::FileHeader:    return "fileHeader";
        case ModuleType::XrefTable:  return "xrefTable";
        case ModuleType::Tabular:  return "tabular";
        case ModuleType::Image:    return "image";
        default:                     return "unknown";
    }
}

ModuleType module_type_from_string(const string& str) {
    string s = str;
    transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "fileHeader")     return ModuleType::FileHeader;
    if (s == "xreftable")   return ModuleType::XrefTable;
    if (s == "tabular")   return ModuleType::Tabular;
    if (s == "image")     return ModuleType::Image;

    throw invalid_argument("Invalid ModuleType string: " + str);
}

bool isValidModuleType(const string& str) {
     return (str == "fileHeader" || str == "xreftable" || str == "tabular" || str == "image");
}

ostream& operator<<(ostream& os, ModuleType type) {
    return os << module_type_to_string(type);
}