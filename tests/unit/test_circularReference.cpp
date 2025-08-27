#include <catch2/catch_all.hpp>
#include "DataModule/dataModule.hpp"
#include "DataModule/Tabular/tabularData.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace nlohmann;
namespace fs = std::filesystem;

TEST_CASE("Circular reference detection during schema parsing", "[circular][parsing]") {
    
    SECTION("Detects circular reference when parsing schema with $ref") {
        // Create a schema that references itself
        json selfRefSchema = {
            {"type", "object"},
            {"module_type", "tabular"},
            {"properties", {
                {"metadata", {
                    {"type", "object"},
                    {"properties", {
                        {"name", {{"type", "string"}, {"length", 32}}}
                    }}
                }},
                {"data", {
                    {"type", "object"},
                    {"properties", {
                        {"self", {{"$ref", "./self_ref.json"}}}
                    }},
                    {"required", {"self"}}
                }}
            }}
        };
        
        fs::path testDir = "build/tests_tmp/circular_parsing_test";
        fs::create_directories(testDir);
        
        std::string schemaPath = (testDir / "self_ref.json").string();
        std::ofstream out(schemaPath);
        out << selfRefSchema.dump(2);
        out.close();
        
        // This should throw due to circular reference when TabularData tries to parse the schema
        REQUIRE_THROWS_AS(
            TabularData(schemaPath, UUID(), EncryptionData{}),
            std::runtime_error
        );
        
        // Verify the error message contains circular reference information
        try {
            TabularData(schemaPath, UUID(), EncryptionData{});
        } catch (const std::runtime_error& e) {
            std::string errorMsg = e.what();
            std::cout << "Actual error message: " << errorMsg << std::endl;
            REQUIRE(errorMsg.find("Circular reference detected") != std::string::npos);
        }
    }
    
    SECTION("Detects indirect circular reference during schema parsing") {
        // Create chain: A -> B -> C -> A
        json schemaA = {
            {"type", "object"},
            {"module_type", "tabular"},
            {"properties", {
                {"metadata", {
                    {"type", "object"},
                    {"properties", {
                        {"name", {{"type", "string"}, {"length", 32}}}
                    }}
                }},
                {"data", {
                    {"type", "object"},
                    {"properties", {
                        {"ref", {{"$ref", "./schemaB.json"}}}
                    }},
                    {"required", {"ref"}}
                }}
            }}
        };
        
        json schemaB = {
            {"type", "object"},
            {"properties", {
                {"ref", {{"$ref", "./schemaC.json"}}}
            }}
        };
        
        json schemaC = {
            {"type", "object"},
            {"properties", {
                {"ref", {{"$ref", "./schemaA.json"}}}
            }}
        };
        
        fs::path testDir = "build/tests_tmp/circular_parsing_test";
        fs::create_directories(testDir);
        
        // Write all schemas
        std::ofstream outA((testDir / "schemaA.json").string());
        outA << schemaA.dump(2); outA.close();
        std::ofstream outB((testDir / "schemaB.json").string());
        outB << schemaB.dump(2); outB.close();
        std::ofstream outC((testDir / "schemaC.json").string());
        outC << schemaC.dump(2); outC.close();
        
        // This should throw due to circular reference chain
        REQUIRE_THROWS_AS(
            TabularData((testDir / "schemaA.json").string(), UUID(), EncryptionData{}),
            std::runtime_error
        );
        
        // Verify the error message contains circular reference information
        try {
            TabularData((testDir / "schemaA.json").string(), UUID(), EncryptionData{});
        } catch (const std::runtime_error& e) {
            std::string errorMsg = e.what();
            std::cout << "Actual error message: " << errorMsg << std::endl;
            REQUIRE(errorMsg.find("Circular reference detected") != std::string::npos);
        }
    }
}
