<!-- SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoProbe-Commercial -->
# Entwicklungspläne: Implementierung und Nachweise

Stand: 11. Oktober 2026. Zuerst wurden Compile-out und Konfigurator umgesetzt,
danach die Trace-Funktionen. KymoCore 6.3.1 und KymoStudio 2.1.0 sind
veröffentlicht. Die Probe-Erweiterungen werden mit diesem Stand in main
integriert; es wurde keine Firmware geflasht.

## Ergebnis

Plan 2: explizite Feature-Auflösung und vollständiger globaler Compile-out,
wirkungslose Instrumentierungsmakros ohne Argumentauswertung, versionierte
Projektdateien, CLI und Qt-Konfigurator, unveränderliche Generationen mit
Integritätsprüfung, Build-Exports für GCC/CMake/PlatformIO, reale Laufzeit- und
Trace-Callbackverdrahtung sowie Entwicklung-/Produktionsprofile in Probe.

Plan 1: normativer Wire-v2-Vertrag und gemeinsame Fixtures, statischer Recorder,
Transport-Multiplexer mit Backpressure und geliehenen Puffern, generierte
Metadaten und Dictionary, ESP32-Demo, Source-Build-FreeRTOS-Hookadapter,
getrennte Studio-Empfangswege, konservative Aufrufrekonstruktion, Laufzeit- und
Funktionsablaufdiagramm, begrenztes Anzeigefenster, Capture/Replay einschließlich
Cursor-Sprung, CSV/Chrome-Export und explizite Übergabekorrelation.
CPU- und Eigenzeiten werden nur bei belegter vollständiger Abdeckung berechnet.
Der synthetische CPU-Referenzfall besteht; die Demo behauptet diese
Hardwareabdeckung ausdrücklich noch nicht.

## Verifikation

| Prüfung | Ergebnis |
|---|---|
| Studio: vollständiges pytest | 257 Tests und 35 Untertests bestanden; ein expliziter GPU-Hardwaretest übersprungen |
| Konfigurator einschließlich echter Qt-Workflows | 22 Tests und 26 Untertests bestanden |
| Generierter shared-v2-Code | C++ kompiliert und ausgeführt; META, DICTIONARY, BATCH, DATA und Adapterfreigabe geprüft |
| Probe `tools/test_native.py --app ../KymoStudio` | Alle nativen Prüfungen bestanden, einschließlich Compile-out und Trace |
| Native Trace-Demo | 267 Frames, 256 Spans und 128 Marker erfolgreich in Studio eingelesen; FreeRTOS-Mock bestanden |
| ESP32 Trace-Build | RAM 27.400 / 327.680 Byte; Flash 273.625 / 1.310.720 Byte |
| Probe Entwicklung / Produktion | RAM 21.504 / 21.032 Byte; Flash 272.109 / 233.257 Byte |
| Bestehende Firmwarebeispiele | ESP32 Serial/MQTT, AVR Simple/Multi und STM32 UART/USB bauten erfolgreich |
| Qt-Start und Trace-Renderer | Start ohne QML-Fehler; beide Trace-Ansichten gerendert und geprüft |
| QML-JavaScript | 50 Dateien, 5 Skripte und 332 eingebettete Funktionen geprüft |
| Doxygen | Erfolgreich, leere Warnungsdatei |
| Paketprüfung | Archivinhalt, Dokumentationslinks und tatsächlich entpacktes natives Beispiel bestanden |

Die Firmwaregrößen beschreiben unterschiedliche Programme und sind keine
isolierte Messung des Recorder-Overheads.

## Lastmessung

Rechner: Windows 10 19045, AMD64 Family 21 Model 2, Python 3.12.9.
`KymoStudio/tools/benchmark_trace.py --projection` verarbeitet deterministisch
300.000 Ereignisse in 50.000 Frames, entsprechend 60 Sekunden Quellzeit bei
5.000 Ereignissen/s. Gemessen: 6,86 Sekunden Verarbeitung (43.719 Ereignisse/s).
Für 30 Zoom-/Pan-Fenster: p50 10,44 ms, p95 30,11 ms, Maximum 196,15 ms
(einschließlich erster Indexerstellung). Dies misst Datenaufbereitung, nicht
GPU-Zeichenrate oder die vollständige Eingabelatenz.

Ein separater Lauf mit `--memory` ergab 115.024.369 Byte Spitzenbelegung der
von tracemalloc erfassten Python-Allokationen. Die zuvor erzeugten Framebytes
und native Qt-/GPU-Allokationen sind darin nicht enthalten. Dieser instrumentierte
Lauf dauerte 84,17 Sekunden; seine Datenrate ist keine normale Laufzeitmessung.
Ein zusätzlicher Test mit 300.005 Ereignissen sowie kleinen konfigurierten
Fenstern prüft fortlaufende Verarbeitung und Eviktion statt Sitzungsabbruch.

## Review und verbleibende Abnahme

Standards-Review: fehlende Lizenzköpfe und unzureichende öffentliche API-Dokumentation
wurden nachgebessert. Spezifikationsreview: die zunächst fehlende generierte
Recorder-/Mux-Verdrahtung wurde implementiert und durch einen ausführbaren
Regressionstest abgesichert. Weitere Reviewkorrekturen betreffen DMA-Pufferbesitz,
Verlustrekonstruktion, CPU-Zuordnung, Fenstergrenzen und die unverfälschte
64-Bit-Zeitdarstellung.

Die vollständige Definition of Done von Plan 1 ist noch **nicht** abgenommen:

- T05/T12/T13: reale ESP32-ISR-Verschachtelung, Hook-/Critical-Section-Kontexte,
  GPIO-/Clock-Vergleich, Multicore-Migration sowie GPIO/Zykluszähler-Messungen
  des Recorder-Overheads sind offen. Das mitgelieferte Arduino-Framework nutzt
  einen vorkompilierten Kernel; der Source-Build-Adapter ist nur mock-geprüft.
- Die GPU-Zeichenrate und die vollständige UI-Eingabelatenz unter Echtzeitlast
  sind noch nicht als Hardware-Abnahmemessung belegt.
- T15: die messungsabhängige Entscheidung über Kompression und weitere
  MCU-/CAN-FD-Traceprofile steht nach den Targetmessungen aus. Das implementierte
  Baselineformat bleibt unverändert; es wird kein Nutzen einer ungemessenen
  Kompression behauptet.

Siehe [Trace-Benutzung und Portgrenzen](trace.md), den normativen
[Wire-Vertrag](../lib/KymoCore/docs/TRACE_PROTOCOL.md) und
[Capture-/Replay-Semantik](../../KymoStudio/docs/TRACE_CAPTURE.md).
