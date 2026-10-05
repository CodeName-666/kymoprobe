/**
 * @file plotter_features.h
 * @brief Load and validate the internal feature configuration.
 * @details Defaults provide the C++11 scalar encoder and cooperative Init/Main runtime.
 * Edit plotter_build_config.h to select features centrally. Optional compiler
 * definitions override those settings. Only 0 and 1 are accepted.
 * C++ translation units require C++11 or newer; C callers retain the C API.
 * Public data layouts and wire constants do not depend on these switches.
 * @par Usage
 * Include this header through any PlotterLib API; edit plotter_build_config.h.
 */
#ifndef PLOTTER_FEATURES_H
/**
 * @brief Include guard for feature configuration.
 * @details Internal header guard; not an application setting.
 * @par Usage
 * Include plotter_features.h normally; the compiler manages this macro.
 */
#define PLOTTER_FEATURES_H

#include "plotter_build_config.h"

#if defined(__cplusplus) && (__cplusplus < 201103L)
#error "PlotterLib requires C++11 or newer"
#endif

#if ((PLOTTER_ENABLE_RUNTIME != 0) && (PLOTTER_ENABLE_RUNTIME != 1)) || \
    ((PLOTTER_ENABLE_X != 0) && (PLOTTER_ENABLE_X != 1)) || \
    ((PLOTTER_ENABLE_Z != 0) && (PLOTTER_ENABLE_Z != 1)) || \
    ((PLOTTER_ENABLE_TIMESTAMP != 0) && (PLOTTER_ENABLE_TIMESTAMP != 1)) || \
    ((PLOTTER_ENABLE_CRC != 0) && (PLOTTER_ENABLE_CRC != 1)) || \
    ((PLOTTER_ENABLE_DECODER != 0) && (PLOTTER_ENABLE_DECODER != 1)) || \
    ((PLOTTER_ENABLE_CPP != 0) && (PLOTTER_ENABLE_CPP != 1))
#error "Plotter feature switches must be 0 or 1"
#endif

#endif /* PLOTTER_FEATURES_H */
