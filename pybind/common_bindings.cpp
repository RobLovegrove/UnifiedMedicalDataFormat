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
        .def("__getitem__", [](const nlohmann::json& self, const std::string& key) {
            if (self.contains(key)) {
                return self[key];
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
