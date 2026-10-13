"""Build the selected feature firmware on the host and verify its wire output."""
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    compiler_dir = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
    if not shutil.which("g++") and os.name == "nt":
        os.environ["PATH"] = str(compiler_dir) + os.pathsep + os.environ["PATH"]
    output = ROOT / ".pio/native/features_demo"
    output.mkdir(parents=True, exist_ok=True)
    core = ROOT / "lib/KymoCore/src"
    executable = output / "features_demo.exe"
    command = ["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
               "-fno-exceptions", "-fno-rtti", "-DKYMO_DEMO_MODE=KYMO_DEMO_FEATURES",
               "-include", str(ROOT / "include/demo_selection.h"),
               "-I" + str(ROOT / "include"), "-I" + str(ROOT / "test/stress_stub"),
               "-I" + str(core), str(ROOT / "test/native/features_demo_test.cpp"),
               str(core / "kymo.cpp"), str(core / "kymo_protocol.cpp"),
               str(core / "kymo_runtime.cpp"), "-o", str(executable)]
    subprocess.run(command, cwd=ROOT, check=True)
    subprocess.run([str(executable)], cwd=ROOT, check=True)
    print("Feature firmware passed: central dispatch, XYZ/time, CRC rejection, "
          "decoder roundtrip, partial writes, stalls and clock rollover")


if __name__ == "__main__":
    main()
