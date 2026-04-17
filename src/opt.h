#ifndef OPT_H
#define OPT_H

#include "instruction.h"

typedef struct {
    int enabled;
} OptConfig;

typedef struct {
    int passes_run;
    int changes;
    int canonicalized_nops;
    int removed_cells;
    int width_before;
    int height_before;
    int width_after;
    int height_after;
} OptStats;

int optimize_decoded_program(unsigned char **pixel_type, int **pixel_instr, int **pixel_data,
                             int *width, int *height, const OptConfig *config, OptStats *stats);

#endif
