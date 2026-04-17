#include "ir.h"
#include "instruction.h"

#include <stdlib.h>

int ir_from_image(const unsigned char *img, int width, int height, IRProgram *out_program)
{
    if (!img || !out_program || width <= 0 || height <= 0)
        return 0;

    int count = width * height;
    // separate arrays keep optimizer mutation simple later
    unsigned char *pixel_type = (unsigned char *)malloc((size_t)count * sizeof(unsigned char));
    int *pixel_instr = (int *)malloc((size_t)count * sizeof(int));
    int *pixel_data = (int *)malloc((size_t)count * sizeof(int));

    if (!pixel_type || !pixel_instr || !pixel_data)
    {
        free(pixel_type);
        free(pixel_instr);
        free(pixel_data);
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        // decode once here to avoid doing same work in many passes
        int idx = i * 4;
        DecodedPixel pixel = decode_pixel(img[idx + 0], img[idx + 1], img[idx + 2], img[idx + 3]);
        pixel_type[i] = (unsigned char)pixel.type;
        pixel_instr[i] = (int)pixel.instr;
        pixel_data[i] = pixel.data_value;
    }

    out_program->width = width;
    out_program->height = height;
    out_program->count = count;
    out_program->pixel_type = pixel_type;
    out_program->pixel_instr = pixel_instr;
    out_program->pixel_data = pixel_data;

    return 1;
}

void ir_free(IRProgram *program)
{
    if (!program)
        return;

    free(program->pixel_type);
    free(program->pixel_instr);
    free(program->pixel_data);

    program->pixel_type = NULL;
    program->pixel_instr = NULL;
    program->pixel_data = NULL;
    program->width = 0;
    program->height = 0;
    program->count = 0;
}
