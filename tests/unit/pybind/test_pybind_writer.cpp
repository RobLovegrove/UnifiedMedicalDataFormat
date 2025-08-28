#include <catch2/catch_test_macros.hpp>
#include <pybind11/embed.h>
#include <iostream> // Added for std::cout
#include <filesystem> // Added for std::filesystem
#include <cstdio> // Added for std::remove

TEST_CASE("Pybind Writer class functionality", "[pybind][writer]") {
    pybind11::scoped_interpreter guard{};
    
    // Add pybind directory to Python path
    pybind11::module_::import("sys").attr("path").attr("append")("pybind");
    
    auto module = pybind11::module_::import("umdf_reader");
    
    SECTION("Writer can be instantiated") {
        auto writer_class = module.attr("Writer");
        auto writer = writer_class();
        
        REQUIRE(writer.ptr() != nullptr);
    }
    
    SECTION("Writer createNewFile method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("createNewFile"));
    }
    
    SECTION("Writer openFile method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("openFile"));
    }
    
    SECTION("Writer updateModule method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("updateModule"));
    }
    
    SECTION("Writer createNewEncounter method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("createNewEncounter"));
    }
    
    SECTION("Writer addModuleToEncounter method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("addModuleToEncounter"));
    }
    
    SECTION("Writer addDerivedModule method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("addDerivedModule"));
    }
    
    SECTION("Writer addAnnotation method exists") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("addAnnotation"));
    }
    
    SECTION("Writer closeFile method exists and can be called") {
        auto writer = module.attr("Writer")();
        
        REQUIRE_NOTHROW(writer.attr("closeFile"));
        
        // Test calling closeFile
        auto result = writer.attr("closeFile")();
        REQUIRE(result.ptr() != nullptr);
        REQUIRE_NOTHROW(result.attr("success"));
        REQUIRE_NOTHROW(result.attr("message"));
    }
    
    SECTION("Writer createNewFile basic functionality") {
        auto writer = module.attr("Writer")();
        
        std::cout << "DEBUG: Testing basic createNewFile functionality..." << std::endl;
        
        // Create a new file
        auto createResult = writer.attr("createNewFile")("test_basic_file.umdf", "testauthor", "testpassword");
        REQUIRE(createResult.attr("success").cast<bool>() == true);
        std::cout << "DEBUG: File created successfully" << std::endl;
        
        // For now, let's just close the file without encounters to see if basic file creation works
        // We'll add encounters later once we get the basic flow working
        std::cout << "DEBUG: Attempting to close file without encounters..." << std::endl;
        
        // Close the file immediately - let's see if even a basic file can be read
        auto closeResult = writer.attr("closeFile")();
        REQUIRE(closeResult.attr("success").cast<bool>() == true);
        std::cout << "DEBUG: File closed successfully" << std::endl;
        
        // Verify the file was created and has some content
        // After closeFile, the .tmp file should be renamed to the final filename
        REQUIRE(std::filesystem::exists("test_basic_file.umdf"));
        REQUIRE(std::filesystem::file_size("test_basic_file.umdf") > 0);
        
        std::cout << "DEBUG: Basic file size: " << std::filesystem::file_size("test_basic_file.umdf") << " bytes" << std::endl;
        
        // Clean up
        std::remove("test_basic_file.umdf");
        
        std::cout << "DEBUG: Basic file test completed successfully!" << std::endl;
    }
}
