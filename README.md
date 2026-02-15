# Glint

A compiled esoteric programming language where source code is an image.

---

## What is Glint exactly?

Glint is a visual, compiled esoteric language where:

- **Source code** = PNG image
- **Instructions** = HSV color values of pixels
- **Execution** = 2D pointer movement through the image
- **Memory** = Stack + 3 registers (A, B, C)
- **Image Reading** = ``stb_image.h`` library is used for that specific job

---

## Core Concept

### Saturation Determines Meaning

| Saturation | Meaning |
|------------|---------|
| **S ≤ 20%** | **DATA**: Push brightness (V, 0-255) onto stack |
| **S > 20%** | **CODE**: Execute instruction based on Hue |

### Value Modifies Instructions

For CODE pixels (S > 20%), the Value component selects between two instruction variants:

| Value | Effect |
|-------|--------|
| **V < 128** | Base instruction |
| **V ≥ 128** | Alternate instruction |

* An exception applies to this condition, as listed below;

| Pixel | Detection | Effect |
|-------|-------|--------|
| **BLACK** | H = 0, S=0, V=0, A=255 | exit(0) |
| **TRANSPARENT or NULL** | A=0 | exit(1) |

---

## Instruction Set

| Hue Range | Color | V < 128 | V ≥ 128 | Stack Effect |
|-----------|-------|---------|---------|--------------|
| 0-17° | Red | RIGHT | RIGHT + SKIP | — |
| 18-35° | Orange | DOWN | DOWN + SKIP | — |
| 36-53° | Yellow | LEFT | LEFT + SKIP | — |
| 54-71° | Lime | UP | UP + SKIP | — |
| 72-89° | Green | POP | SWAP | `[a]→[]` / `[a,b]→[b,a]` |
| 90-107° | Teal | ADD | SUB | `[a,b]→[a+b]` / `[a,b]→[a-b]` |
| 108-125° | Cyan | MUL | DIV | `[a,b]→[a*b]` / `[a,b]→[a/b]` |
| 126-143° | Sky | MOD | NEG | `[a,b]→[a%b]` / `[a]→[-a]` |
| 144-161° | Blue | DUP | OVER | `[a]→[a,a]` / `[a,b]→[a,b,a]` |
| 162-179° | Indigo | STORE A | LOAD A | Stack ↔ Register A |
| 180-197° | Purple | STORE B | LOAD B | Stack ↔ Register B |
| 198-215° | Violet | STORE C | LOAD C | Stack ↔ Register C |
| 216-233° | Magenta | JNZ | JZ | `[a]→[]`, turn right if condition met |
| 234-251° | Pink | JPOS | JNEG | `[a]→[]`, turn right if condition met |
| 252-269° | Rose | JNZ (peek) | JZ (peek) | `[a]→[a]`, non-destructive check |
| 270-287° | Salmon | IN (num) | IN (char) | `[]→[input]` |
| 288-305° | Coral | OUT (num) | OUT (char) | `[a]→[]` |
| 306-323° | Peach | INC | DEC | `[a]→[a+1]` / `[a]→[a-1]` |
| 324-341° | Gold | ROT | ROTR | `[a,b,c]→[b,c,a]` / `→[c,a,b]` |
| 342-359° | Crimson | SHL | SHR | `[a,b]→[a<<b]` / `[a,b]→[a>>b]` |

---

## Quick Reference

```
DIRECTION (0-71°):
┌─────────┬─────────┬─────────┬─────────┐
│   RED   │ ORANGE  │ YELLOW  │  LIME   │
│  0-17°  │ 18-35°  │ 36-53°  │ 54-71°  │
│    →    │    ↓    │    ←    │    ↑    │
└─────────┴─────────┴─────────┴─────────┘

STACK (72-89°, 144-161°, 324-341°):
┌─────────┬─────────┬─────────┐
│  GREEN  │  BLUE   │  GOLD   │
│ POP/SWAP│ DUP/OVER│ ROT/ROTR│
└─────────┴─────────┴─────────┘

MATH (90-143°):
┌─────────┬─────────┬─────────┐
│  TEAL   │  CYAN   │   SKY   │
│ ADD/SUB │ MUL/DIV │ MOD/NEG │
└─────────┴─────────┴─────────┘

REGISTERS (162-215°):
┌─────────┬─────────┬─────────┐
│  INDIGO │  PURPLE │  VIOLET │
│  REG A  │  REG B  │  REG C  │
└─────────┴─────────┴─────────┘

CONDITIONALS (216-269°):
┌─────────┬─────────┬─────────┐
│ MAGENTA │  PINK   │  ROSE   │
│ JNZ/JZ  │JPOS/JNEG│ (peek)  │
└─────────┴─────────┴─────────┘

I/O (270-305°):
┌─────────┬─────────┐
│ SALMON  │  CORAL  │
│   IN    │   OUT   │
└─────────┴─────────┘

CONTROL & BITWISE (306-359°):
┌─────────┬─────────┬─────────┐
│  PEACH  │  GOLD   │ CRIMSON │
│NOP/HALT │ROT/ROTR │ SHL/SHR │
└─────────┴─────────┴─────────┘

DATA (any hue, S ≤ 20%):
┌─────────────────────────────┐
│  Grayscale pixels = PUSH V  │
│  Black (0) to White (255)   │
└─────────────────────────────┘
```

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

┌─────────┬─────────┬─────────┐
│  REG A  │  REG B  │  REG C  │
│  (int)  │  (int)  │  (int)  │
└─────────┴─────────┴─────────┘
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
│ Salmon  │ Salmon  │  Teal   │  Coral  │  Black  │
│ IN(num) │ IN(num) │   ADD   │OUT(num) │  HALT   │
└─────────┴─────────┴─────────┴─────────┴─────────┘
```

Execution:
1. `IN` → Read 3, stack: `[3]`
2. `IN` → Read 2, stack: `[3, 2]`
3. `ADD` → Pop both, push sum, stack: `[5]`
4. `OUT` → Print `5`
5. Black pixel → HALT

---

### Example 2: Print "Hi"

**Output**: `Hi`

```
┌──────────┬──────────┬──────────┬──────────┐
│ Gray     │ Coral    │ Gray     │ Coral    │
│ S=10%    │ V≥128    │ S=10%    │ V≥128    │
│ V=72     │ OUT(chr) │ V=105    │ OUT(chr) │
│ PUSH 72  │          │ PUSH 105 │          │
└──────────┴──────────┴──────────┴──────────┘
```

Execution:
1. Gray V=72 → Push 72 (ASCII 'H'), stack: `[72]`
2. Coral V≥128 → OUT as ASCII, print `H`, stack: `[]`
3. Gray V=105 → Push 105 (ASCII 'i'), stack: `[105]`
4. Coral V≥128 → OUT as ASCII, print `i`, stack: `[]`

---

## Project Structure

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
