#ifndef WRITER_BINDINGS_HPP
#define WRITER_BINDINGS_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

void register_writer_bindings(py::module_& m);

#endif
