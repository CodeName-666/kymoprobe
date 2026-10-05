# Prüfstand — 2026-10-05

## PlatformIO-Paketvorbereitung vom 05.10.2026

Die Library wurde als eigenständiges Paket validiert. Ergänzt sind Lizenztext,
Changelog, Veröffentlichungshinweise, ein portables Beispiel, ausführliche
README-Anleitung mit KymoStudio-Link und drei PNG-Diagramme. Der bisherige
Arbeitsstand einschließlich C++11-/Feature-Umstellung bleibt die Grundlage.
Diese Paketvorbereitung verändert keine öffentliche API und kein Wire-Verhalten.

Frisch erfolgreich ausgeführt:

- `python tools/test_native.py --app ../KymoStudio`: sechs Checker-Tests,
  Single-Return-Prüfung über 40 Quelldateien, interne Headerkonfiguration,
  Featureprofile, beide CRC-Modi und App-Abgleich mit 256 Kanälen, acht Layouts
  und 42 Fragmentgrößen.
- `python tools/test_package.py`: Manifest durch PlatformIO gepackt,
  Archivinhalt/Versionsgleichheit/lokale Dokumentationslinks geprüft,
  portables Beispiel aus dem entpackten Archiv kompiliert und ausgeführt.
  README-Sketch in getrennten Projekten ausschließlich gegen das installierte
  Paketarchiv für Uno und ESP32 gebaut.
- Nach Bereinigung einer Python-Tar-Deprecation-Warnung den Paket-/Hostteil mit
  `python tools/test_package.py --skip-firmware` erneut erfolgreich ausgeführt.
- Die sechs bestehenden Firmwareziele erneut gebaut; Details unten.
- Drei erzeugte Diagramme visuell geprüft; `git diff --check` ohne Inhaltsfehler.

| Aktueller Build | RAM der Demo | Flash der Demo |
|---|---:|---:|
| README aus Archiv / uno | 250 B | 2.884 B |
| README aus Archiv / esp32dev | 21.504 B | 268.109 B |
| Hauptprojekt / nodemcu-32s | 21.504 B | 272.029 B |
| Arduino simple / uno | 258 B | 3.804 B |
| Arduino multidimensional / uno | 268 B | 4.148 B |
| ESP32 serial / esp32dev | 21.504 B | 272.077 B |
| ESP32 MQTT / esp32dev | 44.432 B | 720.465 B |
| STM32 UART C/HAL / nucleo_f401re | 160 B | 10.596 B |

Das Paket liegt lokal unter `.pio/package/KymoCore-6.1.0.tar.gz`, der
zugehörige SHA256-Wert in der benachbarten `.sha256`-Datei. Jeder erneute
Paketlauf aktualisiert das Archiv; vor Upload immer den letzten Stand verwenden.
CI wurde um denselben Paketcheck ergänzt, aber nicht remote ausgeführt.

Nicht Bestandteil dieser Prüfung: Upload in die Registry, Registry-Rendering,
Prüfung des verfügbaren Owner-/Versionsnamens, elektrische Boardtests oder die
komplette App-Testsuite. Der App-Abgleich verwendet den lokalen Nachbarcheckout,
nicht den öffentlich verfügbaren GitHub-Stand. Die Hinweise in
`lib/KymoCore/PUBLISHING.md` beschreiben die verbleibenden Release-Schritte.

## Historischer Prüfstand vom 30.09.2026

Die folgenden Ergebnisse dokumentieren den damaligen Stand. Für die aktuelle
Paketvorbereitung gelten die oben neu ausgeführten Prüfungen.

## Plattformtrennung

STM32-Adapter liegen jetzt unter `examples/stm32/adapters/`, der optionale
Arduino-Print-Adapter unter `examples/common/`. KymoCore enthält keine
SDK-Header, Herstellertypen oder ARDUINO-/STM32-Abzweigungen mehr. C-API und
Wire-Protokoll sind unverändert. Frühere Arduino-C++-Komfortüberladungen werden
durch explizite anwendungsseitige Stream-Adapter ersetzt.

