#!/usr/bin/env bash
# Regenerate Conan dependency files into build/ from scratch.
# Extra args are forwarded to `conan install`, e.g. ./conan_install.sh -s build_type=Debug
set -euo pipefail

cd "$(dirname "$0")"

rm -rf build CMakeUserPresets.json compile_commands.json .cache
conan install . -b missing "$@"

echo
echo "Done. In VSCode: CMake: Select Configure Preset -> conan-release, then Configure."
