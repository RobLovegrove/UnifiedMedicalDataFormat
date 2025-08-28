#include "json_type_caster.hpp"  // Must be first for type_caster to work
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "common_bindings.hpp"
#include "reader_bindings.hpp"
#include "writer_bindings.hpp"

namespace py = pybind11;

PYBIND11_MODULE(umdf_reader, m) {
    m.doc() = "UMDF Reader Python Module"; 
    
    // Register common types
    register_common_bindings(m);
    
    // Register Reader and Writer
    register_reader_bindings(m);
    register_writer_bindings(m);
}