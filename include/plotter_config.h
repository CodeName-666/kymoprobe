/**
 * @brief Application-owned configuration for the ESP32 demo.
 *
 * @details This header exposes application configuration, not platform code in PlotterLib.
 * The implementation in src/plotter_config.cpp binds clock, sample and Serial callbacks.
 * @file plotter_config.h
 * @defgroup application_config Application configuration
 * @{
 * @par Usage
 * Include this header in the application entrypoint after initializing hardware.
 */
#ifndef APPLICATION_PLOTTER_CONFIG_H
/**
 * @brief Include guard for plotter_config.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define APPLICATION_PLOTTER_CONFIG_H

#include <plotter_runtime.h>

/**
 * @brief Persistent configuration used by the main application.
 *
 * @details Read-only input to Plotter_Init. Its callbacks provide hardware access; its referenced
 * scheduling array is writable runtime storage. Both outlive the global PlotterContext.
 * @par Usage
 * Plotter_Init(&plotter, &plotter_config);
 */
extern const PlotterConfig plotter_config;

/** @} */
#endif /* APPLICATION_PLOTTER_CONFIG_H */
