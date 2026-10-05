<p align="center">
  <a href="README.md"><img alt="English" src="https://img.shields.io/badge/%F0%9F%8C%90-English-15123A"></a>
  <a href="README.de.md"><img alt="Deutsch" src="https://img.shields.io/badge/%F0%9F%8C%90-Deutsch-A78BFA"></a>
</p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/kymotrace-logo-dark.svg">
    <img src="docs/images/kymotrace-logo-light.svg" alt="Kymotrace – Embedded Telemetry" width="480">
  </picture>
</p>

<h3 align="center">KymoProbe · The firmware side of Kymotrace</h3>

<p align="center">
  Ready-made firmware and examples that stream measurements from ESP32, Arduino and STM32 live to KymoStudio.
</p>

<p align="center">
  <img alt="License GPLv3 or commercial" src="https://img.shields.io/badge/License-GPLv3%20%7C%20commercial-15123A">
  <img alt="KymoCore 6.2.2" src="https://img.shields.io/badge/KymoCore-6.2.2-7C5CFF">
  <img alt="PlatformIO" src="https://img.shields.io/badge/PlatformIO-ESP32%20%C2%B7%20AVR%20%C2%B7%20STM32-FDE047">
</p>

---

## What is it about?

KymoProbe is the fastest way from a microcontroller to **live measurement
curves**. Flash the board, connect in
[KymoStudio](https://github.com/CodeName-666/kymostudio), and within minutes the
data scrolls across your screen. Then replace the test signals with your own
sensors, control variables or states.

Under the hood runs [KymoCore](https://github.com/CodeName-666/kymocore), a lean
library without heap and without RTOS. It schedules the measurements, encodes
them into 9 to 21 bytes and sends them over the interface of your choice.
KymoProbe shows what this looks like on real hardware: via UART, USB, Wi-Fi or
MQTT.

<p align="center">
  <img src="docs/images/kymostudio-workbench.png" alt="Microcontroller measurements live in KymoStudio" width="900">
  <br><sub>This is how the data arrives: time series and XY trace in KymoStudio.</sub>
</p>

## Live data in three steps

```sh
# 1. Clone with submodules (KymoCore lives in lib/KymoCore)
git clone --recurse-submodules https://github.com/CodeName-666/kymoprobe.git

# 2. Build and flash the ESP32
pio run -e nodemcu-32s -t upload

# 3. Open a serial connection in KymoStudio: 115200 baud, 8N1
```

The board sends two test signals (sine and sawtooth, 50 Hz each). KymoStudio
detects the binary frames automatically. Important: close the PlatformIO serial
monitor first.

## Sending your own measurements

Everything you need to adapt is in one file:
[`src/kymo_config.cpp`](src/kymo_config.cpp). There you define channels and
sampling intervals and provide the measured value:

```cpp
static uint8_t sample(void *, uint8_t id, KymoSample *out)
{
    out->value = id == 0 ? readTemperature() : readPressure();  // your sensors
    return 1;
}

static const KymoChannel channels[] = {
    {20, 0, 0},   // channel 0 every 20 ms
    {100, 1, 0}   // channel 1 every 100 ms
};
```

The main loop stays tiny: call `Kymo_Init()` once, then `Kymo_Main()` on every
pass. KymoCore decides which channel is due and never waits for the interface.

## What is included

| Example | Hardware | Shows |
|---|---|---|
| [Main project](src/) | ESP32 (NodeMCU-32S) | minimal configuration over UART |
| [simple_analog_example](examples/arduino/simple_analog_example/) | Arduino Uno, Nano, Mega | one analog value, as simple as possible |
| [multi_sensor_example](examples/arduino/multi_sensor_example/) | Arduino Uno, Nano, Mega | Y, XY and XYZ at different rates |
| [serial_example](examples/esp32/serial_example/) | ESP32, ESP32-S3, ESP32-C3 | serial transmission with timestamps |
| [wifi_mqtt_example](examples/esp32/wifi_mqtt_example/) | ESP32, ESP32-S3 | wireless via Wi-Fi and an MQTT broker |
| [stm32/uart_example](examples/stm32/uart_example/) | Nucleo F401RE, F411RE, Blue Pill | UART with the STM32 HAL |
| [stm32/usb_cdc_example](examples/stm32/usb_cdc_example/) | Blue Pill F103C8 | virtual COM port over USB CDC |
| [native](examples/native/) | PC | generate frames without hardware |

All examples have been built and checked; CI builds the most important ones automatically on every push.

## What it is used for

- **Tuning controllers:** compare setpoint and actual value of a PID loop live.
- **Testing sensors:** see noise, drift and response directly.
- **Drives and power electronics:** follow current, speed and position with timestamps.
- **Measuring wirelessly:** receive data from moving or remote devices with the MQTT example.
- **Learning and tinkering:** make measurement data tangible instead of reading columns of numbers in a terminal.

## How it all fits together

![From measurement to chart](docs/images/architecture.png)

| Project | Role |
|---|---|
| [KymoCore](https://github.com/CodeName-666/kymocore) | library: schedules, encodes and sends measurements (here as submodule `lib/KymoCore`) |
| **[KymoProbe](https://github.com/CodeName-666/kymoprobe)** | this repository: firmware and hardware examples |
| [KymoStudio](https://github.com/CodeName-666/kymostudio) | desktop app: receive, display, analyse, export |

## Documentation

- **[Manual](docs/MANUAL.md):** setup, examples, architecture, C/C++ API and checks
- **[KymoCore guide](https://github.com/CodeName-666/kymocore/blob/main/docs/GUIDE.md):** configuration, features, transports
- **[Protocol](https://github.com/CodeName-666/kymocore/blob/main/PROTOCOL.md):** byte layout of the frames
- **[Examples in detail](examples/README.md)** · **[Verification results](docs/verification.md)** (German) · **[Contributing](CONTRIBUTING.md)**

## License

Copyright (c) 2026 Christof Seidel. KymoProbe and the KymoCore library are
dual-licensed: **GPLv3** ([LICENSE](LICENSE)), free of charge for hobby,
tinkering, learning and open source, or a **commercial license** for companies
that distribute firmware or devices without disclosing their source code.
Details are in [COMMERCIAL.md](COMMERCIAL.md). `lib/Events` is MIT-licensed.
