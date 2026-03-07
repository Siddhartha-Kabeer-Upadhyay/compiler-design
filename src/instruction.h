#ifndef INSTRUCTION_H

#define INSTRUCTION_H
#include "hsv.h"

typedef enum {
    PIXEL_HALT,         // Black (R=0, G=0, B=0)
    PIXEL_ERROR,        // Transparent (A=0)
    PIXEL_DATA,         // S <= 20%
    PIXEL_CODE          // S > 20%
} PixelType;

typedef enum {
    // Direction (Hue 0-96)
    INSTR_RIGHT,        // Red, V < 128
    INSTR_RIGHT_SKIP,   // Red, V >= 128
    INSTR_DOWN,         // Orange, V < 128
    INSTR_DOWN_SKIP,    // Orange, V >= 128
    INSTR_LEFT,         // Yellow, V < 128
    INSTR_LEFT_SKIP,    // Yellow, V >= 128
    INSTR_UP,           // Lime, V < 128
    INSTR_UP_SKIP,      // Lime, V >= 128

    // Stack (Hue 96-120)
    INSTR_POP,          // Green, V < 128
    INSTR_SWAP,         // Green, V >= 128

    // Math (Hue 120-216)
    INSTR_ADD,          // Teal, V < 128
    INSTR_SUB,          // Teal, V >= 128
    INSTR_MUL,          // Cyan, V < 128
    INSTR_DIV,          // Cyan, V >= 128
    INSTR_MOD,          // Sky, V < 128
    INSTR_NEG,          // Sky, V >= 128
    INSTR_INC,          // Blue, V < 128
    INSTR_DEC,          // Blue, V >= 128

    // Registers (Hue 216-264)
    INSTR_STORE_A,      // Indigo, V < 128
    INSTR_LOAD_A,       // Indigo, V >= 128
    INSTR_STORE_B,      // Violet, V < 128
    INSTR_LOAD_B,       // Violet, V >= 128

    // Conditionals (Hue 264-288)
    INSTR_JNZ,          // Purple, V < 128
    INSTR_JZ,           // Purple, V >= 128

    // I/O (Hue 288-336)
    INSTR_IN_NUM,       // Magenta, V < 128
    INSTR_IN_CHR,       // Magenta, V >= 128
    INSTR_OUT_NUM,      // Pink, V < 128
    INSTR_OUT_CHR,      // Pink, V >= 128

    // Control (Hue 336-360)
    INSTR_NOP,          // Rose, V < 128
    INSTR_DEBUG,        // Rose, V >= 128
    INSTR_NONE          // No instruction 
} Instruction;

typedef struct 
{
    PixelType type;
    Instruction instr;  // type == PIXEL_CODE
    int data_value;     // type == PIXEL_DATA (0-255)
} DecodedPixel;

PixelType classify_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a, HSV hsv);
Instruction decode_instruction(HSV hsv);
DecodedPixel decode_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
const char* pixel_type_name(PixelType type);
const char* instruction_name(Instruction instr);

#endif