#include "py_guidestar.h"
#include "py_blob.h"

#include "py/obj.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <arm_math.h>
#include "py/runtime.h"
#include "py/runtime0.h"
#include "py/objstr.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py_helper.h"
#include "py_mathhelper.h"

#define SENSOR_WIDTH  (2592)
#define SENSOR_HEIGHT (1944)
#define SENSOR_DIAG   (3240)

typedef struct py_guidestar_obj {
    mp_obj_base_t base;
    py_blob_obj_t* blob;
    //mp_obj_t r;
    mp_obj_t star_rating;
    mp_obj_t clustered;
    //mp_obj_t ishot;
} py_guidestar_obj_t;

static void py_guidestar_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    py_guidestar_obj_t *self = self_in;
    mp_printf(print, "{\"obj_type\":\"CGuideStar\",\"cx\":%0.1f,\"cy\":%0.1f}"
              ,mp_obj_get_int(self->blob->cx)
              ,mp_obj_get_int(self->blob->cy)
              );
}

static mp_obj_t py_guidestar_eval(mp_obj_t self_in)
{
    py_guidestar_obj_t *o = (py_guidestar_obj_t*)self_in;

    /*
    if (mp_obj_get_int(o->ishot) != 0) {
        o->star_rating = mp_obj_new_int(0);
        return o->star_rating;
    }
    //*/

    /*
    int center_x = SENSOR_WIDTH  / 2;
    int center_y = SENSOR_HEIGHT / 2;
    float star_x = mp_obj_get_float(o->blob->cx);
    float star_y = mp_obj_get_float(o->blob->cy);
    int center_dist = fast_roundf(calc_dist_float(center_x, center_y, star_x, star_y));
    bool outofview = (star_x < (float)((SENSOR_WIDTH * 1.0) / 3.0) || star_x > (float)((SENSOR_WIDTH * 2.0) / 3.0)) || (star_y < (float)((SENSOR_HEIGHT * 1.0) / 3.0) || star_y > (float)((SENSOR_HEIGHT * 2.0) / 3.0));
    int falloff = SENSOR_HEIGHT / 4;
    int score_centerdist = 100;
    if (center_dist > falloff) {
        score_centerdist -= map_val_int(center_dist, falloff, SENSOR_DIAG / 2, 0, 100);
    }
    */

    #define BEST_MAXBRIGHT (256 - 64)
    int b = mp_obj_get_int(o->blob->maxbrightness);
    int score_maxbright = 0;
    if (b >= BEST_MAXBRIGHT && b != 254) {
        score_maxbright = 100;
    }
    else if (b < BEST_MAXBRIGHT) {
        score_maxbright = map_val_int(b, 0, BEST_MAXBRIGHT, 0, 100);
    }
    else {
        score_maxbright = 75;
    }

    int satcnt = mp_obj_get_int(o->blob->saturation_cnt);
    int score_saturation = 100;
    if (satcnt > 1) {
        int area = mp_obj_get_int(o->blob->pixels);
        score_saturation -= map_val_int(satcnt, 0, area, 0, 100);
    }

    int score_pointiness = mp_obj_get_int(o->blob->star_pointiness);

    int total =   (score_pointiness * 66)
                + (score_maxbright  * 17)
                + (score_saturation * 17);

    int clustered = mp_obj_get_int(o->clustered);
    if (clustered > 0) {
        total /= 4;
    }

    if (satcnt >= 9) {
        total /= 2;
    }

    total += 50;
    total /= 100;
    o->star_rating = mp_obj_new_int(total);

    return o->star_rating;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_eval_obj, py_guidestar_eval);

mp_obj_t py_guidestar_cxf(mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->cx; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_cxf_obj, py_guidestar_cxf);
mp_obj_t py_guidestar_cyf(mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->cy; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_cyf_obj, py_guidestar_cyf);
mp_obj_t py_guidestar_r(mp_obj_t self_in) {
    //return ((py_guidestar_obj_t *) self_in)->r;
    return mp_obj_new_int((mp_obj_get_int(((py_guidestar_obj_t *) self_in)->blob->w) + mp_obj_get_int(((py_guidestar_obj_t *) self_in)->blob->h)) / 3);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_r_obj, py_guidestar_r);
