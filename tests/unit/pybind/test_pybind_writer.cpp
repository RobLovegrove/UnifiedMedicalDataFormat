#include <catch2/catch_test_macros.hpp>
#include "pybind_test_fixture.hpp"
#include <iostream>
#include <filesystem>
#include <cstdlib> // For std::time

namespace py = pybind11;

TEST_CASE("Pybind Writer class functionality", "[pybind][writer]") {
    std::cout << "DEBUG: Testing Writer class functionality..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    std::cout << "DEBUG: Module reference obtained successfully" << std::endl;
    
    // Test Writer class exists
    REQUIRE_NOTHROW(module.attr("Writer"));
    std::cout << "DEBUG: Writer class exists" << std::endl;
    
    // Test Writer instantiation
    auto writerClass = module.attr("Writer");
    REQUIRE(writerClass.ptr() != nullptr);
    std::cout << "DEBUG: Writer class reference obtained" << std::endl;
    
    auto writer = writerClass();
    REQUIRE(writer.ptr() != nullptr);
    std::cout << "DEBUG: Writer instance created successfully" << std::endl;
    
    // Test createNewFile method exists
    REQUIRE_NOTHROW(writer.attr("createNewFile"));
    std::cout << "DEBUG: createNewFile method exists" << std::endl;
    
    // Test createNewEncounter method exists
    REQUIRE_NOTHROW(writer.attr("createNewEncounter"));
    std::cout << "DEBUG: createNewEncounter method exists" << std::endl;
    
    // Test closeFile method exists
    REQUIRE_NOTHROW(writer.attr("closeFile"));
    std::cout << "DEBUG: closeFile method exists" << std::endl;
    
    std::cout << "DEBUG: Writer class test completed successfully!" << std::endl;
}

TEST_CASE("Writer createNewFile basic functionality", "[pybind][writer][createNewFile]") {
    std::cout << "DEBUG: Testing basic createNewFile functionality..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer instance
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Use a unique filename for this test
    std::string filename = "test_basic_file_" + std::to_string(std::time(nullptr)) + ".umdf";
    
    // Test creating a new file
    std::cout << "DEBUG: About to create new file: " << filename << std::endl;
    auto result = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    std::cout << "DEBUG: createNewFile called successfully" << std::endl;
    
    // Verify the result structure
    REQUIRE_NOTHROW(result.attr("success"));
    REQUIRE_NOTHROW(result.attr("message"));
    
    bool success = result.attr("success").cast<bool>();
    std::string message = result.attr("message").cast<std::string>();
    
    std::cout << "DEBUG: createNewFile result - success: " << (success ? "true" : "false") 
              << ", message: '" << message << "'" << std::endl;
    
    // Should succeed
    REQUIRE(success == true);
    
    // Create an encounter and add a module so the file gets finalized
    std::cout << "DEBUG: Creating encounter and adding module..." << std::endl;
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    
    auto encounterId = encounterResult.attr("value")();
    
    // Create a ModuleData object
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    
    // Close the file - it should be finalized since we have a module
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    
    // Test that the file was created and finalized
    REQUIRE(std::filesystem::exists(filename));
    REQUIRE_FALSE(std::filesystem::exists(filename + ".tmp"));
    std::cout << "DEBUG: File created and finalized successfully" << std::endl;
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: createNewFile test completed successfully!" << std::endl;
}

TEST_CASE("Writer createNewEncounter functionality", "[pybind][writer][createNewEncounter]") {
    std::cout << "DEBUG: Testing createNewEncounter functionality..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer instance
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a new file first
    std::string filename = "test_encounter_" + std::to_string(std::time(nullptr)) + ".umdf";
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File created successfully: " << filename << std::endl;
    
    // Test creating a new encounter
    std::cout << "DEBUG: About to create new encounter..." << std::endl;
    auto encounterResult = writer.attr("createNewEncounter")();
    std::cout << "DEBUG: createNewEncounter called successfully" << std::endl;
    
    // Verify the result structure
    REQUIRE_NOTHROW(encounterResult.attr("has_value"));
    REQUIRE_NOTHROW(encounterResult.attr("value"));
    
    bool hasValue = encounterResult.attr("has_value")().cast<bool>();
    REQUIRE(hasValue == true);
    
    auto encounterId = encounterResult.attr("value")();
    REQUIRE(encounterId.ptr() != nullptr);
    
    std::cout << "DEBUG: Encounter created successfully" << std::endl;
    std::cout << "DEBUG: Encounter ID: " << encounterId.attr("toString")().cast<std::string>() << std::endl;
    
    // Add a minimal module to the encounter so the file gets finalized
    std::cout << "DEBUG: Adding minimal module to encounter..." << std::endl;
    
    // Create a ModuleData object
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter (this will populate the xrefTable)
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    std::cout << "DEBUG: Module added to encounter successfully" << std::endl;
    
    // Now close the file - it should be finalized since we have a module
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File closed and finalized successfully" << std::endl;
    
    // Verify the file was created and finalized
    REQUIRE(std::filesystem::exists(filename));
    REQUIRE_FALSE(std::filesystem::exists(filename + ".tmp"));
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: createNewEncounter test completed successfully!" << std::endl;
}

TEST_CASE("Writer complete workflow test", "[pybind][writer][workflow]") {
    std::cout << "DEBUG: Testing complete Writer workflow..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer instance
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a new file
    std::string filename = "test_workflow_" + std::to_string(std::time(nullptr)) + ".umdf";
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File created successfully: " << filename << std::endl;
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    std::cout << "DEBUG: Encounter created successfully" << std::endl;
    
    auto encounterId = encounterResult.attr("value")();
    std::cout << "DEBUG: Encounter ID: " << encounterId.attr("toString")().cast<std::string>() << std::endl;
    
    // Test adding a module to the encounter
    std::cout << "DEBUG: About to add module to encounter..." << std::endl;
    
    // Create a ModuleData object
    std::cout << "DEBUG: Creating ModuleData object..." << std::endl;
    
    // Check if ModuleData class exists in module
    REQUIRE_NOTHROW(module.attr("ModuleData"));
    std::cout << "DEBUG: ModuleData class exists" << std::endl;
    
    auto moduleDataClass = module.attr("ModuleData");
    REQUIRE(moduleDataClass.ptr() != nullptr);
    std::cout << "DEBUG: ModuleData class reference obtained successfully!" << std::endl;
    std::cout << "DEBUG: ModuleData class ptr: " << moduleDataClass.ptr() << std::endl;
    
    // Create ModuleData instance
    std::cout << "DEBUG: About to call ModuleData constructor..." << std::endl;
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    std::cout << "DEBUG: ModuleData object created successfully" << std::endl;
    
    // Set some metadata
    std::cout << "DEBUG: Setting metadata..." << std::endl;
    
    // Use a simple string for metadata instead of complex JSON for now
    std::string simpleMetadata = "{\"test_key\": \"test_value\"}";
    
    // Set the metadata using the set_metadata method
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    std::cout << "DEBUG: Metadata set successfully" << std::endl;
    
    // Add the module to the encounter so the file gets finalized
    std::cout << "DEBUG: Adding module to encounter..." << std::endl;
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    std::cout << "DEBUG: Module added to encounter successfully" << std::endl;
    
    // Close the file
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File closed successfully" << std::endl;
    
    // Verify the file was created and finalized
    REQUIRE(std::filesystem::exists(filename));
    REQUIRE_FALSE(std::filesystem::exists(filename + ".tmp"));
    REQUIRE(std::filesystem::file_size(filename) > 0);
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: Complete workflow test completed successfully!" << std::endl;
}
