Import("env")
import os

# Ensure that the .pio build src directory exists before building, to avoid
# intermittent failures where the assembler can't create object files.
# Run as a pre-action for the 'buildprog' target.

def ensure_build_src(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    src_dir = os.path.join(build_dir, "src")
    if not os.path.isdir(src_dir):
        os.makedirs(src_dir, exist_ok=True)

# Create it immediately when this script is loaded (ensures it exists before compilation)
build_dir = env.subst("$BUILD_DIR")
_os_src_dir = os.path.join(build_dir, "src")
if not os.path.isdir(_os_src_dir):
    os.makedirs(_os_src_dir, exist_ok=True)

# Also hook it to run before the program is linked/built and before the build target
env.AddPreAction("buildprog", ensure_build_src)
env.AddPreAction("build", ensure_build_src)

# Use a compiler wrapper that will recreate the build/src directory before each
# compilation invocation. This protects against mid-build deletions that cause
# the assembler to fail with "can't create ... .o: No such file or directory".
# We will create a small wrapper with the same name as the toolchain compiler
# and prepend the project's scripts folder to PATH so SCons/PlatformIO will
# pick up the wrapper automatically.
import shutil

tool_name = env.get("CXX", "xtensa-esp32-elf-g++")
real_tool = shutil.which(tool_name) or tool_name
scripts_dir = os.path.join(env.subst("${PROJECT_DIR}"), "scripts")
wrapper_path = os.path.join(scripts_dir, tool_name)
# Write the wrapper script
try:
    with open(wrapper_path, "w") as wf:
        wf.write("#!/bin/sh\n")
        wf.write("# Ensure build src directories exist\n")
        wf.write("mkdir -p .pio/build/*/src 2>/dev/null || true\n")
        wf.write(f"exec \"{real_tool}\" \"$@\"\n")
    os.chmod(wrapper_path, 0o755)
except Exception:
    pass

# Prepend scripts dir to PATH so the wrapper is found first
env.PrependENVPath("PATH", scripts_dir)
