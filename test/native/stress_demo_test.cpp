/** @file stress_demo_test.cpp
 * @brief Exercise actual ESP32 USB source with UART stalls and timer rollover.
 */
#include <assert.h>
#include <math.h>
#include <Arduino.h>

uint32_t testClockMs = UINT32_MAX - 5u;
bool testIsBlocked = true;
std::vector<uint8_t> testWire;
TestSerial Serial;

#include "../../src/stress_demo.cpp"

/** @brief Decode all captured frames and assert round-robin delivery.
 * @return Verified complete frames. */
/*******************************************************************************
 * verifyCapturedFrames
 ******************************************************************************/
static uint32_t verifyCapturedFrames()
{
    size_t position = 0u;
    uint32_t frames = 0u;
    uint32_t previousTimestamp = 0u;
    uint32_t counts[KYMO_STRESS_CHANNEL_COUNT] = {};
    while (position < testWire.size()) {
        const size_t frameLength = kymo_frame_length(testWire[position + 2u]);
        KymoDataPoint point = {};
        uint32_t phase = 0u;
        float expected = 0.0f;
        assert(frameLength == (KYMO_STRESS_XYZ_ENABLE ? 21u : 13u));
        assert(position + frameLength <= testWire.size());
        assert(kymo_decode_data(testWire.data() + position, frameLength, &point) != 0);
        assert(point.id == frames % KYMO_STRESS_CHANNEL_COUNT);
        phase = (counts[point.id] + static_cast<uint32_t>(point.id) * 17u) % 2048u;
        expected = static_cast<float>(phase) / 1023.5f - 1.0f;
        assert(fabsf(point.value - expected) < 0.00001f);
        assert((point.flags & KYMO_FLAG_TIMESTAMP) != 0u);
        assert(point.timestamp_ms >= previousTimestamp);
#if KYMO_STRESS_XYZ_ENABLE
        assert((point.flags & (KYMO_FLAG_X | KYMO_FLAG_Z)) == (KYMO_FLAG_X | KYMO_FLAG_Z));
        assert(fabsf(point.x - (static_cast<float>((phase + 512u) % 2048u) / 1023.5f - 1.0f)) < 0.00001f);
        assert(fabsf(point.z - (static_cast<float>((phase + 1024u) % 2048u) / 1023.5f - 1.0f)) < 0.00001f);
#endif
        ++counts[point.id];
        ++frames;
        previousTimestamp = point.timestamp_ms;
        position += frameLength;
    }
    assert(frames > 256u);
    for (uint16_t channelIndex = 0u; channelIndex < KYMO_STRESS_CHANNEL_COUNT; ++channelIndex) {
        assert(counts[channelIndex] > 0u);
        assert(counts[channelIndex] == s_sampleCounts[channelIndex]);
    }
    return frames;
}

/** @brief Run blocked, partial-write and stream-reset phases. @return Zero. */
/*******************************************************************************
 * main
 ******************************************************************************/
int32_t main()
{
    uint32_t sampledWhileBlocked = 0u;
    demoStressInit();
    assert(s_isReady);
    assert(Serial.m_baud == KYMO_STRESS_BAUD);
    for (uint16_t iteration = 0u; iteration < 1000u; ++iteration) {
        demoStressMain();
        assert(s_outputLength <= OUTPUT_CAPACITY);
    }
    assert(testWire.empty());
    for (uint16_t channelIndex = 0u; channelIndex < KYMO_STRESS_CHANNEL_COUNT; ++channelIndex) {
        sampledWhileBlocked += s_sampleCounts[channelIndex];
    }
    assert(sampledWhileBlocked <= OUTPUT_CAPACITY / 13u);
    for (uint16_t iteration = 0u; iteration < 16000u; ++iteration) {
        testIsBlocked = iteration % 3u == 0u;
        demoStressMain();
    }
    testIsBlocked = false;
    while (s_outputLength > 0u) {
        flushStressOutput();
    }
    (void)verifyCapturedFrames();

    // Start a new session after discarding a partially transmitted batch.
    for (uint16_t iteration = 0u; (iteration < 10u) && (s_outputOffset == 0u); ++iteration) {
        demoStressMain();
    }
    assert(s_outputOffset > 0u);
    resetStressStream();
    testWire.clear();
    for (uint16_t iteration = 0u; iteration < 2000u; ++iteration) {
        demoStressMain();
    }
    while (s_outputLength > 0u) {
        flushStressOutput();
    }
    (void)verifyCapturedFrames();
    return 0;
}
