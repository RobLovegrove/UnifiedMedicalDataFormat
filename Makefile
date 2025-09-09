# ============================================
# Makefile for UMDF (Unified Medical Data Format)
#
# Usage:
#   make           - builds in release mode
#   make release   - same as above
#   make debug     - builds with debug flags
#   make clean     - removes build artifacts
#   make test      - builds and runs tests
#   make test-build - builds tests only
#
# File structure:
#   src/     - source files (.cpp)
#   include/ - header files (.h, .hpp)
#   build/   - object and dependency files
#   tests/   - test files
#
# ============================================

CXX := g++
CXXFLAGS := -std=c++23 -Iinclude -Isrc -Wall -Wextra -MMD -MP

# Use pkg-config for portable library detection, fallback to common paths
OPENJPEG_CFLAGS := $(shell pkg-config --cflags libopenjp2 2>/dev/null || echo "-I/opt/homebrew/include/openjpeg-2.5 -I/usr/include/openjpeg-2.5 -I/usr/local/include/openjpeg-2.5")
OPENJPEG_LIBS := $(shell pkg-config --libs libopenjp2 2>/dev/null || echo "-L/opt/homebrew/lib -L/usr/lib -L/usr/local/lib -lopenjp2")
PNG_CFLAGS := $(shell pkg-config --cflags libpng 2>/dev/null || echo "-I/opt/homebrew/include -I/usr/include -I/usr/local/include")
PNG_LIBS := $(shell pkg-config --libs libpng 2>/dev/null || echo "-L/opt/homebrew/lib -L/usr/lib -L/usr/local/lib -lpng")
ZSTD_CFLAGS := $(shell pkg-config --cflags libzstd 2>/dev/null || echo "-I/opt/homebrew/opt/zstd/include -I/usr/include -I/usr/local/include")
ZSTD_LIBS := $(shell pkg-config --libs libzstd 2>/dev/null || echo "-L/opt/homebrew/opt/zstd/lib -L/usr/lib -L/usr/local/lib -lzstd")
LIBSODIUM_CFLAGS := $(shell pkg-config --cflags libsodium 2>/dev/null || echo "-I/opt/homebrew/opt/libsodium/include -I/usr/include -I/usr/local/include")
LIBSODIUM_LIBS := $(shell pkg-config --libs libsodium 2>/dev/null || echo "-L/opt/homebrew/opt/libsodium/lib -L/usr/lib -L/usr/local/lib -lsodium")
CATCH2_INCLUDE := -I/opt/homebrew/opt/catch2/include -I/usr/include -I/usr/local/include
CATCH2_LIBS := -L/opt/homebrew/opt/catch2/lib -L/usr/lib -L/usr/local/lib -lCatch2 -lCatch2Main
PYBIND11_CFLAGS := -I/opt/homebrew/include -I/usr/include -I/usr/local/include -I/opt/homebrew/Cellar/python@3.11/3.11.13/Frameworks/Python.framework/Versions/3.11/include/python3.11 -I/usr/include/python3.11 -I/usr/local/include/python3.11
PYBIND11_LIBS := -L/opt/homebrew/Cellar/python@3.11/3.11.13/Frameworks/Python.framework/Versions/3.11/lib -L/usr/lib -L/usr/local/lib -lpython3.11

# pybind module target
PYBIND_MODULE := umdf_reader
PYBIND_SRC := pybind/pybind11_bridge.cpp

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build
TARGET := umdf_tool
TEST_TARGET := umdf_tests

# Find all .cpp files recursively in src/
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')

# Find all test .cpp files recursively in tests/ (excluding test_main.cpp)
TEST_SRCS := $(shell find $(TEST_DIR) -name '*.cpp' ! -name 'test_main.cpp')
TEST_MAIN := $(TEST_DIR)/test_main.cpp

