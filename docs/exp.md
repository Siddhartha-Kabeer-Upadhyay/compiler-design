# main.c — Code Explanation

This file loads a PNG image and prints the RGBA and HSV values of every pixel.

---

## Includes

```c
#include <stdio.h>
#include <stdlib.h>
```

| Header | Purpose |
|--------|---------|
| `stdio.h` | Standard I/O functions: `printf`, `fprintf` |
| `stdlib.h` | General utilities: `exit` codes, memory |

---

## stb_image Setup

```c
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

| Line | Purpose |
|------|---------|
| `#define STB_IMAGE_IMPLEMENTATION` | Tells stb_image to include the actual code, not just declarations. Only do this in ONE .c file. |
| `#include "stb_image.h"` | Includes the single-header image loading library |

**Why stb_image?**
- Single header file (no linking hassle)
- Supports PNG, JPG, BMP, GIF
- Public domain / MIT license
- Simple API

---

## HSV Include

```c
#include "hsv.h"
```

| What It Does | How It Works |
|--------------|--------------|
| Declares `rgb_to_hsv` function | Header only tells compiler "this function exists" |
| Declares `HSV` struct | Lets us use the HSV type |

**Important**: The actual code for `rgb_to_hsv` is in `hsv.c`. The Makefile tells GCC to compile both files and link them together.

```
Makefile: SRC = src/main.c src/hsv.c
                           ^^^^^^^^^^
                           This is why it works
```

Without `hsv.c` in the Makefile, you'd get: `undefined reference to 'rgb_to_hsv'`

---

## Main Function

```c
int main(int argc, char *argv[])
```

| Parameter | Purpose |
|-----------|---------|
| `argc` | Argument count — how many command-line arguments |
| `argv` | Argument vector — array of argument strings |

**Example**: `./glint test.png`
- `argc` = 2
- `argv[0]` = `"./glint"`
- `argv[1]` = `"test.png"`

---

## Argument Check

```c
if (argc < 2) {
    fprintf(stderr, "Usage: %s <image.png>\n", argv[0]);
    return 1;
}
```

| Part | Purpose |
|------|---------|
| `argc < 2` | Check if user provided a filename |
| `fprintf(stderr, ...)` | Print error to stderr (not stdout) |
| `return 1` | Exit with error code 1 |

---

## Loading the Image

```c
int width, height, channels;
unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 4);
```

| Variable | Purpose |
|----------|---------|
| `width` | Image width in pixels (filled by stbi_load) |
| `height` | Image height in pixels (filled by stbi_load) |
| `channels` | Original channel count (1=gray, 3=RGB, 4=RGBA) |
| `img` | Pointer to pixel data array |

**stbi_load parameters:**

| Parameter | Value | Purpose |
|-----------|-------|---------|
| `argv[1]` | Filename | Path to PNG file |
| `&width` | Pointer | Where to store width |
| `&height` | Pointer | Where to store height |
| `&channels` | Pointer | Where to store original channel count |
| `4` | Desired channels | Force output to RGBA (4 channels) |

**Why force 4 channels?**
- Consistent format regardless of source image
- Always have alpha channel for transparency detection
- Simpler indexing math

---

## Error Check

```c
if (!img) {
    fprintf(stderr, "Error: Could not load image '%s'\n", argv[1]);
    return 1;
}
```

| Part | Purpose |
|------|---------|
| `!img` | Check if stbi_load returned NULL (failed) |
| `fprintf(stderr, ...)` | Print error message |
| `return 1` | Exit with error code |

**Possible failure reasons:**
- File not found
- File is not a valid image
- File is corrupted
- Out of memory

---

## Print Image Info

```c
printf("Loaded: %s\n", argv[1]);
printf("Size: %d x %d\n", width, height);
printf("Channels: %d (forced to 4)\n\n", channels);
```

Prints basic info about the loaded image.

---

## Pixel Loop

```c
for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
```

| Loop | Purpose |
|------|---------|
| Outer (`y`) | Iterate through rows (top to bottom) |
| Inner (`x`) | Iterate through columns (left to right) |

**Order**: Row-major (standard for images)

```
(0,0) (1,0) (2,0) ...
(0,1) (1,1) (2,1) ...
(0,2) (1,2) (2,2) ...
```

---

## Pixel Index Calculation

```c
int idx = (y * width + x) * 4;
```

