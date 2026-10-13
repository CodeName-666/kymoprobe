/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
/** @file features_demo_test.cpp @brief Verify actual feature firmware and dispatch.
 * @details Host-only capture uses heap; application/Core retain fixed storage.
 */
#include <assert.h>
#include <Arduino.h>
#include <kymo_protocol.h>

uint32_t testClockMs = UINT32_MAX - 5u;
bool testIsBlocked = true;
std::vector<uint8_t> testWire;
TestSerial Serial;

#include "../../src/features_demo.cpp"
#include "../../src/main.cpp"

/** @brief Exercise partial writes, clock wrap and every feature channel.
 * @return Zero when all assertions pass. */
/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    size_t position = 0u;
    uint32_t frameCount = 0u;
    uint32_t counts[4] = {};
    KymoDataPoint point = {};
    size_t length = 0u;
    setup();
    assert(Serial.m_baud == 115200u);
    assert(isDecoderValid);
    for (uint16_t step = 0u; step < 100u; ++step) {
        ++testClockMs;
        loop();
    }
    assert(testWire.empty());
    testIsBlocked = false;
    for (uint16_t step = 0u; step < 500u; ++step) {
        ++testClockMs;
        loop();
    }
    while (!featureSender.flush()) {
    }
    while (position < testWire.size()) {
        assert(testWire.size() - position >= 3u);
        length = kymo_frame_length(testWire[position + 2u]);
        assert(length > 0u && position + length <= testWire.size());
        assert(kymo_decode_data(&testWire[position], length, &point) != 0);
        assert(point.id == frameCount % 4u);
        assert((point.flags & KYMO_FLAG_TIMESTAMP) != 0u);
        assert((testWire[position + 2u] & KYMO_DESCRIPTOR_NO_CRC) == 0u);
        assert(((point.flags & KYMO_FLAG_X) != 0u) ==
               ((point.id == 1u) || (point.id == 2u)));
        assert(((point.flags & KYMO_FLAG_Z) != 0u) == (point.id == 2u));
        if (point.id == 3u) {
            assert(point.value > 0.99f && point.value < 1.01f);
        }
        ++counts[point.id];
        ++frameCount;
        position += length;
    }
    assert(counts[0] >= 10u && counts[1] >= 10u &&
           counts[2] >= 10u && counts[3] >= 10u);
    return 0;
}
