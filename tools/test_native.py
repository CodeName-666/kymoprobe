"""Compile real C99/C++11 code; optionally verify against the sibling app.

python tools/test_native.py --app ../PlotterApp
Requires gcc and g++; on Windows also discovers PlatformIO's gccmingw32.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "lib/PlotterLib/src"
BUILD = ROOT / ".pio/native"

def run(*args: object, capture: bool = False) -> str:
    result = subprocess.run([str(a) for a in args], cwd=ROOT, check=True,
                            text=True, stdout=subprocess.PIPE if capture else None)
    return result.stdout or ""

def verify_app(emitter: Path, app: Path) -> None:
    sys.path.insert(0, str(app.resolve() / "python"))
    from Core.parsing import parse_payload
    from Receiver.binary_protocol import ProtocolStreamDecoder, encode_data_point
    from Receiver.message import PlotDataPoint

    frames = [bytes.fromhex(line) for line in run(emitter, capture=True).splitlines()]
    assert len(frames) == 256
    expected = [PlotDataPoint(id=i, value=-2.5,
                x=1.25 if i & 4 else None, z_value=9.0 if i & 2 else None,
                timestamp=1.234 if i & 1 else None) for i in range(256)]
    assert frames == [encode_data_point(point) for point in expected]
    wire = b"".join(frames)
    for chunk_size in range(1, 43):
        decoder = ProtocolStreamDecoder()
        recovered = []
        for start in range(0, len(wire), chunk_size):
            recovered.extend(decoder.feed(wire[start:start + chunk_size]))
        assert [parse_payload(frame) for frame in recovered] == expected
    broken = bytearray(frames[0])
    broken[-1] ^= 0x80
    assert ProtocolStreamDecoder().feed(bytes(broken) + wire) == frames
    print("App integration passed: 256 channels, 8 layouts, 42 fragment sizes, CRC recovery")

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", type=Path)
    args = parser.parse_args()
    run(sys.executable, "-m", "unittest", "discover", "-s", "tools",
        "-p", "test_embedded_style.py")
    run(sys.executable, ROOT / "tools/check_embedded_style.py")
    if not shutil.which("gcc") and os.name == "nt":
        gcc_bin = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        os.environ["PATH"] = str(gcc_bin) + os.pathsep + os.environ["PATH"]
    if not shutil.which("gcc") or not shutil.which("g++"):
        parser.error("Install gcc/g++ or: pio pkg install -g -t platformio/toolchain-gccmingw32")
    BUILD.mkdir(parents=True, exist_ok=True)
    flags = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-I" + str(LIB)]
    objects = []
    for name in ("plotter_protocol", "plotter_runtime"):
        obj = BUILD / (name + ".o")
        run("gcc", "-std=c99", *flags, "-c", LIB / (name + ".c"), "-o", obj)
        objects.append(obj)
    for name in ("common_test", "runtime_test", "codec_test", "emit_frames"):
        exe = BUILD / (name + ".exe")
        run("gcc", "-std=c99", *flags, ROOT / "test/native" / (name + ".c"),
            *objects, "-lm", "-o", exe)
        if name != "emit_frames": run(exe)
    for source in (ROOT / "test/native/common_test.c",
                   ROOT / "test/native/push_test.cpp",
                   ROOT / "lib/PlotterLib/test/protocol_golden_test.cpp"):
        exe = BUILD / (source.stem + "_cpp.exe")
        run("g++", "-std=c++11", *flags, source, LIB / "plotter.cpp",
            *objects, "-o", exe)
        run(exe)
    # Defining target macros must not introduce SDK headers or change the API.
    portable = BUILD / "portable_cpp.exe"
    run("g++", "-std=c++11", *flags, "-DARDUINO=10819", "-DSTM32",
        "-DESP_PLATFORM", ROOT / "lib/PlotterLib/test/protocol_golden_test.cpp",
        LIB / "plotter.cpp", *objects, "-o", portable)
    run(portable)
    if args.app: verify_app(BUILD / "emit_frames.exe", args.app)
    print("All native checks passed")

if __name__ == "__main__": main()
