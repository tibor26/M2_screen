/**
 * @file delay.h
 *
 */

#ifndef DELAY_H
#define DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/
#include <stdint.h>

void delay_ms(uint16_t count);
void delay_us(uint32_t count);
void delay_measure_start(void);
void delay_measure_end(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DELAY_H*/
