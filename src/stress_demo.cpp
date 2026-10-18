/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file stress_demo.cpp
 * @brief Configurable ESP32 load generator using all 256 protocol channels.
 * @details 32-bit Arduino ESP32 superloop; integer waveform generation also
 * supports ESP32 variants without an FPU. The application owns the output batch
 * and copies borrowed core bytes before returning. No application heap usage.
 * @warning The Arduino Serial/WiFi adapters allocate internally; TCP writes
 * may block under backpressure. This SDK adapter is not a hard real-time driver.
 */
#include <KymoCore.h>
#include "demo_selection.h"
#if KYMO_CONFIG_API_VERSION != 2
#error "KymoProbe stress demo requires KymoCore configuration API version 2"
#endif
#if defined(KYMO_STRESS_ENABLE) && KYMO_STRESS_ENABLE && KYMO_EFFECTIVE_RUNTIME
#include <Arduino.h>
#include <string.h>

#ifndef KYMO_STRESS_CHANNEL_COUNT
/** @brief Number of unique wire IDs, 1..256; override with a build definition. */
#define KYMO_STRESS_CHANNEL_COUNT 256
#endif
#ifndef KYMO_STRESS_PERIOD_MS
/** @brief Per-channel interval; zero sends as fast as the transport permits. */
#define KYMO_STRESS_PERIOD_MS 0
#endif
#ifndef KYMO_STRESS_BAUD
/** @brief USB UART baud rate; must match the Studio connection. */
#define KYMO_STRESS_BAUD 2000000
#endif
#ifndef KYMO_STRESS_TCP_ENABLE
/** @brief Select TCP (1) or USB UART (0). */
#define KYMO_STRESS_TCP_ENABLE 0
#endif
#ifndef KYMO_STRESS_XYZ_ENABLE
/** @brief Include X and Z in addition to Y and the millisecond timestamp. */
#define KYMO_STRESS_XYZ_ENABLE 0
#endif

#if (KYMO_STRESS_CHANNEL_COUNT < 1) || (KYMO_STRESS_CHANNEL_COUNT > 256)
#error "KYMO_STRESS_CHANNEL_COUNT must be 1..256"
#endif
#if KYMO_STRESS_CHANNEL_COUNT > KYMO_MAX_CHANNELS
#error "Set KYMO_MAX_CHANNELS to at least KYMO_STRESS_CHANNEL_COUNT"
#endif
#if (KYMO_STRESS_PERIOD_MS < 0) || (KYMO_STRESS_PERIOD_MS > 2147483647)
#error "KYMO_STRESS_PERIOD_MS must be 0..INT32_MAX"
#endif
#if (KYMO_STRESS_BAUD < 1)
#error "KYMO_STRESS_BAUD must be positive"
#endif
#if ((KYMO_STRESS_TCP_ENABLE != 0) && (KYMO_STRESS_TCP_ENABLE != 1)) || \
    ((KYMO_STRESS_XYZ_ENABLE != 0) && (KYMO_STRESS_XYZ_ENABLE != 1))
#error "Stress feature switches must be 0 or 1"
#endif
#if !KYMO_ENABLE_RUNTIME || !KYMO_ENABLE_TIMESTAMP
#error "Stress demo requires KYMO_ENABLE_RUNTIME=1 and KYMO_ENABLE_TIMESTAMP=1"
#endif
#if KYMO_STRESS_XYZ_ENABLE && (!KYMO_ENABLE_X || !KYMO_ENABLE_Z)
#error "XYZ stress demo requires KYMO_ENABLE_X=1 and KYMO_ENABLE_Z=1"
#endif
#if defined(KYMO_DEMO_NETWORK_ENABLE) && KYMO_DEMO_NETWORK_ENABLE
#error "Select either the network demo or the stress demo"
#endif

#if KYMO_STRESS_TCP_ENABLE
#include <WiFi.h>
#include "network_secrets.h"
static WiFiServer s_tcpServer(2323u);
static WiFiClient s_tcpClient;
static bool s_wasWifiConnected = false;
#endif

