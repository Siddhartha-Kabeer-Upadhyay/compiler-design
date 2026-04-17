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

static Instruction decode_base(HSV hsv)
{
    int h = hsv.h;
    int high_v = (hsv.v >= 128);

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

static Instruction decode_ex(HSV hsv)
{
    int h = hsv.h;
    int high_v = (hsv.v >= 128);

    if (h < 24) return high_v ? INSTR_TRAP_01 : INSTR_TRAP_00;
    if (h < 48) return high_v ? INSTR_TRAP_03 : INSTR_TRAP_02;
    if (h < 72) return high_v ? INSTR_TRAP_05 : INSTR_TRAP_04;
    if (h < 96) return high_v ? INSTR_TRAP_07 : INSTR_TRAP_06;
    if (h < 120) return high_v ? INSTR_OVER : INSTR_DUP;
    if (h < 144) return high_v ? INSTR_OR : INSTR_AND;
    if (h < 168) return high_v ? INSTR_XOR : INSTR_NOT;
    if (h < 192) return high_v ? INSTR_ROTR : INSTR_ROT;
    if (h < 216) return high_v ? INSTR_WRITE : INSTR_READ;
    if (h < 240) return high_v ? INSTR_LOAD_C : INSTR_STORE_C;
    if (h < 264) return high_v ? INSTR_LOAD_D : INSTR_STORE_D;
    if (h < 288) return high_v ? INSTR_JLT : INSTR_JGT;
    if (h < 312) return high_v ? INSTR_RET : INSTR_CALL;
    if (h < 336) return high_v ? INSTR_TRAP_13 : INSTR_TRAP_12;
    return high_v ? INSTR_CLEAR : INSTR_DEPTH;
}

Instruction decode_instruction(HSV hsv)
{
    if (hsv.s > 60)
        return decode_ex(hsv);
    return decode_base(hsv);
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
        case INSTR_STORE_C:    return "STORE_C";
        case INSTR_LOAD_C:     return "LOAD_C";
        case INSTR_STORE_D:    return "STORE_D";
        case INSTR_LOAD_D:     return "LOAD_D";
        case INSTR_JNZ:        return "JNZ";
        case INSTR_JZ:         return "JZ";
        case INSTR_IN_NUM:     return "IN_NUM";
        case INSTR_IN_CHR:     return "IN_CHR";
        case INSTR_OUT_NUM:    return "OUT_NUM";
        case INSTR_OUT_CHR:    return "OUT_CHR";
        case INSTR_NOP:        return "NOP";
        case INSTR_DEBUG:      return "DEBUG";
        case INSTR_JGT:        return "JGT";
        case INSTR_JLT:        return "JLT";
        case INSTR_TRAP_00:    return "TRAP_00";
        case INSTR_TRAP_01:    return "TRAP_01";
        case INSTR_CALL:       return "CALL";
        case INSTR_RET:        return "RET";
        case INSTR_TRAP_02:    return "TRAP_02";
        case INSTR_TRAP_03:    return "TRAP_03";
        case INSTR_DUP:        return "DUP";
        case INSTR_OVER:       return "OVER";
        case INSTR_TRAP_04:    return "TRAP_04";
        case INSTR_TRAP_05:    return "TRAP_05";
        case INSTR_ROT:        return "ROT";
        case INSTR_ROTR:       return "ROTR";
        case INSTR_TRAP_06:    return "TRAP_06";
        case INSTR_TRAP_07:    return "TRAP_07";
        case INSTR_READ:       return "READ";
        case INSTR_WRITE:      return "WRITE";
        case INSTR_TRAP_08:    return "TRAP_08";
        case INSTR_TRAP_09:    return "TRAP_09";
        case INSTR_AND:        return "AND";
        case INSTR_OR:         return "OR";
        case INSTR_TRAP_10:    return "TRAP_10";
        case INSTR_TRAP_11:    return "TRAP_11";
        case INSTR_NOT:        return "NOT";
        case INSTR_XOR:        return "XOR";
        case INSTR_TRAP_12:    return "TRAP_12";
        case INSTR_TRAP_13:    return "TRAP_13";
        case INSTR_DEPTH:      return "DEPTH";
        case INSTR_CLEAR:      return "CLEAR";
        case INSTR_NONE:       return "NONE";
        default:               return "UNKNOWN";
    }
}
