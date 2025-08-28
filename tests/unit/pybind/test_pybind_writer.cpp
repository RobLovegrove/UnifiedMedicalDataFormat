#include <catch2/catch_test_macros.hpp>
#include <pybind11/embed.h>

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
}
