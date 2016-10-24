/**
  ******************************************************************************
  * @file    Project/STM32F0xx_StdPeriph_Templates/stm32f0xx_it.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    05-December-2014
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
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
#include "stm32f0xx_it.h"

/** @addtogroup Li-Ion Ctrl
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
  /* This interrupt is generated when HSE clock fails */
  if (RCC_GetITStatus(RCC_IT_CSS) != RESET)
  {
    /* At this stage: HSE, PLL are disabled (but no change on PLL config) and HSI
       is selected as system clock source */

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Enable HSE Ready and PLL Ready interrupts */
    RCC_ITConfig(RCC_IT_HSERDY | RCC_IT_PLLRDY, ENABLE);

    /* Clear Clock Security System interrupt pending bit */
    RCC_ClearITPendingBit(RCC_IT_CSS);

    /* Once HSE clock recover, the HSERDY interrupt is generated and in the RCC ISR
       routine the system clock will be reconfigured to its previous state (before
       HSE clock failure) */
  }
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
void SysTick_Handler(void)
{
}
  */

/**
  * @brief  This function handles WWDG.
  * @param  None
  * @retval None
  */
void WWDG_IRQHandler(void)
{
}

/**
  * @brief  This function handles RTC.
  * @param  None
  * @retval None
  */
void RTC_IRQHandler(void)
{
}

/**
  * @brief  This function handles FLASH.
  * @param  None
  * @retval None
  */
void FLASH_IRQHandler(void)
{
}

/**
  * @brief  This function handles RCC interrupt request.
  * @param  None
  * @retval None
  */
void RCC_IRQHandler(void)
{
#ifdef USE_HSE_PLL
  if(RCC_GetITStatus(RCC_IT_HSERDY) != RESET)
  {
    /* Clear HSERDY interrupt pending bit */
    RCC_ClearITPendingBit(RCC_IT_HSERDY);

    /* Check if the HSE clock is still available */
    if (RCC_GetFlagStatus(RCC_FLAG_HSERDY) != RESET)
    {
      /* Enable PLL: once the PLL is ready the PLLRDY interrupt is generated */
      RCC_PLLCmd(ENABLE);
    }
  }

  if(RCC_GetITStatus(RCC_IT_PLLRDY) != RESET)
  {
    /* Clear PLLRDY interrupt pending bit */
    RCC_ClearITPendingBit(RCC_IT_PLLRDY);

    /* Check if the PLL is still locked */
    if (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) != RESET)
    {
      /* Select PLL as system clock source */
      RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    }
  }
#endif // USE_HSE_PLL
}

/**
  * @brief  This function handles EXTI0_1.
  * @param  None
  * @retval None
  */
void EXTI0_1_IRQHandler(void)
{
}

/**
  * @brief  This function handles EXTI2_3.
  * @param  None
  * @retval None
  */
void EXTI2_3_IRQHandler(void)
{
}

/**
  * @brief  This function handles EXTI4_15.
  * @param  None
  * @retval None
  */
void EXTI4_15_IRQHandler(void)
{
}

/**
  * @brief  This function handles DMA1_Channel1.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
}

/**
  * @brief  This function handles DMA1_Channel2_3.
  * @param  None
  * @retval None
  */
void DMA1_Channel2_3_IRQHandler(void)
{
}

/**
  * @brief  This function handles DMA1_Channel4_5.
  * @param  None
  * @retval None
  */
void DMA1_Channel4_5_IRQHandler(void)
{
}

/**
  * @brief  This function handles ADC1.
  * @param  None
  * @retval None
  */
void ADC1_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM1_BRK_UP_TRG_COM.
  * @param  None
  * @retval None
  */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM1_CC.
  * @param  None
  * @retval None
  */
void TIM1_CC_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM3.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM14.
  * @param  None
  * @retval None
  */
void TIM14_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM16.
  * @param  None
  * @retval None
  */
void TIM16_IRQHandler(void)
{
}

/**
  * @brief  This function handles TIM17.
  * @param  None
  * @retval None
  */
void TIM17_IRQHandler(void)
{
}

/**
  * @brief  This function handles I2C1.
  * @param  None
  * @retval None
  */
void I2C1_IRQHandler(void)
{
}

/**
  * @brief  This function handles SPI1.
  * @param  None
  * @retval None
  */
void SPI1_IRQHandler(void)
{
}

/**
  * @brief  This function handles USART1.
  * @param  None
  * @retval None
void USART1_IRQHandler(void)
{
}
  */

/******************************************************************************/
/*                 STM32F0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f0xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
