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

if [[ "$failures" -ne 0 ]]; then
  printf '\nParity tests failed: %s\n' "$failures"
  exit 1
fi

printf '\nAll parity tests passed\n'
