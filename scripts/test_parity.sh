#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
GLINT_BIN="$ROOT_DIR/glint"

if [[ ! -x "$GLINT_BIN" ]]; then
  echo "FAIL: glint binary not found. Run make first." >&2
  exit 1
fi

FIXTURES=(
  "image3.png"
  "image2.png"
  "loop.png"
)

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

run_and_capture() {
  local stdout_file="$1"
  local stderr_file="$2"
  shift 2

  set +e
  "$@" >"$stdout_file" 2>"$stderr_file"
  local exit_code=$?
  set -e

  printf '%s' "$exit_code"
}

failures=0

expect_success() {
  local label="$1"
  shift

  if "$@" >/dev/null 2>"$TMP_DIR/${label}.err"; then
    echo "PASS [$label]"
  else
    echo "FAIL [$label]: command failed"
    if [[ -s "$TMP_DIR/${label}.err" ]]; then
      echo "  stderr: $(tr '\n' ' ' < "$TMP_DIR/${label}.err")"
    fi
    failures=$((failures + 1))
  fi
}

expect_failure() {
  local label="$1"
  shift

  if "$@" >/dev/null 2>"$TMP_DIR/${label}.err"; then
    echo "FAIL [$label]: command unexpectedly succeeded"
    failures=$((failures + 1))
  else
    echo "PASS [$label]"
  fi
}

for fixture in "${FIXTURES[@]}"; do
  fixture_path="$ROOT_DIR/$fixture"
  if [[ ! -f "$fixture_path" ]]; then
    echo "FAIL [$fixture]: missing fixture file"
    failures=$((failures + 1))
    continue
  fi

  interp_stdout="$TMP_DIR/${fixture}.interp.out"
  interp_stderr="$TMP_DIR/${fixture}.interp.err"
  gen_c="$TMP_DIR/${fixture}.gen.c"
  gen_bin="$TMP_DIR/${fixture}.gen.bin"
  gen_stdout="$TMP_DIR/${fixture}.gen.out"
  gen_stderr="$TMP_DIR/${fixture}.gen.err"

  interp_exit=$(run_and_capture "$interp_stdout" "$interp_stderr" "$GLINT_BIN" "$fixture_path")

  if ! "$GLINT_BIN" "$fixture_path" -o "$gen_c" >/dev/null 2>"$TMP_DIR/${fixture}.codegen.err"; then
    echo "FAIL [$fixture]: code generation failed"
    failures=$((failures + 1))
    continue
  fi

  if ! gcc "$gen_c" -o "$gen_bin" >"$TMP_DIR/${fixture}.gcc.out" 2>"$TMP_DIR/${fixture}.gcc.err"; then
    echo "FAIL [$fixture]: generated C compilation failed"
    failures=$((failures + 1))
    continue
  fi

  gen_exit=$(run_and_capture "$gen_stdout" "$gen_stderr" "$gen_bin")

  fixture_failed=0

  if [[ "$interp_exit" != "$gen_exit" ]]; then
    echo "FAIL [$fixture]: exit code mismatch (interp=$interp_exit generated=$gen_exit)"
    fixture_failed=1
  fi

  if ! cmp -s "$interp_stdout" "$gen_stdout"; then
    echo "FAIL [$fixture]: stdout mismatch"
    echo "  interpreter: $(tr '\n' ' ' < "$interp_stdout")"
    echo "  generated:   $(tr '\n' ' ' < "$gen_stdout")"
    fixture_failed=1
  fi

  if ! cmp -s "$interp_stderr" "$gen_stderr"; then
    echo "FAIL [$fixture]: stderr mismatch"
    echo "  interpreter: $(tr '\n' ' ' < "$interp_stderr")"
    echo "  generated:   $(tr '\n' ' ' < "$gen_stderr")"
    fixture_failed=1
  fi

  if [[ "$fixture_failed" -eq 0 ]]; then
    echo "PASS [$fixture]"
  else
    failures=$((failures + 1))
  fi
done

