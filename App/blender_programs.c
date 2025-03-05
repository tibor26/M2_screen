/**
 * @file blender_programs.c
 *
 */

/*********************
 *      INCLUDES
 ********************/
#include "blender_programs.h"
#include "app.h"

/*********************
 *      DEFINES
 *********************/
#define PROGRAM_RUN_TIME_DEFAULT      600

#define PROGRAM_SOFT_START_INIT       700
#define PROGRAM_SOFT_START_INC_2S     350   // 2s soft start from 8000 rpm increment
#define PROGRAM_SOFT_START_INC_05S    2860  // 0.5s soft start from 700 rpm increment

#define PROGRAM_RESUME_SPEED_MAX      2000  // max speed to resume without ramp up
#define PROGRAM_RESUME_STEPS          5     // resume ramp up steps (0.5s)

#define PROGRAM_RUN_TIME_SMOOTHIE     600
#define PROGRAM_START_SPEED_SMOOTHIE  8000
#define PROGRAM_MAX_SPEED_SMOOTHIE    15000
#define PROGRAM_INC_SPEED_SMOOTHIE    67

#define PROGRAM_RUN_TIME_CLEANING     600
#define PROGRAM_START_SPEED_CLEANING  8000
#define PROGRAM_MAX_SPEED_CLEANING    15000

#define PROGRAM_RUN_TIME_GRIND        600
#define PROGRAM_START_SPEED_GRIND     700
#define PROGRAM_INIT_SPEED_GRIND      8000
#define PROGRAM_MAX_SPEED_GRIND       14000
#define PROGRAM_INC_SPEED_SS_GRIND    1460
#define PROGRAM_INC_SPEED_GRIND       10

#define PROGRAM_RUN_TIME_SOUP         4500
#define PROGRAM_START_SPEED_SOUP      700
#define PROGRAM_INIT_SPEED_SOUP       10000
#define PROGRAM_INC_SPEED_SS_SOUP     1860
#define PROGRAM_MAX_SPEED_SOUP        15000

#define PROGRAM_RUN_TIME_ICE_CRUSH    600
#define PROGRAM_START_SPEED_ICE_CRUSH 700
#define PROGRAM_MAX_SPEED_ICE_CRUSH   15000

/**********************
 *      TYPEDEFS
 **********************/

static int16_t speed_ramp_current = 0;
static int16_t speed_ramp_inc = 0;
static int16_t speed_ramp_end = 0;
static int16_t resume_ramp_inc = 0;
static int16_t resume_ramp_end = 0;
static int16_t last_target_speed = 0;


/**********************
 *   STATIC FUNCTIONS
 **********************/


// send speed
// ramp up speed if we are resuming
static void program_send_speed(int16_t speed)
{
    if (resume_ramp_end == 0)
    {
        last_target_speed = speed;
        send_speed(speed);
    }
    else
    {
        int16_t speed_r = last_target_speed + resume_ramp_inc;
        if (speed <= speed_r)
        {
            send_speed(speed);
            last_target_speed = speed;
            resume_ramp_end = 0;
        }
        else
        {
            send_speed(speed_r);
            last_target_speed = speed_r;
        }
    }
}


static void speed_ramp_init(int16_t start_speed, int16_t increment, int16_t end_speed)
{
    speed_ramp_current = start_speed;
    speed_ramp_inc = increment;
    speed_ramp_end = end_speed;
    program_send_speed(start_speed);
}

static void speed_ramp(void)
{
    if (speed_ramp_current < speed_ramp_end)
    {
        speed_ramp_current += speed_ramp_inc;
        if (speed_ramp_current > speed_ramp_end)
        {
            speed_ramp_current = speed_ramp_end;
        }
        program_send_speed(speed_ramp_current);
    }
    else if (resume_ramp_end)
    {
        program_send_speed(resume_ramp_end);
    }
}


/**********************
 *   GLOBAL FUNCTIONS
 **********************/


int16_t blender_program_start_speed(modes_t program)
{
    speed_ramp_init(0, 0, 0);  // disable speed ramp
    resume_ramp_end = 0; // disable resume ramp up
    switch (program)
    {
    case SMOOTHIE:
        last_target_speed = PROGRAM_START_SPEED_SMOOTHIE;
        break;
    case SOUP:
        last_target_speed = PROGRAM_START_SPEED_SOUP;
        break;
    case CLEANING:
        last_target_speed = PROGRAM_START_SPEED_CLEANING;
        break;
    case GRIND:
        last_target_speed = PROGRAM_START_SPEED_GRIND;
        break;
    case ICE_CRUSH:
        last_target_speed = PROGRAM_START_SPEED_ICE_CRUSH;
        break;
    case MANUAL:
        last_target_speed = 0;
        break;
    }
    return last_target_speed;
}


int16_t blender_program_resume_init(void)
{
    if (last_target_speed > PROGRAM_RESUME_SPEED_MAX)
    {
        resume_ramp_inc = (last_target_speed - PROGRAM_SOFT_START_INIT) / PROGRAM_RESUME_STEPS;
        resume_ramp_end = last_target_speed;
        last_target_speed = PROGRAM_SOFT_START_INIT;
    }
    else
    {
        resume_ramp_end = 0;
    }
    return last_target_speed;
}


