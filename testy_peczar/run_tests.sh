#!/usr/bin/env bash
set -u

CXX="${CXX:-g++}"
CXXFLAGS="${CXXFLAGS:--Wall -Wextra -O2 -std=c++23}"
OUTDIR="${OUTDIR:-build_tests}"

TEST_SOURCES=(playlist_test.cpp playlist_test_extern.cpp)

# Tests that are supposed to FAIL compilation (by design).
EXPECT_COMPILE_FAIL=(103)

is_expected_compile_fail() {
  local x="$1"
  for t in "${EXPECT_COMPILE_FAIL[@]}"; do
    [[ "$t" == "$x" ]] && return 0
  done
  return 1
}

have_valgrind=0
if command -v valgrind >/dev/null 2>&1; then
  have_valgrind=1
fi

# Extract all TEST_NUM values that appear in the test sources.
extract_all_tests() {
  grep -hroE 'TEST_NUM[[:space:]]*==[[:space:]]*[0-9]+' "${TEST_SOURCES[@]}" \
    | sed -E 's/.*==[[:space:]]*([0-9]+).*/\1/' \
    | sort -n -u
}

usage() {
  cat <<'EOF'
Usage:
  ./run_tests.sh                 # run all tests found in sources
  ./run_tests.sh 101 201 606     # run selected TEST_NUMs

Env:
  CXX=g++|clang++
  CXXFLAGS="..."
  OUTDIR="build_tests"
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

# Sanity checks
for f in "${TEST_SOURCES[@]}" playlist.h; do
  if [[ ! -f "$f" ]]; then
    echo "ERROR: Missing file: $f"
    echo "Run this script from the directory containing playlist.h and the test .cpp files."
    exit 1
  fi
done

mkdir -p "$OUTDIR"

if [[ "$#" -gt 0 ]]; then
  TEST_NUMS=("$@")
else
  mapfile -t TEST_NUMS < <(extract_all_tests)
fi

echo "Compiler : $CXX"
echo "CXXFLAGS : $CXXFLAGS"
echo "OUTDIR   : $OUTDIR"
echo "Valgrind : $([[ $have_valgrind -eq 1 ]] && echo yes || echo no)"
echo

for tn in "${TEST_NUMS[@]}"; do
  exe="${OUTDIR}/playlist_test_${tn}"
  log="${OUTDIR}/build_${tn}.log"

  echo "============================================================"
  echo "[TEST]  TEST_NUM=${tn}"

  # Compile (capture output so we can decide what to do).
  set +e
  "$CXX" $CXXFLAGS -DTEST_NUM="$tn" "${TEST_SOURCES[@]}" -o "$exe" >"$log" 2>&1
  rc=$?
  set -e

  if is_expected_compile_fail "$tn"; then
    if [[ $rc -ne 0 ]]; then
      echo "[PASS]  Compilation failed as expected for TEST_NUM=${tn}"
      echo "        (build log: $log)"
      continue
    else
      echo "[FAIL]  Compilation unexpectedly SUCCEEDED for TEST_NUM=${tn} (should fail)."
      echo "        Binary: $exe"
      exit 1
    fi
  else
    if [[ $rc -ne 0 ]]; then
      echo "[FAIL]  Compilation failed for TEST_NUM=${tn}"
      echo "-------- compiler output (from $log) --------"
      cat "$log"
      echo "--------------------------------------------"
      exit 1
    fi
  fi

  # Run
  echo "[RUN]   $exe"
  "$exe"

  # Optional Valgrind
  if [[ $have_valgrind -eq 1 ]]; then
    echo "[VG]    $exe"
    valgrind \
      --error-exitcode=123 \
      --leak-check=full \
      --show-leak-kinds=all \
      --errors-for-leak-kinds=all \
      --run-cxx-freeres=yes \
      -q \
      "$exe"
  fi

  echo "[OK]    TEST_NUM=${tn}"
done

echo "============================================================"
echo "All selected tests completed."
