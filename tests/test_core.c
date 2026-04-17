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
    test_tracer_moves();

    if (failures)
    {
        printf("\nunit tests failed: %d\n", failures);
        return 1;
    }

    printf("unit tests passed\n");
    return 0;
}
