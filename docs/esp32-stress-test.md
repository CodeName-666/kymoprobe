# ESP32-Belastungstest fuer KymoStudio

Die Root-Firmware besitzt drei zusaetzliche PlatformIO-Profile. Die bisherigen
USB- und TCP/MQTT-Demos bleiben verfuegbar. Der Stresstest sendet standardmaessig
alle 256 Kanal-IDs (0..255), gleichmaessig im Round-Robin-Verfahren und so schnell,
wie der Transport Bytes annimmt. Die kuenstlichen Rampen liegen in [-1, 1]; ihre
Periodendauer haengt von der erreichten Datenrate ab. Es werden keine Sensoren
benoetigt. Kein Stresstest-Profil publiziert MQTT-Nachrichten.

| Profil | Transport | Daten je Frame | Framegroesse |
| --- | --- | --- | --- |
| `nodemcu-32s-stress-usb` | USB/UART, 2.000.000 Baud, 8N1 | Y + Zeitstempel | 13 Byte |
| `nodemcu-32s-stress-tcp` | WLAN/TCP, Port 2323 | Y + Zeitstempel | 13 Byte |
| `nodemcu-32s-stress-tcp-xyz` | WLAN/TCP, Port 2323 | X/Y/Z + Zeitstempel | 21 Byte |

Die Anwendung buendelt vollstaendige Binaerframes in einem festen 4-KiB-Puffer.
Teilweise angenommene Uebertragungen werden fortgesetzt. Ein voller Puffer haelt
das Sampling an; es gibt keine nachtraeglichen Nachhol-Bursts. TCP-Neuverbindungen
starten an einer Framegrenze mit neuer relativer Zeitbasis. Die Loop verarbeitet
hoechstens 512 Core-Schritte und gibt danach Rechenzeit an das SDK ab. Die
Arduino-Treiber verwenden intern Heap; synchrone TCP-Writes koennen blockieren.

## Start

1. In KymoStudio die bisherige Erfassung stoppen, damit insbesondere COM4 frei ist.
2. Aus dem KymoProbe-Verzeichnis ein Profil bauen und flashen, beispielsweise:
   `pio run -e nodemcu-32s-stress-usb -t upload --upload-port COM4`.
   Der Upload bleibt bei der bisherigen konservativen Geschwindigkeit; 2 Mbaud
   betrifft ausschliesslich den anschliessenden Datenstrom.
3. USB: in KymoStudio `start_esp32_stress_usb.bat` starten und die gespeicherte
   Verbindung aktivieren. COM4 und Baudrate muessen zur Firmware passen.
4. TCP: vorhandene `include/network_secrets.h` verwenden; bei Bedarf gemaess
   [Netzwerk-Demo](esp32-network-demo.md) anlegen. Eines der TCP-Profile flashen,
   im USB-Monitor bei 115200 Baud die ausgegebene `KYMO_STRESS_IP` ablesen.
   In KymoStudio die Board-IP in `config/esp32-stress-tcp.json` aktualisieren
   und `start_esp32_stress_tcp.bat` starten. USB liefert dabei nur Startdiagnosen.

Beide Studio-Konfigurationen enthalten 256 benannte Signale und vier Diagramme
mit je 64 Kurven. Die Erfassung startet durch Aktivieren der gespeicherten
Verbindung. Fuer den XYZ-Transport kann dieselbe TCP-Konfiguration benutzt werden;
X und Z werden mit erfasst, weitere XY/XYZ-Diagramme lassen sich im Studio zuweisen.

Konfigurationen neu erzeugen (aus dem KymoStudio-Verzeichnis):

```
.venv\Scripts\python.exe tools\prepare_esp32_stress.py tcp --host 192.168.178.92
.venv\Scripts\python.exe tools\prepare_esp32_stress.py usb --port COM4 --baud 2000000
```

`--channels 64` passt die Studio-Metadaten an eine Firmware mit 64 Kanaelen an.
`--visible-channels 0` erfasst weiterhin alle Kanaele und isoliert den Durchsatz
von der Kurvendarstellung. Der Generator ersetzt ausschliesslich die angegebene
Stresstest-Konfiguration; vorherige manuelle Anpassungen dort gehen dabei verloren.

## Last einstellen

In `platformio.ini` die Werte im USB-Basisprofil aendern und neu flashen; die
TCP-Profile erben Kanalanzahl und Intervall:

- `KYMO_STRESS_CHANNEL_COUNT`: 1..256.
- `KYMO_STRESS_PERIOD_MS`: 0 = maximale Rate, 1 = angefordert bis zu 1000 Hz je
  Kanal, 10 = bis zu 100 Hz je Kanal. Transportstau senkt die tatsaechliche Rate.
- `KYMO_STRESS_BAUD`: USB-Baudrate; auch Studio-Konfiguration anpassen.
- `KYMO_STRESS_XYZ_ENABLE=1`: groesster Frame, 21 statt 13 Byte. Die Profile
  kompilieren die dafuer notwendigen Core-Features bereits ein.

