#include "py_guidepulser.h"

#include <stdint.h>
#include <stdlib.h>

//#include "i2c.h"
#include "systick.h"
#include "stm32h7xx_hal_dma.h"
#include "stm32h7xx_hal_i2c.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mperrno.h"
#include "py/mphal.h"

extern I2C_HandleTypeDef I2CHandle2;
extern void pyb_i2c_init(I2C_HandleTypeDef *i2c);
//extern int i2c_init(i2c_t *i2c, mp_hal_pin_obj_t scl, mp_hal_pin_obj_t sda, uint32_t freq, uint16_t timeout_ms);

#define DEV_I2C_ADDR (0x38)

#define BM_SHUTTER   (0x10)
#define BM_RA_EAST   (0x08)
#define BM_RA_WEST   (0x01)
#define BM_DEC_NORTH (0x04)
#define BM_DEC_SOUTH (0x02)
#define BM_LED_RED   (0x80)
#define BM_LED_GREEN (0x20)
#define BM_LED_BLUE  (0x40)

static uint8_t    cur_data;
static bool       shutter_flag;
static uint32_t   grace_time;
static uint8_t    panic_state;
static uint32_t   panic_time;
static uint8_t    hw_err;

static uint32_t   pulse_time;
static uint32_t   ra_end, dec_end;
static uint32_t   move_end;
static uint32_t   move_end_cache;
static uint32_t   move_start_cache;
static uint32_t   shutter_time;

static bool       led_enable;
static bool       long_mode;
static uint32_t   shutter_led_flasher;
static int32_t    flip_ra;
static int32_t    flip_dec;

void guidepulser_init(void)
{
    #if 1
    I2CHandle2.Init.AddressingMode    = I2C_ADDRESSINGMODE_7BIT;
    I2CHandle2.Init.DualAddressMode   = I2C_DUALADDRESS_DISABLED;
    I2CHandle2.Init.GeneralCallMode   = I2C_GENERALCALL_DISABLED;
    I2CHandle2.Init.NoStretchMode     = I2C_NOSTRETCH_DISABLE;
    I2CHandle2.Init.Timing            = 0x70330309;
    //I2CHandle2.Init.OwnAddress1       = PYB_I2C_MASTER_ADDRESS;
    //I2CHandle2.Init.OwnAddress2       = 0; // unused
    //I2CHandle2.Init.ClockSpeed        = 400000;
    //I2CHandle2.Init.DutyCycle         = I2C_DUTYCYCLE_16_9;
    pyb_i2c_init(&I2CHandle2);
    #endif

    cur_data = 0;
    shutter_flag = false;
    grace_time = 0;
    panic_state = 0;
    panic_time = 0;
    pulse_time = 0;
    ra_end = 0; dec_end = 0;
    flip_ra  = 1;
    flip_dec = 1;
    move_end = 0;
    move_end_cache = 0;
    move_start_cache = 0;
    shutter_time = 0;
    hw_err = 0;
    led_enable = true;
    long_mode = false;
    shutter_led_flasher = 0;

    guidepulser_write(0x03, ~cur_data);
    guidepulser_write(0x01, 0x00);
}

STATIC mp_obj_t py_guidepulser_init(void)
{
    guidepulser_init();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_init_obj, py_guidepulser_init);

void guidepulser_write(uint8_t reg, uint8_t data)
{
    uint8_t buff[] = {reg, data};
    HAL_StatusTypeDef sts = HAL_I2C_Master_Transmit(&I2CHandle2, DEV_I2C_ADDR << 1, buff, 2, 200);
    if (sts == HAL_OK) {
        hw_err = 0;
    }
    else {
        hw_err = (hw_err < 64) ? (hw_err + 1) : (hw_err);
    }
}

