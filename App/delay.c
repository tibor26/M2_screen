/**
 * @file delay.c
 *
 */

/*********************
 *      INCLUDES
 ********************/
#include "delay.h"
#include "main.h"

static volatile uint32_t delay_measure = 0;

/*********************
 *      DEFINES
 *********************/
#define DELAY_TIMER  TIM7


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void delay_us(uint32_t count)
{
    if (count)
    {
        DELAY_TIMER->CR1 = 0;
        DELAY_TIMER->ARR = count;
        DELAY_TIMER->CNT = 0;
        DELAY_TIMER->SR = 0;
        DELAY_TIMER->CR1 = TIM_CR1_CEN;
        while ((DELAY_TIMER->SR & TIM_SR_UIF) == 0) ;
    }
}

void delay_ms(uint16_t count)
{
    delay_us(count * 1000);
}

void delay_measure_start(void)
{
    TIM6->CR1 = 0;
    TIM6->CNT = 0;
    TIM6->CR1 = TIM_CR1_CEN;
}

void delay_measure_end(void)
{
    TIM6->CR1 = 0;
    delay_measure = TIM6->CNT;
}
