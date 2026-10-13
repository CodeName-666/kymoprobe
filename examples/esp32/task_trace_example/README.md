# ESP32: experimenteller FreeRTOS-/ISR-Laufzeittest

Kandidat fuer ein neues KymoProbe-Beispiel. Erst nach erfolgreicher Pruefung
auf echter Hardware als regulaeres Beispiel uebernehmen. Ziel ist der klassische
Dual-Core-ESP32 (NodeMCU-32S), Arduino 2.0.17 / ESP-IDF 4.4, PlatformIO
`espressif32@6.12.0`. Keine Sensoren, Jumper oder WLAN-Zugangsdaten erforderlich.

## Aufbau

| Kontext | Core | Prioritaet | Verhalten |
| --- | --- | --- | --- |
| fast_worker | 1 | 3 | 200..500 us Arbeit, danach 50 ms Pause |
| slow_worker | 0 | 1 | 1500..3000 us Arbeit, danach 70 ms Pause |
| background_worker | 1 | 1 | 700..1600 us Arbeit, danach 80 ms Pause |
| pipeline | 1 | 2 | alle 150 ms ein Frame mit vier Verschachtelungsebenen |
| compute_low | 0 | 1 | alle 120 ms ca. 30 ms Rechnung in drei Schritten |
| control_high | 0 | 5 | alle 30 ms kurzer Regelzyklus, unterbricht compute_low |
| alarm_producer | 1 | 2 | alle 50 ms zwei unabhaengige Zufallsalarme |
| irq_consumer | 1 | 4 | ISR-Benachrichtigungen sammeln, 100 us Arbeit |
| timer_fast / timer_slow | 1 | ISR | zwei echte Group-0-Timer, One-Shot |
| main | 1 | Arduino | Metadaten, Diagnosemarker und UART-Transport |

Die Timer laufen mit 1 MHz (80-MHz-APB / 80). Jede Runde setzt der Producer
die Alarme auf jeweils 2..35 ms in der Zukunft. Die Zufallswerte kommen aus
`esp_random()` im Task; Zufallsqualitaet ohne Funk ist fuer diesen Lasttest
ausreichend, aber kein Versprechen kryptographischer Entropie.
Die ISRs zeichnen Eintritt/Austritt auf und benachrichtigen den Consumer.
Alle Tasks und Stacks sind statisch angelegt; SDK-Initialisierung darf Heap
benutzen. Nur `loop()` schreibt UART oder bedient den Mux.

`worker_cycle` enthaelt den verschachtelten Funktionsaufruf `busy_work`.
Der Marker `requested_us` zeigt den Sollwert. Die Schlafzeit liegt ausserhalb
der Spanne. Gemessene Zeiten sind Wandzeiten einschliesslich Praeemption,
Interrupts und Instrumentierung, keine exklusiven CPU-Zeiten. Scheduler-Hooks,
Idle-Zeiten und fremde SDK-Interrupts werden nicht erfasst. Metadaten behaupten
daher weder vollstaendige Scheduler- noch vollstaendige ISR-Abdeckung.
Bei Unterbrechung eines unbekannten SDK-/Idle-Tasks ist Kontext 0 korrekt.

### Szenarien fuer die Trace-Ansicht

- **Tiefe Verschachtelung (`pipeline`):** `process_frame` enthaelt die
  Geschwister `decode_header` (40 us), `parse_payload` und `apply_result`
  (150 us). `parse_payload` enthaelt `checksum` (kurz) und `decompress`,
  darin ein bis drei `inflate_block`. Die Laenge haengt vom Marker
  `payload_bytes` (64..512) ab; `frame_index` nummeriert die Frames.
  Im Laufzeitdiagramm erscheinen vier Ebenen untereinander, im Function flow
  der Baum eines Frames.
