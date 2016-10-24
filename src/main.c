/**
  ******************************************************************************
  * @file    Project/STM32F0xx_StdPeriph_Templates/main.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    05-December-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "delay.h"
#include "lcd1202_spi.h"
#include "usart.h"
#include "i2c.h"
#include "ina219.h"
#include "printf.h"

/** @addtogroup STM32F0xx_StdPeriph_Templates
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
typedef enum {
  modeStop      = 0x00,
  modeStopChrg  = 0x02, // stop after charge
  modeCharge    = 0x03,
  modeStopDech  = 0x04, // stop agter decharge
  modeDecharge  = 0x05
} chnl_mode_t;

typedef struct {
  chnl_mode_t  Mode; // current mode
  chnl_mode_t  prMode; // previous mode

  uint16_t  Volt;
  uint16_t  Curr;
  uint32_t  Power;

  uint32_t  chrgTime;
  uint32_t  dechTime;

  uint32_t  chrgCapI;
  uint32_t  chrgCapP;

  uint32_t  dechCapI;
  uint32_t  dechCapP;
} channel_t;

/* Private define ------------------------------------------------------------*/
#define AVG_BUFF_SIZE 10
#define LOAD_DIS_VOLT 2500
#define LOAD_EN_VOLT  3000
#define NOISE_LEVEL   12

/* Private macro -------------------------------------------------------------*/
#define LCD_LED_PORT  GPIOB
#define LCD_LED_PIN   GPIO_Pin_1
#define LCD_LED_ON    LCD_LED_PORT->BSRR = LCD_LED_PIN
#define LCD_LED_OFF   LCD_LED_PORT->BRR = LCD_LED_PIN

#define LOAD_DIS_PORT GPIOA
#define LOAD_DIS_PIN  GPIO_Pin_1
#define LOAD_DISABLE  LOAD_DIS_PORT->BRR = LOAD_DIS_PIN
#define LOAD_ENABLE   LOAD_DIS_PORT->BSRR = LOAD_DIS_PIN

/* Private variables ---------------------------------------------------------*/
__IO channel_t Channel;
__IO _flag_t flag;

uint16_t bufVolt[AVG_BUFF_SIZE];
int16_t bufCurr[AVG_BUFF_SIZE];
uint32_t bufPwr[AVG_BUFF_SIZE];
uint8_t bufIdx = 0;

extern uint8_t TxBuffer[];
extern uint8_t RxBufferp[];
extern uint8_t NbrOfDataToTransfer;
extern uint8_t NbrOfDataToRead;
extern __IO uint8_t TxCount;
extern __IO uint16_t RxCount;

/* Private function prototypes -----------------------------------------------*/
static void GPIO_Config(void);
static void outputInfo(void);

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f0xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f0xx.c file
   */

  /* RCC configuration */
  RCC_ClocksTypeDef RCC_ClockFreq;

  /* This function fills the RCC_ClockFreq structure with the current
     frequencies of different on chip clocks (for debug purpose) */
  RCC_GetClocksFreq(&RCC_ClockFreq);

#ifdef USE_HSE_PLL
  GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable Clock Security System(CSS): this will generate an NMI exception
  when HSE clock fails */
  RCC_ClockSecuritySystemCmd(ENABLE);

  /* Enable and configure RCC global IRQ channel, will be used to manage HSE ready
     and PLL ready interrupts.
     These interrupts are enabled in stm32f0xx_it.c file */
  NVIC_InitStructure.NVIC_IRQChannel = RCC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
