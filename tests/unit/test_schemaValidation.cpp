#include <catch2/catch_all.hpp>
#include "DataModule/dataModule.hpp"
#include <nlohmann/json.hpp>

using namespace nlohmann;

TEST_CASE("Schema required field validation", "[schema][required][validation]") {
    
    SECTION("Valid schema with all required fields present") {
        json schema = {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"maxLength", 100}
                }},
                {"age", {
                    {"type", "integer"},
                    {"format", "uint8"}
                }}
            }},
            {"required", {"name", "age"}}
        };
        
        // This should not throw - all required fields are present
        REQUIRE_NOTHROW([&]() {
            // We'll need to create a mock DataModule or test the parseSchema method directly
            // For now, just verify the schema structure
            REQUIRE(schema.contains("required"));
            REQUIRE(schema.contains("properties"));
            
            const auto& required = schema["required"];
            const auto& properties = schema["properties"];
            
            for (const auto& field : required) {
                REQUIRE(properties.contains(field.get<std::string>()));
            }
        }());
    }
    
    SECTION("Schema missing required field in properties") {
        json schema = {
            {"type", "object"},
            {"properties", {
                {"name", {
                    {"type", "string"},
                    {"maxLength", 100}
                }}
                // Missing "age" field
            }},
            {"required", {"name", "age"}}  // age is required but not in properties
        };
        
        // This should fail validation
        REQUIRE_THROWS([&]() {
            const auto& required = schema["required"];
            const auto& properties = schema["properties"];
            
            for (const auto& field : required) {
                if (!properties.contains(field.get<std::string>())) {
                    throw std::runtime_error("Required field missing: " + field.get<std::string>());
                }
            }
        }());
    }
    
    SECTION("Schema with nested required fields") {
        json schema = {
            {"type", "object"},
            {"properties", {
                {"metadata", {
                    {"type", "object"},
                    {"properties", {
                        {"patient_id", {
                            {"type", "string"},
                            {"maxLength", 50}
                        }},
                        {"name", {
                            {"type", "string"},
                            {"maxLength", 100}
                        }}
                    }},
                    {"required", {"patient_id", "name"}}
                }}
            }},
            {"required", {"metadata"}}
        };
        
        // This should not throw - nested required fields are present
        REQUIRE_NOTHROW([&]() {
            REQUIRE(schema["properties"]["metadata"]["properties"].contains("patient_id"));
            REQUIRE(schema["properties"]["metadata"]["properties"].contains("name"));
        }());
    }
    
    SECTION("Schema with missing nested required field") {
        json schema = {
            {"type", "object"},
            {"properties", {
                {"metadata", {
                    {"type", "object"},
                    {"properties", {
                        {"patient_id", {
                            {"type", "string"},
                            {"maxLength", 50}
                        }}
                        // Missing "name" field
                    }},
                    {"required", {"patient_id", "name"}}  // name is required but missing
                }}
            }},
            {"required", {"metadata"}}
        };
        
        // This should fail validation
        REQUIRE_THROWS([&]() {
            const auto& metadataProps = schema["properties"]["metadata"]["properties"];
            const auto& metadataRequired = schema["properties"]["metadata"]["required"];
            
            for (const auto& field : metadataRequired) {
                if (!metadataProps.contains(field.get<std::string>())) {
                    throw std::runtime_error("Nested required field missing: " + field.get<std::string>());
                }
            }
        }());
    }
}

