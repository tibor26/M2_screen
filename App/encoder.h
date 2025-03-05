/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __Encode_H__
#define __Encode_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"


void encoder1_init(void);
uint8_t key_pressed(void);
int16_t enc_get_moves(void);
void check_encoder(void);


#ifdef __cplusplus
}
#endif

#endif /* __Encode_H__ */
