#!/usr/bin/env python3
"""Compile the actual OTA source against real ArduinoJson and test doubles."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("arduinojson_src", type=Path, help="Directory containing ArduinoJson.h")
args = parser.parse_args()
json_src = args.arduinojson_src.resolve()
if not (json_src / "ArduinoJson.h").is_file():
    parser.error("ArduinoJson.h not found")
root = Path(__file__).resolve().parent.parent
native = root / "tests/native"
flags = [os.environ.get("CXX", "clang++"), "-std=c++11", "-O2", "-g",
         "-Wall", "-Wextra", "-Werror", "-Wno-non-c-typedef-for-linkage",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
         "-DARDUINOJSON_ENABLE_ARDUINO_STRING=0", "-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0",
         "-I" + str(native / "stubs"), "-I" + str(root / "src"), "-I" + str(json_src)]
with tempfile.TemporaryDirectory(prefix="ota-native-") as directory:
    build = Path(directory)
    subprocess.run(flags + ["-include", str(native / "allocation_hook.h"), "-c",
                           str(root / "src/GithubReleaseOTA.cpp"), "-o", str(build / "ota.o")], check=True)
    subprocess.run(flags + [str(native / "test.cpp"), str(build / "ota.o"),
                           "-o", str(build / "test")], check=True)
    subprocess.run([str(build / "test")], check=True, timeout=30)
