#include "codegen.h"
#include "instruction.h"
#include "ir.h"

#include <stdio.h>

#define TRY_IO(x) do { if ((x) < 0) return 0; } while (0)
#define TRY_OK(x) do { if (!(x)) return 0; } while (0)

static int emit_str(FILE *out, const char *s)
{ return fputs(s, out) >= 0; }

static int emit_int_array(FILE *out, const int *arr, int n)
{
    for (int i = 0; i < n; i++)
        TRY_IO(fprintf(out, "  %d%s", arr[i], (i + 1 == n) ? "\n" : ","));
    return 1;
}

static int emit_u8_array(FILE *out, const unsigned char *arr, int n)
{
    for (int i = 0; i < n; i++)
        TRY_IO(fprintf(out, "  %u%s", (unsigned)arr[i], (i + 1 == n) ? "\n" : ","));
    return 1;
}

static int emit_head(FILE *out, int w, int h, int n)
{
    if (fprintf(out,
                "#include <stdio.h>\n"
                "#include <stdlib.h>\n\n"
                "#define WIDTH %d\n"
                "#define HEIGHT %d\n"
                "#define PIXEL_COUNT %d\n"
                "#define STACK_MAX 1024\n\n"
                "static const unsigned char pixel_type[PIXEL_COUNT] = {\n",
                w, h, n) < 0)
        return 0;

    return 1;
}

static int emit_tabs(FILE *out, const unsigned char *type, const int *instr,
                     const int *data, int w, int h)
{
    int n = w * h;

    if (!emit_head(out, w, h, n)) return 0;

    for (int i = 0; i < n; i++)
        TRY_IO(fprintf(out, "  %d%s", (int)type[i], (i + 1 == n) ? "\n" : ","));

    if (!emit_str(out, "};\n\nstatic const int pixel_instr[PIXEL_COUNT] = {\n")) return 0;
    if (!emit_int_array(out, instr, n)) return 0;

    if (!emit_str(out, "};\n\nstatic const int pixel_data[PIXEL_COUNT] = {\n")) return 0;
    if (!emit_int_array(out, data, n)) return 0;

    if (!emit_str(out, "};\n\n")) return 0;

    return 1;
}

