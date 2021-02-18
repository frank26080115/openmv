#ifndef _PY_MATHHELPER_H_
#define _PY_MATHHELPER_H_

extern int fast_roundf(float x);
extern float fast_sqrtf(float x);

float map_val_float(float x, float in_min, float in_max, float out_min, float out_max);
int map_val_int(int x, int in_min, int in_max, int out_min, int out_max);
int calc_dist_int(int x1, int y1, int x2, int y2);
float calc_dist_float(float x1, float y1, float x2, float y2);

#endif