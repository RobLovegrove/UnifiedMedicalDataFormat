#ifndef MODULETYPE_HPP
#define MODULETYPE_HPP

#include <string>

enum class ModuleType : uint8_t {
    FileHeader = 0,
    XrefTable,   
    Tabular,
    Image
};

// Converts ModuleType to string
std::string module_type_to_string(ModuleType type);

// Converts string to ModuleType (case-insensitive)
ModuleType module_type_from_string(const std::string& str);

// Stream output support
std::ostream& operator<<(std::ostream& os, ModuleType type);

#endif