# Map source files to corresponding object files in build/
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Map test files to corresponding object files in build/
TEST_OBJS = build/unit/test_circularReference.o \
            build/unit/test_dataModule.o \
            build/unit/test_dataField.o \
            build/unit/test_schema.o \
            build/unit/test_dateTime.o \
            build/unit/test_imageEncoder.o \
            build/unit/test_schemaResolver.o \
            build/unit/test_imageData.o \
            build/unit/pybind/pybind_test_fixture.o \
            build/unit/pybind/test_cleanup.o \
            build/unit/pybind/test_pybind_writer.o \
            build/unit/pybind/test_pybind_reader.o \
            build/unit/pybind/test_pybind.o \
            build/unit/pybind/test_pybind_moduledata.o \
            build/unit/test_schemaValidation.o \
            build/integration/test_schemaParsing.o \
            build/integration/test_endToEnd.o \
            build/integration/test_fileWorkflow.o

TEST_MAIN_OBJ := $(BUILD_DIR)/test_main.o

# Dependency files (.d) for each object
DEPS := $(OBJS:.o=.d)
TEST_DEPS := $(TEST_OBJS:.o=.d)

# Tell make where to look for prerequisites (source files)
VPATH := $(SRC_DIR):$(TEST_DIR)

.PHONY: all debug release clean test test-build pybind cleanup-test-files

# Default target is release
all: release

debug: CXXFLAGS += -g -O0 $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS)
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS)
release: $(TARGET)

# Test targets
test: test-build pybind
	@echo "Running tests..."
	./$(TEST_TARGET)

test-build: $(TEST_TARGET)

# pybind module target
pybind: $(PYBIND_MODULE).so

$(PYBIND_MODULE).so: pybind/pybind11_bridge.cpp pybind/common_bindings.cpp pybind/reader_bindings.cpp pybind/writer_bindings.cpp $(OBJS)
	$(CXX) -std=c++23 -fPIC -shared $(PYBIND11_CFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS) -Iinclude -Isrc -Ipybind -Wall -Wextra -o $@ pybind/pybind11_bridge.cpp pybind/common_bindings.cpp pybind/reader_bindings.cpp pybind/writer_bindings.cpp $(OBJS) $(PYBIND11_LIBS) $(OPENJPEG_LIBS) $(PNG_LIBS) $(ZSTD_LIBS) $(LIBSODIUM_LIBS)

# Ensure build directory exists before compiling objects
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile nested test files to object files (e.g., tests/unit/pybind/*.cpp)
$(BUILD_DIR)/unit/pybind/%.o: tests/unit/pybind/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS) $(CATCH2_INCLUDE) $(PYBIND11_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile unit test files to object files
$(BUILD_DIR)/unit/%.o: tests/unit/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS) $(CATCH2_INCLUDE) $(PYBIND11_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile integration test files to object files
$(BUILD_DIR)/integration/%.o: tests/integration/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS) $(CATCH2_INCLUDE) $(PYBIND11_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile source files to object files, creating directories as needed
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) $(LIBSODIUM_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile test main file with Catch2 includes
$(BUILD_DIR)/test_main.o: tests/test_main.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CATCH2_INCLUDE) $(PYBIND11_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Link all object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OPENJPEG_LIBS) $(PNG_LIBS) $(ZSTD_LIBS) $(LIBSODIUM_LIBS)

# Link test object files into test executable (without main.o)
TEST_OBJS_FILTERED := $(filter-out build/main.o, $(OBJS))
$(TEST_TARGET): $(TEST_MAIN_OBJ) $(TEST_OBJS) $(TEST_OBJS_FILTERED)
	$(CXX) $(CXXFLAGS) $(CATCH2_INCLUDE) $(PYBIND11_CFLAGS) -o $@ $^ $(OPENJPEG_LIBS) $(PNG_LIBS) $(ZSTD_LIBS) $(LIBSODIUM_LIBS) $(CATCH2_LIBS) $(PYBIND11_LIBS)

# Include dependency files to enable automatic rebuilding
-include $(DEPS)
-include $(TEST_DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

# Clean up test files
cleanup-test-files:
	@echo "Cleaning up test files..."
	@find . -name "test_*.umdf" -type f -delete
	@find . -name "*.umdf.tmp" -type f -delete
	@echo "Test files cleaned up."
