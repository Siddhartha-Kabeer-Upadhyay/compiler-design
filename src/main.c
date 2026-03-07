#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "hsv.h"
#include "instruction.h"

int main(int argc, char *argv[]) 
{
	// check how many args were passed, argv[0] returns the executable name
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.png>\n", argv[0]);
        return 1;
    }

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 4);


    if (!img) // image not found
	{
        fprintf(stderr, "Error: Could not load image '%s'\n", argv[1]);
        return 1;
    }

	// image information and pixel values
    printf("Loaded: \t%s\n", argv[1]);
    printf("Size: \t\t%d x %d\n", width, height);
  
    printf("Pixels:\n");
    for (int y = 0; y < height; y++) 
	{
        for (int x = 0; x < width; x++) 
		{
            int idx = (y * width + x) * 4;
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];
            unsigned char a = img[idx + 3];
            
            HSV hsv = rgb_to_hsv(r, g, b); // calling function for hsv resolution from hsv.c
            DecodedPixel decoded = decode_pixel(r, g, b, a); // calling fuction for instruction resolution from instruction.c
            
            printf("(%d,%d): ", x, y);
            printf("RGB(%3d,%3d,%3d) | ", r, g, b);
            printf("HSV(%3d,%3d,%3d) -> ", hsv.h, hsv.s, hsv.v);
            printf("%s", pixel_type_name(decoded.type));
            
            if (decoded.type == PIXEL_CODE)
                printf(" [%s]", instruction_name(decoded.instr));
            else if (decoded.type == PIXEL_DATA)
                printf(" [PUSH %d]", decoded.data_value);
            printf("\n");
        }
    }

	// cleanup
    stbi_image_free(img);
    return 0;
}