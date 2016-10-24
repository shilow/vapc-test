/*
http://cxem.net/mc/mc362.php
�� ����������� ���������� I2C ��� ����������������� STM32F030
���������� �� ������������
SPL ������������ ������ ��� ������ �������������� �������
��� ������� ����� ��������� AF ������� � �������� SPL ������
--------------------------
����� Zlodey
masya-chel@mail.ru
masya-chel@yandex.ru
--------------------------
���� 2015�.
*/

#include "i2c.h"

/**
  * ��������� ��������� ��������� ����.
  * ����������� �����, �������������� �������.
  * ����������� �������� ��� ������ ����.
  */
void I2C_Initialization (void)
{
  /* Enable the peripheral clock of GPIOA */
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

  /* (1) open drain for I2C signals */
  /* (2) AF4 for I2C signals */
  /* (3) Select AF mode (10) on PA9 and PA10 */
  GPIOA->OTYPER |= GPIO_OTYPER_OT_9 | GPIO_OTYPER_OT_10; /* (1) */
  GPIOA->AFR[1] = (GPIOA->AFR[1] & ~(GPIO_AFRH_AFR9 | GPIO_AFRH_AFR10)) \
                  | (4 << ( 1 * 4 )) | (4 << (2 * 4)); /* (2) */
  GPIOA->MODER = (GPIOA->MODER & ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10)) \
                 | (GPIO_MODER_MODER9_1 | GPIO_MODER_MODER10_1); /* (3) */
  //GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR9 | GPIO_OSPEEDR_OSPEEDR10; // ������������ ��������

  /* Configure RCC for I2C1 */
  /* (1) Enable the peripheral clock I2C1 */
  /* (2) Use SysClk for I2C CLK */
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; /* (1) */
  RCC->CFGR3 |= RCC_CFGR3_I2C1SW; /* (2) */

  I2C_BUS->CR1 &= ~I2C_CR1_PE; // �������� I2C

  while (I2C_BUS->CR1 & I2C_CR1_PE) {}; // ��� ���� ����������

  // ���������� ��������
  I2C_BUS->TIMINGR = I2C_TIMINGS;

  I2C_BUS->CR1 |= I2C_CR1_PE; // ������� I2C

  while ((I2C_BUS->CR1 & I2C_CR1_PE)==0) {}; // ��� ���� ���������
}

/**
  * ��� ��������� �������, ������������ � �� �����.
  * ������������� ����������� ������ - ���� ��� ��������.
  * ����� ����� ������������ ������.
  * ����� ����� �������� ����������.
  * ����� ����� �� ����.
  * ���������:
  * Direction - ����������� (0-��������, 1-����)
  * Adress - ����� �������� ����������
  * Size - ������ ������ (�� 1 �� 255 ����)
  */
void I2C_Start_Direction_Adress_Size (I2C_Direction Direction, uint8_t Adress, uint8_t Size)
{
  //I2C_BUS->CR2 &= ~I2C_CR2_AUTOEND; // �������� ���� �������
  //I2C_BUS->CR2 &= ~I2C_CR2_RELOAD; // �� ������������ ����� ������������
  if (Direction) { // ����� �����
    I2C_BUS->CR2 |= I2C_CR2_RD_WRN;
  } else { // ����� ��������
    I2C_BUS->CR2 &= ~I2C_CR2_RD_WRN;
  }

  I2C_BUS->CR2 &= ~I2C_CR2_NBYTES; // �������� ������ ������
  I2C_BUS->CR2 |= Size<<I2C_OFFSET_CR2_NBYTES; // ���������� ������ ������
  I2C_BUS->CR2 &= ~I2C_CR2_SADD; // �������� ����� �������� ����������
  I2C_BUS->CR2 |= Adress; // ���������� ����� �������� ����������
  I2C_BUS->CR2 |= I2C_CR2_START; // ������ ����� �� ����

  while ((I2C_BUS->ISR & I2C_ISR_BUSY)==0) {}; // ������� ������ ������
}

/**
  * ��� ��������� �������, ������������ � �� �����.
  * ����� ���� �� ����.
  * ������� �����.
  * ��������� ������� ������, ������� ����� ������.
  */