for fixture in "${FIXTURES[@]}"; do
  fixture_path="$ROOT_DIR/$fixture"
  if [[ ! -f "$fixture_path" ]]; then
    echo "FAIL [opt-$fixture]: missing fixture file"
    failures=$((failures + 1))
    continue
  fi

  interp_stdout="$TMP_DIR/opt.${fixture}.interp.out"
  interp_stderr="$TMP_DIR/opt.${fixture}.interp.err"
  gen_c="$TMP_DIR/opt.${fixture}.gen.c"
  gen_bin="$TMP_DIR/opt.${fixture}.gen.bin"
  gen_stdout="$TMP_DIR/opt.${fixture}.gen.out"
  gen_stderr="$TMP_DIR/opt.${fixture}.gen.err"

  interp_exit=$(run_and_capture "$interp_stdout" "$interp_stderr" "$GLINT_BIN" "$fixture_path")

  if ! "$GLINT_BIN" "$fixture_path" --opt -o "$gen_c" >/dev/null 2>"$TMP_DIR/opt.${fixture}.codegen.err"; then
    echo "FAIL [opt-$fixture]: code generation failed"
    failures=$((failures + 1))
    continue
  fi

  if ! gcc "$gen_c" -o "$gen_bin" >"$TMP_DIR/opt.${fixture}.gcc.out" 2>"$TMP_DIR/opt.${fixture}.gcc.err"; then
    echo "FAIL [opt-$fixture]: generated C compilation failed"
    failures=$((failures + 1))
    continue
  fi

  gen_exit=$(run_and_capture "$gen_stdout" "$gen_stderr" "$gen_bin")

  fixture_failed=0

  if [[ "$interp_exit" != "$gen_exit" ]]; then
    echo "FAIL [opt-$fixture]: exit code mismatch (interp=$interp_exit generated=$gen_exit)"
    fixture_failed=1
  fi

  if ! cmp -s "$interp_stdout" "$gen_stdout"; then
    echo "FAIL [opt-$fixture]: stdout mismatch"
    echo "  interpreter: $(tr '\n' ' ' < "$interp_stdout")"
    echo "  generated:   $(tr '\n' ' ' < "$gen_stdout")"
    fixture_failed=1
  fi

  if ! cmp -s "$interp_stderr" "$gen_stderr"; then
    echo "FAIL [opt-$fixture]: stderr mismatch"
    echo "  interpreter: $(tr '\n' ' ' < "$interp_stderr")"
    echo "  generated:   $(tr '\n' ' ' < "$gen_stderr")"
    fixture_failed=1
  fi

  if [[ "$fixture_failed" -eq 0 ]]; then
    echo "PASS [opt-$fixture]"
  else
    failures=$((failures + 1))
  fi
done

# Basic CLI behavior checks
expect_success "cli-default-run-image3" "$GLINT_BIN" "$ROOT_DIR/image3.png"
expect_success "cli-trace-image3" "$GLINT_BIN" "$ROOT_DIR/image3.png" --trace 25
expect_success "cli-dump-image3" "$GLINT_BIN" "$ROOT_DIR/image3.png" --dump

expect_failure "cli-missing-image" "$GLINT_BIN"
expect_failure "cli-unknown-flag" "$GLINT_BIN" "$ROOT_DIR/image3.png" --wat
expect_failure "cli-steplimit-with-run" "$GLINT_BIN" "$ROOT_DIR/image3.png" --run 10
expect_failure "cli-missing-o-value" "$GLINT_BIN" "$ROOT_DIR/image3.png" -o
expect_failure "cli-opt-without-o" "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt
expect_failure "cli-opt-level-without-o" "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-level 1
expect_failure "cli-opt-report-without-o" "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-report
expect_failure "cli-invalid-opt-level" "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-level 9 -o "$TMP_DIR/bad.c"

