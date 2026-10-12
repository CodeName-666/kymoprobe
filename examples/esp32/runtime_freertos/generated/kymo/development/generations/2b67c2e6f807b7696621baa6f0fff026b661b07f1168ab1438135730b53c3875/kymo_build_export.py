# Generated PlatformIO pre-script; reference this immutable file in extra_scripts.
from pathlib import Path
Import("env")
snapshot = Path(Dir(".").srcnode().abspath)
env.Append(CPPPATH=[str(snapshot)])
env.Append(CPPDEFINES=[("KYMO_USER_CONFIG_HEADER", r'\"kymo_user_config.h\"')])
env.Append(CXXFLAGS=["-std=c++11"])
