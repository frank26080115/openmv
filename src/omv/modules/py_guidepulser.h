#ifndef _PY_GUIDEPULSER_H_
#define _PY_GUIDEPULSER_H_

#include <stdint.h>
#include <stdbool.h>

void guidepulser_init(void);
void guidepulser_write(uint8_t reg, uint8_t data);
void guidepulser_writeIfDiff(uint8_t data);
uint32_t guidepulser_move(int32_t ra, int32_t dec, uint32_t grace);
void guidepulser_stop(void);
void guidepulser_task(void);
void guidepulser_panic(int state);
void guidepulser_shutter(uint32_t tspan);
void guidepulser_haltShutter(void);
bool guidepulser_isMoving(void);
uint32_t guidepulser_getStopTime(void);
uint32_t guidepulser_getStartTime(void);
bool guidepulser_isShutterOpen(void);
bool guidepulser_isPanic(void);
void guidepulser_enableLed(void);
void guidepulser_disableLed(void);
void guidepulser_setFlipRa(int32_t x);
int guidepulser_getFlipRa(void);
void guidepulser_setFlipDec(int32_t x);
int guidepulser_getFlipDec(void);

#endif
