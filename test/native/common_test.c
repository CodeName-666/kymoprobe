/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include <assert.h>
#include <string.h>
#include "common/embedded_bits.h"
#include "common/embedded_bytes.h"
#include "common/embedded_crc.h"

/* Constant-expression macros must work in static initializers. */
static const uint8_t flags = EMB_U8_OR(0x08u, EMB_U8_OR(0x04u, 0x02u));

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    static const uint8_t masks[] = {1, 2, 4, 8, 16, 32, 64, 128};
    uint8_t bytes[6] = {0xcc, 0, 0, 0, 0, 0xdd};
    uint8_t bitset[32] = {0};
    uint8_t index;
    unsigned once = 1;
    assert(flags == 14);
    assert(EMB_U8_OR(once++, 0x80u) == 0x81u && once == 2);
    for (index = 0; index < 8; ++index) {
        assert(emb_u8_bit(index) == masks[index]);
    }
    assert(emb_u8_bit(8) == 0 && emb_u8_bit(255) == 0);
    assert(emb_u8_mask(0xff, 0x0e) == 0x0e);
    assert(emb_u8_has_any(0x08, 0x0e));
    assert(!emb_u8_has_any(0xf0, 0x0e));
    assert(emb_u8_only_bits(0, 0x0e));
    assert(emb_u8_only_bits(0x0e, 0x0e));
    assert(!emb_u8_only_bits(0x81, 0x0e));
    assert(emb_bitset_set(bitset, sizeof(bitset), 0));
    assert(emb_bitset_set(bitset, sizeof(bitset), 7));
    assert(emb_bitset_set(bitset, sizeof(bitset), 8));
    assert(emb_bitset_set(bitset, sizeof(bitset), 255));
    assert(bitset[0] == 0x81 && bitset[1] == 1 && bitset[31] == 0x80);
    assert(emb_bitset_test(bitset, sizeof(bitset), 255));
    assert(!emb_bitset_test(bitset, sizeof(bitset), 254));
    assert(!emb_bitset_set(bitset, sizeof(bitset), 256));
    assert(!emb_bitset_test(bitset, sizeof(bitset), (size_t)-1));
    assert(!emb_bitset_set(NULL, 32, 0));
    assert(!emb_bitset_test(NULL, 32, 0));
    assert(!emb_bitset_set(bitset, 0, 0));

    emb_write_u32_le(bytes + 1, UINT32_C(0x89abcdef));
    assert(memcmp(bytes, "\xcc\xef\xcd\xab\x89\xdd", 6) == 0);
    assert(emb_read_u32_le(bytes + 1) == UINT32_C(0x89abcdef));
    emb_write_u32_le(bytes + 1, UINT32_MAX);
    assert(memcmp(bytes + 1, "\xff\xff\xff\xff", 4) == 0);
    emb_write_f32_le(bytes + 1, -2.5f);
    assert(memcmp(bytes + 1, "\x00\x00\x20\xc0", 4) == 0);
    assert(emb_read_f32_le(bytes + 1) == -2.5f);
    assert(bytes[0] == 0xcc && bytes[5] == 0xdd);

    assert(emb_crc8_msb((const uint8_t *)"123456789", 9, 0x07, 0) == 0xf4);
    /* CRC-8/SAE-J1850, before its final XOR of 0xff: 0x4b ^ 0xff. */
    assert(emb_crc8_msb((const uint8_t *)"123456789", 9, 0x1d, 0xff) == 0xb4);
    assert(emb_crc8_msb(NULL, 0, 0x07, 0) == 0);
    assert(emb_crc8_msb(bytes, 0, 0x07, 0x42) == 0x42);
    return 0;
}
