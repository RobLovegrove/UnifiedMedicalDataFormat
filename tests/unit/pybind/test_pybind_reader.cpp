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

TEST_CASE("Reader getModuleData method functionality", "[pybind][reader][getModuleData]") {
    std::cout << "DEBUG: Testing getModuleData method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer to create a test file
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a test file
    std::string filename = "test_getModuleData_" + std::to_string(std::time(nullptr)) + ".umdf";
    std::cout << "DEBUG: Creating new file: " << filename << std::endl;
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    
    auto encounterId = encounterResult.attr("value")();
    
    // Add a module to the encounter
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\", \"module_type\": \"test\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    
    auto moduleId = addModuleResult.attr("value")();
    std::string moduleIdStr = moduleId.attr("toString")().cast<std::string>();
    
    // Close the file
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    
    // Now test Reader getModuleData
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    auto openResult = reader.attr("openFile")(filename, "testpassword");
    REQUIRE(openResult.attr("success").cast<bool>() == true);
    
    // Test getModuleData with the module ID
    std::cout << "DEBUG: Testing getModuleData with module ID: " << moduleIdStr << std::endl;
    auto retrievedModuleData = reader.attr("getModuleData")(moduleIdStr);
    
    // getModuleData returns an optional ModuleData, so we need to check if it has a value
    REQUIRE_NOTHROW(retrievedModuleData.attr("has_value"));
    bool hasValue = retrievedModuleData.attr("has_value")().cast<bool>();
    REQUIRE(hasValue == true);
    
    if (hasValue) {
        auto actualModuleData = retrievedModuleData.attr("value")();
        REQUIRE(actualModuleData.ptr() != nullptr);
        
        // Test that we can access the metadata
        auto metadata = actualModuleData.attr("get_metadata")();
        REQUIRE(metadata.ptr() != nullptr);
        
        std::cout << "DEBUG: Successfully retrieved module data" << std::endl;
    }
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: getModuleData test completed successfully!" << std::endl;
}

TEST_CASE("Reader getAuditTrail method functionality", "[pybind][reader][getAuditTrail]") {
    std::cout << "DEBUG: Testing getAuditTrail method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer to create a test file
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a test file
    std::string filename = "test_getAuditTrail_" + std::to_string(std::time(nullptr)) + ".umdf";
    std::cout << "DEBUG: Creating new file: " << filename << std::endl;
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    
    auto encounterId = encounterResult.attr("value")();
    
    // Add a module to the encounter
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\", \"module_type\": \"test\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    
    auto moduleId = addModuleResult.attr("value")();
    std::string moduleIdStr = moduleId.attr("toString")().cast<std::string>();
    
    // Close the file
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    
    // Now test Reader getAuditTrail
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    auto openResult = reader.attr("openFile")(filename, "testpassword");
    REQUIRE(openResult.attr("success").cast<bool>() == true);
    
    // Test getAuditTrail with the module ID
    std::cout << "DEBUG: Testing getAuditTrail with module ID: " << moduleIdStr << std::endl;
    auto auditTrailResult = reader.attr("getAuditTrail")(moduleId);
    
    // getAuditTrail returns an expected<ModuleTrail>, so we need to check if it has a value
    REQUIRE_NOTHROW(auditTrailResult.attr("has_value"));
    bool hasValue = auditTrailResult.attr("has_value")().cast<bool>();
    REQUIRE(hasValue == true);
    
    if (hasValue) {
        auto auditTrail = auditTrailResult.attr("value")();
        REQUIRE(auditTrail.ptr() != nullptr);
        
        std::cout << "DEBUG: Successfully retrieved audit trail" << std::endl;
    }
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: getAuditTrail test completed successfully!" << std::endl;
}