static const uint16_t OUTPUT_CAPACITY = 4096u;
static const uint16_t MAIN_CALLS_PER_LOOP = 512u;
static KymoContext s_context;
static KymoChannel s_channels[KYMO_STRESS_CHANNEL_COUNT];
static uint32_t s_sampleCounts[KYMO_STRESS_CHANNEL_COUNT];
static uint8_t s_output[OUTPUT_CAPACITY];
static uint16_t s_outputLength = 0u;
static uint16_t s_outputOffset = 0u;
static bool s_isReady = false;

/** @brief Read monotonic milliseconds. @param[in] user Unused.
 * @return SDK tick modulo 2^32. */
/*******************************************************************************
 * readStressClockMs
 ******************************************************************************/
static uint32_t readStressClockMs(void *user)
{
    (void)user;
    return millis();
}

/** @brief Generate distinct bounded ramps, without expensive trigonometry.
 * @param[in] user Unused. @param[in] id Channel ID.
 * @param[out] out Coordinates in [-1, 1]. @return One on success, zero otherwise.
 * @note Not ISR-safe. Each channel advances only when actually sampled. */
/*******************************************************************************
 * readStressSample
 ******************************************************************************/
static uint8_t readStressSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0u;
    uint32_t phase = 0u;
    bool isValidId = true;
    (void)user;
#if KYMO_STRESS_CHANNEL_COUNT < 256
    isValidId = id < KYMO_STRESS_CHANNEL_COUNT;
#endif
    if ((out != nullptr) && isValidId) {
        phase = (s_sampleCounts[id] + static_cast<uint32_t>(id) * 17u) % 2048u;
        out->value = static_cast<float>(phase) / 1023.5f - 1.0f;
#if KYMO_STRESS_XYZ_ENABLE
        out->x = static_cast<float>((phase + 512u) % 2048u) / 1023.5f - 1.0f;
        out->z = static_cast<float>((phase + 1024u) % 2048u) / 1023.5f - 1.0f;
#endif
        ++s_sampleCounts[id];
        result = 1u;
    }
    return result;
}

/** @brief Copy one core frame into application-owned batch storage.
 * @param[in] user Unused. @param[in] bytes Borrowed frame bytes.
 * @param[in] length Byte count. @return Length when copied, zero if full. */
/*******************************************************************************
 * enqueueStressFrame
 ******************************************************************************/
static uint8_t enqueueStressFrame(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t written = 0u;
    (void)user;
    if ((bytes != nullptr) && (length <= OUTPUT_CAPACITY - s_outputLength)) {
        (void)memcpy(s_output + s_outputLength, bytes, length);
        s_outputLength = static_cast<uint16_t>(s_outputLength + length);
        written = length;
    }
    return written;
}

/** @brief Apply backpressure before sampling when a full frame cannot fit.
 * @param[in] user Unused. @return One when the batch has insufficient space. */
/*******************************************************************************
 * isStressOutputBusy
 ******************************************************************************/
static uint8_t isStressOutputBusy(void *user)
{
    (void)user;
    return static_cast<uint8_t>(static_cast<uint16_t>(OUTPUT_CAPACITY - s_outputLength) < KYMO_FRAME_MAX_SIZE);
}

static KymoConfig makeStressConfig()
{
    KymoConfig config = {};
    config.channels = s_channels;
    config.channel_count = KYMO_STRESS_CHANNEL_COUNT;
    config.clock_ms = readStressClockMs;
    config.sample = readStressSample;
    config.write = enqueueStressFrame;
    config.busy = isStressOutputBusy;
    return config;
}
static const KymoConfig STRESS_CONFIG = makeStressConfig();

/** @brief Restart at a frame boundary, discarding stale data after reconnect. */
/*******************************************************************************
 * resetStressStream
 ******************************************************************************/
static void resetStressStream()
{
    s_outputLength = 0u;
    s_outputOffset = 0u;
    (void)memset(s_sampleCounts, 0, sizeof(s_sampleCounts));
    s_isReady = Kymo_Init(&s_context, &STRESS_CONFIG) == KYMO_OK;
}

/** @brief Consume a batch prefix once, preserving unsent bytes for the next loop.
 * @note Not ISR-safe. UART writes never exceed available TX space. */
/*******************************************************************************
 * flushStressOutput
 ******************************************************************************/
