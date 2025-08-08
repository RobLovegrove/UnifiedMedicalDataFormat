#!/usr/bin/env python3
"""
Build script for the UMDF Python bindings.
Run this from the pybind directory to build the umdf_reader module.
"""

import os
import sys
import subprocess
from pathlib import Path

def main():
    # Get the parent directory (UMDF root)
    umdf_root = Path(__file__).parent.parent
    os.chdir(umdf_root)
    
    print("Building UMDF Python bindings...")
    print(f"Working directory: {os.getcwd()}")
    
    # Run the setup.py build
    result = subprocess.run([
        sys.executable, 
        "pybind/setup.py", 
        "build_ext", 
        "--inplace"
    ], capture_output=True, text=True)
    
    if result.returncode == 0:
        print("✅ Build successful!")
        print("The umdf_reader module is ready to use.")
    else:
        print("❌ Build failed!")
        print("STDOUT:", result.stdout)
        print("STDERR:", result.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main() 