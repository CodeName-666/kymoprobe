# ESP32

The repository root is the main NodeMCU-32S example. `serial_example` supports
ESP32 DevKit/S3/C3 through the same C Init/Main API. `wifi_mqtt_example` uses
configurable WiFi/MQTT callbacks and publishes whole binary frames. Edit its
credentials, broker and unique client ID before use. PubSubClient reconnects
may block; this example is not a hard real-time networking driver.
See [example build instructions](../README.md).