#endif // USE_HSE_PLL

  SystemCoreClockUpdate();

  /* Variable init */
  flag.LoadDisabled = 0;
  flag.TickSek = 0;
  flag.TickSsek = 0;

  /* Delay configuration */
  Delay_Config();

  /* GPIO configuration */
  GPIO_Config();

  /* LCD 1202 configuration */
  LCD_Config();
  LCD_LED_ON;
  LOAD_ENABLE;

  /* USART configuration */
  USART_Config();

  USART_Put_Str((uint8_t *)"\n\rINA219 based Li-Ion simple controller.\n\r");

  /* I2C configuration */
  I2C_Initialization();
  /* INA219 configuration */
  INA219_InitTypeDef ina;
  INA219_StructInit(&ina);
  ina.INA219_BVR = INA219_RANGE_16V;
  ina.INA219_PG  = INA219_GAIN_80MV;
  ina.INA219_BADC = INA219_SHUNT_RES_12BIT_128S;
  ina.INA219_SADC = INA219_SHUNT_RES_12BIT_128S;
  INA219_Config(&ina);

  INA219_Calibrate(20, 2000);

  /* Infinite loop */
  while (1) {
    /* 0.1 sek period */
    if (flag.TickSsek == 1) {
      // тут заполняем буфера напр., тока и мощности
      // проверяем режимы и меняем флаги.
      flag.TickSsek = 0;

      int16_t shuntVoltVal;
      uint8_t cnt;
      uint32_t tmpU=0, tmpP=0;
      int32_t tmpI=0;

      bufVolt[bufIdx] = readBusVoltage();
      if (bufVolt[bufIdx] <= LOAD_DIS_VOLT && flag.LoadDisabled == 0) {
        LOAD_DISABLE;
        flag.LoadDisabled = 1;
      } else if (flag.LoadDisabled == 1 && bufVolt[bufIdx] > LOAD_EN_VOLT) {
        LOAD_ENABLE;
        flag.LoadDisabled = 0;
      }
      bufCurr[bufIdx] = readBusCurrent();
      bufPwr[bufIdx] = readBusPower();

      shuntVoltVal = readShuntVoltage();
      if (shuntVoltVal > NOISE_LEVEL) {
        /* charge mode */
        Channel.prMode = Channel.Mode;
        Channel.Mode = modeCharge;
      } else if (shuntVoltVal < 0) {
        /* decharge mode */
        Channel.prMode = Channel.Mode;
        Channel.Mode = modeDecharge;
      } else {
        /* stop mode */
        if (Channel.Mode == modeCharge) {
          Channel.Mode = modeStopChrg;
          Channel.prMode = modeCharge;
        } else if (Channel.Mode == modeDecharge) {
          Channel.Mode = modeStopDech;
          Channel.prMode = modeDecharge;
        } else {
          //Channel.prMode = Channel.Mode;
          //Channel.Mode = modeStop;
        }
        bufCurr[bufIdx] = 0;
        bufPwr[bufIdx] = 0;
      }

      for (cnt=0; cnt<AVG_BUFF_SIZE; cnt++) {
          tmpU += bufVolt[cnt];
          tmpP += bufPwr[cnt];
          tmpI += bufCurr[cnt];
      }
      Channel.Volt = (uint16_t)((tmpU + (AVG_BUFF_SIZE>>1)) / AVG_BUFF_SIZE);
      Channel.Curr = (uint16_t)((tmpI + (AVG_BUFF_SIZE>>1)) / AVG_BUFF_SIZE);
      Channel.Power = (tmpP + (AVG_BUFF_SIZE>>1)) / AVG_BUFF_SIZE;

      bufIdx ++;
      if (bufIdx == AVG_BUFF_SIZE) {
        bufIdx = 0;
      }
    }

    /* 1 sek period */
    if (flag.TickSek == 1) {
      flag.TickSek = 0;
      // тут считаем время, ампер* и ватт*часы
      if (Channel.Mode == modeCharge) {
        Channel.chrgTime ++;
        Channel.chrgCapI += Channel.Curr;
        Channel.chrgCapP += Channel.Power;
      } else if (Channel.Mode == modeDecharge) {
        Channel.dechTime ++;
        Channel.dechCapI += Channel.Curr;
        Channel.dechCapP += Channel.Power;
      } else if (Channel.Mode == modeStop) {
        //Channel.chrgTime = 0;
        //Channel.dechTime = 0;
      }
      // вывод в ком-порт
    }

    outputInfo();

    /* Request to enter WFI mode */
    __WFI();
  }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function enables the peripheral clocks on GPIO port A and port B.
  * @param  None
  * @retval None
  */
__INLINE void GPIO_Config(void) {
  /*
   (1) Enable the peripheral clock of GPIOB
   (1a) Enable the peripheral clock of GPIOA
   (2) Select output mode (01) on GPIOB pin 1 (LCD Led control)
   (2a) Select output mode (01) on GPIOA pin 1 (LOAD control)
   */

  RCC->AHBENR |= RCC_AHBENR_GPIOBEN; /* (1) */
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN; /* (1a) */

  GPIOB->MODER = (GPIOB->MODER & ~(GPIO_MODER_MODER1)) | (GPIO_MODER_MODER1_0); /* (2) */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER1)) | (GPIO_MODER_MODER1_0); /* (2a) */
}

