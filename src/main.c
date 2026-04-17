#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "hsv.h"
#include "instruction.h"
#include "tracer.h"
#include "runtime.h"
#include "codegen.h"

int main(int argc, char *argv[]) 
{
	// check how many args were passed, argv[0] returns the executable name
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image.png> [--run|--trace [step_limit]|--dump] [--opt] [--opt-level 0|1|2] [--opt-report] [-o output.c]\n", argv[0]);
        return 1;
    }

	// the whole section deals with passing step limit and trace argument after the first 2 required arguments
	int trace_mode = 0;
    int dump_mode = 0;
    int run_mode = 0;
    int codegen_mode = 0;
    int opt_mode = 0;
    int opt_report = 0;
    int opt_level = -1;
    const char *codegen_out = NULL;
    int step_limit = 1000; // default value 
    int step_limit_set = 0;
    
    for (int i = 2; i < argc; i++) // finds --trace and step limit
    {
        if (strcmp(argv[i], "--trace") == 0)
        {
            if (dump_mode || run_mode)
            {
                fprintf(stderr, "Error: --trace cannot be combined with --run/--dump\n");
                return 1;
            }
            trace_mode = 1;
        }
        else if (strcmp(argv[i], "--run") == 0)
        {
            if (trace_mode || dump_mode)
            {
                fprintf(stderr, "Error: --run cannot be combined with --trace/--dump\n");
                return 1;
            }
            run_mode = 1;
        }
        else if (strcmp(argv[i], "--dump") == 0)
        {
            if (trace_mode || run_mode)
            {
                fprintf(stderr, "Error: --dump cannot be combined with --run/--trace\n");
                return 1;
            }
            dump_mode = 1;
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Missing output path after -o\n");
                return 1;
            }
            codegen_mode = 1;
            codegen_out = argv[++i];
        }
        else if (strcmp(argv[i], "--opt") == 0)
        {
            opt_mode = 1;
        }
        else if (strcmp(argv[i], "--opt-report") == 0)
        {
            opt_report = 1;
        }
        else if (strcmp(argv[i], "--opt-level") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: Missing value after --opt-level\n");
                return 1;
            }

            char *end = NULL;
            long parsed_level = strtol(argv[++i], &end, 10);
            if (*end != '\0' || parsed_level < 0 || parsed_level > 2)
            {
                fprintf(stderr, "Error: Invalid --opt-level '%s' (expected 0, 1, or 2)\n", argv[i]);
                return 1;
            }
            opt_level = (int)parsed_level;
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Error: Unknown flag '%s'\n", argv[i]);
            return 1;
        }
        else
        {
            char *end = NULL;
            long parsed = strtol(argv[i], &end, 10); // parse numeric step limit 
            if (*argv[i] == '\0' || *end != '\0' || parsed <= 0)
            {
                fprintf(stderr, "Error: Invalid step_limit '%s'\n", argv[i]);
                return 1;
            }
            if (step_limit_set)
            {
                fprintf(stderr, "Error: step_limit provided multiple times\n");
                return 1;
            }
            step_limit = (int)parsed; // typecasting because errors without it
            step_limit_set = 1;
        }
    }

    if (!trace_mode && !dump_mode && !run_mode)
        run_mode = 1;

    if (step_limit_set && !trace_mode)
    {
        fprintf(stderr, "Error: step_limit can only be used with --trace\n");
        return 1;
    }

    if (opt_mode && !codegen_mode)
    {
        fprintf(stderr, "Error: --opt can only be used with -o\n");
        return 1;
    }

    if (opt_report && !codegen_mode)
    {
        fprintf(stderr, "Error: --opt-report can only be used with -o\n");
        return 1;
    }

    if (opt_level >= 0 && !codegen_mode)
    {
        fprintf(stderr, "Error: --opt-level can only be used with -o\n");
        return 1;
    }

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 4);


    if (!img) // image not found
	{
        fprintf(stderr, "Error: Could not load image '%s'\n", argv[1]);
        return 1;
    }

    if (codegen_mode)
    {
        CodegenOptions cg_options;
        cg_options.enable_opt = opt_mode || (opt_level > 0);
        cg_options.opt_level = (opt_level >= 0) ? opt_level : (cg_options.enable_opt ? 1 : 0);
        cg_options.opt_report = opt_report;
        // codegen path exits early because run/trace modes are different pipeline
        int ok = generate_c_from_image(codegen_out, img, width, height, &cg_options);
        stbi_image_free(img);
        if (!ok)
        {
            fprintf(stderr, "Error: Failed to generate C file '%s'\n", codegen_out);
            return 1;
        }

        if (opt_report)
        {
            printf("OPT_REPORT: passes=%d changes=%d nops=%d dirs=%d lit=%d removed=%d dims=%dx%d->%dx%d\n",
                   cg_options.opt_stats.passes_run,
                   cg_options.opt_stats.changes,
                   cg_options.opt_stats.can_nops,
                   cg_options.opt_stats.can_dirs,
                   cg_options.opt_stats.lit_ops,
                   cg_options.opt_stats.removed_cells,
                   cg_options.opt_stats.width_before,
                   cg_options.opt_stats.height_before,
                   cg_options.opt_stats.width_after,
                   cg_options.opt_stats.height_after);
        }
        return 0;
    }

    if(dump_mode)
    {
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

    else if(trace_mode || run_mode)
    {
        TracerState state = tracer_init();
        RuntimeState rt;
        runtime_init(&rt);
        int step = 0;
        int reached_step_limit = 0;

        if (trace_mode)
        {
            printf("Loaded: \t%s\n", argv[1]);
            printf("Size: \t\t%d x %d\n", width, height);
		    printf("Steps:\n");
        }

	    while (1)
        {
            if (trace_mode && step >= step_limit)
            {
                reached_step_limit = 1;
                break;
            }

            if (state.halted || state.error) break;

            int idx = (state.y * width + state.x) * 4; // Current pixel from tracer state position
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];
            unsigned char a = img[idx + 3];

            DecodedPixel pixel = decode_pixel(r, g, b, a);
			ExecStatus exec = execute_pixel(&rt, pixel);
			if (exec != EXEC_OK && exec != EXEC_HALT)
			{
			    fprintf(stderr, "EXEC_ERROR: %s\n", exec_status_name(exec));
			    state.error = 1;
			    break;
			}

            if (trace_mode)
            {
			    printf("STEP: %d POS: [%d,%d] DIR: %s TYPE: %s",
			           step, state.x, state.y,
			           direction_name(state.dir),
			           pixel_type_name(pixel.type));
			
			    if (pixel.type == PIXEL_CODE)
			        printf(" | INSTR: %s", instruction_name(pixel.instr));
			    else if (pixel.type == PIXEL_DATA)
			        printf(" | PUSH: %d", pixel.data_value);
			
			    if (rt.sp > 0) // top check for debugging
			        printf(" | TOP: %d", rt.stack[rt.sp - 1]);
			    else
			        printf(" | TOP: EMPTY");
			
			    printf(" | SP: %d | A: %d | B: %d", rt.sp, rt.reg_a, rt.reg_b);
                printf("\n");
            }

            tracer_step(&state, pixel);
            if (exec == EXEC_HALT) { state.halted = 1; break; } // breaks preemptively for optimization
            if (state.halted || state.error) break; // breaks preemptively for optimization
            if (!tracer_move(&state, width, height, pixel)) break; // moves pointer else breaks if cannot
            if (!tracer_move_conditional(&state, width, height, rt.last_conditional_jump)) break;
            step++;
        }

        if (trace_mode)
        {
            if(state.halted)
                printf("END: HALT at [%d,%d]\n", state.x, state.y);
            else if(state.error)
                printf("END: ERROR at [%d,%d]\n", state.x, state.y);
            else if (reached_step_limit)
                printf("END: STEP_LIMIT [%d]\n", step_limit);
        }

        // cleanup
        stbi_image_free(img);

        if (state.halted)
            return 0;

        return 1;
    }
    
    else // in case of exceptions 
    {
	    printf("UNREACHABLE POSITION REACHED.");
    	return 1;
	}
	
	// cleanup
    stbi_image_free(img);
    return 0;
}
