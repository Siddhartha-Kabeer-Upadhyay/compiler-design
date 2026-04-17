#ifndef TRACER_H

#define TRACER_H
#include "runtime.h"

typedef enum
{
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT,
    DIR_UP
} Direction;

typedef struct
{
    int x;
    int y;
    Direction dir;
    int halted;
    int error;
} TracerState;

typedef struct
{
    int x;
    int y;
    DecodedPixel pixel;
} TracedStep;

TracerState tracer_init(void);
int tracer_apply(TracerState *st, int w, int h, const RouteEffect *fx);
const char* direction_name(Direction dir);

#endif
