"""Resolve the trace demo's verified immutable configurator snapshot."""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

Import("env")

project_root = Path(Dir("#").srcnode().abspath)
profile = "production" if str(env["PIOENV"]).endswith("-production") else "development"
profile_root = project_root / "generated" / "kymo" / profile
selector_path = profile_root / "active_generation.json"
try:
    selector = json.loads(selector_path.read_text(encoding="utf-8"))
    generation = selector["generation"]
    if not isinstance(generation, str) or not re.fullmatch(r"[a-f0-9]{64}(?:-repair\d*)?", generation):
        raise ValueError("invalid generation name")
    snapshot = profile_root / "generations" / generation
    manifest_bytes = (snapshot / "kymo_generation_manifest.json").read_bytes()
    if hashlib.sha256(manifest_bytes).hexdigest() != selector.get("manifest_sha256"):
        raise ValueError("manifest hash does not match selector")
    manifest = json.loads(manifest_bytes)
    if manifest.get("profile") != profile or manifest.get("core_config_api_version") != 1:
        raise ValueError("snapshot profile or Core contract differs")
    for name, digest in manifest.get("files", {}).items():
        if not isinstance(name, str) or Path(name).name != name:
            raise ValueError("snapshot manifest contains an unsafe filename")
        if hashlib.sha256((snapshot / name).read_bytes()).hexdigest() != digest:
            raise ValueError("snapshot file hash mismatch: " + name)
    snapshot = snapshot.resolve(strict=True)
except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
    raise RuntimeError("Generate the trace example profile first: " + str(exc)) from exc

env["KYMO_PROFILE"] = profile
env["KYMO_SNAPSHOT_DIR"] = str(snapshot)
# SCons ignores include paths when deciding what to rebuild and cannot see the
# macro-included config header; naming the snapshot forces a full rebuild on change.
env.AppendUnique(CPPDEFINES=[("KYMO_CONFIG_SNAPSHOT", "kymo_" + snapshot.name.replace("-", "_"))])
env.SConscript(str(snapshot / "kymo_build_export.py"), exports={"env": env})
env.AppendUnique(CXXFLAGS=["-fno-exceptions", "-fno-rtti"])