static int emit_rt(FILE *out)
{
    if (fprintf(out,
                "static int in_bounds(int x, int y)\n"
                "{\n"
                "  return !(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT);\n"
                "}\n\n"
                "static void move_once(int *x, int *y, int dir)\n"
                "{\n"
                "  switch (dir)\n"
                "  {\n"
                "    case 0: (*x)++; break;\n"
                "    case 1: (*y)++; break;\n"
                "    case 2: (*x)--; break;\n"
                "    case 3: (*y)--; break;\n"
                "  }\n"
                "}\n\n"
                "int main(void)\n"
                "{\n"
                "  int stack[STACK_MAX];\n"
                "  int sp = 0;\n"
                "  int reg_a = 0;\n"
                "  int reg_b = 0;\n"
                "  int reg_c = 0;\n"
                "  int reg_d = 0;\n"
                "  int x = 0;\n"
                "  int y = 0;\n"
                "  int dir = 0;\n"
                "\n"
                "  for (;;)\n"
                "  {\n"
                "    int idx;\n"
                "    int type;\n"
                "    int instr;\n"
                "    int data;\n"
                "    int a, b;\n"
                "    int conditional_move = 0;\n"
                "\n"
                "    if (!in_bounds(x, y))\n"
                "    {\n"
                "      fprintf(stderr, \"EXEC_ERROR: ERR_OUT_OF_BOUNDS\\n\");\n"
                "      return 1;\n"
                "    }\n"
                "\n"
                "    idx = y * WIDTH + x;\n"
                "    type = (int)pixel_type[idx];\n"
                "    instr = pixel_instr[idx];\n"
                "    data = pixel_data[idx];\n"
                "\n"
                "    if (type == %d) return 0;\n"
                "\n"
                "    if (type == %d)\n"
                "    {\n"
                "      fprintf(stderr, \"EXEC_ERROR: ERR_TRANSPARENT_PIXEL\\n\");\n"
                "      return 1;\n"
                "    }\n"
                "\n"
                "    if (type == %d)\n"
                "    {\n"
                "      if (sp >= STACK_MAX)\n"
                "      {\n"
                "        fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\");\n"
                "        return 1;\n"
                "      }\n"
                "      stack[sp++] = data;\n"
                "    }\n"
                "    else if (type == %d)\n"
                "    {\n"
                "      switch (instr)\n"
                "      {\n"
                "        case %d: case %d: case %d: case %d:\n"
                "        case %d: case %d: case %d: case %d:\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          sp--;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[sp - 1];\n"
                "          b = stack[sp - 2];\n"
                "          stack[sp - 1] = b;\n"
                "          stack[sp - 2] = a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          b = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = b + a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          b = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = b - a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          b = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = b * a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          b = stack[--sp];\n"
                "          if (a == 0) { fprintf(stderr, \"EXEC_ERROR: ERR_DIV_ZERO\\n\"); return 1; }\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = b / a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          b = stack[--sp];\n"
                "          if (a == 0) { fprintf(stderr, \"EXEC_ERROR: ERR_MOD_ZERO\\n\"); return 1; }\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = b %% a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = -a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = a + 1;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = a - 1;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          reg_a = stack[--sp];\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = reg_a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          reg_b = stack[--sp];\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = reg_b;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          reg_c = stack[--sp];\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = reg_c;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          reg_d = stack[--sp];\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = reg_d;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          conditional_move = (a != 0);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          conditional_move = (a == 0);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (scanf(\"%%d\", &a) != 1) { fprintf(stderr, \"EXEC_ERROR: ERR_INPUT\\n\"); return 1; }\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = a;\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          a = getchar();\n"
                "          if (a == EOF) { fprintf(stderr, \"EXEC_ERROR: ERR_INPUT\\n\"); return 1; }\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          stack[sp++] = (unsigned char)(a & 0xFF);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          printf(\"%%d\", a);\n"
                "          fflush(stdout);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          a = stack[--sp];\n"
                "          putchar((unsigned char)(a & 0xFF));\n"
                "          fflush(stdout);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "          if (sp > 0)\n"
                "            printf(\"DEBUG: SP=%%d TOP=%%d A=%%d B=%%d C=%%d D=%%d\\n\", sp, stack[sp - 1], reg_a, reg_b, reg_c, reg_d);\n"
                "          else\n"
                "            printf(\"DEBUG: SP=%%d TOP=EMPTY A=%%d B=%%d C=%%d D=%%d\\n\", sp, reg_a, reg_b, reg_c, reg_d);\n"
                "          break;\n"
                "\n"
                "        case %d:\n"
                "        case %d:\n"
                "          break;\n"
                "\n"
                "        default:\n"
                "          break;\n"
                "      }\n"
                "    }\n"
                "\n"
                "    if (type == %d)\n"
                "    {\n"
                "      switch (instr)\n"
                "      {\n"
                "        case %d: case %d: dir = 0; break;\n"
                "        case %d: case %d: dir = 1; break;\n"
                "        case %d: case %d: dir = 2; break;\n"
                "        case %d: case %d: dir = 3; break;\n"
                "        default: break;\n"
                "      }\n"
                "    }\n"
                "\n"
                "    move_once(&x, &y, dir);\n"
                "    if (!in_bounds(x, y))\n"
                "    {\n"
                "      fprintf(stderr, \"EXEC_ERROR: ERR_OUT_OF_BOUNDS\\n\");\n"
                "      return 1;\n"
                "    }\n"
                "\n"
                "    if (type == %d && (instr == %d || instr == %d || instr == %d || instr == %d))\n"
                "    {\n"
                "      move_once(&x, &y, dir);\n"
                "      if (!in_bounds(x, y))\n"
                "      {\n"
                "        fprintf(stderr, \"EXEC_ERROR: ERR_OUT_OF_BOUNDS\\n\");\n"
                "        return 1;\n"
                "      }\n"
                "    }\n"
                "\n"
                "    if (conditional_move)\n"
                "    {\n"
                "      move_once(&x, &y, dir);\n"
                "      if (!in_bounds(x, y))\n"
                "      {\n"
                "        fprintf(stderr, \"EXEC_ERROR: ERR_OUT_OF_BOUNDS\\n\");\n"
                "        return 1;\n"
                "      }\n"
                "    }\n"
                "  }\n"
                "}\n",
                PIXEL_HALT,
                PIXEL_ERROR,
                PIXEL_DATA,
                PIXEL_CODE,
                INSTR_RIGHT, INSTR_RIGHT_SKIP, INSTR_DOWN, INSTR_DOWN_SKIP,
                INSTR_LEFT, INSTR_LEFT_SKIP, INSTR_UP, INSTR_UP_SKIP,
                INSTR_POP,
                INSTR_SWAP,
                INSTR_ADD,
                INSTR_SUB,
                INSTR_MUL,
                INSTR_DIV,
                INSTR_MOD,
                INSTR_NEG,
                INSTR_INC,
                INSTR_DEC,
                INSTR_STORE_A,
                INSTR_LOAD_A,
                INSTR_STORE_B,
                INSTR_LOAD_B,
                INSTR_STORE_C,
                INSTR_LOAD_C,
                INSTR_STORE_D,
                INSTR_LOAD_D,
                INSTR_JNZ,
                INSTR_JZ,
                INSTR_IN_NUM,
                INSTR_IN_CHR,
                INSTR_OUT_NUM,
                INSTR_OUT_CHR,
                INSTR_DEBUG,
                INSTR_NOP,
                INSTR_NONE,
                PIXEL_CODE,
                INSTR_RIGHT, INSTR_RIGHT_SKIP,
                INSTR_DOWN, INSTR_DOWN_SKIP,
                INSTR_LEFT, INSTR_LEFT_SKIP,
                INSTR_UP, INSTR_UP_SKIP,
                PIXEL_CODE, INSTR_RIGHT_SKIP, INSTR_DOWN_SKIP, INSTR_LEFT_SKIP, INSTR_UP_SKIP) < 0)
        return 0;

    return 1;
}

