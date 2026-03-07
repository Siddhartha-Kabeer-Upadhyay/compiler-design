# hsv.c — Code Explanation

This file converts RGB color values to HSV (Hue, Saturation, Value).

---

## Why HSV?

Glint uses HSV because:
- **Hue** determines the instruction type (0-360°)
- **Saturation** distinguishes code from data (≤20% = data)
- **Value** selects instruction variant (< 128 vs ≥ 128)

RGB is how images store color. HSV is how Glint interprets meaning.

---

## The HSV Struct

```c
typedef struct {
    int h;  // Hue: 0-360
    int s;  // Saturation: 0-100
    int v;  // Value: 0-100
} HSV;
```

| Field | Range | Meaning |
|-------|-------|---------|
| `h` | 0-360 | Color angle on color wheel |
| `s` | 0-100 | Color intensity (0=gray, 100=vivid) |
| `v` | 0-100 | Brightness (0=black, 100=bright) |

---

## The Conversion Function

```c
HSV rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b)
```

**Input**: RGB values (0-255 each)
**Output**: HSV struct with converted values

---

## Step 1: Find Max and Min

```c
max = r;
if (g > max) max = g;
if (b > max) max = b;

min = r;
if (g < min) min = g;
if (b < min) min = b;

diff = max - min;
```

| Variable | Purpose |
|----------|---------|
| `max` | Brightest channel |
| `min` | Darkest channel |
| `diff` | Range between them |

**Why?**
- `max` determines Value
- `diff / max` determines Saturation
- Which channel is `max` determines Hue

---

## Step 2: Calculate Value

```c
result.v = (max * 100) / 255;
```

| RGB Max | HSV Value |
|---------|-----------|
| 0 | 0 (black) |
| 127 | 49 |
| 255 | 100 (full bright) |

Value is simply "how bright is the brightest channel?"

---

## Step 3: Calculate Saturation

```c
if (max == 0) {
    result.s = 0;
} else {
    result.s = (diff * 100) / max;
}
```

| Condition | Result |
|-----------|--------|
| `max == 0` | Black pixel, saturation undefined (use 0) |
| `diff == 0` | All channels equal = gray = 0% saturation |
| `diff == max` | One channel is 0 = 100% saturation |

Saturation = "how colorful vs gray?"

---

## Step 4: Calculate Hue

```c
if (diff == 0) {
    result.h = 0;
} else if (max == r) {
    result.h = (60 * ((g - b) * 100 / diff) + 36000) % 36000 / 100;
} else if (max == g) {
    result.h = (60 * ((b - r) * 100 / diff) + 12000) / 100;
} else {
    result.h = (60 * ((r - g) * 100 / diff) + 24000) / 100;
}
```

| Condition | Hue Range | Color |
|-----------|-----------|-------|
| `diff == 0` | 0 (undefined) | Gray |
| `max == r` | 0° or 360° | Red |
| `max == g` | 120° | Green |
| `max == b` | 240° | Blue |

**The math:**
- Color wheel is 360°
- Red at 0°, Green at 120°, Blue at 240°
- Formula interpolates between these based on secondary channel

**Why the weird numbers?**
- `* 100 / diff` — Integer math to avoid floats
- `+ 36000` — Ensure positive before modulo
- `% 36000 / 100` — Get final 0-360 result

---

## Example Conversions

| RGB | HSV | Color |
|-----|-----|-------|
| (255, 0, 0) | H=0, S=100, V=100 | Pure red |
| (0, 255, 0) | H=120, S=100, V=100 | Pure green |
| (0, 0, 255) | H=240, S=100, V=100 | Pure blue |
| (255, 255, 255) | H=0, S=0, V=100 | White |
| (0, 0, 0) | H=0, S=0, V=0 | Black |
| (128, 128, 128) | H=0, S=0, V=50 | Gray |
| (255, 128, 0) | H=30, S=100, V=100 | Orange |

---

## How Glint Uses HSV

| HSV Property | Glint Meaning |
|--------------|---------------|
| S ≤ 20 | DATA pixel — push V to stack |
| S > 20 | CODE pixel — execute instruction |
| H (0-360) | Which instruction (15 hue slots) |
| V < 128 | Base instruction variant |
| V ≥ 128 | Alternate instruction variant |

---

## Glossary

| Term | Definition |
|------|------------|
| **Hue** | Color angle (0-360°), red=0, green=120, blue=240 |
| **Saturation** | Color intensity (0=gray, 100=vivid) |
| **Value** | Brightness (0=black, 100=full) |
| **Color wheel** | Circular arrangement of colors by hue |
| **Integer math** | Math using only whole numbers (no decimals) |