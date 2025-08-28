#include <catch2/catch_test_macros.hpp>
#include <pybind11/embed.h>
#include <iostream>

TEST_CASE("Pybind Reader class functionality", "[pybind][reader]") {
    pybind11::scoped_interpreter guard{};
    
    // Add pybind directory to Python path
    pybind11::module_::import("sys").attr("path").attr("append")("pybind");
    
    auto module = pybind11::module_::import("umdf_reader");
    
    SECTION("Reader can be instantiated") {
        auto reader_class = module.attr("Reader");
        auto reader = reader_class();
        
        REQUIRE(reader.ptr() != nullptr);
    }
    
    SECTION("Reader methods exist") {
        auto reader = module.attr("Reader")();
        
        // Just check that methods exist, don't call them
        REQUIRE_NOTHROW(reader.attr("openFile"));
        REQUIRE_NOTHROW(reader.attr("closeFile"));
        REQUIRE_NOTHROW(reader.attr("getFileInfo"));
        REQUIRE_NOTHROW(reader.attr("getModuleData"));
        REQUIRE_NOTHROW(reader.attr("getAuditTrail"));
        REQUIRE_NOTHROW(reader.attr("getAuditData"));
    }
    
    SECTION("Reader openFile method functionality") {
        auto reader = module.attr("Reader")();
        
        std::cout << "DEBUG: About to call openFile..." << std::endl;
        
        // Test opening a non-existent file should fail
        auto result = reader.attr("openFile")("non_existent_file.umdf", "password123");
        REQUIRE(result.ptr() != nullptr);
        
        std::cout << "DEBUG: openFile called successfully, result ptr: " << result.ptr() << std::endl;
        
        // Check that result has success and message attributes
        REQUIRE_NOTHROW(result.attr("success"));
        REQUIRE_NOTHROW(result.attr("message"));
        
        std::cout << "DEBUG: Attributes exist, getting values..." << std::endl;
        
        // DEBUG: Let's see what we're actually getting
        auto success = result.attr("success").cast<bool>();
        auto message = result.attr("message").cast<std::string>();
        
        std::cout << "DEBUG: openFile result - success: " << (success ? "true" : "false") 
                  << ", message: '" << message << "'" << std::endl;
        
        // The result should indicate failure
        REQUIRE(success == false);
        REQUIRE_FALSE(message.empty());
        REQUIRE(message.find("Failed to open file") != std::string::npos);
        
        std::cout << "DEBUG: All assertions passed!" << std::endl;
    }
}
