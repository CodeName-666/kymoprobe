/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file network_demo.cpp
 * @brief Selectable ESP32 TCP, MQTT or combined waveform application.
 * @details Each enabled transport owns a separate fixed runtime buffer.
 * @warning Arduino WiFi and PubSubClient use SDK heap and synchronous calls.
 * Connections, DNS and writes can block; this adapter is not hard real time.
 */
#include "demo_selection.h"
#include <KymoCore.h>
#if KYMO_DEMO_NETWORK_ENABLE && KYMO_EFFECTIVE_RUNTIME
#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
#include "network_secrets.h"
#if KYMO_DEMO_MQTT_ENABLE
#include <PubSubClient.h>
static const char MQTT_HOST[] = "broker.hivemq.com";
static const char MQTT_TOPIC[] = "kymotrace/demo/7c92e4a83f16/saw";
static const char MQTT_CLIENT_ID[] = "kymo-7c92e4a83f16-board";
static WiFiClient mqttSocket;
static PubSubClient mqtt(mqttSocket);
static KymoContext mqttContext;
static uint32_t lastMqttAttemptMs;
#endif
#if KYMO_DEMO_TCP_ENABLE
static const uint16_t TCP_PORT = 2323u;
static WiFiServer tcpServer(TCP_PORT);
static WiFiClient tcpClient;
static KymoContext tcpContext;
#endif
static uint32_t lastStatusMs;
static bool wasWifiConnected = false;
static bool isReady = false;

/** @brief Read monotonic milliseconds. @param[in] user Unused.
 * @return Milliseconds modulo 2^32. */
/*******************************************************************************
 * readClockMs
 ******************************************************************************/
static uint32_t readClockMs(void *user)
{
    (void)user;
    return millis();
}

/** @brief Generate a waveform. @param[in] user Unused.
 * @param[in] id 2 selects sine, 3 sawtooth. @param[out] out Sample destination.
 * @return One when a sample was written, otherwise zero. */
/*******************************************************************************
 * readSample
 ******************************************************************************/
static uint8_t readSample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 0u;
    const float phase = static_cast<float>(millis() % 2000u) / 2000.0f;
    (void)user;
    if (out != nullptr) {
        out->value = id == 2u ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
        result = 1u;
    }
    return result;
}

#if KYMO_DEMO_TCP_ENABLE
/** @brief Copy a TCP prefix. @param[in] user Unused.
 * @param[in] bytes Borrowed bytes. @param[in] length Available bytes.
 * @return Bytes consumed, or zero when disconnected. */
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
static const KymoChannel tcpChannels[] = {{100u, 2u, KYMO_FLAG_TIMESTAMP}};
static KymoConfig makeTcpConfig()
{
    KymoConfig config = {};
    config.channels = tcpChannels;
    config.channel_count = 1u;
    config.clock_ms = readClockMs;
    config.sample = readSample;
    config.write = writeTcp;
    return config;
}
static const KymoConfig tcpConfig = makeTcpConfig();

/** @brief Accept one client and reset new streams to a complete frame. */
/*******************************************************************************
 * serviceTcp
 ******************************************************************************/
static void serviceTcp(void)
{
    WiFiClient extraClient;
    if (tcpServer.hasClient()) {
        if (!tcpClient.connected()) {
            tcpClient.stop();
            tcpClient = tcpServer.available();
            (void)tcpClient.setNoDelay(true);
            (void)tcpClient.setTimeout(1u);
            (void)Kymo_Init(&tcpContext, &tcpConfig);
            (void)Serial.println("KYMO_TCP_CONNECTED=1");
        } else {
            extraClient = tcpServer.available();
            extraClient.stop();
        }
    }
}
#endif

#if KYMO_DEMO_MQTT_ENABLE
/** @brief Publish one complete frame. @param[in] user Unused.
 * @param[in] bytes Borrowed frame. @param[in] length Frame length.
 * @return Whole length on success, zero for retry. */
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
static const KymoChannel mqttChannels[] = {{100u, 3u, KYMO_FLAG_TIMESTAMP}};
static KymoConfig makeMqttConfig()
{
    KymoConfig config = {};
    config.channels = mqttChannels;
    config.channel_count = 1u;
    config.clock_ms = readClockMs;
    config.sample = readSample;
    config.write = writeMqtt;
    return config;
}
static const KymoConfig mqttConfig = makeMqttConfig();

