"""Compile and verify actual USB stress firmware with a host Arduino adapter."""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "lib/KymoCore/src"
BUILD = ROOT / ".pio/native/stress"


def main() -> None:
    if not shutil.which("g++") and os.name == "nt":
        compiler_directory = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        os.environ["PATH"] = str(compiler_directory) + os.pathsep + os.environ["PATH"]
    if not shutil.which("g++"):
        raise RuntimeError("Install g++ or PlatformIO's toolchain-gccmingw32")
    BUILD.mkdir(parents=True, exist_ok=True)
    for channels, xyz, period in ((256, 0, 0), (256, 1, 0), (3, 0, 5), (1, 1, 1)):
        executable = BUILD / f"stress_{channels}_{xyz}_{period}.exe"
        flags = ["-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                 "-fno-exceptions", "-fno-rtti", "-DKYMO_STRESS_ENABLE=1", "-DKYMO_MAX_CHANNELS=256",
                 f"-DKYMO_STRESS_CHANNEL_COUNT={channels}",
                 f"-DKYMO_STRESS_XYZ_ENABLE={xyz}", f"-DKYMO_STRESS_PERIOD_MS={period}",
                 "-DKYMO_ENABLE_TIMESTAMP=1", "-DKYMO_ENABLE_X=1", "-DKYMO_ENABLE_Z=1",
                 "-DKYMO_ENABLE_DECODER=1", "-DKYMO_ENABLE_CRC=0", "-DKYMO_ENABLE_RUNTIME=1",
                 "-I" + str(ROOT / "test/stress_stub"), "-I" + str(LIB),
                 "-I" + str(ROOT / "include")]
        subprocess.run(["g++", *flags, str(ROOT / "test/native/stress_demo_test.cpp"),
                        str(LIB / "telemetry/kymo_runtime.cpp"), str(LIB / "telemetry/kymo_protocol.cpp"),
                        "-o", str(executable)], cwd=ROOT, check=True)
        subprocess.run([str(executable)], cwd=ROOT, check=True)
        print(f"Passed: {channels} channels, XYZ={xyz}, period={period} ms", flush=True)
    # Reject invalid channel counts instead of truncating IDs at compile time.
    for channels in (0, 257):
        result = subprocess.run(["g++", "-std=c++11", "-fsyntax-only",
                                 "-DKYMO_STRESS_ENABLE=1", "-DKYMO_ENABLE_TIMESTAMP=1",
                                 f"-DKYMO_STRESS_CHANNEL_COUNT={channels}",
                                 "-I" + str(ROOT / "test/stress_stub"), "-I" + str(LIB),
                                 "-I" + str(ROOT / "include"),
                                 str(ROOT / "src/stress_demo.cpp")],
                                cwd=ROOT, capture_output=True, text=True)
        assert result.returncode != 0 and "must be 1..256" in result.stderr
    print("Stress firmware checks passed: partial writes, stalls, rollover, stream reset")


if __name__ == "__main__":
    main()
