# ESP32: PI-Regler mit Live-Parametern (Rückkanal)

Ein simulierter Heizungsregler, dessen Verstärkungen und Sollwert KymoStudio während
der Messung verstellt. Das Beispiel zeigt den KymoCore-Rückkanal (`KymoParam`) von
der Parametertabelle bis zur Bestätigung in Studio.

| ID | Signal | Rate |
|---|---|---|
| 0 | Temperatur (°C) der simulierten Heizung | 50 Hz |
| 1 | Heizleistung 0…1, Ausgang des PI-Reglers | 50 Hz |
| 2 | Sollwert (°C) | 50 Hz |
| 3 | Längste Bearbeitungszeit des Rückkanals in µs je 20 ms | 50 Hz |
| 100 | `kp` (Rückmeldung) | bei Änderung |
| 101 | `ki` (Rückmeldung) | bei Änderung |
| 102 | `setpoint` (Rückmeldung) | bei Änderung |

## Wie die Parameter in die Firmware kommen

```cpp
static float kp = 0.20f;
static float ki = 0.02f;
static float setpoint = 45.0f;

static const KymoParameter params[] = {
    KYMO_PARAM(100, kp,       0.0f, 10.0f, ""),
    KYMO_PARAM(101, ki,       0.0f,  2.0f, ""),
    KYMO_PARAM(102, setpoint, 20.0f, 90.0f, "°C"),
};
```

`KYMO_PARAM` nimmt den Variablennamen als Parameternamen. Mehr ist nicht nötig. Studio
fragt beim Verbinden nach der Liste und füllt sein Parameter-Panel daraus.

In `loop()` liest die Firmware ohne zu warten, was angekommen ist, und reicht es an
`KymoParam_Feed` weiter. Danach sendet sie fällige Beschreibungen
(`KymoParam_NextDescription`) und Rückmeldungen (`KymoParam_NextReadback`). Der Regler
rechnet weiter mit denselben Variablen. Ein angenommener Wert wirkt ab dem nächsten
Regelschritt.

Die Firmware hat das letzte Wort: Ein Sollwert von 95 °C liegt außerhalb von 20…90.
Das Gerät behält dann den alten Wert und meldet ihn zurück, Studio zeigt „abgelehnt“.

## Bauen und ausprobieren

Der Rückkanal kommt mit KymoCore 7.1. Bis dieser Stand getaggt ist und das Submodul
`lib/KymoCore` nachzieht, baut das Beispiel gegen den Entwicklungs-Checkout
`../KymoCore` neben KymoProbe. Deshalb ist es noch nicht in der CI.

```bash
pio run -d examples/esp32/pi_controller -t upload --upload-port COM4
```

`kymostudio.json` enthält die Quelle (COM4, 115200 Baud) und zwei Diagramme:
Temperatur mit Sollwert sowie Heizleistung mit der Rückkanal-Zeit.

Erwartung in KymoStudio:

- Nach dem Verbinden zeigt das Parameter-Panel `kp`, `ki` und `setpoint °C` mit
  Grenzen und aktuellem Wert.
- `kp` von 0.2 auf 2.0: Die Temperatur erreicht den Sollwert schneller und schwingt
  über. Das Panel zeigt „bestätigt“.
- `setpoint` auf 95: Das Gerät lehnt ab, das Panel zeigt „Gerät behält 45.0“.
- Signal 3 bleibt im Bereich einiger Mikrosekunden. Der Rückkanal bremst den Regler
  also nicht.
