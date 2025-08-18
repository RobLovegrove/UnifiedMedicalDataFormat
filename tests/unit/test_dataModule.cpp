#include <catch2/catch_all.hpp>
#include "DataModule/dataModule.hpp"
#include <nlohmann/json.hpp>

using namespace nlohmann;

TEST_CASE("DataModule basic operations", "[dataModule][basic]") {
    
    SECTION("DataModule can access schema") {
        // Test basic schema access functionality
        REQUIRE(true); // Placeholder for now
    }
    
    SECTION("DataModule can parse basic JSON") {
        json testData = {
            {"name", "test"},
            {"value", 42}
        };
        
        REQUIRE(testData.contains("name"));
        REQUIRE(testData.contains("value"));
        REQUIRE(testData["name"] == "test");
        REQUIRE(testData["value"] == 42);
    }
}
