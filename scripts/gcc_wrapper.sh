#!/bin/sh
# Wrapper for compiler invoked by PlatformIO/SCons.
# Ensures that .pio build src directories exist before invoking real compiler.

# Create src dirs for all build environments
mkdir -p .pio/build/*/src 2>/dev/null || true

# Execute real compiler (forward all args)
exec xtensa-esp32-elf-g++ "$@"
