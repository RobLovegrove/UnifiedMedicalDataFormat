#include <catch2/catch_test_macros.hpp>
#include "pybind_test_fixture.hpp"
#include <iostream>
#include <filesystem>
#include <cstdlib> // For std::time

namespace py = pybind11;

TEST_CASE("Pybind Reader class functionality", "[pybind][reader]") {
    std::cout << "DEBUG: Testing Reader class functionality..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    std::cout << "DEBUG: Module reference obtained successfully" << std::endl;
    
    // Test Reader class exists
    REQUIRE_NOTHROW(module.attr("Reader"));
    std::cout << "DEBUG: Reader class exists" << std::endl;
    
    // Test Reader instantiation
    auto readerClass = module.attr("Reader");
    REQUIRE(readerClass.ptr() != nullptr);
    std::cout << "DEBUG: Reader class reference obtained" << std::endl;
    
    auto reader = readerClass();
    REQUIRE(reader.ptr() != nullptr);
    std::cout << "DEBUG: Reader instance created successfully" << std::endl;
    
    // Test openFile method exists
    REQUIRE_NOTHROW(reader.attr("openFile"));
    std::cout << "DEBUG: openFile method exists" << std::endl;
    
    // Test getFileInfo method exists
    REQUIRE_NOTHROW(reader.attr("getFileInfo"));
    std::cout << "DEBUG: getFileInfo method exists" << std::endl;
    
    std::cout << "DEBUG: Reader class test completed successfully!" << std::endl;
}

TEST_CASE("Reader openFile method functionality", "[pybind][reader][openFile]") {
    std::cout << "DEBUG: Testing Reader openFile method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Reader instance
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    // Test openFile with non-existent file
    std::cout << "DEBUG: About to call openFile..." << std::endl;
    auto result = reader.attr("openFile")("non_existent_file.umdf", "testpassword");
    std::cout << "DEBUG: openFile called successfully, result ptr: " << result.ptr() << std::endl;
    
    // Verify the result structure
    REQUIRE_NOTHROW(result.attr("success"));
    REQUIRE_NOTHROW(result.attr("message"));
    std::cout << "DEBUG: Attributes exist, getting values..." << std::endl;
    
    bool success = result.attr("success").cast<bool>();
    std::string message = result.attr("message").cast<std::string>();
    
    std::cout << "DEBUG: openFile result - success: " << (success ? "true" : "false") 
              << ", message: '" << message << "'" << std::endl;
    
    // Should fail since file doesn't exist
    REQUIRE(success == false);
    REQUIRE(message.find("Failed to open file") != std::string::npos);
    
    std::cout << "DEBUG: All assertions passed!" << std::endl;
}

TEST_CASE("Reader getFileInfo method functionality", "[pybind][reader][getFileInfo]") {
    std::cout << "DEBUG: Testing getFileInfo method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Reader instance
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    // Test getFileInfo method exists
    REQUIRE_NOTHROW(reader.attr("getFileInfo"));
    std::cout << "DEBUG: getFileInfo method exists" << std::endl;
    
    // Test calling getFileInfo (should fail since no file is open)
    std::cout << "DEBUG: About to call getFileInfo..." << std::endl;
    auto result = reader.attr("getFileInfo")();
    std::cout << "DEBUG: getFileInfo called successfully!" << std::endl;
    
    // getFileInfo returns a Json object, not a Result object
    // We need to check if it contains an error message
    // Instead of using contains(), let's try to access the fields directly
    try {
        // Try to access the success field first
        bool success = result.attr("__getitem__")("success").cast<bool>();
        REQUIRE(success == false); // Should be false when no file is open
        
        // Now try to get the error message
        std::string error = result.attr("__getitem__")("error").cast<std::string>();
        std::cout << "DEBUG: getFileInfo error: '" << error << "'" << std::endl;
        
        // Should contain an error message since no file is open
        REQUIRE_FALSE(error.empty());
        REQUIRE(error.find("No file is currently open") != std::string::npos);
    } catch (const std::exception& e) {
        // If we can't access the fields, the test should fail
        FAIL("Could not access fields from getFileInfo result: " << e.what());
    }
    
    std::cout << "DEBUG: getFileInfo test completed successfully!" << std::endl;
}

TEST_CASE("Reader getFileInfo method functionality with open file", "[pybind][reader][getFileInfo][withFile]") {
    std::cout << "DEBUG: Testing getFileInfo with open file..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer to create a test file
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a test file
    std::string filename = "test_reader_file_" + std::to_string(std::time(nullptr)) + ".umdf";
    std::cout << "DEBUG: Creating new file: " << filename << std::endl;
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File created successfully" << std::endl;
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    std::cout << "DEBUG: Encounter created successfully" << std::endl;
    
    auto encounterId = encounterResult.attr("value")();
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
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    std::cout << "DEBUG: Module added to encounter successfully" << std::endl;
    
    // Close the file
    std::cout << "DEBUG: Closing file..." << std::endl;
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    std::cout << "DEBUG: File closed successfully" << std::endl;
    
    // Now test Reader with the created file
    std::cout << "DEBUG: Opening file with reader..." << std::endl;
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    auto openResult = reader.attr("openFile")(filename, "testpassword");
    std::cout << "DEBUG: openFile result - success: " << openResult.attr("success").cast<bool>() << std::endl;
    
    // This should succeed now
    REQUIRE(openResult.attr("success").cast<bool>() == true);
    
    // Test getFileInfo
    auto fileInfoResult = reader.attr("getFileInfo")();
    REQUIRE(fileInfoResult.ptr() != nullptr);
    
    // Check if it contains success field by accessing it directly
    try {
        bool success = fileInfoResult.attr("__getitem__")("success").cast<bool>();
        REQUIRE(success == true); // Should be true when file is open
    } catch (const std::exception& e) {
        FAIL("Could not access success field from getFileInfo result: " << e.what());
    }
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: getFileInfo test completed" << std::endl;
}