Ein zusätzlicher nativer Build definiert ARDUINO, STM32 und ESP_PLATFORM,
kompiliert dabei ohne SDK und führt die C++-Golden-Vector-Tests aus. Dieser
Build scheiterte vor der Trennung an Print.h und besteht jetzt. Native Tests
und App-Decoder-Abgleich sowie die Builds ESP32-Hauptprojekt, Arduino Uno und
STM32-C/HAL wurden nach dieser Trennung erneut ausgeführt.

## Single-Return-/Common-Überarbeitung

Alle 35 aktiven C/C++-Quelldateien (Library, Anwendung, Beispiele und native
Tests) bestehen `tools/check_embedded_style.py`: höchstens ein Return am Ende
der Funktion, keine Returns in Makros. Sechs Tests prüfen den Strukturchecker
selbst. Es handelt sich um eine Prüfung des hier verwendeten Sprachumfangs,
nicht um einen vollständigen C++-Parser oder eine Safety-Zertifizierung.

Bitmasken/Bitsets, Little-Endian/binary32-Konvertierung und CRC sind im
wiederverwendbaren Header-Modul `lib/KymoCore/src/common/` gebündelt und
werden separat als C99 und C++11 getestet. Dynamische Bitindizes werden vor
Shifts begrenzt. Alle sechs Firmwareziele wurden nach dem Umbau neu gebaut.

Der statische RAM-Bedarf ist unverändert. Flash stieg im einfachen Uno-Beispiel
von 3.968 auf 4.222 Bytes (+254), im ESP32-Hauptprojekt von 272.249 auf 272.321
Bytes (+72). Das ist kein Geschwindigkeitsnachweis; Laufzeit/Stack wurden nicht
auf Hardware gemessen. Ein neuer Fehlerpfadtest belegt: Auch CRC-gültige Frames
mit nicht-endlichen Messwerten verändern bei Ablehnung den Ausgabedatensatz
nicht mehr. Wire-Format und gültige Ergebnisse bleiben unverändert.

## Erfolgreiche lokale Prüfungen

Windows, PlatformIO Core 6.1.19. Plattformversionen sind in den INI-Dateien
festgelegt: espressif32 6.12.0, atmelavr 5.3.0, ststm32 20.0.0.

| Projekt / Ziel | Ergebnis | RAM der gesamten Demo | Flash der gesamten Demo |
| --- | --- | ---: | ---: |
| Hauptprojekt / nodemcu-32s | Build erfolgreich | 21.504 B | 272.321 B |
| Arduino simple / uno | Build erfolgreich | 258 B | 4.222 B |
| Arduino multidimensional / uno | Build erfolgreich | 268 B | 4.268 B |
| ESP32 serial / esp32dev | Build erfolgreich | 21.504 B | 272.445 B |
| ESP32 MQTT / esp32dev | Build erfolgreich | 44.432 B | 720.705 B |
| STM32 UART C/HAL / nucleo_f401re | Build erfolgreich | 160 B | 10.820 B |

Das sind Linkerwerte inklusive Framework, Beispiel und Transport, keine
isolierten Library-Messungen oder maximale Stack-/Heap-Werte. Der C-Kontext
ist im 32-Bit-Host-Test 36 Bytes groß, auf AVR laut ELF-Symboltabelle 32 Bytes.
Für die zwei AVR-Kanäle kommen 8 Bytes Laufzeit-Zeitstempel, 12 Bytes
Kanalkonfiguration und 22 Bytes Callback-Konfiguration hinzu. AVR legt diese
gewöhnlichen const-Tabellen in RAM ab; es wird kein PROGMEM-Zugriff behauptet.

`python tools/test_native.py --app ../KymoStudio` ist erfolgreich:

