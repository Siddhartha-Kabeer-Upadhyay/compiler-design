#ifndef OPT_H
#define OPT_H

typedef struct {
    int enabled;
} OptConfig;

typedef struct {
    int passes_run;
    int changes;
} OptStats;

int optimize_decoded_program(unsigned char *pixel_type, int *pixel_instr, int *pixel_data, int count, const OptConfig *config, OptStats *stats);

#endif