mp_obj_t py_guidestar_pixels        (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->pixels          ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_pixels_obj         , py_guidestar_pixels         );
mp_obj_t py_guidestar_maxbrightness (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->maxbrightness   ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_maxbrightness_obj  , py_guidestar_maxbrightness  );
mp_obj_t py_guidestar_brightnesssum (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->brightness      ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_brightnesssum_obj  , py_guidestar_brightnesssum  );
mp_obj_t py_guidestar_starpointiness(mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->star_pointiness ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_starpointiness_obj , py_guidestar_starpointiness );
mp_obj_t py_guidestar_starprofile   (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->blob->star_profile    ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_starprofile_obj    , py_guidestar_starprofile    );
mp_obj_t py_guidestar_clustered     (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->clustered             ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_clustered_obj      , py_guidestar_clustered      );
mp_obj_t py_guidestar_starrating    (mp_obj_t self_in) { return ((py_guidestar_obj_t *) self_in)->star_rating           ; } STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_starrating_obj     , py_guidestar_starrating     );

static mp_obj_t py_guidestar_clone(mp_obj_t self_in)
{
    py_guidestar_obj_t* original = (py_guidestar_obj_t*)self_in;
    py_guidestar_obj_t* o = m_new_obj(py_guidestar_obj_t);
    o->base.type = &py_guidestar_type;
    o->blob = (py_blob_obj_t*)m_new_obj(py_blob_obj_t);
    memcpy(o->blob, original->blob, sizeof(py_blob_obj_t));
    o->blob->base.type       = original->blob->base.type;
    o->blob->x               = mp_obj_new_int  (mp_obj_get_int  (original->blob->x));
    o->blob->y               = mp_obj_new_int  (mp_obj_get_int  (original->blob->y));
    o->blob->w               = mp_obj_new_int  (mp_obj_get_int  (original->blob->w));
    o->blob->h               = mp_obj_new_int  (mp_obj_get_int  (original->blob->h));
    o->blob->cx              = mp_obj_new_float(mp_obj_get_float(original->blob->cx));
    o->blob->cy              = mp_obj_new_float(mp_obj_get_float(original->blob->cy));
    o->blob->pixels          = mp_obj_new_int  (mp_obj_get_int  (original->blob->pixels));
    o->blob->maxbrightness   = mp_obj_new_int  (mp_obj_get_int  (original->blob->maxbrightness));
    o->blob->star_pointiness = mp_obj_new_int  (mp_obj_get_int  (original->blob->star_pointiness));
    o->blob->star_profile    = original->blob->star_profile;
    o->star_rating           = mp_obj_new_int(mp_obj_get_int(original->star_rating));
    o->clustered             = mp_obj_new_int(mp_obj_get_int(original->clustered));
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_clone_obj, py_guidestar_clone);

static mp_obj_t py_guidestar_movecoord(mp_obj_t self_in, mp_obj_t x, mp_obj_t y)
{
    py_guidestar_obj_t* original = (py_guidestar_obj_t*)self_in;
    original->blob->cx = mp_obj_new_float(mp_obj_get_float(x));
    original->blob->cy = mp_obj_new_float(mp_obj_get_float(y));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_guidestar_movecoord_obj, py_guidestar_movecoord);

static mp_obj_t py_guidestar_coord(mp_obj_t self_in)
{
    py_guidestar_obj_t* original = (py_guidestar_obj_t*)self_in;
    mp_obj_t result_tuple[2];
    result_tuple[0] = mp_obj_new_float(mp_obj_get_float(original->blob->cx));
    result_tuple[1] = mp_obj_new_float(mp_obj_get_float(original->blob->cy));
    return mp_obj_new_tuple(2, result_tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_coord_obj, py_guidestar_coord);

STATIC const mp_rom_map_elem_t py_guidestar_obj_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_cxf             ), MP_ROM_PTR(&py_guidestar_cxf_obj            ) },
    { MP_ROM_QSTR(MP_QSTR_cyf             ), MP_ROM_PTR(&py_guidestar_cyf_obj            ) },
    { MP_ROM_QSTR(MP_QSTR_r               ), MP_ROM_PTR(&py_guidestar_r_obj              ) },
    { MP_ROM_QSTR(MP_QSTR_pixels          ), MP_ROM_PTR(&py_guidestar_pixels_obj         ) },
    { MP_ROM_QSTR(MP_QSTR_max_brightness  ), MP_ROM_PTR(&py_guidestar_maxbrightness_obj  ) },
    { MP_ROM_QSTR(MP_QSTR_brightness_sum  ), MP_ROM_PTR(&py_guidestar_brightnesssum_obj  ) },
    { MP_ROM_QSTR(MP_QSTR_star_pointiness ), MP_ROM_PTR(&py_guidestar_starpointiness_obj ) },
    { MP_ROM_QSTR(MP_QSTR_star_profile    ), MP_ROM_PTR(&py_guidestar_starprofile_obj    ) },
    { MP_ROM_QSTR(MP_QSTR_clustered       ), MP_ROM_PTR(&py_guidestar_clustered_obj      ) },
    { MP_ROM_QSTR(MP_QSTR_star_rating     ), MP_ROM_PTR(&py_guidestar_starrating_obj     ) },
    { MP_ROM_QSTR(MP_QSTR_eval            ), MP_ROM_PTR(&py_guidestar_eval_obj           ) },
    { MP_ROM_QSTR(MP_QSTR_coord           ), MP_ROM_PTR(&py_guidestar_coord_obj          ) },
    { MP_ROM_QSTR(MP_QSTR_clone           ), MP_ROM_PTR(&py_guidestar_clone_obj          ) },
    { MP_ROM_QSTR(MP_QSTR_move_coord      ), MP_ROM_PTR(&py_guidestar_movecoord_obj      ) },
};
STATIC MP_DEFINE_CONST_DICT(py_guidestar_obj_locals_dict, py_guidestar_obj_locals_dict_table);

