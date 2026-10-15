/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file Waveforms.h @brief Host-testable signal generator, independent of SDKs.
 * @details No ADC or real sensor is sampled. Time refers to the mathematical
 * generator grid. All values belong to the same grid point. The noise is a
 * stateless hash of the grid index, so the sequence is reproducible.
 * @par Usage Call calculateMeasurementSet with a multiple of SAMPLE_PERIOD_MS. */
#pragma once
#include <math.h>
#include "Config.h"
#include "Types.h"
/** @brief Phase in [0, 1) of a grid index for a period in samples. */
/*******************************************************************************
 * gridPhase
 ******************************************************************************/
static inline float gridPhase(uint32_t sequence, uint32_t periodSamples)
{
    return static_cast<float>(sequence % periodSamples) / static_cast<float>(periodSamples);
}

/** @brief Uniform pseudo-noise in [-1, 1] derived from the grid index. */
/*******************************************************************************
 * gridNoise
 ******************************************************************************/
static inline float gridNoise(uint32_t sequence)
{
    uint32_t hash = sequence * 2654435761u;
    hash ^= hash >> 16u;
    hash *= 2246822519u;
    hash ^= hash >> 13u;
    return static_cast<float>(hash & 0xFFFFu) / 32767.5f - 1.0f;
}

/** @brief Calculate coherent values of all channels.
 * @param[in] logicalTimeMs Generator time in milliseconds.
 * @return Dimensionless values and the supplied timestamp.
 * @note Not ISR-safe; floating-point arithmetic is used. */
/*******************************************************************************
 * calculateMeasurementSet
 ******************************************************************************/
static inline MeasurementSet_T calculateMeasurementSet(uint32_t logicalTimeMs)
{
    constexpr float FULL_TURN = 6.283185307179586f;
    MeasurementSet_T result = {};
    const uint32_t sequence = logicalTimeMs / SAMPLE_PERIOD_MS;
    const float tonePhase = gridPhase(sequence, TONE_PERIOD_SAMPLES);
    result.timestampMs = logicalTimeMs;
    result.values[0] = sinf(FULL_TURN * gridPhase(sequence, SINE_PERIOD_SAMPLES));
    result.values[1] = 2.0f * gridPhase(sequence, SAW_PERIOD_SAMPLES) - 1.0f;
    result.values[2] = 0.6f * sinf(FULL_TURN * tonePhase) + 0.3f * sinf(4.0f * FULL_TURN * tonePhase) +
        0.1f * sinf(8.0f * FULL_TURN * tonePhase);
    result.values[3] = (gridPhase(sequence, SQUARE_PERIOD_SAMPLES) < 0.5f ? 1.0f : -1.0f) +
        SQUARE_NOISE_AMPLITUDE * gridNoise(sequence);
    return result;
}
