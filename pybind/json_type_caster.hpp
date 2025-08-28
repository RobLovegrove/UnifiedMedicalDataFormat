#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <nlohmann/json.hpp>

namespace pybind11 { namespace detail {

template <> struct type_caster<nlohmann::json> {
public:
    PYBIND11_TYPE_CASTER(nlohmann::json, _("json"));

    // Python -> C++
    bool load(handle src, bool) {
        if (!src) return false;

        try {
            // Use py::module_::import to dump Python object to string
            auto dumps = pybind11::module_::import("json").attr("dumps");
            std::string s = pybind11::cast<std::string>(dumps(src));
            value = nlohmann::json::parse(s);
            return true;
        } catch (...) {
            return false;
        }
    }

    // C++ -> Python
    static handle cast(const nlohmann::json& src, return_value_policy, handle) {
        try {
            auto loads = pybind11::module_::import("json").attr("loads");
            std::string s = src.dump();
            return loads(pybind11::str(s)).release();
        } catch (...) {
            return pybind11::none().release();
        }
    }
};

}} // namespace pybind11::detail