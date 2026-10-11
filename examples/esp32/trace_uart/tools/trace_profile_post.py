"""Compile the generated trace/profile sources into a dedicated library."""
from pathlib import Path

Import("env")

project_root = Path(Dir("#").srcnode().abspath)
snapshot = Path(env.get("KYMO_SNAPSHOT_DIR", ""))
if not snapshot.is_dir():
    raise RuntimeError("Trace profile pre-script did not select a snapshot")
project_env = env.Clone()
project_env.AppendUnique(CPPPATH=[str(project_root / "../../../lib/KymoCore/src")])
env.SConscript(str(snapshot / "kymo_build_sources.py"),
               exports={"env": env, "projenv": project_env})
