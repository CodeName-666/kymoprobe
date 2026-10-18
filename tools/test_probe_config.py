"""Compile Probe and Core translation units against both global profiles."""
from __future__ import annotations

import shutil
import subprocess
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "lib/KymoCore/src"
APP_FIXTURE = ROOT / "test/native/config_contract.c"
SWITCHES = ("RUNTIME", "X", "Z", "TIMESTAMP", "CRC", "DECODER",
            "CPP", "TRACE", "TRACE_ISR", "TRACE_SCHEDULER", "TRACE_FLOW")
EFFECTIVE_FEATURES = ("RUNTIME", "X", "Z", "TIMESTAMP", "CRC", "DECODER",
                      "CPP", "TRACE", "TRACE_ISR", "TRACE_SCHEDULER", "TRACE_FLOW")


def run(*args: object) -> None:
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True)


def resolved_features(compiler: str, language: str, flags: list[str]) -> dict[str, int]:
    """Ask the compiler to evaluate effective gates under one TU's flags."""
    probe = ['#include <kymo_config.h>']
    for feature in EFFECTIVE_FEATURES:
        probe.extend((f"#if KYMO_EFFECTIVE_{feature}",
                      f"PROBE_{feature} 1", "#else",
                      f"PROBE_{feature} 0", "#endif"))
    result = subprocess.run(
        [compiler, "-std=c99" if language == "c" else "-std=c++11",
         "-x", language, "-E", "-P", *flags, "-"], cwd=ROOT,
        input="\n".join(probe), text=True, capture_output=True, check=True)
    values: dict[str, int] = {}
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) == 2 and fields[0].startswith("PROBE_"):
            values[fields[0][6:]] = int(fields[1])
    assert len(values) == len(EFFECTIVE_FEATURES), result.stdout
    return values


def verify_profile(name: str, enabled: int) -> None:
    build = ROOT / ".pio/native" / ("probe_config_" + name)
    build.mkdir(parents=True, exist_ok=True)
    flags = ["-I" + str(CORE), "-I" + str(ROOT / "include"),
             "-DKYMO_ENABLED=" + str(enabled),
             "-DKYMO_EXPECTED_ENABLED=" + str(enabled)]
    flags.extend("-DKYMO_ENABLE_" + switch + "=1" for switch in SWITCHES)

    run("gcc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
        *flags, "-fsyntax-only", APP_FIXTURE)
    app_features = resolved_features("gcc", "c", flags)
    core_features = resolved_features("g++", "c++", flags)
    assert app_features == core_features, (name, app_features, core_features)
    assert app_features["RUNTIME"] == enabled, (name, app_features)
    assert all(value == enabled for value in app_features.values()), (name, app_features)
    objects = []
    for source in ("telemetry/kymo_protocol.cpp", "telemetry/kymo_runtime.cpp", "telemetry/kymo_sender.cpp"):
        obj = build / (Path(source).name + ".o")
        run("g++", "-std=c++11", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fno-exceptions", "-fno-rtti", *flags, "-c", CORE / source,
            "-o", obj)
        objects.append(obj)
    if enabled == 0:
        symbols = subprocess.run(["nm", "--defined-only", *map(str, objects)],
                                 cwd=ROOT, text=True, capture_output=True, check=True)
        definitions = [line for line in symbols.stdout.splitlines()
                       if line and ":" not in line and line[0].isalnum()
                       and not line.split()[-1].startswith(".")]
        assert not definitions, ("Production Core objects define symbols", definitions)
    print(f"Probe/Core config contract passed: {name}")


def main() -> None:
    if os.name == "nt" and (not shutil.which("gcc") or not shutil.which("g++")):
        mingw = Path.home() / ".platformio/packages/toolchain-gccmingw32/bin"
        if mingw.is_dir():
            os.environ["PATH"] = str(mingw) + os.pathsep + os.environ["PATH"]
    if not shutil.which("gcc") or not shutil.which("g++"):
        raise SystemExit("Install gcc/g++ before running the Probe config contract")
    if not shutil.which("nm"):
        raise SystemExit("Install nm before running the production symbol check")
    verify_profile("development", 1)
    verify_profile("production", 0)
    app_flags = ["-I" + str(CORE), "-I" + str(ROOT / "include"),
                 "-DKYMO_ENABLED=0"]
    core_flags = [flag for flag in app_flags if flag != "-DKYMO_ENABLED=0"]
    core_flags.append("-DKYMO_ENABLED=1")
    app_features = resolved_features("gcc", "c", app_flags)
    core_features = resolved_features("g++", "c++", core_flags)
    assert app_features != core_features, "Mismatched App/Core profiles were not detected"
    print("Mismatched App/Core feature settings correctly detected")


if __name__ == "__main__":
    main()
