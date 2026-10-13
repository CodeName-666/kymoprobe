/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
/** @file features_demo.cpp
 * @brief C++ sender, Y/XY/XYZ, timestamps, CRC and decoder demonstration.
 * @details Sends channels 0..2 as scalar/XY/XYZ and channel 3 as decoder status.
 * Fixed buffers remain application-owned. Arduino Serial uses SDK heap at init.
 */
#include "demo_selection.h"
#include <kymo_features.h>
#if (KYMO_DEMO_MODE == KYMO_DEMO_FEATURES) && KYMO_ENABLED
#include <Arduino.h>
#include <math.h>
#include <kymo.h>
#include <kymo_protocol.h>

/** @brief Copy-only UART adapter for the polymorphic KymoStream interface.
 * @details Virtual dispatch is required by the library's transport contract.
 * No application heap allocation or retained borrowed pointers are used.
 */
class FeatureSerialStream : public KymoStream {
public:
    /** @brief Copy an available prefix into UART.
     * @param[in] bytes Borrowed frame data. @param[in] length Available bytes.
     * @return Copied count, or zero for backpressure/invalid input. */
    /***************************************************************************
     * FeatureSerialStream::write
     **************************************************************************/
    size_t write(const uint8_t *bytes, size_t length) override
    {
        size_t written = 0u;
        const int32_t available = Serial.availableForWrite();
        if ((bytes != nullptr) && (available > 0)) {
            if (length > static_cast<size_t>(available)) {
                length = static_cast<size_t>(available);
            }
            written = Serial.write(bytes, length);
        }
        return written;
    }
};
static FeatureSerialStream featureStream;
static Kymo featureSender;
static uint32_t lastSampleMs = 0u;
static uint8_t nextChannel = 0u;
static bool isDecoderValid = false;

/** @brief Read the SDK millisecond clock. @return Tick modulo 2^32. */
/*******************************************************************************
 * featureClockMs
 ******************************************************************************/
static uint32_t featureClockMs(void)
{
    return millis();
}

/** @brief Verify XYZ/time roundtrip and rejection of a damaged CRC.
 * @return True when valid decoding and corrupted-frame rejection both pass. */
/*******************************************************************************
 * verifyFeatureDecoder
 ******************************************************************************/
static bool verifyFeatureDecoder(void)
{
    KymoDataPoint point = {};
    KymoDataPoint decoded = {};
    uint8_t frame[KYMO_FRAME_MAX_SIZE] = {};
    size_t length = 0u;
    bool isValid = false;
    point.id = 7u;
    point.flags = KYMO_FLAG_X | KYMO_FLAG_Z | KYMO_FLAG_TIMESTAMP;
    point.x = 1.0f;
    point.value = -0.5f;
    point.z = 0.25f;
    point.timestamp_ms = 1234u;
    length = kymo_encode_data(&point, frame, sizeof(frame));
    if (length != 0u) {
        isValid = (kymo_decode_data(frame, length, &decoded) != 0) &&
            (decoded.id == point.id) && (decoded.flags == point.flags) &&
            (decoded.timestamp_ms == point.timestamp_ms) &&
            (fabsf(decoded.x - point.x) < 0.0001f) &&
            (fabsf(decoded.value - point.value) < 0.0001f) &&
            (fabsf(decoded.z - point.z) < 0.0001f);
        frame[length - 1u] ^= 0x01u;
        isValid = (kymo_decode_data(frame, length, &decoded) == 0) && isValid;
    }
    return isValid;
}

/** @brief Initialize UART, clock and the local codec self-check. */
/*******************************************************************************
 * demoFeaturesInit
 ******************************************************************************/
void demoFeaturesInit(void)
{
    Serial.begin(115200u);
    featureSender.begin(featureStream, true);
    featureSender.setMillisecondCallback(featureClockMs);
    isDecoderValid = verifyFeatureDecoder();
    lastSampleMs = millis();
    nextChannel = 0u;
}

/** @brief Send four channels at 50 Hz without blocking on a pending frame. */
/*******************************************************************************
 * demoFeaturesMain
 ******************************************************************************/
void demoFeaturesMain(void)
{
    const uint32_t nowMs = millis();
    const float phase = static_cast<float>(nowMs % 2000u) / 2000.0f;
    const float sine = sinf(phase * 6.283185307f);
    bool isAccepted = false;
    const bool canSend = featureSender.flush();
    if (canSend && (nowMs - lastSampleMs >= 20u)) {
        switch (nextChannel) {
        case 0u:
            isAccepted = featureSender.send(0u, sine, true);
            break;
        case 1u:
            isAccepted = featureSender.send2D(1u, phase, sine, true);
            break;
        case 2u:
            isAccepted = featureSender.send3D(2u, phase, sine, 1.0f - phase, true);
            break;
        default:
            isAccepted = featureSender.send(3u, isDecoderValid ? 1.0f : 0.0f, true);
            break;
        }
        if (isAccepted) {
            ++nextChannel;
            if (nextChannel >= 4u) {
                nextChannel = 0u;
                lastSampleMs = nowMs;
            }
        }
    }
}
#endif
