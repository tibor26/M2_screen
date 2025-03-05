/**
 * @file blender_programs.h
 *
 */

#ifndef BLENDER_PROGRAMS_H
#define BLENDER_PROGRAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/
#include <stdint.h>
#include "M2_ui/ui_app.h"

void blender_program_run_smoothie(uint16_t time_count);
void blender_program_run_cleaning(uint16_t time_count);
void blender_program_run_grind(uint16_t time_count);
void blender_program_run_soup(uint16_t time_count);
void blender_program_run_ice_crush(uint16_t time_count);

int16_t blender_program_start_speed(modes_t program);
int16_t blender_program_run_time(modes_t program);
int16_t blender_program_resume_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*BLENDER_PROGRAMS_H*/
