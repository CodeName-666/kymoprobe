# Plotter Compact Binary Protocol v6.0

This document is the authoritative wire-format contract between embedded
senders and PlotterApp. The primary format is compact binary. PlotterApp still
accepts the v5 JSON format so existing firmware does not need to be upgraded
immediately.

## Design goals

- 9â€“21 bytes per measurement
- one write per measurement when the driver accepts a full frame; retries for short writes
- constant-time frame length calculation
- no allocation, `snprintf`, JSON library, or float-to-text conversion on MCU
- deterministic little-endian representation
- recovery after dropped or corrupt stream bytes
- CRC protection per measurement

The protocol transports telemetry from a device to PlotterApp. A command or
configuration protocol in the opposite direction is not part of v6.0.

## Binary data frame

```text
Offset  Size  Field
0       1     Sync 0: 0xA5
1       1     Sync 1: 0x5A
2       1     Descriptor
3       1     Channel ID (0..255)
4       0/4   X, IEEE-754 float32 little-endian
4/8     4     Y/value, IEEE-754 float32 little-endian
...     0/4   Z, IEEE-754 float32 little-endian
...     0/4   Relative timestamp in milliseconds, uint32 little-endian
last    1     CRC-8/ATM
```

Payload order is always `X?`, `Y`, `Z?`, `timestamp_ms?`. Y is the only
mandatory numeric field. Optional fields are indicated by the descriptor, so
no length byte is necessary.

### Descriptor

```text
Bit 7..6  Wire version: 01
Bit 5..4  Message type: 00 (data point)
Bit 3     X present
Bit 2     Z present
Bit 1     Timestamp present
Bit 0     Reserved, must be 0
```

Valid data descriptors are `0x40`, `0x42`, `0x44`, `0x46`, `0x48`, `0x4A`,
`0x4C`, and `0x4E`. Unsupported versions, message types, or reserved bits are
rejected.

### Frame sizes

| Measurement | Without timestamp | With timestamp |
|---|---:|---:|
| Y | 9 bytes | 13 bytes |
| X/Y | 13 bytes | 17 bytes |
| Y/Z | 13 bytes | 17 bytes |
| X/Y/Z | 17 bytes | 21 bytes |

### Numeric rules

- Channel ID is an unsigned byte, 0â€“255.
- X, Y, and Z are finite IEEE-754 binary32 values. NaN and infinity are invalid.
- Timestamp is an unsigned 32-bit count of milliseconds relative to the sender
  start time. It wraps after approximately 49.7 days.
- Multi-byte values are little-endian regardless of MCU endianness.
- Binary32 intentionally trades decimal text precision for fixed size and fast
  MCU encoding. It provides roughly seven significant decimal digits.

### CRC

CRC-8/ATM parameters:

- polynomial: `0x07`
- initial value: `0x00`
- input/output reflection: false
- xor-out: `0x00`
- covered bytes: descriptor, channel, and payload; sync bytes are excluded

A receiver must discard a frame with a bad CRC and search for the next
`A5 5A` sync marker.

## Golden vectors

These vectors are shared by the Python and Embedded implementations.

```text
Point: id=7, y=1.0
Bytes: A5 5A 40 07 00 00 80 3F 54
Size:  9

Point: id=3, x=1.25, y=-2.5, z=9.0, timestamp=1234 ms
Bytes: A5 5A 4E 03 00 00 A0 3F 00 00 20 C0 00 00 10 41 D2 04 00 00 89
Size:  21
```

## Embedded sender interface

PlotterLib 6.1 adds a portable C99 cyclic API without changing this v6 wire
contract. `Plotter_Init(context, config)` binds a static configuration;
`Plotter_Main(context)` schedules samples, encodes frames and resumes short
writes. See [README.md](README.md) for configuration and transport ownership.

The optional C++ push API remains available:

```cpp
plotter.send(0, temperature);           // Y, 9 bytes
plotter.send2D(1, position, force);     // X/Y, 13 bytes
plotter.send3D(2, x, y, z, true);        // X/Y/Z + timestamp, 21 bytes
```

A true send result means accepted into the fixed 21-byte buffer. Call `flush()`
cyclically to complete a short write before submitting another point. Timestamp
use in the C++ API still requires `setMillisecondCallback` and `setStartTime`.
Async transports must report busy until they release the transmitted buffer.

## Throughput

At 115200 baud, UART 8N1 carries approximately 11,520 wire bytes per second.
Ignoring application and driver overhead, this permits approximately:

| Frame | Binary points/s | Equivalent v5 JSON points/s |
|---|---:|---:|
| Y | 1,280 | 443 |
| X/Y/Z + timestamp | 548 | 155 |

The minimal binary frame is 9 bytes versus 26 bytes for the previous formatted
JSON example. The full golden vector is 21 bytes versus 74 bytes as JSON.

## Transport rules

### Serial, USB CDC, and Telnet/TCP

Frames may be fragmented or concatenated arbitrarily. Receivers must use sync,
descriptor-derived length, and CRC; packet boundaries from individual reads
have no meaning. JSON compatibility messages remain newline-delimited.

### MQTT

One MQTT payload may contain one or more concatenated binary frames. MQTT
message boundaries are sufficient, but the normal stream decoder is used so
concatenation behaves identically to Serial/TCP.

### CAN

For Classic CAN, the native compact mapping is preferred:

- arbitration ID maps to Plotter channel (`data_id` can override it)
- CAN data contains the numeric value in the configured `float32`, `float64`,
  signed, or unsigned format
- scaling and offset are applied by the receiver adapter

This uses only four CAN data bytes for a float32 measurement. CAN-FD may carry
an entire v6 binary frame unchanged when X/Z/timestamp or CRC-compatible
cross-transport framing is required.

## Legacy JSON compatibility

PlotterApp continues to accept UTF-8 JSON:

```json
{"id":0,"value":25.5}
{"id":0,"value":25.5,"timestamp":1.234}
{"id":0,"x":10.5,"y":25.5}
{"id":0,"x":10.5,"value":25.5,"z":99.9,"timestamp":1.234}
```

Rules:

- `id`: required integer, 0â€“255
- `value` or `y`: required finite number
- `x`, `z`: optional finite numbers
- `timestamp`: optional finite, non-negative seconds
- a plain finite number is accepted as legacy channel 0
- stream transports require `\n`; MQTT may use one JSON object per payload

JSON is a compatibility and diagnostics format, not the recommended embedded
format. New embedded firmware should emit binary v6.

## Implementations

- Python codec: `PlotterApp/python/Receiver/binary_protocol.py`
- Backend parser: `PlotterApp/python/Backend/backend.py::_parse_data_point`
- Embedded codec: `src/plotter_protocol.h/.c`
- Embedded cyclic runtime: `src/plotter_runtime.h/.c`
- Optional C++ sender: `src/plotter.h/.cpp`
- Embedded golden-vector test: `test/protocol_golden_test.cpp`
- Desktop golden-vector tests: `PlotterApp/tests/test_binary_protocol.py`

## Versioning

- Protocol v6.0 / wire version 1: compact binary frames; JSON v5 receive fallback
- Protocol v5.0: JSON X/Y/Z format
- Protocol v4.0: JSON timestamp format

Any future incompatible binary layout must use a new wire-version value. New
message semantics that fit the current layout may use a reserved message type,
but v6 receivers must reject types they do not understand.
