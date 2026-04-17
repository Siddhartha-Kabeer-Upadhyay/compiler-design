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

static ExecStatus exec_code(RuntimeState *rt, Instruction instr, RouteEffect *fx)
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
        case INSTR_RET:
        case INSTR_READ:
        case INSTR_WRITE:
            return EXEC_OK;

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

ExecStatus execute_pixel(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx)
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
        return exec_code(rt, pixel.instr, fx);
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
        case EXEC_ERR_STACK_UNDERFLOW: return "ERR_STACK_UNDERFLOW";
        case EXEC_ERR_STACK_OVERFLOW: return "ERR_STACK_OVERFLOW";
        case EXEC_ERR_DIV_ZERO: return "ERR_DIV_ZERO";
        case EXEC_ERR_MOD_ZERO: return "ERR_MOD_ZERO";
        case EXEC_ERR_INPUT: return "ERR_INPUT";
        default: return "UNKNOWN_EXEC_STATUS";
    }
}
