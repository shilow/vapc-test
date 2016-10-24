/**
  * USART RX/TX on ring buffers, interrupterd.
  * http://we.easyelectronics.ru/STM32/uart-usart-na-stm32l-stm32.html
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H
#define __USART_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void USART_Config(void);
void USART_Put_Char(uint8_t c);
void USART_Put_Str(uint8_t *s);
void USART_Put_Int(int32_t data);
void USART1_IRQHandler(void);

#ifndef USART_TX_ONLY
uint8_t USART_Get_Char(void);
#endif // USART_TX_ONLY

#endif /* __USART_H */
