"""Compile the same immutable Kymo snapshot after PlatformIO resolves libraries."""
from pathlib import Path

Import("env")

project_root = Path(Dir("#").srcnode().abspath)
snapshot = Path(env.get("KYMO_SNAPSHOT_DIR", ""))
if env.get("KYMO_SNAPSHOT_DIR"):
    project_env = env.Clone()
    project_env.AppendUnique(CPPPATH=[str(project_root / "lib" / "KymoCore" / "src")])
    env.SConscript(str(snapshot / "kymo_build_sources.py"),
                   exports={"env": env, "projenv": project_env})
