/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file network_demo.cpp
 * @brief ESP32 TCP sine and public MQTT sawtooth demonstration, each 10 Hz.
 * @details Two independent Kymo runtimes own their fixed transmission buffers.
 * Transport callbacks synchronously consume bytes; none retain core pointers.
 * @warning Arduino WiFi and PubSubClient use heap internally. Connect/DNS/write
 * calls can block: this SDK adapter is a demonstration, not hard real time.
 */
#include <kymo_features.h>
#include <kymo_version.h>
#if KYMO_CONFIG_API_VERSION != 1
#error "KymoProbe network demo requires KymoCore configuration API version 1"
#endif
#if defined(KYMO_DEMO_NETWORK_ENABLE) && KYMO_DEMO_NETWORK_ENABLE && \
    KYMO_EFFECTIVE_RUNTIME
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <kymo_runtime.h>
#include "network_secrets.h"

static const uint16_t TCP_PORT = 2323u;
static const char MQTT_HOST[] = "broker.hivemq.com";
static const char MQTT_TOPIC[] = "kymotrace/demo/7c92e4a83f16/saw";
static const char MQTT_CLIENT_ID[] = "kymo-7c92e4a83f16-board";
static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;
static WiFiClient mqttSocket;
static PubSubClient mqtt(mqttSocket);
static KymoContext tcpContext;
static KymoContext mqttContext;
static uint32_t tcpLastSampleMs[1];
static uint32_t mqttLastSampleMs[1];
static uint32_t lastMqttAttemptMs;
static uint32_t lastStatusMs;
static bool wasWifiConnected = false;
static bool isReady = false;

/** @brief Read the SDK monotonic tick. @param[in] user Unused.
 * @return Milliseconds modulo 2^32. */
/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *user)
{
    (void)user;
    return millis();
}

/** @brief Generate artificial data. @param[in] user Unused.
 * @param[in] id 2 selects sine, 3 sawtooth. @param[out] out Sample destination.
 * @return One if a sample was written, otherwise zero. */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0u;
    const float phase = static_cast<float>(millis() % 2000u) / 2000.0f;
    (void)user;
    if (out != nullptr) {
        out->value = (id == 2u) ? sinf(phase * 6.283185307f) :
                                2.0f * phase - 1.0f;
        result = 1u;
    }
    return result;
}

/** @brief Copy a stream prefix into TCP. @param[in] user Unused.
 * @param[in] bytes Borrowed frame data. @param[in] length Available bytes.
 * @return Bytes consumed; zero when unavailable. */
/*******************************************************************************
 * writeTcp
 ******************************************************************************/
static uint8_t writeTcp(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t written = 0u;
    (void)user;
    if ((bytes != nullptr) && tcpClient.connected()) {
        written = static_cast<uint8_t>(tcpClient.write(bytes, length));
    }
    return written;
}

/** @brief Publish one complete frame, QoS 0, not retained.
 * @param[in] user Unused. @param[in] bytes Borrowed complete frame.
 * @param[in] length Frame size. @return Entire length or zero for retry. */
/*******************************************************************************
 * writeMqtt
 ******************************************************************************/
static uint8_t writeMqtt(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t written = 0u;
    (void)user;
    if ((bytes != nullptr) && mqtt.connected() &&
        mqtt.publish(MQTT_TOPIC, bytes, length, false)) {
        written = length;
    }
    return written;
}

static const KymoChannel tcpChannels[] = {{100u, 2u, KYMO_FLAG_TIMESTAMP}};
static const KymoChannel mqttChannels[] = {{100u, 3u, KYMO_FLAG_TIMESTAMP}};
static const KymoConfig tcpConfig = {
    tcpChannels, tcpLastSampleMs, readClockMs, nullptr, readSample, nullptr,
    writeTcp, nullptr, nullptr, nullptr, 1u
};
static const KymoConfig mqttConfig = {
    mqttChannels, mqttLastSampleMs, readClockMs, nullptr, readSample, nullptr,
    writeMqtt, nullptr, nullptr, nullptr, 1u
};

