/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief Stack reserve of three FreeRTOS tasks as normal measurement signals.
 * @details Every 500 ms the Arduino loop task reads uxTaskGetStackHighWaterMark()
 * of each task and sends the minimum free stack since task start as a Kymo DATA
 * frame. On ESP-IDF the value is in bytes (vanilla FreeRTOS returns words).
 * The parser task deepens a recursion by one level per second up to a cap, so
 * its reserve falls in steps and then stays flat; a high-water mark never rises.
 * The scan costs time proportional to the unused stack, so it runs at 2 Hz in
 * the lowest-priority task, never in an ISR or a time-critical path.
 * @note Arduino allocates SDK resources during Serial initialization only. */
#include <Arduino.h>
#include <KymoCore.h>

static const uint32_t STACK_BAUD = 115200u;
static const uint32_t REPORT_PERIOD_MS = 500u;
static const uint32_t SENSOR_STACK_BYTES = 2048u;
static const uint32_t PARSER_STACK_BYTES = 4096u;
static const uint32_t PARSER_MAX_DEPTH = 24u;  // ~110 bytes per level, leaves ~1 KiB reserve
static const uint32_t SIGNAL_COUNT = 4u;

static TaskHandle_t loopHandle = nullptr;
static TaskHandle_t sensorHandle = nullptr;
static TaskHandle_t parserHandle = nullptr;
static volatile uint32_t parserDepth = 0u;
static uint32_t lastReportMs = 0u;

/** @brief Use one stack frame per level; volatile keeps the frame from being optimized away.
 * @param[in] depth Remaining recursion levels. @return Checksum that depends on every frame. */
/*******************************************************************************
 * consumeStack
 ******************************************************************************/
static uint32_t __attribute__((noinline)) consumeStack(uint32_t depth)
{
    volatile uint8_t frame[64] = {};
    uint32_t sum = depth;
    frame[0] = static_cast<uint8_t>(depth);
    if (depth > 1u) {
        sum += consumeStack(depth - 1u);
    }
    frame[63] = static_cast<uint8_t>(sum);
    return sum + frame[0] + frame[63];
}

/** @brief Light, constant stack use: a 100-Hz pseudo sensor. @param[in] arg Unused. */
/*******************************************************************************
 * sensorTask
 ******************************************************************************/
static void sensorTask(void *arg)
{
    volatile float filtered = 0.0f;
    (void)arg;
    for (;;) {
        filtered = (0.9f * filtered) + (0.1f * static_cast<float>(micros() % 1000u));
        vTaskDelay(pdMS_TO_TICKS(10u));
    }
}

/** @brief Stack use that grows by one recursion level per second up to the cap. @param[in] arg Unused. */
/*******************************************************************************
 * parserTask
 ******************************************************************************/
static void parserTask(void *arg)
{
    volatile uint32_t checksum = 0u;
    (void)arg;
    for (;;) {
        if (parserDepth < PARSER_MAX_DEPTH) {
            parserDepth = parserDepth + 1u;
        }
        checksum = consumeStack(parserDepth);
        vTaskDelay(pdMS_TO_TICKS(1000u));
    }
}

/** @brief Encode and send one signal with the shared timestamp.
 * @param[in] id Signal identifier. @param[in] value Value. @param[in] nowMs Device time. */
/*******************************************************************************
 * sendSignal
 ******************************************************************************/
static void sendSignal(uint8_t id, float value, uint32_t nowMs)
{
    uint8_t frame[KYMO_FRAME_MAX_SIZE] = {};
    KymoDataPoint point = {};
    size_t length = 0u;
    point.id = id;
    point.value = value;
    point.flags = KYMO_FLAG_TIMESTAMP;
    point.timestamp_ms = nowMs;
    length = KymoProtocol_Encode(&point, frame, sizeof(frame));
    if (length != 0u) {
        (void)Serial.write(frame, length);
    }
}

/** @brief Start the UART and the two worker tasks; setup() runs in the loop task. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup(void)
{
    Serial.setTxBufferSize(SIGNAL_COUNT * KYMO_FRAME_MAX_SIZE * 4u);
    Serial.begin(STACK_BAUD);
    loopHandle = xTaskGetCurrentTaskHandle();
    (void)xTaskCreate(sensorTask, "sensor", SENSOR_STACK_BYTES, nullptr, 2u, &sensorHandle);
    (void)xTaskCreate(parserTask, "parser", PARSER_STACK_BYTES, nullptr, 2u, &parserHandle);
    lastReportMs = millis();
}

/** @brief Every 500 ms: free stack bytes of loop (0), sensor (1), parser (2) and the parser depth (3). */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    const uint32_t nowMs = millis();
    if ((nowMs - lastReportMs) >= REPORT_PERIOD_MS) {
        lastReportMs += REPORT_PERIOD_MS;
        sendSignal(0u, static_cast<float>(uxTaskGetStackHighWaterMark(loopHandle)), nowMs);
        if (sensorHandle != nullptr) {
            sendSignal(1u, static_cast<float>(uxTaskGetStackHighWaterMark(sensorHandle)), nowMs);
        }
        if (parserHandle != nullptr) {
            sendSignal(2u, static_cast<float>(uxTaskGetStackHighWaterMark(parserHandle)), nowMs);
        }
        sendSignal(3u, static_cast<float>(parserDepth), nowMs);
    }
    vTaskDelay(pdMS_TO_TICKS(10u));
}
