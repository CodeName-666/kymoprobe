"""Select generated settings when the central demo switch needs a profile."""
from pathlib import Path
import re

Import("env")

project_root = Path(Dir("#").srcnode().abspath)
header = (project_root / "include/demo_selection.h").read_text(encoding="utf-8")
constants = dict(re.findall(r"^#define\s+(KYMO_DEMO_\w+)\s+(\d+)\s*$", header, re.M))
default = re.search(r"^#define\s+KYMO_DEMO_MODE\s+(\w+)\s*$", header, re.M)
flags = env.get("BUILD_FLAGS", [])
flag_text = flags if isinstance(flags, str) else " ".join(str(flag) for flag in flags)
overrides = re.findall(r"-DKYMO_DEMO_MODE=(\w+)", flag_text)
selection = overrides[-1] if overrides else default.group(1) if default else ""
mode_text = constants.get(selection, selection)
if not mode_text.isdigit() or int(mode_text) not in range(12):
    raise RuntimeError("Select a named KYMO_DEMO_MODE or an integer from 0 to 11")
env["KYMO_SELECTED_DEMO"] = int(mode_text)
if int(mode_text) == 8:
    env.SConscript(str(project_root / "tools/kymo_profile_pre.py"), exports={"env": env})
elif int(mode_text) in (9, 10):
    env.SConscript(str(project_root / "tools/probe_trace_pre.py"), exports={"env": env})
