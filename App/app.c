/**
 * @file app.c
 *
 */

/*********************
 *      INCLUDES
 ********************/
#include "app.h"
#include "main.h"
#include "delay.h"
#include "lvgl/lvgl.h"
#include "M2_ui/screens.h"
#include "M2_ui/ui_app.h"
#include "M2_ui/ui.h"
#include "blender_programs.h"

/*********************
 *      DEFINES
 *********************/
#define COMMAND_RESPONSE_TIMEOUT      30
#define BLENDER_MANUAL_STEP           400   // RPM of step
#define BLENDER_MANUAL_START          600   // RPM of 1st step


/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    GET_SENSORS  = 1,
    GET_MODE,
    SET_MODE,
    GET_SPEED,
    SET_SPEED,
    SET_P_FACTOR,
    SET_I_FACTOR,
    SET_D_FACTOR,
} commands_spi_t;


typedef enum
{
    MODE_INIT             = 0,
    MODE_SLEEP,
    MODE_STANDBY,
    MODE_MILK_MAKER_BLEND,
    MODE_MILK_MAKER_POUR,
    MODE_MILK_MAKER_SPIN,
    MODE_BLENDER_PULSE,
    MODE_MANUAL
} app_mode_t;


typedef enum
{
    JUG_EMPTY,
    JUG_MILK_MAKER,
    JUG_BLENDER,
    JUG_PERSONAL_BLENDER,
    JUG_OTHER,
    JUG_UNKNOWN,
} jug_type_t;


typedef enum
{
    EXIT_CODE_RUNNING        = 0,
    EXIT_CODE_INTERLOCK,
    EXIT_CODE_NOZZLE,
    EXIT_CODE_THERMOSTAT,
    EXIT_CODE_TIMEOUT,
    EXIT_CODE_COMMAND_ERROR,
    EXIT_CODE_DONE,
    EXIT_CODE_COMMAND_CHANGE,
    EXIT_CODE_SPEED_ERROR
} app_exit_code_t;


extern SPI_HandleTypeDef hspi1;

static jug_type_t jug_type = JUG_UNKNOWN;
static interlock_t interlock_state = INTERLOCK_UNKNOWN;
static nozzle_t nozzle_state = NOZZLE_UNKNOWN;
static app_mode_t control_board_mode = MODE_INIT;
static app_exit_code_t control_board_exit_code = EXIT_CODE_RUNNING;

static uint8_t motor_on = 0;
static milk_maker_step_t milk_maker_step = MILK_MAKER_BLEND;

static lv_timer_t* timer_check_status = NULL;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/


// measure duration
#if 0
volatile uint32_t duration = 0;

TIM6->CR1 = 0;
TIM6->CNT = 0;
TIM6->CR1 = TIM_CR1_CEN;

TIM6->CR1 = 0;
duration = TIM6->CNT;
#endif


static uint8_t data_tx[6] = { 0xAA, 0x00, 0x00, 0x00, 0x00, 0xA5 };
static uint8_t data_rx[6] = { 0x00 };

void start_motor_manual(int16_t speed)
{
    // set speed
    send_speed(speed);

    delay_us(100);

    // set mode to manual
    spi_command(SET_MODE, MODE_MANUAL, 0, 10);
    motor_on = 1;
}

void start_motor_milk_maker_blend(void)
{
    spi_command(SET_MODE, MODE_MILK_MAKER_BLEND, 0, 10);
    motor_on = 1;
}

uint8_t start_milk_maker_pour(void)
{
    spi_command(SET_MODE, MODE_MILK_MAKER_POUR, 0, 10);
    motor_on = 1;
    return data_rx[3];
}

void start_motor_milk_maker_spin(void)
{
    spi_command(SET_MODE, MODE_MILK_MAKER_SPIN, 0, 10);
    motor_on = 1;
}

void stop_motor(void)
{
    // set mode to standby
    spi_command(SET_MODE, MODE_STANDBY, 0, 10);

    motor_on = 0;
}

uint8_t get_motor_state(void)
{
    return motor_on;
}

interlock_t get_interlock(void)
{
    return interlock_state;
}

nozzle_t get_nozzle(void)
{
    return nozzle_state;
}


uint16_t arc_value_to_speed(uint8_t value)
{
    // from 600 rpm to 15000 rpm (max arc value is 37)
    return ((BLENDER_MANUAL_START - BLENDER_MANUAL_STEP) + BLENDER_MANUAL_STEP * value);
}


void send_speed(int16_t speed)
{
    // set speed
    spi_command(SET_SPEED, speed >> 8, speed & 0xFF, 10);
}


