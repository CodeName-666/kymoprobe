# PlotterLib examples

Primary examples (run from repository root):

```sh
pio run -e nodemcu-32s
pio run -d examples/arduino/simple_analog_example -e uno
pio run -d examples/arduino/multi_sensor_example -e uno
pio run -d examples/esp32/serial_example -e esp32dev
pio run -d examples/esp32/wifi_mqtt_example -e esp32dev
```

Arduino and serial ESP32 examples share `common/arduino_serial_config.h` and
send deterministic waveforms through the C Init/Main API. Modify the sample
callback to acquire actual sensor values. The multidimensional example uses
three independently scheduled channels. The `.ino` is the single entrypoint
source; PlatformIO's `src/main.cpp` includes it to avoid duplicate code.
Keep `examples/common` with the sketches when using the Arduino IDE.

Serial examples: 115200 baud, 8N1, binary v6. Connect PlotterApp to the board's
serial port. Do not share that port with a text monitor. MQTT: edit WiFi/broker
settings, subscribe PlotterApp to `sensor/data`; one whole frame per payload.
PubSubClient connect is synchronous even though sampling is cyclic.

Optional portability examples:

```sh
pio run -d examples/stm32/uart_example -e nucleo_f401re
pio run -d examples/stm32/usb_cdc_example -e bluepill_f103c8
pio run -d examples/native -e native
```

STM32 UART is C with STM32Cube HAL and interrupt-driven USART2 TX on PA2;
no CubeMX-generated files are required. Use a 3.3 V serial adapter or the
Nucleo ST-Link virtual COM port. USB CDC uses STM32duino on the Blue Pill's
native USB connector (not the programmer connector). Other listed boards
must be validated on their actual hardware. Native requires gcc/g++ on PATH;
run the generated `.pio/build/native/program` (Windows: `program.exe`).

Only the ESP32 and Arduino examples are required for the primary workflow.
See `../docs/verification.md` for exactly which targets were compiled here;
compilation is not a hardware test.
