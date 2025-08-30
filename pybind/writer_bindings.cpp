#include "writer_bindings.hpp"
#include "../src/writer.hpp"

#include "writer_bindings.hpp"
#include "../src/writer.hpp"

void register_writer_bindings(py::module_& m) {
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
        .def("cancelThenClose", &Writer::cancelThenClose, "Cancel the current operation and close the file")
        .def("closeFile", &Writer::closeFile, "Close the file");
}