static void flushStressOutput()
{
    uint16_t remaining = static_cast<uint16_t>(s_outputLength - s_outputOffset);
    uint16_t written = 0u;
#if !KYMO_STRESS_TCP_ENABLE
    int32_t available = static_cast<int32_t>(Serial.availableForWrite());
#endif
    if (remaining > 0u) {
#if KYMO_STRESS_TCP_ENABLE
        if (s_tcpClient.connected()) {
            written = static_cast<uint16_t>(s_tcpClient.write(s_output + s_outputOffset, remaining));
        }
#else
        if (available > 0) {
            if (static_cast<uint32_t>(available) < remaining) {
                remaining = static_cast<uint16_t>(available);
            }
            written = static_cast<uint16_t>(Serial.write(s_output + s_outputOffset, remaining));
        }
#endif
        s_outputOffset = static_cast<uint16_t>(s_outputOffset + written);
        if (s_outputOffset == s_outputLength) {
            s_outputLength = 0u;
            s_outputOffset = 0u;
        }
    }
}

#if KYMO_STRESS_TCP_ENABLE
/** @brief Accept one TCP client; reset the stream after WiFi or TCP reconnect. */
/*******************************************************************************
 * serviceStressNetwork
 ******************************************************************************/
static void serviceStressNetwork()
{
    const bool isWifiConnected = WiFi.status() == WL_CONNECTED;
    WiFiClient extraClient;
    if (isWifiConnected != s_wasWifiConnected) {
        s_tcpClient.stop();
        s_tcpServer.end();
        resetStressStream();
        if (isWifiConnected) {
            s_tcpServer.begin();
            (void)Serial.print("KYMO_STRESS_IP=");
            (void)Serial.println(WiFi.localIP());
        }
        s_wasWifiConnected = isWifiConnected;
    }
    if (isWifiConnected && s_tcpServer.hasClient()) {
        if (!s_tcpClient.connected()) {
            s_tcpClient.stop();
            s_tcpClient = s_tcpServer.available();
            (void)s_tcpClient.setNoDelay(true);
            (void)s_tcpClient.setTimeout(1u);
            resetStressStream();
        } else {
            extraClient = s_tcpServer.available();
            extraClient.stop();
        }
    }
}
#endif

/** @brief Initialize the channel table and selected SDK transport. */
/*******************************************************************************
 * demoStressInit
 ******************************************************************************/
void demoStressInit()
{
    for (uint16_t channelIndex = 0u; channelIndex < KYMO_STRESS_CHANNEL_COUNT; ++channelIndex) {
        s_channels[channelIndex].id = static_cast<uint8_t>(channelIndex);
        s_channels[channelIndex].period_ms = KYMO_STRESS_PERIOD_MS;
        s_channels[channelIndex].flags = KYMO_FLAG_TIMESTAMP;
#if KYMO_STRESS_XYZ_ENABLE
        s_channels[channelIndex].flags = KYMO_FLAG_TIMESTAMP | KYMO_FLAG_X | KYMO_FLAG_Z;
#endif
    }
#if KYMO_STRESS_TCP_ENABLE
    Serial.begin(115200u);
    WiFi.persistent(false);
    (void)WiFi.mode(WIFI_STA);
    (void)WiFi.setAutoReconnect(true);
    (void)WiFi.setSleep(false);
    (void)WiFi.begin(KYMO_WIFI_SSID, KYMO_WIFI_PASSWORD);
    (void)Serial.println("KYMO_STRESS_TCP_PORT=2323");
#else
    // Allocate the SDK UART TX ring only at startup; no text on the binary port.
    (void)Serial.setTxBufferSize(8192u);
    Serial.begin(KYMO_STRESS_BAUD);
#endif
    resetStressStream();
}

/** @brief Advance a bounded number of round-robin samples and yield to the SDK. */
/*******************************************************************************
 * demoStressMain
 ******************************************************************************/
void demoStressMain()
{
    bool canSend = s_isReady;
    KymoStatus status = KYMO_OK;
#if KYMO_STRESS_TCP_ENABLE
    serviceStressNetwork();
    canSend = s_isReady && s_wasWifiConnected && s_tcpClient.connected();
#endif
    if (canSend) {
        flushStressOutput();
        for (uint16_t callIndex = 0u;
             (callIndex < MAIN_CALLS_PER_LOOP) && (status == KYMO_OK); ++callIndex) {
            status = Kymo_Main(&s_context);
        }
        flushStressOutput();
    }
    yield();
}
#endif
