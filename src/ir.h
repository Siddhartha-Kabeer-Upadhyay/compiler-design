#ifndef IR_H
#define IR_H

typedef struct {
    int width;
    int height;
    int count;
    unsigned char *pixel_type;
    int *pixel_instr;
    int *pixel_data;
} IRProgram;

int ir_from_image(const unsigned char *img, int width, int height, IRProgram *out_program);
void ir_free(IRProgram *program);

#endif
