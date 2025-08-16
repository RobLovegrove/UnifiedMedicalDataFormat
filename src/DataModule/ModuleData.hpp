#ifndef MODULEDATA_HPP
#define MODULEDATA_HPP

#include <nlohmann/json.hpp>
#include <variant>
#include <vector>
#include <cstdint>

// Forward declaration for recursive structure
struct ModuleData;

struct ModuleData {
    nlohmann::json metadata;
    std::variant<
        nlohmann::json,                    // For tabular data
        std::vector<uint8_t>,              // For frame pixel data
        std::vector<ModuleData>            // For N-dimensional data
    > data;
};

#endif