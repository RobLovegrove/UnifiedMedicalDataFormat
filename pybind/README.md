# UMDF Python Bindings

This directory contains the Python bindings for the UMDF C++ library using pybind11.

## Files

- `pybind11_bridge.cpp` - The main pybind11 binding code
- `setup.py` - Build configuration for the Python module
- `build.py` - Convenient build script
- `README.md` - This file

## Building the Python Module

### Option 1: Using the build script
```bash
cd pybind
python build.py
```

### Option 2: Manual build
```bash
cd pybind
python setup.py build_ext --inplace
```

## Usage

After building, the `umdf_reader` module can be imported in Python:

```python
import umdf_reader

# Read a UMDF file
reader = umdf_reader.Reader()
reader.readFile("example.umdf")

# Get module data
module_data = reader.getModuleData("module_id")
```

## What Gets Built

The build process creates:
- `umdf_reader.cpython-XX-XX.so` - The compiled Python module
- `build/` directory - Build artifacts (can be deleted)

## Dependencies

- pybind11
- C++17 compatible compiler
- Python 3.7+ 