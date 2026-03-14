Import("env")

import os
import subprocess
import shutil
import urllib.request
import time
import hashlib


PROJECT_DIR = env.subst("$PROJECT_DIR")
PYTHON_EXE = env.subst("$PYTHONEXE")
FXDATA_TXT = os.path.join(PROJECT_DIR, "fxdata", "fxdata.txt")
FXDATA_BIN = os.path.join(PROJECT_DIR, "fxdata", "fxdata.bin")
TOOLS_DIR = os.path.join(PROJECT_DIR, "tools")
FXDATA_HASH_FILE = os.path.join(PROJECT_DIR, ".pio", "fxdata.last.sha256")
FX_BUILD_URL = "https://raw.githubusercontent.com/MrBlinky/Arduboy-Python-Utilities/main/fxdata-build.py"
FX_UPLOAD_URL = "https://raw.githubusercontent.com/MrBlinky/Arduboy-Python-Utilities/main/fxdata-upload.py"


def _run(cmd, env_vars=None):
    print("[FX] Running:", " ".join(cmd))
    env_copy = os.environ.copy()
    if env_vars:
        env_copy.update(env_vars)
    subprocess.check_call(cmd, cwd=PROJECT_DIR, env=env_copy)


def _require_file(path, message):
    if not os.path.isfile(path):
        print("[FX] ERROR:", message)
        env.Exit(1)


def _ensure_python_package(module_name, package_name):
    try:
        __import__(module_name)
    except ImportError:
        print("[FX] Installing Python package:", package_name)
        _run([PYTHON_EXE, "-m", "pip", "install", package_name])


def _download_tool(script_name, url):
    os.makedirs(TOOLS_DIR, exist_ok=True)
    destination = os.path.join(TOOLS_DIR, script_name)
    print("[FX] Downloading", script_name)
    urllib.request.urlretrieve(url, destination)
    return destination


def _sha256_of_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        while True:
            chunk = handle.read(65536)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def _read_last_uploaded_hash():
    if not os.path.isfile(FXDATA_HASH_FILE):
        return ""
    with open(FXDATA_HASH_FILE, "r", encoding="ascii") as handle:
        return handle.read().strip()


def _write_last_uploaded_hash(hash_value):
    os.makedirs(os.path.dirname(FXDATA_HASH_FILE), exist_ok=True)
    with open(FXDATA_HASH_FILE, "w", encoding="ascii") as handle:
        handle.write(hash_value)


def _upload_only_if_changed_enabled():
    value = env.GetProjectOption("custom_fx_upload_only_if_changed", "yes")
    return str(value).strip().lower() in ("1", "true", "yes", "on")


def _find_tool(script_name):
    local_script = os.path.join(TOOLS_DIR, script_name)
    if os.path.isfile(local_script):
        return [PYTHON_EXE, local_script]

    path_script = shutil.which(script_name)
    if path_script:
        return [PYTHON_EXE, path_script]

    if script_name == "fxdata-build.py":
        fetched = _download_tool(script_name, FX_BUILD_URL)
        return [PYTHON_EXE, fetched]

    if script_name == "fxdata-upload.py":
        fetched = _download_tool(script_name, FX_UPLOAD_URL)
        return [PYTHON_EXE, fetched]

    print("[FX] ERROR: Could not find", script_name)
    env.Exit(1)


def _build_fxdata(*_, **__):
    _ensure_python_package("PIL", "pillow")
    _require_file(
        FXDATA_TXT,
        "Missing fxdata/fxdata.txt. Create it before building FX data.",
    )
    cmd = _find_tool("fxdata-build.py")
    _run(cmd + [FXDATA_TXT])
    _require_file(
        FXDATA_BIN,
        "fxdata build did not produce fxdata/fxdata.bin.",
    )


def _upload_fxdata(*_, **__):
    _ensure_python_package("serial", "pyserial")
    _build_fxdata()

    current_hash = _sha256_of_file(FXDATA_BIN)
    if _upload_only_if_changed_enabled():
        last_hash = _read_last_uploaded_hash()
        if last_hash and last_hash == current_hash:
            print("[FX] No fxdata changes detected, skipping FX upload")
            return

    cmd = _find_tool("fxdata-upload.py")
    forced_port = env.GetProjectOption("custom_fx_port", "").strip()
    extra_env = {"FX_UPLOAD_PORT": forced_port} if forced_port else None

    last_error = None
    for attempt in range(1, 4):
        try:
            _run(cmd + [FXDATA_BIN], env_vars=extra_env)
            _write_last_uploaded_hash(current_hash)
            return
        except Exception as exc:
            last_error = exc
            print(f"[FX] Upload attempt {attempt}/3 failed")
            time.sleep(2.5)

    print("[FX] ERROR: FX upload failed after retries")
    raise last_error


def _auto_upload_enabled():
    value = env.GetProjectOption("custom_fx_upload_on_upload", "no")
    return str(value).strip().lower() in ("1", "true", "yes", "on")


env.AddCustomTarget(
    name="fxdata",
    dependencies=None,
    actions=[_build_fxdata],
    title="Build FX Data",
    description="Build fxdata/fxdata.bin and fxdata/fxdata.h from fxdata/fxdata.txt",
)

env.AddCustomTarget(
    name="uploadfx",
    dependencies=None,
    actions=[_upload_fxdata],
    title="Upload FX Data",
    description="Build and upload fxdata/fxdata.bin to the FX chip",
)

if _auto_upload_enabled():
    env.AddPostAction("upload", _upload_fxdata)