const mp_obj_type_t py_guidestar_type = {
    { &mp_type_type },
    .name  = MP_QSTR_CGuideStar,
    .print = py_guidestar_obj_print,
    .locals_dict = (mp_obj_t) &py_guidestar_obj_locals_dict
};

static mp_obj_t py_blob2guidestar(mp_obj_t blob)
{
    py_guidestar_obj_t *o = m_new_obj(py_guidestar_obj_t);
    o->base.type = &py_guidestar_type;
    o->blob = (py_blob_obj_t*)blob;
    //o->r = mp_obj_new_int((mp_obj_get_int(o->blob->w) + mp_obj_get_int(o->blob->h)) / 3);
    o->star_rating = mp_obj_new_int(0);
    o->clustered = mp_obj_new_int(0);
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blob2guidestar_obj, py_blob2guidestar);

static mp_obj_t py_blobs2guidestars(mp_obj_t blob_list_)
{
    mp_obj_list_t* blob_list = (mp_obj_list_t*)blob_list_;
    int len = blob_list->len;
    mp_obj_list_t* new_list = mp_obj_new_list(len, NULL);
    for (int i = 0; i < len; i++)
    {
        py_guidestar_obj_t* o = (py_guidestar_obj_t*)py_blob2guidestar(blob_list->items[i]);
        new_list->items[i] = o;
    }
    return new_list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_blobs2guidestars_obj, py_blobs2guidestars);

static int guidestar_cmpfunc(const void * a, const void * b)
{
    mp_obj_t* aa = (mp_obj_t*)a;
    mp_obj_t* bb = (mp_obj_t*)b;
    py_guidestar_obj_t* aaa = (py_guidestar_obj_t*)(*aa);
    py_guidestar_obj_t* bbb = (py_guidestar_obj_t*)(*bb);
    int aaaa = mp_obj_get_int(aaa->star_rating);
    int bbbb = mp_obj_get_int(bbb->star_rating);
    return ( bbbb - aaaa );
}

static mp_obj_t py_guidestar_sort(mp_obj_t list)
{
    mp_obj_list_t* objects_list = (mp_obj_list_t*)list;
    qsort(objects_list->items, objects_list->len, sizeof(mp_obj_t), guidestar_cmpfunc);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_sort_obj, py_guidestar_sort);

