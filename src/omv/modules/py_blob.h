#ifndef _PY_BLOB_H_
#define _PY_BLOB_H_

#include <imlib.h>

#ifdef ASTROPHOTOGEAR
#define py_blob_obj_size 12
typedef struct py_blob_obj {
    mp_obj_base_t base;
    mp_obj_t corners;
    mp_obj_t min_corners;
    mp_obj_t x, y, w, h, pixels, cx, cy;
    mp_obj_t maxbrightness, brightness, saturation_cnt;
    mp_obj_t star_profile, star_pointiness;
    #ifndef ASTROPHOTOGEAR
    mp_obj_t rotation;
    #endif
    #ifndef ASTROPHOTOGEAR
    mp_obj_t code;
    #endif
    #ifndef ASTROPHOTOGEAR
    mp_obj_t count;
    #endif
    #ifndef ASTROPHOTOGEAR
    mp_obj_t perimeter, roundness;
    mp_obj_t x_hist_bins;
    mp_obj_t y_hist_bins;
    #endif
} py_blob_obj_t;
#endif

#endif