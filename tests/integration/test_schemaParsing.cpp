#include <catch2/catch_all.hpp>
#include "DataModule/Tabular/tabularData.hpp"
#include "Utility/uuid.hpp"
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

TEST_CASE("TabularData parses schema from file and enforces required fields", "[integration][schema][tabular]") {
    // Minimal tabular schema with required metadata and data fields
    json schema = {
        {"module_type", "tabular"},
        {"properties", {
            {"metadata", {
                {"type", "object"},
                {"properties", {
                    {"patient_id", {{"type", "string"}, {"length", 16}}},
                    {"name", {{"type", "string"}}}
                }},
                {"required", {"patient_id", "name"}}
            }},
            {"data", {
                {"type", "object"},
                {"properties", {
                    {"age", {{"type", "integer"}, {"format", "uint8"}, {"minimum", 0}, {"maximum", 120}}},
                    {"height_cm", {{"type", "number"}, {"format", "float32"}}}
                }},
                {"required", {"age"}}
            }}
        }}
    };

    std::string schemaPath = writeTempFile("tabular_schema.json", schema);

    // Construct TabularData with real file path -> triggers file IO and parse
    REQUIRE_NOTHROW([&]() {
        TabularData td(schemaPath, UUID{}, EncryptionData{});
    }());

    // Now use the instance to validate metadata and data enforcement
    TabularData td(schemaPath, UUID{}, EncryptionData{});

    SECTION("Valid metadata + data passes") {
        json meta = {{"patient_id", "P0001"}, {"name", "Jane Doe"}};
        json dataRow = {{"age", 30}, {"height_cm", 165.5}};
        REQUIRE_NOTHROW(td.addMetaData(meta));
        REQUIRE_NOTHROW(td.addData(dataRow));
    }

    SECTION("Missing required metadata fails") {
        json meta = {{"patient_id", "P0001"}}; // missing name
        REQUIRE_THROWS(td.addMetaData(meta));
    }

    SECTION("Missing required data field fails") {
        json meta = {{"patient_id", "P0001"}, {"name", "Jane Doe"}};
        json dataRow = {{"height_cm", 165.5}}; // missing age
        td.addMetaData(meta);
        REQUIRE_THROWS(td.addData(dataRow));
    }

    SECTION("Type mismatch fails (string instead of integer)") {
        json meta = {{"patient_id", "P0001"}, {"name", "Jane Doe"}};
        json dataRow = {{"age", "thirty"}, {"height_cm", 165.5}};
        td.addMetaData(meta);
        REQUIRE_THROWS(td.addData(dataRow));
    }
}

TEST_CASE("TabularData resolves $ref in schema from file", "[integration][schema][ref]") {
    // Referenced definition for a small object
    json refDef = {
        {"type", "object"},
        {"properties", {
            {"x", {{"type", "integer"}, {"format", "uint8"}}},
            {"y", {{"type", "integer"}, {"format", "uint8"}}}
        }},
        {"required", {"x", "y"}}
    };

    // Main schema uses $ref to include the object into data
    json mainSchema = {
        {"module_type", "tabular"},
        {"properties", {
            {"metadata", {
                {"type", "object"},
                {"properties", {{"id", {{"type", "string"}}}}},
                {"required", {"id"}}
            }},
            {"data", {
                {"type", "object"},
                {"properties", {{"point", {{"$ref", "./ref_point.json"}}}}},
                {"required", {"point"}}
            }}
        }}
    };

    std::string baseDir = "build/tests_tmp/ref_case";
    fs::create_directories(baseDir);
    std::string refPath = writeTempFile("ref_case/ref_point.json", refDef);
    std::string mainPath = writeTempFile("ref_case/main_schema.json", mainSchema);

    // Instantiate -> should resolve $ref relative to main schema path
    TabularData td(mainPath, UUID{}, EncryptionData{});

    // Valid case
    json meta = {{"id", "A1"}};
    json dataRow = {{"point", {{"x", 5}, {"y", 7}}}};
    td.addMetaData(meta);
    REQUIRE_NOTHROW(td.addData(dataRow));

    // Missing nested required field -> should fail
    json badRow = {{"point", {{"x", 5}}}}; // missing y
    REQUIRE_THROWS(td.addData(badRow));
}
