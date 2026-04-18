#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

void runtime_init(RuntimeState *rt)
{
    rt->sp = 0;
    rt->reg_a = 0;
    rt->reg_b = 0;
    rt->reg_c = 0;
    rt->reg_d = 0;
    rt->csp = 0;
}

void route_reset(RouteEffect *fx)
{
    if (!fx) return;
    fx->set_dir = 0;
    fx->dir = 0;
    fx->do_skip = 0;
    fx->do_cond = 0;
    fx->do_tp = 0;
    fx->tp_x = 0;
    fx->tp_y = 0;
    fx->tp_dir = 0;
}

int runtime_push(RuntimeState *rt, int value)
{
    if (rt->sp >= STACK_MAX) return 0;
    rt->stack[rt->sp++] = value;
    return 1;
}

int runtime_pop(RuntimeState *rt, int *out)
{
    if (rt->sp <= 0) return 0;
    rt->sp--;
    if (out) *out = rt->stack[rt->sp];
    return 1;
}

int runtime_peek(RuntimeState *rt, int *out)
{
    if (rt->sp <= 0) return 0;
    if (out) *out = rt->stack[rt->sp - 1];
    return 1;
}

static ExecStatus exec_code(RuntimeState *rt, Instruction instr, RouteEffect *fx,
                            int cur_x, int cur_y, int cur_d)
{
    int a, b;

    switch (instr)
    {
        case INSTR_RIGHT:
            if (fx) { fx->set_dir = 1; fx->dir = 0; }
            return EXEC_OK;
        case INSTR_RIGHT_SKIP:
            if (fx) { fx->set_dir = 1; fx->dir = 0; fx->do_skip = 1; }
            return EXEC_OK;
        case INSTR_DOWN:
            if (fx) { fx->set_dir = 1; fx->dir = 1; }
            return EXEC_OK;
        case INSTR_DOWN_SKIP:
            if (fx) { fx->set_dir = 1; fx->dir = 1; fx->do_skip = 1; }
            return EXEC_OK;
        case INSTR_LEFT:
            if (fx) { fx->set_dir = 1; fx->dir = 2; }
            return EXEC_OK;
        case INSTR_LEFT_SKIP:
            if (fx) { fx->set_dir = 1; fx->dir = 2; fx->do_skip = 1; }
            return EXEC_OK;
        case INSTR_UP:
            if (fx) { fx->set_dir = 1; fx->dir = 3; }
            return EXEC_OK;
        case INSTR_UP_SKIP:
            if (fx) { fx->set_dir = 1; fx->dir = 3; fx->do_skip = 1; }
            return EXEC_OK;

        case INSTR_JGT:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (fx) fx->do_cond = (b > a);
            return EXEC_OK;

        case INSTR_JLT:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (fx) fx->do_cond = (b < a);
            return EXEC_OK;

        case INSTR_CALL:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (rt->csp >= CALL_MAX) return EXEC_ERR_CALL_OVERFLOW;
            rt->call_x[rt->csp] = cur_x;
            rt->call_y[rt->csp] = cur_y;
            rt->call_d[rt->csp] = cur_d;
            rt->csp++;
            if (fx)
            {
                fx->do_tp = 1;
                fx->tp_x = b;
                fx->tp_y = a;
                fx->tp_dir = cur_d;
            }
            return EXEC_OK;

        case INSTR_RET:
            if (rt->csp <= 0) return EXEC_ERR_CALL_UNDERFLOW;
            rt->csp--;
            if (fx)
            {
                fx->do_tp = 1;
                fx->tp_x = rt->call_x[rt->csp];
                fx->tp_y = rt->call_y[rt->csp];
                fx->tp_dir = rt->call_d[rt->csp];
            }
            return EXEC_OK;

        case INSTR_READ:
        case INSTR_WRITE:
            return EXEC_ERR_MEM_CTX;

        case INSTR_DUP:
            if (!runtime_peek(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_OVER:
            if (rt->sp < 2) return EXEC_ERR_STACK_UNDERFLOW;
            a = rt->stack[rt->sp - 2];
            if (!runtime_push(rt, a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_ROT:
            if (rt->sp < 3) return EXEC_ERR_STACK_UNDERFLOW;
            a = rt->stack[rt->sp - 3];
            b = rt->stack[rt->sp - 2];
            rt->stack[rt->sp - 3] = b;
            rt->stack[rt->sp - 2] = rt->stack[rt->sp - 1];
            rt->stack[rt->sp - 1] = a;
            return EXEC_OK;

        case INSTR_ROTR:
            if (rt->sp < 3) return EXEC_ERR_STACK_UNDERFLOW;
            a = rt->stack[rt->sp - 1];
            b = rt->stack[rt->sp - 3];
            rt->stack[rt->sp - 1] = rt->stack[rt->sp - 2];
            rt->stack[rt->sp - 2] = b;
            rt->stack[rt->sp - 3] = a;
            return EXEC_OK;

        case INSTR_AND:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b & a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_OR:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b | a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_NOT:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, ~a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_XOR:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b ^ a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_DEPTH:
            if (!runtime_push(rt, rt->sp)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_CLEAR:
            rt->sp = 0;
            return EXEC_OK;

        case INSTR_TRAP_00:
        case INSTR_TRAP_01:
        case INSTR_TRAP_02:
        case INSTR_TRAP_03:
        case INSTR_TRAP_04:
        case INSTR_TRAP_05:
        case INSTR_TRAP_06:
        case INSTR_TRAP_07:
        case INSTR_TRAP_08:
        case INSTR_TRAP_09:
        case INSTR_TRAP_10:
        case INSTR_TRAP_11:
        case INSTR_TRAP_12:
        case INSTR_TRAP_13:
            return EXEC_ERR_TRAP;

        case INSTR_POP:
            if (!runtime_pop(rt, NULL)) return EXEC_ERR_STACK_UNDERFLOW;
            return EXEC_OK;

        case INSTR_SWAP:
            if (rt->sp < 2) return EXEC_ERR_STACK_UNDERFLOW;
            a = rt->stack[rt->sp - 1];
            b = rt->stack[rt->sp - 2];
            rt->stack[rt->sp - 1] = b;
            rt->stack[rt->sp - 2] = a;
            return EXEC_OK;

        case INSTR_ADD:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b + a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_SUB:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b - a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_MUL:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, b * a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_DIV:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (a == 0) return EXEC_ERR_DIV_ZERO;
            if (!runtime_push(rt, b / a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_MOD:
            if (!runtime_pop(rt, &a) || !runtime_pop(rt, &b)) return EXEC_ERR_STACK_UNDERFLOW;
            if (a == 0) return EXEC_ERR_MOD_ZERO;
            if (!runtime_push(rt, b % a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_NEG:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, -a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_INC:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, a + 1)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_DEC:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (!runtime_push(rt, a - 1)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_STORE_A:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            rt->reg_a = a;
            return EXEC_OK;

        case INSTR_LOAD_A:
            if (!runtime_push(rt, rt->reg_a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_STORE_B:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            rt->reg_b = a;
            return EXEC_OK;

        case INSTR_LOAD_B:
            if (!runtime_push(rt, rt->reg_b)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_STORE_C:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            rt->reg_c = a;
            return EXEC_OK;

        case INSTR_LOAD_C:
            if (!runtime_push(rt, rt->reg_c)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_STORE_D:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            rt->reg_d = a;
            return EXEC_OK;

        case INSTR_LOAD_D:
            if (!runtime_push(rt, rt->reg_d)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_JNZ:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (fx) fx->do_cond = (a != 0);
            return EXEC_OK;

        case INSTR_JZ:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            if (fx) fx->do_cond = (a == 0);
            return EXEC_OK;

        case INSTR_IN_NUM:
            if (scanf("%d", &a) != 1) return EXEC_ERR_INPUT;
            if (!runtime_push(rt, a)) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_IN_CHR:
            a = getchar();
            if (a == EOF) return EXEC_ERR_INPUT;
            if (!runtime_push(rt, (unsigned char)(a & 0xFF))) return EXEC_ERR_STACK_OVERFLOW;
            return EXEC_OK;

        case INSTR_OUT_NUM:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            printf("%d", a);
            fflush(stdout);
            return EXEC_OK;

        case INSTR_OUT_CHR:
            if (!runtime_pop(rt, &a)) return EXEC_ERR_STACK_UNDERFLOW;
            putchar((unsigned char)(a & 0xFF));
            fflush(stdout);
            return EXEC_OK;

        case INSTR_DEBUG:
            if (rt->sp > 0)
                printf("DEBUG: SP=%d TOP=%d A=%d B=%d C=%d D=%d\n", rt->sp, rt->stack[rt->sp - 1], rt->reg_a, rt->reg_b, rt->reg_c, rt->reg_d);
            else
                printf("DEBUG: SP=%d TOP=EMPTY A=%d B=%d C=%d D=%d\n", rt->sp, rt->reg_a, rt->reg_b, rt->reg_c, rt->reg_d);
            return EXEC_OK;

        case INSTR_NOP:
        case INSTR_NONE:
            return EXEC_OK;
    }

    return EXEC_OK;
}

static ExecStatus exec_mem(RuntimeState *rt, Instruction instr, unsigned char *img, int w, int h)
{
    int x, y, v;
    int idx;

    if (!img || w <= 0 || h <= 0) return EXEC_ERR_MEM_CTX;

    if (instr == INSTR_READ)
    {
        if (!runtime_pop(rt, &y) || !runtime_pop(rt, &x)) return EXEC_ERR_STACK_UNDERFLOW;
        if (x < 0 || x >= w || y < 0 || y >= h) return EXEC_ERR_MEM_OOB;
        idx = (y * w + x) * 4;
        if (img[idx + 3] == 0) return EXEC_ERR_MEM_ALPHA;
        v = img[idx + 0];
        if (img[idx + 1] > v) v = img[idx + 1];
        if (img[idx + 2] > v) v = img[idx + 2];
        if (!runtime_push(rt, v)) return EXEC_ERR_STACK_OVERFLOW;
        return EXEC_OK;
    }

    if (instr == INSTR_WRITE)
    {
        if (!runtime_pop(rt, &y) || !runtime_pop(rt, &x) || !runtime_pop(rt, &v))
            return EXEC_ERR_STACK_UNDERFLOW;
        if (x < 0 || x >= w || y < 0 || y >= h) return EXEC_ERR_MEM_OOB;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        idx = (y * w + x) * 4;
        img[idx + 0] = (unsigned char)v;
        img[idx + 1] = (unsigned char)v;
        img[idx + 2] = (unsigned char)v;
        img[idx + 3] = 255;
        return EXEC_OK;
    }

    return EXEC_OK;
}

ExecStatus execute_pixel(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx)
{ return execute_pixel_at(rt, pixel, fx, 0, 0, 0); }

ExecStatus execute_pixel_at(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx,
                            int cur_x, int cur_y, int cur_d)
{ return execute_pixel_ctx(rt, pixel, fx, cur_x, cur_y, cur_d, NULL, 0, 0); }

ExecStatus execute_pixel_ctx(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx,
                             int cur_x, int cur_y, int cur_d,
                             unsigned char *img, int w, int h)
{
    route_reset(fx);

    if (pixel.type == PIXEL_HALT) return EXEC_HALT;
    if (pixel.type == PIXEL_ERROR) return EXEC_OK; // tracer will handle this

    if (pixel.type == PIXEL_DATA)
    {
        if (!runtime_push(rt, pixel.data_value)) return EXEC_ERR_STACK_OVERFLOW;
        return EXEC_OK;
    }

    if (pixel.type == PIXEL_CODE)
    {
        if (pixel.instr == INSTR_READ || pixel.instr == INSTR_WRITE)
            return exec_mem(rt, pixel.instr, img, w, h);
        return exec_code(rt, pixel.instr, fx, cur_x, cur_y, cur_d);
    }

    return EXEC_OK;
}

const char* exec_status_name(ExecStatus s)
{
    switch (s)
    {
        case EXEC_OK: return "OK";
        case EXEC_HALT: return "HALT";
        case EXEC_ERR_TRAP: return "ERR_TRAP";
        case EXEC_ERR_CALL_UNDERFLOW: return "ERR_CALL_UNDERFLOW";
        case EXEC_ERR_CALL_OVERFLOW: return "ERR_CALL_OVERFLOW";
        case EXEC_ERR_MEM_OOB: return "ERR_MEM_OOB";
        case EXEC_ERR_MEM_ALPHA: return "ERR_MEM_ALPHA";
        case EXEC_ERR_MEM_CTX: return "ERR_MEM_CTX";
        case EXEC_ERR_STACK_UNDERFLOW: return "ERR_STACK_UNDERFLOW";
        case EXEC_ERR_STACK_OVERFLOW: return "ERR_STACK_OVERFLOW";
        case EXEC_ERR_DIV_ZERO: return "ERR_DIV_ZERO";
        case EXEC_ERR_MOD_ZERO: return "ERR_MOD_ZERO";
        case EXEC_ERR_INPUT: return "ERR_INPUT";
        default: return "UNKNOWN_EXEC_STATUS";
    }
}
