# Generated PlatformIO post-script; run after library include resolution.
from pathlib import Path
Import("env")
snapshot = Path(Dir(".").srcnode().abspath)
env.BuildSources(str(Path(env.subst("$BUILD_DIR")) / "kymo-generated"),
                 str(snapshot), src_filter=["+<kymo_generated_config.cpp>"])
