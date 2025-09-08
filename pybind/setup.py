from setuptools import setup, Extension
from pybind11.setup_helpers import Pybind11Extension, build_ext
import pybind11

# Define the extension module
ext_modules = [
    Pybind11Extension(
        "umdf_reader",
        ["pybind/pybind11_bridge.cpp",
         "pybind/common_bindings.cpp",
         "pybind/reader_bindings.cpp",
         "pybind/writer_bindings.cpp",
         "src/reader.cpp",
         "src/writer.cpp",
         "src/Header/header.cpp",
         "src/Xref/xref.cpp",
         "src/DataModule/Tabular/tabularData.cpp",
         "src/DataModule/Image/imageData.cpp",
         "src/DataModule/Image/Encoding/ImageEncoder.cpp",
         "src/DataModule/Image/Encoding/JPEG2000Compression.cpp",
         "src/DataModule/Image/Encoding/PNGCompression.cpp",
         "src/DataModule/Image/Encoding/CompressionFactory.cpp",
         "src/DataModule/Image/FrameData.cpp",
         "src/DataModule/Unknown/unknownData.cpp",
         "src/DataModule/Header/dataHeader.cpp",
         "src/DataModule/dataModule.cpp",
         "src/DataModule/dataField.cpp",
         "src/DataModule/stringBuffer.cpp",
         "src/DataModule/SchemaResolver.cpp",
         "src/Links/moduleGraph.cpp",
         "src/AuditTrail/auditTrail.cpp",
         "src/Utility/utils.cpp",
         "src/Utility/uuid.cpp",
         "src/Utility/moduleType.cpp",
         "src/Utility/tlvHeader.cpp",
         "src/Utility/Compression/CompressionType.cpp",
         "src/Utility/Compression/ZstdCompressor.cpp",
         "src/Utility/Encryption/encryptionManager.cpp",
         "src/Utility/dateTime.cpp"],
        include_dirs=[
            "include",
            "src",
            "src/Header",
            "src/Xref",
            "src/DataModule",
            "src/DataModule/Tabular",
            "src/DataModule/Image",
            "src/DataModule/Image/Encoding",
            "src/Utility",
            "pybind",
            pybind11.get_include(),
            "/opt/homebrew/include/openjpeg-2.5",  # OpenJPEG include path
            "/opt/homebrew/include",  # libpng include path
            "/opt/homebrew/opt/zstd/include",
            "/opt/homebrew/opt/libsodium/include"
        ],
        library_dirs=[
            "/opt/homebrew/lib",  # OpenJPEG library path
            "/opt/homebrew/opt/zstd/lib",
            "/opt/homebrew/opt/libsodium/lib"
        ],
        libraries=[
            "openjp2",  # OpenJPEG library name
            "png16",    # libpng library name
            "zstd",     # zstd library name
            "sodium"    # libsodium library name
        ],
        language='c++',
        cxx_std=23,
        extra_compile_args=['-mmacosx-version-min=10.15']
    ),
]

setup(
    name="umdf_reader",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
) 