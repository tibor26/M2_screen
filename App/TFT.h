/**
 * @file TFT.h
 *
 */

#ifndef TFT_H
#define TFT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/
#include <stdint.h>


/*********************
 *      DEFINES
 *********************/
#define TFT_COLOR_BLACK    0x0000
#define TFT_COLOR_WHITE    0xFFFF
#define TFT_COLOR_RED      0xF800
#define TFT_COLOR_GREEN    0x07E0
#define TFT_COLOR_BLUE     0x001F
#define TFT_COLOR_YELLOW   0xFFE0
#define TFT_COLOR_CYAN     0x07FF
#define TFT_COLOR_MAGENTA  0xF81F
#define TFT_COLOR_ORANGE   0xFBE0


/**********************
 *      TYPEDEFS
 **********************/

void tft_initialize(void);
void tft_fill_rectangle(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height, uint16_t Color);
void tft_fill(uint16_t Color);
void tft_set_image_rgb888(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height, const uint8_t * image);
void tft_copy_buffer_dma(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize);
void tft_set_backlight(uint8_t value);
void tft_toggle_backlight(void);
void tft_backlight_max(void);
void tft_backlight_dim(void);
uint8_t * tft_get_framebuffer(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*TFT_H*/
