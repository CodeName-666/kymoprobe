/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief Four 1-kHz numeric signals for live KymoStudio load tests.
 * @details Arduino SDK calls are confined to this board adapter. Fixed transmit
 * storage retains partial writes. Late generator slots are skipped with time
 * gaps; no artificial backlog is replayed. No text shares the binary UART.
 * @note Arduino allocates SDK resources during Serial initialization only. */
#include <Arduino.h>
#include <KymoCore.h>
#include "Waveforms.h"

static uint8_t transmitBytes[MEASUREMENT_CHANNEL_COUNT * KYMO_FRAME_MAX_SIZE] = {};
static size_t transmitLength = 0u;
static size_t transmitOffset = 0u;
static uint32_t epochMs = 0u;
static uint32_t lastGeneratorMs = 0u;

/** @brief Encode all channels into owned static transmit storage.
 * @param[in] set Synthetic signal values with a shared logical timestamp.
 * @return True if all frames were successfully encoded. */
/*******************************************************************************
 * prepareMeasurementFrames
 ******************************************************************************/
static bool prepareMeasurementFrames(const MeasurementSet_T &set)
{
    KymoDataPoint point = {};
    size_t length = 0u;
    size_t frameLength = 0u;
    bool isEncoded = true;
    point.flags = KYMO_FLAG_TIMESTAMP;
    point.timestamp_ms = set.timestampMs;
    for (uint32_t channel = 0u; (channel < MEASUREMENT_CHANNEL_COUNT) && isEncoded; ++channel) {
        point.id = static_cast<uint8_t>(channel);
        point.value = set.values[channel];
        frameLength = KymoProtocol_Encode(&point, transmitBytes + length, sizeof(transmitBytes) - length);
        isEncoded = frameLength != 0u;
        length += frameLength;
    }
    transmitLength = isEncoded ? length : 0u;
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
    Serial.setTxBufferSize(MEASUREMENT_TX_BUFFER);
    Serial.begin(MEASUREMENT_BAUD);
    epochMs = millis();
    lastGeneratorMs = 0u;
}

/** @brief Generate at most one set per pass, then progress its transmission.
 * @details Timestamps name the logical synthetic grid, not ADC sample times.
 * Missed grid intervals remain visible as timestamp gaps in Studio. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    const uint32_t elapsedMs = millis() - epochMs;
    const uint32_t dueMs = elapsedMs - lastGeneratorMs;
    MeasurementSet_T set = {};
    bool isPrepared = false;
    flushMeasurementFrames();
    if ((transmitOffset == transmitLength) && (dueMs >= SAMPLE_PERIOD_MS)) {
        lastGeneratorMs += (dueMs / SAMPLE_PERIOD_MS) * SAMPLE_PERIOD_MS;
        set = calculateMeasurementSet(lastGeneratorMs);
        isPrepared = prepareMeasurementFrames(set);
        if (isPrepared) {
            flushMeasurementFrames();
        }
    }
}
