#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define MAX_ROWS 1024
#define MAX_COLS 1024

typedef struct 
{
    const char *name;
    int h;
    int vmin, vmax;
    int ex; // 1 = Extended, 0 = Base
} Op;

// Hue, Value boundary, EX flag
Op ops[] = {
    // Base Bank
    {"RIGHT", 12, 20, 120, 0},      {"RIGHT_SKIP", 12, 140, 255, 0},
    {"DOWN", 36, 20, 120, 0},       {"DOWN_SKIP", 36, 140, 255, 0},
    {"LEFT", 60, 20, 120, 0},       {"LEFT_SKIP", 60, 140, 255, 0},
    {"UP", 84, 20, 120, 0},         {"UP_SKIP", 84, 140, 255, 0},
    {"POP", 108, 20, 120, 0},       {"SWAP", 108, 140, 255, 0},
    {"ADD", 132, 20, 120, 0},       {"SUB", 132, 140, 255, 0},
    {"MUL", 156, 20, 120, 0},       {"DIV", 156, 140, 255, 0},
    {"MOD", 180, 20, 120, 0},       {"NEG", 180, 140, 255, 0},
    {"INC", 204, 20, 120, 0},       {"DEC", 204, 140, 255, 0},
    {"STORE_A", 228, 20, 120, 0},   {"LOAD_A", 228, 140, 255, 0},
    {"STORE_B", 252, 20, 120, 0},   {"LOAD_B", 252, 140, 255, 0},
    {"JNZ", 276, 20, 120, 0},       {"JZ", 276, 140, 255, 0},
    {"IN_NUM", 300, 20, 120, 0},    {"IN_CHR", 300, 140, 255, 0},
    {"OUT_NUM", 324, 20, 120, 0},   {"OUT_CHR", 324, 140, 255, 0},
    {"NOP", 348, 20, 120, 0},       {"DEBUG", 348, 140, 255, 0},
    {"HALT", 0, 0, 0, 0},           {"ERROR", 0, 0, 0, 0},

    // Extended Bank
    {"TRAP_00", 12, 20, 120, 1},    {"TRAP_01", 12, 140, 255, 1},
    {"TRAP_02", 36, 20, 120, 1},    {"TRAP_03", 36, 140, 255, 1},
    {"TRAP_04", 60, 20, 120, 1},    {"TRAP_05", 60, 140, 255, 1},
    {"TRAP_06", 84, 20, 120, 1},    {"TRAP_07", 84, 140, 255, 1},
    {"DUP", 108, 20, 120, 1},       {"OVER", 108, 140, 255, 1},
    {"AND", 132, 20, 120, 1},       {"OR", 132, 140, 255, 1},
    {"NOT", 156, 20, 120, 1},       {"XOR", 156, 140, 255, 1},
    {"ROT", 180, 20, 120, 1},       {"ROTR", 180, 140, 255, 1},
    {"READ", 204, 20, 120, 1},      {"WRITE", 204, 140, 255, 1},
    {"STORE_C", 228, 20, 120, 1},   {"LOAD_C", 228, 140, 255, 1},
    {"STORE_D", 252, 20, 120, 1},   {"LOAD_D", 252, 140, 255, 1},
    {"JGT", 276, 20, 120, 1},       {"JLT", 276, 140, 255, 1},
    {"CALL", 300, 20, 120, 1},      {"RET", 300, 140, 255, 1},
    {"TRAP_12", 324, 20, 120, 1},   {"TRAP_13", 324, 140, 255, 1},
    {"DEPTH", 348, 20, 120, 1},     {"CLEAR", 348, 140, 255, 1}
};
int n_ops = sizeof(ops) / sizeof(ops[0]);

typedef struct 
{ 
    const char *alias; 
    const char *real; 
} Alias;

Alias als[] = {
    {">", "RIGHT"}, {"v", "DOWN"}, {"<", "LEFT"}, {"^", "UP"},
    {">>", "RIGHT_SKIP"}, {"vv", "DOWN_SKIP"}, {"<<", "LEFT_SKIP"}, {"^^", "UP_SKIP"},
    {"+", "ADD"}, {"-", "SUB"}, {"*", "MUL"}, {"/", "DIV"}, {"%", "MOD"},
    {".", "NOP"}, {"!", "HALT"}, {"?", "DEBUG"}
};
int n_als = sizeof(als) / sizeof(als[0]);

