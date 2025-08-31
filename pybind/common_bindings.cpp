#include "common_bindings.hpp"
#include "json_type_caster.hpp"  // For nlohmann::json type casting
#include "../src/Utility/uuid.hpp"  // For UUID class
#include "../src/DataModule/ModuleData.hpp"  // For ModuleData struct
#include "../src/writer.hpp"  // For Result struct
#include "../src/AuditTrail/auditTrail.hpp"  // For ModuleTrail struct
#include "../src/Utility/dateTime.hpp"  // For DateTime class
#include <iostream>  // For std::cout
#include <expected>  // For std::expected

namespace py = pybind11;

// Convert nlohmann::json <-> py::object
py::object json_to_py(const nlohmann::json& j) {
    return py::module_::import("json").attr("loads")(j.dump());
}

nlohmann::json py_to_json(const py::object& obj) {
    auto s = py::module_::import("json").attr("dumps")(obj).cast<std::string>();
    return nlohmann::json::parse(s);
}

// Helper to convert std::variant to Python object
py::object variant_to_py(const std::variant<
    nlohmann::json,
    std::vector<uint8_t>,
    std::vector<ModuleData>
>& v) {
    return std::visit([](auto&& arg) -> py::object {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, nlohmann::json>) {
            return json_to_py(arg);
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            return py::bytes(reinterpret_cast<const char*>(arg.data()), arg.size());
        } else if constexpr (std::is_same_v<T, std::vector<ModuleData>>) {
            return py::cast(arg);
        } else {
            throw std::runtime_error("Unsupported type in variant");
        }
    }, v);
}

void register_common_bindings(py::module_& m) {
    // Note: nlohmann::json is handled by type_caster, no custom class binding needed
    
    // Register UUID class
    py::class_<UUID>(m, "UUID")
        .def(py::init<>())
        .def_static("fromString", &UUID::fromString, "Create UUID from string")
        .def("toString", &UUID::toString, "Convert UUID to string")
        .def("__str__", &UUID::toString)
        .def("__repr__", &UUID::toString);
    
    // Register ModuleData struct with method-based approach
    py::class_<ModuleData>(m, "ModuleData")
        .def(py::init<>())
        .def("get_metadata", [](const ModuleData& self) -> py::object {
            return py::cast(self.metadata);
        }, "Get metadata")
        .def("set_metadata", [](ModuleData& self, const py::object& metadata) {
            try {
                // Use the type_caster to convert directly
                auto json_data = py::cast<nlohmann::json>(metadata);
                self.metadata = json_data;
            } catch (const std::exception& e) {
                throw;
            }
        }, "Set metadata")
        .def("get_data", [](const ModuleData& self) {
            return variant_to_py(self.data);
        }, "Get data as Python object")
        .def("set_tabular_data", [](ModuleData& self, const py::object& data) {
            self.data = py_to_json(data);
        }, "Set tabular data (JSON)")
        .def("set_binary_data", [](ModuleData& self, const py::bytes& data) {
            // Convert py::bytes to std::vector<uint8_t>
            std::string str = py::cast<std::string>(data);
            std::vector<uint8_t> bytes(str.begin(), str.end());
            self.data = bytes;
        }, "Set binary data")
        .def("set_nested_data", [](ModuleData& self, const std::vector<ModuleData>& data) {
            self.data = data;
        }, "Set nested ModuleData");
    
    // Register std::expected<UUID, std::string> as a simple wrapper
    py::class_<std::expected<UUID, std::string>>(m, "ExpectedUUID")
        .def("has_value", [](const std::expected<UUID, std::string>& self) { return self.has_value(); })
        .def("value", [](const std::expected<UUID, std::string>& self) -> py::object {
            if (self.has_value()) {
                return py::cast(self.value());
            } else {
                throw std::runtime_error("Expected has no value: " + self.error());
            }
        })
        .def("error", [](const std::expected<UUID, std::string>& self) -> py::object {
            if (self.has_value()) {
                throw std::runtime_error("Expected has value, no error");
            } else {
                return py::cast(self.error());
            }
        });
    
    // Register Result struct
    py::class_<Result>(m, "Result")
        .def_readwrite("success", &Result::success)
        .def_readwrite("message", &Result::message);
    
    // Register ModuleTrail struct
    py::class_<ModuleTrail>(m, "ModuleTrail")
        .def_readwrite("moduleOffset", &ModuleTrail::moduleOffset)
        .def_readwrite("isCurrent", &ModuleTrail::isCurrent)
        .def_readwrite("createdAt", &ModuleTrail::createdAt)
        .def_readwrite("modifiedAt", &ModuleTrail::modifiedAt)
        .def_readwrite("createdBy", &ModuleTrail::createdBy)
        .def_readwrite("modifiedBy", &ModuleTrail::modifiedBy)
        .def_readwrite("moduleSize", &ModuleTrail::moduleSize)
        .def_readwrite("moduleType", &ModuleTrail::moduleType)
        .def_readwrite("moduleID", &ModuleTrail::moduleID);
    
    // Register std::expected<ModuleData, std::string> wrapper
    py::class_<std::expected<ModuleData, std::string>>(m, "ExpectedModuleData")
        .def("has_value", [](const std::expected<ModuleData, std::string>& self) { return self.has_value(); })
        .def("value", [](const std::expected<ModuleData, std::string>& self) -> py::object {
            if (self.has_value()) {
                return py::cast(self.value());
            } else {
                throw std::runtime_error("Expected has no value: " + self.error());
            }
        })
        .def("error", [](const std::expected<ModuleData, std::string>& self) -> py::object {
            if (self.has_value()) {
                throw std::runtime_error("Expected has value, no error");
            } else {
                return py::cast(self.error());
            }
        });
    
    // Register std::expected<std::vector<ModuleTrail>, std::string> wrapper
    py::class_<std::expected<std::vector<ModuleTrail>, std::string>>(m, "ExpectedModuleTrail")
        .def("has_value", [](const std::expected<std::vector<ModuleTrail>, std::string>& self) { return self.has_value(); })
        .def("value", [](const std::expected<std::vector<ModuleTrail>, std::string>& self) -> py::object {
            if (self.has_value()) {
                return py::cast(self.value());
            } else {
                throw std::runtime_error("Expected has no value: " + self.error());
            }
        })
        .def("error", [](const std::expected<std::vector<ModuleTrail>, std::string>& self) -> py::object {
            if (self.has_value()) {
                throw std::runtime_error("Expected has value, no error");
            } else {
                return py::cast(self.error());
            }
        });
}