static mp_obj_t py_guidestar_markclusters(mp_obj_t list_, mp_obj_t tol_)
{
    mp_obj_list_t* list = (mp_obj_list_t*)list_;
    int lim = list->len;
    int tol = mp_obj_get_int(tol_);
    for (int i = 0; i < lim; i++)
    {
        for (int j = 0; j < lim; j++)
        {
            if (i != j)
            {
                py_guidestar_obj_t* star_a = (py_guidestar_obj_t*)list->items[i];
                py_guidestar_obj_t* star_b = (py_guidestar_obj_t*)list->items[j];
                int mag = fast_roundf(calc_dist_float(mp_obj_get_float(star_a->blob->cx), mp_obj_get_float(star_a->blob->cy), mp_obj_get_float(star_b->blob->cx), mp_obj_get_float(star_b->blob->cy)));
                if (mag < tol)
                {
                    star_a->clustered = mp_obj_new_int(tol);
                    star_b->clustered = mp_obj_new_int(tol);
                }
            }
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_guidestar_markclusters_obj, py_guidestar_markclusters);

static mp_obj_t py_guidestar_filterhotpixels(mp_obj_t list_, mp_obj_t hotpixs_, mp_obj_t tol_)
{
    if (hotpixs_ == NULL || hotpixs_ == mp_const_none) {
        return mp_const_none;
    }
    mp_obj_list_t* list = (mp_obj_list_t*)list_;
    mp_obj_list_t* hotpixs = (mp_obj_list_t*)hotpixs_;
    int star_lim = list->len;
    int hotpixs_lim = hotpixs->len;
    int tol = 2;
    if (tol_ == NULL || tol_ == mp_const_none) {
        tol = mp_obj_get_int(tol_);
    }
    for (int i = 0; i < star_lim; i++)
    {
        for (int j = 0; j < hotpixs_lim; j++)
        {
            py_guidestar_obj_t* star_a = (py_guidestar_obj_t*)list->items[i];
            mp_obj_tuple_t*     star_b = (mp_obj_tuple_t*) hotpixs->items[j];
            int mag = fast_roundf(calc_dist_float(mp_obj_get_float(star_a->blob->cx), mp_obj_get_float(star_a->blob->cy), mp_obj_get_int(star_b->items[0]), mp_obj_get_int(star_b->items[1])));
            if (mag <= tol)
            {
                star_lim -= 1;
                int k;
                for (k = i; k < star_lim; k++)
                {
                    list->items[k] = list->items[k + 1];
                }
                list->items[k] = mp_const_none;
                list->len = (size_t)star_lim;
                i -= 1;
            }
        }
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(py_guidestar_filterhotpixels_obj, py_guidestar_filterhotpixels);

static mp_obj_t py_guidestar_stars2hotpixels(mp_obj_t list_)
{
    mp_obj_list_t* list = (mp_obj_list_t*)list_;
    int lim = list->len;
    mp_obj_list_t* hotpixs = mp_obj_new_list(lim, NULL);
    for (int i = 0; i < lim; i++)
    {
        py_guidestar_obj_t* star = (py_guidestar_obj_t*)list->items[i];
        mp_obj_t coord_tuple[2];
        coord_tuple[0] = mp_obj_new_int(fast_roundf(mp_obj_get_float(star->blob->cx)));
        coord_tuple[1] = mp_obj_new_int(fast_roundf(mp_obj_get_float(star->blob->cy)));
        hotpixs->items[i] = mp_obj_new_tuple(2, coord_tuple);
    }
    return hotpixs;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_guidestar_stars2hotpixels_obj, py_guidestar_stars2hotpixels);

static mp_obj_t py_guidestar_processlist_(mp_obj_t list_, mp_obj_t cluster_tol_, mp_obj_t hotpixels_, mp_obj_t hotpixel_tol_, mp_obj_t min_rating_)
{
    mp_obj_list_t* list = (mp_obj_list_t*)list_;
    int b4len = list->len;
    if (hotpixels_ != NULL && hotpixels_ != mp_const_none) {
        py_guidestar_filterhotpixels(list_, hotpixels_, hotpixel_tol_);
    }
    if (cluster_tol_ != NULL && cluster_tol_ != mp_const_none) {
        py_guidestar_markclusters(list_, cluster_tol_);
    }
    int len = list->len;
    //int min_score = 9999;
    int max_score = 0;
    for (int i = 0; i < len; i++)
    {
        mp_obj_t scoreobj = py_guidestar_eval(list->items[i]);
        int score = mp_obj_get_int(scoreobj);
        if (score > max_score) { max_score = score; }
        //if (score < min_score) { min_score = score; }
    }
    //max_score -= min_score;
    int min_rating = 0;
    int good_enough = 0;
    if (min_rating_ != NULL && min_rating_ != mp_const_none) {
        min_rating = mp_obj_get_int(min_rating_);
    }
    for (int i = 0; i < len; i++)
    {
        py_guidestar_obj_t* star = (py_guidestar_obj_t*)list->items[i];
        mp_obj_t scoreobj = star->star_rating;
        int score = mp_obj_get_int(scoreobj);
        //score -= min_score;
        score *= 100;
        score += max_score / 2;
        score /= max_score;
        star->star_rating = mp_obj_new_int(score);
        if (score >= min_rating) {
            good_enough += 1;
        }
    }
    py_guidestar_sort(list_);

    mp_obj_t ret_tuple[4];
    ret_tuple[0] = list_;
    ret_tuple[1] = mp_obj_new_int(b4len);
    ret_tuple[2] = mp_obj_new_int(len);
    ret_tuple[3] = mp_obj_new_int(good_enough);
    return mp_obj_new_tuple(4, ret_tuple);
}
static mp_obj_t py_guidestar_processlist(uint n_args, const mp_obj_t *args) {
    return py_guidestar_processlist_(args[0]
        , n_args >= 2 ? args[1] : NULL
        , n_args >= 3 ? args[2] : NULL
        , n_args >= 4 ? args[3] : NULL
        , n_args >= 5 ? args[4] : NULL
        );
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(py_guidestar_processlist_obj, 1, py_guidestar_processlist);

static mp_obj_t py_guidestar_selectfirst_(mp_obj_t list_, mp_obj_t boundary_)
{
    int boundary = 100 - mp_obj_get_int(boundary_);
    int region = SENSOR_HEIGHT * boundary;
    region /= 200;
    int left   = region;
    int right  = SENSOR_WIDTH - region;
    int top    = region;
    int bottom = SENSOR_HEIGHT - region;

    mp_obj_list_t* list = (mp_obj_list_t*)list_;
    int lim = list->len;

    for (int i = 0; i < lim; i++)
    {
        py_guidestar_obj_t* star = (py_guidestar_obj_t*)list->items[i];
        int x = fast_roundf(mp_obj_get_float(star->blob->cx));
        int y = fast_roundf(mp_obj_get_float(star->blob->cy));
        if (x >= left && x <= right && y >= top && y <= bottom)
        {
            return star;
        }
    }
    return py_guidestar_selectfirst_(list_, mp_obj_new_int(boundary + 10));
}
static mp_obj_t py_guidestar_selectfirst(uint n_args, const mp_obj_t *args) {
    if (n_args >= 2) {
        return py_guidestar_selectfirst_(args[0], args[1]);
    }
    else {
        return py_guidestar_selectfirst_(args[0], mp_obj_new_int(50));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(py_guidestar_selectfirst_obj, 1, py_guidestar_selectfirst);

typedef struct
{
    float dx, dy;
    float mag;
    int star_idx;
    py_guidestar_obj_t* star;
    int   nearby;
    int   err_sum;
    float err_avg;
    int   score;
}
possiblemove_t;

static void eval_move(possiblemove_t* move, int tolerance, mp_obj_list_t* old_list, mp_obj_list_t* new_list)
{
    move->nearby  = 0;
    move->err_sum = 0;
    move->err_avg = 0;
    move->score   = 0;

    int old_list_len = old_list->len;
    int new_list_len = new_list->len;

    for (int i = 0; i < old_list_len; i++)
    {
        // if this is the movement, then where should the new star be, referencing the old star
        py_guidestar_obj_t* star = (py_guidestar_obj_t*)old_list->items[i];
        float nx = mp_obj_get_float(star->blob->cx) + move->dx;
        float ny = mp_obj_get_float(star->blob->cy) + move->dy;
        float min_dist = SENSOR_DIAG;
        // find the closest match in the new list to the new star
        for (int j = 0; j < new_list_len; j++)
        {
            py_guidestar_obj_t* new_star = (py_guidestar_obj_t*)new_list->items[j];
            float nsx = mp_obj_get_float(new_star->blob->cx);
            float nsy = mp_obj_get_float(new_star->blob->cy);
            float mag = calc_dist_float(nx, ny, nsx, nsy);
            if (mag < min_dist)
            {
                min_dist = mag;
            }
        }
        if (fast_roundf(min_dist) < tolerance)
        {
            move->nearby += 1;
            move->err_sum += min_dist;
        }
    }
    if (move->nearby > 0)
    {
        move->err_avg = move->err_sum;
        move->err_avg /= (float)move->nearby;
    }
}

#define RETURN_TUPLE_STAR_MOVEMENT(star_obj, err_avg, counter) do { mp_obj_t result_tuple[3]; result_tuple[0] = (mp_obj_t)(star_obj); result_tuple[1] = (mp_obj_t)(err_avg); result_tuple[2] = (mp_obj_t)(counter); return mp_obj_new_tuple(3, result_tuple); } while (0)
#define RETURN_TUPLE_STAR_MOVEMENT_II(star_obj, err_avg, counter) RETURN_TUPLE_STAR_MOVEMENT((star_obj), mp_obj_new_int(err_avg), mp_obj_new_int(counter))
#define RETURN_TUPLE_STAR_MOVEMENT_FI(star_obj, err_avg, counter) RETURN_TUPLE_STAR_MOVEMENT((star_obj), mp_obj_new_float(err_avg), mp_obj_new_int(counter))

static mp_obj_t py_get_single_star_motion_(mp_obj_t old_list_, mp_obj_t new_list_, mp_obj_t selected_star_, mp_obj_t tolerance_, mp_obj_t fast_mode_)
{
    if (old_list_ == NULL || old_list_ == mp_const_none || new_list_ == NULL || new_list_ == mp_const_none || selected_star_ == NULL || selected_star_ == mp_const_none) {
        RETURN_TUPLE_STAR_MOVEMENT_II(mp_const_none, SENSOR_DIAG, 0);
    }
    mp_obj_list_t* old_list = (mp_obj_list_t*)old_list_;
    mp_obj_list_t* new_list = (mp_obj_list_t*)new_list_;
    int old_list_len = old_list->len;
    int new_list_len = new_list->len;
    if (new_list_len < 1) {
        RETURN_TUPLE_STAR_MOVEMENT_II(mp_const_none, SENSOR_DIAG - 1, 0);
    }
    else if (new_list_len == 1) {
        RETURN_TUPLE_STAR_MOVEMENT_II(new_list->items[0], 0, 1);
    }
    int fast_mode = mp_obj_get_int(fast_mode_);
    int quick_match_required = new_list_len * fast_mode / 100;
    int tolerance = mp_obj_get_int(tolerance_);
    py_guidestar_obj_t* selected_star = (py_guidestar_obj_t*)selected_star_;

    // each star in the new list is a possible movement vector
    possiblemove_t* move_list = xalloc0(new_list_len * sizeof(possiblemove_t));
    float closest_mag = SENSOR_DIAG;
    py_guidestar_obj_t* closest_star = NULL;
    for (int i = 0; i < new_list_len; i++)
    {
        py_guidestar_obj_t* new_star = (py_guidestar_obj_t*)new_list->items[i];
        possiblemove_t* move = (possiblemove_t*)&(move_list[i]);
        float dx, dy, mag;
        move->star_idx = i;
        move->star = new_star;
        move->dx  = dx  = mp_obj_get_float(new_star->blob->cx) - mp_obj_get_float(selected_star->blob->cx);
        move->dy  = dy  = mp_obj_get_float(new_star->blob->cy) - mp_obj_get_float(selected_star->blob->cy);
        move->mag = mag = fast_sqrtf((dx * dx) + (dy * dy));
        if (mag < closest_mag) {
            closest_mag = mag;
            closest_star = new_star;
        }
        if (fast_mode > 0) {
            // fast mode means do not care about all other results if one looks great
            eval_move(move, tolerance, old_list, new_list);
            int err_avg = fast_roundf(move->err_avg);
            if (err_avg < tolerance && move->nearby >= quick_match_required) {
                xfree(move_list);
                RETURN_TUPLE_STAR_MOVEMENT_II(new_star, err_avg, move->nearby);
            }
        }
    }

    if (old_list_len <= 1) {
        if (closest_star != NULL) {
            xfree(move_list);
            RETURN_TUPLE_STAR_MOVEMENT_II(closest_star, 0, 1);
        }
    }

    if (fast_mode <= 0) {
        for (int i = 0; i < new_list_len; i++) {
            possiblemove_t* move = (possiblemove_t*)&(move_list[i]);
            eval_move(move, tolerance, old_list, new_list);
        }
    }

    // find the criteria for calculating score
    int best_nearby = quick_match_required;
    for (int i = 0; i < new_list_len; i++)
    {
        possiblemove_t* move = (possiblemove_t*)&(move_list[i]);
        int nb = move->nearby;
        if (nb > best_nearby) {
            best_nearby = nb;
        }
    }

    int best_score = 0;
    int best_err = SENSOR_DIAG;
    int best_move_nearby = 0;
    py_guidestar_obj_t* best_star = (py_guidestar_obj_t*)mp_const_none;
    possiblemove_t* best_move = NULL;
    // score all possible moves and find the best one
    for (int i = 0; i < new_list_len; i++)
    {
        possiblemove_t* move = (possiblemove_t*)&(move_list[i]);
        int nb = move->nearby;
        int err_avg = fast_roundf(move->err_avg);
        if (err_avg >= tolerance) {
            continue;
        }
        int score_nearby = map_val_int(nb, 0, best_nearby, 0, 100) * 50;
        int score_erravg = (100 - map_val_int(err_avg, 0, tolerance, 0, 100)) * 50;
        int score_total = (score_nearby + score_erravg) / 100;
        move->score = score_total;
        if (score_total > best_score) {
            best_score = score_total;
            best_err = err_avg;
            best_move_nearby = nb;
            best_star = move->star;
            best_move = move;
        }
    }
    xfree(move_list);
    if (best_move != NULL && best_move_nearby > 0) {
        RETURN_TUPLE_STAR_MOVEMENT_II(best_star, best_err, best_move_nearby);
    }
    RETURN_TUPLE_STAR_MOVEMENT_II(mp_const_none, SENSOR_DIAG - 2, 0);
}
static mp_obj_t py_get_single_star_motion(uint n_args, const mp_obj_t *args) {
    return py_get_single_star_motion_(args[0], args[1], args[2], args[3], args[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(py_get_single_star_motion_obj, 5, py_get_single_star_motion);

#define RETURN_TUPLE_MULTISTAR_MOVEMENT(star_obj, nx, ny, err_avg, nearby, mstar_cnt) do { mp_obj_t result_tuple[6]; result_tuple[0] = (mp_obj_t)(star_obj); result_tuple[1] = (mp_obj_t)(nx); result_tuple[2] = (mp_obj_t)(ny); result_tuple[3] = (mp_obj_t)(err_avg); result_tuple[4] = (mp_obj_t)(nearby); result_tuple[5] = (mp_obj_t)(mstar_cnt); return mp_obj_new_tuple(6, result_tuple); } while (0)
#define RETURN_TUPLE_MULTISTAR_MOVEMENT_FFIII(star_obj, nx, ny, err_avg, nearby, mstar_cnt) RETURN_TUPLE_MULTISTAR_MOVEMENT(star_obj, mp_obj_new_float(nx), mp_obj_new_float(ny), mp_obj_new_int(err_avg), mp_obj_new_int(nearby), mp_obj_new_int(mstar_cnt))

static mp_obj_t py_get_multi_star_motion_(mp_obj_t old_list_, mp_obj_t new_list_, mp_obj_t selected_star_, mp_obj_t tolerance_, mp_obj_t fast_mode_, mp_obj_t rating_thresh_, mp_obj_t mstarcnt_min_, mp_obj_t mstarcnt_max_)
{
    mp_obj_tuple_t* single_res = (mp_obj_tuple_t*)py_get_single_star_motion_(old_list_, new_list_, selected_star_, tolerance_, fast_mode_);
    py_guidestar_obj_t* star = single_res->items[0];
    if ((mp_obj_t)star == mp_const_none || star == NULL) {
        RETURN_TUPLE_MULTISTAR_MOVEMENT_FFIII(mp_const_none, -1, -1, SENSOR_DIAG, 0, 0);
    }
    int mstarcnt_max = mp_obj_get_int(mstarcnt_max_);
    mp_obj_list_t* old_list = (mp_obj_list_t*)old_list_;
    mp_obj_list_t* new_list = (mp_obj_list_t*)new_list_;
    int old_list_len = old_list->len;
    int new_list_len = new_list->len;
    if (mstarcnt_max <= 1 || new_list_len <= 1) {
        RETURN_TUPLE_MULTISTAR_MOVEMENT_FFIII(star, mp_obj_get_float(star->blob->cx), mp_obj_get_float(star->blob->cy), mp_obj_get_int(single_res->items[1]), mp_obj_get_int(single_res->items[2]), 1);
    }
    py_guidestar_obj_t* selected_star = (py_guidestar_obj_t*)selected_star_;
    int tolerance = mp_obj_get_int(tolerance_);
    int rating_thresh = mp_obj_get_int(rating_thresh_);
    int mstarcnt_min = mp_obj_get_int(mstarcnt_min_);
    float nsx, nsy, ssx, ssy;
    float dx = (nsx = mp_obj_get_float(star->blob->cx)) - (ssx = mp_obj_get_float(selected_star->blob->cx));
    float dy = (nsy = mp_obj_get_float(star->blob->cy)) - (ssy = mp_obj_get_float(selected_star->blob->cy));
    float dx_sum = 0, dy_sum = 0, dx_avg, dy_avg;
    int avg_cnt = 0;
    float avg_weight = 0;
    for (int i = 0; i < old_list_len; i++)
    {
        py_guidestar_obj_t* old_star = (py_guidestar_obj_t*)old_list->items[i];
        float ox = mp_obj_get_float(old_star->blob->cx);
        float oy = mp_obj_get_float(old_star->blob->cy);
        float onx = ox + dx;
        float ony = oy + dy;
        int nearest_rating = -1;
        float nearest_mag = SENSOR_DIAG;
        float best_dx, best_dy;
        for (int j = 0; j < new_list_len; j++)
        {
            py_guidestar_obj_t* new_star = (py_guidestar_obj_t*)new_list->items[j];
            float nx = mp_obj_get_float(new_star->blob->cx);
            float ny = mp_obj_get_float(new_star->blob->cy);
            float ndx = nx - onx;
            float ndy = ny - ony;
            float mag = fast_sqrtf((ndx * ndx) + (ndy * ndy));
            if (mag < nearest_mag)
            {
                // if we find the one closest to the new predicted coordinate, remember the actual movement between the old and new coordinate
                nearest_mag = mag;
                nearest_rating = mp_obj_get_int(new_star->star_rating);
                best_dx = ndx;
                best_dy = ndy;
            }
        }
        // if confidently found one closest to the new predicted coordinate
        // use it in the average movement if it meets the criteria
        if (nearest_rating >= 0 && fast_roundf(nearest_mag) < tolerance &&
            (avg_cnt < mstarcnt_min || (mp_obj_get_int(old_star->star_rating) >= rating_thresh && nearest_rating >= rating_thresh))
        )
        {
            float ssdistx = ssx - ox;
            float ssdisty = ssy - oy;
            float dist_ori = fast_sqrtf((ssdistx * ssdistx) + (ssdisty * ssdisty));
            float dist_weight = SENSOR_DIAG - dist_ori;
            // the average movement is computed with a weight
            // the closer it is to selected_star, the more weight it has
            dx_sum += best_dx * dist_weight;
            dy_sum += best_dy * dist_weight;
            avg_cnt += 1;
            avg_weight += dist_weight;
            if (avg_cnt >= mstarcnt_max) {
                break;
            }
        }
    }

    if (avg_cnt <= 0) {
        // hmm... all stars have bad rating?
        // fall back on using the original calculated move
        dx_avg = dx;
        dy_avg = dy;
    } else {
        // average all of the movements
        dx_avg = dx_sum / avg_weight;
        dy_avg = dy_sum / avg_weight;
    }
    RETURN_TUPLE_MULTISTAR_MOVEMENT_FFIII(star, mp_obj_get_float(star->blob->cx) + dx_avg, mp_obj_get_float(star->blob->cy) + dy_avg, mp_obj_get_int(single_res->items[1]), mp_obj_get_int(single_res->items[2]), avg_cnt);
}
static mp_obj_t py_get_multi_star_motion(uint n_args, const mp_obj_t *args) {
    return py_get_multi_star_motion_(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(py_get_multi_star_motion_obj, 8, py_get_multi_star_motion);

STATIC const mp_rom_map_elem_t pyb_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),                MP_ROM_QSTR(MP_QSTR_guidestar) },
    { MP_ROM_QSTR(MP_QSTR_CGuideStar),              MP_ROM_PTR(&py_guidestar_type) },
    { MP_ROM_QSTR(MP_QSTR_blob2guidestar),          MP_ROM_PTR(&py_blob2guidestar_obj) },
    { MP_ROM_QSTR(MP_QSTR_blobs2guidestars),        MP_ROM_PTR(&py_blobs2guidestars_obj) },
    { MP_ROM_QSTR(MP_QSTR_guidestar_sort),          MP_ROM_PTR(&py_guidestar_sort_obj) },
    { MP_ROM_QSTR(MP_QSTR_mark_clusters),           MP_ROM_PTR(&py_guidestar_markclusters_obj) },
    { MP_ROM_QSTR(MP_QSTR_filter_hotpixels),        MP_ROM_PTR(&py_guidestar_filterhotpixels_obj) },
    { MP_ROM_QSTR(MP_QSTR_stars2hotpixels),         MP_ROM_PTR(&py_guidestar_stars2hotpixels_obj) },
    { MP_ROM_QSTR(MP_QSTR_process_list),            MP_ROM_PTR(&py_guidestar_processlist_obj) },
    { MP_ROM_QSTR(MP_QSTR_select_first),            MP_ROM_PTR(&py_guidestar_selectfirst_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_single_star_motion),  MP_ROM_PTR(&py_get_single_star_motion_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_multi_star_motion),   MP_ROM_PTR(&py_get_multi_star_motion_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_module_globals, pyb_module_globals_table);

const mp_obj_module_t guidestar_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pyb_module_globals,
};
