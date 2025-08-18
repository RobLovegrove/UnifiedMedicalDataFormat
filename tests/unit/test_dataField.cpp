#include <catch2/catch_all.hpp>
#include "DataModule/dataField.hpp"
#include <nlohmann/json.hpp>

using namespace nlohmann;

TEST_CASE("IntegerField validation", "[dataField][validation]") {
    
    SECTION("Valid integer within range") {
        IntegerFormatInfo format{true, 8}; // signed, 8 bytes
        IntegerField field("test", format, 0, 100);
        
        json value = 50;
        REQUIRE(field.validateValue(value) == true);
    }
    
    SECTION("Integer at boundary values") {
        IntegerFormatInfo format{true, 8};
        IntegerField field("test", format, 0, 100);
        
        json minValue = 0;
        json maxValue = 100;
        
        REQUIRE(field.validateValue(minValue) == true);
        REQUIRE(field.validateValue(maxValue) == true);
    }
    
    SECTION("Integer outside range") {
        IntegerFormatInfo format{true, 8};
        IntegerField field("test", format, 0, 100);
        
        json belowMin = -1;
        json aboveMax = 101;
        
        REQUIRE(field.validateValue(belowMin) == false);
        REQUIRE(field.validateValue(aboveMax) == false);
    }
    
    SECTION("Integer with no constraints") {
        IntegerFormatInfo format{true, 8};
        IntegerField field("test", format, std::nullopt, std::nullopt);
        
        json value = 999999;
        REQUIRE(field.validateValue(value) == true);
    }
    
    SECTION("Unsigned integer validation") {
        IntegerFormatInfo format{false, 8}; // unsigned, 8 bytes
        IntegerField field("test", format, 0, 255);
        
        json negativeValue = -1;
        json validValue = 128;
        
        REQUIRE(field.validateValue(negativeValue) == false);
        REQUIRE(field.validateValue(validValue) == true);
    }
}

TEST_CASE("FloatField validation", "[dataField][validation]") {
    
    SECTION("Valid float within range") {
        FloatField field("test", "float32", -100, 100);
        
        json value = 50.5;
        REQUIRE(field.validateValue(value) == true);
    }
    
    SECTION("Float at boundary values") {
        FloatField field("test", "float32", -100, 100);
        
        json minValue = -100.0;
        json maxValue = 100.0;
        
        REQUIRE(field.validateValue(minValue) == true);
        REQUIRE(field.validateValue(maxValue) == true);
    }
    
    SECTION("Float outside range") {
        FloatField field("test", "float32", -100, 100);
        
        json belowMin = -101.0;
        json aboveMax = 101.0;
        
        REQUIRE(field.validateValue(belowMin) == false);
        REQUIRE(field.validateValue(aboveMax) == false);
    }
    
    SECTION("Float with no constraints") {
        FloatField field("test", "float32", std::nullopt, std::nullopt);
        
        json value = 999999.999;
        REQUIRE(field.validateValue(value) == true);
    }
}

TEST_CASE("StringField validation", "[dataField][validation]") {
    
    SECTION("Valid string") {
        StringField field("test", "string", 100);
        
        json value = "Hello World";
        REQUIRE(field.validateValue(value) == true);
    }
    
    SECTION("Invalid type") {
        StringField field("test", "string", 100);
        
        json value = 42;
        REQUIRE(field.validateValue(value) == false);
    }
}

TEST_CASE("EnumField validation", "[dataField][validation]") {
    
    SECTION("Valid enum value") {
        std::vector<std::string> options = {"red", "green", "blue"};
        EnumField field("test", options);
        
        json value = "red";
        REQUIRE(field.validateValue(value) == true);
    }
    
    SECTION("Invalid enum value") {
        std::vector<std::string> options = {"red", "green", "blue"};
        EnumField field("test", options);
        
        json value = "yellow";
        REQUIRE(field.validateValue(value) == false);
    }
}
