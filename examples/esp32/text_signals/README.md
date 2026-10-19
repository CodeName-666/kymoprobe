# ESP32: Textdaten ohne Zeitstempel (Prüffall P2)

Testfirmware für KymoStudios Pfad ohne Gerätezeit. Alle 10 ms sendet das Board
zwei JSON-Zeilen ohne Zeitstempel:

```text
{"id":0,"value":0.30902}
{"id":1,"value":-0.84000}
```

| ID | Signal | Frequenz | Werte | Rate |
|---|---|---|---|---|
| 0 | Sinus | 1 Hz | −1 bis +1 | 100 Hz |
| 1 | Sägezahn | 0,5 Hz | −1 bis +0,99 | 100 Hz |

UART: 115200 Baud, 8N1, Text. Kein KymoCore-Framing und keine CRC.

```bash
pio run -d examples/esp32/text_signals -t upload
```

`kymostudio.json` enthält die passende Quelle (COM4) und ein Zeitreihen-Diagramm.

Erwartung in KymoStudio:

- Zeitbasis „Empfangszeit“ im Inspector und in der CSV-Spalte `time_basis`.
- Werte innerhalb eines Empfangsblocks können dieselbe Empfangszeit haben.
- Zeitabhängige Analysen (Frequenz, Spektrum, Ableitung, Trigger) sind mit
  Begründung gesperrt. Wertestatistik bleibt verfügbar.
- Mit der Abtastrate 100 Hz unter „Zeitbasis“ wird die Zeit rekonstruiert.
