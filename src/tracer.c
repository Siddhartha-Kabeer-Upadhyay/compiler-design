#include "tracer.h"
#include "utils.h"

static int tracer_in_bounds(TracerState *state, int width, int height)
{ return in_bounds(state->x, state->y, width, height); }

static void tracer_move_once(TracerState *state)
{
    move_once(&state->x, &state->y, state->dir);
}

TracerState tracer_init(void)
{
    TracerState state;
    state.x = 0;
    state.y = 0;
    state.dir = DIR_RIGHT;
    state.halted = 0;
    state.error = 0;
    return state;
}

void tracer_step(TracerState *state, DecodedPixel pixel)
{
    if (pixel.type == PIXEL_HALT)
    {
        state->halted = 1;
        return;
    }

    if (pixel.type == PIXEL_ERROR)
    {
        state->error = 1;
        return;
    }

    if (pixel.type == PIXEL_CODE)
    {
        switch (pixel.instr)
        {
            case INSTR_RIGHT:
            case INSTR_RIGHT_SKIP:
				state->dir = DIR_RIGHT; 
				break;

            case INSTR_DOWN:
            case INSTR_DOWN_SKIP:
				state->dir = DIR_DOWN;  
				break;

            case INSTR_LEFT:
            case INSTR_LEFT_SKIP:
				state->dir = DIR_LEFT;  
				break;

            case INSTR_UP:
            case INSTR_UP_SKIP:
				state->dir = DIR_UP;    
				break;

            default: 
				break;
        }
    }
}

int tracer_move(TracerState *state, int width, int height, DecodedPixel pixel)
{
    tracer_move_once(state); // every pixel moves the tracer but only code type can have skip jumps

    if (!tracer_in_bounds(state, width, height))
    {
        state->error = 1;
        return 0;
    }

    if (pixel.type == PIXEL_CODE && instr_is_skip(pixel.instr))
    {
        tracer_move_once(state);

        if (!tracer_in_bounds(state, width, height))
        {
            state->error = 1;
            return 0;
        }
    }

    return 1;
}

int tracer_move_conditional(TracerState *state, int width, int height, int should_move)
{
    if (!should_move) return 1;

    tracer_move_once(state);

    if (!tracer_in_bounds(state, width, height))
    {
        state->error = 1;
        return 0;
    }

    return 1;
}

const char* direction_name(Direction dir)
{
    switch (dir)
    {
        case DIR_RIGHT: return "RIGHT";
        case DIR_DOWN:  return "DOWN";
        case DIR_LEFT:  return "LEFT";
        case DIR_UP:    return "UP";
        default:        return "UNKNOWN";
    }
}
