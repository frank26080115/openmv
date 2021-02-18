#include "py_mathhelper.h"

#include <stdlib.h>
#include <math.h>

extern int fast_roundf(float x);
extern float fast_sqrtf(float x);

float map_val_float(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int map_val_int(int x, int in_min, int in_max, int out_min, int out_max)
{
    int y = (x - in_min) * (out_max - out_min);
    int div = (in_max - in_min);
    y += div / 2;
    y /= div;
    y += out_min;
    return y;
}

int calc_dist_int(int x1, int y1, int x2, int y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return fast_roundf(fast_sqrtf((dx * dx) + (dy * dy)));
}

float calc_dist_float(float x1, float y1, float x2, float y2)
{
    float dx = x1 - x2;
    float dy = y1 - y2;
    return fast_sqrtf((dx * dx) + (dy * dy));
}
