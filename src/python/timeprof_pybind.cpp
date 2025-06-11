#include <pybind11/pybind11.h>
#include <string>

namespace py = pybind11;
#ifdef __cplusplus
extern "C"
{
#endif

    extern void TIMEPROF_regionStart(const char *regionName);
    extern double TIMEPROF_regionStop(const char *regionName);
    extern void TIMEPROF_evaluate();
    extern double TIMEPROF_getLastRegionDuration();

#ifdef __cplusplus
}
#endif

PYBIND11_MODULE(libtimeprofpy, m)
{
    m.doc() = "Python bindings for TIMEPROF API functions";
    m.def("regionStart", &TIMEPROF_regionStart, "Start region");
    m.def("regionStop", &TIMEPROF_regionStop, "Stop region");
    m.def("evaluate", &TIMEPROF_evaluate, "Evaluate");
    m.def("getLastRegionDuration", &TIMEPROF_getLastRegionDuration, "Return last duration");
}