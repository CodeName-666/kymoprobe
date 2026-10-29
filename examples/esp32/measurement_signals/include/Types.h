/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file Types.h @brief One coherent set of synthetic measurements.
 * @par Usage Pass by reference; values are dimensionless and share a timestamp. */
#pragma once
#include <stdint.h>
#include "Config.h"
/** @brief All channel values at one logical test-signal instant. */
struct MeasurementSet_T {
    /** @brief Logical generator time in milliseconds since initialization. */
    uint32_t timestampMs;
    /** @brief Values indexed by channel ID: sine, sawtooth, multi-tone, noisy square. */
    float values[MEASUREMENT_CHANNEL_COUNT];
};
