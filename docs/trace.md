# Runtime trace firmware

KymoTrace is an opt-in firmware feature for recording bounded function/scope
events and sending them as wire-v2 frames. The `examples/esp32/trace_uart`
project is a separate ESP32 environment; the existing measurement, network and
stress-test environments keep their own configuration.

## Build and connect

From the trace example directory, build and flash with PlatformIO:

```sh
pio run -e esp32-trace
pio run -e esp32-trace -t upload
```

Close PlatformIO's serial monitor before opening the same UART in KymoStudio.
Use the baud rate printed in the example configuration. Trace frames share
that serial link, so do not run a second logger that writes arbitrary text to
it.

## Instrument code

Include `<kymo_instrumentation.h>` and the generated `kymo_trace_ids.h`. Place
`KYMO_TRACE_SCOPE(&recorder, KYMO_CONTEXT_MAIN, KYMO_SYMBOL_LOOP)` at a scope
whose duration you want to measure. Use `KYMO_TRACE_MARK` for discrete events
and the explicit flow send/receive macros only when both ends of a correlation
are instrumented. Keep transport servicing in the main loop; hooks only append
to fixed recorder storage and never transmit.

The demo uses a monotonic ESP32 microsecond clock and a per-boot session ID.
Its port critical callbacks serialize recorder access and its recorder storage
is statically allocated. Session metadata and the symbol/context dictionary
are resent periodically, so starting a Studio capture after the board boots
still receives the description of the trace.

## Studio views and capture

Create a **Runtime trace** chart for a timeline with lanes, markers, gaps and
density bands. Create a **Function flow** chart to inspect a selected span and
its nested calls. The same selected span and time cursor are shared between the
views. Capture the live session from the trace toolbar, then pause, step, seek
to the session start, or replay at a selected speed. CSV and Chrome Trace
exports are available from the same toolbar; opening a saved capture restores
the recorded session and dictionary.

## Capability limits

The demo builds on Arduino's precompiled ESP32 FreeRTOS kernel, so it does not
install the source-built hooks. ISR, scheduler, flow and core-state capability
claims remain false. Only explicit instrumentation sites appear in its trace.
Core-state coverage requires a proven source-built port adapter and a recovery
checkpoint only when the corresponding per-core trace ring is empty.

`src/kymo_freertos_trace_adapter.*` and
`src/kymo_freertos_trace_hooks.h` provide an opt-in source-built hook shim. Add
the shim from the `FreeRTOSConfig.h` that is used to compile the kernel, define
`KYMO_FREERTOS_TRACE_CORE_ID()` using the target's verified core-ID API, declare
the global `kymoFreeRtosTraceAdapter`, and initialize it with the recorder.
If the project already defines any `traceTASK_*` macro, preserve its behavior
explicitly in the kernel configuration before including the shim. Task handles
are opaque. Applications may install `KymoFreeRtosTrace_SetContextResolver`
to map a handle to a context ID in the generated dictionary; mapped IDs must
be unique among live tasks. The adapter advances that slot's generation when
the same declared context is recreated. If the resolver returns zero, the
adapter allocates an unnamed fallback ID from the high half of the ID space.
Fallback contexts have no generated label and should be treated as unknown by
Studio. IDs are never reused within a session.
The adapter is not enabled for this Arduino build because that kernel is
precompiled. Before enabling scheduler or ISR tracing, verify the exact
FreeRTOS hook call contexts, task-delete behavior, per-core callback semantics,
and paired critical-section implementation against the source-built SDK. Hooks
that run in interrupt or scheduler-critical context must not use an unverified
lock, allocate, or call transport code. Do not enable hooks at higher-priority
interrupt levels than the chosen critical primitive supports. Capability flags
stay false until SDK and hardware tests prove those conditions.

The reported ESP timer is a monotonic local clock shared across the ESP32 cores;
it is not synchronized to an external device. The current demo uses no
cross-device clock calibration.
