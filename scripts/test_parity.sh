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

generate_store_load_fixture() {
  local out_png="$1"
  python3 - "$out_png" <<'PY'
import colorsys
import struct
import zlib
import binascii
import sys

out_path = sys.argv[1]

def hsv_to_rgb(h, s, v):
    r, g, b = colorsys.hsv_to_rgb(h / 360.0, s / 100.0, v / 255.0)
    return int(round(r * 255)), int(round(g * 255)), int(round(b * 255))

def png_chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", binascii.crc32(tag + data) & 0xFFFFFFFF)

# Program: DATA(7) -> STORE_A -> LOAD_A -> OUT_NUM -> HALT
data_px = (7, 7, 7, 255)
store_a_px = (*hsv_to_rgb(228, 100, 120), 255)   # Indigo low-V
load_a_px = (*hsv_to_rgb(228, 100, 220), 255)    # Indigo high-V
out_num_px = (*hsv_to_rgb(324, 100, 120), 255)   # Pink low-V
halt_px = (0, 0, 0, 255)

pixels = [data_px, store_a_px, load_a_px, out_num_px, halt_px]
width = len(pixels)
height = 1

raw = bytearray()
raw.append(0)  # filter byte for row 0
for r, g, b, a in pixels:
    raw.extend([r, g, b, a])

ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
idat = zlib.compress(bytes(raw))

with open(out_path, "wb") as f:
    f.write(b"\x89PNG\r\n\x1a\n")
    f.write(png_chunk(b"IHDR", ihdr))
    f.write(png_chunk(b"IDAT", idat))
    f.write(png_chunk(b"IEND", b""))
PY
}

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
  if grep -q "OPT_REPORT: passes=6 changes=1 nops=0 dirs=0 lit=0 removed=5 dims=4x2->3x1" "$l2_report_out"; then
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

  if grep -q "OPT_REPORT: passes=3 changes=1 nops=0 dirs=0 lit=0 removed=5 dims=4x2->3x1" "$opt_report_out"; then
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

store_load_png="$TMP_DIR/store_load_fixture.png"
generate_store_load_fixture "$store_load_png"

sl_l1_report="$TMP_DIR/store_load_l1_report.out"
sl_l2_report="$TMP_DIR/store_load_l2_report.out"

if "$GLINT_BIN" "$store_load_png" --opt-level 1 --opt-report -o "$TMP_DIR/store_load_l1.c" >"$sl_l1_report" 2>"$TMP_DIR/store_load_l1.err" && \
   "$GLINT_BIN" "$store_load_png" --opt-level 2 --opt-report -o "$TMP_DIR/store_load_l2.c" >"$sl_l2_report" 2>"$TMP_DIR/store_load_l2.err"; then
  if grep -q "OPT_REPORT: passes=3 changes=0 nops=0 dirs=0 lit=0 removed=0 dims=5x1->5x1" "$sl_l1_report"; then
    echo "PASS [opt-store-load-level1-report]"
  else
    echo "FAIL [opt-store-load-level1-report]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$sl_l1_report")"
    failures=$((failures + 1))
  fi

  if grep -q "OPT_REPORT: passes=6 changes=2 nops=2 dirs=0 lit=0 removed=0 dims=5x1->5x1" "$sl_l2_report"; then
    echo "PASS [opt-store-load-level2-report]"
  else
    echo "FAIL [opt-store-load-level2-report]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$sl_l2_report")"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [opt-store-load-reports]: codegen/report command failed"
  failures=$((failures + 1))
fi

lit_png="$TMP_DIR/lit_fold_fixture.png"
python3 - "$lit_png" <<'PY'
import colorsys
import struct
import zlib
import binascii
import sys

out_path = sys.argv[1]

def hsv_to_rgb(h, s, v):
    r, g, b = colorsys.hsv_to_rgb(h / 360.0, s / 100.0, v / 255.0)
    return int(round(r * 255)), int(round(g * 255)), int(round(b * 255))

def chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", binascii.crc32(tag + data) & 0xFFFFFFFF)

# DATA(6), DATA(7), ADD, OUT_NUM, HALT
px = [
    (6, 6, 6, 255),
    (7, 7, 7, 255),
    (*hsv_to_rgb(132, 100, 120), 255),  # Teal low-V => ADD
    (*hsv_to_rgb(324, 100, 120), 255),  # Pink low-V => OUT_NUM
    (0, 0, 0, 255),
]

