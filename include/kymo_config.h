/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Application-owned configuration for the ESP32 demo.
 *
 * @details This header exposes application configuration, not platform code in KymoCore.
 * The implementation in src/kymo_config.cpp binds clock, sample and Serial callbacks.
 * @file kymo_config.h
 * @defgroup application_config Application configuration
 * @{
 * @par Usage
 * Include this header in the application entrypoint after initializing hardware.
 */
#ifndef APPLICATION_KYMO_CONFIG_H
/**
 * @brief Include guard for kymo_config.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define APPLICATION_KYMO_CONFIG_H

#include <kymo_runtime.h>

/**
 * @brief Persistent configuration used by the main application.
 *
 * @details Read-only input to Kymo_Init. Its callbacks provide hardware access; its referenced
 * scheduling array is writable runtime storage. Both outlive the global KymoContext.
 * @par Usage
 * Kymo_Init(&kymo, &kymo_config);
 */
extern const KymoConfig kymo_config;

/** @} */
#endif /* APPLICATION_KYMO_CONFIG_H */
