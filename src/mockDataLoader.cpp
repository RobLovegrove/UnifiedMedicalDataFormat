#include "mockDataLoader.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <random>

std::pair<std::string, ModuleData> MockDataLoader::loadMockData(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open mock data file: " + filePath);
    }
    
    nlohmann::json jsonData;
    file >> jsonData;
    file.close();
    
    ModuleData moduleData;
    
    // Load metadata
    if (jsonData.contains("metadata")) {
        if (jsonData.contains("frame_config")) {
            // Image data - metadata should be a single object, not array
            moduleData.metadata = jsonData["metadata"];
        } else {
            // Tabular data - metadata should be an array
            if (jsonData["metadata"].is_array()) {
                moduleData.metadata = jsonData["metadata"];
            } else {
                moduleData.metadata = nlohmann::json::array();
                moduleData.metadata.push_back(jsonData["metadata"]);
            }
        }
    }
    
    // Load data
    if (jsonData.contains("data")) {
        if (jsonData["data"].is_array()) {
            // Tabular data
            moduleData.data = jsonData["data"];
        } else {
            moduleData.data = jsonData["data"];
        }
    } else if (jsonData.contains("frame_config")) {
        // Image data - generate frames
        moduleData.data = generateImageFrames(jsonData["frame_config"]);
    }
    
    // Get schema path
    std::string schemaPath = jsonData.value("schema_path", "");
    if (schemaPath.empty()) {
        throw std::runtime_error("No schema_path specified in mock data file: " + filePath);
    }
    
    return {schemaPath, moduleData};
}

std::vector<ModuleData> MockDataLoader::generateImageFrames(const nlohmann::json& frameConfig) {
    std::vector<ModuleData> frameModules;
    
    int width = frameConfig.value("width", 256);
    int height = frameConfig.value("height", 256);
    int depth = frameConfig.value("depth", 12);
    int timePoints = frameConfig.value("timePoints", 5);
    
    // Create frames for each slice and time point
    for (int time = 0; time < timePoints; time++) {
        for (int slice = 0; slice < depth; slice++) {
            std::vector<uint8_t> sliceData = generateImagePattern(width, height, slice, time, depth, timePoints, frameConfig);
            
            // Create frame metadata with 4D positioning
            ModuleData frameModule;
            frameModule.metadata = {
                {"position", {0.0, 0.0, static_cast<double>(slice), static_cast<double>(time)}},
                {"orientation", {
                    {"row_cosine", {1.0, 0.0, 0.0}},
                    {"column_cosine", {0.0, 1.0, 0.0}}
                }},
                {"timestamp", "2024-01-01T12:00:00Z"},
                {"frame_number", slice + time * depth},
                {"time_point", time},
                {"slice_number", slice}
            };
            
            frameModule.data = sliceData;
            frameModules.push_back(frameModule);
        }
    }
    
    return frameModules;
}

std::vector<uint8_t> MockDataLoader::generateImagePattern(int width, int height, int slice, int time, int depth, int timePoints, const nlohmann::json& frameConfig) {
    std::string patternType = frameConfig.value("pattern_type", "rgb_gradient");
    
    if (patternType == "grayscale_gradient") {
        // Generate grayscale data for MRI
        int channels = frameConfig.value("channels", 1);
        int bitDepth = frameConfig.value("bit_depth", 16);
        int bytesPerPixel = (bitDepth == 16) ? 2 : 1;
        int totalBytes = width * height * channels * bytesPerPixel;
        
        std::vector<uint8_t> sliceData(totalBytes);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                size_t pixel_index = static_cast<size_t>(y * width + x);
                
                // Z dimension (slice) affects brightness
                double zBrightness = 1.0 - (static_cast<double>(slice) / (depth - 1)) * 0.7;
                zBrightness = std::max(0.3, std::min(1.0, zBrightness));
                
                // Create grayscale pattern
                uint16_t intensity = static_cast<uint16_t>(255 * zBrightness);
                
                if (bitDepth == 16) {
                    // 16-bit grayscale
                    sliceData[pixel_index * 2 + 0] = intensity & 0xFF;        // Low byte
                    sliceData[pixel_index * 2 + 1] = (intensity >> 8) & 0xFF; // High byte
                } else {
                    // 8-bit grayscale
                    sliceData[pixel_index] = static_cast<uint8_t>(intensity);
                }
            }
        }
        
        return sliceData;
    } else {
        // Default to RGB pattern
        return generateRGBPattern(width, height, slice, time, depth, timePoints);
    }
}

std::vector<uint8_t> MockDataLoader::generateRGBPattern(int width, int height, int slice, int time, int depth, int /* timePoints */) {
    std::vector<uint8_t> sliceData(width * height * 3);  // RGB data
    
    // Fill with RGB pattern - create high contrast patterns that change with slice and time
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t pixel_index = static_cast<size_t>(y * width + x);
            
            // Z dimension (slice) affects brightness - creates a clear light-to-dark gradient
            // First slice (z=0) is brightest, last slice (z=depth-1) is darkest
            double zBrightness = 1.0 - (static_cast<double>(slice) / (depth - 1)) * 0.7;
            zBrightness = std::max(0.3, std::min(1.0, zBrightness));
            
            // Time dimension affects color dominance - each time point gets a distinct color
            uint8_t r, g, b;
            
            if (time == 0) {
                // Time 0: Red dominant
                r = static_cast<uint8_t>(255 * zBrightness);
                g = static_cast<uint8_t>(80 * zBrightness);
                b = static_cast<uint8_t>(80 * zBrightness);
            } else if (time == 1) {
                // Time 1: Green dominant
                r = static_cast<uint8_t>(80 * zBrightness);
                g = static_cast<uint8_t>(255 * zBrightness);
                b = static_cast<uint8_t>(80 * zBrightness);
            } else if (time == 2) {
                // Time 2: Blue dominant
                r = static_cast<uint8_t>(80 * zBrightness);
                g = static_cast<uint8_t>(80 * zBrightness);
                b = static_cast<uint8_t>(255 * zBrightness);
            } else if (time == 3) {
                // Time 3: Yellow dominant (Red + Green)
                r = static_cast<uint8_t>(255 * zBrightness);
                g = static_cast<uint8_t>(255 * zBrightness);
                b = static_cast<uint8_t>(80 * zBrightness);
            } else {
                // Time 4: Magenta dominant (Red + Blue)
                r = static_cast<uint8_t>(255 * zBrightness);
                g = static_cast<uint8_t>(80 * zBrightness);
                b = static_cast<uint8_t>(255 * zBrightness);
            }
            
            // Interleaved RGB format: RGBRGBRGB...
            sliceData[pixel_index * 3 + 0] = r;  // Red
            sliceData[pixel_index * 3 + 1] = g;  // Green
            sliceData[pixel_index * 3 + 2] = b;  // Blue
        }
    }
    
    return sliceData;
}

std::vector<std::string> MockDataLoader::listAvailableMockData() {
    std::vector<std::string> mockDataFiles;
    std::string mockDataDir = "mock_data";
    
    if (!std::filesystem::exists(mockDataDir)) {
        return mockDataFiles;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(mockDataDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            mockDataFiles.push_back(entry.path().string());
        }
    }
    
    return mockDataFiles;
}
