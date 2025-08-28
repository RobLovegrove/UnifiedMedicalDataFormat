#include <catch2/catch_test_macros.hpp>
#include "pybind_test_fixture.hpp"
#include <iostream>

TEST_CASE("ModuleData bindings functionality", "[pybind][moduledata]") {
    std::cout << "DEBUG: Testing ModuleData bindings..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    std::cout << "DEBUG: Module reference obtained successfully" << std::endl;
    
    // Test if ModuleData class exists
    REQUIRE_NOTHROW(module.attr("ModuleData"));
    std::cout << "DEBUG: ModuleData class exists" << std::endl;
    
    // Test if we can get a reference to the class
    auto moduleDataClass = module.attr("ModuleData");
    REQUIRE(moduleDataClass.ptr() != nullptr);
    std::cout << "DEBUG: ModuleData class reference obtained" << std::endl;
    
    // Test if we can create an instance
    REQUIRE_NOTHROW(moduleDataClass());
    std::cout << "DEBUG: ModuleData constructor callable" << std::endl;
    
    auto moduleData = moduleDataClass();
    REQUIRE(moduleData.ptr() != nullptr);
    std::cout << "DEBUG: ModuleData instance created successfully" << std::endl;
    
    // Test if we can call the get_metadata method
    REQUIRE_NOTHROW(moduleData.attr("get_metadata"));
    std::cout << "DEBUG: get_metadata method accessible" << std::endl;
    
    // Test if we can read the metadata using the method
    auto metadata = moduleData.attr("get_metadata")();
    REQUIRE(metadata.ptr() != nullptr);
    std::cout << "DEBUG: Metadata field readable via method" << std::endl;
    
    std::cout << "DEBUG: Test completed successfully!" << std::endl;
}
