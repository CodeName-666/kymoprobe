# KymoProbe

Universelle Embedded-Telemetrie für KymoStudio: C++11-Kern, statische Konfiguration,
einmal `Kymo_Init()` und zyklisch `Kymo_Main()`. Das Hauptprojekt ist ein
ESP32-PlatformIO-Projekt. Der Kern benötigt weder Arduino noch RTOS oder Heap.

## Klonen

Die Library KymoCore ist ein eigenes Repository
([CodeName-666/kymocore](https://github.com/CodeName-666/kymocore)) und hier als
Git-Submodul unter `lib/KymoCore` eingebunden. Deshalb mit Submodulen klonen:

```sh
git clone --recurse-submodules https://github.com/CodeName-666/kymoprobe.git
```

In einem bestehenden Checkout reicht `git submodule update --init`.
Library-Änderungen werden in kymocore committet, getaggt und gepusht. Danach
in diesem Repository den Submodul-Stand auf den neuen Tag setzen:

```sh
git -C lib/KymoCore fetch --tags
git -C lib/KymoCore checkout v6.1.0   # gewünschte Version
git add lib/KymoCore && git commit -m "KymoCore auf v6.1.0 aktualisieren"
```

## Start mit ESP32

```sh
pio run -e nodemcu-32s
pio run -e nodemcu-32s -t upload
```

`src/main.cpp` initialisiert Serial mit **115200 Baud** und ruft Init/Main auf.
`src/kymo_config.cpp` enthält Kanäle, Abtastintervalle, Messwertfunktion und
Transport. Im Beispiel senden Kanal 0 und 1 Sinus/Sägezahn mit je 50 Hz und
Zeitstempeln. Eigene Messwerte werden im `sample`-Callback übernommen.

In KymoStudio eine serielle Verbindung zum Board mit **115200, 8N1** öffnen.
Die App erkennt die Binärframes automatisch. Die Datenbytes sind kein Text;
den Port nicht gleichzeitig mit dem PlatformIO-Serial-Monitor öffnen.
Kanalnamen/Einheiten werden in der App eingestellt, nicht über v6 übertragen.

## Arduino

```sh
pio run -d examples/arduino/simple_analog_example -e uno
pio run -d examples/arduino/simple_analog_example -e uno -t upload
pio run -d examples/arduino/multi_sensor_example -e uno
```

Die Beispiele erzeugen reproduzierbare Testsignale. Im gemeinsamen
`examples/common/arduino_serial_config.h` kann `example_sample` durch
`analogRead()` oder eigene Sensorwerte ersetzt werden. Das zweite Beispiel
zeigt Y, X/Y und X/Y/Z mit unabhängigen Abtastraten. Arduino Uno/Nano/Mega
und ESP32-Projekte nutzen denselben C++11-Kern.

## Library und Schnittstellen

Die vollständige Header-API lässt sich mit `doxygen Doxyfile` erzeugen.
[Doxygen-Anleitung](docs/doxygen.md) beschreibt Ausgabe und Dokumentationsregeln.

[KymoCore](lib/KymoCore/README.md) (eigenes Repository, Submodul) dokumentiert die Konfiguration, Speicher-
und Callback-Verträge. [PROTOCOL.md](lib/KymoCore/PROTOCOL.md) definiert das
mit KymoStudio identische Wire-Format. Es sind 9–21 Bytes pro Messung:
bytebasierte IDs/Flags, explizites Little Endian, float32, optionale uint32-
Millisekunden und ein Abschlussbyte. CRC8 ist standardmäßig deaktiviert;
`NO_CRC` im Descriptor kennzeichnet Frames mit Null-Platzhalter statt CRC.
Mit `build_flags = -DKYMO_ENABLE_CRC=1` lässt sich CRC für den Bibliotheksbuild
einschalten. Für den Standardmodus ist die aktualisierte KymoStudio erforderlich.
Keine Pointer oder C-Strukturen werden direkt gesendet.

Der Standardbuild enthält nur den skalaren C-Sender und die Init/Main-Runtime.
X, Z, Zeitstempel, CRC, MCU-Decoder und C++-Wrapper werden über die
`KYMO_ENABLE_*`-Schalter in
[`kymo_build_config.h`](lib/KymoCore/src/kymo_build_config.h) zugeschaltet:
direkt im Header `0` für aus oder `1` für ein setzen und vollständig neu bauen.
Vorhandene Build-Flags haben Vorrang vor den Headerwerten. Auch die Runtime
lässt sich für reine Codec-Nutzung abschalten. Die vollständige Schaltertabelle
steht in [KymoCore/README.md](lib/KymoCore/README.md#minimal-build-and-optional-features).
Die Beispielprojekte aktivieren ihre benötigten Extras ausdrücklich.

Für andere ECUs werden ausschließlich Zeit-, Mess- und Transport-Callbacks
angepasst. UART, USB CDC, TCP oder MQTT brauchen keinen anderen Codec.
Asynchrone Treiber melden Busy, bis der übergebene Puffer nicht mehr verwendet
wird. Kurze Writes werden beim nächsten Main fortgesetzt. Classic CAN benötigt
sein natives Mapping; ein v6-Frame passt erst in CAN-FD.

## Verwendung und Architektur

Die folgenden Mermaid-Diagramme werden auf GitHub und in Markdown-Viewern mit
Mermaid-Unterstützung dargestellt. Der bevorzugte Einstieg ist die C-Runtime:
Die Anwendung stellt eine statische Konfiguration bereit, initialisiert einmal
und ruft danach zyklisch `Kymo_Main()` auf. Die optionale C++-API ist ein
separater Sender für Anwendungen, die Messungen selbst zeitlich steuern.

### Datenweg: Anwendung bis KymoStudio

```mermaid
flowchart LR
    subgraph ECU["ECU / Firmware"]
        direction TB
        Values["Sensoren und Anwendungsparameter"]
        Config["kymo_config.c / .cpp<br/>Kanäle, Perioden, Flags, Callbacks"]
        Task["setup / Systemstart: Kymo_Init<br/>loop / zyklischer Task: Kymo_Main"]
        Runtime["C++11-Runtime<br/>Zeitplanung und Teilübertragungen"]
        Codec["v6-Codec<br/>Little Endian, float32, optionale CRC8"]
        Buffer["KymoContext.tx<br/>maximal 21 Bytes"]
        Driver["Anwendungsadapter / Treiber<br/>UART, USB, TCP oder MQTT"]
        Config --> Task
        Task --> Runtime
        Values -->|sample| Runtime
        Runtime --> Codec
        Codec --> Buffer
        Buffer -->|write| Driver
        Driver -->|akzeptierte Bytes / busy| Runtime
    end
    subgraph PC["PC: KymoStudio"]
        Receive["Verbindung empfängt Bytes"]
        Decode["Frames erkennen und prüfen<br/>Länge, Descriptor, optionale CRC"]
        Route["Messwerte nach Kanal-ID zuordnen<br/>Y, optional X / Z / Zeit"]
        View["Darstellung im Kymo"]
        Receive --> Decode --> Route --> View
    end
    Driver -->|"binäres v6: 9 bis 21 Bytes"| Receive
```

Die Konfiguration ist eine kompilierte C-Struktur, keine zur Laufzeit gelesene
Datei. Die Anwendung initialisiert Hardware und Verbindungen. Der Codec
serialisiert einzelne Werte; er überträgt keine Speicherabbilder von Strukturen.

### C-Runtime: Konfiguration, Speicher und Callback-Zugriffe

```mermaid
flowchart TB
    App["Anwendung besitzt alle Objekte<br/>Lebensdauer mindestens bis zum Nutzungsende"]
    Context["KymoContext<br/>config, start_ms, next_channel<br/>tx, tx_length, tx_offset, fault"]
    Config["const KymoConfig<br/>Kanalanzahl, Referenzen und Funktionszeiger"]
    Channels["const KymoChannel channels[N]<br/>period_ms, id, flags"]
    Times["uint32_t last_sample_ms[N]<br/>veränderlicher Zustand pro Kanal"]
    Clock["clock_ms(clock_user)<br/>liefert uint32 Millisekunden"]
    Sample["sample(sample_user, id, out)<br/>füllt KymoSample: value, x, z<br/>0 = überspringen; ungleich 0 = Messung"]
    Write["write(transport_user, bytes, length)<br/>liefert akzeptierte Byteanzahl"]
    Busy["busy(transport_user), optional<br/>ungleich 0 = Treiber belegt / Puffer geliehen"]
    Service["service(transport_user), optional<br/>Verbindung / Treiber zyklisch bedienen"]
    App --> Context
    App --> Config
    Context -->|"Referenz, keine Kopie"| Config
    Config --> Channels
    Config --> Times
    Config -.->|Funktionszeiger| Clock
    Config -.->|Funktionszeiger| Sample
    Config -.->|Funktionszeiger| Write
    Config -.->|Funktionszeiger| Busy
    Config -.->|Funktionszeiger| Service
```

`clock_user`, `sample_user` und `transport_user` transportieren eigene
Anwendungszustände oder Treiber-Handles als `void *`. So benötigt die Library
keine CPU-spezifischen Typen. Die C-Strukturen verwenden **keine Vererbung**.
Jede Runtime-Instanz benötigt einen eigenen `KymoContext` und ein eigenes
`last_sample_ms`-Array. Konfiguration und Kanaltabelle bleiben nach Init konstant.

| Schnittstelle | Aufgabe der Anwendung | Aufgabe der Library |
| --- | --- | --- |
| `clock_ms` | Monotone Millisekundenzeit liefern | Fälligkeit und relative Zeitstempel berechnen |
| `sample` | Werte für die angefragte Kanal-ID in `out` schreiben | Höchstens eine Messung pro Main-Aufruf anfordern |
| `write` | Bytes annehmen und die tatsächliche Anzahl melden | Nicht angenommene Bytes später erneut anbieten |
| `busy` | Belegung und geliehene Puffer melden | Puffer währenddessen nicht verändern |
| `service` | Begrenzte Treiber-/Verbindungsarbeit erledigen | Einmal pro gültigem Main-Aufruf aufrufen, auch bei Busy oder gespeichertem Fehler |

### Zyklischer Ablauf und Rückgabestatus

```mermaid
flowchart TD
    Start["Hardware initialisieren"] --> Init["Kymo_Init mit Kontext und Konfiguration"]
    Init --> Valid{"Konfiguration gültig?"}
    Valid -->|Nein| Fix["KYMO_BAD_CONFIG<br/>Konfiguration korrigieren"]
    Fix --> Init
    Valid -->|Ja| Ready["KYMO_OK<br/>Kanäle sind sofort erstmals fällig"]
    Ready --> Main["Nächster zyklischer Aufruf: Kymo_Main"]
    Main --> ContextOK{"Kontext und config vorhanden?"}
    ContextOK -->|Nein| BadConfig["KYMO_BAD_CONFIG"]
    ContextOK -->|Ja| Service["Optional service aufrufen"]
    Service --> Fault{"IO-Fehler gespeichert?"}
    Fault -->|Ja| IOError["KYMO_IO_ERROR"]
    Fault -->|Nein| Busy{"Optional busy meldet belegt?"}
    Busy -->|Ja| Wait["KYMO_BUSY"]
    Busy -->|Nein| Pending{"Restbytes vorhanden?"}
    Pending -->|Ja| Write["write mit verbleibenden Bytes"]
    Pending -->|Nein| Due{"Fälligen Kanal per Round-Robin gefunden?"}
    Due -->|Nein| Idle["KYMO_IDLE"]
    Due -->|Ja| Sample["sample aufrufen<br/>Abtastzeit aktualisieren"]
    Sample --> Available{"Messung verfügbar?"}
    Available -->|Nein| Skip["KYMO_SKIPPED"]
    Available -->|Ja| Encode["Werte prüfen und v6-Frame codieren"]
    Encode --> Finite{"Erforderliche Werte endlich?"}
    Finite -->|Nein| BadSample["KYMO_BAD_SAMPLE"]
    Finite -->|Ja| Write
    Write --> Count{"Gemeldete Byteanzahl?"}
    Count -->|Mehr als angefordert| Latch["IO-Fehler speichern"]
    Latch --> IOError
    Count -->|Rest bleibt, auch bei 0 Bytes| Wait
    Count -->|Alles angenommen| OK["KYMO_OK"]
    BadConfig --> End["Status an Anwendung zurückgeben"]
    IOError --> End
    Wait --> End
    Idle --> End
    Skip --> End
    BadSample --> End
    OK --> End
    End -->|"später erneut, keine Warteschleife in Main"| Main
```

`KYMO_OK` bedeutet, dass der Treiber den vollständigen Frame angenommen hat;
es ist keine Empfangsbestätigung der KymoStudio. Bei DMA oder USB muss `busy`
so lange gesetzt bleiben, wie der Treiber den Puffer benutzt. Ein gespeicherter
IO-Fehler erfordert eine Treiberkorrektur und sichere Neuinitialisierung.
Verpasste Abtastungen werden nicht als nachträglicher Messwertstapel aufgeholt.

### Usage: vorhandene ESP32-Konfiguration verwenden

Dieses Beispiel verwendet die bereits definierte `kymo_config` aus
[src/kymo_config.cpp](src/kymo_config.cpp). Dort werden eigene Kanäle,
Messwertzugriffe und Transport-Callbacks angepasst.

```cpp
#include <Arduino.h>
#include "kymo_config.h"

static KymoContext context;
static KymoStatus init_status = KYMO_BAD_CONFIG;
static KymoStatus last_status = KYMO_IDLE;

void setup()
{
    Serial.begin(115200);
    init_status = Kymo_Init(&context, &kymo_config);
}

void loop()
{
    if (init_status == KYMO_OK) {
        last_status = Kymo_Main(&context);
        // last_status bei Bedarf in der Anwendungsdiagnose auswerten.
    }
    // Weitere begrenzte Anwendungsarbeit erledigen.
}
```

Für eine andere ECU bleiben Init/Main und die Datenstrukturen gleich. Ersetzt
werden Hardwareinitialisierung und Callbacks. Diagnoseausgaben dürfen nicht in
denselben binären Datenstrom geschrieben werden. Aufrufe erfolgen aus einem
Task und nicht gleichzeitig aus Interrupts oder mehreren Threads.

### Optionale C++-API: Vererbung und Verwendung

```mermaid
classDiagram
    class Kymo {
        -KymoStream* stream
        -uint8_t buffer[21]
        +begin(outputStream, enableTimestamp)
        +setMillisecondCallback(callback)
        +send(channelId, yValue, includeTimestamp) bool
        +send2D(channelId, xValue, yValue, includeTimestamp) bool
        +send3D(channelId, xValue, yValue, zValue, includeTimestamp) bool
        +flush() bool
    }
    class KymoStream {
        <<abstract>>
        +write(data, length) size_t
        +busy() bool
    }
    class PrintStream {
        -Print* printObj
        +attach(p)
        +write(data, length) size_t
    }
    class UARTStream {
        -UART_HandleTypeDef* handle
        +write(data, length) size_t
        +busy() bool
    }
    class CDCStream {
        -TransmitFn transmit
        -BusyFn isBusy
        +write(data, length) size_t
        +busy() bool
    }
    class ApplicationStream {
        <<example>>
        +write(data, length) size_t
        +busy() bool
    }
    Kymo --> KymoStream : verwendet, besitzt nicht
    KymoStream <|-- PrintStream
    KymoStream <|-- UARTStream
    KymoStream <|-- CDCStream
    KymoStream <|-- ApplicationStream
```

Die Pfeile mit leerer Dreiecksspitze zeigen Vererbung. `Kymo` hält einen
Zeiger auf `KymoStream`; er erbt nicht davon und löscht den Stream nicht.
`ApplicationStream` ist eine mögliche eigene Implementierung, keine vorhandene
Projektklasse. Die Methodenliste zeigt die wesentlichen Schnittstellen; die
Header dokumentieren alle Überladungen.

`KymoStream::write()` ist rein virtuell. `busy()` hat eine Standardimplementierung
mit `false`; asynchrone Adapter müssen sie passend überschreiben. `PrintStream`
liegt in `examples/common/kymo_arduino.h`, die STM32-Beispieladapter liegen in
`examples/stm32/adapters/kymo_stm32.h`. Nur Anwendung und Adapter binden SDKs
ein; `Kymo` und `KymoStream` bleiben plattformunabhängig.

```mermaid
sequenceDiagram
    participant A as Anwendung
    participant P as Kymo
    participant S as KymoStream-Adapter
    A->>P: send / send2D / send3D
    P->>S: busy()
    alt Puffer frei, Transport bereit und Messung gültig
        P->>P: Frame codieren und ersten flush versuchen
        P->>S: write(bytes, length)
        S-->>P: akzeptierte Byteanzahl
        P-->>A: true, sofern kein Write-Vertragsfehler
    else Noch Daten offen, busy oder ungültige Messung
        P-->>A: false
    end
    loop In späteren Anwendungszyklen
        A->>P: flush()
        P->>S: busy()
        opt Transport bereit und Restbytes vorhanden
            P->>S: write(Restbytes)
            S-->>P: akzeptierte Byteanzahl
        end
        P-->>A: true wenn bereit und nichts mehr offen, sonst false
    end
```

Bei dieser API übernimmt die Anwendung die Abtastplanung. Ein erfolgreiches
`send*()` bedeutet Aufnahme in den Sendepuffer; kurze Writes werden durch
spätere `flush()`-Aufrufe abgeschlossen. Die Überladungen ohne
`includeTimestamp` senden ohne Zeitstempel. Für Zeitstempel muss zusätzlich
zur aktivierten Zeitstempeloption ein Millisekunden-Callback gesetzt sein.
Die C++-API verwendet denselben Codec wie die C-Runtime, ruft aber weder
`Kymo_Init` noch `Kymo_Main` auf.

## Prüfung

```sh
python tools/test_native.py --app ../KymoStudio
# Alternativ: nur C/C++-Tests ohne App
python tools/test_native.py
```

Benötigt gcc/g++. Unter Windows findet das Skript auch die PlatformIO-Toolchain:
`pio pkg install -g -t platformio/toolchain-gccmingw32`.
Die Prüfung kompiliert C99 und C++11 mit Warnungen als Fehler und prüft echte
C-Senderbytes gegen den Python-Decoder/Parser. Die App-Gesamtsuite kann im
App-Verzeichnis mit `python -m pytest -q` ausgeführt werden.

Weitere Beispiele und Build-Kommandos: [examples](examples/README.md).
Analyse/Entscheidungen: [docs/embedded-design.md](docs/embedded-design.md).
Prüfergebnisse: [docs/verification.md](docs/verification.md).

Der frühere A5A5-Prototyp wurde nach `legacy/` verschoben. Er wird nicht mehr
gebaut und ist nicht mit KymoStudio v6 kompatibel. Das unabhängige Events-
Submodul bleibt unverändert und ist keine Library-Abhängigkeit.

## Embedded-Coderegeln

Aktive C/C++-Funktionen verwenden höchstens ein `return` am Funktionsende.
Bitoperationen, Bytekonvertierung und CRC sind in einer unabhängigen
[Common-Komponente](lib/KymoCore/src/common/README.md) gebündelt.
`python tools/test_native.py` prüft die Coderegel und die Hilfsfunktionen mit.
Die dauerhaften Vorgaben stehen in [AGENTS.md](AGENTS.md).
