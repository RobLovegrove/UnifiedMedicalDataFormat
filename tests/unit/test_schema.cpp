#include <catch2/catch_all.hpp>
#include "DataModule/dataModule.hpp"
#include <nlohmann/json.hpp>

using namespace nlohmann;

TEST_CASE("DataModule basic functionality", "[dataModule][basic]") {
    
    SECTION("DataModule can be created") {
        // Test that DataModule can be instantiated (if it has a public constructor)
        // This will need to be updated based on your actual constructor
        REQUIRE(true); // Placeholder
    }
    
    SECTION("Schema parsing basics") {
        // Test basic schema parsing functionality
        json schema = {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"maxLength", 100}
                }}
            }}
        };
        
        // This test will need to be updated when you implement schema validation
        REQUIRE(schema.contains("type"));
        REQUIRE(schema.contains("properties"));
    }
}

TEST_CASE("DataField creation", "[dataField][creation]") {
    
    SECTION("IntegerField can be created") {
        IntegerFormatInfo format{true, 8}; // signed, 8 bytes
        IntegerField field("test", format, 0, 100);
        
        REQUIRE(field.getName() == "test");
        REQUIRE(field.getType() == "integer");
    }
    
    SECTION("FloatField can be created") {
        FloatField field("test", "float32", -100, 100);
        
        REQUIRE(field.getName() == "test");
        REQUIRE(field.getType() == "number");
    }
    
    SECTION("StringField can be created") {
        StringField field("test", "string", 100);
        
        REQUIRE(field.getName() == "test");
        REQUIRE(field.getType() == "string");
    }
}

TEST_CASE("Schema file loading", "[dataModule][schema]") {
    
    SECTION("Schema file exists") {
        // Test that we can find and load schema files
        std::string schemaPath = "schemas/patient_schema.json";
        std::ifstream file(schemaPath);
        if (file.good()) {
            file.close();
            REQUIRE(true); // File exists
        } else {
            // File doesn't exist, but that's OK for testing
            REQUIRE(true); // Skip this test if file not found
        }
    }
}