int is_num(const char *s) 
{
    if (*s == '-') s++;
    if (!*s) return 0;
    while (*s) {
        if (!isdigit((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

char *dup_str(const char *s) 
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) 
{
    float c = (v / 255.0f) * (s / 100.0f);
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = (v / 255.0f) - c;
    float rf = 0, gf = 0, bf = 0;

    if      (h < 60)  { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else              { rf = c; gf = 0; bf = x; }

    *r = (uint8_t)roundf((rf + m) * 255.0f);
    *g = (uint8_t)roundf((gf + m) * 255.0f);
    *b = (uint8_t)roundf((bf + m) * 255.0f);
}

int main(int argc, char *argv) 
{
    if (argc != 3) {
        printf("Usage: %s <source.gasm> <output.png>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        printf("Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    // Dynamic grid allocation
    char ***grid = calloc(MAX_ROWS, sizeof(char**));
    for(int i=0; i<MAX_ROWS; i++) grid[i] = calloc(MAX_COLS, sizeof(char*));
    
    int row_lens[MAX_ROWS] = {0};
    int h_out = 0, w_out = 0;
    char ln[4096];

    while(fgets(ln, sizeof(ln), f) && h_out < MAX_ROWS) {
        char *cmt = strchr(ln, '#');
        if (cmt) *cmt = '\0';
        
        int c_out = 0;
        char *tok = strtok(ln, " \t\r\n");
        while(tok && c_out < MAX_COLS) {
            char *cp = dup_str(tok);
            if (!cp) {
                printf("Error: OOM on tokens.\n");
                fclose(f);
                return 1;
            }
            grid[h_out][c_out++] = cp;
            tok = strtok(NULL, " \t\r\n");
        }
        
        if (c_out > 0) {
            row_lens[h_out] = c_out;
            if (c_out > w_out) w_out = c_out;
            h_out++;
        }
    }
    fclose(f);

    if (h_out == 0 || w_out == 0) {
        printf("Error: Empty source.\n");
        return 1;
    }

    uint8_t *img = calloc(w_out * h_out * 4, 1);
    
    // Find NOP fallback
    int nop_i = 0;
    for(int i = 0; i < n_ops; i++) if(strcmp("NOP", ops[i].name) == 0) nop_i = i;

    for (int y = 0; y < h_out; y++) {
        for (int x = 0; x < w_out; x++) {
            int px = (y * w_out + x) * 4;
            char *tok = (x < row_lens[y]) ? grid[y][x] : "NOP";
            
            char utok[64];
            strncpy(utok, tok, 63); utok[63] = '\0';
            for(int i=0; utok[i]; i++) utok[i] = toupper(utok[i]);
            
            const char *ftok = utok;
            for(int i=0; i<n_als; i++) {
                if(strcmp(utok, als[i].alias) == 0) {
                    ftok = als[i].real;
                    break;
                }
            }

            uint8_t r = 0, g = 0, b = 0, a = 255;

            if (strcmp(ftok, "HALT") == 0) {
                r = 0; g = 0; b = 0; a = 255;
            } else if (strcmp(ftok, "ERROR") == 0) {
                r = 0; g = 0; b = 0; a = 0;
            } else if (strncmp(ftok, "PUSH_", 5) == 0 || is_num(ftok)) {
                int val = strncmp(ftok, "PUSH_", 5) == 0 ? atoi(ftok + 5) : atoi(ftok);
                if (val < 0) val = 0; 
                if (val > 255) val = 255;

                r = (uint8_t)val; g = (uint8_t)val; b = (uint8_t)val;
                
                // Hardware execution safety: Max channel must strictly match value
                if (r >= g && r >= b) r = val;
                else if (g >= r && g >= b) g = val;
                else b = val;
            } else {
                int found = 0;
                for(int i=0; i<n_ops; i++) {
                    if(strcmp(ftok, ops[i].name) == 0) {
                        float hue = (float)ops[i].h;
                        if (hue < 0) hue += 360;
                        if (hue >= 360) hue -= 360;

                        // Struct directly dictates target saturation bank
                        float sat = ops[i].ex ? 88.0f : 40.0f;
                        float val = (ops[i].vmin + ops[i].vmax) / 2.0f;
                        
                        hsv2rgb(hue, sat, val, &r, &g, &b);
                        found = 1; break;
                    }
                }
                if(!found) {
                    printf("Warning: Unknown token '%s' at (%d,%d). Defaulting to NOP.\n", tok, x, y);
                    float hue = (float)ops[nop_i].h;
                    if (hue < 0) hue += 360;
                    if (hue >= 360) hue -= 360;
                    
                    float sat = 40.0f;
                    float val = (ops[nop_i].vmin + ops[nop_i].vmax) / 2.0f;
                    hsv2rgb(hue, sat, val, &r, &g, &b);
                }
            }

            img[px] = r; img[px+1] = g; img[px+2] = b; img[px+3] = a;
        }
    }
    
    stbi_write_png(argv[2], w_out, h_out, 4, img, w_out * 4);
    printf("Assembled %s -> %s (%dx%d)\n", argv[1], argv[2], w_out, h_out);
    
    // Cleanup
    free(img);
    for (int y = 0; y < MAX_ROWS; y++) {
        for (int x = 0; x < MAX_COLS; x++) {
            if (grid[y][x]) free(grid[y][x]);
        }
        free(grid[y]);
    }
    free(grid);

    return 0;
}