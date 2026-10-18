# Generated PlatformIO pre-script; reference this immutable file in extra_scripts.
from pathlib import Path
Import("env")
snapshot = Path(Dir(".").srcnode().abspath)
env.Append(CPPPATH=[str(snapshot)])
# SCons ignores include paths when deciding what to rebuild and cannot see the
# macro-included config header; naming the snapshot forces a full rebuild on change.
env.Append(CPPDEFINES=[("KYMO_USER_CONFIG_HEADER", r'\"kymo_user_config.h\"'),
                       ("KYMO_CONFIG_SNAPSHOT", "kymo_" + snapshot.name.replace("-", "_"))])
env.Append(CXXFLAGS=["-std=c++11"])
