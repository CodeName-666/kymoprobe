#include <assert.h>
#include <math.h>
#include <string.h>
#include "plotter_runtime.h"

#if PLOTTER_ENABLE_RUNTIME
static uint8_t captured[21];
static uint8_t captured_length;

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *user) { (void)user; return 1234u; }

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, PlotterSample *out)
{
    (void)user;
    (void)id;
    out->value = 1.0f;
    out->x = 2.0f;
    out->z = 3.0f;
    return 1u;
}

/*******************************************************************************
 * write_bytes
 ******************************************************************************/
static uint8_t write_bytes(void *user, const uint8_t *bytes, uint8_t length)
{
    (void)user;
    memcpy(captured, bytes, length);
    captured_length = length;
    return length;
}
#endif

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    uint8_t enabled = 0u;
    uint8_t flags;
    uint8_t frame[21];
    PlotterDataPoint point = {0};
#if PLOTTER_ENABLE_X
    enabled |= PLOTTER_FLAG_X;
#endif
#if PLOTTER_ENABLE_Z
    enabled |= PLOTTER_FLAG_Z;
#endif
#if PLOTTER_ENABLE_TIMESTAMP
    enabled |= PLOTTER_FLAG_TIMESTAMP;
#endif
    point.id = 7u;
    point.value = 1.0f;
    point.x = 2.0f;
    point.z = 3.0f;
    point.timestamp_ms = 1234u;
    for (flags = 0u; flags <= 14u; flags = (uint8_t)(flags + 2u)) {
        uint8_t supported = (uint8_t)((flags & enabled) == flags);
        size_t expected = 9u;
        size_t length;
        uint8_t before[21];
        point.flags = flags;
        if ((flags & PLOTTER_FLAG_X) != 0u) expected += 4u;
        if ((flags & PLOTTER_FLAG_Z) != 0u) expected += 4u;
        if ((flags & PLOTTER_FLAG_TIMESTAMP) != 0u) expected += 4u;
        memset(frame, 0xcc, sizeof(frame));
        memcpy(before, frame, sizeof(frame));
        length = plotter_encode_data(&point, frame, sizeof(frame));
        if (supported != 0u) {
            assert(length == expected);
            assert(frame[3] == 7u);
#if PLOTTER_ENABLE_CRC
            assert(frame[2] == (uint8_t)(0x40u | flags));
            assert(frame[length - 1u] == plotter_crc8(frame + 2, length - 3u));
#else
            assert(frame[2] == (uint8_t)(0x41u | flags));
            assert(frame[length - 1u] == 0u);
#endif
#if PLOTTER_ENABLE_DECODER
            {
                PlotterDataPoint decoded = {0};
                assert(plotter_decode_data(frame, length, &decoded));
                assert(decoded.flags == flags && decoded.value == 1.0f);
                assert(decoded.x == ((flags & PLOTTER_FLAG_X) ? 2.0f : 0.0f));
                assert(decoded.z == ((flags & PLOTTER_FLAG_Z) ? 3.0f : 0.0f));
                assert(decoded.timestamp_ms == ((flags & PLOTTER_FLAG_TIMESTAMP) ? 1234u : 0u));
            }
#endif
        } else {
            assert(length == 0u);
            assert(memcmp(frame, before, sizeof(frame)) == 0);
            assert(plotter_frame_length((uint8_t)(0x41u | flags)) == 0u);
        }
#if PLOTTER_ENABLE_RUNTIME
        {
            PlotterChannel channel = {20u, 7u, flags};
            uint32_t last[1];
            PlotterConfig config = {&channel, last, clock_ms, NULL, sample, NULL,
                                    write_bytes, NULL, NULL, NULL, 1u};
            PlotterContext context;
            PlotterStatus status = Plotter_Init(&context, &config);
            assert(status == (supported ? PLOTTER_OK : PLOTTER_BAD_CONFIG));
            if (supported != 0u) {
                assert(Plotter_Main(&context) == PLOTTER_OK);
                assert(captured_length == expected && captured[3] == 7u);
                assert(Plotter_Main(&context) == PLOTTER_IDLE);
            }
        }
#endif
    }
    point.flags = 0u;
    point.value = NAN;
    assert(plotter_encode_data(&point, frame, sizeof(frame)) == 0u);
#if PLOTTER_ENABLE_DECODER
    {
        uint8_t legacy[] = {0xa5,0x5a,0x40,7,0,0,0x80,0x3f,0x54};
        PlotterDataPoint decoded = {0};
        assert(plotter_decode_data(legacy, sizeof(legacy), &decoded));
        legacy[8] ^= 1u;
        assert(!plotter_decode_data(legacy, sizeof(legacy), &decoded));
    }
#endif
    return 0;
}
