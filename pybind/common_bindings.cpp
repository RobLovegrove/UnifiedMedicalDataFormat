#include "common_bindings.hpp"
#include "../src/reader.hpp"  // For nlohmann::json

void register_common_bindings(py::module_& m) {
    // Register nlohmann::json type
    py::class_<nlohmann::json>(m, "Json")
        .def("dump", [](const nlohmann::json& self, int indent = -1, char indent_char = ' ', bool ensure_ascii = false) {
            return self.dump(indent, indent_char, ensure_ascii);
        })
        .def("__str__", [](const nlohmann::json& self) {
            return self.dump();
        })
        .def("__getitem__", [](const nlohmann::json& self, const std::string& key) -> py::object {
            if (self.contains(key)) {
                const auto& value = self[key];
                // Return the actual value based on its type
                if (value.is_boolean()) {
                    return py::cast(value.get<bool>());
                } else if (value.is_string()) {
                    return py::cast(value.get<std::string>());
                } else if (value.is_number_integer()) {
                    return py::cast(value.get<int64_t>());
                } else if (value.is_number_float()) {
                    return py::cast(value.get<double>());
                } else {
                    // For other types (arrays, objects), return the JSON object itself
                    return py::cast(value);
                }
            }
            throw std::out_of_range("Key '" + key + "' not found in JSON object");
        })
        .def("contains", [](const nlohmann::json& self, const std::string& key) {
            return self.contains(key);
        });
    
    // Register Result struct
    py::class_<Result>(m, "Result")
        .def_readwrite("success", &Result::success)
        .def_readwrite("message", &Result::message);
}