if "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-level 2 -o "$TMP_DIR/level2.c" >/dev/null 2>"$TMP_DIR/level2.err"; then
  if gcc "$TMP_DIR/level2.c" -o "$TMP_DIR/level2.bin" >/dev/null 2>"$TMP_DIR/level2.gcc.err"; then
    level2_stdout="$TMP_DIR/level2.out"
    level2_stderr="$TMP_DIR/level2.stderr"
    interp2_stdout="$TMP_DIR/level2.interp.out"
    interp2_stderr="$TMP_DIR/level2.interp.stderr"
    level2_exit=$(run_and_capture "$level2_stdout" "$level2_stderr" "$TMP_DIR/level2.bin")
    interp2_exit=$(run_and_capture "$interp2_stdout" "$interp2_stderr" "$GLINT_BIN" "$ROOT_DIR/image3.png")
    if [[ "$level2_exit" == "$interp2_exit" ]] && cmp -s "$level2_stdout" "$interp2_stdout" && cmp -s "$level2_stderr" "$interp2_stderr"; then
      echo "PASS [cli-opt-level-2-parity]"
    else
      echo "FAIL [cli-opt-level-2-parity]: output or exit mismatch"
      failures=$((failures + 1))
    fi
  else
    echo "FAIL [cli-opt-level-2-parity]: generated C compilation failed"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [cli-opt-level-2-parity]: codegen failed"
  failures=$((failures + 1))
fi

l2_report_out="$TMP_DIR/l2_report.out"
if "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-level 2 --opt-report -o "$TMP_DIR/l2report.c" >"$l2_report_out" 2>"$TMP_DIR/l2_report.err"; then
  if grep -q "OPT_REPORT: passes=4 changes=1 nops=0 dirs=0 removed=5 dims=4x2->3x1" "$l2_report_out"; then
    echo "PASS [cli-opt-level-2-report-image3-values]"
  else
    echo "FAIL [cli-opt-level-2-report-image3-values]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$l2_report_out")"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [cli-opt-level-2-report-image3-values]: command failed"
  failures=$((failures + 1))
fi

opt_report_out="$TMP_DIR/opt_report.out"
if "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt --opt-report -o "$TMP_DIR/report.c" >"$opt_report_out" 2>"$TMP_DIR/opt_report.err"; then
  if grep -q "OPT_REPORT: passes=" "$opt_report_out"; then
    echo "PASS [cli-opt-report-output]"
  else
    echo "FAIL [cli-opt-report-output]: missing report output"
    failures=$((failures + 1))
  fi

  if grep -q "OPT_REPORT: passes=3 changes=1 nops=0 dirs=0 removed=5 dims=4x2->3x1" "$opt_report_out"; then
    echo "PASS [cli-opt-report-image3-values]"
  else
    echo "FAIL [cli-opt-report-image3-values]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$opt_report_out")"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [cli-opt-report-output]: command failed"
  failures=$((failures + 1))
fi

if "$GLINT_BIN" "$ROOT_DIR/image3.png" --opt-level 0 -o "$TMP_DIR/level0.c" >/dev/null 2>"$TMP_DIR/level0.err"; then
  if gcc "$TMP_DIR/level0.c" -o "$TMP_DIR/level0.bin" >/dev/null 2>"$TMP_DIR/level0.gcc.err"; then
    level0_stdout="$TMP_DIR/level0.out"
    level0_stderr="$TMP_DIR/level0.stderr"
    interp_stdout="$TMP_DIR/level0.interp.out"
    interp_stderr="$TMP_DIR/level0.interp.stderr"
    level0_exit=$(run_and_capture "$level0_stdout" "$level0_stderr" "$TMP_DIR/level0.bin")
    interp_exit=$(run_and_capture "$interp_stdout" "$interp_stderr" "$GLINT_BIN" "$ROOT_DIR/image3.png")
    if [[ "$level0_exit" == "$interp_exit" ]] && cmp -s "$level0_stdout" "$interp_stdout" && cmp -s "$level0_stderr" "$interp_stderr"; then
      echo "PASS [cli-opt-level-0-parity]"
    else
      echo "FAIL [cli-opt-level-0-parity]: output or exit mismatch"
      failures=$((failures + 1))
    fi
  else
    echo "FAIL [cli-opt-level-0-parity]: generated C compilation failed"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [cli-opt-level-0-parity]: codegen failed"
  failures=$((failures + 1))
fi

if [[ "$failures" -ne 0 ]]; then
  printf '\nParity tests failed: %s\n' "$failures"
  exit 1
fi

printf '\nAll parity tests passed\n'
