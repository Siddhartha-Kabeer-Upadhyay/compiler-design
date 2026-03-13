#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "hsv.h"
#include "instruction.h"
#include "tracer.h"

int main(int argc, char *argv[]) 
{
	// check how many args were passed, argv[0] returns the executable name
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.png> [--trace] [step_limit]\n", argv[0]);
        return 1;
    }

	// the whole section deals with passing step limit and trace argument after the first 2 required arguments
	int trace_mode = 0;
    int step_limit = 1000; // default value 
    
    for (int i = 2; i < argc; i++) // finds --trace and step limit
    {
        if (strcmp(argv[i], "--trace") == 0)
            trace_mode = 1;
        else
            step_limit = atoi(argv[i]); //ascii to intenger to deal for argv being char type
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
    
    if(!trace_mode)
    {
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
	            printf("RGBA(%3d,%3d,%3d,%3d) | ", r, g, b, a);
	            printf("HSV(%3d,%3d,%3d) -> ", hsv.h, hsv.s, hsv.v);
	            printf("%s", pixel_type_name(decoded.type));
	            
	            if (decoded.type == PIXEL_CODE)
	                printf(" [%s]", instruction_name(decoded.instr));
	            else if (decoded.type == PIXEL_DATA)
	                printf(" [PUSH %d]", decoded.data_value);
	            printf("\n");
	        }
	    }
	}

	else
    {
        TracerState state = tracer_init();
		printf("Steps:\n");
        for (int step = 0; step < step_limit; step++)
        {
            if (state.halted || state.error) break;

            int idx = (state.y * width + state.x) * 4; // Current pixel from tracer state position
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];
            unsigned char a = img[idx + 3];

            DecodedPixel pixel = decode_pixel(r, g, b, a);

            printf("STEP: %d POS: [%d,%d] DIR: %s TYPE: %s |",
                   step, state.x, state.y,
                   direction_name(state.dir),
                   pixel_type_name(pixel.type));

            if (pixel.type == PIXEL_CODE)
                printf(" INSTR: %s", instruction_name(pixel.instr));
            else if (pixel.type == PIXEL_DATA)
                printf(" PUSH: %d", pixel.data_value);

            printf("\n");

            tracer_step(&state, pixel);
            if (state.halted || state.error) break; // halts preemptively for optimization
            if (!tracer_move(&state, width, height, pixel)) break; // moves pointer
        }
		
        if(state.halted)
            printf("END: HALT at [%d,%d]\n", state.x, state.y);
        else if(state.error)
            printf("END: ERROR at [%d,%d]\n", state.x, state.y);
        else
            printf("END: STEP_LIMIT [%d]\n", step_limit);
    }
    
	// cleanup
    stbi_image_free(img);
    return 0;
}