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

    // EX flow (S > 60, slots 0-1)
    INSTR_JGT,
    INSTR_JLT,

    // EX traps (S > 60, slots 2-3)
    INSTR_TRAP_00,
    INSTR_TRAP_01,

    // EX call (S > 60, slots 4-5)
    INSTR_CALL,
    INSTR_RET,

    // EX traps (S > 60, slots 6-7)
    INSTR_TRAP_02,
    INSTR_TRAP_03,

    // EX stack (S > 60, slots 8-9)
    INSTR_DUP,
    INSTR_OVER,

    // EX traps (S > 60, slots 10-11)
    INSTR_TRAP_04,
    INSTR_TRAP_05,

    // EX stack (S > 60, slots 12-13)
    INSTR_ROT,
    INSTR_ROTR,

    // EX traps (S > 60, slots 14-15)
    INSTR_TRAP_06,
    INSTR_TRAP_07,

    // EX pixel mem (S > 60, slots 16-17)
    INSTR_READ,
    INSTR_WRITE,

    // EX traps (S > 60, slots 18-19)
    INSTR_TRAP_08,
    INSTR_TRAP_09,

    // EX bitwise (S > 60, slots 20-21)
    INSTR_AND,
    INSTR_OR,

    // EX traps (S > 60, slots 22-23)
    INSTR_TRAP_10,
    INSTR_TRAP_11,

    // EX bitwise (S > 60, slots 24-25)
    INSTR_NOT,
    INSTR_XOR,

    // EX traps (S > 60, slots 26-27)
    INSTR_TRAP_12,
    INSTR_TRAP_13,

    // EX stack state (S > 60, slots 28-29)
    INSTR_DEPTH,
    INSTR_CLEAR,

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
