#include "moduleType.hpp"

#include <stdexcept>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

string to_string(ModuleType type) {
    switch (type) {
        case ModuleType::Patient:    return "patient";
        case ModuleType::XrefTable:  return "xrefTable";
        case ModuleType::Encounter:  return "encounter";
        case ModuleType::Imaging:    return "image";
        default:                     return "unknown";
    }
}

ModuleType module_type_from_string(const string& str) {
    string s = str;
    transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "patient")     return ModuleType::Patient;
    if (s == "xreftable")   return ModuleType::XrefTable;
    if (s == "encounter")   return ModuleType::Encounter;
    if (s == "imaging")     return ModuleType::Imaging;

    throw invalid_argument("Invalid ModuleType string: " + str);
}

ostream& operator<<(ostream& os, ModuleType type) {
    return os << to_string(type);
}