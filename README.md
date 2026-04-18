# Glint Extended

A visual esoteric language where source code is a PNG image and output is generated C.

---

## What is Glint Extended exactly?

- **Source code** = PNG image
- **Target code** = C
- **Instructions** = HSV color values of pixels
- **Execution** = 2D pointer movement through the image
- **Memory** = Stack + 4 registers (A, B, C, D)
- **Image Reading** = `stb_image.h` library

### Glint Extended treats an image as executable space:

- Hue determines instruction category
- Saturation determines data vs code
- Value modifies instruction behavior
- Alpha can terminate execution

---

## Core Concept

### Combination Determines Meaning

| Condition | Type | Behavior |
|-----------|------|----------|
| Alpha = 0 | Transparent | Error halt, `exit(1)` |
| R=0, G=0, B=0 | Black | Clean halt, `exit(0)` |
| S ≤ 20% | Data | Push V (0-255) onto stack |
| 20% < S ≤ 60% | Base code | Execute base bank instruction |
| S > 60% | EX code | Execute Glint Extended bank instruction |

### Value Modifies Instructions

For CODE pixels (`S > 20%`), Value selects between two variants:

| Value | Effect |
|-------|--------|
| **V < 128** | Base instruction |
| **V ≥ 128** | Alternate instruction |

---

## Instruction Set

### Base Bank (`20% < S <= 60%`)

| Hue | Color | V < 128 | V ≥ 128 | Stack Effect | Category |
|-----|-------|---------|---------|--------------|----------|
| 0-24° | Red | RIGHT | RIGHT_SKIP | — | Direction |
| 24-48° | Orange | DOWN | DOWN_SKIP | — | Direction |
| 48-72° | Yellow | LEFT | LEFT_SKIP | — | Direction |
| 72-96° | Lime | UP | UP_SKIP | — | Direction |
| 96-120° | Green | POP | SWAP | `[a]→[]` / `[a,b]→[b,a]` | Stack |
| 120-144° | Teal | ADD | SUB | `[a,b]→[a+b]` / `[a,b]→[a-b]` | Math |
| 144-168° | Cyan | MUL | DIV | `[a,b]→[a*b]` / `[a,b]→[a/b]` | Math |
| 168-192° | Sky | MOD | NEG | `[a,b]→[a%b]` / `[a]→[-a]` | Math |
| 192-216° | Blue | INC | DEC | `[a]→[a+1]` / `[a]→[a-1]` | Math |
| 216-240° | Indigo | STORE_A | LOAD_A | `[a]→[]` / `[]→[A]` | Register |
| 240-264° | Violet | STORE_B | LOAD_B | `[a]→[]` / `[]→[B]` | Register |
| 264-288° | Purple | JNZ | JZ | `[a]→[]` | Conditional |
| 288-312° | Magenta | IN_NUM | IN_CHR | `[]→[input]` | I/O |
| 312-336° | Pink | OUT_NUM | OUT_CHR | `[a]→[]` | I/O |
| 336-360° | Rose | NOP | DEBUG | — | Control |

### EX Bank (`S > 60%`)

Pairing follows hue rows across banks (`A <-> C`, `B <-> D`).

| Hue | Color | V < 128 | V ≥ 128 | Stack Effect | Category |
|-----|-------|---------|----------|--------------|----------|
| 0-24° | Red | TRAP_00 | TRAP_01 | — | Trap |
| 24-48° | Orange | TRAP_02 | TRAP_03 | — | Trap |
| 48-72° | Yellow | TRAP_04 | TRAP_05 | — | Trap |
| 72-96° | Lime | TRAP_06 | TRAP_07 | — | Trap |
| 96-120° | Green | DUP | OVER | `[a]→[a,a]` / `[a,b]→[a,b,a]` | Stack |
| 120-144° | Teal | AND | OR | `[a,b]→[a&b]` / `[a,b]→[a\|b]` | Bitwise |
| 144-168° | Cyan | NOT | XOR | `[a]→[~a]` / `[a,b]→[a^b]` | Bitwise |
| 168-192° | Sky | ROT | ROTR | `[a,b,c]→[b,c,a]` / `[a,b,c]→[c,a,b]` | Stack |
| 192-216° | Blue | READ | WRITE | `[x,y]→[v]` / `[v,x,y]→[]` | Pixel Mem |
| 216-240° | Indigo | STORE_C | LOAD_C | `[a]→[]` / `[]→[C]` | Register |
| 240-264° | Violet | STORE_D | LOAD_D | `[a]→[]` / `[]→[D]` | Register |
| 264-288° | Purple | JGT | JLT | `[a,b]→[]` | Flow |
| 288-312° | Magenta | CALL | RET | `[...]→[...]` | Call |
| 312-336° | Pink | TRAP_12 | TRAP_13 | — | Trap |
| 336-360° | Rose | DEPTH | CLEAR | `[]→[sp]` / `[...]→[]` | Stack State |

---

## Execution Model

1. Pointer starts at `(0, 0)`, direction = `RIGHT`
2. Read pixel at current position
3. Convert RGB → HSV
4. If `S ≤ 20%`: Push `V` onto stack
5. Else: Execute instruction from hue table (modified by V)
6. Move pointer one step in current direction
7. If black pixel (`R=0,G=0,B=0`) → **Clean HALT**
8. If out of bounds OR transparent pixel (`A=0`) → **Error HALT**
9. Repeat from step 2

