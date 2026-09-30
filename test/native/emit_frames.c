/* Real cyclic C sender, all 256 IDs and all eight optional-field layouts. */
#include <assert.h>
#include <stdio.h>
#include "plotter_runtime.h"
static uint32_t now;

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *user) { (void)user; return now; }

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, PlotterSample *out) {
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
    PlotterChannel channels[256];
    uint32_t last[256];
    PlotterContext ctx;
    PlotterConfig config = {channels, last, clock_ms, NULL, sample, NULL,
                            emit, NULL, NULL, NULL, 256};
    unsigned i;
    for (i = 0; i < 256; ++i) {
        channels[i].id = (uint8_t)i;
        channels[i].flags = (uint8_t)((i & 7u) << 1);
        channels[i].period_ms = 100;
    }
    assert(Plotter_Init(&ctx, &config) == PLOTTER_OK);
    now = 1234;
    for (i = 0; i < 256; ++i) assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_IDLE);
    return 0;
}