/**
  * output info to lcd
  */
static void outputInfo(void) {
  static uint16_t oldU, oldI;
  static uint32_t oldTime;
  static uint32_t oldP, oldCapIH, oldCapPH;
  uint8_t hh, mm, ss;
  uint16_t intgr, fract;
  uint16_t tmp16;

  LCD_GotoXY(0, 0);
  if (Channel.Mode == modeCharge || Channel.Mode == modeStopChrg) {
    if (oldTime != Channel.chrgTime) {
      oldTime = Channel.chrgTime;
      hh = Channel.chrgTime / 3600;
      mm = (Channel.chrgTime - (hh * 3600)) / 60;
      ss = Channel.chrgTime - (hh * 3600) - (mm * 60);
      lprintf((uint8_t *)"Charge    %02d:%02d:%02d    ", hh, mm, ss);
    }
    if (oldCapIH != Channel.chrgCapI) {
      oldCapIH = Channel.chrgCapI;
      tmp16 = (oldCapIH + 1800) / 3600;
      intgr = tmp16 / 1000;
      fract = tmp16 % 1000;
      LCD_GotoXY(0, 4);
      lprintf((uint8_t *)"CI: %3d.%03d A*H    ", intgr, fract);
    }
    if (oldCapPH != Channel.chrgCapP) {
      oldCapPH = Channel.chrgCapP;
      tmp16 = (oldCapPH + 1800) / 3600;
      intgr = tmp16 / 1000;
      fract = tmp16 % 1000;
      LCD_GotoXY(0, 5);
      lprintf((uint8_t *)"CP: %3d.%03d W*H    ", intgr, fract);
    }
  } else if (Channel.Mode == modeDecharge || Channel.Mode == modeStopDech) {
    if (oldTime != Channel.dechTime) {
      oldTime = Channel.dechTime;
      hh = Channel.dechTime / 3600;
      mm = (Channel.dechTime - (hh * 3600)) / 60;
      ss = Channel.dechTime - (hh * 3600) - (mm * 60);
      lprintf((uint8_t *)"Decharge  %02d:%02d:%02d    ", hh, mm, ss);
    }
    if (oldCapIH != Channel.dechCapI) {
      oldCapIH = Channel.dechCapI;
      tmp16 = (oldCapIH + 1800) / 3600;
      intgr = tmp16 / 1000;
      fract = tmp16 % 1000;
      LCD_GotoXY(0, 4);
      lprintf((uint8_t *)"CI: %3d.%03d A*H    ", intgr, fract);
    }
    if (oldCapPH != Channel.dechCapP) {
      oldCapPH = Channel.dechCapP;
      tmp16 = (oldCapPH + 1800) / 3600;
      intgr = tmp16 / 1000;
      fract = tmp16 % 1000;
      LCD_GotoXY(0, 5);
      lprintf((uint8_t *)"CP: %3d.%03d W*H    ", intgr, fract);
    }
  } else {
    lprintf((uint8_t *)"Bypass    --:--:--    ");
    LCD_GotoXY(0, 4);
    lprintf((uint8_t *)"CI: ---.--- A*H    ");
    LCD_GotoXY(0, 5);
    lprintf((uint8_t *)"CP: ---.--- W*H    ");
  }

  if (oldU != Channel.Volt) {
    oldU = Channel.Volt;
    intgr = oldU / 1000;
    fract = (oldU % 1000) / 10;
    LCD_GotoXY(0, 1);
    lprintf((uint8_t *)"U: %2d.%02d V    ", intgr, fract);
  }

  if (oldI != Channel.Curr) {
    oldI = Channel.Curr;
    intgr = oldI / 1000;
    fract = oldI % 1000;
    LCD_GotoXY(0, 2);
    lprintf((uint8_t *)"I: %1d.%03d A    ", intgr, fract);
  }

  if (oldP != Channel.Power) {
    oldP = Channel.Power;
    intgr = oldP / 1000;
    fract = oldP % 1000;
    LCD_GotoXY(0, 3);
    lprintf((uint8_t *)"P: %2d.%03d W    ", intgr, fract);
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
