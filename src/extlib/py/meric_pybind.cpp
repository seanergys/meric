// meric_pybind.cpp

#include <pybind11/pybind11.h>
#include <string>
#include "../../meric/meric.h"

namespace py = pybind11;

PYBIND11_MODULE(libmericpy, m) {
    py::class_<meric::Meric>(m, "Meric")
        .def(py::init<>())  // default ctor

        // static method
        .def_static("readRegionParameters", &meric::Meric::readRegionParameters)

        // public methods
        .def("start", &meric::Meric::start, py::arg("region"))
        .def("stop", &meric::Meric::stop, py::arg("region") = "")
        .def("startIgnore", &meric::Meric::startIgnore)
        .def("stopIgnore", &meric::Meric::stopIgnore)
        .def("save", &meric::Meric::save)
        .def("close", &meric::Meric::close);
}