w, h = len(px), 1
raw = bytearray([0])
for r, g, b, a in px:
    raw.extend([r, g, b, a])

ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
idat = zlib.compress(bytes(raw))

with open(out_path, "wb") as f:
    f.write(b"\x89PNG\r\n\x1a\n")
    f.write(chunk(b"IHDR", ihdr))
    f.write(chunk(b"IDAT", idat))
    f.write(chunk(b"IEND", b""))
PY

lit_l1_report="$TMP_DIR/lit_l1_report.out"
lit_l2_report="$TMP_DIR/lit_l2_report.out"

if "$GLINT_BIN" "$lit_png" --opt-level 1 --opt-report -o "$TMP_DIR/lit_l1.c" >"$lit_l1_report" 2>"$TMP_DIR/lit_l1.err" && \
   "$GLINT_BIN" "$lit_png" --opt-level 2 --opt-report -o "$TMP_DIR/lit_l2.c" >"$lit_l2_report" 2>"$TMP_DIR/lit_l2.err"; then
  if grep -q "OPT_REPORT: passes=3 changes=0 nops=0 dirs=0 lit=0 removed=0 dims=5x1->5x1" "$lit_l1_report"; then
    echo "PASS [opt-lit-level1-report]"
  else
    echo "FAIL [opt-lit-level1-report]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$lit_l1_report")"
    failures=$((failures + 1))
  fi

  if grep -q "OPT_REPORT: passes=6 changes=1 nops=0 dirs=0 lit=1 removed=0 dims=5x1->5x1" "$lit_l2_report"; then
    echo "PASS [opt-lit-level2-report]"
  else
    echo "FAIL [opt-lit-level2-report]: unexpected report values"
    echo "  got: $(tr '\n' ' ' < "$lit_l2_report")"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [opt-lit-reports]: codegen/report command failed"
  failures=$((failures + 1))
fi

if gcc "$TMP_DIR/lit_l2.c" -o "$TMP_DIR/lit_l2.bin" >/dev/null 2>"$TMP_DIR/lit_l2.gcc.err"; then
  lit_interp_out="$TMP_DIR/lit_interp.out"
  lit_interp_err="$TMP_DIR/lit_interp.err"
  lit_gen_out="$TMP_DIR/lit_gen.out"
  lit_gen_err="$TMP_DIR/lit_gen.err"
  lit_interp_exit=$(run_and_capture "$lit_interp_out" "$lit_interp_err" "$GLINT_BIN" "$lit_png")
  lit_gen_exit=$(run_and_capture "$lit_gen_out" "$lit_gen_err" "$TMP_DIR/lit_l2.bin")

  if [[ "$lit_interp_exit" == "$lit_gen_exit" ]] && cmp -s "$lit_interp_out" "$lit_gen_out" && cmp -s "$lit_interp_err" "$lit_gen_err"; then
    echo "PASS [opt-lit-level2-parity]"
  else
    echo "FAIL [opt-lit-level2-parity]: output or exit mismatch"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [opt-lit-level2-parity]: generated C compilation failed"
  failures=$((failures + 1))
fi

if gcc "$TMP_DIR/store_load_l2.c" -o "$TMP_DIR/store_load_l2.bin" >/dev/null 2>"$TMP_DIR/store_load_l2.gcc.err"; then
  sl_interp_out="$TMP_DIR/store_load_interp.out"
  sl_interp_err="$TMP_DIR/store_load_interp.err"
  sl_gen_out="$TMP_DIR/store_load_gen.out"
  sl_gen_err="$TMP_DIR/store_load_gen.err"
  sl_interp_exit=$(run_and_capture "$sl_interp_out" "$sl_interp_err" "$GLINT_BIN" "$store_load_png")
  sl_gen_exit=$(run_and_capture "$sl_gen_out" "$sl_gen_err" "$TMP_DIR/store_load_l2.bin")

  if [[ "$sl_interp_exit" == "$sl_gen_exit" ]] && cmp -s "$sl_interp_out" "$sl_gen_out" && cmp -s "$sl_interp_err" "$sl_gen_err"; then
    echo "PASS [opt-store-load-level2-parity]"
  else
    echo "FAIL [opt-store-load-level2-parity]: output or exit mismatch"
    failures=$((failures + 1))
  fi
else
  echo "FAIL [opt-store-load-level2-parity]: generated C compilation failed"
  failures=$((failures + 1))
fi

if [[ "$failures" -ne 0 ]]; then
  printf '\nParity tests failed: %s\n' "$failures"
  exit 1
fi

printf '\nAll parity tests passed\n'
