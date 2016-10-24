/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 64

#define USARTXBASE  (uint32_t)0x40013800
#define RXNE_MASK   (uint32_t)0x00000020
#define TXE_MASK    (uint32_t)0x00000080

/* Private macro -------------------------------------------------------------*/
#define RXNE_IT_ENABLE    *(__IO uint32_t*)USARTXBASE &= ~RXNE_MASK
#define RXNE_IT_DISABLE   *(__IO uint32_t*)USARTXBASE |= RXNE_MASK
#define TXE_IT_ENABLE     *(__IO uint32_t*)USARTXBASE &= ~TXE_MASK
#define TXE_IT_DISABLE    *(__IO uint32_t*)USARTXBASE |= TXE_MASK

/* Private variables ---------------------------------------------------------*/
#ifndef USART_TX_ONLY
__IO uint8_t rx_buffer[RX_BUFFER_SIZE];
__IO uint8_t rx_wr_index=0, rx_rd_index=0, rx_counter=0;
uint8_t rx_buffer_overflow=0;
#endif // USART_TX_ONLY

__IO uint8_t tx_buffer[TX_BUFFER_SIZE];
__IO uint8_t tx_wr_index=0, tx_rd_index=0, tx_counter=0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function configure USART.
  * @param  None
  * @retval None
  */
void USART_Config(void) {
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

  /* GPIO configuration */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1); //PA2 to TX USART1
#ifndef USART_TX_ONLY
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1); //PA3 to RX USART1
#endif // USART_TX_ONLY

#ifndef USART_TX_ONLY
  GPIO_InitStructure.GPIO_Pin =  (GPIO_Pin_2 | GPIO_Pin_3);
#else
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
#endif // USART_TX_ONLY
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USART configuration */
  //RCC_USARTCLKConfig(RCC_USART1CLK_SYSCLK);
  RCC_APB2PeriphClockCmd(RCC_APB2ENR_USART1EN, ENABLE);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#ifndef USART_TX_ONLY
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
#else
  USART_InitStructure.USART_Mode = USART_Mode_Tx;
#endif // USART_TX_ONLY
  USART_Init(USART1, &USART_InitStructure);

  /* NVIC configuration */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable USART */
  USART_Cmd(USART1, ENABLE);
#ifndef USART_TX_ONLY
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif // USART_TX_ONLY
} /* End of USART_Config */

/**
  * @brief  This function handles USART1 IRQ Handler.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void) {
#ifndef USART_TX_ONLY
  /* RX interrupt */
  if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET) {
    if ((USART1->ISR & (USART_FLAG_NE|USART_FLAG_FE|USART_FLAG_PE|USART_FLAG_ORE)) == 0) {
      rx_buffer[rx_wr_index++] = (uint8_t)(USART1->RDR & 0xFF);
      if (rx_wr_index == RX_BUFFER_SIZE) {
        rx_wr_index = 0;
      }
      if (++rx_counter == RX_BUFFER_SIZE) {
        rx_counter = 0;
        /* хрень вроде как. тут -- это не переполнение буфера.
        переполенение будет когда rx_wr_index догонит и перегонит rx_rd_index */
        rx_buffer_overflow = 1;
      }
    } else {
      //в идеале пишем здесь обработчик ошибок, в данном случае просто пропускаем ошибочный байт.
      uint8_t tmp = (uint8_t)(USART1->RDR & 0xFF);
    }
  }

  /* прерывание по переполнению буфера */
  if (USART_GetITStatus(USART2, USART_IT_ORE) == SET) {
    //в идеале пишем здесь обработчик переполнения буфера, но мы просто сбрасываем этот флаг прерывания чтением из регистра данных.
    uint8_t tmp = (uint8_t)(USART1->RDR & 0xFF);
  }
#endif // USART_TX_ONLY

  /* TX interrupt */
  if (USART_GetITStatus(USART1, USART_IT_TXE) == SET) {
    if (tx_counter) {
      -- tx_counter;
      USART1->TDR = (tx_buffer[tx_rd_index++]);
      if (tx_rd_index == TX_BUFFER_SIZE) {
        tx_rd_index = 0;
      }
    } else {
      USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
    }
  }

} /* End of USART1_IRQHandler */

#ifndef USART_TX_ONLY
/**
  * @brief  This function return char retrived from USART.
  * @param  None
  * @retval Retrived char.
  */
uint8_t USART_Get_Char(void)
{
  uint8_t data;

  /* если буфер пуст -- ждём данные */
  while (rx_counter == 0);

  data = rx_buffer[rx_rd_index++];
  if (rx_rd_index == RX_BUFFER_SIZE) {
    rx_rd_index = 0;
  }

  USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
  -- rx_counter;
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

  return data;
}
#endif // USART_TX_ONLY

/**
  * @brief  This function put given char to USART.
  * @param  Char for putting to USART.
  * @retval None
  */
void USART_Put_Char(uint8_t c) {
  /* Если буфер полон -- ждём пока освободиться */
  while (tx_counter == TX_BUFFER_SIZE);

  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);

  /* если в буфере уже что-то есть или если в данный момент что-то уже передается */
  if (tx_counter || ((USART1->ISR & USART_FLAG_TXE) == (uint16_t)RESET)) {
    /* то кладем данные в буфер */
    tx_buffer[tx_wr_index++] = c;
    if (tx_wr_index == TX_BUFFER_SIZE) {
      tx_wr_index = 0;
    }
    ++ tx_counter;
    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
  } else {
    /* если UART свободен, передаем данные без прерывания */
    USART1->TDR = c;
  }
}

/**
  * @brief  This function put given string to USART.
  * @param  Pointer to string.
  * @retval None
  */
void USART_Put_Str(uint8_t *s)
{
  while (*s != 0) {
    USART_Put_Char(*s++);
  }
}

/**
  * @brief  This function convert given integer to string and put it to USART.
  * @param  Integer
  * @retval None
  */
void USART_Put_Int(int32_t data)
{
  uint8_t temp[10], count = 0;

  if (data < 0) {
    data = -data;
    USART_Put_Char('-');
  }

  if (data) {
    while (data) {
      temp[count++] = data % 10 + '0';
      data /= 10;
    }
    while (count) {
      USART_Put_Char(temp[--count]);
    }
  } else {
    USART_Put_Char('0');
  }
}

/*****************************END OF FILE****/
