"""Run KymoCore's native suite and the frame emitter; optionally verify against the sibling app.

python tools/test_native.py --app ../KymoStudio
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
LIB = ROOT / "lib/KymoCore/src"
BUILD = ROOT / ".pio/native"

def run(*args: object, capture: bool = False) -> str:
    result = subprocess.run([str(a) for a in args], cwd=ROOT, check=True,
                            text=True, stdout=subprocess.PIPE if capture else None)
    return result.stdout or ""

def verify_app(emitter: Path, app: Path, crc_enabled: bool) -> None:
    sys.path.insert(0, str(app.resolve() / "python"))
    from Core.parsing import parse_payload
    from Receiver.binary_protocol import ProtocolStreamDecoder, encode_data_point
    from Receiver.message import PlotDataPoint

    frames = [bytes.fromhex(line) for line in run(emitter, capture=True).splitlines()]
    assert len(frames) == 256
    expected = [PlotDataPoint(id=i, value=-2.5,
                x=1.25 if i & 4 else None, z_value=9.0 if i & 2 else None,
                timestamp=1.234 if i & 1 else None) for i in range(256)]
    assert frames == [encode_data_point(point, crc_enabled=crc_enabled) for point in expected]
    wire = b"".join(frames)
    for chunk_size in range(1, 43):
        decoder = ProtocolStreamDecoder()
        recovered = []
        for start in range(0, len(wire), chunk_size):
            recovered.extend(decoder.feed(wire[start:start + chunk_size]))
        assert [parse_payload(frame) for frame in recovered] == expected
    broken = bytearray(encode_data_point(expected[0], crc_enabled=True))
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
    # KymoCore owns its library tests; KymoProbe only checks its own wire consumers.
    run(sys.executable, ROOT / "lib/KymoCore/test/native_test.py")
    sources = [LIB / "telemetry/kymo_protocol.cpp", LIB / "telemetry/kymo_runtime.cpp"]
    for crc_enabled in (False, True):
        print(f"Testing encoder CRC={crc_enabled}", flush=True)
        flags = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-I" + str(LIB),
                 "-DKYMO_PROFILE=KYMO_PROFILE_XYZ", "-DKYMO_MAX_CHANNELS=256",
                 f"-DKYMO_ENABLE_CRC={int(crc_enabled)}"]
        objects = []
        for source in sources:
            obj = BUILD / (source.stem + ".o")
            run("g++", "-std=c++11", "-fno-exceptions", "-fno-rtti", *flags, "-c", source, "-o", obj)
            objects.append(obj)
        client = BUILD / "emit_frames_client.o"
        exe = BUILD / "emit_frames.exe"
        run("gcc", "-std=c99", *flags, "-c", ROOT / "test/native/emit_frames.c", "-o", client)
        run("g++", client, *objects, "-lm", "-o", exe)
        run(exe, capture=True)
        if args.app: verify_app(exe, args.app, crc_enabled)
    print("All native checks passed")

if __name__ == "__main__": main()
