// Define to prevent recursive inclusion
#define _ENCODER_C_

// Files includes

#include "encoder.h"
#include "M2_ui/ui_app.h"

uint8_t key_pressed(void)
{
    if (HAL_GPIO_ReadPin(Encode_KEY_GPIO_Port, Encode_KEY_Pin) == 0)
    {
        return 1;
    }
    return 0;
}

static int16_t encoder_steps = 0;

void check_encoder(void)
{
    const uint8_t step = 1;

    int16_t new_moves = 0;
    static uint8_t encA = 2;
    static uint8_t encB = 2;

    if (encA != HAL_GPIO_ReadPin(Encode_A_GPIO_Port, Encode_A_Pin))
    {
        encA = HAL_GPIO_ReadPin(Encode_A_GPIO_Port, Encode_A_Pin);
        if (encA == encB)
        {
            new_moves = step;
            tft_backlight_wake();
        }
    }
    if (encB != HAL_GPIO_ReadPin(Encode_B_GPIO_Port, Encode_B_Pin))
    {
        encB = HAL_GPIO_ReadPin(Encode_B_GPIO_Port, Encode_B_Pin);
        if (encA == encB)
        {
            new_moves = -step;
            tft_backlight_wake();
        }
    }

    encoder_steps += new_moves;
}


int16_t enc_get_moves(void)
{
    int16_t new_moves = encoder_steps;
    encoder_steps = 0;
    return new_moves;
}








