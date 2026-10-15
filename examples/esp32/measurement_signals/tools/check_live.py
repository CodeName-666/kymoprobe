"""Read actual ESP32 numeric UART frames and verify time/frequency/amplitude."""
import argparse
from collections import Counter
import json
from pathlib import Path
import sys
import time

EXAMPLE = Path(__file__).resolve().parents[1]
STUDIO = EXAMPLE.parents[2].parent/'KymoStudio'
sys.path.insert(0, str(STUDIO/'python'))
import serial
from Receiver.binary_protocol import ProtocolStreamDecoder, decode_data_frame
from Core.measurement_analysis import analyse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--port', default='COM4')
    parser.add_argument('--seconds', type=float, default=15)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert 6 <= args.seconds <= 60
    port = serial.Serial()
    port.port = args.port; port.baudrate = 921600; port.timeout = .1
    port.dtr = False; port.rts = False
    decoder = ProtocolStreamDecoder()
    rows = {key: [] for key in range(4)}
    invalid = 0
    port.open()
    try:
        end = time.monotonic()+args.seconds
        while time.monotonic() < end:
            for frame in decoder.feed(port.read(max(1, min(port.in_waiting, 65536)))):
                try:
                    p = decode_data_frame(frame)
                    if p.id in rows:
                        rows[p.id].append((p.timestamp, p.value))
                    else:
                        invalid += 1
                except ValueError:
                    invalid += 1
    finally:
        port.close()
    gaps = {str(key): sum(abs(b[0]-a[0]-.001)>1e-6 for a,b in zip(values, values[1:])) for key,values in rows.items()}
    intervals = {str(key): dict(Counter(round(1000*(b[0]-a[0])) for a,b in zip(values,values[1:]))) for key,values in rows.items()}
    assert all(len(values) >= 1024 for values in rows.values()), 'Insufficient samples from the actual board'
    sine = analyse(rows[0], [], [], {'spectral': True, 'fft_size':1024,'window':'rectangular','level':0})
    saw = analyse(rows[1], [], [], {})
    peak = max(sine['spectrum'],key=lambda b:b[1])
    report = {'port':args.port,'baud':921600,'duration_s':args.seconds,
              'received':{str(k):len(v) for k,v in rows.items()},'invalid':invalid,
              'timestamp_gaps':gaps,'interval_counts_ms':intervals,
              'sine_frequency':sine['metrics']['frequency'],'sine_rms':sine['metrics']['rms'],
              'sine_fft_peak':peak,'saw_frequency':saw['metrics']['frequency']}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    assert invalid == 0 and not any(gaps.values()), report
    assert abs(report['sine_frequency']-62.5)<.001 and abs(report['saw_frequency']-7.8125)<.0001, report
    assert abs(peak[0]-62.5)<.0001 and abs(peak[1]-1)<.0001, report
    print(json.dumps(report))


if __name__ == '__main__':
    main()
