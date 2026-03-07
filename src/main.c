#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "hsv.h"

int main(int argc, char *argv[]) 
{
	// check how many args were passed, argv[0] returns the executable name
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.png>\n", argv[0]);
        return 1;
    }

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 4);

	// image not found
    if (!img) 
	{
        fprintf(stderr, "Error: Could not load image '%s'\n", argv[1]);
        return 1;
    }

	// image information and pixel values
    printf("Loaded: \t%s\n", argv[1]);
    printf("Size: \t\t%d x %d\n", width, height);
    printf("Channels: \t%d \n\n", channels);

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
            
            HSV hsv = rgb_to_hsv(r, g, b); // calling function rgb to hsv from hsv.c
            printf("(%d,%d): R=%3d G=%3d B=%3d A=%3d | H=%3d S=%3d V=%3d\n", x, y, r, g, b, a, hsv.h, hsv.s, hsv.v); 
        }
    }

	// cleanup
    stbi_image_free(img);
    return 0;
}