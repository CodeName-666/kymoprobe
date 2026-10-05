#include <assert.h>
#include <math.h>
#include <string.h>
#include "kymo_protocol.h"

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void) {
    static const uint8_t sizes[] = {9,13,13,17,13,17,17,21};
    unsigned descriptor, flags;
    KymoDataPoint point = {0}, decoded;
    uint8_t frame[23];
    assert(kymo_crc8((const uint8_t *)"123456789", 9) == 0xf4);
    for (descriptor = 0; descriptor < 256; ++descriptor) {
        size_t expected = 0;
        for (flags = 0; flags < 8; ++flags)
            if ((descriptor & 0xfeu) == 0x40u + 2u * flags) expected = sizes[flags];
        assert(kymo_frame_length((uint8_t)descriptor) == expected);
    }
    for (flags = 0; flags < 8; ++flags) {
        size_t length;
        point.id = 255;
        point.flags = (uint8_t)(flags << 1);
        point.value = -2.5f; point.x = 1.25f; point.z = 9.0f;
        point.timestamp_ms = UINT32_MAX;
        memset(frame, 0xcc, sizeof(frame));
        assert(kymo_encode_data(&point, frame + 1, sizes[flags] - 1) == 0);
        for (descriptor = 0; descriptor < sizeof(frame); ++descriptor)
            assert(frame[descriptor] == 0xcc);
        length = kymo_encode_data(&point, frame + 1, sizes[flags]);
        assert(length == sizes[flags]);
        assert(frame[0] == 0xcc && frame[length + 1] == 0xcc);
        assert(kymo_decode_data(frame + 1, length, &decoded));
        assert(decoded.id == 255 && decoded.value == -2.5f);
        assert(decoded.x == ((flags & 4) ? 1.25f : 0.0f));
        assert(decoded.z == ((flags & 2) ? 9.0f : 0.0f));
        assert(decoded.timestamp_ms == ((flags & 1) ? UINT32_MAX : 0));
        assert(!kymo_decode_data(frame + 1, length - 1, &decoded));
        frame[length] ^= 1;
#if KYMO_ENABLE_CRC
        assert(!kymo_decode_data(frame + 1, length, &decoded));
#else
        assert(frame[3] == (uint8_t)(0x41u | point.flags));
        assert(frame[length] == 1u);
        assert(kymo_decode_data(frame + 1, length, &decoded));
#endif
        /* A decoder must validate legacy CRC frames in either build mode. */
        frame[3] &= 0xfeu;
        frame[length] = kymo_crc8(frame + 3, length - 3u);
        assert(kymo_decode_data(frame + 1, length, &decoded));
        frame[length] ^= 1u;
        assert(!kymo_decode_data(frame + 1, length, &decoded));
        /* NO_CRC ignores the trailer in either build mode. */
        frame[3] |= 1u;
        assert(kymo_decode_data(frame + 1, length, &decoded));
    }
    point.flags = 1;
    assert(!kymo_encode_data(&point, frame, sizeof(frame)));
    point.flags = 0;
    point.value = NAN;
    assert(!kymo_encode_data(&point, frame, sizeof(frame)));
    point.value = 1.0f; point.x = INFINITY;
    assert(kymo_encode_data(&point, frame, sizeof(frame)) == 9);
    point.flags = KYMO_FLAG_X;
    assert(!kymo_encode_data(&point, frame, sizeof(frame)));
    assert(!kymo_decode_data(NULL, 0, &decoded));
    assert(!kymo_encode_data(NULL, frame, sizeof(frame)));
    assert(!kymo_encode_data(&point, NULL, sizeof(frame)));
    point.flags = 0;
    assert(kymo_encode_data(&point, frame, sizeof(frame)) == 9);
    assert(!kymo_decode_data(frame, 9, NULL));
    frame[0] = 0;
    assert(!kymo_decode_data(frame, 9, &decoded));
    {
        /* A CRC-valid but non-finite wire value must not corrupt output. */
        uint8_t invalid[] = {0xa5,0x5a,0x40,7,0,0,0x80,0x7f,0};
        KymoDataPoint before;
        memset(&before, 0x5a, sizeof(before));
        memcpy(&decoded, &before, sizeof(decoded));
        invalid[8] = kymo_crc8(invalid + 2, 6);
        assert(!kymo_decode_data(invalid, sizeof(invalid), &decoded));
        assert(memcmp(&decoded, &before, sizeof(decoded)) == 0);
    }
    return 0;
}