- **Ausreisser:** Jeder 16. Frame (`frame_index` durch 16 teilbar) rechnet seine
  `inflate_block` achtmal langsamer. `max` und `p95` liegen dann deutlich ueber
  dem Median; ueber die Mindestdauer lassen sich die normalen Frames ausblenden.
- **Praeemption auf einem Kern (`compute_low` / `control_high`):** Beide laufen
  auf Core 0. `long_compute` mit drei `matrix_step` dauert ca. 30 ms;
  `control_high` (Prioritaet 5) laeuft alle 30 ms mit `control_loop` und
  `update_control` dazwischen. Die lange Spanne laeuft also sichtbar ueber die
  Aktivitaet der anderen Spur hinweg. Da Zeiten Wandzeiten sind, enthaelt
  `matrix_step` die Unterbrechung.
- **ISR in einer Verschachtelung:** Die Timer-ISRs laufen auf Core 1 wie
  `pipeline`. Faellt ein Alarm in einen Frame, liegt das ISR-Paar zeitlich
  innerhalb der laufenden Pipeline-Spannen.
- **Beide Kerne:** `pipeline` (Core 1) und `compute_low`/`control_high`
  (Core 0) laufen gleichzeitig.

Last: geschaetzt ca. 900 Trace-Events/s (32 Byte je Event, vorher ca. 780)
bei 460800 Baud. `fast_worker` und `background_worker` laufen dafuer seltener
als vorher. `recorder_drops` zeigt an, falls der UART nicht mehr nachkommt.

## Bauen und starten

Aus dem KymoProbe-Repository:

```powershell
pio run -d examples/esp32/runtime_freertos -e esp32-runtime-test
pio run -d examples/esp32/runtime_freertos -e esp32-runtime-test -t upload --upload-port COM4
```

`COM4` durch den Port des Boards ersetzen. Vor dem Flashen die Erfassung stoppen.
Danach KymoStudio mit **460800 Baud, 8N1** verbinden und ein Runtime-Trace-Diagramm
anlegen. Den Port exklusiv verwenden: keine parallelen Textmonitore.
UART sendet ausschliesslich Binaerframes. Session und Dictionary werden alle
fuenf Sekunden erneut gesendet, damit auch eine spaetere Verbindung Namen erhaelt.
Spanne auswaehlen und Function flow oeffnen, um die Verschachtelung zu pruefen.

## Abnahme auf dem Board

1. Mindestens 10 Minuten laufen lassen: keine Resets oder Watchdog-Ausloesung.
2. Beide Kerne zeigen Worker-Spannen; `busy_work` liegt innerhalb `worker_cycle`.
   Zeiten passen zum `requested_us`-Marker zuzueglich Mess-/Unterbrechungskosten.
3. Beide Timer zeigen passende ISR_ENTER-/ISR_EXIT-Paare. Zusammen nominal
   etwa 40 Interrupts/s, mit variierenden Abstaenden. Consumer-Wartezeiten
   sind keine langen Funktionsspannen; `irq_count` darf bei Zusammenfassung >1 sein.
4. `startup_ok` und `alarm_ok` bleiben 1. Die einmal pro Sekunde gesendeten
   kumulativen Marker `recorder_errors` und `recorder_drops` bleiben 0.
   Auch Studio-Verlustzaehler pruefen. Bei Verlusten ist eine vollstaendige
   Timing-Auswertung nicht bestanden.
5. Studio-Verbindung nach >5 Sekunden neu oeffnen: Namen und Session kommen
   wieder. Board neu starten: neuer Session-Identifier.
6. Ergebnis mit Board, Port, Dauer und beobachteten Zaehlern dokumentieren.
   Erst danach Eintrag im regulaeren Beispielkatalog und ggf. Root-Demoprofil.

Die ISR-Aufrufkette von KymoCore liegt in Flash. Daher werden die Timer bewusst
ohne `ESP_INTR_FLAG_IRAM` registriert; Flash-/OTA-/Dateisystemoperationen sind
nicht Teil des Tests. Ein `IRAM_ATTR` am Callback allein macht seine komplette
Aufrufkette nicht cache-unabhaengig. Siehe
[Espressif Timer-Dokumentation](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/peripherals/timer.html).

