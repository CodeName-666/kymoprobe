# Umschaltbare KymoProbe-Demos

`src/main.cpp` ist der einzige Arduino-Einstiegspunkt des Hauptprojekts.
`setup()` und `loop()` rufen die ausgewählte Demo auf. Die größeren Anwendungen
liegen in getrennten Modulen unter `src/`; sie besitzen keine eigenen
`setup()`-/`loop()`-Definitionen. Die Board-Beispiele bleiben unter `examples/`
verfügbar. In KymoCore liegen nur wenige kleine Referenzbeispiele.

## Auswahl

Ein benanntes PlatformIO-Profil bauen oder den Standard in
`include/demo_selection.h` ändern:

```cpp
#define KYMO_DEMO_MODE KYMO_DEMO_UART
```

Danach `pio run -e nodemcu-32s` ausführen. Auch die generierten UART- und
Trace-Modi werden automatisch erkannt. Benannte Profile überschreiben die
Header-Auswahl. Es wird jeweils eine Demo einkompiliert; der Wechsel erfolgt
beim Build.

| Modus | PlatformIO-Profil | Inhalt / Verbindung |
|---|---|---|
| `KYMO_DEMO_UART` | `nodemcu-32s-uart` | Zwei 50-Hz-Wellenformen, UART 115200, Zeitstempel |
| `KYMO_DEMO_MQTT` | `nodemcu-32s-mqtt` | Nur MQTT: Kanal 3, Sägezahn, 10 Hz |
| `KYMO_DEMO_TCP` | `nodemcu-32s-tcp` | Nur TCP: Kanal 2, Sinus, 10 Hz, Port 2323 |
| `KYMO_DEMO_NETWORK` | `nodemcu-32s-network` | TCP/MQTT mit unabhängigen Runtime-Puffern |
| `KYMO_DEMO_MAX_USB` | `nodemcu-32s-stress-usb` | 256 Kanäle ohne Ratenbegrenzung, UART 2000000 |
| `KYMO_DEMO_MAX_TCP` | `nodemcu-32s-stress-tcp` | 256 Kanäle ohne Ratenbegrenzung, TCP 2323 |
| `KYMO_DEMO_MAX_XYZ` | `nodemcu-32s-stress-tcp-xyz` | TCP-Lasttest mit XYZ/Zeit, 21-Byte-Frames |
| `KYMO_DEMO_MAX_UDP` | `nodemcu-32s-stress-udp` | 256 Kanäle über UDP an Port 2324 des Studio-PCs, ohne Gegendruck |
| `KYMO_DEMO_FEATURES` | `nodemcu-32s-features` | C++-Sender, Y/XY/XYZ, Zeit, CRC und Decoder |
| `KYMO_DEMO_GENERATED` | `nodemcu-32s-development` | UART-Konfiguration aus dem Configurator |
| `KYMO_DEMO_TRACE` | `nodemcu-32s-trace` | Scope/Marker, Metadaten, Dictionary, UART v2 460800 |
| `KYMO_DEMO_TRACE_FLOW` | `nodemcu-32s-trace-flow` | Trace mit Send/Receive-Korrelation |

Aus dem KymoProbe-Verzeichnis:

```sh
pio run -e nodemcu-32s-tcp
pio run -e nodemcu-32s-features
pio run -e nodemcu-32s-trace-flow
```

Zum Flashen `-t upload --upload-port COM4` mit dem tatsächlichen Board-Port
ergänzen. KymoStudio auf die passende Baudrate bzw. TCP-/MQTT-Verbindung
einstellen. Binäre UART-Ausgabe nicht mit einem Textmonitor teilen.

## Netzwerk und maximale Datenrate

Für WLAN `include/network_secrets.h.example` nach `include/network_secrets.h`
kopieren und SSID/Passwort eintragen. Diese Datei ist Git-ignoriert.
Der USB-Monitor der Netzwerkmodi zeigt die Board-IP bei 115200 Baud.
MQTT-Broker, Topic und Client-ID stehen in `src/network_demo.cpp`.
MQTT sendet ganze Frames, QoS 0, nicht retained; der vorbelegte öffentliche
Broker ist für künstliche Signale gedacht. TCP ist ein binärer Datenstrom.

Die Max-Performance-Demo arbeitet mit festen 4-KiB-Batches, Teilübertragungen
und höchstens 512 Core-Schritten pro Schleifendurchlauf. Kein Lastprofil
veröffentlicht MQTT-Nachrichten. `KYMO_STRESS_CHANNEL_COUNT`,
`KYMO_STRESS_PERIOD_MS` und `KYMO_STRESS_BAUD` konfigurieren den Lastgenerator.
Der Durchsatz hängt von Board, UART-Adapter, WLAN und Empfänger ab;
SDK-Netzwerkaufrufe können blockieren. Siehe [Lasttest](esp32-stress-test.md)
und [Netzwerkdetails](esp32-network-demo.md).

## Feature-Demos

Die Feature-Demo sendet vier Kanäle bei 115200 Baud: Kanal 0 ist Y,
Kanal 1 XY, Kanal 2 XYZ, jeweils mit Zeitstempel und CRC. Kanal 3 meldet
`1`, wenn der lokale XYZ/Zeitstempel-Decoder-Roundtrip erfolgreich war und
ein beschädigter CRC-Frame korrekt abgewiesen wurde; andernfalls `0`.
Der C++-Sender behandelt Sendestau über `flush()` ohne Sample-Duplikate.

Trace sendet Session-Metadaten, Symbol-Dictionary und begrenzte Event-Batches
über den Transport-Multiplexer. In KymoStudio eine Runtime-Trace-Ansicht anlegen.
Das Flow-Profil verbindet explizite Producer-/Consumer-Ereignisse mit derselben
Korrelations-ID. ISR-/Scheduler-Erfassung und entsprechende Vollständigkeits-
angaben bleiben deaktiviert, bis ein echter Port verifiziert ist.

## Configurator und Compile-out

UART verwendet `kymo.project.json`; Trace verwendet
`examples/esp32/trace_uart/kymo.project.json`. Die ausgewählten unveränderlichen
Snapshots sind eingecheckt, Firmware-Builds benötigen kein Python/Qt.
Nach Konfigurationsänderungen Profile mit dem Configurator neu generieren.
Trace besitzt `development`, `production` und `flow`.

`nodemcu-32s-production` und `nodemcu-32s-trace-production` setzen
`KYMO_ENABLED=0` und bauen eine leere zentrale Anwendung. Der Auswahl-Header
wird projektweit eingebunden; bei generierten Modi liefert der Snapshot die
Core-Feature-Werte.

Die Library bleibt das auf einen offiziellen Release gepinnte Submodul
`lib/KymoCore`. Library-Entwicklung erfolgt ausschließlich im separaten
KymoCore-Repository.