---

## Memory Model

### Stack
- LIFO
- Signed integer values
- Grows upward, standard operations

### Registers
- A (int)
- B (int)
- C (int)
- D (int)

---

## Safety Features

| Situation | Behavior |
|-----------|----------|
| Stack underflow | HALT with error: `"Stack underflow at (x,y)"` |
| Stack overflow | HALT with error: `"Stack overflow at (x,y)"` |
| Trap opcode | HALT with error: `"ERR_TRAP"` |
| Division by zero | HALT with error: `"Division by zero at (x,y)"` |
| Modulo by zero | HALT with error: `"Modulo by zero at (x,y)"` |
| Out of bounds | HALT with error: `"Reached Out of Bounds"` |
| Black pixel entered | Clean HALT |

---

## Building & Running

```bash
# run the make file first
make

# Run Glint directly (default run mode)
./glint tests/image3.png

# Trace execution with optional step limit
./glint tests/image3.png --trace 1000

# Dump pixel decode table
./glint tests/image3.png --dump

# Generate C from a Glint source image
./glint tests/image3.png -o output.c

# Generate C with optimization passes enabled
./glint tests/image3.png --opt -o output_opt.c

# Explicit optimization level (0 = off, 1 = current safe passes)
./glint tests/image3.png --opt-level 1 -o output_opt.c

# Reserved level for future advanced passes (currently behaves like level 1)
./glint tests/image3.png --opt-level 2 -o output_opt2.c

# Emit optimization report to stdout during code generation
./glint tests/image3.png --opt --opt-report -o output_opt.c

# Compile the generated C code
gcc output.c -o program

# Run the program
./program
```

### Codegen Notes

- `--opt` is only valid with `-o` and currently maps to safe optimization passes.
- `--opt-level` supports `0`, `1`, `2` (`2` runs one conservative direction-canonicalization round).
- Base-bank programs use optimized static decode codegen path.
- Glint Extended programs use generated runtime decode-on-step with mutable RGBA memory.

### Support Matrix

| Capability | Interpreter (`--run` / `--trace`) | Codegen (`-o`) |
|------------|-------------------------------------|----------------|
| Base bank (`20 < S <= 60`) | Supported | Supported |
| Extended bank (`S > 60`) | Supported | Supported |
| `CALL` / `RET` teleport routing | Supported | Supported |
| `READ` / `WRITE` mutable image memory | Supported | Supported |
| `A/B/C/D` registers | Supported | Supported |
| Trap opcode error (`ERR_TRAP`) | Supported | Supported |

- `--opt-report` prints stats in this format:
  - `OPT_REPORT: passes=<n> changes=<n> nops=<n> dirs=<n> lit=<n> removed=<n> dims=<w>x<h>-><w>x<h>`
  - `passes`: total passes run
  - `changes`: total optimization changes
  - `nops`: instructions canonicalized to `NOP`
  - `dirs`: redundant direction writes canonicalized to `NOP`
  - `lit`: literal arithmetic folds applied
  - `removed`: cells removed by reachability crop
  - `dims`: dimensions before and after optimization

### Optimization Report Stability

- The `OPT_REPORT` line format and field order are treated as stable test contract.
- If report fields or ordering change, update tests and README in the same patch.
- Current stable order: `passes`, `changes`, `nops`, `dirs`, `lit`, `removed`, `dims`.

---

## Examples

### Example 1: Add Two Numbers

**Input**: `3`, `2`  
**Output**: `5`

```
Magenta	: IN (num)
Magenta	: IN (num)
Teal	: ADD
Black	: HALT
```

Execution:
1. `IN` → Read 3, stack: `[3]`
2. `IN` → Read 2, stack: `[3, 2]`
3. `ADD` → Pop both, push sum, stack: `[5]`
4. `OUT` → Print `5`
5. Black pixel → HALT

---

## Architecture

- `instruction.*`: pixel classification and opcode decoding from RGBA/HSV.
- `runtime.*`: stack/register/math/io semantics and runtime status handling.
- `tracer.*`: pointer movement application from route effects and bounds errors.
- `main.c`: CLI parsing, mode orchestration (`run`, `trace`, `dump`, `codegen`), exit codes.
- `ir.*`: decoded array representation consumed by optimizer and codegen.
- `opt.*`: optimization passes by level (`0/1/2`) and optimization stats.
- `codegen.*`: emits standalone C (optimized IR path for base bank, runtime decode path for extended bank).

Execution paths:

- Interpreter path: `main -> decode -> runtime -> tracer`.
- Codegen path: `main -> ir -> opt -> codegen`.

Test paths:

- Unit semantics are covered by the local unit test binary target in `make test`.
- Parity and CLI/report checks are covered by the local parity test step in `make test`.
- CI gate: `.github/workflows/ci.yml` runs `make` and `make test`.

---

## Known Limitations

- Optimizer passes are tuned for safety-first behavior and are intentionally conservative.
- Some optimization report counters are aggregate and not split by every micro-transform.
- Generated C follows VM-style runtime execution and prioritizes semantic parity over aggressive lowering.

## Quality Checklist

- `make` passes.
- `make test` passes.
- Interpreter and generated C parity is green for baseline and extended fixtures.

---
