#ifndef UTILS_H

#define UTILS_H // to remove max and min repitition everytime we need the values

#include "instruction.h"

static inline int in_bounds(int x, int y, int width, int height)
{
    return !(x < 0 || x >= width || y < 0 || y >= height);
}

static inline void move_once(int *x, int *y, int dir)
{
    switch (dir)
    {
        case 0: (*x)++; break;
        case 1: (*y)++; break;
        case 2: (*x)--; break;
        case 3: (*y)--; break;
    }
}

static inline int instr_is_skip(int instr)
{
    return instr == INSTR_RIGHT_SKIP || instr == INSTR_DOWN_SKIP || instr == INSTR_LEFT_SKIP || instr == INSTR_UP_SKIP;
}

static inline int max3(int a, int b, int c) 
{
    int max = a;
    if (b > max) max = b;
    if (c > max) max = c;
    return max;
}

static inline int min3(int a, int b, int c) 
{
    int min = a;
    if (b < min) min = b;
    if (c < min) min = c;
    return min;
}

#endif
