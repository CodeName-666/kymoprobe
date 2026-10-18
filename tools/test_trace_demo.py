"""Build a native trace producer and ingest its real frames with KymoStudio."""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "lib/KymoCore/src"
STUDIO = ROOT.parent / "KymoStudio"
if not (STUDIO / "python").is_dir():
    STUDIO = ROOT / "KymoStudio"
BUILD = ROOT / ".pio/native/trace_demo"
CONFIGURATOR = ROOT / "lib/KymoCore/tools/configurator/src"
sys.path.insert(0, str(STUDIO / "python"))
sys.path.insert(0, str(CONFIGURATOR))

from Core.trace_model import TraceSessionStore
from Receiver.trace_protocol import decode_frame
from kymo_configurator.generation import generate
from kymo_configurator.model import load_project


def compiler_path() -> None:
    """Add PlatformIO's bundled MinGW compiler to PATH when needed."""
    if os.name == "nt" and (not shutil.which("g++") or not shutil.which("nm")):
        toolchain = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        if toolchain.is_dir():
            os.environ["PATH"] = str(toolchain) + os.pathsep + os.environ["PATH"]


def compile_demo(executable: Path, snapshot: Path) -> None:
    """Compile trace API, protocol, mux and the host producer together."""
    command = [
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-fno-exceptions", "-fno-rtti", "-I" + str(CORE),
        "-I" + str(snapshot),
        "-DKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\"",
        str(ROOT / "test/native/trace_demo.cpp"),
        str(CORE / "trace/kymo_trace.cpp"),
        str(CORE / "trace/kymo_trace_protocol.cpp"),
        str(CORE / "transport/kymo_mux.cpp"), "-o", str(executable),
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def compile_adapter_test(executable: Path, snapshot: Path) -> None:
    """Compile the source-built FreeRTOS adapter against deterministic hooks."""
    command = [
        "g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
        "-fno-exceptions", "-fno-rtti", "-DKYMO_ENABLE_TRACE_ISR=1",
        "-DKYMO_ENABLE_TRACE_SCHEDULER=1", "-I" + str(CORE),
        "-I" + str(snapshot),
        "-I" + str(ROOT / "examples/esp32/trace_uart/src"),
        "-DKYMO_USER_CONFIG_HEADER=\"kymo_user_config.h\"",
        str(ROOT / "test/native/freertos_adapter_test.cpp"),
        str(ROOT / "examples/esp32/trace_uart/src/kymo_freertos_trace_adapter.cpp"),
        str(CORE / "trace/kymo_trace.cpp"), "-o", str(executable),
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def decode_stream(data: bytes) -> list:
    """Extract complete wire-v2 frames from the raw binary demo output."""
    frames = []
    offset = 0
    while offset < len(data):
        if data[offset:offset + 2] != b"\xa5\x5a":
            raise AssertionError(f"invalid sync at output offset {offset}")
        if len(data) - offset < 24:
            raise AssertionError("truncated frame header")
        frame_length = 24 + int.from_bytes(data[offset + 4:offset + 6], "little")
        end = offset + frame_length
        if end > len(data):
            raise AssertionError("truncated trace frame")
        try:
            frames.append(decode_frame(data[offset:end]))
        except Exception as exc:
            raise AssertionError(
                f"invalid frame at {offset}: {data[offset:end].hex()}") from exc
        offset = end
    return frames


def main() -> None:
    """Build, run and validate the native-to-Studio trace path."""
    compiler_path()
    BUILD.mkdir(parents=True, exist_ok=True)
    project_path = ROOT / "examples/esp32/trace_uart/kymo.project.json"
    snapshot = generate(load_project(project_path), "development",
                        BUILD / "generated")
    executable = BUILD / "trace_demo.exe"
    compile_demo(executable, snapshot)
    adapter_executable = BUILD / "freertos_adapter_test.exe"
    compile_adapter_test(adapter_executable, snapshot)
    subprocess.run([str(adapter_executable)], cwd=ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix="kymo-trace-demo-") as directory:
        capture = Path(directory) / "trace.ktr"
        with capture.open("wb") as stream:
            subprocess.run([str(executable)], cwd=ROOT, stdout=stream, check=True)
        frames = decode_stream(capture.read_bytes())
    store = TraceSessionStore()
    for frame in frames:
        store.ingest(frame)
    if len(store.sessions) != 1:
        raise AssertionError(f"expected one session, found {len(store.sessions)}")
    session = next(iter(store.sessions.values()))
    if session.metadata.get("wire_version") != 2:
        raise AssertionError("SESSION_META did not reach the Studio session model")
    if session.dictionary.get("generation") != 1:
        raise AssertionError("trace dictionary did not reach the Studio session model")
    if not session.spans or not session.markers:
        raise AssertionError("trace events did not reconstruct spans and markers")
    print(f"Native trace round trip passed: {len(frames)} frames, "
          f"{len(session.spans)} spans, {len(session.markers)} markers")


if __name__ == "__main__":
    main()