int16_t blender_program_run_time(modes_t program)
{
    switch (program)
    {
    case SMOOTHIE:
        return PROGRAM_RUN_TIME_SMOOTHIE;
        break;
    case SOUP:
        return PROGRAM_RUN_TIME_SOUP;
        break;
    case CLEANING:
        return PROGRAM_RUN_TIME_CLEANING;
        break;
    case GRIND:
        return PROGRAM_RUN_TIME_GRIND;
        break;
    case ICE_CRUSH:
        return PROGRAM_RUN_TIME_ICE_CRUSH;
        break;
    case MANUAL:
        break;
    }
    return PROGRAM_RUN_TIME_DEFAULT;
}


void blender_program_run_smoothie(uint16_t time_count)
{
    switch (time_count)
    {
    case 594:
        speed_ramp_init(PROGRAM_START_SPEED_SMOOTHIE + PROGRAM_SOFT_START_INC_2S, PROGRAM_SOFT_START_INC_2S, PROGRAM_MAX_SPEED_SMOOTHIE);
        break;
    case 465:
    case 430:
    case 395:
        speed_ramp_init(PROGRAM_SOFT_START_INIT, PROGRAM_SOFT_START_INC_05S, PROGRAM_MAX_SPEED_SMOOTHIE);
        break;
    case 490:
    case 455:
    case 420:
    case 385:
        program_send_speed(0);
        break;
    case 360:
        speed_ramp_init(PROGRAM_START_SPEED_SMOOTHIE, PROGRAM_INC_SPEED_SMOOTHIE, PROGRAM_MAX_SPEED_SMOOTHIE);
        break;
    default:
        speed_ramp();
        break;
    }
}


void blender_program_run_cleaning(uint16_t time_count)
{
    switch (time_count)
    {
    case 594:
    case 284:
        speed_ramp_init(PROGRAM_START_SPEED_CLEANING + PROGRAM_SOFT_START_INC_2S, PROGRAM_SOFT_START_INC_2S, PROGRAM_MAX_SPEED_CLEANING);
        break;
    case 425:
    case 380:
    case 335:
    case 165:
    case 130:
    case 95:
    case 60:
        speed_ramp_init(PROGRAM_SOFT_START_INIT, PROGRAM_SOFT_START_INC_05S, PROGRAM_MAX_SPEED_CLEANING);
        break;
    case 460:
    case 415:
    case 370:
    case 325:
    case 200:
    case 155:
    case 120:
    case 85:
        program_send_speed(0);
        break;
    case 290:
        program_send_speed(PROGRAM_START_SPEED_CLEANING);
        break;
    default:
        speed_ramp();
        break;
    }
}


void blender_program_run_grind(uint16_t time_count)
{
    switch (time_count)
    {
    case 599:
        speed_ramp_init(PROGRAM_START_SPEED_GRIND + PROGRAM_INC_SPEED_SS_GRIND, PROGRAM_INC_SPEED_SS_GRIND, PROGRAM_INIT_SPEED_GRIND);
    case 595:
        speed_ramp_init(PROGRAM_INIT_SPEED_GRIND, PROGRAM_INC_SPEED_GRIND, PROGRAM_MAX_SPEED_GRIND);
    default:
        speed_ramp();
        break;
    }
}


void blender_program_run_soup(uint16_t time_count)
{
    switch (time_count)
    {
    case 4499:
        speed_ramp_init(PROGRAM_START_SPEED_SOUP + PROGRAM_INC_SPEED_SS_SOUP, PROGRAM_INC_SPEED_SS_SOUP, PROGRAM_INIT_SPEED_SOUP);
        break;
    case 4440:
        program_send_speed(PROGRAM_MAX_SPEED_SOUP);
        break;
    case 30:
        program_send_speed(PROGRAM_INIT_SPEED_SOUP);
        break;
    default:
        speed_ramp();
        break;
    }
}


void blender_program_run_ice_crush(uint16_t time_count)
{
    if (time_count == 599)
    {
        speed_ramp_init(PROGRAM_SOFT_START_INIT + PROGRAM_SOFT_START_INC_05S, PROGRAM_SOFT_START_INC_05S, PROGRAM_MAX_SPEED_ICE_CRUSH);
    }
    else if (time_count >= 145)
    {
        switch ((PROGRAM_RUN_TIME_ICE_CRUSH - time_count) % 35)
        {
        case 0:
            speed_ramp_init(PROGRAM_SOFT_START_INIT, PROGRAM_SOFT_START_INC_05S, PROGRAM_MAX_SPEED_ICE_CRUSH);
            break;
        case 10:
            program_send_speed(0);
            break;
        default:
            speed_ramp();
            break;
        }
    }
    else
    {
        switch (time_count)
        {
        case 135:
        case 85:
        case 35:
            program_send_speed(0);
            break;
        case 95:
        case 45:
            speed_ramp_init(PROGRAM_SOFT_START_INIT, PROGRAM_SOFT_START_INC_05S, PROGRAM_MAX_SPEED_ICE_CRUSH);
            break;
        default:
            speed_ramp();
            break;
        }
    }
}
