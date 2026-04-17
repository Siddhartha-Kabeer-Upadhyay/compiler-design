#include "codegen.h"
#include "instruction.h"
#include "ir.h"

#include <stdio.h>

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
    {
        if (fprintf(out, "  %d%s", (int)type[i], (i + 1 == n) ? "\n" : ",") < 0) return 0;
    }

    if (fprintf(out, "};\n\nstatic const int pixel_instr[PIXEL_COUNT] = {\n") < 0) return 0;

    for (int i = 0; i < n; i++)
    {
        if (fprintf(out, "  %d%s", instr[i], (i + 1 == n) ? "\n" : ",") < 0) return 0;
    }

    if (fprintf(out, "};\n\nstatic const int pixel_data[PIXEL_COUNT] = {\n") < 0) return 0;

    for (int i = 0; i < n; i++)
    {
        if (fprintf(out, "  %d%s", data[i], (i + 1 == n) ? "\n" : ",") < 0) return 0;
    }

    if (fprintf(out, "};\n\n") < 0) return 0;

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

int generate_c_from_image(const char *output_path, const unsigned char *img, int width, int height,
                          CodegenOptions *options)
{
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
