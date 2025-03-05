/**
 * @file TFT.c
 *
 */

/*********************
 *      INCLUDES
 ********************/
#include "TFT.h"
#include "main.h"
#include "ltdc.h"
#include "dsihost.h"
#include "dma2d.h"
#include "tim.h"

/*********************
 *      DEFINES
 *********************/
// backlight is set using timer 1 channel 4
// max duty cycle is 200 (100% brightness)
#define TFT_BACKLIGHT_MAX  200  // 100%
#define TFT_BACKLIGHT_DIM  20   // 5%


/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/
static void LCD_Set_Default_Clock(void);
static uint32_t SetPanelConfig(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static volatile uint16_t framebuffer[LCD_HEIGHT][LCD_WIDTH] = { 0 };

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void tft_initialize(void)
{
    LCD_Set_Default_Clock();
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)framebuffer, LTDC_LAYER_1);
    if (SetPanelConfig() != 0)
    {
        Error_Handler();
    }
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Backlight
    tft_backlight_max();
}


void tft_fill_rectangle(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height, uint16_t Color)
{
    for (uint16_t i = 0; i < Height; i++)
    {
        for (uint16_t j = 0; j < Width; j++)
        {
            framebuffer[Ypos + i][Xpos + j] = Color;
        }
    }
}


void tft_fill(uint16_t Color)
{
    for (uint32_t i = 0; i < LCD_HEIGHT*LCD_WIDTH; i++)
    {
        ((volatile uint16_t *)framebuffer)[i] = Color;
    }
}

void tft_set_image_rgb888(uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height, const uint8_t * image)
{
    uint32_t k = 0;
    for (uint32_t i = 0; i < Height; i++)
    {
        for (uint16_t j = 0; j < Width; j++)
        {
            uint16_t px = ((image[k] >> 3) << 11) | ((image[k + 1] >> 2) << 5) | (image[k + 2] >> 2);
            framebuffer[Ypos + i][Xpos + j] = px;
            k += 3;
        }
    }
}


/**
  * @brief  Converts a line to an ARGB8888 pixel format.
  * @param  pSrc: Pointer to source buffer
  * @param  pDst: Output color
  * @param  xSize: Buffer width
  * @param  ColorMode: Input color mode
  * @retval None
  */
void tft_copy_buffer_dma(uint32_t *pSrc, uint32_t *pDst, uint16_t x, uint16_t y, uint16_t xsize, uint16_t ysize)
{
    uint32_t destination = (uint32_t)pDst + (y * 400 + x) * 4;
    uint32_t source      = (uint32_t)pSrc;

    hdma2d.Init.OutputOffset = 400 - xsize;

    /* DMA2D Initialization */
    if (HAL_DMA2D_Init(&hdma2d) == HAL_OK)
    {
        if (HAL_DMA2D_Start(&hdma2d, source, destination, xsize, ysize) == HAL_OK)
        {
            /* Polling For DMA transfer */
            HAL_DMA2D_PollForTransfer(&hdma2d, 100);
        }
    }
}


void tft_set_backlight(uint8_t value)
{
    TIM1->CCR4 = value;
}


void tft_toggle_backlight(void)
{
    if (TIM1->CCR4 == TFT_BACKLIGHT_MAX)
    {
        tft_backlight_dim();
    }
    else
    {
        tft_backlight_max();
    }
}


void tft_backlight_max(void)
{
    tft_set_backlight(TFT_BACKLIGHT_MAX);
}


