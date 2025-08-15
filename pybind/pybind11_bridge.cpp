#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "reader.hpp"
#include "writer.hpp"
#include "DataModule/Image/imageData.hpp"
#include "Utility/moduleType.hpp"

namespace py = pybind11;

PYBIND11_MODULE(umdf_reader, m) {
    m.doc() = "UMDF Reader Python Module"; 

    // Register nlohmann::json type
    py::class_<nlohmann::json>(m, "Json")
        .def("dump", [](const nlohmann::json& self, int indent = -1, char indent_char = ' ', bool ensure_ascii = false) {
            return self.dump(indent, indent_char, ensure_ascii);
        })
        .def("__str__", [](const nlohmann::json& self) {
            return self.dump();
        });

    // Expose the Reader class with new methods
    py::class_<Reader>(m, "Reader")
        .def(py::init<>())
        .def("openFile", &Reader::openFile, "Open a UMDF file")
        .def("getFileInfo", [](Reader& reader) -> py::object {
            auto result = reader.getFileInfo();
            // Convert nlohmann::json to Python dict
            return py::module_::import("json").attr("loads")(result.dump());
        }, "Get file information")
        .def("getModuleData", [](Reader& reader, const std::string& moduleId) -> py::object {
            auto result = reader.getModuleData(moduleId);
            
            if (result.has_value()) {
                // Success case - convert ModuleData to Python dict
                auto moduleData = result.value();
                
                py::dict result_dict;
                result_dict["success"] = true;
                result_dict["metadata"] = moduleData.metadata;
                
                // Handle the variant data
                std::visit([&result_dict](const auto& data) {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, nlohmann::json>) {
                        result_dict["data"] = data;
                        result_dict["data_type"] = "json";
                    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                        result_dict["data"] = py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
                        result_dict["data_type"] = "binary";
                        result_dict["data_size"] = data.size();
                    } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
                        // Handle nested modules (frames)
                        py::list frame_list;
                        for (const auto& frame : data) {
                            py::dict frame_dict;
                            frame_dict["metadata"] = frame.metadata;
                            
                            std::visit([&frame_dict](const auto& frame_data) {
                                using FrameT = std::decay_t<decltype(frame_data)>;
                                if constexpr (std::is_same_v<FrameT, nlohmann::json>) {
                                    frame_dict["data"] = frame_data;
                                    frame_dict["data_type"] = "json";
                                } else if constexpr (std::is_same_v<FrameT, std::vector<uint8_t>>) {
                                    frame_dict["data"] = py::bytes(reinterpret_cast<const char*>(frame_data.data()), frame_data.size());
                                    frame_dict["data_type"] = "binary";
                                    frame_dict["data_size"] = frame_data.size();
                                }
                            }, frame.data);  // Fixed: was frame_data.data, should be frame.data
                            
                            frame_list.append(frame_dict);
                        }
                        result_dict["data"] = frame_list;
                        result_dict["data_type"] = "frames";
                        result_dict["frame_count"] = data.size();
                    }
                }, moduleData.data);
                
                return result_dict;
            } else {
                // Error case
                py::dict error_dict;
                error_dict["success"] = false;
                error_dict["error"] = result.error();
                return error_dict;
            }
        }, "Get data for a specific module")
        .def("closeFile", &Reader::closeFile, "Close the currently open file");

    // Expose the Writer class
    py::class_<Writer>(m, "Writer")
        .def(py::init<>())
        .def("writeNewFile", &Writer::writeNewFile, "Write a new UMDF file")
        .def("setFileAccessMode", &Writer::setFileAccessMode, "Set file access mode");

    // Expose FileAccessMode enum
    py::enum_<FileAccessMode>(m, "FileAccessMode")
        .value("FailIfExists", FileAccessMode::FailIfExists)
        .value("AllowUpdate", FileAccessMode::AllowUpdate)
        .value("Overwrite", FileAccessMode::Overwrite);
}