TEST_CASE("Reader getAuditData method functionality", "[pybind][reader][getAuditData]") {
    std::cout << "DEBUG: Testing getAuditData method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer to create a test file
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a test file
    std::string filename = "test_getAuditData_" + std::to_string(std::time(nullptr)) + ".umdf";
    std::cout << "DEBUG: Creating new file: " << filename << std::endl;
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    
    auto encounterId = encounterResult.attr("value")();
    
    // Add a module to the encounter
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\", \"module_type\": \"test\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    
    auto moduleId = addModuleResult.attr("value")();
    std::string moduleIdStr = moduleId.attr("toString")().cast<std::string>();
    
    // Close the file
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    
    // Now test Reader getAuditData
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    auto openResult = reader.attr("openFile")(filename, "testpassword");
    REQUIRE(openResult.attr("success").cast<bool>() == true);
    
    // First get the audit trail to get a ModuleTrail object
    auto auditTrailResult = reader.attr("getAuditTrail")(moduleId);
    REQUIRE(auditTrailResult.attr("has_value")().cast<bool>() == true);
    
    auto auditTrailVector = auditTrailResult.attr("value")();
    REQUIRE(auditTrailVector.ptr() != nullptr);
    
    // getAuditTrail returns a vector, so we need to get the first element
    auto firstTrail = auditTrailVector.attr("__getitem__")(0);
    REQUIRE(firstTrail.ptr() != nullptr);
    
    // Test getAuditData with the first ModuleTrail
    std::cout << "DEBUG: Testing getAuditData with first ModuleTrail" << std::endl;
    auto auditDataResult = reader.attr("getAuditData")(firstTrail);
    
    // getAuditData returns an expected<ModuleData>, so we need to check if it has a value
    REQUIRE_NOTHROW(auditDataResult.attr("has_value"));
    bool hasValue = auditDataResult.attr("has_value")().cast<bool>();
    REQUIRE(hasValue == true);
    
    if (hasValue) {
        auto retrievedModuleData = auditDataResult.attr("value")();
        REQUIRE(retrievedModuleData.ptr() != nullptr);
        
        // Test that we can access the metadata
        auto metadata = retrievedModuleData.attr("get_metadata")();
        REQUIRE(metadata.ptr() != nullptr);
        
        std::cout << "DEBUG: Successfully retrieved audit data" << std::endl;
    }
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: getAuditData test completed successfully!" << std::endl;
}

TEST_CASE("Reader closeFile method functionality", "[pybind][reader][closeFile]") {
    std::cout << "DEBUG: Testing closeFile method..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    // Create a Writer to create a test file
    auto writer = module.attr("Writer")();
    REQUIRE(writer.ptr() != nullptr);
    
    // Create a test file
    std::string filename = "test_closeFile_" + std::to_string(std::time(nullptr)) + ".umdf";
    std::cout << "DEBUG: Creating new file: " << filename << std::endl;
    auto createResult = writer.attr("createNewFile")(filename, "testauthor", "testpassword");
    REQUIRE(createResult.attr("success").cast<bool>() == true);
    
    // Create an encounter
    auto encounterResult = writer.attr("createNewEncounter")();
    REQUIRE(encounterResult.attr("has_value")().cast<bool>() == true);
    
    auto encounterId = encounterResult.attr("value")();
    
    // Add a module to the encounter
    auto moduleDataClass = module.attr("ModuleData");
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    
    // Set some basic metadata
    std::string simpleMetadata = "{\"test_key\": \"test_value\", \"module_type\": \"test\"}";
    REQUIRE_NOTHROW(moduleData.attr("set_metadata")(simpleMetadata));
    
    // Add the module to the encounter
    auto addModuleResult = writer.attr("addModuleToEncounter")(encounterId, "test_schema.json", moduleData);
    REQUIRE(addModuleResult.attr("has_value")().cast<bool>() == true);
    
    // Close the writer file
    auto closeResult = writer.attr("closeFile")();
    REQUIRE(closeResult.attr("success").cast<bool>() == true);
    
    // Now test Reader closeFile
    auto reader = module.attr("Reader")();
    REQUIRE(reader.ptr() != nullptr);
    
    auto openResult = reader.attr("openFile")(filename, "testpassword");
    REQUIRE(openResult.attr("success").cast<bool>() == true);
    
    // Test closeFile
    std::cout << "DEBUG: Testing closeFile method" << std::endl;
    auto readerCloseResult = reader.attr("closeFile")();
    
    // closeFile doesn't return a value, so we just verify it doesn't throw
    REQUIRE_NOTHROW(reader.attr("closeFile"));
    
    // Clean up
    std::filesystem::remove(filename);
    
    std::cout << "DEBUG: closeFile test completed successfully!" << std::endl;
}
