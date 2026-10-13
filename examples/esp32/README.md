# ESP32

- [Measurement signals](measurement_signals/README.md): sine and sawtooth for live
  cursors, statistics, FFT, filters and triggers in KymoStudio.
- [Saved task trace example](task_trace_example/RESTORE.md): the previous ESP32
  FreeRTOS/ISR/pipeline example, independently rebuildable and uploadable.

The repository root is the main NodeMCU-32S example. `serial_example` supports
ESP32 DevKit/S3/C3 through the same C Init/Main API. `wifi_mqtt_example` uses
configurable WiFi/MQTT callbacks and publishes whole binary frames. Edit its
credentials, broker and unique client ID before use. PubSubClient reconnects
may block; this example is not a hard real-time networking driver.
See [example build instructions](../README.md).

For testing KymoStudio's limits, the root project provides 256-channel USB/TCP
load profiles, including full XYZ frames. See the
[ESP32 stress-test instructions and current limits](../../docs/esp32-stress-test.md).
