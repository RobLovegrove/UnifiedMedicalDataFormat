# UMDF - Unified Medical Data Format

A C++ implementation of a unified medical data format for storing and managing medical data with encryption, compression, and audit trail capabilities.

## Building

```bash
make
```

Dependencies: OpenJPEG, libpng, zstd, libsodium, Catch2 (for tests), Python 3.11+ (for pybind11)

## Usage

### Demo
```bash
./umdf_tool demo
```
Runs a comprehensive demonstration with sample data. Output defaults to `demo.umdf` (deleted after completion) or specify with `-o`.

### Reading Files
```bash
./umdf_tool read -i <file.umdf> [-p password]
```

### Writing Files
```bash
# Create new file
./umdf_tool write create -i <mock_data.json> -o <output.umdf> [-p password] [-a author]

# Add to existing file
./umdf_tool write add -i <mock_data.json> -o <existing.umdf> [-e encounter-id] [-p password] [-a author]
# -e encounter-id: Optional. If provided, adds module to existing encounter. If omitted, creates new encounter.

# Update existing module
./umdf_tool write update -i <mock_data.json> -o <file.umdf> --module-id <UUID> [-p password] [-a author]

# Add variant to existing module
./umdf_tool write addVariant -i <mock_data.json> -o <file.umdf> --module-id <parent_UUID> [-p password] [-a author]

# Add annotation to existing module
./umdf_tool write addAnnotation -i <mock_data.json> -o <file.umdf> --module-id <parent_UUID> [-p password] [-a author]
```

## Mock Data

Available in `mock_data/`:
- `ct_image_data.json` - CT scan with 60 axial slices
- `mri_image_data.json` - MRI scan with 24 slices
- `patient_data.json` - Patient demographics and metadata
- `tabular_data.json` - Tabular clinical data

## Example Files

`example_file1.umdf` contains actual CT imaging data:
- Topogram (scout image)
- Axial series with 60 slices
- Encrypted with password: `password`

```bash
./umdf_tool read -i example_file1.umdf -p password
```
