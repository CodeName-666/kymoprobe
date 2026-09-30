# Embedded Plotter Implementation Plan

Goal: Universelle, statische Init/Main-Telemetrie mit unverÃ¤ndertem App-Protokoll.
Spec: [embedded-design.md](embedded-design.md).
Tech Stack: C99, C++11, PlatformIO, Python/pytest.

## Anforderungen

Kein Heap im Kern, 21-Byte-TX-Puffer, bytebasierte Flags/Status/Offsets,
explizites Little Endian, v6 Wire-Version 1, 1â€“256 KanÃ¤le, 0â€“INT32_MAX ms.
Bestehende nicht versionierte Dateien sind Ausgangsstand und werden erhalten
oder gezielt migriert; keine Commits fremder Ã„nderungen.

## Schritte

- [x] C-Verhaltenstests in `test/native/runtime_test.c`: Init-Validierung,
  feste Golden Bytes, kurze Writes/Busy, Fairness, ZeitÃ¼berlauf, Samplefehler,
  unabhÃ¤ngige Instanzen; zuerst gegen fehlende Runtime ausfÃ¼hren.
- [x] `lib/PlotterLib/src/plotter_runtime.h/.c`: Konfiguration und Main-State.
  C99 mit Warnungen als Fehler kompilieren; Verhaltenstests ausfÃ¼hren.
- [x] C++-Push-API und STM32-Adapter auf RÃ¼ckstau/Pufferlebensdauer prÃ¼fen,
  Regressionstests ergÃ¤nzen und Golden-Test weiter bestehen lassen.
- [x] `src/` und `include/` auf neue API migrieren. Beispielkonfigurationen,
  STM32-HAL-Initialisierung, USB-Projekt, portable Manifestangaben ergÃ¤nzen.
- [x] Nativen C-Sender aus der Runtime durch echten App-Streamdecoder und
  Parser prÃ¼fen; alle optionalen Felder und FragmentgrÃ¶ÃŸen abdecken.
- [x] PlatformIO-Builds AVR/ESP32/STM32 und App-Gesamttests durchfÃ¼hren.
  Vorbestehende TestportabilitÃ¤tsprobleme separat nachvollziehbar korrigieren.
- [x] Dokumentation/API-Anleitung, CI, reproduzierbare PrÃ¼fkommandos und
  tatsÃ¤chliche Build-/Speicherergebnisse festhalten; abschlieÃŸenden Diff prÃ¼fen.

## Review-Fokus

- Nach Busy oder kurzem Write keine Ã¼berschriebenen oder doppelten Bytes.
- Fehlerhafte Konfiguration deaktiviert die Instanz sicher.
- Kein schneller Kanal verdrÃ¤ngt langsamere KanÃ¤le.
- Async-CDC darf denselben Puffer erst nach Completion wiederverwenden.
- UnabhÃ¤ngigkeit von Arduino, C++-Runtime, Host-Endianness und Enum-Breite.

Die Detailergebnisse und nicht geprüften optionalen Plattformen stehen in [verification.md](verification.md). Priorität laut Nutzer: ESP32 und Arduino.
