/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file Config.h @brief Fixed bounds for the ESP32 numeric measurement fixture.
 * @details Classic ESP32, single-precision FPU; synthetic signals in a mainloop.
 * All periods are powers of two up to 256 samples, so every tone lies exactly
 * on a bin of each FFT size from 256 upwards.
 * @par Usage Include from the fixture; rebuild after changing constants. */
#pragma once
#include <stdint.h>
/** @brief Binary UART baud rate, matching the Studio configuration.
 * @details 4 x 1000 frames/s x 13 byte x 10 bit = 520 kbit/s, about 56 % of 921600. */
constexpr uint32_t MEASUREMENT_BAUD = 921600u;
/** @brief UART driver transmit buffer, about 18 ms of data at full rate. */
constexpr uint32_t MEASUREMENT_TX_BUFFER = 1024u;
/** @brief Logical sample period, 1 ms = 1000 samples/s per signal.
 * @note The wire timestamp has 1 ms resolution; faster grids would collide. */
constexpr uint32_t SAMPLE_PERIOD_MS = 1u;
/** @brief Number of transmitted channels (IDs 0..3). */
constexpr uint32_t MEASUREMENT_CHANNEL_COUNT = 4u;
/** @brief Sine period in samples, 62.5 Hz. */
constexpr uint32_t SINE_PERIOD_SAMPLES = 16u;
/** @brief Sawtooth period in samples, 7.8125 Hz. */
constexpr uint32_t SAW_PERIOD_SAMPLES = 128u;
/** @brief Multi-tone base period in samples, 31.25 Hz (+125 Hz, +250 Hz). */
constexpr uint32_t TONE_PERIOD_SAMPLES = 32u;
/** @brief Square period in samples, 15.625 Hz. */
constexpr uint32_t SQUARE_PERIOD_SAMPLES = 64u;
/** @brief Peak amplitude of the uniform pseudo-noise added to the square. */
constexpr float SQUARE_NOISE_AMPLITUDE = 0.1f;
