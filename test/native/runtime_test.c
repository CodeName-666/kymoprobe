#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "plotter_runtime.h"

typedef struct {
    uint32_t now;
    uint8_t bytes[512];
    size_t length;
    uint8_t limit, busy, asynchronous, skip, invalid, bad_write;
    unsigned samples, writes, services;
    const uint8_t *borrowed;
    uint8_t borrowed_copy[21];
    uint8_t borrowed_length;
} Fixture;

/*******************************************************************************
 * tick
 ******************************************************************************/
static uint32_t tick(void *user) { return ((Fixture *)user)->now; }

/*******************************************************************************
 * service
 ******************************************************************************/
static void service(void *user) { ++((Fixture *)user)->services; }

/*******************************************************************************
 * busy
 ******************************************************************************/
static uint8_t busy(void *user) { return ((Fixture *)user)->busy; }

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, PlotterSample *out) {
    Fixture *f = (Fixture *)user;
    (void)id;
    ++f->samples;
    out->x = 1.25f;
    out->value = f->invalid ? INFINITY : -2.5f;
    out->z = 9.0f;
    return (uint8_t)!f->skip;
}

/*******************************************************************************
 * write_bytes
 ******************************************************************************/
static uint8_t write_bytes(void *user, const uint8_t *bytes, uint8_t length) {
    Fixture *f = (Fixture *)user;
    uint8_t count = length < f->limit ? length : f->limit;
    ++f->writes;
    if (f->bad_write) {
        count = (uint8_t)(length + 1u);
    } else {
        assert(f->length + count <= sizeof(f->bytes));
        memcpy(f->bytes + f->length, bytes, count);
        f->length += count;
        if (f->asynchronous && count) {
            f->borrowed = bytes;
            f->borrowed_length = count;
            memcpy(f->borrowed_copy, bytes, count);
            f->busy = 1;
        }
    }
    return count;
}

/*******************************************************************************
 * config
 ******************************************************************************/
static PlotterConfig config(Fixture *f, const PlotterChannel *channels,
                            uint32_t *last, uint16_t count) {
    PlotterConfig c = {0};
    c.channels = channels;
    c.last_sample_ms = last;
    c.channel_count = count;
    c.clock_ms = tick;
    c.clock_user = f;
    c.sample = sample;
    c.sample_user = f;
    c.write = write_bytes;
    c.busy = busy;
    c.service = service;
    c.transport_user = f;
    return c;
}

/*******************************************************************************
 * golden_and_period
 ******************************************************************************/
static void golden_and_period(void) {
    static const uint8_t golden[] = {
        0xa5,0x5a,PLOTTER_ENABLE_CRC ? 0x4e : 0x4f,3,0,0,0xa0,0x3f,0,0,0x20,0xc0,
        0,0,0x10,0x41,0xd2,4,0,0,PLOTTER_ENABLE_CRC ? 0x89 : 0};
    Fixture f = {0};
    PlotterChannel channel = {50, 3, PLOTTER_ALLOWED_FLAGS};
    uint32_t last[1];
    PlotterConfig c = config(&f, &channel, last, 1);
    PlotterContext ctx;
    f.limit = 21;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_OK);
    f.now = 1234;
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(f.length == 21 && memcmp(f.bytes, golden, 21) == 0);
    assert(Plotter_Main(&ctx) == PLOTTER_IDLE);
    f.now += 49;
    assert(Plotter_Main(&ctx) == PLOTTER_IDLE);
    ++f.now;
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(f.samples == 2 && f.services == 4);
}

/*******************************************************************************
 * partial_and_busy
 ******************************************************************************/
static void partial_and_busy(void) {
    Fixture f = {0};
    PlotterChannel channels[] = {{0, 3, PLOTTER_ALLOWED_FLAGS}, {0, 7, 0}};
    uint32_t last[2];
    PlotterConfig c = config(&f, channels, last, 2);
    PlotterContext ctx;
    PlotterDataPoint decoded;
    uint8_t original[21];
    assert(Plotter_Init(&ctx, &c) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_BUSY); /* zero write retains sample */
    assert(f.samples == 1 && f.length == 0);
    f.limit = 3;
    f.now = 25;
    assert(Plotter_Main(&ctx) == PLOTTER_BUSY);
    f.busy = 1;
    assert(Plotter_Main(&ctx) == PLOTTER_BUSY);
    assert(f.writes == 2 && f.samples == 1);
    f.busy = 0;
    while (f.length < 21) (void)Plotter_Main(&ctx);
    assert(plotter_decode_data(f.bytes, 21, &decoded));
    assert(decoded.id == 3 && decoded.timestamp_ms == 0);
    memcpy(original, f.bytes, 21);
    f.limit = 21;
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(f.length == 30 && f.samples == 2);
    assert(memcmp(original, f.bytes, 21) == 0);
    assert(plotter_decode_data(f.bytes + 21, 9, &decoded) && decoded.id == 7);
}

/*******************************************************************************
 * asynchronous_buffer_lifetime
 ******************************************************************************/
