/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file Types.h @brief One coherent synthetic measurement pair.
 * @par Usage Pass by reference; values are dimensionless and share a timestamp. */
#pragma once
#include <stdint.h>
/** @brief Sine and sawtooth values at one logical test-signal instant. */
struct MeasurementPair_T {
    /** @brief Logical generator time in milliseconds since initialization. */
    uint32_t timestampMs;
    /** @brief Sine value in the range [-1, 1]. */
    float sine;
    /** @brief Sawtooth value in the range [-1, 0.984375]. */
    float sawtooth;
};
