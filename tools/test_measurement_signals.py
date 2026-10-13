"""Compile the actual generator/codec, then check it with Studio's measurements."""
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
STUDIO = ROOT.parent / 'KymoStudio'
sys.path.insert(0, str(STUDIO / 'python'))
from Receiver.binary_protocol import ProtocolStreamDecoder, decode_data_frame
from Core.measurement_analysis import analyse


def main():
    compiler = shutil.which('g++') or str(Path.home()/'.platformio/packages/toolchain-gccmingw32/bin/g++.exe')
    build = ROOT/'.pio/measurement-native'
    build.mkdir(parents=True, exist_ok=True)
    exe = build/('signals.exe' if os.name == 'nt' else 'signals')
    library = ROOT/'lib/KymoCore/src'
    subprocess.run([compiler, '-std=c++11', '-Wall', '-Wextra', '-Werror',
                    '-DKYMO_ENABLE_TIMESTAMP=1', '-DKYMO_ENABLE_CRC=1',
                    '-I'+str(library), '-I'+str(ROOT/'examples/esp32/measurement_signals/include'),
                    str(ROOT/'test/native/measurement_signals_test.cpp'),
                    str(library/'kymo_protocol.cpp'), '-o', str(exe)], check=True)
    raw = build/'signals.bin'
    environment = {**os.environ, 'PATH': str(Path(compiler).parent)+os.pathsep+os.environ['PATH']}
    subprocess.run([str(exe), str(raw)], check=True, env=environment)
    points = [decode_data_frame(frame) for frame in ProtocolStreamDecoder().feed(raw.read_bytes())]
    rows = {key: [(p.timestamp, p.value) for p in points if p.id == key] for key in (0, 1)}
    assert all(len(values) == 1024 for values in rows.values())
    assert [t for t, _ in rows[0]] == [t for t, _ in rows[1]]
    for values in rows.values():
        assert all(abs(b[0]-a[0]-.005) < 1e-9 for a, b in zip(values, values[1:]))
    sine = analyse(rows[0], [], [], {'spectral': True, 'fft_size': 1024, 'window': 'rectangular', 'level': 0,
                                   'trigger': {'after': .1, 'pre': .1, 'post': .5}})
    peak = max(sine['spectrum'], key=lambda bin: bin[1])
    assert abs(sine['metrics']['frequency']-6.25) < 1e-5
    assert abs(sine['metrics']['rms']-1/math.sqrt(2)) < 1e-6
    assert abs(peak[0]-6.25) < 1e-9 and abs(peak[1]-1) < 1e-6
    assert sine['trigger'] is not None
    saw = analyse(rows[1], [], [], {})
    assert abs(saw['metrics']['frequency']-1.5625) < 1e-8
    assert abs(saw['metrics']['mean']+.0078125) < 1e-8
    print(json.dumps({'measurement_signals': 'passed', 'decoded_crc_frames': len(points),
                      'sine_peak': peak, 'saw_frequency': saw['metrics']['frequency']}))


if __name__ == '__main__':
    main()
