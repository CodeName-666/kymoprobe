# ESP32: Stack-Reserve als Messsignal

Das Beispiel zeigt, wie viel Stack drei FreeRTOS-Tasks noch frei haben. Alle 500 ms
liest der Arduino-Loop-Task `uxTaskGetStackHighWaterMark()` jedes Tasks und sendet
das Ergebnis als normales Kymo-DATA-Signal mit Zeitstempel. Dafür braucht es kein
neues Protokoll und keinen Trace.

| ID | Signal | Stack | Verlauf |
|---|---|---|---|
| 0 | `loopTask` frei (Byte) | 8192 (Arduino-Vorgabe) | nahezu konstant |
| 1 | `sensor` frei (Byte) | 2048 | konstant, leichte Last |
| 2 | `parser` frei (Byte) | 4096 | fällt in Stufen, dann flach bei ≈ 1 KiB |
| 3 | `parser` Rekursionstiefe | – | steigt 1 → 24, eine Stufe pro Sekunde |

Der Parser-Task vertieft seine Rekursion jede Sekunde um eine Ebene (112 Byte pro
Ebene) bis 24 Ebenen. Signal 2 fällt deshalb im Gleichschritt mit Signal 3 und
bleibt dann stehen.

Gemessen wird der **kleinste freie Stack seit Taskstart** (High-Water-Mark). Der Wert
steigt nie wieder, auch wenn der Task später weniger Stack braucht. Er zeigt also
die bisher knappste Stelle.

- **Einheit:** Unter ESP-IDF und Arduino-ESP32 liefert die Funktion Byte. Vanilla-FreeRTOS
  (z. B. STM32) liefert Wörter, dort mit `sizeof(StackType_t)` multiplizieren.
- **Kosten:** Die Funktion durchsucht den ungenutzten Teil des Stacks. Deshalb wird
  nur mit 2 Hz im niedrig priorisierten Loop-Task gemessen, nie in einer ISR oder
  einem zeitkritischen Pfad.
- **Abgrenzung:** `high_water` im HEALTH-Frame von KymoCore ist die Belegung des
  Recorder-Ringpuffers, nicht der Stack.

UART: 115200 Baud, 8N1, binäres Kymo-DATA mit Zeitstempel und CRC.

```bash
pio run -d examples/esp32/stack_reserve -t upload --upload-port COM4
```

`kymostudio.json` enthält die passende Quelle (COM4, 115200 Baud) und ein
Zeitreihen-Diagramm mit allen vier Signalen.

Erwartung in KymoStudio:

- Signal 2 fällt in den ersten 24 s in 24 Stufen um je ≈ 112 Byte und bleibt dann
  stehen. Jede Stufe fällt mit einer Stufe von Signal 3 zusammen.
- Die Signale 0 und 1 bleiben nach dem Start konstant.
- Kein Neustart des Boards: Ein Stack-Überlauf würde einen Panic auslösen, und die
  Kurven würden neu beginnen.
- Für einen Grenzwert-Alarm in der Analyse ein Toleranzband z. B. bei 512 Byte setzen.
