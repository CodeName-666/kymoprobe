/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/** @file main.cpp @brief PI temperature controller whose gains and setpoint KymoStudio tunes live.
 * @details A simulated heater (first-order thermal model) is controlled by a PI
 * controller at 100 Hz. Temperature, heater output and setpoint go out at 50 Hz as
 * timestamped Kymo DATA frames. kp, ki and setpoint are registered with KymoParam:
 * KymoStudio asks for the list when it connects, shows them in its parameter
 * panel and writes new values over the same UART. Every accepted or refused
 * write is answered with a readback of the value now in force.
 * Signal 3 reports the longest back-channel handling time per report period, so
 * the cost of the back channel is visible next to the control loop.
 * @note Arduino allocates SDK resources during Serial initialization only. */
#include <Arduino.h>
#include <KymoCore.h>

static const uint32_t BAUD = 115200u;
static const uint32_t CONTROL_PERIOD_MS = 10u;
static const uint32_t REPORT_PERIOD_MS = 20u;
static const float AMBIENT_C = 20.0f;
static const float HEATER_GAIN_C = 80.0f;   // steady-state rise at full heater power
static const float TIME_CONSTANT_S = 8.0f;
static const float DT_S = 0.01f;

/* Tunable from KymoStudio; names come from the variable names. */
static float kp = 0.20f;
static float ki = 0.02f;
static float setpoint = 45.0f;

static const KymoParameter params[] = {
    KYMO_PARAM(100, kp, 0.0f, 10.0f, ""),
    KYMO_PARAM(101, ki, 0.0f, 2.0f, ""),
    KYMO_PARAM(102, setpoint, 20.0f, 90.0f, "\xC2\xB0" "C"),
};

static KymoParamReceiver receiver;
static float temperature = AMBIENT_C;
static float heater = 0.0f;
static float integral = 0.0f;
static uint32_t lastControlMs = 0u;
static uint32_t lastReportMs = 0u;
static uint32_t backChannelMaxUs = 0u;

/** @brief Encode and send one sample with the device time.
 * @param[in] id Signal identifier. @param[in] value Value. @param[in] nowMs Device time. */
/*******************************************************************************
 * sendSample
 ******************************************************************************/
static void sendSample(uint8_t id, float value, uint32_t nowMs)
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

/** @brief Read what the host sent, apply writes, answer list requests and readbacks.
 * @param[in] nowMs Device time for the readbacks. */
/*******************************************************************************
 * serviceBackChannel
 ******************************************************************************/
static void serviceBackChannel(uint32_t nowMs)
{
    const uint32_t startUs = micros();
    uint8_t rx[32] = {};
    size_t count = 0u;
    uint8_t frame[KYMO_PARAM_FRAME_MAX_SIZE] = {};
    size_t length = 0u;
    KymoDataPoint readback = {};
    while ((Serial.available() > 0) && (count < sizeof(rx))) {
        rx[count] = static_cast<uint8_t>(Serial.read());
        ++count;
    }
    (void)KymoParam_Feed(&receiver, rx, count);
    length = KymoParam_NextDescription(&receiver, frame, sizeof(frame));
    while (length != 0u) {
        (void)Serial.write(frame, length);
        length = KymoParam_NextDescription(&receiver, frame, sizeof(frame));
    }
    while (KymoParam_NextReadback(&receiver, &readback) == KYMO_OK) {
        sendSample(readback.id, readback.value, nowMs);
    }
    const uint32_t elapsedUs = micros() - startUs;
    if (elapsedUs > backChannelMaxUs) {
        backChannelMaxUs = elapsedUs;
    }
}

/** @brief One PI step with anti-windup and one step of the heater model. */
/*******************************************************************************
 * controlStep
 ******************************************************************************/
static void controlStep(void)
{
    const float error = setpoint - temperature;
    const float candidate = integral + (error * DT_S);
    float output = (kp * error) + (ki * candidate);
    if ((output > 0.0f) && (output < 1.0f)) {
        integral = candidate;  // integrate only while the heater is not saturated
    }
    output = constrain(output, 0.0f, 1.0f);
    heater = output;
    temperature += (((heater * HEATER_GAIN_C) - (temperature - AMBIENT_C)) / TIME_CONSTANT_S) * DT_S;
}

/** @brief Start the UART and register the parameter table. */
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup(void)
{
    Serial.setTxBufferSize(1024u);
    Serial.begin(BAUD);
    (void)KymoParam_Init(&receiver, params, KYMO_COUNT_OF(params));
    lastControlMs = millis();
    lastReportMs = lastControlMs;
}

/** @brief Back channel every pass, control at 100 Hz, report at 50 Hz. */
/*******************************************************************************
 * loop
 ******************************************************************************/
void loop(void)
{
    const uint32_t nowMs = millis();
    serviceBackChannel(nowMs);
    if ((nowMs - lastControlMs) >= CONTROL_PERIOD_MS) {
        lastControlMs += CONTROL_PERIOD_MS;
        controlStep();
    }
    if ((nowMs - lastReportMs) >= REPORT_PERIOD_MS) {
        lastReportMs += REPORT_PERIOD_MS;
        sendSample(0u, temperature, nowMs);
        sendSample(1u, heater, nowMs);
        sendSample(2u, setpoint, nowMs);
        sendSample(3u, static_cast<float>(backChannelMaxUs), nowMs);
        backChannelMaxUs = 0u;
    }
}