static void asynchronous_buffer_lifetime(void) {
    Fixture f = {0};
    PlotterChannel channel = {0, 3, PLOTTER_ALLOWED_FLAGS};
    uint32_t last[1];
    PlotterConfig c = config(&f, &channel, last, 1);
    PlotterContext ctx;
    f.limit = 21;
    f.asynchronous = 1;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    f.now = 99;
    assert(Plotter_Main(&ctx) == PLOTTER_BUSY);
    assert(f.samples == 1 && f.writes == 1 && f.services == 2);
    assert(memcmp(f.borrowed, f.borrowed_copy, f.borrowed_length) == 0);
    f.busy = 0;
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(f.samples == 2);
}

/*******************************************************************************
 * validation_and_failures
 ******************************************************************************/
static void validation_and_failures(void) {
    Fixture f = {0};
    PlotterChannel channels[] = {{10, 3, 0}, {20, 3, 0}};
    uint32_t last[2];
    PlotterConfig c = config(&f, channels, last, 2);
    PlotterContext ctx = {0};
    PlotterConfig invalid;
    assert(Plotter_Main(&ctx) == PLOTTER_BAD_CONFIG);
    assert(Plotter_Main(NULL) == PLOTTER_BAD_CONFIG);
    assert(Plotter_Init(NULL, &c) == PLOTTER_BAD_CONFIG);
    assert(Plotter_Init(&ctx, &c) == PLOTTER_BAD_CONFIG); /* duplicate IDs */
    c.channel_count = 1;
    invalid = c;
    invalid.channel_count = 0;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    invalid.channel_count = 257;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    invalid = c;
    invalid.channels = NULL;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    invalid = c;
    invalid.last_sample_ms = NULL;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    invalid = c;
    invalid.clock_ms = NULL;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    invalid = c;
    invalid.sample = NULL;
    assert(Plotter_Init(&ctx, &invalid) == PLOTTER_BAD_CONFIG);
    channels[0].flags = 0x80;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_BAD_CONFIG);
    channels[0].flags = 0;
    channels[0].period_ms = UINT32_MAX;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_BAD_CONFIG);
    channels[0].period_ms = 0;
    c.write = NULL;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_BAD_CONFIG);
    c.write = write_bytes;
    f.limit = 21;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_OK);
    f.skip = 1;
    assert(Plotter_Main(&ctx) == PLOTTER_SKIPPED);
    f.skip = 0;
    f.invalid = 1;
    assert(Plotter_Main(&ctx) == PLOTTER_BAD_SAMPLE);
    assert(f.writes == 0);
    f.invalid = 0;
    f.bad_write = 1;
    assert(Plotter_Main(&ctx) == PLOTTER_IO_ERROR);
    assert(Plotter_Main(&ctx) == PLOTTER_IO_ERROR); /* latched: cannot guess bytes sent */
    assert(f.writes == 1);
    assert(Plotter_Init(&ctx, NULL) == PLOTTER_BAD_CONFIG);
    assert(Plotter_Main(&ctx) == PLOTTER_BAD_CONFIG);
}

/*******************************************************************************
 * rollover_and_fairness
 ******************************************************************************/
static void rollover_and_fairness(void) {
    Fixture f = {0};
    PlotterChannel channels[] = {{0, 0, PLOTTER_FLAG_TIMESTAMP}, {20, 255, PLOTTER_FLAG_TIMESTAMP}};
    uint32_t last[2];
    PlotterConfig c = config(&f, channels, last, 2);
    PlotterContext ctx;
    PlotterDataPoint decoded;
    f.now = UINT32_MAX - 9;
    f.limit = 21;
    assert(Plotter_Init(&ctx, &c) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(f.bytes[3] == 0 && f.bytes[16] == 255);
    f.now = 10; /* +20 ms */
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(Plotter_Main(&ctx) == PLOTTER_OK);
    assert(plotter_decode_data(f.bytes + 39, 13, &decoded));
    assert(decoded.id == 255 && decoded.timestamp_ms == 20);
}

/*******************************************************************************
 * independent_instances
 ******************************************************************************/
static void independent_instances(void) {
    Fixture a = {0}, b = {0};
    PlotterChannel channel = {100, 3, 0};
    uint32_t last_a[1], last_b[1];
    PlotterConfig ca = config(&a, &channel, last_a, 1);
    PlotterConfig cb = config(&b, &channel, last_b, 1);
    PlotterContext first, second;
    b.limit = 21;
    assert(Plotter_Init(&first, &ca) == PLOTTER_OK);
    assert(Plotter_Init(&second, &cb) == PLOTTER_OK);
    assert(Plotter_Main(&first) == PLOTTER_BUSY);
    assert(Plotter_Main(&second) == PLOTTER_OK);
    assert(a.length == 0 && b.length == 9);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void) {
    golden_and_period();
    partial_and_busy();
    asynchronous_buffer_lifetime();
    validation_and_failures();
    rollover_and_fairness();
    independent_instances();
    printf("Runtime tests passed; context=%u bytes, channel=%u bytes\n",
           (unsigned)sizeof(PlotterContext), (unsigned)sizeof(PlotterChannel));
    return 0;
}
