#include <pybind11/pybind11.h>
#include <string>
#include "../../include/meric.h"

namespace py = pybind11;


PYBIND11_MODULE(libmericpy, m) {
    m.doc() = "Python bindings for MERIC API functions";

    // Initialize and Close
    m.def("init", &MERIC_Init, "Initialize MERIC");
    m.def("close", &MERIC_Close, "Close MERIC");

    // Measurement functions
    m.def("measureStart", &MERIC_MeasureStart,
          py::arg("region"), 
          "Start measuring a region");

    m.def("measureStop", &MERIC_MeasureStop,
          py::arg("region"), 
          "Stop measuring a region and return elapsed time");

    m.def("measureStopStart", &MERIC_MeasureStopStart,
          py::arg("stop_region"), py::arg("start_region"),
          "Stop one region and start another, return stop duration");

    // Ignore functions
    m.def("ignoreStart", &MERIC_IgnoreStart, "Start ignoring measurements");
    m.def("ignoreStop", &MERIC_IgnoreStop, "Stop ignoring measurements");

}
