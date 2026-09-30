# Arduino

Use `simple_analog_example` for two waveform channels, or
`multi_sensor_example` for Y/XY/XYZ. Both use `Plotter_Init` and `Plotter_Main`.
Change channel periods, flags and the sample callback in
`../common/arduino_serial_config.h`. Replace waveform generation with
`analogRead(A0)` for actual ADC measurements. Select Uno, Nano or Mega in the
project's platformio.ini. Serial connection: 115200, 8N1, PlotterApp binary v6.
See [example build instructions](../README.md).

For optional C++ push use, `../common/plotter_arduino.h` contains PrintStream.
Instantiate it explicitly and pass it to Plotter. The library itself does not
include Arduino Print.h. The recommended C Init/Main examples need no wrapper.
