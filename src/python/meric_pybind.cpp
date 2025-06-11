// meric_pybind.cpp

#include <pybind11/pybind11.h>
#include <string>
// #include "../../meric/meric.h"
#include "../../include/meric.h"

namespace py = pybind11;

PYBIND11_MODULE(libmericpy, m) {

    m.doc() = "Python bindings for MERIC API functions";

    m.def("init", &MERIC_Init, "Initialize MERIC");
    m.def("close", &MERIC_Close, "Close MERIC");

    m.def("measure_start", &MERIC_MeasureStart,
          py::arg("region"),
          "Start measuring a region");

    m.def("measure_stop", &MERIC_MeasureStop,
          py::arg("region"),
          "Stop measuring a region and return elapsed time");

    m.def("measure_stop_start", &MERIC_MeasureStopStart,
          py::arg("stop_region"), py::arg("start_region"),
          "Stop one region and start another, return stop duration");

    m.def("ignore_start", &MERIC_IgnoreStart, "Start ignoring measurements");
    m.def("ignore_stop", &MERIC_IgnoreStop, "Stop ignoring measurements");

}
