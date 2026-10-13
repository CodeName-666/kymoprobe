# Gesichertes Task-/Trace-Beispiel

Dies ist die am 13. Oktober 2026 vor dem Wechsel auf numerische Testsignale
gesicherte Firmware aus `../runtime_freertos`. Die laufende Firmware auf COM4
meldete `KymoProbeRuntimeFreeRtosCandidate`, passend zur hier gespeicherten
Trace-Dictionary-Prüfsumme. Quellcode, SDK-Adapter, Konfiguration und die aktive
generierte Development-Konfiguration wurden unverändert kopiert.

`saved_example.json` dokumentiert die gelesene Geräteidentität, den KymoCore-
Commit und SHA-256-Werte aller gesicherten Dateien. Das Originalverzeichnis
`runtime_freertos` wurde nicht überschrieben. Die vorhandenen lokalen Build-
Binärdateien liegen zusätzlich unter `.pio/saved-task-example/` im Repository;
sie sind Build-Artefakte und werden nicht eingecheckt.

Vom KymoProbe-Repository aus:

```sh
pio run -d examples/esp32/task_trace_example -e esp32-runtime-test
pio run -d examples/esp32/task_trace_example -e esp32-runtime-test -t upload --upload-port COM4
```

KymoStudio: 460800 Baud, Runtime-Trace- oder Function-Flow-Diagramm. Die
bisherigen Versuche und Einschränkungen stehen in der unverändert übernommenen
`README.md`. Dieses Archivieren ist keine zusätzliche Abnahme des Task-/ISR-
Adapters; insbesondere bleiben initiale Trace-Lücken gesondert zu bewerten.