/** @brief Print connection diagnostics over USB; never prints WiFi secrets. */
/*******************************************************************************
 * printStatus
 ******************************************************************************/
static void printStatus()
{
    (void)Serial.print("KYMO_WIFI_STATUS=");
    (void)Serial.println(static_cast<int32_t>(WiFi.status()));
    if (WiFi.status() == WL_CONNECTED) {
        (void)Serial.print("KYMO_WIFI_IP=");
        (void)Serial.println(WiFi.localIP());
    }
    (void)Serial.print("KYMO_MQTT_CONNECTED=");
    (void)Serial.println(mqtt.connected() ? 1 : 0);
    (void)Serial.print("KYMO_MQTT_STATE=");
    (void)Serial.println(mqtt.state());
}

/** @brief Advance network sessions; discard pending data across reconnects. */
/*******************************************************************************
 * serviceNetwork
 ******************************************************************************/
static void serviceNetwork()
{
    const bool isWifiConnected = WiFi.status() == WL_CONNECTED;
    const uint32_t nowMs = millis();
    if (isWifiConnected != wasWifiConnected) {
        tcpClient.stop();
        mqttSocket.stop();
        tcpServer.end();
        if (isWifiConnected) {
            tcpServer.begin();
            lastMqttAttemptMs = nowMs - 5000u;
        }
        wasWifiConnected = isWifiConnected;
        printStatus();
    }
    if (isWifiConnected) {
        if (!tcpClient.connected() && tcpServer.hasClient()) {
            tcpClient.stop();
            tcpClient = tcpServer.available();
            (void)tcpClient.setNoDelay(true);
            (void)tcpClient.setTimeout(1u);
            // A fresh stream must start at a complete frame, never a suffix.
            (void)Kymo_Init(&tcpContext, &tcpConfig);
            (void)Serial.println("KYMO_TCP_CONNECTED=1");
        } else if (tcpServer.hasClient()) {
            WiFiClient extraClient = tcpServer.available();
            extraClient.stop();
        }
        if (!mqtt.connected() && (nowMs - lastMqttAttemptMs >= 5000u)) {
            lastMqttAttemptMs = nowMs;
            if (mqtt.connect(MQTT_CLIENT_ID)) {
                (void)Kymo_Init(&mqttContext, &mqttConfig);
            }
        }
        (void)mqtt.loop();
    }
    if (nowMs - lastStatusMs >= 5000u) {
        lastStatusMs = nowMs;
        printStatus();
    }
}

/** @brief Initialize local credentials, SDK transports and the two runtimes. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
    Serial.begin(115200);
    WiFi.persistent(false);
    (void)WiFi.mode(WIFI_STA);
    (void)WiFi.setAutoReconnect(true);
    (void)WiFi.begin(KYMO_WIFI_SSID, KYMO_WIFI_PASSWORD);
    (void)mqttSocket.setTimeout(1u);
    (void)mqtt.setServer(MQTT_HOST, 1883u);
    (void)mqtt.setSocketTimeout(1u);
    isReady = (Kymo_Init(&tcpContext, &tcpConfig) == KYMO_OK);
    isReady = (Kymo_Init(&mqttContext, &mqttConfig) == KYMO_OK) && isReady;
    (void)Serial.println(isReady ? "KYMO_NETWORK_READY=1" : "KYMO_NETWORK_READY=0");
}

/** @brief Service connections and sample connected channels cooperatively. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop()
{
    serviceNetwork();
    if (isReady && wasWifiConnected) {
        if (tcpClient.connected()) {
            (void)Kymo_Main(&tcpContext);
        }
        if (mqtt.connected()) {
            (void)Kymo_Main(&mqttContext);
        }
    }
    yield();
}
#endif