STATIC mp_obj_t py_guidepulser_write(mp_obj_t reg, mp_obj_t data)
{
    guidepulser_write(mp_obj_get_int(reg), mp_obj_get_int(data));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(py_guidepulser_write_obj, py_guidepulser_write);

void guidepulser_writeIfDiff(uint8_t data)
{
    if (cur_data != data) {
        guidepulser_write(0x03, ~data);
    }
    cur_data = data;
}

uint32_t guidepulser_move(int32_t ra, int32_t dec, uint32_t grace)
{
    char add_grace = 0;
    uint8_t data = cur_data;

    grace_time = grace;

    ra  *= flip_ra;
    dec *= flip_dec;

    pulse_time = HAL_GetTick();
    move_start_cache = pulse_time;
    ra_end = pulse_time + abs(ra);
    dec_end = pulse_time + abs(dec);

    if ((data & (BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH)) != 0x00) {
        add_grace = 1;
    }

    data &= 0xFF ^ (BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH);
    if (ra > 0) {
        data |= BM_RA_EAST;
    }
    else if (ra < 0) {
        data |= BM_RA_WEST;
    }
    if (dec > 0) {
        data |= BM_DEC_NORTH;
    }
    else if (dec < 0) {
        data |= BM_DEC_SOUTH;
    }

    if ((data & (BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH)) != 0x00) {
        add_grace = 1;
    }

    move_end = ((ra_end > dec_end) ? ra_end : dec_end) + grace_time;
    move_end_cache = move_end;

    if (add_grace != 0) {
        if (led_enable) {
            data |= BM_LED_GREEN;
        }
        move_end = ((ra_end > dec_end) ? ra_end : dec_end) + grace_time;
        move_end_cache = move_end;
    }
    else {
        data &= ~BM_LED_GREEN;
    }

    guidepulser_writeIfDiff(data);

    return move_end_cache;
}

STATIC mp_obj_t py_guidepulser_move(mp_obj_t ra, mp_obj_t dec, mp_obj_t grace)
{
    uint32_t x = guidepulser_move(mp_obj_get_int(ra), mp_obj_get_int(dec), mp_obj_get_int(grace));
    return mp_obj_new_int(x);
}
MP_DEFINE_CONST_FUN_OBJ_3(py_guidepulser_move_obj, py_guidepulser_move);

void guidepulser_stop(void)
{
    uint8_t data = cur_data;
    data &= ~(BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH | BM_LED_RED);
    guidepulser_writeIfDiff(data);
    move_end = 0;
    ra_end = 0; dec_end = 0;
}

STATIC mp_obj_t py_guidepulser_stop(void)
{
    guidepulser_stop();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_stop_obj, py_guidepulser_stop);

void guidepulser_task(void)
{
    uint8_t data = cur_data;
    uint32_t t = HAL_GetTick();

    if (shutter_flag)
    {
        if (t >= shutter_time)
        {
            shutter_flag = false;
        }
        #if 0
        // this chunk of code is disabled, the flashing does not work well with a heavy workload
        else if (led_enable && long_mode)
        {
            uint32_t tremain = shutter_time - t;
            uint32_t add_me = 0;
            if (tremain > 33000)
            {
                // do nothing
            }
            else if (tremain > 10000)
            {
                if (shutter_led_flasher == 0) { shutter_led_flasher = t & (~(1)); }
                add_me = ((shutter_led_flasher % 2) == 0) ? 3000-101 : 101;
            }
            else if (tremain > 8500) add_me = ((shutter_led_flasher % 2) == 0) ? 1000 - 121 : 121;
            else if (tremain > 6000) add_me = ((shutter_led_flasher % 2) == 0) ? 1000 - 141 : 141;
            else if (tremain > 5000) add_me = ((shutter_led_flasher % 2) == 0) ? 499        : 151;
            else if (tremain > 4000) add_me = ((shutter_led_flasher % 2) == 0) ? 419        : 141;
            else if (tremain > 3000) add_me = ((shutter_led_flasher % 2) == 0) ? 329        : 131;
            else                     add_me = ((shutter_led_flasher % 2) == 0) ? 239        : 121;
            //else                     add_me = ((shutter_led_flasher % 2) == 0) ? 139        : 99;
            //else                     add_me = ((shutter_led_flasher % 2) == 0) ? 79         : 79;

            if (t >= shutter_led_flasher)
            {
                if ((shutter_led_flasher % 2) == 0) {
                    data |= (BM_LED_BLUE);
                }
                else
                {
                    data &= ~(BM_LED_BLUE);
                }
                shutter_led_flasher += add_me;
            }
        }
        #endif
        else if (led_enable == false) { data &= ~(BM_LED_BLUE); } else { data |= (BM_LED_BLUE); }
    }

    if (shutter_flag == false)
    {
        data &= ~(BM_SHUTTER | BM_LED_BLUE);
    }

    if (panic_state != 0)
    {
        data &= ~(BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH | BM_LED_GREEN);
        if (t > panic_time)
        {
            if ((panic_state % 2) == 1)
            {
                panic_state = 2;
                data |= BM_LED_RED;
            }
            else// if ((panic_state % 2) == 0)
            {
                panic_state = 1;
                data &= ~(BM_LED_RED | BM_LED_GREEN);
            }
            panic_time += 150; // setup next blink
        }
    }
    else
    {
        if (t >= ra_end) {
            data &= ~(BM_RA_EAST | BM_RA_WEST);
        }
        if (t >= dec_end) {
            data &= ~(BM_DEC_NORTH | BM_DEC_SOUTH);
        }
        if ((data & (BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH)) == 0x00) {
            data &= ~(BM_LED_RED | BM_LED_GREEN);
        }
    }

    guidepulser_writeIfDiff(data);
}

STATIC mp_obj_t py_guidepulser_task(void)
{
    guidepulser_task();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_task_obj, py_guidepulser_task);

void guidepulser_panic(int state)
{
    panic_state = state;
    guidepulser_task();
}

STATIC mp_obj_t py_guidepulser_panic(mp_obj_t sts)
{
    guidepulser_panic(mp_obj_get_int(sts));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(py_guidepulser_panic_obj, py_guidepulser_panic);

void guidepulser_shutter(uint32_t tspan)
{
    uint32_t t = HAL_GetTick();
    uint8_t data = cur_data;
    shutter_flag = true;
    data |= (BM_SHUTTER | (led_enable ? BM_LED_BLUE : 0));
    shutter_time = t + (tspan * 1000);
    long_mode = tspan >= 30;
    shutter_led_flasher = 0;
    guidepulser_writeIfDiff(data);
}

STATIC mp_obj_t py_guidepulser_shutter(mp_obj_t tspan)
{
    guidepulser_shutter(mp_obj_get_int(tspan));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(py_guidepulser_shutter_obj, py_guidepulser_shutter);

void guidepulser_haltShutter(void)
{
    uint8_t data = cur_data;
    shutter_flag = false;
    data &= ~(BM_SHUTTER | BM_LED_BLUE);
    guidepulser_writeIfDiff(data);
}

STATIC mp_obj_t py_guidepulser_haltShutter(void)
{
    guidepulser_haltShutter();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_haltShutter_obj, py_guidepulser_haltShutter);

bool guidepulser_isMoving(void)
{
    return HAL_GetTick() < move_end && (cur_data & (BM_RA_EAST | BM_RA_WEST | BM_DEC_NORTH | BM_DEC_SOUTH)) != 0x00;
}

STATIC mp_obj_t py_guidepulser_isMoving(void)
{
    return guidepulser_isMoving() ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_isMoving_obj, py_guidepulser_isMoving);

uint32_t guidepulser_getStartTime(void)
{
    return move_start_cache;
}

uint32_t guidepulser_getStopTime(void)
{
    return move_end_cache;
}

STATIC mp_obj_t py_guidepulser_getStartTime(void)
{
    return mp_obj_new_int(move_start_cache);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getStartTime_obj, py_guidepulser_getStartTime);

STATIC mp_obj_t py_guidepulser_getStopTime(void)
{
    return mp_obj_new_int(move_end_cache);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getStopTime_obj, py_guidepulser_getStopTime);

uint32_t guidepulser_getHwErr(void)
{
    return hw_err;
}

STATIC mp_obj_t py_guidepulser_getHwErr(void)
{
    return mp_obj_new_int(hw_err);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getHwErr_obj, py_guidepulser_getHwErr);

bool guidepulser_isShutterOpen(void)
{
    return shutter_flag || (cur_data & BM_SHUTTER) != 0x00;
}

STATIC mp_obj_t py_guidepulser_isShutterOpen(void)
{
    return guidepulser_isShutterOpen() ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_isShutterOpen_obj, py_guidepulser_isShutterOpen);

bool guidepulser_isPanicking(void)
{
    return panic_state != 0;
}

STATIC mp_obj_t py_guidepulser_isPanicking(void)
{
    return guidepulser_isPanicking() ? mp_const_true : mp_const_false;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_isPanicking_obj, py_guidepulser_isPanicking);

void guidepulser_setFlipRa(int32_t x)
{
    flip_ra = (x <= 0) ? -1 : 1;
}

int guidepulser_getFlipRa(void)
{
    return (int)flip_ra;
}

void guidepulser_setFlipDec(int32_t x)
{
    flip_dec = (x <= 0) ? -1 : 1;
}

int guidepulser_getFlipDec(void)
{
    return (int)flip_dec;
}

STATIC mp_obj_t py_guidepulser_setFlipRa(mp_obj_t x)
{
    guidepulser_setFlipRa(mp_obj_get_int(x));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(py_guidepulser_setFlipRa_obj, py_guidepulser_setFlipRa);

STATIC mp_obj_t py_guidepulser_setFlipDec(mp_obj_t x)
{
    guidepulser_setFlipDec(mp_obj_get_int(x));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(py_guidepulser_setFlipDec_obj, py_guidepulser_setFlipDec);

STATIC mp_obj_t py_guidepulser_getFlipRa(void)
{
    return mp_obj_new_int(flip_ra);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getFlipRa_obj, py_guidepulser_getFlipRa);

STATIC mp_obj_t py_guidepulser_getFlipDec(void)
{
    return mp_obj_new_int(flip_dec);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getFlipDec_obj, py_guidepulser_getFlipDec);

void guidepulser_enableLed(void)
{
    led_enable = true;
}

void guidepulser_disableLed(void)
{
    led_enable = false;
    guidepulser_task();
}

STATIC mp_obj_t py_guidepulser_enableLed(void)
{
    guidepulser_enableLed();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_enableLed_obj, py_guidepulser_enableLed);

STATIC mp_obj_t py_guidepulser_disableLed(void)
{
    guidepulser_disableLed();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_disableLed_obj, py_guidepulser_disableLed);

int guidepulser_getRemainingShutter(void)
{
    if (guidepulser_isShutterOpen() == false) {
        return 0;
    }
    return shutter_time - HAL_GetTick();
}

STATIC mp_obj_t py_guidepulser_getRemainingShutter(void)
{
    int x = guidepulser_getRemainingShutter();
    return mp_obj_new_int(x);
}
MP_DEFINE_CONST_FUN_OBJ_0(py_guidepulser_getRemainingShutter_obj, py_guidepulser_getRemainingShutter);

STATIC const mp_rom_map_elem_t pyb_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_guidepulser) },
    { MP_ROM_QSTR(MP_QSTR_init),                MP_ROM_PTR(&py_guidepulser_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&py_guidepulser_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_move),                MP_ROM_PTR(&py_guidepulser_move_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&py_guidepulser_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_task),                MP_ROM_PTR(&py_guidepulser_task_obj) },
    { MP_ROM_QSTR(MP_QSTR_panic),               MP_ROM_PTR(&py_guidepulser_panic_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutter),             MP_ROM_PTR(&py_guidepulser_shutter_obj) },
    { MP_ROM_QSTR(MP_QSTR_halt_shutter),        MP_ROM_PTR(&py_guidepulser_haltShutter_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_moving),           MP_ROM_PTR(&py_guidepulser_isMoving_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_stop_time),       MP_ROM_PTR(&py_guidepulser_getStopTime_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_start_time),      MP_ROM_PTR(&py_guidepulser_getStartTime_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_flip_ra),         MP_ROM_PTR(&py_guidepulser_getFlipRa_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_flip_dec),        MP_ROM_PTR(&py_guidepulser_getFlipDec_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_flip_ra),         MP_ROM_PTR(&py_guidepulser_setFlipRa_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_flip_dec),        MP_ROM_PTR(&py_guidepulser_setFlipDec_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_hw_err),          MP_ROM_PTR(&py_guidepulser_getHwErr_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_shutter_open),     MP_ROM_PTR(&py_guidepulser_isShutterOpen_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutter_remaining),   MP_ROM_PTR(&py_guidepulser_getRemainingShutter_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_panicking),        MP_ROM_PTR(&py_guidepulser_isPanicking_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable_led),          MP_ROM_PTR(&py_guidepulser_enableLed_obj) },
    { MP_ROM_QSTR(MP_QSTR_disable_led),         MP_ROM_PTR(&py_guidepulser_disableLed_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_module_globals, pyb_module_globals_table);

const mp_obj_module_t guidepulser_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pyb_module_globals,
};
