#include "runtime.h"
#include <stdlib.h>

void runtime_init(RuntimeState *rt)
{
    rt->sp = 0;
    rt->reg_a = 0;
    rt->reg_b = 0;
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

static ExecStatus exec_code(RuntimeState *rt, Instruction instr)
{
    int a, b;

    switch (instr)
    {
        // direction + skip are tracer concerns only
        case INSTR_RIGHT:
        case INSTR_RIGHT_SKIP:
        case INSTR_DOWN:
        case INSTR_DOWN_SKIP:
        case INSTR_LEFT:
        case INSTR_LEFT_SKIP:
        case INSTR_UP:
        case INSTR_UP_SKIP:
            return EXEC_OK;

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
            if (a == 0) return EXEC_ERR_DIV_ZERO;
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

        // placeholder
        case INSTR_JNZ:
        case INSTR_JZ:
        case INSTR_IN_NUM:
        case INSTR_IN_CHR:
        case INSTR_OUT_NUM:
        case INSTR_OUT_CHR:
        case INSTR_DEBUG:
        case INSTR_NOP:
        case INSTR_NONE:
            return EXEC_OK;
    }

    return EXEC_OK;
}

ExecStatus execute_pixel(RuntimeState *rt, DecodedPixel pixel)
{
    if (pixel.type == PIXEL_HALT) return EXEC_HALT;
    if (pixel.type == PIXEL_ERROR) return EXEC_OK; // tracer will handle this

    if (pixel.type == PIXEL_DATA)
    {
        if (!runtime_push(rt, pixel.data_value)) return EXEC_ERR_STACK_OVERFLOW;
        return EXEC_OK;
    }

    if (pixel.type == PIXEL_CODE)
    {
        return exec_code(rt, pixel.instr);
    }

    return EXEC_OK;
}

const char* exec_status_name(ExecStatus s)
{
    switch (s)
    {
        case EXEC_OK: return "OK";
        case EXEC_HALT: return "HALT";
        case EXEC_ERR_STACK_UNDERFLOW: return "ERR_STACK_UNDERFLOW";
        case EXEC_ERR_STACK_OVERFLOW: return "ERR_STACK_OVERFLOW";
        case EXEC_ERR_DIV_ZERO: return "ERR_DIV_ZERO";
        default: return "UNKNOWN_EXEC_STATUS";
    }
}