**Memory layout**: Pixels stored as flat array.

| Formula Part | Purpose |
|--------------|---------|
| `y * width` | Skip to correct row |
| `+ x` | Move to correct column |
| `* 4` | Each pixel has 4 bytes (R, G, B, A) |

**Example** (3x2 image, getting pixel at (1, 1)):
```
Row 0: [R G B A] [R G B A] [R G B A]
Row 1: [R G B A] [R G B A] [R G B A]
                 ^^^^^^^^^
                 idx = (1 * 3 + 1) * 4 = 16
```

---

## Extract RGBA Values

```c
unsigned char r = img[idx + 0];
unsigned char g = img[idx + 1];
unsigned char b = img[idx + 2];
unsigned char a = img[idx + 3];
```

| Offset | Channel | Range |
|--------|---------|-------|
| `idx + 0` | Red | 0-255 |
| `idx + 1` | Green | 0-255 |
| `idx + 2` | Blue | 0-255 |
| `idx + 3` | Alpha | 0-255 (0=transparent, 255=opaque) |

---

## Convert to HSV

```c
HSV hsv = rgb_to_hsv(r, g, b);
```

| What Happens | Where |
|--------------|-------|
| Function declared | `hsv.h` |
| Function defined | `hsv.c` |
| Function called | `main.c` |
| Linked together | `Makefile` |

---

## Print Pixel

```c
printf("(%d,%d): R=%3d G=%3d B=%3d A=%3d | H=%3d S=%3d V=%3d\n",
       x, y, r, g, b, a, hsv.h, hsv.s, hsv.v);
```

| Format | Purpose |
|--------|---------|
| `%d` | Integer |
| `%3d` | Integer, padded to 3 characters (alignment) |

**Example output:**
```
(0,0): R=255 G=  0 B=  0 A=255 | H=  0 S=100 V=100
(1,0): R=  0 G=255 B=  0 A=255 | H=120 S=100 V=100
```

---

## Cleanup

```c
stbi_image_free(img);
return 0;
```

| Line | Purpose |
|------|---------|
| `stbi_image_free(img)` | Free allocated memory (prevent leak) |
| `return 0` | Exit with success code |

---

## Test Output

```
siddhartha@MSI-Desktop:~/Glint/v1$ ./glint image.png
Loaded:         image.png
Size:   4 x 4
Channels:       3

Pixels:
(0,0):  R=  0 G=  0 B=  0 A=255
(1,0):  R=255 G=255 B=255 A=255
(2,0):  R=255 G=255 B=255 A=255
(3,0):  R=236 G= 28 B= 36 A=255
(0,1):  R=255 G=255 B=255 A=255
(1,1):  R=  0 G=  0 B=  0 A=255
(2,1):  R=  0 G=168 B=243 A=255
(3,1):  R=255 G=255 B=255 A=255
(0,2):  R=255 G=255 B=255 A=255
(1,2):  R=  1 G=168 B=243 A=255
(2,2):  R=  2 G=  2 B=  2 A=255
(3,2):  R=255 G=255 B=255 A=255
(0,3):  R=236 G= 28 B= 36 A=255
(1,3):  R=255 G=255 B=255 A=255
(2,3):  R=255 G=255 B=255 A=255
(3,3):  R=  4 G=  4 B=  4 A=255
```

**Observations from test image:**
- 4x4 pixel image
- Original had 3 channels (RGB), forced to 4 (RGBA)
- Alpha is 255 for all (fully opaque, no transparency in source)
- Contains: black pixels, white pixels, red pixels, cyan pixels, gray pixels

---

## Glossary

| Term | Definition |
|------|------------|
| **RGBA** | Red, Green, Blue, Alpha — 4-channel color format |
| **HSV** | Hue, Saturation, Value — color model used by Glint |
| **Alpha** | Transparency channel (0=invisible, 255=solid) |
| **Pixel** | Single point in an image with color values |
| **Channel** | One component of color (R, G, B, or A) |
| **Row-major** | Memory layout where rows are stored sequentially |
| **stb_image** | Single-header library for loading images |
| **Header file (.h)** | Contains declarations (function signatures, types) |
| **Source file (.c)** | Contains definitions (actual code) |
| **Linker** | Combines compiled files, connects function calls to code |
| **stderr** | Standard error stream for error messages |
| **stdout** | Standard output stream for normal output |