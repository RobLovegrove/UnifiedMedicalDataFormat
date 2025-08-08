#!/usr/bin/env python3
"""
Test script to verify UMDF integration with the UI.
"""

import asyncio
import sys
import os

# Add the app directory to the path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'app'))

from importers.file_importer import FileImporter

async def test_umdf_import():
    """Test importing a UMDF file through the UI's file importer."""
    
    print("Testing UMDF Integration with UI")
    print("=" * 40)
    
    # Create file importer
    importer = FileImporter()
    
    # Read the example UMDF file
    with open('example.umdf', 'rb') as f:
        content = f.read()
    
    print(f"Loaded UMDF file: {len(content)} bytes")
    
    try:
        # Import the UMDF file
        result = await importer.import_file(
            content=content,
            filename="example.umdf",
            file_type="umdf"
        )
        
        print("✅ UMDF import successful!")
        print(f"File type: {result['file_type']}")
        print(f"File path: {result['file_path']}")
        print(f"Number of modules: {len(result['modules'])}")
        
        # Print module details
        for i, module in enumerate(result['modules']):
            print(f"\nModule {i+1}:")
            print(f"  ID: {module.id}")
            print(f"  Type: {module.type}")
            print(f"  Schema: {module.schema_url}")
            print(f"  Metadata entries: {len(module.metadata)}")
            
            if module.type == 'image':
                data = module.data
                if isinstance(data, dict) and 'frames' in data:
                    print(f"  Frame count: {data['frame_count']}")
                    for j, frame in enumerate(data['frames'][:3]):  # Show first 3 frames
                        print(f"    Frame {j}: shape={frame.get('shape', 'unknown')}")
        
        print("\n✅ All tests passed!")
        
    except Exception as e:
        print(f"❌ Test failed: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    asyncio.run(test_umdf_import()) 