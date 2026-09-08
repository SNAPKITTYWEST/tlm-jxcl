#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")" && pwd)
command -v nvcc >/dev/null && nvcc -O3 -arch=sm_80 "$ROOT/cuda/kimi_butterfly.cu" -o "$ROOT/kimi_butterfly" || echo 'nvcc not present: CUDA compile skipped'
command -v c++ >/dev/null && c++ -std=c++20 -O2 -c "$ROOT/compiler/forge_compiler.cpp" -o "$ROOT/compiler/forge_compiler.o" || echo 'c++ not present: compiler compile skipped'
command -v rustc >/dev/null && rustc --crate-type lib "$ROOT/runtime/threadpiper.rs" -o "$ROOT/runtime/libthreadpiper.rlib" || echo 'rustc not present: runtime compile skipped'
command -v rustc >/dev/null && rustc --crate-type lib "$ROOT/audit/state_audit.rs" -o "$ROOT/audit/libstate_audit.rlib" || echo 'rustc not present: audit compile skipped'
