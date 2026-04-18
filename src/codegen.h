#ifndef CODEGEN_H
#define CODEGEN_H

#include "opt.h"

typedef struct {
    int enable_opt;
    int opt_level;
    int opt_report;
    int fast_c;
    OptStats opt_stats;
} CodegenOptions;

int generate_c_from_image(const char *output_path, const unsigned char *img, int width, int height,
                          CodegenOptions *options);

#endif