## Anpassen

Perioden und Arbeitsdauer der Worker stehen in `src/main.cpp`, Alarmgrenzen,
Baudrate sowie Perioden von pipeline/compute/control und die Ausreisser-
Einstellungen in `include/Config.h`. Alarmmaximum unter der Producer-Periode halten, damit
ausstehende One-Shots nicht durch die naechste Runde ersetzt werden.
Task-Kontexte und Marker stehen in `kymo.project.json`; generierte Dateien
nicht von Hand aendern. Nach einer Aenderung aus dem Repository neu erzeugen:

```powershell
$env:PYTHONPATH = (Resolve-Path lib/KymoCore/tools/configurator/src).Path
python -m kymo_configurator generate examples/esp32/runtime_freertos/kymo.project.json --profile development --out examples/esp32/runtime_freertos/generated/kymo/development
```

Der eingecheckte Snapshot erlaubt Bauen ohne Installation des Configurators.
Die KymoCore-Submodule-Dateien werden fuer dieses Beispiel nicht veraendert.

## Bisherige Verifikation

Am 11.10.2026: ESP32-Target erfolgreich gebaut (52120 Byte statischer RAM,
281193 Byte Flash); `python tools/test_native.py --app ../KymoStudio` erfolgreich,
einschliesslich Trace-/Mux-Pruefungen, Stilcheck und Protokollintegration.
Diese nativen Tests pruefen KymoCore und die bestehende Studio-Anbindung,
nicht die Ausfuehrung dieses SDK-Adapters.

Hardware-Test am 11.10.2026 auf ESP32-D0WD-V3, Revision 3, COM4/460800 Baud:
600 Sekunden in der echten KymoStudio-Anwendung, eine Session, beide Kerne,
kein beobachteter Reset. `startup_ok` und `alarm_ok` waren 1,
`recorder_drops` und `recorder_errors` blieben 0. Beide Timer lieferten jeweils
11986 ISR_ENTER-/ISR_EXIT-Paare. Die nach dem Verbindungsstart begonnene
Aufnahme enthielt 197398 lueckenlose Wire-Frames, gueltige Pruefsummen und
einen Abschlussblock. Je Timer 11812 vollstaendige ISR-Paare innerhalb der
Aufnahme; Abstaende variierten ca. 17 bis 83 ms.

Beim initialen Verbindungsaufbau meldete Studio Paket-, Core- und Kontextluecken.
Daher ist die vollstaendige Abnahme noch offen; das Beispiel bleibt ein Kandidat
und ist noch nicht in den regulaeren Beispielkatalog aufgenommen.
Die normale `window_eviction`-Meldung bezeichnet begrenzte Historie.
Verschachtelte `worker_cycle`/`busy_work` wurden mit echten Aufnahmedaten
im Laufzeitdiagramm und im Function-flow-Renderer geprueft.

`kymostudio.json` startet das passende Diagramm und die COM4-Verbindung.
`tools/check_live.py` startet die echte Studio-Anwendung mit einem 10-Minuten-
Pruefbericht und laesst das Fenster anschliessend offen. Ausgabe standardmaessig
im temporaeren Verzeichnis, optional `KYMO_RUNTIME_TEST_OUTPUT` und
`KYMO_RUNTIME_TEST_SECONDS` setzen. Die Diagrammeinstellungen stehen rechts:
Mausrad scrollt Spuren, Umschalt+Mausrad verschiebt die Zeit, Strg+Mausrad zoomt;
die horizontale Scrollleiste verschiebt den sichtbaren Zeitbereich.
Doxygen-Eingaben sind ergaenzt; Doxygen ist in dieser Umgebung nicht installiert.
