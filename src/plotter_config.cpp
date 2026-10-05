#include <Arduino.h>
#include <math.h>
#include "plotter_config.h"

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *) { return millis(); }

/*******************************************************************************
 * serial_write
 ******************************************************************************/
static uint8_t serial_write(void *, const uint8_t *bytes, uint8_t length)
{
    uint8_t written = 0;
    int available = Serial.availableForWrite();
    if (available > 0) {
        if (available < length) {
            length = static_cast<uint8_t>(available);
        }
        written = static_cast<uint8_t>(Serial.write(bytes, length));
    }
    return written;
}

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *, uint8_t id, PlotterSample *out)
{
    const float phase = (millis() % 2000u) / 2000.0f;
    out->value = id == 0 ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
    return 1;
}
static const PlotterChannel channels[] = {
    {20, 0, 0},
    {20, 1, 0}
};
static uint32_t last_sample_ms[2];

const PlotterConfig plotter_config = {
    channels, last_sample_ms, clock_ms, nullptr, sample, nullptr,
    serial_write, nullptr, nullptr, nullptr, 2
};