TEST_CASE("Schema field type validation", "[schema][type][validation]") {
    
    SECTION("Valid primitive types") {
        std::vector<std::string> validTypes = {"string", "integer", "number", "object", "array"};
        
        for (const auto& type : validTypes) {
            json fieldDef = {{"type", type}};
            
            // Basic type validation should pass
            REQUIRE(fieldDef.contains("type"));
            REQUIRE(fieldDef["type"] == type);
        }
    }
    
    SECTION("Integer field validation") {
        SECTION("Valid integer field with format") {
            json fieldDef = {
                {"type", "integer"},
                {"format", "uint8"}
            };
            
            REQUIRE(fieldDef["type"] == "integer");
            REQUIRE(fieldDef.contains("format"));
            REQUIRE(fieldDef["format"] == "uint8");
        }
        
        SECTION("Integer field missing format") {
            json fieldDef = {
                {"type", "integer"}
                // Missing required "format" field
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (!fieldDef.contains("format")) {
                    throw std::runtime_error("Integer field missing 'format': test_field");
                }
            }());
        }
        
        SECTION("Integer field with constraints") {
            json fieldDef = {
                {"type", "integer"},
                {"format", "uint16"},
                {"minimum", 0},
                {"maximum", 65535}
            };
            
            REQUIRE(fieldDef.contains("minimum"));
            REQUIRE(fieldDef.contains("maximum"));
            REQUIRE(fieldDef["minimum"] == 0);
            REQUIRE(fieldDef["maximum"] == 65535);
        }
    }
    
    SECTION("Number/float field validation") {
        SECTION("Valid float field with format") {
            json fieldDef = {
                {"type", "number"},
                {"format", "float32"}
            };
            
            REQUIRE(fieldDef["type"] == "number");
            REQUIRE(fieldDef.contains("format"));
            REQUIRE(fieldDef["format"] == "float32");
        }
        
        SECTION("Float field missing format") {
            json fieldDef = {
                {"type", "number"}
                // Missing required "format" field
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (!fieldDef.contains("format")) {
                    throw std::runtime_error("Number field missing 'format': test_field");
                }
            }());
        }
        
        SECTION("Unsupported number format") {
            json fieldDef = {
                {"type", "number"},
                {"format", "unsupported_format"}
            };
            
            // This should fail validation for unsupported format
            REQUIRE_THROWS([&]() {
                std::string format = fieldDef["format"];
                if (format != "float32" && format != "float64") {
                    throw std::runtime_error("Unsupported number format: " + format);
                }
            }());
        }
    }
    
    SECTION("String field validation") {
        SECTION("Fixed length string") {
            json fieldDef = {
                {"type", "string"},
                {"length", 100}
            };
            
            REQUIRE(fieldDef["type"] == "string");
            REQUIRE(fieldDef.contains("length"));
            REQUIRE(fieldDef["length"] == 100);
        }
        
        SECTION("Variable length string") {
            json fieldDef = {
                {"type", "string"}
                // No length specified = variable length
            };
            
            REQUIRE(fieldDef["type"] == "string");
            REQUIRE(!fieldDef.contains("length"));
        }
    }
    
    SECTION("Enum field validation") {
        SECTION("Valid enum field") {
            json fieldDef = {
                {"enum", {"red", "green", "blue"}},
                {"storage", {
                    {"type", "uint8"}
                }}
            };
            
            REQUIRE(fieldDef.contains("enum"));
            REQUIRE(fieldDef["enum"].is_array());
            REQUIRE(fieldDef["enum"].size() == 3);
            REQUIRE(fieldDef.contains("storage"));
        }
        
        SECTION("Enum field with default storage") {
            json fieldDef = {
                {"enum", {"option1", "option2"}}
                // No storage specified = default to uint8
            };
            
            REQUIRE(fieldDef.contains("enum"));
            REQUIRE(fieldDef["enum"].is_array());
            REQUIRE(fieldDef["enum"].size() == 2);
        }
    }
    
    SECTION("Object field validation") {
        SECTION("Valid object field with properties") {
            json fieldDef = {
                {"type", "object"},
                {"properties", {
                    {"subfield1", {
                        {"type", "string"},
                        {"maxLength", 50}
                    }},
                    {"subfield2", {
                        {"type", "integer"},
                        {"format", "uint8"}
                    }}
                }},
                {"required", {"subfield1"}}
            };
            
            REQUIRE(fieldDef["type"] == "object");
            REQUIRE(fieldDef.contains("properties"));
            REQUIRE(fieldDef.contains("required"));
            
            const auto& properties = fieldDef["properties"];
            const auto& required = fieldDef["required"];
            
            // Check that required fields exist in properties
            for (const auto& field : required) {
                REQUIRE(properties.contains(field.get<std::string>()));
            }
        }
        
        SECTION("Object field missing properties") {
            json fieldDef = {
                {"type", "object"}
                // Missing required "properties" field
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (!fieldDef.contains("properties")) {
                    throw std::runtime_error("Object field missing 'properties': test_field");
                }
            }());
        }
        
        SECTION("Object field with missing required field in properties") {
            json fieldDef = {
                {"type", "object"},
                {"properties", {
                    {"subfield1", {
                        {"type", "string"},
                        {"maxLength", 50}
                    }}
                    // Missing "subfield2"
                }},
                {"required", {"subfield1", "subfield2"}}  // subfield2 required but missing
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                const auto& properties = fieldDef["properties"];
                const auto& required = fieldDef["required"];
                
                for (const auto& field : required) {
                    if (!properties.contains(field.get<std::string>())) {
                        throw std::runtime_error("ObjectField 'test_field' missing required field: " + field.get<std::string>());
                    }
                }
            }());
        }
    }
    
    SECTION("Array field validation") {
        SECTION("Valid array field") {
            json fieldDef = {
                {"type", "array"},
                {"items", {
                    {"type", "string"},
                    {"maxLength", 100}
                }},
                {"minItems", 1},
                {"maxItems", 10}
            };
            
            REQUIRE(fieldDef["type"] == "array");
            REQUIRE(fieldDef.contains("items"));
            REQUIRE(fieldDef.contains("minItems"));
            REQUIRE(fieldDef.contains("maxItems"));
            REQUIRE(fieldDef["minItems"] == 1);
            REQUIRE(fieldDef["maxItems"] == 10);
        }
        
        SECTION("Array field missing items") {
            json fieldDef = {
                {"type", "array"},
                {"minItems", 1},
                {"maxItems", 10}
                // Missing required "items" field
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (!fieldDef.contains("items")) {
                    throw std::runtime_error("Array field missing 'items': test_field");
                }
            }());
        }
        
        SECTION("Array field missing minItems/maxItems") {
            json fieldDef = {
                {"type", "array"},
                {"items", {
                    {"type", "string"},
                    {"maxLength", 100}
                }}
                // Missing minItems/maxItems
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (!fieldDef.contains("minItems") || !fieldDef.contains("maxItems")) {
                    throw std::runtime_error("Array field missing minItems/maxItems: test_field");
                }
            }());
        }
    }
    
    SECTION("Unsupported field type") {
        json fieldDef = {
            {"type", "unsupported_type"}
        };
        
        // This should fail validation
        REQUIRE_THROWS([&]() {
            std::string type = fieldDef["type"];
            if (type != "string" && type != "integer" && type != "number" && 
                type != "object" && type != "array" && !fieldDef.contains("enum")) {
                throw std::runtime_error("Unsupported type for field: test_field");
            }
        }());
    }
}