void spi1_transmit(uint8_t * data, uint16_t Size)
{
    CLEAR_BIT(SPI1->CFG2, SPI_CFG2_COMM); // enable full duplex
    //MODIFY_REG(SPI1->CFG2, SPI_CFG2_COMM, SPI_DIRECTION_2LINES_TXONLY);
    MODIFY_REG(SPI1->CR2, SPI_CR2_TSIZE, Size);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);
    SET_BIT(SPI1->CR1, SPI_CR1_CSTART);
    uint16_t i = 0;
    while (i < Size)
    {
        if (SPI1->SR & SPI_SR_TXP)
        {
            *((__IO uint8_t *)&SPI1->TXDR) = data[i];
            i++;
        }
    }
    while ((SPI1->SR & SPI_SR_EOT) == 0) ;
    SET_BIT(SPI1->IFCR, SPI_IFCR_EOTC);
    SET_BIT(SPI1->IFCR, SPI_IFCR_TXTFC);
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
}


void spi1_receive(uint8_t * data, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickend = HAL_GetTick() + Timeout;
    //CLEAR_BIT(SPI1->CFG2, SPI_CFG2_COMM); // enable full duplex
    MODIFY_REG(SPI1->CFG2, SPI_CFG2_COMM, SPI_DIRECTION_2LINES_RXONLY);
    MODIFY_REG(SPI1->CR2, SPI_CR2_TSIZE, Size);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);
    SET_BIT(SPI1->CR1, SPI_CR1_CSTART);
    uint16_t i = 0;
    while (i < Size && HAL_GetTick() <= tickend)
    {
        if (SPI1->SR & SPI_SR_RXP)
        {
            data[i] = *((__IO uint8_t *)&SPI1->RXDR);
            i++;
        }
    }
//    while ((SPI1->SR & SPI_SR_EOT) == 0) ;
//    SET_BIT(SPI1->IFCR, SPI_IFCR_EOTC);
    SET_BIT(SPI1->IFCR, SPI_IFCR_TXTFC);
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
}

uint8_t spi_command(uint8_t cmd, uint8_t data0, uint8_t data1, uint32_t Timeout)
{
    uint32_t tickend = HAL_GetTick() + Timeout;

    data_tx[1] = cmd;
    data_tx[2] = data0;
    data_tx[3] = data1;
    set_checksum();

    CLEAR_BIT(SPI1->CFG2, SPI_CFG2_COMM); // enable full duplex
    MODIFY_REG(SPI1->CR2, SPI_CR2_TSIZE, 12);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);
    SET_BIT(SPI1->CR1, SPI_CR1_CSTART);
    uint8_t count_rx = 0;
    uint8_t count_tx = 0;

    // tx
    while ((count_rx < 6 || count_tx < 6) && HAL_GetTick() <= tickend)
    {
        if (SPI1->SR & SPI_SR_TXP && count_tx < 6)
        {
            *((__IO uint8_t *)&SPI1->TXDR) = data_tx[count_tx];
            count_tx++;
        }
        if (SPI1->SR & SPI_SR_RXP && count_rx < 6)
        {
            volatile uint8_t dummy = *((__IO uint8_t *)&SPI1->RXDR);
            UNUSED(dummy);
            count_rx++;
        }
    }

    // wait for rx ready
    data_rx[1] = 0; // clear response command code
    uint16_t timeout = COMMAND_RESPONSE_TIMEOUT;
    while (HAL_GPIO_ReadPin(SPI1_NOTIFY_GPIO_Port, SPI1_NOTIFY_Pin) == 0 && timeout)
    {
        delay_us(1);
        timeout--;
    }

    // rx
    count_tx = 0;
    count_rx = 0;
    while ((count_rx < 6 || count_tx < 6) && HAL_GetTick() <= tickend)
    {
        if (SPI1->SR & SPI_SR_TXP && count_tx < 6)
        {
            *((__IO uint8_t *)&SPI1->TXDR) = 0;
            count_tx++;
        }
        if (SPI1->SR & SPI_SR_RXP && count_rx < 6)
        {
            data_rx[count_rx] = *((__IO uint8_t *)&SPI1->RXDR);
            count_rx++;
        }
    }

    while ((SPI1->SR & SPI_SR_EOT) == 0 && HAL_GetTick() <= tickend) ;
    SET_BIT(SPI1->IFCR, SPI_IFCR_EOTC | SPI_IFCR_TXTFC);
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);

    if (data_rx[1] == cmd)
    {
        return 1;
    }
    return 0;
}


uint8_t read_sensors(void)
{
    if (spi_command(GET_SENSORS, 0, 0, 10))
    {
        interlock_state = data_rx[2] & 0x01;
        nozzle_state = (data_rx[2] >> 1) & 0x01;
        jug_type = data_rx[3];
        return 1;
    }

    return 0;
}