Fuer eine systematische Messung mit 16, 64, 128 und 256 Kanaelen sowie 100, 1000
Hz und unbegrenzter Rate arbeiten. Jeweils mit 0, 64 und allen sichtbaren Kurven
vergleichen. In Studio die Empfangsrate und Verlustzaehler beobachten:
`ingressDropped` = Empfangsqueue ueberlastet, `displayDropped` = Anzeigepuffer
ueberlastet, `signalLimitDropped` = zu viele Signale, `invalid` = ungueltige
Nutzdaten. `evicted` zaehlt das erwartete Entfernen alter Rohdaten beim Erreichen
des Speicherlimits. Niedrige Empfangsrate ohne Studio-Verluste kann durch die
Firmware oder den Transport entstehen. Sie ist kein Nachweis der Studio-Maximalrate.
Zeitenstempel haben Millisekunden-Aufloesung; gleiche Zeitstempel bei maximaler
Rate sind erlaubt. Zeitstempel allein sind kein verlustfreier Sequenzzaehler.

## Aktuelle Grenzen des Arbeitsstands (10.10.2026)

| Bereich | Grenze | Quelle im Studio/Probe |
| --- | --- | --- |
| Binaere Kanal-ID | 0..255, 256 IDs pro Verbindung | `Receiver/binary_protocol.py`, Core-Protokoll |
| Studio-Signale | **256 insgesamt**, ueber alle Verbindungen | `Backend/backend.py`, `Core/samples.py` |
| Gespeicherter Workspace | 16 Diagramme, 64 Kurven je Diagramm, 1024 Zuordnungen insgesamt | `Core/workspace.py` |
| Angezeigte Historie | 500..50.000 Punkte je Signal | `Core/configuration.py` |
| Rohdatenhistorie | 1000..10.000.000 Samples je Signal, 1000..50.000.000 insgesamt einstellbar | `Core/configuration.py` |
| Empfangsqueue | 32.768 Samples / 16 MiB pro Verbindung, max. 64 KiB pro Eintrag | `Receiver/receiver.py`, `Core/ingress.py` |
| Anzeigepuffer | 4096 Punkte je Signal vor der Uebergabe | `Backend/backend.py` |
| Darstellungstakt | 16..500 ms, normalerweise 33 ms | `Core/configuration.py` |
| Sampling im Core | 1 ms = angefordert 1000 Hz je Kanal; 0 = keine feste Begrenzung | `kymo_runtime.cpp` |
| Binaerframe | 9 Byte Y, 13 Byte Y+Zeit, 21 Byte XYZ+Zeit | Core und Studio-Protokoll |

Die Historien-Obergrenzen sind erlaubte Konfigurationswerte, keine Zusage, dass
50 Millionen Python-Samples auf dem vorhandenen PC in RAM passen. Die vorbereiteten
Stresstest-Konfigurationen behalten 1 Million Rohsamples insgesamt, maximal
20.000 je Signal und 5000 Anzeigepunkte je Signal; Downsampling ist ausgeschaltet.
Bei 256 gleichmaessigen Kanaelen greift das Gesamtlimit bereits bei etwa 3906
Rohsamples je Kanal. Hoehere Historienwerte separat in `performance` einstellen.

Es gibt **keine fest programmierte maximale Empfangsrate** in Samples/s.
Die reale Grenze muss mit PC, Transport und sichtbaren Kurven gemessen werden.
Ein XYZ-Frame enthaelt drei Floatwerte, zaehlt im Studio aber als ein Sample.
Der Downsampling-Regler (bis 100.000 Hz) betrifft die Anzeige, nicht den Eingang.

Theoretische UART-Obergrenzen bei 8N1 (10 Bit je Byte), vor sonstigen Pausen:

| Baud | Y+Zeit, 13 Byte: Samples/s gesamt | Bei 256 Kanaelen: Hz je Kanal |
| --- | ---: | ---: |
| 115.200 | 886 | 3,46 |
| 921.600 | 7089 | 27,69 |
| 2.000.000 | 15.384 | 60,10 |

XYZ+Zeit bei 2 Mbaud: etwa 9523 Samples/s gesamt bzw. 37,20 Hz je Kanal.
256 Kanaele mit je 1000 Hz benoetigen bereits 3,328 MB/s Nutzdaten bei 13-Byte-
Frames (UART: 33,28 Mbaud) bzw. 5,376 MB/s bei 21-Byte-Frames. TCP vermeidet die
UART-Grenze, erreicht diese Zielrate aber nicht automatisch. 2 Mbaud muss vom
USB-UART-Adapter und Treiber unterstuetzt werden; bei Problemen auf 921600
zurueckgehen und Firmware sowie Studio gemeinsam anpassen. Native USB-CDC ist
nicht durch diese UART-Rechnung begrenzt.

## Verifikation

`python tools/test_native.py --app ../KymoStudio` prueft den Core und die
Desktop-Protokollkompatibilitaet. `python tools/test_stress_demo.py` prueft die
tatsaechliche USB-Stressfirmware am Host mit simuliertem TX-Stau und Teilwrites,
einschliesslich aller 256 IDs, Werte, Zeitstempel und der XYZ-Variante.
Die PlatformIO-Builds pruefen zusaetzlich die reale ESP32-SDK-Anbindung. Ein
Hardware-Durchsatztest ist separat erforderlich; aus Build- und Hosttests
folgt keine gemessene Maximalrate.