/** @brief Retry MQTT every five seconds and service its connection. */
/*******************************************************************************
 * serviceMqtt
 ******************************************************************************/
static void serviceMqtt(void)
{
    const uint32_t nowMs = millis();
    if (!mqtt.connected() && (nowMs - lastMqttAttemptMs >= 5000u)) {
        lastMqttAttemptMs = nowMs;
        if (mqtt.connect(MQTT_CLIENT_ID)) {
            (void)Kymo_Init(&mqttContext, &mqttConfig);
        }
    }
    (void)mqtt.loop();
}
#endif

/** @brief Print USB connection status without disclosing credentials. */
/*******************************************************************************
 * printStatus
 ******************************************************************************/
static void printStatus(void)
{
    (void)Serial.print("KYMO_WIFI_STATUS=");
    (void)Serial.println(static_cast<int32_t>(WiFi.status()));
    if (WiFi.status() == WL_CONNECTED) {
        (void)Serial.print("KYMO_WIFI_IP=");
        (void)Serial.println(WiFi.localIP());
    }
#if KYMO_DEMO_MQTT_ENABLE
    (void)Serial.print("KYMO_MQTT_CONNECTED=");
    (void)Serial.println(mqtt.connected() ? 1 : 0);
    (void)Serial.print("KYMO_MQTT_STATE=");
    (void)Serial.println(mqtt.state());
#endif
}

/** @brief Advance WiFi and selected transport connections. */
/*******************************************************************************
 * serviceNetwork
 ******************************************************************************/
static void serviceNetwork(void)
{
    const bool isWifiConnected = WiFi.status() == WL_CONNECTED;
    const uint32_t nowMs = millis();
    if (isWifiConnected != wasWifiConnected) {
#if KYMO_DEMO_TCP_ENABLE
        tcpClient.stop();
        tcpServer.end();
        if (isWifiConnected) {
            tcpServer.begin();
        }
#endif
#if KYMO_DEMO_MQTT_ENABLE
        mqttSocket.stop();
        lastMqttAttemptMs = nowMs - 5000u;
#endif
        wasWifiConnected = isWifiConnected;
        printStatus();
    }
    if (isWifiConnected) {
#if KYMO_DEMO_TCP_ENABLE
        serviceTcp();
#endif
#if KYMO_DEMO_MQTT_ENABLE
        serviceMqtt();
#endif
    }
    if (nowMs - lastStatusMs >= 5000u) {
        lastStatusMs = nowMs;
        printStatus();
    }
}

/** @brief Initialize WiFi, selected transports and fixed runtimes. */
/*******************************************************************************
 * demoNetworkInit
 ******************************************************************************/
void demoNetworkInit(void)
{
    Serial.begin(115200u);
    WiFi.persistent(false);
    (void)WiFi.mode(WIFI_STA);
    (void)WiFi.setAutoReconnect(true);
    (void)WiFi.begin(KYMO_WIFI_SSID, KYMO_WIFI_PASSWORD);
    isReady = true;
#if KYMO_DEMO_TCP_ENABLE
    isReady = (Kymo_Init(&tcpContext, &tcpConfig) == KYMO_OK) && isReady;
#endif
#if KYMO_DEMO_MQTT_ENABLE
    (void)mqttSocket.setTimeout(1u);
    (void)mqtt.setServer(MQTT_HOST, 1883u);
    (void)mqtt.setSocketTimeout(1u);
    isReady = (Kymo_Init(&mqttContext, &mqttConfig) == KYMO_OK) && isReady;
#endif
    (void)Serial.println(isReady ? "KYMO_NETWORK_READY=1" : "KYMO_NETWORK_READY=0");
}

/** @brief Sample connected channels and yield to the SDK. */
/*******************************************************************************
 * demoNetworkMain
 ******************************************************************************/
void demoNetworkMain(void)
{
    serviceNetwork();
    if (isReady && wasWifiConnected) {
#if KYMO_DEMO_TCP_ENABLE
        if (tcpClient.connected()) {
            (void)Kymo_Main(&tcpContext);
        }
#endif
#if KYMO_DEMO_MQTT_ENABLE
        if (mqtt.connected()) {
            (void)Kymo_Main(&mqttContext);
        }
#endif
    }
    yield();
}
#endif