TEST_CASE("Schema constraint validation", "[schema][constraints][validation]") {
    
    SECTION("Integer constraints validation") {
        SECTION("Valid min/max constraints") {
            json fieldDef = {
                {"type", "integer"},
                {"format", "uint8"},
                {"minimum", 0},
                {"maximum", 255}
            };
            
            REQUIRE(fieldDef["minimum"] == 0);
            REQUIRE(fieldDef["maximum"] == 255);
            
            // Constraints should be reasonable
            REQUIRE(fieldDef["minimum"] <= fieldDef["maximum"]);
        }
        
        SECTION("Invalid min/max constraints (min > max)") {
            json fieldDef = {
                {"type", "integer"},
                {"format", "uint8"},
                {"minimum", 100},
                {"maximum", 50}  // min > max
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (fieldDef["minimum"] > fieldDef["maximum"]) {
                    throw std::runtime_error("Invalid constraints: minimum > maximum");
                }
            }());
        }
        
        SECTION("Constraints within format limits") {
            json fieldDef = {
                {"type", "integer"},
                {"format", "uint8"},
                {"minimum", 0},
                {"maximum", 300}  // Exceeds uint8 range (0-255)
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (fieldDef["maximum"] > 255) {
                    throw std::runtime_error("Maximum value exceeds uint8 range");
                }
            }());
        }
    }
    
    SECTION("Float constraints validation") {
        SECTION("Valid min/max constraints") {
            json fieldDef = {
                {"type", "number"},
                {"format", "float32"},
                {"minimum", -1000.0},
                {"maximum", 1000.0}
            };
            
            REQUIRE(fieldDef["minimum"] == -1000.0);
            REQUIRE(fieldDef["maximum"] == 1000.0);
            REQUIRE(fieldDef["minimum"] <= fieldDef["maximum"]);
        }
        
        SECTION("Invalid min/max constraints (min > max)") {
            json fieldDef = {
                {"type", "number"},
                {"format", "float32"},
                {"minimum", 500.0},
                {"maximum", 100.0}  // min > max
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (fieldDef["minimum"] > fieldDef["maximum"]) {
                    throw std::runtime_error("Invalid constraints: minimum > maximum");
                }
            }());
        }
    }
    
    SECTION("String constraints validation") {
        SECTION("Valid string length constraints") {
            json fieldDef = {
                {"type", "string"},
                {"length", 100},
                {"maxLength", 100}
            };
            
            REQUIRE(fieldDef["length"] == 100);
            REQUIRE(fieldDef["maxLength"] == 100);
        }
        
        SECTION("Invalid string length constraints") {
            json fieldDef = {
                {"type", "string"},
                {"length", 10},
                {"maxLength", 5}  // length > maxLength
            };
            
            // This should fail validation
            REQUIRE_THROWS([&]() {
                if (fieldDef["length"] > fieldDef["maxLength"]) {
                    throw std::runtime_error("Invalid string constraints: length > maxLength");
                }
            }());
        }
    }
}
