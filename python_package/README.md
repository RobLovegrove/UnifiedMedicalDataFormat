# UMDF Python Package

Python bindings for the Unified Medical Data Format (UMDF) library.

## Installation

### Prerequisites

#### System Dependencies (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install libopenjp2-7-dev libpng-dev libzstd-dev libsodium-dev build-essential cmake
```

#### System Dependencies (macOS)
```bash
brew install openjpeg png zstd libsodium
```

#### Python Dependencies
```bash
pip install pybind11 setuptools wheel
```

### Install UMDF

#### From Source (Development)
```bash
# Clone the repository
git clone <your-repo-url>
cd umdf/python_package

# Install in development mode
pip install -e .
```

#### From Built Package
```bash
# If you have a pre-built wheel
pip install umdf-1.0.0-py3-none-any.whl
```

## Quick Start

```python
import umdf

# Create a new UMDF file
writer = umdf.Writer()
result = writer.createNewFile("medical_data.umdf", "doctor", "password")

if result.success:
    print("File created successfully!")
    
    # Create an encounter
    encounter_result = writer.createNewEncounter()
    if encounter_result.has_value():
        encounter_id = encounter_result.value()
        
        # Add medical data
        module_data = umdf.ModuleData()
        module_data.set_metadata({"patient_id": "12345", "diagnosis": "Healthy"})
        
        # Add to encounter
        add_result = writer.addModuleToEncounter(encounter_id, "schema.json", module_data)
        if add_result.has_value():
            print(f"Module added with ID: {add_result.value()}")
    
    # Close the file
    close_result = writer.closeFile()
    print(f"File closed: {close_result.success}")

# Read the file back
reader = umdf.Reader()
open_result = reader.openFile("medical_data.umdf", "password")

if open_result.success:
    # Get file information
    file_info = reader.getFileInfo()
    print(f"File info: {file_info}")
    
    # Close the file
    reader.closeFile()
```

## API Reference

### Core Classes

- **`Writer`** - Create and write UMDF files
- **`Reader`** - Read existing UMDF files
- **`ModuleData`** - Store medical data and metadata
- **`UUID`** - Unique identifiers for modules and encounters

### Key Methods

#### Writer
- `createNewFile(filename, author, password)` - Create a new UMDF file
- `createNewEncounter()` - Create a new medical encounter
- `addModuleToEncounter(encounter_id, schema_path, module_data)` - Add data to encounter
- `closeFile()` - Finalize and close the file

#### Reader
- `openFile(filename, password)` - Open an existing UMDF file
- `getFileInfo()` - Get information about the open file
- `getModuleData(module_id)` - Retrieve specific module data
- `closeFile()` - Close the open file

## Examples

See the `examples/` directory for more detailed usage examples.

## License

[Your License Here]
