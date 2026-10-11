# Generated PlatformIO post-script; run after library include resolution.
from pathlib import Path
Import("env")
snapshot = Path(Dir(".").srcnode().abspath)
library = env.BuildLibrary(str(Path(env.subst("$BUILD_DIR")) / "kymo-generated"),
                           str(snapshot), src_filter=["+<kymo_generated_config.cpp>"])
env.Append(LIBS=[library])
