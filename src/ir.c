#include "ir.h"
#include "instruction.h"

#include <stdlib.h>

int ir_from_image(const unsigned char *img, int w, int h, IRProgram *out)
{
    if (!img || !out || w <= 0 || h <= 0)
        return 0;

    int n = w * h;
    // separate arrays keep optimizer mutation simple later
    unsigned char *type = (unsigned char *)malloc((size_t)n * sizeof(unsigned char));
    int *instr = (int *)malloc((size_t)n * sizeof(int));
    int *data = (int *)malloc((size_t)n * sizeof(int));

    if (!type || !instr || !data)
    {
        free(type);
        free(instr);
        free(data);
        return 0;
    }

    for (int i = 0; i < n; i++)
    {
        // decode once here to avoid doing same work in many passes
        int idx = i * 4;
        DecodedPixel pixel = decode_pixel(img[idx + 0], img[idx + 1], img[idx + 2], img[idx + 3]);
        type[i] = (unsigned char)pixel.type;
        instr[i] = (int)pixel.instr;
        data[i] = pixel.data_value;
    }

    out->width = w;
    out->height = h;
    out->count = n;
    out->pixel_type = type;
    out->pixel_instr = instr;
    out->pixel_data = data;

    return 1;
}

void ir_free(IRProgram *ir)
{
    if (!ir)
        return;

    free(ir->pixel_type);
    free(ir->pixel_instr);
    free(ir->pixel_data);

    ir->pixel_type = NULL;
    ir->pixel_instr = NULL;
    ir->pixel_data = NULL;
    ir->width = 0;
    ir->height = 0;
    ir->count = 0;
}
