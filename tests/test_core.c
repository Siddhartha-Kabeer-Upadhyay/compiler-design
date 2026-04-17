#include <stdio.h>

#include "hsv.h"
#include "instruction.h"
#include "runtime.h"
#include "tracer.h"

static int failures = 0;

static void expect_int(const char *name, int got, int expected)
{
    if (got != expected)
    {
        printf("FAIL [%s] got=%d expected=%d\n", name, got, expected);
        failures++;
    }
}

static void test_hsv(void)
{
    HSV g = rgb_to_hsv(128, 128, 128);
    expect_int("hsv-gray-h", g.h, 0);
    expect_int("hsv-gray-s", g.s, 0);
    expect_int("hsv-gray-v", g.v, 128);

    HSV r = rgb_to_hsv(255, 0, 0);
    expect_int("hsv-red-h", r.h, 0);
    expect_int("hsv-red-s", r.s, 100);
    expect_int("hsv-red-v", r.v, 255);

    HSV b = rgb_to_hsv(0, 0, 255);
    expect_int("hsv-blue-h", b.h, 240);
    expect_int("hsv-blue-s", b.s, 100);
    expect_int("hsv-blue-v", b.v, 255);
}

static void test_decode_precedence(void)
{
    DecodedPixel p0 = decode_pixel(0, 0, 0, 0);
    expect_int("decode-alpha-before-black", p0.type, PIXEL_ERROR);

    DecodedPixel p1 = decode_pixel(0, 0, 0, 255);
    expect_int("decode-black-halt", p1.type, PIXEL_HALT);

    DecodedPixel p2 = decode_pixel(200, 200, 200, 255);
    expect_int("decode-data-type", p2.type, PIXEL_DATA);
    expect_int("decode-data-value", p2.data_value, 200);

    DecodedPixel p3 = decode_pixel(255, 153, 153, 255);
    expect_int("decode-code-type", p3.type, PIXEL_CODE);
    expect_int("decode-base-v-threshold", p3.instr, INSTR_RIGHT_SKIP);
}

static void test_runtime_core(void)
{
    RuntimeState rt;
    RouteEffect fx;
    runtime_init(&rt);

    DecodedPixel d2 = { PIXEL_DATA, INSTR_NONE, 2 };
    DecodedPixel d3 = { PIXEL_DATA, INSTR_NONE, 3 };
    DecodedPixel d5 = { PIXEL_DATA, INSTR_NONE, 5 };
    DecodedPixel d7 = { PIXEL_DATA, INSTR_NONE, 7 };
    DecodedPixel add = { PIXEL_CODE, INSTR_ADD, 0 };
    DecodedPixel jz = { PIXEL_CODE, INSTR_JZ, 0 };
    DecodedPixel div = { PIXEL_CODE, INSTR_DIV, 0 };
    DecodedPixel sc = { PIXEL_CODE, INSTR_STORE_C, 0 };
    DecodedPixel lc = { PIXEL_CODE, INSTR_LOAD_C, 0 };
    DecodedPixel sd = { PIXEL_CODE, INSTR_STORE_D, 0 };
    DecodedPixel ld = { PIXEL_CODE, INSTR_LOAD_D, 0 };
    DecodedPixel t0 = { PIXEL_CODE, INSTR_TRAP_00, 0 };
    DecodedPixel t9 = { PIXEL_CODE, INSTR_TRAP_09, 0 };

    expect_int("runtime-push-2", execute_pixel(&rt, d2, &fx), EXEC_OK);
    expect_int("runtime-push-3", execute_pixel(&rt, d3, &fx), EXEC_OK);
    expect_int("runtime-add", execute_pixel(&rt, add, &fx), EXEC_OK);
    expect_int("runtime-sp-after-add", rt.sp, 1);
    expect_int("runtime-top-after-add", rt.stack[rt.sp - 1], 5);

    expect_int("runtime-jz-pop", execute_pixel(&rt, jz, &fx), EXEC_OK);
    expect_int("runtime-jz-flag", fx.do_cond, 0);
    expect_int("runtime-sp-after-jz", rt.sp, 0);

    expect_int("runtime-div-underflow", execute_pixel(&rt, div, &fx), EXEC_ERR_STACK_UNDERFLOW);

    DecodedPixel d4 = { PIXEL_DATA, INSTR_NONE, 4 };
    DecodedPixel d0 = { PIXEL_DATA, INSTR_NONE, 0 };
    expect_int("runtime-push-4", execute_pixel(&rt, d4, &fx), EXEC_OK);
    expect_int("runtime-push-0", execute_pixel(&rt, d0, &fx), EXEC_OK);
    expect_int("runtime-div-zero", execute_pixel(&rt, div, &fx), EXEC_ERR_DIV_ZERO);

    expect_int("runtime-push-5", execute_pixel(&rt, d5, &fx), EXEC_OK);
    expect_int("runtime-store-c", execute_pixel(&rt, sc, &fx), EXEC_OK);
    expect_int("runtime-load-c", execute_pixel(&rt, lc, &fx), EXEC_OK);
    expect_int("runtime-top-c", rt.stack[rt.sp - 1], 5);

    expect_int("runtime-push-7", execute_pixel(&rt, d7, &fx), EXEC_OK);
    expect_int("runtime-store-d", execute_pixel(&rt, sd, &fx), EXEC_OK);
    expect_int("runtime-load-d", execute_pixel(&rt, ld, &fx), EXEC_OK);
    expect_int("runtime-top-d", rt.stack[rt.sp - 1], 7);

    expect_int("runtime-trap-00", execute_pixel(&rt, t0, &fx), EXEC_ERR_TRAP);
    expect_int("runtime-trap-09", execute_pixel(&rt, t9, &fx), EXEC_ERR_TRAP);
}

