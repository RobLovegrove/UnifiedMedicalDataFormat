#include <catch2/catch_test_macros.hpp>
#include <pybind11/embed.h>

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
    
    SECTION("Reader methods exist (without calling them)") {
        auto reader = module.attr("Reader")();
        
        // Just check that methods exist, don't call them
        REQUIRE_NOTHROW(reader.attr("openFile"));
        REQUIRE_NOTHROW(reader.attr("closeFile"));
        REQUIRE_NOTHROW(reader.attr("getFileInfo"));
        REQUIRE_NOTHROW(reader.attr("getModuleData"));
        REQUIRE_NOTHROW(reader.attr("getAuditTrail"));
        REQUIRE_NOTHROW(reader.attr("getAuditData"));
    }
}
