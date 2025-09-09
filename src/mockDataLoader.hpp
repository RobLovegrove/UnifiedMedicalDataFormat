#pragma once

#include <string>
#include <vector>
#include <map>
#include "DataModule/dataModule.hpp"
#include "nlohmann/json.hpp"

class MockDataLoader {
public:
    // Load mock data from a JSON file
    static std::pair<std::string, ModuleData> loadMockData(const std::string& filePath);
    
    // Generate image frame data based on configuration
    static std::vector<ModuleData> generateImageFrames(const nlohmann::json& frameConfig);
    
    // List available mock data files
    static std::vector<std::string> listAvailableMockData();
    
private:
    // Generate image pattern data for image frames
    static std::vector<uint8_t> generateImagePattern(int width, int height, int slice, int time, int depth, int timePoints, const nlohmann::json& frameConfig);
    // Generate RGB pattern data for image frames
    static std::vector<uint8_t> generateRGBPattern(int width, int height, int slice, int time, int depth, int timePoints);
};
