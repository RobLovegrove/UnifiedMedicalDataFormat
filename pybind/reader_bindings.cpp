#include "reader_bindings.hpp"
#include "../src/reader.hpp"

void register_reader_bindings(py::module_& m) {
    // Expose the Reader class with basic methods
    py::class_<Reader>(m, "Reader")
        .def(py::init<>())
        .def("openFile", &Reader::openFile, "Open a UMDF file")
        .def("getFileInfo", &Reader::getFileInfo, "Get file information")
        .def("getModuleData", &Reader::getModuleData, "Get data for a specific module")
        .def("getAuditTrail", &Reader::getAuditTrail, "Get audit trail for a module")
        .def("getAuditData", &Reader::getAuditData, "Get audit data for a module")
        .def("closeFile", &Reader::closeFile, "Close the currently open file");
}
