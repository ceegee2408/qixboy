Import("env")
import shutil
import os

# Path to the generated hex
firmware_path = env.subst("$BUILD_DIR/${PROGNAME}.hex")

# Destination folder
archive_dir = os.path.join(env["PROJECT_DIR"], "hex_archive")
os.makedirs(archive_dir, exist_ok=True)

# Fixed filename (will be replaced every build)
archive_path = os.path.join(archive_dir, "firmware_latest.hex")

def after_build(source, target, env):
    if os.path.exists(firmware_path):
        shutil.copyfile(firmware_path, archive_path)
        print(f"Updated HEX: {archive_path}")
    else:
        print("HEX file not found.")

env.AddPostAction("buildprog", after_build)