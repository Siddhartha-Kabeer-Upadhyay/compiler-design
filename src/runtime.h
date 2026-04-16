#ifndef RUNTIME_H

#define RUNTIME_H
#include "instruction.h"
#define STACK_MAX 1024 // random big value, will adjust later

typedef enum
{
    EXEC_OK,
    EXEC_HALT,
    EXEC_ERR_STACK_UNDERFLOW,
    EXEC_ERR_STACK_OVERFLOW,
    EXEC_ERR_DIV_ZERO,
    EXEC_ERR_MOD_ZERO,
    EXEC_ERR_INPUT
} ExecStatus;

typedef struct
{
    int stack[STACK_MAX];
    int sp; // stack pointer for next free index
    int reg_a;
    int reg_b;
    int last_conditional_jump;
} RuntimeState;

void runtime_init(RuntimeState *rt);
int runtime_push(RuntimeState *rt, int value);
int runtime_pop(RuntimeState *rt, int *out);
int runtime_peek(RuntimeState *rt, int *out);

ExecStatus execute_pixel(RuntimeState *rt, DecodedPixel pixel);
const char* exec_status_name(ExecStatus s);

#endif
