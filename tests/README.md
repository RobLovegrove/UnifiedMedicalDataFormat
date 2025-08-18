# UMDF Test Suite

This directory contains comprehensive tests for the UMDF library validation system.

## Structure

```
tests/
├── unit/                   # Unit tests for individual components
│   ├── test_dataField.cpp # DataField validation tests
│   ├── test_dataModule.cpp # DataModule tests
│   ├── test_validation.cpp # General validation tests
│   └── test_schema.cpp    # Schema validation tests
├── integration/            # Integration tests
│   ├── test_fileWorkflow.cpp # End-to-end file operations
│   └── test_endToEnd.cpp  # Complete workflow tests
├── fixtures/              # Test data and schemas
│   ├── valid_schemas/     # Valid schemas for testing
│   ├── invalid_schemas/   # Invalid schemas for testing
│   └── test_data/         # Sample data for testing
└── CMakeLists.txt         # Test build configuration
```

## Running Tests

### Prerequisites
- CMake 3.16+
- Catch2 testing framework
- C++23 compiler

### Build and Run
```bash
# From project root
mkdir -p build
cd build
cmake ..
make

# Run all tests
./tests/umdf_tests

# Run specific test categories
./tests/umdf_tests "[dataField]"
./tests/umdf_tests "[validation]"
./tests/umdf_tests "[schema]"

# Run with verbose output
./tests/umdf_tests --verbose
```

## Test Categories

### Unit Tests
- **DataField Tests**: Validate individual field types (Integer, Float, String, Enum)
- **Schema Tests**: Test schema parsing and validation
- **Constraint Tests**: Verify min/max validation logic
- **Type Tests**: Ensure proper type checking

### Integration Tests
- **File Workflow**: Test complete file read/write operations
- **End-to-End**: Validate entire data processing pipelines
- **Error Handling**: Test error conditions and recovery

### Test Data
- **Valid Schemas**: Correct schemas that should pass validation
- **Invalid Schemas**: Malformed schemas that should fail
- **Test Data**: Sample data for testing validation logic

## Adding New Tests

1. **Create test file** in appropriate directory
2. **Use Catch2 syntax**:
   ```cpp
   TEST_CASE("Test description", "[tags]") {
       SECTION("Specific test case") {
           REQUIRE(condition == true);
       }
   }
   ```
3. **Add to CMakeLists.txt** if creating new test files
4. **Run tests** to ensure they pass

## Test Tags

- `[dataField]` - DataField validation tests
- `[validation]` - General validation tests
- `[schema]` - Schema parsing tests
- `[integration]` - End-to-end workflow tests

## Continuous Integration

Tests are configured to run automatically with:
- Timeout limits (300 seconds)
- Environment variable setup
- CTest integration
- Memory leak detection (when available)
