/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include <Arduino.h>
#include "demo_selection.h"
#include <kymo_features.h>

/** @file main.cpp @brief Central entrypoint for the selected KymoProbe demo.
 * @details Select KYMO_DEMO_MODE in demo_selection.h or a PlatformIO profile.
 * Only one application is compiled; library settings are shared build-wide.
 */

/** @brief Initialize the selected application. @note Not ISR-safe. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
#if KYMO_ENABLED
#if KYMO_DEMO_MODE == KYMO_DEMO_UART
    demoUartInit();
#elif KYMO_DEMO_NETWORK_ENABLE
    demoNetworkInit();
#elif KYMO_STRESS_ENABLE
    demoStressInit();
#elif KYMO_DEMO_MODE == KYMO_DEMO_FEATURES
    demoFeaturesInit();
#elif KYMO_DEMO_MODE == KYMO_DEMO_GENERATED
    demoGeneratedInit();
#elif (KYMO_DEMO_MODE == KYMO_DEMO_TRACE) || \
      (KYMO_DEMO_MODE == KYMO_DEMO_TRACE_FLOW)
    demoTraceInit();
#endif
#endif
}

/** @brief Advance the selected application once. @note Not ISR-safe. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() {
#if KYMO_ENABLED
#if KYMO_DEMO_MODE == KYMO_DEMO_UART
    demoUartMain();
#elif KYMO_DEMO_NETWORK_ENABLE
    demoNetworkMain();
#elif KYMO_STRESS_ENABLE
    demoStressMain();
#elif KYMO_DEMO_MODE == KYMO_DEMO_FEATURES
    demoFeaturesMain();
#elif KYMO_DEMO_MODE == KYMO_DEMO_GENERATED
    demoGeneratedMain();
#elif (KYMO_DEMO_MODE == KYMO_DEMO_TRACE) || \
      (KYMO_DEMO_MODE == KYMO_DEMO_TRACE_FLOW)
    demoTraceMain();
#endif
#endif
}
