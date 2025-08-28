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
        });
}
