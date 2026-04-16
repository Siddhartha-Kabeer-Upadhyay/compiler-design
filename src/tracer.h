#ifndef TRACER_H

#define TRACER_H
#include "instruction.h" // for directions and DecodedPixel

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
void tracer_step(TracerState *state, DecodedPixel pixel);
int tracer_move(TracerState *state, int width, int height, DecodedPixel pixel);
int tracer_move_conditional(TracerState *state, int width, int height, int should_move);
const char* direction_name(Direction dir);

#endif
