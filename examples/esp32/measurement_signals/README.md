# ESP32: Sinus und Sägezahn für Live-Messungen

Dieses Beispiel ersetzt auf dem Board die Trace-Firmware durch normale numerische
Messwerte. Das bisherige Task-/ISR-/Flow-Beispiel ist vollständig als
[`../task_trace_example`](../task_trace_example) gesichert; sein
`saved_example.json` enthält die beim Sichern vom ESP32 gelesene Identität sowie
Hashes der gesicherten Quell- und Konfigurationsdateien.

| ID | Signal | Frequenz | Werte | Abtastrate |
|---|---|---|---|---|
| 0 | Sinus | 6,25 Hz | −1 bis +1; RMS ≈ 0,707107 | 200 Hz |
| 1 | Sägezahn | 1,5625 Hz | −1 bis +0,984375; Mittelwert −0,0078125 | 200 Hz |

Beide Signale haben denselben Zeitstempel. Die 5-ms-Zeitstempel bezeichnen das
logische Raster des synthetischen Generators. Das Beispiel misst keine Sensoren
oder ADC-Abtastzeitpunkte. Verzögerungen werden durch Überspringen ganzer
Rasterpunkte sichtbar, statt Messungen rückwirkend zu erfinden. UART: 460800,
8N1, binäres Kymo-v6-DATA mit Zeitstempel und CRC. Keine Textausgabe und keine
FreeRTOS-Task-/ISR-Instrumentierung; Anwendung im Arduino-Mainloop.

## Bauen, aufspielen und ausprobieren

Vom KymoProbe-Repository aus (COM-Port bei Bedarf anpassen):

```sh
pio run -d examples/esp32/measurement_signals
pio run -d examples/esp32/measurement_signals -t upload --upload-port COM4
```

Falls der automatische Bootloader-Wechsel scheitert, BOOT beim
Verbindungsversuch gedrückt halten, EN/RESET kurz drücken und BOOT erst nach
erkannter Verbindung loslassen. Auf dem geprüften Board war ein manueller
Bootloader-Wechsel erforderlich; das Aufspielen gelang mit 115200 Baud und
ohne Flasher-Stub. Nach dem Upload läuft die Datenverbindung mit 460800 Baud.

Den seriellen Monitor geschlossen halten. KymoStudio mit 460800 Baud verbinden,
beide Signale einem Zeitdiagramm zuordnen und **Analyse** öffnen.

Mit dem aktuellen KymoStudio-Quellcode im Geschwisterverzeichnis geht dies direkt:

```sh
../KymoStudio/.venv/Scripts/python.exe examples/esp32/measurement_signals/tools/start_studio.py
```

Das öffnet einen separaten Demo-Workspace samt Live-Analyse und verbindet COM4.
Die Konfiguration wird in ein temporäres Sitzungsverzeichnis kopiert.
In `kymostudio.json` kann vor dem Start ein anderer COM-Port eingetragen werden.
Die normale Studio-Konfiguration bleibt erhalten.

Der Hardwarecheck benötigt exklusiven Zugriff auf den Port, also vor dem Öffnen
von Studio ausführen:

```sh
../KymoStudio/.venv/Scripts/python.exe examples/esp32/measurement_signals/tools/check_live.py --port COM4 --seconds 15 --report .pio/measurement-live.json
```

- **Live / Cursor:** Sinusperiode 0,16 s, Sägezahnperiode 0,64 s. Für RMS/Integral
  ganze Perioden auswählen; Anfang/Ende werden interpoliert.
- **FFT:** 256 oder 1024 Punkte, Rechteckfenster. Beim Sinus liegt ein Bin exakt
  bei 6,25 Hz, Spitzenamplitude ≈ 1. Hann ebenfalls ausprobieren. Der Sägezahn
  zeigt die Grundfrequenz und Oberwellen; seine Sprungflanke erzeugt Aliasing.
- **Filter:** Tiefpass und gleitenden Mittelwert am Sägezahn vergleichen. Filter
  sind kausal; Einschwingzeit und Verzögerung berücksichtigen.
- **Trigger:** Sinus, Schwelle 0, steigend, 0,2 s Vorlauf / 0,5 s Nachlauf.
- **Grenzen:** Sinus mit −0,5/+0,5 begrenzen, Verletzungsdauer/-anzahl ansehen.
- **Referenz:** Eine Periode merken, zeitlich um 0,16 s verschieben und vergleichen.
- **Rechensignale/Dashboard:** Zeitzuordnung für A/B bestätigen, Differenz oder
  Produkt der beiden Kanäle bilden und Kennzahlen anheften.

Die Frequenzen liegen bei 200 Hz Abtastrate auf FFT-Bins aller Größen ab 256;
so lässt sich eine falsche Frequenzachse von normaler spektraler Leckage unterscheiden.
Bei tatsächlich fehlenden Rasterpunkten darf die FFT diese nicht still reparieren.
Signalwerte sind dimensionslos, in Studio deshalb keine physikalische Einheit
behaupten. Die Vorschau ist reduziert; Kennzahlen nutzen die Rohwerte.

## Task-/Trace-Beispiel wiederherstellen

```sh
pio run -d examples/esp32/task_trace_example -e esp32-runtime-test -t upload --upload-port COM4
```

Danach Studio wieder mit 460800 Baud verbinden und Runtime-/Function-Flow-
Diagramme verwenden. Das gesicherte Beispiel behält seine dokumentierten Grenzen,
einschließlich der noch offenen Bewertung initialer Trace-Lücken.

## Geprüfter Stand vom 13. Oktober 2026

- Beide Beispiele für NodeMCU-32S erfolgreich gebaut; der gesicherte Task-Snapshot
  stimmt auch als Git-Dateiinhalt mit allen 24 dokumentierten Prüfsummen überein.
- Native KymoCore-/Studio-Protokollprüfungen und C/C++-Strukturprüfung bestanden.
- 2048 Frames des tatsächlichen C++-Generators/Codecs von Studio dekodiert:
  Sinus-FFT-Peak 6,25 Hz / 0,99999999, Sägezahnfrequenz 1,5625 Hz.
- Neue Firmware auf ESP32-D0WD-V3, Revision 3, COM4 aufgespielt; alle vier
  Flash-Blöcke vom Uploader per Hash geprüft.
- Echte KymoStudio-Oberfläche empfängt die beiden numerischen Signale mit etwa
  400 Werten/s; die Live-Analyse berechnet Bereichskennzahlen und Frequenz.

Der separate serielle Rohdatencheck konnte während der bereits gestarteten
Studio-Verbindung nicht exklusiv auf COM4 zugreifen. Eine vollständige Aussage
über Zeitstempel-Lücken im Hardware-Dauerlauf ist damit noch nicht belegt.
Für diesen Check die Studio-Erfassung vorher stoppen. Doxygen ist lokal nicht
installiert; die neuen Header sind in dessen Eingaben aufgenommen.