void tft_backlight_dim(void)
{
    tft_set_backlight(TFT_BACKLIGHT_DIM);
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static void LCD_Set_Default_Clock(void)
{
    RCC_PeriphCLKInitTypeDef  DSIPHYInitPeriph;

    /* Switch to DSI PHY PLL clock */
    DSIPHYInitPeriph.PeriphClockSelection = RCC_PERIPHCLK_DSI;
    DSIPHYInitPeriph.DsiClockSelection    = RCC_DSICLKSOURCE_DSIPHY;

    HAL_RCCEx_PeriphCLKConfig(&DSIPHYInitPeriph);

    //  /* LCD Reset */
    HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(TFT_RESET_GPIO_Port, TFT_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(120);
}


/**
  * @brief  Check for user input.
  * @param  None
  * @retval Input state (1 : active / 0 : Inactive)
  */
static uint32_t SetPanelConfig(void)
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (HAL_DSI_Start(&hdsi) != HAL_OK) return 1;
    /* Set Panel Related Registers */
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xF0, 0xC3) != HAL_OK) return 1;
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xF0, 0x96) != HAL_OK) return 2;
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xF0, 0xA5) != HAL_OK) return 3;
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xED, 0xC3) != HAL_OK) return 4;

    uint8_t InitParam1[2] = { 0x40, 0x0F };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 2, 0xE4, InitParam1) != HAL_OK) return 5;
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xE7, 0x83) != HAL_OK) return 6;
    uint8_t InitParam2[4] = { 0x33, 0x02, 0x25, 0x04 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, 0xC3, InitParam2) != HAL_OK) return 7;
    uint8_t InitParam3[14] = { 0xB2, 0xF5, 0xBD, 0x24, 0x22, 0x25, 0x10, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 14, 0xE5, InitParam3) != HAL_OK) return 8;
    uint8_t InitParam4[7] = { 0x00, 0x55, 0x00, 0x00, 0x00, 0x49, 0x22 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 7, 0xEC, InitParam4) != HAL_OK) return 9;

    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0x36, 0x0C) != HAL_OK) return 10;  //Memory Data Access Control
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xB2, 0x00) != HAL_OK) return 11;    //Gate Scan Control
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0x3A, 0x05) != HAL_OK) return 12;    //Interface Pixel Format (5: RGB565, 7: RGB888)
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xC5, 0x6B) != HAL_OK) return 13;
    ////-------------------------------------Gamma Cluster Setting-------------------------------------------//
    uint8_t InitParam5[14] = { 0x88, 0x0B, 0x10, 0x08, 0x07, 0x03, 0x2C, 0x33, 0x43, 0x08, 0x16, 0x16, 0x2A, 0x2E };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 14, 0xE0, InitParam5) != HAL_OK) return 14;
    uint8_t InitParam6[14] = { 0x88, 0x0B, 0x10, 0x08, 0x06, 0x02, 0x2B, 0x32, 0x42, 0x09, 0x16, 0x15, 0x2A, 0x2E };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 14, 0xE1, InitParam6) != HAL_OK) return 15;
    uint8_t InitParam7[2] = { 0xC0, 0x63 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 2, 0xA4, InitParam7) != HAL_OK) return 16;

    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xD9, 0xD2) != HAL_OK) return 17;

    uint8_t InitParam8[2] = { 0xC7, 0x31 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 2, 0xB6, InitParam8) != HAL_OK) return 18;

    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xB3, 0x01) != HAL_OK) return 19;//Video Mode Display Control    //1DOT

    uint8_t InitParam9[4] = { 0x77, 0x07, 0xC2, 0x15 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, 0xC1, InitParam9) != HAL_OK) return 20;//Power Control 1 //VGH+15V  VGL-10V //Gamma resistor VREGP and VREGN voltage level selection.

    uint8_t InitParam10[9] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x16, 0x2A, 0x8A, 0x02 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 9, 0xA5, InitParam10) != HAL_OK) return 21;

    uint8_t InitParam11[7] = { 0x0A, 0x5A, 0x23, 0x10, 0x25, 0x02, 0x00 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 7, 0xBA, InitParam11) != HAL_OK) return 22;

    uint8_t InitParam12[8] = { 0x00, 0x27, 0x00, 0x29, 0x82, 0x87, 0x18, 0x00 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 8, 0xBB, InitParam12) != HAL_OK) return 23;

    uint8_t InitParam13[9] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x16, 0x2A, 0x8A, 0x02 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 9, 0xA6, InitParam13) != HAL_OK) return 24;

    uint8_t InitParam14[8] = { 0x00, 0x27, 0x00, 0x29, 0x82, 0x87, 0x18, 0x00 };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 8, 0xBC, InitParam14) != HAL_OK) return 25;

    uint8_t InitParam15[11] = { 0xA1, 0xB2, 0x2B, 0x1A, 0x56, 0x43, 0x34, 0x65, 0xFF, 0xFF, 0x0F };
    if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 11, 0xBD, InitParam15) != HAL_OK) return 26;

    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x21, 0x00) != HAL_OK) return 27;

    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0x35, 0x00) != HAL_OK) return 28;

    /* Exit Sleep Mode*/
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x11, 0x00) != HAL_OK) return 29;

    HAL_Delay(120);

    /* Clear LCD_FRAME_BUFFER */
    //memset((uint32_t *)LCD_FRAME_BUFFER, 0x00, 0xFFFFF);

    /* Display On */
    if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P0, 0x29, 0x00) != HAL_OK) return 30;

    HAL_Delay(120);

    //-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////自检2///////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xF0, 0xA5) != HAL_OK) return 34;
    //  if (HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, 0xB0, 0xA5) != HAL_OK) return 35;
    //  uint8_t InitParam17[9] = {0x40,0x00,0x3F,0x00,0x0A,0x0A,0xEA,0xEA,0x03};
    //  if (HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 9, 0xCC, InitParam17) != HAL_OK) return 36;
    //

      /* All setting OK */
    return 0;
}


uint8_t * tft_get_framebuffer(void)
{
    return (uint8_t*)framebuffer;
}

#if 0
static void convertTo32Bit(const uint8_t* byteData, uint32_t* int24Data, int width, int height)
{
    int j = 0;
    for (int i = 0; i < width * height; i++)
    {
        int24Data[i] = (uint32_t)(byteData[j] << 16 | byteData[j + 1] << 8 | byteData[j + 2]); // R G B
        j = j + 3;
    }
}
#endif