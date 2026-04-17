# Glint

A transpiled esoteric programming language where source code is a PNG image and the output is generated C code.

---

## What is Glint exactly?

- **Source code** = PNG image
- **Target code** = C
- **Instructions** = HSV color values of pixels
- **Execution** = 2D pointer movement through the image
- **Memory** = Stack + 2 registers (A, B)
- **Image Reading** = ``stb_image.h`` stb library

### Glint treats an image as executable space:

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
| S > 20% | Code | Execute instruction based on hue |

### Value Modifies Instructions

For CODE pixels (S > 20%), the Value component selects between two instruction variants:

| Value | Effect |
|-------|--------|
| **V < 128** | Base instruction |
| **V ≥ 128** | Alternate instruction |

---

## Instruction Set

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
- Integer values (0–255)
- Grows upward, standard operations

### Registers
- A (int)
- B (int)

---

## Safety Features

| Situation | Behavior |
|-----------|----------|
| Stack underflow | HALT with error: `"Stack underflow at (x,y)"` |
| Stack overflow | HALT with error: `"Stack overflow at (x,y)"` |
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
./glint input.png

# Trace execution with optional step limit
./glint input.png --trace 1000

# Dump pixel decode table
./glint input.png --dump

# Generate C from a Glint source image
./glint input.png -o output.c

# Generate C with optimization passes enabled
./glint input.png --opt -o output_opt.c

# Explicit optimization level (0 = off, 1 = current safe passes)
./glint input.png --opt-level 1 -o output_opt.c

# Reserved level for future advanced passes (currently behaves like level 1)
./glint input.png --opt-level 2 -o output_opt2.c

# Emit optimization report to stdout during code generation
./glint input.png --opt --opt-report -o output_opt.c

# Compile the generated C code
gcc output.c -o program

# Run the program
./program
```

### Codegen Notes

- `--opt` is only valid with `-o` and currently maps to safe optimization passes.
- `--opt-level` supports `0`, `1`, `2` (`2` is a placeholder for future advanced passes).
- `--opt-report` prints stats in this format:
  - `OPT_REPORT: passes=<n> changes=<n> nops=<n> dirs=<n> removed=<n> dims=<w>x<h>-><w>x<h>`
  - `passes`: total passes run
  - `changes`: total optimization changes
  - `nops`: instructions canonicalized to `NOP`
  - `dirs`: redundant direction writes canonicalized to `NOP`
  - `removed`: cells removed by reachability crop
  - `dims`: dimensions before and after optimization

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
