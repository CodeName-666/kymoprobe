# PlotterLib 6.1 — C99 Init/Main, wire protocol v6 unchanged

The recommended API is `plotter_runtime.h`. `plotter_protocol.h` remains a
standalone C codec; `plotter.h` offers an optional C++11 push sender. The C
runtime needs no Arduino, C++ runtime, heap, virtual dispatch or global state.
Copy the C headers and `plotter_runtime.c` / `plotter_protocol.c` into any C99
project, or install the complete directory as a PlatformIO/Arduino library.
PlatformIO's framework/platform compatibility is unrestricted.

## Static configuration

Define this in your application's `plotter_config.c` (or `.cpp`):

```c
#include <plotter_runtime.h>

static uint32_t clock_ms(void *user); /* your monotonic millisecond clock */
static uint8_t read_sample(void *user, uint8_t id, PlotterSample *out);
static uint8_t write_bytes(void *user, const uint8_t *bytes, uint8_t length);

static const PlotterChannel channels[] = {
    {20, 0, PLOTTER_FLAG_TIMESTAMP},
    {100, 1, PLOTTER_FLAG_X | PLOTTER_FLAG_Z | PLOTTER_FLAG_TIMESTAMP}
};
static uint32_t last_sample_ms[2];
const PlotterConfig plotter_config = {
    channels, last_sample_ms, clock_ms, 0, read_sample, 0,
    write_bytes, 0, 0, 0, 2
};
```

Implement the three callbacks in that file. `read_sample` assigns `out->value`
and optional `out->x` / `out->z`; returning zero skips a measurement until its
next scheduled interval. It can read shared application parameters through
`sample_user`. The runtime zero-initializes all sample fields before calling it.
Hardware handles belong in `transport_user`; clocks can use `clock_user`.

Initialize hardware first, then call:

```c
static PlotterContext plotter;
if (Plotter_Init(&plotter, &plotter_config) != PLOTTER_OK) {
    /* handle invalid configuration */
}
/* From your existing cyclic task: */
PlotterStatus status = Plotter_Main(&plotter);
```

Config, channels and last_sample_ms must remain alive. Do not mutate config or
channels after Init. Every instance needs its own context and last_sample_ms
array. Hardware setup remains the application's responsibility. Configuration
is compiled data, not a runtime file requiring a filesystem/parser.

## Scheduling and memory

- 1–256 unique channel IDs; IDs, flags, status, TX lengths and offsets are bytes.
- Periods are uint32 milliseconds, limited to INT32_MAX; zero means every
  eligible Main invocation. First samples are due immediately.
- One fixed **21-byte TX buffer** per context and **4 bytes runtime state per
  channel**. Config is separate and immutable. No measurement queue or heap.
- At most one sample callback and one transport write per Main; at most N
  channel checks. Round-robin selection avoids starvation. Call Main faster
  than the sum of configured sample rates; short writes need additional calls.
- Slow transports apply backpressure. A pending frame retains its original
  value/timestamp; after it finishes, overdue channels sample current values.
  There is no attempt to replay missed samples or an unbounded catch-up burst.
- uint32 subtraction handles clock rollover. Invoke Main at least once within
  INT32_MAX ms. Wire timestamps wrap after approximately 49.7 days, per v6.
- All entrypoints are single-task/non-reentrant. Synchronize application values
  shared with interrupts. Do not reinitialize while DMA/USB borrows a buffer.
- Native pointer/numeric structs remain naturally aligned. Explicit byte
  encoding avoids packed structs, enum-width and endian dependencies. IEEE-754
  binary32 floats are required; do not enable finite-math-only/fast-math modes.

## Transport callbacks

`write(user, bytes, length)` returns **accepted bytes**, from zero to length.
Zero means retry later. Short writes resume at their remaining offset. A driver
must never consume bytes it reports as unaccepted. More than length is a
contract error: Main latches IO_ERROR until reinitialization after driver reset.

Synchronous drivers copy/consume bytes before returning. Asynchronous UART/DMA
or CubeMX CDC may borrow the pointer only with a `busy(user)` callback that
stays nonzero until completion. Main neither rewrites nor advances the pending
buffer while busy. A disconnected driver can also report busy. Never mix other
producers into the same byte stream while a frame is partially written.

