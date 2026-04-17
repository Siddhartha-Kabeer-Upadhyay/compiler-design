#include "opt.h"

int optimize_decoded_program(unsigned char *pixel_type, int *pixel_instr, int *pixel_data,
                             int count, const OptConfig *config, OptStats *stats)
{
    (void)pixel_type;
    (void)pixel_instr;
    (void)pixel_data;
    (void)count;

    if (stats)
    {
        stats->passes_run = 0;
        stats->changes = 0;
    }

    if (!config || !config->enabled)
        return 1;

    if (stats)
        stats->passes_run = 1;

    return 1;
}
