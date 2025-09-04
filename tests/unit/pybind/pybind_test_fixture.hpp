#pragma once

#include <pybind11/embed.h>
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include "test_cleanup.hpp"

namespace py = pybind11;

// Global pybind test fixture to ensure module is imported only once
class PybindTestFixture {
private:
    static py::scoped_interpreter* g_interpreter;
    static py::module_ g_module;
    static bool g_initialized;

public:
    // Initialize the Python interpreter and import the module
    static void initialize();
    
    // Get the shared module reference
    static py::module_& getModule();
    
    // Cleanup (called at program exit)
    static void cleanup();
    
    // Check if initialized
    static bool isInitialized();
    
    // Global test file cleanup
    static void cleanupTestFiles();
};

// RAII wrapper for automatic cleanup
class PybindTestFixtureRAII {
public:
    PybindTestFixtureRAII();
    
    ~PybindTestFixtureRAII();
    
    py::module_& getModule();
};

// Macro to easily get the module in tests
#define GET_PYBIND_MODULE() PybindTestFixture::getModule()

// Macro for test file cleanup
#define CLEANUP_TEST_FILES() test_cleanup::cleanup_all_test_files()

// Macro for tracking test files
#define TRACK_TEST_FILE(filename) test_cleanup::TestFileManager::getInstance().add_file(filename)
