#ifndef HSV_H //if not defined 
#define HSV_H

typedef struct {
    int h;  // Hue: 0-360 Degrees
    int s;  // Saturation: 0-100
    int v;  // Value: 0-100
} HSV;

HSV rgb_to_hsv(unsigned char r, unsigned char g, unsigned char b); //called by hsv.c

#endif