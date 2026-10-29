/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Christof Seidel */
/* Real cyclic C sender, all 256 IDs and all eight optional-field layouts. */
#include <assert.h>
#include <stdio.h>
#include <KymoCore.h>
static uint32_t now;

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *user) { (void)user; return now; }

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, KymoSample *out) {
    (void)user; (void)id;
    out->x = 1.25f; out->value = -2.5f; out->z = 9.0f;
    return 1;
}

/*******************************************************************************
 * emit
 ******************************************************************************/
static uint8_t emit(void *user, const uint8_t *bytes, uint8_t length) {
    uint8_t i;
    (void)user;
    for (i = 0; i < length; ++i) printf("%02x", bytes[i]);
    putchar('\n');
    return length;
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void) {
    KymoChannel channels[256];
    KymoContext ctx;
    KymoConfig config = {
        .channels = channels,
        .channel_count = 256,
        .clock_ms = clock_ms,
        .sample = sample,
        .write = emit
    };
    unsigned i;
    for (i = 0; i < 256; ++i) {
        channels[i].id = (uint8_t)i;
        channels[i].flags = (uint8_t)((i & 7u) << 1);
        channels[i].period_ms = 100;
    }
    assert(Kymo_Init(&ctx, &config) == KYMO_OK);
    now = 1234;
    for (i = 0; i < 256; ++i) assert(Kymo_Main(&ctx) == KYMO_OK);
    assert(Kymo_Main(&ctx) == KYMO_IDLE);
    return 0;
}
