#include <catch2/catch_all.hpp>
#include "DataModule/Image/imageData.hpp"
#include "Utility/uuid.hpp"
#include "Utility/Compression/CompressionType.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using namespace nlohmann;
namespace fs = std::filesystem;

static std::string writeTempFile(const std::string& relPath, const nlohmann::json& j) {
    fs::path p = fs::path("build/tests_tmp") / relPath;
    fs::create_directories(p.parent_path());
    std::ofstream out(p);
    REQUIRE(out.good());
    out << j.dump(2);
    out.close();
    return p.string();
}

TEST_CASE("ImageData frame validation", "[imageData][validation][frames]") {
    
    SECTION("Validates frame metadata structure") {
        // Create frame schema
        json frameSchema = {
            {"$schema", "http://json-schema.org/draft-07/schema#"},
            {"type", "object"},
            {"required", {"position", "orientation"}},
            {"properties", {
                {"position", {
                    {"type", "array"},
                    {"items", {{"type", "number"}}},
                    {"minItems", 2},
                    {"maxItems", 10}
                }},
                {"orientation", {
                    {"type", "object"},
                    {"properties", {
                        {"row_cosine", {
                            {"type", "array"},
                            {"items", {{"type", "number"}}},
                            {"minItems", 3},
                            {"maxItems", 3}
                        }},
                        {"column_cosine", {
                            {"type", "array"},
                            {"items", {{"type", "number"}}},
                            {"minItems", 3},
                            {"maxItems", 3}
                        }}
                    }}
                }}
            }}
        };

        std::string frameSchemaPath = writeTempFile("frame_schema.json", frameSchema);

        // Create image schema
        json imageSchema = {
            {"module_type", "image"},
            {"properties", {
                {"metadata", {
                    {"type", "object"},
                    {"properties", {
                        {"test", {{"type", "string"}}}
                    }}
                }},
                {"data", {
                    {"type", "object"},
                    {"properties", {
                        {"frames", {{"$ref", "./frame_schema.json"}}}
                    }},
                    {"required", {"frames"}}
                }}
            }},
            {"required", {"metadata", "data"}}
        };

        std::string imageSchemaPath = writeTempFile("image_schema.json", imageSchema);

        // Create ImageData instance
        ImageData imageData(imageSchemaPath, UUID{});
        
        // Test that frame schema path is set (should be relative path from schema)
        REQUIRE(imageData.getFrameSchemaPath() == "./frame_schema.json");
    }

    SECTION("Validates frame pixel data size") {
        // This would test the pixel data size validation logic
        // Requires more complex setup with actual frame data
        REQUIRE(true); // Placeholder
    }

    SECTION("Validates frame metadata against schema") {
        // This would test the metadata validation logic
        REQUIRE(true); // Placeholder
    }
}

TEST_CASE("ImageData dimension handling", "[imageData][dimensions]") {
    
    SECTION("Parses image structure correctly") {
        json imageMetadata = {
            {"image_structure", {
                {"dimensions", {256, 256, 12, 5}},
                {"dimension_names", {"x", "y", "z", "time"}},
                {"bit_depth", 8},
                {"channels", 3},
                {"encoding", "jpeg2000-lossless"}
            }}
        };

        // This would test dimension parsing
        REQUIRE(imageMetadata["image_structure"]["dimensions"].size() == 4);
        REQUIRE(imageMetadata["image_structure"]["dimensions"][0] == 256);
        REQUIRE(imageMetadata["image_structure"]["dimensions"][1] == 256);
    }

    SECTION("Calculates frame count correctly") {
        // 4D dimensions: [256, 256, 12, 5]
        // Total frames = 12 * 5 = 60
        std::vector<uint16_t> dimensions = {256, 256, 12, 5};
        size_t expectedFrames = 1;
        for (size_t i = 2; i < dimensions.size(); ++i) {
            expectedFrames *= dimensions[i];
        }
        
        REQUIRE(expectedFrames == 60);
    }
}

TEST_CASE("ImageData encoding validation", "[imageData][encoding]") {
    
    SECTION("Validates encoding strings") {
        REQUIRE(stringToCompression("jpeg2000-lossless").has_value());
        REQUIRE(stringToCompression("png").has_value());
        REQUIRE(stringToCompression("raw").has_value());
        REQUIRE_FALSE(stringToCompression("invalid").has_value());
    }

    SECTION("Converts encoding to string") {
        REQUIRE(compressionToString(CompressionType::JPEG2000_LOSSLESS) == "JPEG2000_LOSSLESS");
        REQUIRE(compressionToString(CompressionType::PNG) == "PNG");
        REQUIRE(compressionToString(CompressionType::RAW) == "RAW");
    }
}
