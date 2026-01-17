#!/usr/bin/env bash
set -euo pipefail

# Simple build script for the Orderbook project.
# - Builds with debug symbols by default (better for LLDB)
# - Enables a strong warning set and treats warnings as errors
# - Adds a few “const-correctness”-adjacent warnings (e.g., -Wcast-qual)
#
# Usage:
#   ./compile.sh            # debug build (default)
#   BUILD=release ./compile.sh
#   SAN=1 ./compile.sh      # add sanitizers (debug only; clang on macOS)
#   TIDY=0 ./compile.sh     # skip clang-tidy
#   TIDY_FIX=1 ./compile.sh # apply clang-tidy fixes (may rewrite files)

BUILD="${BUILD:-debug}"
SAN="${SAN:-0}"
TIDY="${TIDY:-1}"
TIDY_FIX="${TIDY_FIX:-0}"

CXX="${CXX:-clang++}"
SRC="src/main.cpp"
OUT="main"

CXXSTD="-std=c++23"

# Debug vs release flags
CXXFLAGS_COMMON=(
  "${CXXSTD}"
  -fcolor-diagnostics
)

if [[ "${BUILD}" == "release" ]]; then
  CXXFLAGS_MODE=(
    -O3
    -DNDEBUG
  )
else
  CXXFLAGS_MODE=(
    -O0
    -g
    -fno-omit-frame-pointer
  )
fi

# Warnings: strong but not insane (avoid -Weverything for sanity)
# Note: There is no compiler flag that forces "everything that can be const becomes const".
# The closest enforcement comes from tooling (clang-tidy), but these warnings help.
WARNFLAGS=(
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wfloat-conversion
  -Wdouble-promotion
  -Wformat=2
  -Wimplicit-fallthrough
  -Wextra-semi
  -Woverloaded-virtual
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wcast-qual
  -Wnull-dereference
  -Wundef
  -Wunreachable-code
  -Wrange-loop-analysis
  -Wloop-analysis
)

SANFLAGS=()
if [[ "${SAN}" == "1" && "${BUILD}" != "release" ]]; then
  SANFLAGS=(
    -fsanitize=address,undefined
    -fno-sanitize-recover=all
  )
fi

# clang-tidy: const-correctness enforcement
# Note: clang-tidy needs the same compilation flags after `--`.
if [[ "${TIDY}" == "1" ]]; then
  if command -v clang-tidy >/dev/null 2>&1; then
    TIDY_CHECKS=(
      'misc-const-correctness'
      'readability-make-member-function-const'
      'readability-const-return-type'
    )

    TIDY_ARGS=(
      -checks="$(IFS=,; echo "${TIDY_CHECKS[*]}")"
      -warnings-as-errors='*'
      -header-filter='^(.*/)?(src|include)/'
      -quiet
    )

    if [[ "${TIDY_FIX}" == "1" ]]; then
      TIDY_ARGS+=( -fix -fix-errors )
    fi

    clang-tidy "${SRC}" "${TIDY_ARGS[@]}" -- \
      "${CXXFLAGS_COMMON[@]}" \
      "${CXXFLAGS_MODE[@]}" \
      "${WARNFLAGS[@]}" \
      "${SANFLAGS[@]}"
  else
    echo "clang-tidy not found; skipping (install clang-tidy or set TIDY=0)." >&2
  fi
fi

mkdir -p build

"${CXX}" \
  "${SRC}" \
  -o "build/${OUT}" \
  "${CXXFLAGS_COMMON[@]}" \
  "${CXXFLAGS_MODE[@]}" \
  "${WARNFLAGS[@]}" \
  "${SANFLAGS[@]}"

echo "Built: build/${OUT} (BUILD=${BUILD}, SAN=${SAN})"