static int image_has_ex(const unsigned char *img, int w, int h)
{
    int n = w * h;
    for (int i = 0; i < n; i++)
    {
        int k = i * 4;
        DecodedPixel p = decode_pixel(img[k + 0], img[k + 1], img[k + 2], img[k + 3]);
        if (p.type == PIXEL_CODE)
        {
            HSV hsv = rgb_to_hsv(img[k + 0], img[k + 1], img[k + 2]);
            if (hsv.s > 60) return 1;
        }
    }
    return 0;
}

static int emit_img_rgba(FILE *out, const unsigned char *img, int w, int h)
{
    int n = w * h * 4;
    TRY_IO(fprintf(out,
                   "#include <stdio.h>\n"
                   "#include <stdlib.h>\n\n"
                   "#define WIDTH %d\n"
                   "#define HEIGHT %d\n"
                   "#define PIXEL_COUNT %d\n"
                   "#define STACK_MAX 1024\n"
                   "#define CALL_MAX 256\n\n"
                   "static unsigned char img[PIXEL_COUNT * 4] = {\n",
                   w, h, w * h));

    if (!emit_u8_array(out, img, n)) return 0;
    if (!emit_str(out, "};\n\n")) return 0;
    return 1;
}

typedef struct
{
    int h_max;
    const char *lo;
    const char *hi;
} DecodeBand;

static const DecodeBand base_bands[] = {
    {24, "I_RIGHT", "I_RIGHT_SKIP"},
    {48, "I_DOWN", "I_DOWN_SKIP"},
    {72, "I_LEFT", "I_LEFT_SKIP"},
    {96, "I_UP", "I_UP_SKIP"},
    {120, "I_POP", "I_SWAP"},
    {144, "I_ADD", "I_SUB"},
    {168, "I_MUL", "I_DIV"},
    {192, "I_MOD", "I_NEG"},
    {216, "I_INC", "I_DEC"},
    {240, "I_STORE_A", "I_LOAD_A"},
    {264, "I_STORE_B", "I_LOAD_B"},
    {288, "I_JNZ", "I_JZ"},
    {312, "I_IN_NUM", "I_IN_CHR"},
    {336, "I_OUT_NUM", "I_OUT_CHR"},
    {360, "I_NOP", "I_DEBUG"}
};

static const DecodeBand ex_bands[] = {
    {24, "I_TRAP_00", "I_TRAP_01"},
    {48, "I_TRAP_02", "I_TRAP_03"},
    {72, "I_TRAP_04", "I_TRAP_05"},
    {96, "I_TRAP_06", "I_TRAP_07"},
    {120, "I_DUP", "I_OVER"},
    {144, "I_AND", "I_OR"},
    {168, "I_NOT", "I_XOR"},
    {192, "I_ROT", "I_ROTR"},
    {216, "I_READ", "I_WRITE"},
    {240, "I_STORE_C", "I_LOAD_C"},
    {264, "I_STORE_D", "I_LOAD_D"},
    {288, "I_JGT", "I_JLT"},
    {312, "I_CALL", "I_RET"},
    {336, "I_TRAP_12", "I_TRAP_13"},
    {360, "I_DEPTH", "I_CLEAR"}
};

