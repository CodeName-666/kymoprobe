/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file Config.h @brief Fixed bounds for the ESP32 numeric measurement fixture.
 * @details Classic ESP32, single-precision FPU; synthetic signals in a mainloop.
 * @par Usage Include from the fixture; rebuild after changing constants. */
#pragma once
#include <stdint.h>
/** @brief Binary UART baud rate, matching the Studio configuration. */
constexpr uint32_t MEASUREMENT_BAUD = 460800u;
/** @brief Logical sample period, 5 ms = 200 samples/s per signal. */
constexpr uint32_t SAMPLE_PERIOD_MS = 5u;
/** @brief Number of steps in a sine period, yielding 6.25 Hz. */
constexpr uint32_t SINE_PERIOD_SAMPLES = 32u;
/** @brief Number of steps in a sawtooth period, yielding 1.5625 Hz. */
constexpr uint32_t SAW_PERIOD_SAMPLES = 128u;
