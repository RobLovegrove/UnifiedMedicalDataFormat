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
# Author: Rob Lovegrove
# ============================================

CXX := g++
CXXFLAGS := -std=c++23 -Iinclude -Isrc -Wall -Wextra -MMD -MP
OPENJPEG_CFLAGS := -I/opt/homebrew/include/openjpeg-2.5
OPENJPEG_LIBS := -L/opt/homebrew/lib -lopenjp2
PNG_CFLAGS := -I/opt/homebrew/include
PNG_LIBS := -L/opt/homebrew/lib -lpng
ZSTD_CFLAGS := -I/opt/homebrew/opt/zstd/include
ZSTD_LIBS := -L/opt/homebrew/opt/zstd/lib -lzstd

# Catch2 paths
CATCH2_INCLUDE := -I/opt/homebrew/opt/catch2/include
CATCH2_LIBS := -L/opt/homebrew/opt/catch2/lib -lCatch2 -lCatch2Main

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
TEST_OBJS := $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SRCS))
TEST_MAIN_OBJ := $(BUILD_DIR)/test_main.o

# Dependency files (.d) for each object
DEPS := $(OBJS:.o=.d)
TEST_DEPS := $(TEST_OBJS:.o=.d)

# Tell make where to look for prerequisites (source files)
VPATH := $(SRC_DIR):$(TEST_DIR)

.PHONY: all debug release clean test test-build

# Default target is release
all: release

debug: CXXFLAGS += -g -O0 $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS)
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS)
release: $(TARGET)

# Test targets
test: test-build
	@echo "Running tests..."
	./$(TEST_TARGET)

test-build: $(TEST_TARGET)

# Ensure build directory exists before compiling objects
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile source files to object files, creating directories as needed
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OPENJPEG_CFLAGS) $(PNG_CFLAGS) $(ZSTD_CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile test files to object files
$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CATCH2_INCLUDE) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Compile test main file with Catch2 includes
$(BUILD_DIR)/test_main.o: tests/test_main.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CATCH2_INCLUDE) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

# Link all object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(OPENJPEG_LIBS) $(PNG_LIBS) $(ZSTD_LIBS)

# Link test object files into test executable (without main.o)
TEST_OBJS_FILTERED := $(filter-out build/main.o, $(OBJS))
$(TEST_TARGET): $(TEST_MAIN_OBJ) $(TEST_OBJS) $(TEST_OBJS_FILTERED)
	$(CXX) $(CXXFLAGS) $(CATCH2_INCLUDE) -o $@ $^ $(OPENJPEG_LIBS) $(PNG_LIBS) $(ZSTD_LIBS) $(CATCH2_LIBS)

# Include dependency files to enable automatic rebuilding
-include $(DEPS)
-include $(TEST_DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)