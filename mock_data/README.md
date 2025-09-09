# Mock Data Files

This directory contains mock data files that can be used with the UMDF tool to create sample UMDF files.

## Available Mock Data Files

### Patient Data
- `patient_data.json` - Contains sample patient tabular data with demographics and medical information

### Image Data
- `ct_image_data.json` - CT scan image data with RGB patterns and 4D structure (x, y, z, time)
- `mri_image_data.json` - MRI scan image data with grayscale patterns and 3D structure (x, y, z)

## Usage

Use the `write` command with the `-i` flag to specify a mock data file:

```bash
# Create a UMDF file with patient data
./umdf_tool write -i mock_data/patient_data.json -o my_patient_file.umdf

# Create a UMDF file with CT image data
./umdf_tool write -i mock_data/ct_image_data.json -o my_ct_scan.umdf

# Create a UMDF file with MRI image data
./umdf_tool write -i mock_data/mri_image_data.json -o my_mri_scan.umdf
```

## File Format

Each mock data file is a JSON file containing:

- `schema_path`: Path to the schema file that defines the data structure
- `metadata`: Metadata associated with the data module
- `data`: The actual data (for tabular data) or `frame_config` (for image data)
- `frame_config`: Configuration for generating image frame data (only for image data)

## Adding New Mock Data

To add new mock data files:

1. Create a new JSON file in this directory
2. Follow the existing format structure
3. Ensure the `schema_path` points to a valid schema file
4. For image data, include a `frame_config` section with generation parameters
