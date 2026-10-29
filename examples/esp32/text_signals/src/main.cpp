/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief Two 100-Hz signals as JSON text lines without timestamps.
 * @details Test firmware for KymoStudio's reception-time path (test case P2):
 * each line is {"id":n,"value":v}, so Studio must fall back to reception time
 * and refuse time-dependent analysis. No KymoCore framing, no CRC.
 * @note Arduino allocates SDK resources during Serial initialization only. */
#include <Arduino.h>
#include <math.h>
#include <stdio.h>

static const uint32_t TEXT_BAUD = 115200u;
static const uint32_t SAMPLE_PERIOD_MS = 10u;
static const float TWO_PI_F = 6.283185307f;
static uint32_t lastSampleMs = 0u;
static uint32_t sampleIndex = 0u;

/** @brief Send one JSON measurement line.
 * @param[in] id Signal identifier. @param[in] value Measured value. */
/*******************************************************************************
 * sendLine
 ******************************************************************************/
static void sendLine(uint32_t id, float value)
{
    char line[48] = {};
    const int length = snprintf(line, sizeof(line), "{\"id\":%lu,\"value\":%.5f}\n",
                                static_cast<unsigned long>(id), static_cast<double>(value));
    if ((length > 0) && (static_cast<size_t>(length) < sizeof(line))) {
        (void)Serial.write(reinterpret_cast<const uint8_t *>(line), static_cast<size_t>(length));
    }
}

/** @brief Initialize the board UART. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup(void)
{
    Serial.begin(TEXT_BAUD);
    lastSampleMs = millis();
}

/** @brief Every 10 ms send a 1-Hz sine (id 0) and a 0.5-Hz sawtooth (id 1). */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    const uint32_t nowMs = millis();
    float phase = 0.0f;
    if ((nowMs - lastSampleMs) >= SAMPLE_PERIOD_MS) {
        lastSampleMs += SAMPLE_PERIOD_MS;
        phase = static_cast<float>(sampleIndex % 100u) / 100.0f;
        sendLine(0u, sinf(phase * TWO_PI_F));
        sendLine(1u, static_cast<float>(sampleIndex % 200u) / 100.0f - 1.0f);
        ++sampleIndex;
    }
}
