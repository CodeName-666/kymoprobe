/**
 * @brief Application-side Serial configuration shared by Arduino examples.
 *
 * @details Uses static linkage: include from exactly one application translation unit per device.
 * Define PLOTTER_EXAMPLE_MULTIDIMENSIONAL before inclusion to select Y/XY/XYZ channels;
 * otherwise the table contains two timestamped scalar waveforms. This header is not
 * part of PlotterLib and deliberately depends on the Arduino framework.
 * @file arduino_serial_config.h
 * @defgroup example_arduino_config Arduino example configuration
 * @{
 * @par Usage
 * Initialize Serial at 115200 baud; pass example_config to Plotter_Init and run Plotter_Main.
 */
#ifndef EXAMPLE_ARDUINO_SERIAL_CONFIG_H
/**
 * @brief Include guard for arduino_serial_config.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EXAMPLE_ARDUINO_SERIAL_CONFIG_H

#include <Arduino.h>
#include <math.h>
#include <plotter_runtime.h>

/**
 * @brief Read the Arduino millisecond counter as a portable uint32 value.
 *
 * @details Does not modify application context; Arduino rollover is retained.
 * @param[in] user Unused opaque callback context; pass nullptr.
 * @return Current millisecond tick.
 * @par Usage
 * Bound to example_config.clock_ms.
 */
static uint32_t example_clock(void *user) { (void)user; return millis(); }

/**
 * @brief Copy as many bytes as fit into the Serial transmit capacity.
 *
 * @details Checks availableForWrite before writing and reports only accepted bytes. The runtime
 * retains any unsent suffix. Execution time still depends on the concrete Serial driver.
 * @param[in] user Unused callback context; pass nullptr.
 * @param[in] bytes Non-null readable frame suffix; not retained after the call.
 * @param[in] length Requested byte count, 1..21.
 * @return Count copied into Serial, or zero when capacity is unavailable.
 * @par Usage
 * Bound to example_config.write; the library remains independent of Serial.
 */
static uint8_t example_write(void *user, const uint8_t *bytes, uint8_t length) {
    (void)user;
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
/**
 * @brief Generate deterministic two-second waveform measurements.
 *
 * @details X is normalized phase, Y is sine for ID zero and sawtooth for other IDs, and Z
 * is cosine. Channel flags choose which coordinates reach the wire. Replace this
 * callback to read real sensors or application parameters.
 * @param[in] user Unused callback context; pass nullptr.
 * @param[in] id Configured channel ID selecting the Y waveform.
 * @param[out] out Non-null sample destination initialized by the runtime.
 * @return One; this synthetic data source always has a valid sample.
 * @par Usage
 * Use the waveform source to validate app framing before integrating sensors.
 */
static uint8_t example_sample(void *user, uint8_t id, PlotterSample *out) {
    (void)user;
    const float phase = (millis() % 2000u) / 2000.0f;
    out->x = phase;
    out->value = id == 0 ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
    out->z = cosf(phase * 6.283185307f);
    return 1;
}
/**
 * @brief Immutable demo channel table.
 *
 * @details Default: IDs 0/1 at 20 ms, Y plus timestamp. With PLOTTER_EXAMPLE_MULTIDIMENSIONAL:
 * ID 0 is Y/time at 20 ms, ID 1 XY/time at 50 ms, ID 2 XYZ/time at 100 ms.
 * @par Usage
 * Adjust table periods/flags before compiling; do not change it during runtime.
 */
static const PlotterChannel example_channels[] = {
    {20, 0, PLOTTER_FLAG_TIMESTAMP},
#ifdef PLOTTER_EXAMPLE_MULTIDIMENSIONAL
    {50, 1, EMB_U8_OR(PLOTTER_FLAG_X, PLOTTER_FLAG_TIMESTAMP)},
    {100, 2, PLOTTER_ALLOWED_FLAGS},
#else
    {20, 1, PLOTTER_FLAG_TIMESTAMP},
#endif
};
/**
 * @brief Per-channel scheduling timestamps owned by this example.
 *
 * @details Output of Init and input/output of Main. Array length follows example_channels.
 * Static storage ensures it outlives the runtime; applications must not edit it while active.
 * @par Usage
 * Referenced by example_config.last_sample_ms.
 */
static uint32_t example_last[sizeof(example_channels) / sizeof(example_channels[0])];
/**
 * @brief Complete persistent Serial/clock/sample wiring for the example.
 *
 * @details Input to Init; hardware is initialized separately. No busy callback is needed because
 * Serial writes copy data. No service callback or opaque context is used.
 * @par Usage
 * Plotter_Init(&plotter, &example_config);
 */
static const PlotterConfig example_config = {
    example_channels, example_last, example_clock, nullptr, example_sample,
    nullptr, example_write, nullptr, nullptr, nullptr,
    sizeof(example_channels) / sizeof(example_channels[0])
};

/** @} */
#endif /* EXAMPLE_ARDUINO_SERIAL_CONFIG_H */
