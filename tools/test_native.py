"""Compile the C++11 library and C99/C++11 clients; optionally verify against the sibling app.

python tools/test_native.py --app ../KymoStudio
Requires gcc and g++; on Windows also discovers PlatformIO's gccmingw32.
"""
from __future__ import annotations
import argparse
import os
import re
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


def verify_features() -> None:
    """Exercise defaults, independent options, unsupported requests and absent code."""
    profiles = {
        "minimal": {},
        "codec_only": {"RUNTIME": 0},
        "crc_only": {"CRC": 1},
        "cpp_only": {"CPP": 1, "RUNTIME": 0},
    }
    for mask in range(8):
        profiles[f"decoder_layout_{mask}"] = {
            "DECODER": 1, "X": int(bool(mask & 1)),
            "Z": int(bool(mask & 2)), "TIMESTAMP": int(bool(mask & 4)),
        }
    for name, options in profiles.items():
        print(f"Feature profile: {name}", flush=True)
        flags = ["-Os", "-Wall", "-Wextra", "-Werror", "-pedantic", "-I" + str(LIB)]
        # Pin test profiles independently of the user's editable header settings.
        selected = dict.fromkeys(("X", "Z", "TIMESTAMP", "CRC", "DECODER", "CPP"), 0)
        selected.update(RUNTIME=1)
        selected.update(options)
        flags += [f"-DKYMO_ENABLE_{key}={value}" for key, value in selected.items()]
        objects = []
        for source in ("kymo_protocol.cpp", "kymo_runtime.cpp", "kymo.cpp"):
            obj = BUILD / f"{name}_{source}.o"
            run("g++", "-std=c++11", "-fno-exceptions", "-fno-rtti",
                *flags, "-c", LIB / source, "-o", obj)
            objects.append(obj)
        symbols = run("nm", "--defined-only", *objects, capture=True)
        assert ("kymo_decode_data" in symbols) == bool(options.get("DECODER", 0)), name
        assert ("kymo_crc8" in symbols) == bool(options.get("CRC", 0) or options.get("DECODER", 0)), name
        assert ("Kymo_Main" in symbols) == bool(options.get("RUNTIME", 1)), name
        assert ("sendFrame" in symbols) == bool(options.get("CPP", 0)), name
        exe = BUILD / f"{name}.exe"
        client = BUILD / f"{name}_client.o"
        run("gcc", "-std=c99", *flags, "-c", ROOT / "test/native/features_test.c", "-o", client)
        run("g++", client, *objects[:2], "-lm", "-o", exe)
        run(exe)
        if options.get("CPP"):
            run("g++", "-std=c++11", *flags, ROOT / "test/native/features_cpp_test.cpp",
                *objects, "-o", exe)
            run(exe)
    for name in ("RUNTIME", "X", "Z", "TIMESTAMP", "CRC", "DECODER", "CPP"):
        result = subprocess.run(
            ["g++", "-std=c++11", "-I" + str(LIB), f"-DKYMO_ENABLE_{name}=2",
             "-fsyntax-only", str(LIB / "kymo_protocol.cpp")],
            cwd=ROOT, capture_output=True, text=True)
        assert result.returncode != 0 and "must be 0 or 1" in result.stderr, name

