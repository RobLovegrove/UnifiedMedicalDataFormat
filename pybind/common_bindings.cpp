#include "common_bindings.hpp"
#include "json_type_caster.hpp"  // For nlohmann::json type casting
#include "../src/Utility/uuid.hpp"  // For UUID class
#include "../src/DataModule/ModuleData.hpp"  // For ModuleData struct
#include "../src/writer.hpp"  // For Result struct
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
    std::cout << "DEBUG: Starting to register common bindings..." << std::endl;
    
    // Note: nlohmann::json is handled by type_caster, no custom class binding needed
    std::cout << "DEBUG: Json type_caster active, no custom class binding needed" << std::endl;
    
    // Register UUID class
    std::cout << "DEBUG: About to register UUID class..." << std::endl;
    py::class_<UUID>(m, "UUID")
        .def(py::init<>())
        .def("toString", &UUID::toString, "Convert UUID to string")
        .def("__str__", &UUID::toString)
        .def("__repr__", &UUID::toString);
    std::cout << "DEBUG: UUID class registered successfully!" << std::endl;
    
    // Register ModuleData struct with method-based approach
    std::cout << "DEBUG: About to register ModuleData class..." << std::endl;
    py::class_<ModuleData>(m, "ModuleData")
        .def(py::init<>())
        .def("get_metadata", [](const ModuleData& self) -> py::object {
            std::cout << "DEBUG: get_metadata called" << std::endl;
            return py::cast(self.metadata);
        }, "Get metadata")
        .def("set_metadata", [](ModuleData& self, const py::object& metadata) {
            std::cout << "DEBUG: set_metadata called with type: " << py::str(metadata.get_type()).cast<std::string>() << std::endl;
            std::cout << "DEBUG: About to try casting to nlohmann::json..." << std::endl;
            try {
                // Use the type_caster to convert directly
                auto json_data = py::cast<nlohmann::json>(metadata);
                std::cout << "DEBUG: Successfully cast to nlohmann::json" << std::endl;
                self.metadata = json_data;
                std::cout << "DEBUG: Successfully assigned to self.metadata" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "DEBUG: Exception during conversion: " << e.what() << std::endl;
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
    std::cout << "DEBUG: ModuleData class registered successfully!" << std::endl;
    
    // Register std::expected<UUID, std::string> as a simple wrapper
    std::cout << "DEBUG: About to register ExpectedUUID class..." << std::endl;
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
    std::cout << "DEBUG: ExpectedUUID class registered successfully!" << std::endl;
    
    // Register Result struct
    std::cout << "DEBUG: About to register Result class..." << std::endl;
    py::class_<Result>(m, "Result")
        .def_readwrite("success", &Result::success)
        .def_readwrite("message", &Result::message);
    std::cout << "DEBUG: Result class registered successfully!" << std::endl;
    
    std::cout << "DEBUG: All common bindings registered successfully!" << std::endl;
}
