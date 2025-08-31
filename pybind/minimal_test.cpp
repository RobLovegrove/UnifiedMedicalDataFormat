#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(minimal_test, m) {
    m.doc() = "Minimal test module to isolate segmentation fault";
    
    m.def("hello", []() {
        return "Hello from minimal test!";
    });
    
    m.def("add", [](int a, int b) {
        return a + b;
    });
}
