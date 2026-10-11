/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Application-owned configuration for the ESP32 demo.
 *
 * @details This header verifies that the selected Core supports the generated configuration
 * contract and loads the runtime types used by the application.
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

#include <kymo_version.h>
#if KYMO_CONFIG_API_VERSION != 1
#error "KymoProbe requires KymoCore configuration API version 1"
#endif

#include <kymo_runtime.h>

/** @} */
#endif /* APPLICATION_KYMO_CONFIG_H */
