#ifndef READER_BINDINGS_HPP
#define READER_BINDINGS_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

void register_reader_bindings(py::module_& m);

#endif
