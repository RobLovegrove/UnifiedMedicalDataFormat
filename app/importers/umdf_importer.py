import sys
import os
import numpy as np
from typing import Dict, Any, Optional, List
from pathlib import Path

# Add the cpp_interface directory to the Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'cpp_interface'))

try:
    import umdf_reader
except ImportError as e:
    print(f"Warning: Could not import umdf_reader: {e}")
    print("Make sure the umdf_reader.cpython-XX-XX.so file is in the cpp_interface directory")
    umdf_reader = None

from ..models.medical_file import Module
from ..schemas.schema_manager import SchemaManager

class UMDFImporter:
    """Handles importing UMDF files using the C++ pybind11 module."""
    
    def __init__(self):
        self.schema_manager = SchemaManager()
        self.reader = None
        
    def _ensure_reader(self):
        """Ensure the UMDF reader is available."""
        if umdf_reader is None:
            raise ImportError("UMDF reader module not available. Please ensure umdf_reader.cpython-XX-XX.so is in the cpp_interface directory.")
        
        if self.reader is None:
            self.reader = umdf_reader.Reader()
    
    async def import_umdf_file(self, file_path: str) -> Dict[str, Any]:
        """Import a UMDF file and convert it to modules."""
        
        self._ensure_reader()
        
        # Read the UMDF file
        if not self.reader.readFile(file_path):
            raise ValueError(f"Failed to read UMDF file: {file_path}")
        
        # Get file info
        file_info = self.reader.getFileInfo()
        module_ids = self.reader.getModuleIds()
        
        modules = []
        
        for module_id in module_ids:
            try:
                # Get module data using the convenience function
                module_data = umdf_reader.get_module_data(file_path, module_id)
                
                # Convert to Module format
                module = self._convert_to_module(module_data, module_id)
                modules.append(module)
                
            except Exception as e:
                print(f"Warning: Failed to process module {module_id}: {e}")
                continue
        
        return {
            "file_type": "umdf",
            "file_path": file_path,
            "modules": modules,
            "file_info": file_info
        }
    
    def _convert_to_module(self, module_data: Dict[str, Any], module_id: str) -> Module:
        """Convert UMDF module data to the UI's Module format."""
        
        # Extract metadata and data
        metadata = module_data.get('metadata', [])
        data = module_data.get('data', {})
        schema_url = module_data.get('schema_url', '')
        
        # Determine module type based on schema
        module_type = self._determine_module_type(schema_url)
        
        # Convert data based on module type
        converted_data = self._convert_data_by_type(data, module_type)
        
        return Module(
            id=module_id,
            type=module_type,
            metadata=metadata,
            data=converted_data,
            schema_url=schema_url
        )
    
    def _determine_module_type(self, schema_url: str) -> str:
        """Determine the module type based on schema URL."""
        if 'image' in schema_url.lower():
            return 'image'
        elif 'patient' in schema_url.lower():
            return 'patient'
        elif 'tabular' in schema_url.lower():
            return 'tabular'
        else:
            return 'unknown'
    
    def _convert_data_by_type(self, data: Any, module_type: str) -> Any:
        """Convert data based on module type."""
        
        if module_type == 'image':
            return self._convert_image_data(data)
        elif module_type == 'patient':
            return self._convert_patient_data(data)
        elif module_type == 'tabular':
            return self._convert_tabular_data(data)
        else:
            return data
    
    def _convert_image_data(self, data: Any) -> Dict[str, Any]:
        """Convert image data to UI format."""
        if isinstance(data, list) and len(data) > 0:
            # This is a list of frames
            frames = []
            for frame_data in data:
                if isinstance(frame_data, dict):
                    frame_metadata = frame_data.get('metadata', [])
                    frame_pixel_data = frame_data.get('data', [])
                    
                    # Convert pixel data to numpy array if it's bytes
                    if isinstance(frame_pixel_data, bytes):
                        # Try to reshape based on metadata
                        pixel_array = np.frombuffer(frame_pixel_data, dtype=np.uint8)
                        # You might need to reshape this based on image dimensions
                        frames.append({
                            'metadata': frame_metadata,
                            'pixel_data': pixel_array.tolist(),
                            'shape': pixel_array.shape
                        })
                    else:
                        frames.append(frame_data)
            
            return {
                'frames': frames,
                'frame_count': len(frames)
            }
        else:
            return data
    
    def _convert_patient_data(self, data: Any) -> Any:
        """Convert patient data to UI format."""
        # Patient data is usually already in a good format
        return data
    
    def _convert_tabular_data(self, data: Any) -> Any:
        """Convert tabular data to UI format."""
        # Tabular data is usually already in a good format
        return data
    
    def get_image_frames(self, file_path: str, module_id: str) -> List[np.ndarray]:
        """Get image frames as numpy arrays for display."""
        
        self._ensure_reader()
        
        if not self.reader.readFile(file_path):
            raise ValueError(f"Failed to read UMDF file: {file_path}")
        
        # Get module data
        module_data = umdf_reader.get_module_data(file_path, module_id)
        
        if module_data.get('schema_url', '').lower().find('image') == -1:
            raise ValueError(f"Module {module_id} is not an image module")
        
        frames = []
        data = module_data.get('data', [])
        
        if isinstance(data, list):
            for frame_data in data:
                if isinstance(frame_data, dict):
                    pixel_data = frame_data.get('data', [])
                    if isinstance(pixel_data, bytes):
                        # Convert to numpy array
                        pixel_array = np.frombuffer(pixel_data, dtype=np.uint8)
                        
                        # Try to reshape to 2D image (you might need to adjust this)
                        # For now, assume it's a square image
                        size = int(np.sqrt(len(pixel_array)))
                        if size * size == len(pixel_array):
                            pixel_array = pixel_array.reshape((size, size))
                        
                        frames.append(pixel_array)
        
        return frames 