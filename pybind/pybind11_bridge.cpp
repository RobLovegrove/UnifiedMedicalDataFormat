#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "reader.hpp"
#include "writer.hpp"
#include "DataModule/Image/imageData.hpp"
#include "Utility/moduleType.hpp"

namespace py = pybind11;

PYBIND11_MODULE(umdf_reader, m) {
    m.doc() = "UMDF Reader Python Module"; // optional module docstring

    // Register nlohmann::json type
    py::class_<nlohmann::json>(m, "Json")
        .def("dump", [](const nlohmann::json& self, int indent = -1, char indent_char = ' ', bool ensure_ascii = false) {
            return self.dump(indent, indent_char, ensure_ascii);
        })
        .def("__str__", [](const nlohmann::json& self) {
            return self.dump();
        });

    // Expose the Reader class
    py::class_<Reader>(m, "Reader")
        .def(py::init<>())
        .def("readFile", &Reader::readFile, "Read a UMDF file")
        .def("getFileInfo", &Reader::getFileInfo, "Get file information")
        .def("getModuleList", &Reader::getModuleList, "Get list of modules")
        .def("getModuleData", &Reader::getModuleData, "Get data for a specific module")
        .def("getModuleIds", &Reader::getModuleIds, "Get all module IDs");

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

    // Add some convenience functions
    m.def("read_umdf_file", [](const std::string& filename) -> py::object {
        Reader reader;
        if (reader.readFile(filename)) {
            nlohmann::json json_data = reader.getFileInfo();
            return py::module_::import("json").attr("loads")(json_data.dump());
        } else {
            throw std::runtime_error("Failed to read UMDF file: " + filename);
        }
    }, "Read a UMDF file and return file info");

    m.def("get_module_data", [](const std::string& filename, const std::string& module_id) -> py::object {
        Reader reader;
        if (reader.readFile(filename)) {
            nlohmann::json json_data = reader.getModuleData(module_id);
            return py::module_::import("json").attr("loads")(json_data.dump());
        } else {
            throw std::runtime_error("Failed to read UMDF file: " + filename);
        }
    }, "Read a UMDF file and get data for a specific module");

    // New function for direct binary transfer with metadata
    m.def("get_module_data_binary", [](const std::string& filename, const std::string& module_id) -> py::object {
        Reader reader;
        if (reader.readFile(filename)) {
            // Find module by UUID
            for (const auto& module : reader.getLoadedModules()) {
                if (module->getModuleID().toString() == module_id) {
                    // Get the ModuleData structure directly
                    auto moduleData = module->getDataWithSchema();
                    
                    // Create Python dictionary to hold metadata and data
                    py::dict result;
                    
                    // Add metadata (but remove dimensions to avoid confusion with our filtered ones)
                    auto metadata = moduleData.metadata;
                    if (module->getModuleType() == ModuleType::Image) {
                        // Remove dimensions and dimension_names from metadata for image modules
                        // since we'll provide filtered versions below
                        if (metadata.is_array() && !metadata.empty()) {
                            for (auto& row : metadata) {
                                if (row.contains("dimensions")) {
                                    row.erase("dimensions");
                                }
                                if (row.contains("dimension_names")) {
                                    row.erase("dimension_names");
                                }
                            }
                        }
                    }
                    result["metadata"] = py::module_::import("json").attr("loads")(metadata.dump());
                    result["module_id"] = module->getModuleID().toString();
                    result["schema_url"] = module->getSchema()["$id"];
                    
                    // Add non-zero dimensions and dimension names for image modules
                    if (module->getModuleType() == ModuleType::Image) {
                        const auto* imageModule = dynamic_cast<const ImageData*>(module.get());
                        if (imageModule) {
                            auto nonZeroDims = imageModule->getNonZeroDimensions();
                            auto nonZeroNames = imageModule->getNonZeroDimensionNames();
                            
                            // Convert to Python lists
                            py::list dimsList, namesList;
                            for (auto dim : nonZeroDims) {
                                dimsList.append(dim);
                            }
                            for (const auto& name : nonZeroNames) {
                                namesList.append(name);
                            }
                            
                            result["dimensions"] = dimsList;
                            result["dimension_names"] = namesList;
                        }
                    }
                    
                    // Handle the variant data
                    std::visit([&](const auto& data) {
                        using T = std::decay_t<decltype(data)>;
                        if constexpr (std::is_same_v<T, nlohmann::json>) {
                            // JSON data - convert to Python dict
                            result["data"] = py::module_::import("json").attr("loads")(data.dump());
                            result["data_type"] = "json";
                        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                            // Binary data - transfer directly as bytes
                            result["data"] = py::bytes(reinterpret_cast<const char*>(data.data()), data.size());
                            result["data_type"] = "binary";
                            result["data_size"] = data.size();
                        } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
                            // Nested modules (like frames) - handle recursively
                            py::list frame_list;
                            for (const auto& frame : data) {
                                py::dict frame_dict;
                                frame_dict["metadata"] = py::module_::import("json").attr("loads")(frame.metadata.dump());
                                
                                // Handle frame data
                                std::visit([&](const auto& frame_data) {
                                    using FrameT = std::decay_t<decltype(frame_data)>;
                                    if constexpr (std::is_same_v<FrameT, nlohmann::json>) {
                                        frame_dict["data"] = py::module_::import("json").attr("loads")(frame_data.dump());
                                        frame_dict["data_type"] = "json";
                                    } else if constexpr (std::is_same_v<FrameT, std::vector<uint8_t>>) {
                                        frame_dict["data"] = py::bytes(reinterpret_cast<const char*>(frame_data.data()), frame_data.size());
                                        frame_dict["data_type"] = "binary";
                                        frame_dict["data_size"] = frame_data.size();
                                    }
                                }, frame.data);
                                
                                frame_list.append(frame_dict);
                            }
                            result["data"] = frame_list;
                            result["data_type"] = "frames";
                            result["frame_count"] = data.size();
                        }
                    }, moduleData.data);
                    
                    return result;
                }
            }
            throw std::runtime_error("Module not found: " + module_id);
        } else {
            throw std::runtime_error("Failed to read UMDF file: " + filename);
        }
    }, "Read a UMDF file and get data for a specific module with direct binary transfer");

    // New convenience function to read file and get all modules at once
    m.def("read_umdf_file_all_modules", [](const std::string& filename) -> py::object {
        Reader reader;
        if (reader.readFile(filename)) {
            nlohmann::json result;
            result["success"] = true;
            result["modules"] = nlohmann::json::array();
            
            for (const auto& module : reader.getLoadedModules()) {
                auto moduleData = module->getDataWithSchema();
                
                nlohmann::json moduleJson;
                moduleJson["module_id"] = module->getModuleID().toString();
                moduleJson["schema_url"] = module->getSchema()["$id"];
                moduleJson["metadata"] = moduleData.metadata;
                
                // Convert the variant data to JSON
                std::visit([&moduleJson](const auto& data) {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, nlohmann::json>) {
                        moduleJson["data"] = data;
                    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                        moduleJson["data"] = data;
                    } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
                        // Handle frame data conversion
                        nlohmann::json dataArray = nlohmann::json::array();
                        for (const auto& md : data) {
                            nlohmann::json mdJson;
                            mdJson["metadata"] = md.metadata;
                            std::visit([&mdJson](const auto& d) {
                                using D = std::decay_t<decltype(d)>;
                                if constexpr (std::is_same_v<D, nlohmann::json>) {
                                    mdJson["data"] = d;
                                } else if constexpr (std::is_same_v<D, std::vector<uint8_t>>) {
                                    mdJson["data"] = d;
                                }
                            }, md.data);
                            dataArray.push_back(mdJson);
                        }
                        moduleJson["data"] = dataArray;
                    }
                }, moduleData.data);
                
                result["modules"].push_back(moduleJson);
            }
            
            return py::module_::import("json").attr("loads")(result.dump());
        } else {
            throw std::runtime_error("Failed to read UMDF file: " + filename);
        }
    }, "Read a UMDF file and return all modules in one call");
} 