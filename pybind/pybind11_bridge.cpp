#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "../src/reader.hpp"
#include "../src/writer.hpp"
#include "../src/DataModule/Image/imageData.hpp"
#include "../src/Utility/moduleType.hpp"

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

    // Expose the Reader class with basic methods
    py::class_<Reader>(m, "Reader")
        .def(py::init<>())
        .def("openFile", &Reader::openFile, "Open a UMDF file")
        .def("getFileInfo", &Reader::getFileInfo, "Get file information")
        .def("getModuleData", &Reader::getModuleData, "Get data for a specific module")
        .def("getAuditTrail", &Reader::getAuditTrail, "Get audit trail for a module")
        .def("getAuditData", &Reader::getAuditData, "Get audit data for a module")
        .def("closeFile", &Reader::closeFile, "Close the currently open file");

    // Expose the Writer class with basic methods
    py::class_<Writer>(m, "Writer")
        .def(py::init<>())
        .def("createNewFile", &Writer::createNewFile, "Create a new UMDF file")
        .def("openFile", &Writer::openFile, "Open an existing UMDF file")
        .def("updateModule", &Writer::updateModule, "Update an existing module")
        .def("createNewEncounter", &Writer::createNewEncounter, "Create a new encounter")
        .def("addModuleToEncounter", &Writer::addModuleToEncounter, "Add a module to an encounter")
        .def("addDerivedModule", &Writer::addDerivedModule, "Add a derived module")
        .def("addAnnotation", &Writer::addAnnotation, "Add an annotation module")
        .def("closeFile", &Writer::closeFile, "Close the file");
}