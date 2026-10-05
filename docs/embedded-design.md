# Embedded Kymotrace: Analyse und Zielarchitektur

> Nachtrag 2026-10-01: KymoCore wird inzwischen vollst�ndig als C++11
> kompiliert. Die C-kompatible API bleibt f�r C99-Aufrufer erhalten.
> Die folgenden C99-Kern-Angaben beschreiben den urspr�nglichen Plan.

Stand: 2026-09-30. Geprüft: KymoProbe und das benachbarte KymoStudio.

## Befund

- Das Hauptprojekt verwendet einen alten A5A5/PING/PONG-Entwurf mit direkt
  übertragenem Enum-/Struct-Layout und Debugtext auf dem Datenkanal. Es passt
  nicht zum A55A-Binärprotokoll der App. Zusätzlich: leeres DATA-Handling,
  fehlender Rückgabewert und ein Semikolon hinter einer Erfolgsprüfung.
- KymoCore hat bereits einen brauchbaren C-Codec: explizites Little Endian,
  CRC8, endliche float32-Werte, 9–21 Bytes pro Messung, kein Heap.
- Der C++-Sender ignoriert kurze Writes; der CDC-Adapter gibt einen Puffer
  wieder frei, bevor USB die asynchrone Übertragung abgeschlossen hat.
  Die HAL-Funktionsdeklarationen sind nicht typ-/linkagesicher.
- Die STM32-Projekte referenzieren nicht vorhandene CubeMX-Header und enthalten
  leere Initialisierungen. Alte C-Kopien referenzieren eine nicht vorhandene API.
- Die Beispiele duplizieren Scheduler und Quelltext. Das Manifest schränkt
  Frameworks/Plattformen unnötig ein und setzt einen C++-Flag auch für C.
- Events ist ein eigenständiges, derzeit ungenutztes Submodul. Es ist keine
  notwendige Abhängigkeit des Protokolls und bleibt unverändert.
- KymoStudio empfängt v6 über Serial, TCP, MQTT und CAN-FD. Classic CAN hat
  eine separate native Wertzuordnung; ein v6-Frame passt nicht in acht Bytes.
  Der Python-Codec und Core/parsing.py sind die Integrationsgrenze zum Backend.

## Entscheidung

Ein C99-Kern mit `Kymo_Init(context, config)` und `Kymo_Main(context)`.
Eine statische `kymo_config.c/.cpp` liefert die Konfiguration; eine ECU
benötigt weder Dateisystem noch JSON-Parser. Kein neues Wire-Format.
Ein reiner C++-Umbau würde C/HAL-Verbraucher ausschließen; rohe gepackte
Structs würden Alignment, Endianness und Pointergrößen an den Draht koppeln.

Konfiguration: unveränderliche Kanaltabelle (ID, Flags, Periode), vom Aufrufer
bereitgestellte uint32-Zeitstempel pro Kanal, Zeit-/Mess-/Write-Callbacks mit
eigenen user-Pointern und optional ein Transport-Service-/Busy-Callback.
Die Konfiguration und Speicherbereiche müssen die Instanz überleben.
Kanäle haben eindeutige IDs 0–255; die Anzahl ist deshalb uint16 (1–256).
Perioden sind 0–INT32_MAX ms; 0 bedeutet einmal pro Aufruf bei freiem Transport.

Der Kontext hält genau einen 21-Byte-TX-Puffer und Byte-Längen/Offsets.
Main bedient zunächst den Transport und prüft dessen Busy-Zustand. Ein
teilweise geschriebener Frame wird unverändert fortgesetzt. Asynchrone
Treiber melden Busy bis zur Fertigstellung, damit der Puffer gültig bleibt.
Pro Main höchstens ein Mess- und ein Write-Aufruf; eine zyklische Suche über
maximal die konfigurierte Kanalzahl sorgt für Fairness. Bei Rückstau wird
nicht unbegrenzt gepuffert oder nachträglich aufgeholt. Überfällige Kanäle
liefern nach Freigabe einen aktuellen Messwert. Rückgabestatus macht Idle,
Busy, ungültige Messwerte und Treiberfehler sichtbar.

Zeitdifferenzen sind uint32 und damit rolloverfest, solange Main regelmäßig
(mindestens einmal innerhalb INT32_MAX ms) aufgerufen wird. Zeitstempel
bleiben die v6-Millisekunden relativ zu Init. Keine Reentranz/ISR-Aufrufe;
Messwertübergabe aus Interrupts synchronisiert der Anwender. Callbacks müssen
kurz und begrenzt sein; der Kern kann blockierende Treiber nicht verhindern.

Der bestehende C++-Sender bleibt als optionale Push-API erhalten. Er bekommt
eine explizite Flush-/Busy-/Ergebnisbehandlung, damit kurze Writes keine
Frames vermischen. Für neue Anwendungen wird die C-API empfohlen.

## Plattformen und Nachweis

- Hauptprojekt und Arduino-/ESP32-Beispiele nutzen die zyklische API.
- STM32 UART: vollständiges STM32Cube-C-Beispiel für F401/F411/F103.
- STM32 USB: eigenständig baubares STM32duino-CDC-Beispiel für Blue Pill;
  CubeMX-CDC/DMA wird als Integrationsmuster mit Busy-Lebensdauer dokumentiert.
- Native: echte C99/C++-Tests, keine Arduino-Mocks im Kern.
- Gemeinsame Tests für alle acht Flagkombinationen, CRC, Fragmentierung,
  Teilschreiben, Busy, Fairness, fehlerhafte Konfiguration, ungültige Messwerte,
  Rollover und voneinander unabhängige Instanzen.
- Ein nativer C-Sender erzeugt Bytes, die der echte KymoStudio-Decoder und
  Parser lesen. PlatformIO kompiliert repräsentativ AVR, ESP32 und STM32.
- Builds beweisen keine elektrische Hardwarefunktion; Flashen und Messungen
  auf angeschlossenen Boards bleiben gesonderte Hardwareprüfungen.

## Ergänzung: ein Rückgabepunkt und Common-Komponente

Auf Nutzerwunsch gilt für alle aktiven C/C++-Funktionen maximal ein Return am
Ende. Codec und Runtime zerlegen Validierung, Abtastung und Übertragung in
kleine Funktionen mit initialisierten Ergebnisvariablen. Ein struktureller
Quelltextcheck sichert die Regel für diesen Sprachumfang ab.

Wiederverwendbare Bitmasken-/Bitset-Helfer, uint32/float32-Little-Endian-Zugriffe
und CRC8 liegen in `src/common` innerhalb des Library-Pakets. Makros dienen
konstanten Maskenkombinationen; Laufzeitoperationen sind typisierte Inline-
Funktionen. Die Common-Header hängen nicht vom Kymotrace-Protokoll ab.
Decoder-Ergebnisse werden nur nach vollständiger Validierung übernommen.
Das Wire-Format und die Init/Main-Schnittstelle bleiben unverändert.

## Ergänzung: vollständige Plattformtrennung

Die gesamte Library einschließlich C++-Wrapper ist frei von Hersteller- und
Framework-Abhängigkeiten. STM32-/Arduino-Adapter liegen ausschließlich bei den
Beispielen. Zielplattform-Defines ändern weder API noch Speicherlayout der
Library. Hardwarezugriffe erfolgen ausschließlich über konfigurierte Callbacks
oder das abstrakte KymoStream-Interface.