static void test_runtime_ex_ops(void)
{
    RuntimeState rt;
    RouteEffect fx;
    runtime_init(&rt);

    DecodedPixel d1 = { PIXEL_DATA, INSTR_NONE, 1 };
    DecodedPixel d2 = { PIXEL_DATA, INSTR_NONE, 2 };
    DecodedPixel d3 = { PIXEL_DATA, INSTR_NONE, 3 };
    DecodedPixel d5 = { PIXEL_DATA, INSTR_NONE, 5 };
    DecodedPixel d6 = { PIXEL_DATA, INSTR_NONE, 6 };

    DecodedPixel dup = { PIXEL_CODE, INSTR_DUP, 0 };
    DecodedPixel over = { PIXEL_CODE, INSTR_OVER, 0 };
    DecodedPixel rot = { PIXEL_CODE, INSTR_ROT, 0 };
    DecodedPixel rotr = { PIXEL_CODE, INSTR_ROTR, 0 };
    DecodedPixel and_op = { PIXEL_CODE, INSTR_AND, 0 };
    DecodedPixel or_op = { PIXEL_CODE, INSTR_OR, 0 };
    DecodedPixel not_op = { PIXEL_CODE, INSTR_NOT, 0 };
    DecodedPixel xor_op = { PIXEL_CODE, INSTR_XOR, 0 };
    DecodedPixel dep = { PIXEL_CODE, INSTR_DEPTH, 0 };
    DecodedPixel clr = { PIXEL_CODE, INSTR_CLEAR, 0 };
    DecodedPixel jgt = { PIXEL_CODE, INSTR_JGT, 0 };
    DecodedPixel jlt = { PIXEL_CODE, INSTR_JLT, 0 };
    DecodedPixel call = { PIXEL_CODE, INSTR_CALL, 0 };
    DecodedPixel ret = { PIXEL_CODE, INSTR_RET, 0 };

    expect_int("ex-dup-underflow", execute_pixel(&rt, dup, &fx), EXEC_ERR_STACK_UNDERFLOW);
    expect_int("ex-over-underflow", execute_pixel(&rt, over, &fx), EXEC_ERR_STACK_UNDERFLOW);

    execute_pixel(&rt, d1, &fx);
    execute_pixel(&rt, d2, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-rot-ok", execute_pixel(&rt, rot, &fx), EXEC_OK);
    expect_int("ex-rot-a", rt.stack[0], 2);
    expect_int("ex-rot-b", rt.stack[1], 3);
    expect_int("ex-rot-c", rt.stack[2], 1);

    expect_int("ex-rotr-ok", execute_pixel(&rt, rotr, &fx), EXEC_OK);
    expect_int("ex-rotr-a", rt.stack[0], 1);
    expect_int("ex-rotr-b", rt.stack[1], 2);
    expect_int("ex-rotr-c", rt.stack[2], 3);

    expect_int("ex-dup-ok", execute_pixel(&rt, dup, &fx), EXEC_OK);
    expect_int("ex-dup-top", rt.stack[rt.sp - 1], 3);

    expect_int("ex-over-ok", execute_pixel(&rt, over, &fx), EXEC_OK);
    expect_int("ex-over-top", rt.stack[rt.sp - 1], 3);

    runtime_init(&rt);
    execute_pixel(&rt, d6, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-and-ok", execute_pixel(&rt, and_op, &fx), EXEC_OK);
    expect_int("ex-and-top", rt.stack[rt.sp - 1], 2);

    runtime_init(&rt);
    execute_pixel(&rt, d5, &fx);
    execute_pixel(&rt, d2, &fx);
    expect_int("ex-or-ok", execute_pixel(&rt, or_op, &fx), EXEC_OK);
    expect_int("ex-or-top", rt.stack[rt.sp - 1], 7);

    runtime_init(&rt);
    execute_pixel(&rt, d5, &fx);
    expect_int("ex-not-ok", execute_pixel(&rt, not_op, &fx), EXEC_OK);
    expect_int("ex-not-top", rt.stack[rt.sp - 1], ~5);

    runtime_init(&rt);
    execute_pixel(&rt, d6, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-xor-ok", execute_pixel(&rt, xor_op, &fx), EXEC_OK);
    expect_int("ex-xor-top", rt.stack[rt.sp - 1], 5);

    runtime_init(&rt);
    execute_pixel(&rt, d1, &fx);
    execute_pixel(&rt, d2, &fx);
    expect_int("ex-depth-ok", execute_pixel(&rt, dep, &fx), EXEC_OK);
    expect_int("ex-depth-top", rt.stack[rt.sp - 1], 2);

    expect_int("ex-clear-ok", execute_pixel(&rt, clr, &fx), EXEC_OK);
    expect_int("ex-clear-sp", rt.sp, 0);

    runtime_init(&rt);
    execute_pixel(&rt, d3, &fx);
    execute_pixel(&rt, d2, &fx);
    expect_int("ex-jgt-true", execute_pixel(&rt, jgt, &fx), EXEC_OK);
    expect_int("ex-jgt-flag-true", fx.do_cond, 1);

    runtime_init(&rt);
    execute_pixel(&rt, d2, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-jgt-false", execute_pixel(&rt, jgt, &fx), EXEC_OK);
    expect_int("ex-jgt-flag-false", fx.do_cond, 0);

    runtime_init(&rt);
    execute_pixel(&rt, d3, &fx);
    execute_pixel(&rt, d2, &fx);
    expect_int("ex-jlt-false", execute_pixel(&rt, jlt, &fx), EXEC_OK);
    expect_int("ex-jlt-flag-false", fx.do_cond, 0);

    runtime_init(&rt);
    execute_pixel(&rt, d2, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-jlt-true", execute_pixel(&rt, jlt, &fx), EXEC_OK);
    expect_int("ex-jlt-flag-true", fx.do_cond, 1);

    runtime_init(&rt);
    expect_int("ex-ret-underflow", execute_pixel_at(&rt, ret, &fx, 4, 5, DIR_LEFT), EXEC_ERR_CALL_UNDERFLOW);

    execute_pixel(&rt, d2, &fx);
    execute_pixel(&rt, d3, &fx);
    expect_int("ex-call-ok", execute_pixel_at(&rt, call, &fx, 4, 5, DIR_LEFT), EXEC_OK);
    expect_int("ex-call-csp", rt.csp, 1);
    expect_int("ex-call-tp", fx.do_tp, 1);
    expect_int("ex-call-x", fx.tp_x, 2);
    expect_int("ex-call-y", fx.tp_y, 3);
    expect_int("ex-call-d", fx.tp_dir, DIR_LEFT);

    expect_int("ex-ret-ok", execute_pixel_at(&rt, ret, &fx, 9, 9, DIR_UP), EXEC_OK);
    expect_int("ex-ret-csp", rt.csp, 0);
    expect_int("ex-ret-tp", fx.do_tp, 1);
    expect_int("ex-ret-x", fx.tp_x, 4);
    expect_int("ex-ret-y", fx.tp_y, 5);
    expect_int("ex-ret-d", fx.tp_dir, DIR_LEFT);
}

static void test_tracer_moves(void)
{
    TracerState t = tracer_init();
    RouteEffect fx;
    route_reset(&fx);
    fx.set_dir = 1;
    fx.dir = DIR_RIGHT;
    fx.do_skip = 1;

    expect_int("tracer-move-skip-ok", tracer_apply(&t, 6, 1, &fx), 1);
    expect_int("tracer-x-after-skip", t.x, 2);
    expect_int("tracer-y-after-skip", t.y, 0);

    route_reset(&fx);
    fx.do_cond = 1;
    expect_int("tracer-cond-move-ok", tracer_apply(&t, 6, 1, &fx), 1);
    expect_int("tracer-x-after-cond", t.x, 4);

    TracerState t2 = tracer_init();
    route_reset(&fx);
    expect_int("tracer-oob", tracer_apply(&t2, 1, 1, &fx), 0);
    expect_int("tracer-oob-flag", t2.error, 1);
}

int main(void)
{
    test_hsv();
    test_decode_precedence();
    test_runtime_core();
    test_runtime_ex_ops();
    test_tracer_moves();

    if (failures)
    {
        printf("\nunit tests failed: %d\n", failures);
        return 1;
    }

    printf("unit tests passed\n");
    return 0;
}
