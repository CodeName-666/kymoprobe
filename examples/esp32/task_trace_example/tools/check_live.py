"""Run the real sibling KymoStudio with opt-in live runtime diagnostics.

Uses the ordinary Studio receiver, model and QML views. Leaves the GUI open.
No generated input and no modification of the application's receive limits.
"""
from collections import Counter, defaultdict
import json
from pathlib import Path
import sys
import time
import os
import tempfile

EXAMPLE = Path(__file__).resolve().parents[1]
STUDIO = EXAMPLE.parents[2].parent / 'KymoStudio'
sys.path.insert(0, str(STUDIO / 'python'))
from PySide6.QtCore import QTimer, QObject
from Backend.backend import Backend
from Studio.studio import KymoStudio
from Receiver.trace_protocol import decode_batch
from main import main

OUTPUT = Path(os.environ.get('KYMO_RUNTIME_TEST_OUTPUT',
    str(Path(tempfile.gettempdir()) / 'KymoProbe-runtime-test')))
OUTPUT.mkdir(exist_ok=True)
TEST_SECONDS = int(os.environ.get('KYMO_RUNTIME_TEST_SECONDS', '600'))
counts = Counter()
markers = defaultdict(set)
sessions = set()
cores = set()
state = {'started': None, 'capture_started': False, 'complete': False}
original_ingest = Backend._on_receiver_traces
original_load = KymoStudio.load_app


def ingest(backend, interface, messages):
    for message in messages:
        if getattr(message, 'kind', None) == 3:
            _, events = decode_batch(message)
            sessions.add(str(message.session_id))
            if events and state['started'] is None:
                state['started'] = time.monotonic()
            for event in events:
                counts[f'{event.context_id}:{event.event_type}'] += 1
                cores.add(event.core_id)
                if event.flags & 2:
                    counts['invalid_events'] += 1
                if event.event_type == 3:
                    markers[str(event.symbol_id)].add(event.arg)
    return original_ingest(backend, interface, messages)


def inspect(studio):
    backend = Backend.get_instance()
    bridge = backend.traceBridge
    elapsed = 0 if state['started'] is None else time.monotonic() - state['started']
    selected = bridge._selected()
    if selected and not state['capture_started']:
        state['capture_started'] = bridge.startCapture(str(OUTPUT / 'live.kymotrace'))
    report = {'elapsed_s': round(elapsed, 2), 'complete': elapsed >= TEST_SECONDS,
              'session_ids': sorted(sessions), 'cores': sorted(cores),
              'event_counts': dict(counts), 'markers': {k: sorted(v) for k, v in markers.items()},
              'diagnostics': backend.get_diagnostics(), 'capture_status': bridge.status}
    visual_items = [studio.rootObjects()[0].contentItem()]
    for item in visual_items:
        visual_items.extend(item.childItems())
    renderers = [obj for obj in visual_items if obj.objectName() == 'runtimeTraceRenderer']
    report['runtime_views'] = [{key: obj.property(key) for key in
        ('start', 'duration', 'followLive', 'width', 'height')} for obj in renderers]
    if selected:
        session = selected[3]
        span_counts = Counter((s.context_id, s.status) for s in session.spans)
        durations = defaultdict(list)
        for span in session.spans:
            if span.elapsed_ticks is not None:
                durations[f'{span.context_id}:{span.symbol_id}'].append(span.elapsed_ticks)
        report.update(metadata=session.metadata, dictionary=session.dictionary,
                      gaps=[{'reason': g.reason, 'detail': str(g.detail)} for g in session.gaps],
                      span_counts={f'{c}:{s}': n for (c, s), n in span_counts.items()},
                      durations_us={k: {'count': len(v), 'min': min(v), 'max': max(v),
                                        'mean': round(sum(v)/len(v), 2)} for k, v in durations.items()},
                      retained=session.retained_counts)
        snapshot = bridge.snapshot(bridge.selectedSession, 0, 200000, 'runtime_trace', '', '', 0)
        report['runtime_projection'] = {'lanes': snapshot.get('lanes'),
                                        'items': len(snapshot.get('items', [])),
                                        'gaps': snapshot.get('gaps')}
    temporary = OUTPUT / 'live-report.tmp'
    temporary.write_text(json.dumps(report, indent=2, default=str)+'\n')
    temporary.replace(OUTPUT / 'live-report.json')
    if elapsed >= 20 and not state.get('screenshot_saved'):
        state['screenshot_saved'] = studio.rootObjects()[0].grabWindow().save(str(OUTPUT / 'studio-runtime-live.png'))
    if report['complete']:
        bridge.stopCapture()
        state['complete'] = True
        studio.live_timer.stop()
        Backend._on_receiver_traces = original_ingest
        studio.rootObjects()[0].grabWindow().save(str(OUTPUT / 'studio-runtime.png'))


def load(studio):
    result = original_load(studio)
    if studio.rootObjects():
        studio.live_timer = QTimer(studio, interval=10000)
        studio.live_timer.timeout.connect(lambda: inspect(studio))
        studio.live_timer.start()
    return result


if __name__ == '__main__':
    Backend._on_receiver_traces = ingest
    KymoStudio.load_app = load
    sys.exit(main(['--config', str(EXAMPLE / 'kymostudio.json'), '--start']))
