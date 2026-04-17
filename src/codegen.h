#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct {
    int enable_opt;
} CodegenOptions;

int generate_c_from_image(const char *output_path, const unsigned char *img, int width, int height,
                          const CodegenOptions *options);

#endif