def verify_header_config() -> None:
    """Build real library copies with edited internal settings and explicit overrides."""
    copied = BUILD / "header_config" / "lib"
    shutil.copytree(LIB, copied, dirs_exist_ok=True)
    config = copied / "kymo_build_config.h"
    assert config.is_file(), "Internal editable build configuration header is missing"
    settings = config.read_text(encoding="utf-8")
    for name in ("X", "Z", "TIMESTAMP", "CRC", "DECODER", "CPP"):
        settings = re.sub(rf"(?m)^#define KYMO_ENABLE_{name}\s+[^\n]+$",
                          f"#define KYMO_ENABLE_{name} 1", settings)
    settings = re.sub(r"(?m)^#define KYMO_ENABLE_RUNTIME\s+[^\n]+$",
                      "#define KYMO_ENABLE_RUNTIME 0", settings)
    config.write_text(settings, encoding="utf-8")
    for override in (False, True):
        flags = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-I" + str(copied)]
        if override:
            flags += ["-DKYMO_ENABLE_X=0", "-DKYMO_ENABLE_CRC=0",
                      "-DKYMO_ENABLE_CPP=0", "-DKYMO_ENABLE_RUNTIME=1"]
        objects = []
        for source in ("kymo_protocol.cpp", "kymo_runtime.cpp", "kymo.cpp"):
            obj = copied.parent / (source + ".o")
            run("g++", "-std=c++11", "-fno-exceptions", "-fno-rtti",
                *flags, "-c", copied / source, "-o", obj)
            objects.append(obj)
        symbols = run("nm", "--defined-only", *objects, capture=True)
        assert ("Kymo_Main" in symbols) == override
        assert ("sendFrame" in symbols) != override
        obj = copied.parent / "features.o"
        run("gcc", "-std=c99", *flags, "-c", ROOT / "test/native/features_test.c", "-o", obj)
        exe = copied.parent / "features.exe"
        run("g++", obj, *objects, "-o", exe)
        run(exe)
        if not override:
            run("g++", "-std=c++11", *flags,
                ROOT / "lib/KymoCore/test/protocol_golden_test.cpp", *objects, "-o", exe)
            run(exe)
    config.write_text(settings.replace("#define KYMO_ENABLE_Z 1",
                                       "#define KYMO_ENABLE_Z 2"), encoding="utf-8")
    result = subprocess.run(["g++", "-std=c++11", "-I" + str(copied), "-fsyntax-only",
                             str(copied / "kymo_protocol.cpp")],
                            cwd=ROOT, capture_output=True, text=True)
    assert result.returncode != 0 and "must be 0 or 1" in result.stderr
    print("Internal header configuration and compiler overrides passed")


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
    legacy = subprocess.run(
        ["g++", "-std=c++98", "-I" + str(LIB), "-fsyntax-only",
         str(LIB / "kymo_protocol.cpp")], capture_output=True, text=True)
    assert legacy.returncode != 0 and "requires C++11" in legacy.stderr
    verify_header_config()
    verify_features()
    for crc_enabled in (False, True):
        print(f"Testing encoder CRC={crc_enabled}", flush=True)
        flags = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-I" + str(LIB)]
        flags += [f"-DKYMO_ENABLE_{name}=1" for name in
                  ("RUNTIME", "X", "Z", "TIMESTAMP", "DECODER", "CPP")]
        flags.append(f"-DKYMO_ENABLE_CRC={int(crc_enabled)}")
        objects = []
        for name in ("kymo_protocol", "kymo_runtime"):
            obj = BUILD / (name + ".o")
            run("g++", "-std=c++11", "-fno-exceptions", "-fno-rtti",
                *flags, "-c", LIB / (name + ".cpp"), "-o", obj)
            objects.append(obj)
        for name in ("common_test", "runtime_test", "codec_test", "emit_frames"):
            exe = BUILD / (name + ".exe")
            client = BUILD / (name + "_client.o")
            run("gcc", "-std=c99", *flags, "-c",
                ROOT / "test/native" / (name + ".c"), "-o", client)
            run("g++", client, *objects, "-lm", "-o", exe)
            if name != "emit_frames": run(exe)
        for source in (ROOT / "test/native/common_test.c",
                       ROOT / "test/native/push_test.cpp",
                       ROOT / "lib/KymoCore/test/protocol_golden_test.cpp"):
            exe = BUILD / (source.stem + "_cpp.exe")
            run("g++", "-std=c++11", *flags, source, LIB / "kymo.cpp",
                *objects, "-o", exe)
            run(exe)
        # Defining target macros must not introduce SDK headers or change the API.
        portable = BUILD / "portable_cpp.exe"
        run("g++", "-std=c++11", *flags, "-DARDUINO=10819", "-DSTM32",
            "-DESP_PLATFORM", ROOT / "lib/KymoCore/test/protocol_golden_test.cpp",
            LIB / "kymo.cpp", *objects, "-o", portable)
        run(portable)
        if args.app: verify_app(BUILD / "emit_frames.exe", args.app, crc_enabled)
    print("All native checks passed")

if __name__ == "__main__": main()
