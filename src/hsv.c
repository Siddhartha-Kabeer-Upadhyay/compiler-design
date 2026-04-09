#include "hsv.h"
#include "utils.h"

HSV rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b) // converts rgb to hsv with math
{
    HSV result;
    
    int max = max3(r, g, b); // max of rgb
    int min = min3(r, g, b); // min of rgb
    int diff = max - min;

	// Value
    result.v = (max * 100) / 255; 

	// Saturation 
    if (max == 0) 
        result.s = 0;
	else
        result.s = (diff * 100) / max; 

	// Hue 
    if (diff == 0) 
    	result.h = 0; 
    else if (max == r)
        result.h = (60 * ((g - b) * 100 / diff) + 36000) % 36000 / 100;
	else if (max == g)
        result.h = (60 * ((b - r) * 100 / diff) + 12000) / 100;
    else
        result.h = (60 * ((r - g) * 100 / diff) + 24000) / 100;

    return result;
}