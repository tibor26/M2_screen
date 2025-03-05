/**
 * @file app.h
 *
 */

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/
#include <stdint.h>
#include "M2_ui/ui_app.h"


/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    INTERLOCK_OPEN    = 0,
    INTERLOCK_CLOSED  = 1,
    INTERLOCK_UNKNOWN = 2
} interlock_t;

typedef enum
{
    NOZZLE_OPEN    = 0,
    NOZZLE_CLOSED  = 1,
    NOZZLE_UNKNOWN = 2
} nozzle_t;

typedef enum
{
    MILK_MAKER_INIT  = 0,
    MILK_MAKER_BLEND,
    MILK_MAKER_POUR,
    MILK_MAKER_SPIN
} milk_maker_step_t;

void start_motor_manual(int16_t speed);
void stop_motor(void);
uint16_t arc_value_to_speed(uint8_t value);
void send_speed(int16_t speed);
void program_speed_update(modes_t program, uint16_t time_count);
void set_checksum(void);
uint8_t read_sensors(void);
uint8_t read_mode(void);
void init_check_status(void);
uint8_t spi_command(uint8_t cmd, uint8_t data0, uint8_t data1, uint32_t Timeout);
uint8_t get_motor_state(void);
interlock_t get_interlock(void);
nozzle_t get_nozzle(void);
void set_milk_maker_step(milk_maker_step_t step);
milk_maker_step_t get_milk_maker_step(void);
uint8_t get_milk_maker_active();
void start_motor_milk_maker_blend(void);
uint8_t start_milk_maker_pour(void);

void start_motor_milk_maker_spin(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*APP_H*/
