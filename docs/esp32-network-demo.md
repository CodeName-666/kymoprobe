# ESP32: TCP und MQTT parallel

Die Root-Firmware besitzt zwei PlatformIO-Umgebungen. `nodemcu-32s` bleibt die
USB-Demo mit zwei 50-Hz-Kanaelen. `nodemcu-32s-network` verwendet WLAN und sendet
zwei kuenstliche Signale mit jeweils 10 Hz und zwei Sekunden Periodendauer:

| Transport | Signal-ID | Signal | Studio-Einstellung |
| --- | --- | --- | --- |
| TCP | 2 | Sinus, -1 bis +1 | Telnet, Client, Board-IP, Port 2323 |
| MQTT | 3 | Saegezahn, -1 bis +1 | broker.hivemq.com, Port 1883 |

TCP ist ein binaerer Kymo-Datenstrom, kein interaktives Telnet-Protokoll.
MQTT sendet je Nachricht genau einen binaeren Kymo-Frame, QoS 0, nicht retained.
Topic: `kymotrace/demo/7c92e4a83f16/saw`.
Client-ID der Firmware: `kymo-7c92e4a83f16-board`; fuer weitere Boards aendern.
Studio und Testprogramm verwenden eigene Client-IDs.

## Einrichten und starten

1. `include/network_secrets.h.example` nach `include/network_secrets.h` kopieren.
   Dort SSID und Passwort eines 2,4-GHz-WLANs eintragen. Die Datei ist ignoriert;
   auch `.pio/` mit der Zugangsdaten enthaltenden Firmware bleibt ignoriert.
2. USB-Erfassung im Studio stoppen, damit der serielle Port frei wird.
3. `pio run -e nodemcu-32s-network -t upload --upload-port COM4` ausfuehren.
4. Seriellen Monitor mit 115200 Baud oeffnen. Alle fuenf Sekunden erscheinen
   WLAN-Status, Board-IP und MQTT-Verbindungsstatus. Keine Zugangsdaten werden
   ausgegeben. Nach einem Neustart kann DHCP eine andere IP zuweisen.
5. In KymoStudio `config/esp32-network.json` verwenden, die TCP-IP gegebenenfalls
   aktualisieren und Erfassung starten. Der vorbereitete Windows-Launcher liegt
   im Studio-Projekt unter `start_esp32_network.bat`.

Der PC kann ueber Ethernet verbunden sein. Fuer direkten TCP-Zugriff muss er
die Board-IP erreichen koennen; Gastnetz-Isolation kann dies verhindern. MQTT
benoetigt auf beiden Seiten nur eine Verbindung zum gleichen Broker.

Der oeffentliche [HiveMQ-Testbroker](https://www.hivemq.com/mqtt/public-mqtt-broker/)
ist gemeinsam genutzt, ohne Anmeldung und auf Port 1883 unverschluesselt.
Nur diese kuenstlichen Signale verwenden. Ein eindeutiges Topic verhindert
versehentliche Vermischung, bietet aber keinen Zugriffsschutz.

## Grenzen und Pruefung

Zwei Kymo-Kontexte besitzen getrennte feste Sendepuffer. Bei TCP-Neuverbindung
wird ein angefangener Frame verworfen und die Zeitbasis neu gestartet; MQTT
startet nach erfolgreichem Reconnect ebenfalls neu. Verpasste Samples werden
nicht nachgeliefert. Es wird jeweils ein TCP-Client akzeptiert.

Arduino-WiFi und PubSubClient verwenden intern Heap und synchrone Netzwerkaufrufe.
Verbindungsaufbau, DNS und Sendestau koennen die Superloop verzoegern. Dies ist
eine Live-Demo ohne harte Echtzeitgarantie. MQTT wird alle fuenf Sekunden erneut
verbunden, WLAN verwendet den SDK-Reconnect. USB liefert hier nur Diagnosen.

Hardwaretest (Studio-Erfassung vorher stoppen): `tools/test_network_demo.py`
mit dem Python des Studio-venv, `PYTHONPATH=../KymoStudio/python` und Board-IP
als Argument ausfuehren. Er prueft beide Datenwege, Signal-IDs, Wertebereiche,
Zeitstempel, Empfangsraten und TCP-Neuverbindung.

Rueckkehr zur USB-Demo: `pio run -e nodemcu-32s -t upload --upload-port COM4`,
danach den Studio-Launcher `start_esp32.bat` starten.
