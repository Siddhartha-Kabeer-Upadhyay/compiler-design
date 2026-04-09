#ifndef UTILS_H

#define UTILS_H // to remove max and min repitition everytime we need the values

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