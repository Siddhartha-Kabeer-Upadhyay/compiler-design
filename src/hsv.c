#include "hsv.h"

HSV rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b) // converts rgb to hsv
{
    HSV result;
    int max, min, diff;

    
    max = r; // Find max in R, G, B
    if (g > max) max = g;
    if (b > max) max = b;

    min = r; // Find min in R, G, B
    if (g < min) min = g;
    if (b < min) min = b;

    diff = max - min;


    result.v = (max * 100) / 255; // Value

    
    if (max == 0) // Saturation 
        result.s = 0;
	else
        result.s = (diff * 100) / max; 

    
    if (diff == 0) // Hue 
    	result.h = 0; 
    else if (max == r)
        result.h = (60 * ((g - b) * 100 / diff) + 36000) % 36000 / 100;
	else if (max == g)
        result.h = (60 * ((b - r) * 100 / diff) + 12000) / 100;
    else
        result.h = (60 * ((r - g) * 100 / diff) + 24000) / 100;

    return result;
}