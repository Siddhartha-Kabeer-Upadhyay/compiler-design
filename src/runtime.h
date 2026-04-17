#ifndef RUNTIME_H

#define RUNTIME_H
#include "instruction.h"
#define STACK_MAX 1024 // random big value, will adjust later
#define CALL_MAX 256

typedef enum
{
    EXEC_OK,
    EXEC_HALT,
    EXEC_ERR_TRAP,
    EXEC_ERR_CALL_UNDERFLOW,
    EXEC_ERR_CALL_OVERFLOW,
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
    int reg_c;
    int reg_d;
    int call_x[CALL_MAX];
    int call_y[CALL_MAX];
    int call_d[CALL_MAX];
    int csp;
} RuntimeState;

typedef struct
{
    int set_dir;
    int dir;
    int do_skip;
    int do_cond;
    int do_tp;
    int tp_x;
    int tp_y;
    int tp_dir;
} RouteEffect;

void runtime_init(RuntimeState *rt);
int runtime_push(RuntimeState *rt, int value);
int runtime_pop(RuntimeState *rt, int *out);
int runtime_peek(RuntimeState *rt, int *out);
void route_reset(RouteEffect *fx);

ExecStatus execute_pixel(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx);
ExecStatus execute_pixel_at(RuntimeState *rt, DecodedPixel pixel, RouteEffect *fx,
                            int cur_x, int cur_y, int cur_d);
const char* exec_status_name(ExecStatus s);

#endif
