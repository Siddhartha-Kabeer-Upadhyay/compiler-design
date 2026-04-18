#include "tracer.h"
#include "utils.h"

static int tracer_in_bounds(TracerState *st, int w, int h)
{ return in_bounds(st->x, st->y, w, h); }

static void tracer_move_once(TracerState *st)
{
    move_once(&st->x, &st->y, st->dir);
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

int tracer_apply(TracerState *st, int w, int h, const RouteEffect *fx)
{
    if (fx && fx->do_tp)
    {
        st->x = fx->tp_x;
        st->y = fx->tp_y;
        st->dir = (Direction)fx->tp_dir;
        if (!tracer_in_bounds(st, w, h))
        {
            st->error = 1;
            return 0;
        }
        return 1;
    }

    if (fx && fx->set_dir)
        st->dir = (Direction)fx->dir;

    tracer_move_once(st);
    if (!tracer_in_bounds(st, w, h))
    {
        st->error = 1;
        return 0;
    }

    if (fx && fx->do_skip)
    {
        tracer_move_once(st);
        if (!tracer_in_bounds(st, w, h))
        {
            st->error = 1;
            return 0;
        }
    }

    if (fx && fx->do_cond)
    {
        tracer_move_once(st);
        if (!tracer_in_bounds(st, w, h))
        {
            st->error = 1;
            return 0;
        }
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