Optional `service(user)` runs once at the beginning of every valid Main call,
including busy/faulted calls. Use it to maintain network connections. All
callbacks need bounded execution for real-time use; the core cannot make a
blocking driver asynchronous. The PubSubClient example has synchronous network
connection attempts and is therefore not suitable for hard real-time loops.

MQTT/CAN-FD adapters must accept an entire frame or return zero; individual
messages cannot contain arbitrary frame fragments. TCP/UART/USB streams allow
partial writes. CAN-FD padding must be removed before decoding a complete frame
in an adapter; Classic CAN uses the separate mapping described in PROTOCOL.md.

| Main result | Meaning |
| --- | --- |
| PLOTTER_OK | Complete frame accepted by driver; not a delivery acknowledgement |
| PLOTTER_IDLE | No sample due |
| PLOTTER_BUSY | Transport busy or a frame remains partially written |
| PLOTTER_SKIPPED | Sample callback returned zero |
| PLOTTER_BAD_SAMPLE | Non-finite required coordinate; measurement skipped |
| PLOTTER_BAD_CONFIG | Null/uninitialized context or invalid Init configuration |
| PLOTTER_IO_ERROR | Impossible write count; fault latched until repair/reinit |

## Optional C++ push API

Existing `Plotter`, `send`, `send2D`, `send3D`, and timestamp setup remain.
`send*` now returns true when a point is accepted into the fixed buffer; false
means busy, invalid data, no transport, or a driver fault. After acceptance,
call `flush()` cyclically until true to finish any short write. A second point
is rejected while the first is pending. The cyclic C API handles this scheduling
automatically and is recommended for new integrations.

`PlotterStream::busy()` defaults to false, requiring write to copy the data.
Override it for borrowed buffers. Plotter instances are non-copyable; stop
transfers before begin/reinitialization or destruction.

The entire library, including C++ wrappers, has no vendor/SDK dependency or
conditional target API. It knows only standard C/C++ types, configured callbacks
and the abstract PlotterStream. The millisecond callback always returns uint32_t.

Hardware adapters belong to the application. Optional reference adapters are
outside the package, in `examples/common/plotter_arduino.h` and
`examples/stm32/adapters/plotter_stm32.h`. The supplied cyclic examples already
implement hardware access in their own configuration callbacks.

Migration for the earlier Arduino C++ convenience overload: use
`PrintStream stream(Serial); Plotter sender(stream);` after including the
application-side Arduino adapter. Direct `Plotter(Serial)` and `begin(Serial)`
are removed. Wrap millis() with `uint32_t clock_ms() { return millis(); }` when
passing it to the C++ timestamp setter. The C Init/Main API is unchanged.
Consumers of the old STM32 header must include the example/application adapter
from its new location. Recompile all C++ consumers after this interface cleanup.

## Compatibility and verification

The [wire contract](PROTOCOL.md) remains **v6.0 / wire version 1**. Library 6.1
adds the cyclic API without changing one byte of the protocol. PlotterApp's
existing Serial/TCP/MQTT receivers and parser already support these frames.

From the repository root: `python tools/test_native.py --app ../PlotterApp`.
This covers codec bounds/CRC/flags, round-robin scheduling, clock wrap, invalid
configuration, backpressure, asynchronous buffers, independent instances,
C++ compatibility and 256 real C-generated channels decoded by the app.

## Coding rules and shared utilities

All active first-party C/C++ functions have at most one return, as their final
statement at function-body scope. Void functions may have no return. Error
paths use initialized status variables and small helpers; no exit macros or
goto-based substitutes. The source rule is checked automatically by the native
test runner and hence CI. Archived prototypes and the independent Events
submodule are outside the active production scope.

The independent [common component](src/common/README.md) provides reusable
bit/bitset operations, little-endian byte conversion and generic CRC8. Constant
mask combinations use `EMB_U8_OR`; runtime helpers use typed static inline
functions with checked shift counts. They are bundled in the package, requiring
no separate framework or library installation. Macros are not assumed to be
faster than compiler-inlined functions.

The C decoder commits to the output structure only after complete validation;
invalid input leaves it unchanged. These conventions improve consistency and
reviewability, but do not constitute a safety-standard compliance claim.