uint8_t read_mode(void)
{
    if (spi_command(GET_MODE, 0, 0, 10))
    {
        control_board_mode = data_rx[2];
        control_board_exit_code = data_rx[3];
        return 1;
    }

    return 0;
}

void set_checksum(void)
{
    data_tx[4] = data_tx[1] + data_tx[2] + data_tx[3];
}


void program_speed_update(modes_t program, uint16_t time_count)
{
    switch (program)
    {
    case SMOOTHIE:
        blender_program_run_smoothie(time_count);
        break;
    case CLEANING:
        blender_program_run_cleaning(time_count);
        break;
    case GRIND:
        blender_program_run_grind(time_count);
        break;
    case SOUP:
        blender_program_run_soup(time_count);
        break;
    case ICE_CRUSH:
        blender_program_run_ice_crush(time_count);
        break;
    case MANUAL:
        break;
    }
}

void set_milk_maker_step(milk_maker_step_t step)
{
    milk_maker_step = step;
}

milk_maker_step_t get_milk_maker_step(void)
{
    return milk_maker_step;
}


uint8_t get_milk_maker_active(void)
{
    return (lv_display_get_screen_active(NULL) == objects.milk_maker_program);
}


void check_status(lv_timer_t* timer)
{
    if (motor_on == 0)
    {
        if (read_sensors())
        {
            if (lv_display_get_screen_active(NULL) == objects.nama)
            {
                if (jug_type == JUG_MILK_MAKER)
                {
                    loadScreen(SCREEN_ID_MILK_MAKER);
                    set_encoder_to(objects.roller_milk_maker);
                    tft_backlight_wake();
                }
                else if (jug_type == JUG_BLENDER)
                {
                    loadScreen(SCREEN_ID_BLENDER);
                    set_encoder_to(objects.roller_blender);
                    tft_backlight_wake();
                }
            }
            else
            {
                if (jug_type == JUG_EMPTY || jug_type == JUG_UNKNOWN)
                {
                    enjoy_cancel(); // cancel enjoy timer
                    loadScreen(SCREEN_ID_NAMA);
                    set_encoder_to(objects.nama_logo);
                    tft_backlight_wake();
                }
                else if (jug_type == JUG_MILK_MAKER)
                {
                    if (milk_maker_step == MILK_MAKER_BLEND)
                    {
                        if (interlock_state == INTERLOCK_OPEN)
                        {
                            milk_maker_message(CLOSE, LID);
                        }
                        else if (nozzle_state == NOZZLE_OPEN)
                        {
                            milk_maker_message(CLOSE, SPOUT);
                        }
                        else
                        {
                            milk_maker_message(START_MILK_MAKER, 0);
                        }
                    }
                    else if (milk_maker_step == MILK_MAKER_POUR || milk_maker_step == MILK_MAKER_SPIN)
                    {
                        if (interlock_state == INTERLOCK_OPEN)
                        {
                            milk_maker_message(CLOSE, LID);
                        }
                        else if (nozzle_state == NOZZLE_CLOSED)
                        {
                            milk_maker_message(OPEN, SPOUT);
                        }
                        else
                        {
                            milk_maker_message(START_MILK_MAKER, 0);
                            start_program_milk_maker();
                            if (milk_maker_step == MILK_MAKER_POUR)
                            {
                                start_milk_maker_pour();
                            }
                            else
                            {
                                start_motor_milk_maker_spin();
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        // when motor is running check if it's still in manual mode and the exit code
        if (read_mode())
        {
            if (get_milk_maker_active())
            {
                if (milk_maker_step == MILK_MAKER_POUR && control_board_exit_code == EXIT_CODE_DONE)
                {
                    if (get_milk_maker_paused())
                    {
                        stop_motor();
                        set_milk_maker_step(MILK_MAKER_SPIN);
                    }
                }
                else if (control_board_exit_code != EXIT_CODE_RUNNING)
                {
                    if (milk_maker_step == MILK_MAKER_BLEND && control_board_exit_code == EXIT_CODE_NOZZLE)
                    {
                        set_milk_maker_step(MILK_MAKER_POUR);
                        init_milk_maker_pour();
                        motor_on = 0;  // mark the motor as off so no need to pause it
                    }
                    else
                    {
                        ui_pause_program();
                        motor_on = 0;
                    }
                }
            }
            else
            {
                if (control_board_exit_code != EXIT_CODE_RUNNING)
                {
                    ui_pause_program();
                    motor_on = 0;
                }
            }
        }
    }
}

void init_check_status(void)
{
    set_encoder_to(objects.nama_logo);
    timer_check_status = lv_timer_create(check_status, 100, NULL);
}
