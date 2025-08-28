#include <catch2/catch_test_macros.hpp>
#include <pybind11/embed.h>

TEST_CASE("Pybind module basic compilation", "[pybind][basic]") {
    SECTION("Module can be imported") {
        // This will fail if there are compilation/linking issues
        REQUIRE_NOTHROW([]() {
            pybind11::scoped_interpreter guard{};
            
            // Add pybind directory to Python path
            pybind11::module_::import("sys").attr("path").attr("append")("pybind");
            
            auto module = pybind11::module_::import("umdf_reader");
            REQUIRE(module.ptr() != nullptr);
        }());
    }
    
    SECTION("Basic classes can be instantiated") {
        pybind11::scoped_interpreter guard{};
        auto module = pybind11::module_::import("umdf_reader");
        
        // Test Reader instantiation
        REQUIRE_NOTHROW([&module]() {
            auto reader_class = module.attr("Reader");
            auto reader = reader_class();
            REQUIRE(reader.ptr() != nullptr);
        }());
        
        // Test Writer instantiation
        REQUIRE_NOTHROW([&module]() {
            auto writer_class = module.attr("Writer");
            auto writer = writer_class();
            REQUIRE(writer.ptr() != nullptr);
        }());
    }
    
    SECTION("Common types are available") {
        pybind11::scoped_interpreter guard{};
        auto module = pybind11::module_::import("umdf_reader");
        
        // Test that Json class exists
        REQUIRE_NOTHROW(module.attr("Json"));
    }
}
