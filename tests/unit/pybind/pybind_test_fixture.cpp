#include "pybind_test_fixture.hpp"

namespace py = pybind11;

// Static member initialization
py::scoped_interpreter* PybindTestFixture::g_interpreter = nullptr;
py::module_ PybindTestFixture::g_module;
bool PybindTestFixture::g_initialized = false;

// Initialize the Python interpreter and import the module
void PybindTestFixture::initialize() {
    if (!g_initialized) {
        std::cout << "DEBUG: Initializing Python interpreter and importing umdf_reader module..." << std::endl;
        
        // Create the interpreter
        g_interpreter = new py::scoped_interpreter();
        
        // Add the pybind directory to Python path
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")("pybind");
        
        // Import our module
        g_module = py::module_::import("umdf_reader");
        
        g_initialized = true;
        std::cout << "DEBUG: Module imported successfully" << std::endl;
    }
}

// Get the shared module reference
py::module_& PybindTestFixture::getModule() {
    if (!g_initialized) {
        initialize();
    }
    return g_module;
}

// Cleanup (called at program exit)
void PybindTestFixture::cleanup() {
    if (g_initialized) {
        std::cout << "DEBUG: Cleaning up Python interpreter..." << std::endl;
        delete g_interpreter;
        g_interpreter = nullptr;
        g_initialized = false;
    }
}

// Check if initialized
bool PybindTestFixture::isInitialized() {
    return g_initialized;
}

// RAII wrapper implementation
PybindTestFixtureRAII::PybindTestFixtureRAII() {
    PybindTestFixture::initialize();
}

PybindTestFixtureRAII::~PybindTestFixtureRAII() {
    // Note: We don't cleanup here as it can cause issues with static destruction order
    // Cleanup will be handled by the test framework
}

py::module_& PybindTestFixtureRAII::getModule() {
    return PybindTestFixture::getModule();
}
