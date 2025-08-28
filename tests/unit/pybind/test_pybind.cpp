#include <catch2/catch_test_macros.hpp>
#include "pybind_test_fixture.hpp"
#include <iostream>

TEST_CASE("Pybind basic functionality", "[pybind]") {
    std::cout << "DEBUG: Testing basic pybind functionality..." << std::endl;
    
    // Get the shared module reference
    auto& module = GET_PYBIND_MODULE();
    REQUIRE(module.ptr() != nullptr);
    
    std::cout << "DEBUG: Module reference obtained successfully" << std::endl;
    
    // Test basic module functionality
    REQUIRE_NOTHROW(module.attr("__name__"));
    
    std::cout << "DEBUG: Basic pybind test completed successfully!" << std::endl;
}
