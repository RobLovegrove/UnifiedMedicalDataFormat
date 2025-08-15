from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import pybind11

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "umdf_reader",
        ["pybind11_bridge.cpp",
         "../src/reader.cpp",
         "../src/writer.cpp",
         "../src/Header/header.cpp",
         "../src/Xref/xref.cpp",
         "../src/DataModule/Tabular/tabularData.cpp",
         "../src/DataModule/Image/imageData.cpp",
         "../src/DataModule/Image/ImageEncoder.cpp",
         "../src/DataModule/Image/FrameData.cpp",
         "../src/DataModule/Header/dataHeader.cpp",
         "../src/DataModule/dataModule.cpp",
         "../src/DataModule/dataField.cpp",
         "../src/DataModule/stringBuffer.cpp",
         "../src/Utility/utils.cpp",
         "../src/Utility/uuid.cpp",
         "../src/Utility/moduleType.cpp"],
        include_dirs=[
            "../include",
            "../src",
            "../src/Header",
            "../src/Xref",
            "../src/DataModule",
            "../src/DataModule/Tabular",
            "../src/DataModule/Image",
            "../src/Utility",
            pybind11.get_include(),
            "/opt/homebrew/include/openjpeg-2.5",  # OpenJPEG include path
            "/opt/homebrew/include"  # libpng include path
        ],
        library_dirs=[
            "/opt/homebrew/lib"  # OpenJPEG library path
        ],
        libraries=[
            "openjp2",  # OpenJPEG library name
            "png16"     # libpng library name
        ],
        language='c++',
        cxx_std=23
    ),
]

setup(
    name="umdf_reader",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
) 