cmake_minimum_required(VERSION 3.10)
project(UMDF_Python_Interface)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find pybind11
find_package(pybind11 REQUIRED)

# Set output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/Header
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/Xref
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/DataModule
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/DataModule/Tabular
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/DataModule/Image
    ${CMAKE_CURRENT_SOURCE_DIR}/../../UMDF/src/Utility
)

# Source files from the main UMDF project
set(UMDF_SOURCES
    ../../UMDF/src/writer.cpp
    ../../UMDF/src/reader.cpp
    ../../UMDF/src/Header/header.cpp
    ../../UMDF/src/Xref/xref.cpp
    ../../UMDF/src/DataModule/Tabular/tabularData.cpp
    ../../UMDF/src/DataModule/Image/imageData.cpp
    ../../UMDF/src/DataModule/Image/FrameData.cpp
    ../../UMDF/src/DataModule/Header/dataHeader.cpp
    ../../UMDF/src/DataModule/dataModule.cpp
    ../../UMDF/src/DataModule/dataField.cpp
    ../../UMDF/src/DataModule/stringBuffer.cpp
    ../../UMDF/src/Utility/utils.cpp
    ../../UMDF/src/Utility/uuid.cpp
    ../../UMDF/src/Utility/moduleType.cpp
)

# Create pybind11 module
pybind11_add_module(umdf_reader
    pybind11_bridge.cpp
    ${UMDF_SOURCES}
)

# Copy schemas to build directory
file(COPY ../../UMDF/schemas DESTINATION ${CMAKE_CURRENT_BINARY_DIR}) 