- Common-Helfer, Single-Return-Regel und deren Checker-Tests;
- C99 und C++11, `-Wall -Wextra -Werror -pedantic`;
- feste Protokollvektoren und CRC-8/ATM-Prüfwert F4 für `123456789`;
- alle 256 Deskriptoren, Ausgabepuffergrenzen, NaN/Inf und beschädigte Frames;
- Init-Validierung, Perioden, Round-Robin, UINT32-Zeitüberlauf, Samplefehler;
- kurze/Null-Writes, Busy, asynchrone Pufferlebensdauer und Treiberfehler;
- voneinander unabhängige Instanzen und optionaler C++-Sender/CDC-Adapter;
- 256 vom echten C-Init/Main-Sender erzeugte Kanäle, alle acht Feldlayouts;
- bytegenauer Abgleich mit Python-Encoder und Verarbeitung durch echten
  KymoStudio-Streamdecoder und Core-Parser bei 42 Fragmentgrößen;
- Wiederaufnahme nach beschädigtem CRC.

Die Gesamtsuite im tatsächlichen KymoStudio-Verzeichnis ist erfolgreich:
**154 passed, 4 subtests passed** mit `.venv/Scripts/python.exe -m pytest -q`.
Die App-Quellen wurden bei dieser ECU-Änderung nicht bearbeitet. Ihr vorhandenes
v6-Protokoll erfüllt den Vertrag bereits. Zwischenzeitlich vorhandene
Windows-Testprobleme waren im abschließend geprüften App-Stand behoben.

`git diff --check` ist ohne Inhaltsfehler. Git meldet lediglich die lokale
LF/CRLF-Normalisierung. Eine CI-Konfiguration für native Tests und die sechs
oben gebauten Firmware-Ziele liegt unter `.github/workflows/embedded.yml`;
die Remote-CI wurde in dieser Sitzung nicht gestartet.

## Reproduktion

```sh
python tools/test_native.py --app ../KymoStudio
pio run -e nodemcu-32s
pio run -d examples/arduino/simple_analog_example -e uno
pio run -d examples/arduino/multi_sensor_example -e uno
pio run -d examples/esp32/serial_example -e esp32dev
pio run -d examples/esp32/wifi_mqtt_example -e esp32dev
pio run -d examples/stm32/uart_example -e nucleo_f401re
```

PlatformIO-Buildlogs liegen lokal in `.pio/*build.log` (nicht versioniert).
Unter Windows ist PlatformIO hier unter
`C:/Users/NoName/.platformio/penv/Scripts/pio.exe` installiert.

## Grenzen

Keine Boards wurden geflasht oder elektrisch geprüft. Serielle Live-Übertragung,
USB-Enumeration, DMA/IRQ-Timing und ein realer MQTT-Broker sind deshalb nicht
hardwareverifiziert. Nano/Mega, ESP32 S3/C3, STM32 F411/F103 und das optionale
USB-Projekt wurden nicht gebaut. Der native PlatformIO-Beispielwrapper wurde
nicht separat gebaut; seine C-Tests wurden direkt mit GCC ausgeführt.

Der Schwerpunkt liegt entsprechend der Präzisierung auf ESP32 und Arduino.
Für eigene ECUs muss der jeweilige Callback-Adapter mit dem tatsächlichen
Treiber geprüft werden. Das Wire-Protokoll selbst bleibt unverändert.

## Referenzen für Build-Metadaten

Die universelle Manifestkonfiguration folgt den offiziellen
[PlatformIO-Manifestangaben](https://docs.platformio.org/en/latest/manifests/library-json/index.html).
Die optionale STM32duino-CDC-Konfiguration orientiert sich am
[offiziellen Core-Buildskript](https://github.com/stm32duino/Arduino_Core_STM32/blob/main/tools/platformio/platformio-build.py).
Das ersetzt keinen Build-/Hardware-Nachweis für dieses optionale Ziel.
