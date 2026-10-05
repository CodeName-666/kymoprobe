# Portable Init/Main example

This example runs on a host with a C++11 compiler. It uses the default features:
one scalar channel, no timestamp, no outgoing CRC. Ten `Plotter_Main` calls
generate ten frames using a simulated 20 ms clock step. The transport callback
copies each frame into application storage before returning; it does not retain
the library's buffer. There is no terminal output. Exit code zero means success.

From the unpacked **library root**:

```sh
g++ -std=c++11 -fno-exceptions -fno-rtti -Isrc examples/basic/main.cpp src/plotter_protocol.cpp src/plotter_runtime.cpp src/plotter.cpp -o plotter-basic
./plotter-basic
```

On Windows, name the output `plotter-basic.exe` and run `.\plotter-basic.exe`.
The compiler must be on PATH. No files from the surrounding PlotterEcu repository
are required. Keep `PLOTTER_ENABLE_RUNTIME=1`.

For a device, keep the persistent channel/configuration/context storage, replace
the clock/sample/write callbacks and call Init once, then Main from the cyclic
task. The complete Arduino/ESP32 application is in the package README. SDK-based
adapters remain application code; the packaged example itself is portable.