void I2C_Stop (void)
{
  I2C_BUS->CR2 |= I2C_CR2_STOP; // ������ ���� �� ����

  while (I2C_BUS->ISR & I2C_ISR_BUSY) {}; // ������� ������ �����

  // ������ ����� - ���������� ��� ���������� ������ ����
  I2C_BUS->ICR |= I2C_ICR_STOPCF; // STOP ����

  I2C_BUS->ICR |= I2C_ICR_NACKCF; // NACK ����

  // ���� ���� ������ �� ���� - ������ �����
  if (I2C_BUS->ISR & (I2C_ISR_ARLO | I2C_ISR_BERR)) {
    I2C_BUS->ICR |= I2C_ICR_ARLOCF;
    I2C_BUS->ICR |= I2C_ICR_BERRCF;
  }
}

/**
  * ��������� ���������� ������ Size ���� � ������� Register �� ������ Adress.
  * ���������:
  * Adress - ����� �������� ����������
  * Register - �������, � ������� ����� �������� ������
  * Data - ��������� ������ ����� ������ ��� ��������
  * Size - ������� ���� ����� �������� (�� 1 �� 254)
  * ����������:
  * 1 - ���� ������ ������� ��������
  * 0 - ���� ��������� ������
  */
uint8_t I2C_Write_Transaction (uint8_t Adress, uint8_t Register, uint16_t Data)
{
  uint8_t cnt=0; // ������� ������� ���������� ����
  uint8_t dt[2];
  dt[0] = (uint8_t)(Data >> 8);
  dt[1] = (uint8_t)(Data & 0x00FF);

  // �����
  I2C_Start_Direction_Adress_Size (I2C_Transmitter, Adress, 3);

  // ������ ���� I2C �������� ������ ���� ��� ��������,
  // ���� ������� NACK-����, ��������� � ���, ��� ���������� �� ��������.
  // ���� ������� NACK-����, �������� ����������.
  while ((((I2C_BUS->ISR & I2C_ISR_TXIS)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {};

  if (I2C_BUS->ISR & I2C_ISR_TXIS) { // ��������� ����� ��������
    I2C_BUS->TXDR = Register;
  }

  // ���������� ����� �� ��� ���, ���� �� ������� TC-����.
  // ���� ������� NACK-����, �������� ����������.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_TXIS) { // ��������� ������
      if (cnt > 1) {
        return 2;
      }
      I2C_BUS->TXDR = dt[cnt];
      cnt ++;
    }
  }

  I2C_Stop();

  if (cnt == 2) { // ��������� ��� �����?
    return 0;
  } else {
    return 1;
  }
}

/**
  * ��������� ���������� ������ Size ���� �� �������� Register �� ������ Adress.
  * ���������:
  * Adress - ����� �������� ����������
  * Register - �������, �� �������� ����� ������� ������
  * Data - ��������� ���� ���������� �������� ������
  * Size - ������� ���� ����� ������� (�� 1 �� 255)
  * ����������:
  * 1 - ���� ������ ������� �������
  * 0 - ���� ��������� ������
  */
uint8_t I2C_Read_Transaction (uint8_t Adress, uint8_t Register, uint16_t * Data)
{
  uint8_t cnt=0; // ������� ������� �������� ����
  uint8_t dt[2];

  // �����
  I2C_Start_Direction_Adress_Size (I2C_Transmitter, Adress, 1);

  // ������ ���� I2C �������� ������ ���� ��� ��������,
  // ���� ������� NACK-����, ��������� � ���, ��� ���������� �� ��������.
  // ���� ������� NACK-����, �������� ����������.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_TXIS) { // ��������� ����� ��������
      I2C_BUS->TXDR = Register;
    }
  }

  // ��������� �����
  I2C_Start_Direction_Adress_Size (I2C_Receiver, Adress, 2);

  // ��������� ����� �� ��� ���, ���� �� ������� TC-����.
  // ���� ������� NACK-����, ���� ����������.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_RXNE) { // �������� ������
      dt[cnt] = I2C_BUS->RXDR;
      cnt ++;
    }
  }

  I2C_Stop();

  if (cnt == 2) { // ������� 2 �����?
    *Data = (uint16_t)((dt[0] << 8) | dt[1]);
    return 0;
  } else { // �� ����� ������� �������, �� ���� ����� �����
    return cnt;
  }
}
