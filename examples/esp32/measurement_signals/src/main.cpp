/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief Two numeric signals for live KymoStudio measurements.
 * @details Arduino SDK calls are confined to this board adapter. Fixed transmit
 * storage retains partial writes. Late generator slots are skipped with time
 * gaps; no artificial backlog is replayed. No text shares the binary UART.
 * @note Arduino allocates SDK resources during Serial initialization only. */
#include <Arduino.h>
#include <kymo_protocol.h>
#include "Waveforms.h"

static uint8_t transmitBytes[2u * KYMO_FRAME_MAX_SIZE] = {};
static size_t transmitLength = 0u;
static size_t transmitOffset = 0u;
static uint32_t epochMs = 0u;
static uint32_t lastGeneratorMs = 0u;

/** @brief Encode both channels into owned static transmit storage.
 * @param[in] pair Synthetic signal values with a shared logical timestamp.
 * @return True if both frames were successfully encoded. */
/*******************************************************************************
 * prepareMeasurementFrames
 ******************************************************************************/
static bool prepareMeasurementFrames(const MeasurementPair_T &pair)
{
    KymoDataPoint point = {};
    size_t firstLength = 0u;
    size_t secondLength = 0u;
    bool isEncoded = false;
    point.flags = KYMO_FLAG_TIMESTAMP;
    point.timestamp_ms = pair.timestampMs;
    point.id = 0u;
    point.value = pair.sine;
    firstLength = kymo_encode_data(&point, transmitBytes, sizeof(transmitBytes));
    if (firstLength != 0u) {
        point.id = 1u;
        point.value = pair.sawtooth;
        secondLength = kymo_encode_data(&point, transmitBytes + firstLength,
                                       sizeof(transmitBytes) - firstLength);
        isEncoded = secondLength != 0u;
    }
    transmitLength = isEncoded ? firstLength + secondLength : 0u;
    transmitOffset = 0u;
    return isEncoded;
}

/** @brief Advance pending UART bytes without exceeding SDK transmit capacity. */
/*******************************************************************************
 * flushMeasurementFrames
 ******************************************************************************/
static void flushMeasurementFrames(void)
{
    const int32_t available = Serial.availableForWrite();
    size_t remaining = transmitLength - transmitOffset;
    size_t written = 0u;
    if ((remaining != 0u) && (available > 0)) {
        if (remaining > static_cast<size_t>(available)) {
            remaining = static_cast<size_t>(available);
        }
        written = Serial.write(transmitBytes + transmitOffset, remaining);
        if (written <= remaining) {
            transmitOffset += written;
        }
    }
}

/** @brief Initialize the board UART and generator epoch. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup(void)
{
    Serial.begin(MEASUREMENT_BAUD);
    epochMs = millis();
    lastGeneratorMs = 0u;
}

/** @brief Generate at most one pair per pass, then progress its transmission.
 * @details Timestamps name the logical synthetic grid, not ADC sample times.
 * Missed grid intervals remain visible as timestamp gaps in Studio. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    const uint32_t elapsedMs = millis() - epochMs;
    const uint32_t dueMs = elapsedMs - lastGeneratorMs;
    MeasurementPair_T pair = {};
    bool isPrepared = false;
    flushMeasurementFrames();
    if ((transmitOffset == transmitLength) && (dueMs >= SAMPLE_PERIOD_MS)) {
        lastGeneratorMs += (dueMs / SAMPLE_PERIOD_MS) * SAMPLE_PERIOD_MS;
        pair = calculateMeasurementPair(lastGeneratorMs);
        isPrepared = prepareMeasurementFrames(pair);
        if (isPrepared) {
            flushMeasurementFrames();
        }
    }
}
