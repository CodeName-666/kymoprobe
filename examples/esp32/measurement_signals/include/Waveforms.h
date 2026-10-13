/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file Waveforms.h @brief Host-testable signal generator, independent of SDKs.
 * @details No ADC or real sensor is sampled. Time refers to the mathematical
 * generator grid. Both values belong to the same grid point.
 * @par Usage Call calculateMeasurementPair with a multiple of SAMPLE_PERIOD_MS. */
#pragma once
#include <math.h>
#include "Config.h"
#include "Types.h"
/** @brief Calculate coherent sine and sawtooth values.
 * @param[in] logicalTimeMs Generator time in milliseconds.
 * @return Pair of dimensionless values and the supplied timestamp.
 * @note Not ISR-safe; floating-point arithmetic is used. */
/*******************************************************************************
 * calculateMeasurementPair
 ******************************************************************************/
static inline MeasurementPair_T calculateMeasurementPair(uint32_t logicalTimeMs)
{
    MeasurementPair_T result = {};
    const uint32_t sequence = logicalTimeMs / SAMPLE_PERIOD_MS;
    const float sinePhase = static_cast<float>(sequence % SINE_PERIOD_SAMPLES) /
        static_cast<float>(SINE_PERIOD_SAMPLES);
    const float sawPhase = static_cast<float>(sequence % SAW_PERIOD_SAMPLES) /
        static_cast<float>(SAW_PERIOD_SAMPLES);
    result.timestampMs = logicalTimeMs;
    result.sine = sinf(6.283185307179586f * sinePhase);
    result.sawtooth = 2.0f * sawPhase - 1.0f;
    return result;
}
