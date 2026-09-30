# PlotterEcu

Universelle Embedded-Telemetrie für PlotterApp: C99-Kern, statische Konfiguration,
einmal `Plotter_Init()` und zyklisch `Plotter_Main()`. Das Hauptprojekt ist ein
ESP32-PlatformIO-Projekt. Der Kern benötigt weder Arduino noch RTOS oder Heap.

## Start mit ESP32

```sh
pio run -e nodemcu-32s
pio run -e nodemcu-32s -t upload
```

`src/main.cpp` initialisiert Serial mit **115200 Baud** und ruft Init/Main auf.
`src/plotter_config.cpp` enthält Kanäle, Abtastintervalle, Messwertfunktion und
Transport. Im Beispiel senden Kanal 0 und 1 Sinus/Sägezahn mit je 50 Hz und
Zeitstempeln. Eigene Messwerte werden im `sample`-Callback übernommen.

In PlotterApp eine serielle Verbindung zum Board mit **115200, 8N1** öffnen.
Die App erkennt die Binärframes automatisch. Die Datenbytes sind kein Text;
den Port nicht gleichzeitig mit dem PlatformIO-Serial-Monitor öffnen.
Kanalnamen/Einheiten werden in der App eingestellt, nicht über v6 übertragen.

## Arduino

```sh
pio run -d examples/arduino/simple_analog_example -e uno
pio run -d examples/arduino/simple_analog_example -e uno -t upload
pio run -d examples/arduino/multi_sensor_example -e uno
```

Die Beispiele erzeugen reproduzierbare Testsignale. Im gemeinsamen
`examples/common/arduino_serial_config.h` kann `example_sample` durch
`analogRead()` oder eigene Sensorwerte ersetzt werden. Das zweite Beispiel
zeigt Y, X/Y und X/Y/Z mit unabhängigen Abtastraten. Arduino Uno/Nano/Mega
und ESP32-Projekte nutzen denselben C-Kern.

## Library und Schnittstellen

Die vollständige Header-API lässt sich mit `doxygen Doxyfile` erzeugen.
[Doxygen-Anleitung](docs/doxygen.md) beschreibt Ausgabe und Dokumentationsregeln.

[PlotterLib](lib/PlotterLib/README.md) dokumentiert die Konfiguration, Speicher-
und Callback-Verträge. [PROTOCOL.md](lib/PlotterLib/PROTOCOL.md) definiert das
mit PlotterApp identische Wire-Format. Es sind 9–21 Bytes pro Messung:
bytebasierte IDs/Flags, explizites Little Endian, float32, optionale uint32-
Millisekunden und CRC8. Keine Pointer oder C-Strukturen werden direkt gesendet.

Für andere ECUs werden ausschließlich Zeit-, Mess- und Transport-Callbacks
angepasst. UART, USB CDC, TCP oder MQTT brauchen keinen anderen Codec.
Asynchrone Treiber melden Busy, bis der übergebene Puffer nicht mehr verwendet
wird. Kurze Writes werden beim nächsten Main fortgesetzt. Classic CAN benötigt
sein natives Mapping; ein v6-Frame passt erst in CAN-FD.

## Prüfung

```sh
python tools/test_native.py --app ../PlotterApp
# Alternativ: nur C/C++-Tests ohne App
python tools/test_native.py
```

Benötigt gcc/g++. Unter Windows findet das Skript auch die PlatformIO-Toolchain:
`pio pkg install -g -t platformio/toolchain-gccmingw32`.
Die Prüfung kompiliert C99 und C++11 mit Warnungen als Fehler und prüft echte
C-Senderbytes gegen den Python-Decoder/Parser. Die App-Gesamtsuite kann im
App-Verzeichnis mit `python -m pytest -q` ausgeführt werden.

Weitere Beispiele und Build-Kommandos: [examples](examples/README.md).
Analyse/Entscheidungen: [docs/embedded-design.md](docs/embedded-design.md).
Prüfergebnisse: [docs/verification.md](docs/verification.md).

Der frühere A5A5-Prototyp wurde nach `legacy/` verschoben. Er wird nicht mehr
gebaut und ist nicht mit PlotterApp v6 kompatibel. Das unabhängige Events-
Submodul bleibt unverändert und ist keine Library-Abhängigkeit.

## Embedded-Coderegeln

Aktive C/C++-Funktionen verwenden höchstens ein `return` am Funktionsende.
Bitoperationen, Bytekonvertierung und CRC sind in einer unabhängigen
[Common-Komponente](lib/PlotterLib/src/common/README.md) gebündelt.
`python tools/test_native.py` prüft die Coderegel und die Hilfsfunktionen mit.
Die dauerhaften Vorgaben stehen in [AGENTS.md](AGENTS.md).
