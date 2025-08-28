#ifndef COMMON_BINDINGS_HPP
#define COMMON_BINDINGS_HPP

#include <pybind11/pybind11.h>

namespace py = pybind11;

void register_common_bindings(py::module_& m);

#endif