static int emit_decode_fn(FILE *out, const char *name, const DecodeBand *bands, int n)
{
    TRY_IO(fprintf(out, "static int %s(int h, int hv)\n{\n", name));
    for (int i = 0; i < n - 1; i++)
        TRY_IO(fprintf(out, "  if (h < %d) return hv ? %s : %s;\n",
                       bands[i].h_max, bands[i].hi, bands[i].lo));
    TRY_IO(fprintf(out, "  return hv ? %s : %s;\n}\n",
                   bands[n - 1].hi, bands[n - 1].lo));
    return 1;
}

static int emit_rt_dynamic(FILE *out)
{
    if (!emit_str(out,
                "enum { TYPE_HALT, TYPE_ERROR, TYPE_DATA, TYPE_CODE };\n"
                "enum {\n"
                "  I_RIGHT, I_RIGHT_SKIP, I_DOWN, I_DOWN_SKIP, I_LEFT, I_LEFT_SKIP, I_UP, I_UP_SKIP,\n"
                "  I_POP, I_SWAP, I_ADD, I_SUB, I_MUL, I_DIV, I_MOD, I_NEG, I_INC, I_DEC,\n"
                "  I_STORE_A, I_LOAD_A, I_STORE_B, I_LOAD_B, I_STORE_C, I_LOAD_C, I_STORE_D, I_LOAD_D,\n"
                "  I_JNZ, I_JZ, I_IN_NUM, I_IN_CHR, I_OUT_NUM, I_OUT_CHR, I_NOP, I_DEBUG,\n"
                "  I_JGT, I_JLT, I_TRAP_00, I_TRAP_01, I_CALL, I_RET, I_TRAP_02, I_TRAP_03,\n"
                "  I_DUP, I_OVER, I_TRAP_04, I_TRAP_05, I_ROT, I_ROTR, I_TRAP_06, I_TRAP_07,\n"
                "  I_READ, I_WRITE, I_TRAP_08, I_TRAP_09, I_AND, I_OR, I_TRAP_10, I_TRAP_11,\n"
                "  I_NOT, I_XOR, I_TRAP_12, I_TRAP_13, I_DEPTH, I_CLEAR\n"
                "};\n\n"
                "static int max3(int a, int b, int c) { int m = a; if (b > m) m = b; if (c > m) m = c; return m; }\n"
                "static int min3(int a, int b, int c) { int m = a; if (b < m) m = b; if (c < m) m = c; return m; }\n"
                "static int in_bounds(int x, int y) { return !(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT); }\n"
                "static void move_once(int *x, int *y, int d) { if (d == 0) (*x)++; else if (d == 1) (*y)++; else if (d == 2) (*x)--; else (*y)--; }\n"
                "static void rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b, int *h, int *s, int *v)\n"
                "{\n"
                "  int mx = max3(r, g, b), mn = min3(r, g, b), df = mx - mn;\n"
                "  *v = mx;\n"
                "  *s = (mx == 0) ? 0 : (df * 100) / mx;\n"
                "  if (df == 0) *h = 0;\n"
                "  else if (mx == r) *h = ((60 * ((g - b) * 100 / df) + 36000) % 36000) / 100;\n"
                "  else if (mx == g) *h = (60 * ((b - r) * 100 / df) + 12000) / 100;\n"
                "  else *h = (60 * ((r - g) * 100 / df) + 24000) / 100;\n"
                "}\n"
                ))
        return 0;

    if (!emit_decode_fn(out, "dec_base", base_bands, (int)(sizeof(base_bands) / sizeof(base_bands[0]))))
        return 0;
    if (!emit_decode_fn(out, "dec_ex", ex_bands, (int)(sizeof(ex_bands) / sizeof(ex_bands[0]))))
        return 0;

    if (!emit_str(out,
                "static void decode_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a, int *t, int *i, int *d)\n"
                "{\n"
                "  int h, s, v;\n"
                "  *i = I_NOP; *d = 0;\n"
                "  if (a == 0) { *t = TYPE_ERROR; return; }\n"
                "  if (r == 0 && g == 0 && b == 0) { *t = TYPE_HALT; return; }\n"
                "  rgb_to_hsv(r, g, b, &h, &s, &v);\n"
                "  if (s <= 20) { *t = TYPE_DATA; *d = max3(r, g, b); return; }\n"
                "  *t = TYPE_CODE;\n"
                "  *i = (s > 60) ? dec_ex(h, v >= 128) : dec_base(h, v >= 128);\n"
                "}\n\n"
                "int main(void)\n"
                "{\n"
                "  int st[STACK_MAX], sp = 0;\n"
                "  int a = 0, b = 0, c = 0, d = 0;\n"
                "  int cx[CALL_MAX], cy[CALL_MAX], cd[CALL_MAX], csp = 0;\n"
                "  int x = 0, y = 0, dir = 0;\n"
                "  for (;;)\n"
                "  {\n"
                "    int idx, t, i, dv;\n"
                "    int set_dir = 0, ndir = dir, do_skip = 0, do_cond = 0, do_tp = 0, tpx = 0, tpy = 0, tpd = dir;\n"
                "    if (!in_bounds(x, y)) return 1;\n"
                "    idx = (y * WIDTH + x) * 4;\n"
                "    decode_pixel(img[idx + 0], img[idx + 1], img[idx + 2], img[idx + 3], &t, &i, &dv);\n"
                "    if (t == TYPE_HALT) return 0;\n"
                "    if (t == TYPE_DATA)\n"
                "    {\n"
                "      if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "      st[sp++] = dv;\n"
                "    }\n"
                "    else if (t == TYPE_CODE)\n"
                "    {\n"
                "      int u, v, rx, ry, mi;\n"
                "      switch (i)\n"
                "      {\n"
                "        case I_RIGHT: set_dir = 1; ndir = 0; break;\n"
                "        case I_RIGHT_SKIP: set_dir = 1; ndir = 0; do_skip = 1; break;\n"
                "        case I_DOWN: set_dir = 1; ndir = 1; break;\n"
                "        case I_DOWN_SKIP: set_dir = 1; ndir = 1; do_skip = 1; break;\n"
                "        case I_LEFT: set_dir = 1; ndir = 2; break;\n"
                "        case I_LEFT_SKIP: set_dir = 1; ndir = 2; do_skip = 1; break;\n"
                "        case I_UP: set_dir = 1; ndir = 3; break;\n"
                "        case I_UP_SKIP: set_dir = 1; ndir = 3; do_skip = 1; break;\n"
                "        case I_POP: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } sp--; break;\n"
                "        case I_SWAP: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = u; break;\n"
                "        case I_ADD: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v + u; break;\n"
                "        case I_SUB: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v - u; break;\n"
                "        case I_MUL: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v * u; break;\n"
                "        case I_DIV: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; if (u == 0) { fprintf(stderr, \"EXEC_ERROR: ERR_DIV_ZERO\\n\"); return 1; } st[sp++] = v / u; break;\n"
                "        case I_MOD: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; if (u == 0) { fprintf(stderr, \"EXEC_ERROR: ERR_MOD_ZERO\\n\"); return 1; } st[sp++] = v % u; break;\n"
                "        case I_NEG: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } st[sp - 1] = -st[sp - 1]; break;\n"
                "        case I_INC: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } st[sp - 1]++; break;\n"
                "        case I_DEC: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } st[sp - 1]--; break;\n"
                "        case I_STORE_A: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } a = st[--sp]; break;\n"
                "        case I_LOAD_A: if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = a; break;\n"
                "        case I_STORE_B: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } b = st[--sp]; break;\n"
                "        case I_LOAD_B: if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = b; break;\n"
                "        case I_STORE_C: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } c = st[--sp]; break;\n"
                "        case I_LOAD_C: if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = c; break;\n"
                "        case I_STORE_D: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } d = st[--sp]; break;\n"
                "        case I_LOAD_D: if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = d; break;\n"
                "        case I_JNZ: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } do_cond = (st[--sp] != 0); break;\n"
                "        case I_JZ: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } do_cond = (st[--sp] == 0); break;\n"
                "        case I_JGT: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; do_cond = (v > u); break;\n"
                "        case I_JLT: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; do_cond = (v < u); break;\n"
                "        case I_CALL:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          if (csp >= CALL_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_CALL_OVERFLOW\\n\"); return 1; }\n"
                "          u = st[--sp]; v = st[--sp]; rx = x; ry = y; move_once(&rx, &ry, dir);\n"
                "          cx[csp] = rx; cy[csp] = ry; cd[csp] = dir; csp++;\n"
                "          do_tp = 1; tpx = v; tpy = u; tpd = dir;\n"
                "          break;\n"
                "        case I_RET:\n"
                "          if (csp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_CALL_UNDERFLOW\\n\"); return 1; }\n"
                "          csp--; do_tp = 1; tpx = cx[csp]; tpy = cy[csp]; tpd = cd[csp];\n"
                "          break;\n"
                "        case I_DUP: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp] = st[sp - 1]; sp++; break;\n"
                "        case I_OVER: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = st[sp - 2]; break;\n"
                "        case I_ROT: if (sp < 3) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[sp - 3]; st[sp - 3] = st[sp - 2]; st[sp - 2] = st[sp - 1]; st[sp - 1] = u; break;\n"
                "        case I_ROTR: if (sp < 3) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[sp - 1]; st[sp - 1] = st[sp - 2]; st[sp - 2] = st[sp - 3]; st[sp - 3] = u; break;\n"
                "        case I_READ:\n"
                "          if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          u = st[--sp]; v = st[--sp];\n"
                "          if (!in_bounds(v, u)) { fprintf(stderr, \"EXEC_ERROR: ERR_MEM_OOB\\n\"); return 1; }\n"
                "          mi = (u * WIDTH + v) * 4;\n"
                "          if (img[mi + 3] == 0) { fprintf(stderr, \"EXEC_ERROR: ERR_MEM_ALPHA\\n\"); return 1; }\n"
                "          if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; }\n"
                "          st[sp++] = max3(img[mi], img[mi + 1], img[mi + 2]);\n"
                "          break;\n"
                "        case I_WRITE:\n"
                "          if (sp < 3) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; }\n"
                "          u = st[--sp]; v = st[--sp]; mi = st[--sp];\n"
                "          if (!in_bounds(v, u)) { fprintf(stderr, \"EXEC_ERROR: ERR_MEM_OOB\\n\"); return 1; }\n"
                "          if (mi < 0) mi = 0; if (mi > 255) mi = 255;\n"
                "          idx = (u * WIDTH + v) * 4; img[idx] = (unsigned char)mi; img[idx + 1] = (unsigned char)mi; img[idx + 2] = (unsigned char)mi; img[idx + 3] = 255;\n"
                "          break;\n"
                "        case I_AND: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v & u; break;\n"
                "        case I_OR: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v | u; break;\n"
                "        case I_NOT: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } st[sp - 1] = ~st[sp - 1]; break;\n"
                "        case I_XOR: if (sp < 2) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } u = st[--sp]; v = st[--sp]; st[sp++] = v ^ u; break;\n"
                "        case I_DEPTH: if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = sp; break;\n"
                "        case I_CLEAR: sp = 0; break;\n"
                "        case I_IN_NUM: if (scanf(\"%d\", &u) != 1) { fprintf(stderr, \"EXEC_ERROR: ERR_INPUT\\n\"); return 1; } if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = u; break;\n"
                "        case I_IN_CHR: u = getchar(); if (u == EOF) { fprintf(stderr, \"EXEC_ERROR: ERR_INPUT\\n\"); return 1; } if (sp >= STACK_MAX) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_OVERFLOW\\n\"); return 1; } st[sp++] = (unsigned char)(u & 0xFF); break;\n"
                "        case I_OUT_NUM: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } printf(\"%d\", st[--sp]); fflush(stdout); break;\n"
                "        case I_OUT_CHR: if (sp <= 0) { fprintf(stderr, \"EXEC_ERROR: ERR_STACK_UNDERFLOW\\n\"); return 1; } putchar((unsigned char)(st[--sp] & 0xFF)); fflush(stdout); break;\n"
                "        case I_DEBUG:\n"
                "          if (sp > 0) printf(\"DEBUG: SP=%d TOP=%d A=%d B=%d C=%d D=%d\\n\", sp, st[sp - 1], a, b, c, d);\n"
                "          else printf(\"DEBUG: SP=%d TOP=EMPTY A=%d B=%d C=%d D=%d\\n\", sp, a, b, c, d);\n"
                "          break;\n"
                "        case I_TRAP_00: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_00\\n\"); return 1;\n"
                "        case I_TRAP_01: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_01\\n\"); return 1;\n"
                "        case I_TRAP_02: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_02\\n\"); return 1;\n"
                "        case I_TRAP_03: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_03\\n\"); return 1;\n"
                "        case I_TRAP_04: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_04\\n\"); return 1;\n"
                "        case I_TRAP_05: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_05\\n\"); return 1;\n"
                "        case I_TRAP_06: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_06\\n\"); return 1;\n"
                "        case I_TRAP_07: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_07\\n\"); return 1;\n"
                "        case I_TRAP_08: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_08\\n\"); return 1;\n"
                "        case I_TRAP_09: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_09\\n\"); return 1;\n"
                "        case I_TRAP_10: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_10\\n\"); return 1;\n"
                "        case I_TRAP_11: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_11\\n\"); return 1;\n"
                "        case I_TRAP_12: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_12\\n\"); return 1;\n"
                "        case I_TRAP_13: fprintf(stderr, \"EXEC_ERROR: ERR_TRAP_13\\n\"); return 1;\n"
                "        case I_NOP: break;\n"
                "      }\n"
                "    }\n"
                "    if (do_tp)\n"
                "    {\n"
                "      x = tpx; y = tpy; dir = tpd;\n"
                "      if (!in_bounds(x, y)) return 1;\n"
                "      continue;\n"
                "    }\n"
                "    if (set_dir) dir = ndir;\n"
                "    move_once(&x, &y, dir);\n"
                "    if (!in_bounds(x, y)) return 1;\n"
                "    if (do_skip) { move_once(&x, &y, dir); if (!in_bounds(x, y)) return 1; }\n"
                "    if (do_cond) { move_once(&x, &y, dir); if (!in_bounds(x, y)) return 1; }\n"
                "  }\n"
                "}\n"))
        return 0;

    return 1;
}

