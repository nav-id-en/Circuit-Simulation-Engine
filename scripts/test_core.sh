#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${TMPDIR:-/tmp}/proteuslab_sdl_core"
mkdir -p "$build_dir"

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -I"$project_root/include" \
  "$project_root/src/core/Component.cpp" \
  "$project_root/src/core/Circuit.cpp" \
  "$project_root/src/components/Components.cpp" \
  "$project_root/src/simulation/SimulationEngine.cpp" \
  "$project_root/src/simulation/FirmwareLoader.cpp" \
  "$project_root/src/persistence/SimpleJson.cpp" \
  "$project_root/src/persistence/CircuitSerializer.cpp" \
  "$project_root/tests/core_tests.cpp" \
  -o "$build_dir/proteus_core_tests"

"$build_dir/proteus_core_tests"
