# Glint

A transpiled esoteric programming language where source code is an image and output is C.

---

## What is Glint exactly?

- **Source code** = PNG image
- **Target code** = C
- **Instructions** = HSV color values of pixels
- **Execution** = 2D pointer movement through the image
- **Memory** = Stack + 3 registers (A, B, C)
- **Image Reading** = ``stb_image.h`` library is used for that specific job

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

---

## Execution Model

1. Pointer starts at `(0, 0)`, direction = `RIGHT`
2. Read pixel at current position
3. Convert RGB → HSV
4. If `S ≤ 20%`: Push `V` onto stack
5. Else: Execute instruction from hue table (modified by V)
6. Move pointer one step in current direction
7. If out of bounds OR black pixel (`S=0, V=0`) → **HALT**
8. Repeat from step 2

---

## Memory Model

```
┌─────────────────────────────────────┐
│              STACK                  │
│  [bottom] .... [top]                │
│  Grows upward, standard operations  │
└─────────────────────────────────────┘

┌─────────┬─────────┐
│  REG A  │  REG B  │
│  (int)  │  (int)  │
└─────────┴─────────┘
```

---

## Safety Features

| Situation | Behavior |
|-----------|----------|
| Stack underflow | HALT with error: `"Stack underflow at (x,y)"` |
| Stack overflow | HALT with error: `"Stack overflow at (x,y)"` |
| Division by zero | HALT with error: `"Division by zero at (x,y)"` |
| Modulo by zero | HALT with error: `"Modulo by zero at (x,y)"` |
| Out of bounds | Clean HALT (normal exit) |
| Black pixel entered | Clean HALT (normal exit) |

---

## Building & Running

```bash
# Compile Glint source image to C
glint input.png -o output.c

# Compile the generated C code
gcc output.c -o program

# Run the program
./program
```

---

## Examples

### Example 1: Add Two Numbers

**Input**: `3`, `2`  
**Output**: `5`

```
┌─────────┬─────────┬─────────┬─────────┬─────────┐
│ Magenta │ Magenta │  Teal   │  Pink   │  Black  │
│ IN(num) │ IN(num) │  ADD    │ OUT(num)│  HALT   │
└─────────┴─────────┴─────────┴─────────┴─────────┘
```

Execution:
1. `IN` → Read 3, stack: `[3]`
2. `IN` → Read 2, stack: `[3, 2]`
3. `ADD` → Pop both, push sum, stack: `[5]`
4. `OUT` → Print `5`
5. Black pixel → HALT

---

## Project Structure (Subject to Change)

```
glint/
├── src/
│   ├── main.c           # CLI entry point
│   ├── image.c          # PNG loading (using stb_image)
│   ├── image.h
│   ├── hsv.c            # RGB → HSV conversion
│   ├── hsv.h
│   ├── instruction.c    # HSV → Instruction mapping
│   ├── instruction.h
│   ├── grid.c           # 2D instruction grid
│   ├── grid.h
│   ├── tracer.c         # Execution path tracing
│   ├── tracer.h
│   ├── codegen.c        # C code generation
│   ├── codegen.h
│   └── glint.h          # Main header with types/enums
├── external/
│   └── stb_image.h      # Single-header image loading
├── examples/
│   ├── add.png
│   ├── hello.png
│   └── loop.png
├── Makefile
├── LICENSE
└── README.md
```
