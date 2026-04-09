#include "instruction.h"
#include "utils.h"

PixelType classify_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a, HSV hsv) 
{
    if (a == 0)
        return PIXEL_ERROR; // Transparent = error

    if (r == 0 && g == 0 && b == 0) 
        return PIXEL_HALT; // Pure black = halt

    if (hsv.s <= 20) // saturation check
        return PIXEL_DATA;

    return PIXEL_CODE;
}

Instruction decode_instruction(HSV hsv) 
{
    int h = hsv.h;
    int v = hsv.v;

    int high_v = (v >= 50); // 50% Value threshold

    // Hue ranges
    if (h < 24)                     // Red: 0-24
        return high_v ? INSTR_RIGHT_SKIP : INSTR_RIGHT;
    else if (h < 48)                // Orange: 24-48
        return high_v ? INSTR_DOWN_SKIP : INSTR_DOWN;
    else if (h < 72)                // Yellow: 48-72
        return high_v ? INSTR_LEFT_SKIP : INSTR_LEFT;
    else if (h < 96)                // Lime: 72-96
        return high_v ? INSTR_UP_SKIP : INSTR_UP;
    else if (h < 120)               // Green: 96-120
        return high_v ? INSTR_SWAP : INSTR_POP;
    else if (h < 144)               // Teal: 120-144
        return high_v ? INSTR_SUB : INSTR_ADD;
    else if (h < 168)               // Cyan: 144-168
        return high_v ? INSTR_DIV : INSTR_MUL;
    else if (h < 192)               // Sky: 168-192
        return high_v ? INSTR_NEG : INSTR_MOD;
    else if (h < 216)               // Blue: 192-216
        return high_v ? INSTR_DEC : INSTR_INC;
    else if (h < 240)               // Indigo: 216-240
        return high_v ? INSTR_LOAD_A : INSTR_STORE_A;
    else if (h < 264)               // Violet: 240-264
        return high_v ? INSTR_LOAD_B : INSTR_STORE_B;
    else if (h < 288)               // Purple: 264-288
        return high_v ? INSTR_JZ : INSTR_JNZ;
    else if (h < 312)               // Magenta: 288-312
        return high_v ? INSTR_IN_CHR : INSTR_IN_NUM;
    else if (h < 336)               // Pink: 312-336
        return high_v ? INSTR_OUT_CHR : INSTR_OUT_NUM;
    else                            // Rose: 336-360
        return high_v ? INSTR_DEBUG : INSTR_NOP;
}

DecodedPixel decode_pixel(unsigned char r, unsigned char g, unsigned char b, unsigned char a) 
{
    DecodedPixel result;
    result.instr = INSTR_NONE;
    result.data_value = 0;

    HSV hsv = rgb_to_hsv(r, g, b);

    result.type = classify_pixel(r, g, b, a, hsv);

    if (result.type == PIXEL_CODE) 
    {
        result.instr = decode_instruction(hsv);
    }
    else if (result.type == PIXEL_DATA) 
    {
        result.data_value = max3(r, g, b);
    }

    return result;
}

const char* pixel_type_name(PixelType type) 
{
    switch (type) 
    {
        case PIXEL_HALT:  return "HALT";
        case PIXEL_ERROR: return "ERROR";
        case PIXEL_DATA:  return "DATA";
        case PIXEL_CODE:  return "CODE";
        default:          return "UNKNOWN";
    }
}

const char* instruction_name(Instruction instr) 
{
    switch (instr) 
    {
        case INSTR_RIGHT:      return "RIGHT";
        case INSTR_RIGHT_SKIP: return "RIGHT_SKIP";
        case INSTR_DOWN:       return "DOWN";
        case INSTR_DOWN_SKIP:  return "DOWN_SKIP";
        case INSTR_LEFT:       return "LEFT";
        case INSTR_LEFT_SKIP:  return "LEFT_SKIP";
        case INSTR_UP:         return "UP";
        case INSTR_UP_SKIP:    return "UP_SKIP";
        case INSTR_POP:        return "POP";
        case INSTR_SWAP:       return "SWAP";
        case INSTR_ADD:        return "ADD";
        case INSTR_SUB:        return "SUB";
        case INSTR_MUL:        return "MUL";
        case INSTR_DIV:        return "DIV";
        case INSTR_MOD:        return "MOD";
        case INSTR_NEG:        return "NEG";
        case INSTR_INC:        return "INC";
        case INSTR_DEC:        return "DEC";
        case INSTR_STORE_A:    return "STORE_A";
        case INSTR_LOAD_A:     return "LOAD_A";
        case INSTR_STORE_B:    return "STORE_B";
        case INSTR_LOAD_B:     return "LOAD_B";
        case INSTR_JNZ:        return "JNZ";
        case INSTR_JZ:         return "JZ";
        case INSTR_IN_NUM:     return "IN_NUM";
        case INSTR_IN_CHR:     return "IN_CHR";
        case INSTR_OUT_NUM:    return "OUT_NUM";
        case INSTR_OUT_CHR:    return "OUT_CHR";
        case INSTR_NOP:        return "NOP";
        case INSTR_DEBUG:      return "DEBUG";
        case INSTR_NONE:       return "NONE";
        default:               return "UNKNOWN";
    }
}