int generate_c_from_image(const char *output_path, const unsigned char *img, int width, int height,
                          CodegenOptions *options)
{
    if (image_has_ex(img, width, height))
    {
        FILE *out_ex = fopen(output_path, "w");
        if (!out_ex)
            return 0;

        if (!emit_img_rgba(out_ex, img, width, height) || !emit_rt_dynamic(out_ex))
        {
            fclose(out_ex);
            return 0;
        }

        if (fclose(out_ex) != 0) return 0;

        if (options)
        {
            options->opt_stats.passes_run = 0;
            options->opt_stats.changes = 0;
            options->opt_stats.can_nops = 0;
            options->opt_stats.can_dirs = 0;
            options->opt_stats.lit_ops = 0;
            options->opt_stats.removed_cells = 0;
            options->opt_stats.width_before = width;
            options->opt_stats.height_before = height;
            options->opt_stats.width_after = width;
            options->opt_stats.height_after = height;
        }

        return 1;
    }

    if (options && options->opt_report)
    {
        options->opt_stats.passes_run = 0;
        options->opt_stats.changes = 0;
        options->opt_stats.can_nops = 0;
        options->opt_stats.can_dirs = 0;
        options->opt_stats.lit_ops = 0;
        options->opt_stats.removed_cells = 0;
        options->opt_stats.width_before = width;
        options->opt_stats.height_before = height;
        options->opt_stats.width_after = width;
        options->opt_stats.height_after = height;
    }

    IRProgram ir;
    OptConfig cfg;
    OptStats st;
    // defaulting rules keep old cli behavior and new flags both working
    int opt_enabled = (options && options->enable_opt) ? 1 : 0;
    int opt_level = (options && options->opt_level > 0) ? options->opt_level : (opt_enabled ? 1 : 0);

    if (!ir_from_image(img, width, height, &ir))
        return 0;

    cfg.enabled = opt_enabled;
    cfg.level = opt_level;
    if (!optimize_decoded_program(&ir.pixel_type, &ir.pixel_instr, &ir.pixel_data,
                                  &ir.width, &ir.height, &cfg, &st))
    {
        ir_free(&ir);
        return 0;
    }

    if (options)
        options->opt_stats = st;

    FILE *out = fopen(output_path, "w");
    if (!out)
    {
        ir_free(&ir);
        return 0;
    }

    if (!emit_tabs(out, ir.pixel_type, ir.pixel_instr, ir.pixel_data, ir.width, ir.height))
    {
        fclose(out);
        ir_free(&ir);
        return 0;
    }

    if (!emit_rt(out))
    {
        fclose(out);
        ir_free(&ir);
        return 0;
    }

    // free ir before closing file so early-return paths stay simpler
    ir_free(&ir);

    if (fclose(out) != 0) return 0;
    return 1;
}
