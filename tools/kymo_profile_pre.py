"""Resolve one verified immutable Kymo snapshot for a PlatformIO profile."""
from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

Import("env")

project_root = Path(Dir("#").srcnode().abspath)
environment = str(env["PIOENV"])
profile = "production" if environment.endswith("-production") else "development"
profile_root = project_root / "generated" / "kymo" / profile
selector_path = profile_root / "active_generation.json"
try:
    selector_bytes = selector_path.read_bytes()
    selector = json.loads(selector_bytes)
    generation = selector["generation"]
    if not isinstance(generation, str) or not re.fullmatch(r"[a-f0-9]{64}(?:-repair)?", generation):
        raise ValueError("invalid generation name")
    snapshot = profile_root / "generations" / generation
    manifest_bytes = (snapshot / "kymo_generation_manifest.json").read_bytes()
    if hashlib.sha256(manifest_bytes).hexdigest() != selector.get("manifest_sha256"):
        raise ValueError("selected generation manifest hash does not match selector")
    manifest = json.loads(manifest_bytes)
    if manifest.get("profile") != profile or manifest.get("core_config_api_version") != 1:
        raise ValueError("selected generation profile or Core contract version is invalid")
    for name, digest in manifest.get("files", {}).items():
        if not isinstance(name, str) or Path(name).name != name:
            raise ValueError("selected generation manifest contains an unsafe filename")
        if hashlib.sha256((snapshot / name).read_bytes()).hexdigest() != digest:
            raise ValueError("selected generation file hash does not match manifest: " + name)
    snapshot = snapshot.resolve(strict=True)
except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
    raise RuntimeError("Cannot use the generated Kymo " + profile + " profile; run the configurator: " + str(exc)) from exc

env["KYMO_PROFILE"] = profile
env["KYMO_SNAPSHOT_DIR"] = str(snapshot)
env.SConscript(str(snapshot / "kymo_build_export.py"),
               exports={"env": env})
