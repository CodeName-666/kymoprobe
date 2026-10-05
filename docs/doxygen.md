# API-Dokumentation erzeugen

Im Projektverzeichnis mit installiertem Doxygen ausführen:

```sh
doxygen Doxyfile
```

Die Startseite liegt anschließend unter `docs/api/html/index.html`.
Die Konfiguration wurde mit Doxygen 1.18.0 geprüft. Graphviz und LaTeX werden
nicht benötigt. Generierte HTML- und XML-Dateien sind von Git ausgeschlossen.

Erfasst werden alle zwölf aktiven eigenen Header: C-Runtime, Protokoll,
gemeinsame Bit-/Byte-/CRC-Helfer, optionale C++-Schnittstellen sowie
Anwendungskonfiguration und Beispieladapter. Archivierter Code unter `legacy/`
und die unabhängige Fremdbibliothek `lib/Events` gehören nicht zu dieser API.

Die Header gliedern Konstanten, Typen, Konfiguration, Zustand und Funktionen
in benannte Bereiche. Jeder API-Eintrag hat eine Kurzbeschreibung und einen
Usage-Abschnitt. Funktionsparameter tragen `@param[in]`, `@param[out]` oder
`@param[in,out]`; Rückgabewerte werden mit `@return` beschrieben. Bei Feldern
und Defines stehen Bedeutung, Wertebereiche und Verwendung direkt am Eintrag.
Parameter-Tags werden nur dort verwendet, wo tatsächlich Parameter existieren.

Doxygen erfasst auch private Mitglieder und statische Beispielhelfer.
Fehlende Dokumentation, unvollständige Parameterangaben und fehlerhafte
Verweise führen zu einem fehlgeschlagenen Dokumentationslauf; Meldungen stehen
in `docs/api/warnings.log`. Die Dokumentations-Präprozessorwerte blenden den
optionalen UART-Adapter und die mehrdimensionale Beispielkonfiguration ein.
Sie verändern weder Firmware-Builds noch das Protokoll.

Dokumentation ersetzt keine Compiler- oder Funktionstests. Zur Prüfung:

```sh
python tools/test_native.py --app ../KymoStudio
```
