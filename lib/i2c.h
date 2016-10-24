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

#ifndef I2C_H	// �������� ��������� ��������� ����� ������
#define I2C_H

#include "stm32f0xx.h"

#define I2C_BUS I2C1


// ��������������� ����������
typedef enum _I2C_Direction {
  I2C_Transmitter=0,
  I2C_Receiver=1
} I2C_Direction;

/**
  * I2C timings:
  * 48MHz, 400kHz, 100/100 ns == 0x0090194B
  * 48MHz, 1000kHz, 20/20 ns  == 0x0030081B
  */
#define I2C_TIMINGS   0x0030081B

#define I2C_OFFSET_CR2_NBYTES   16

////////////////////////////////////////////////
//    ������ ���������� ��������� �������     //
////////////////////////////////////////////////
void I2C_Initialization(void);
void I2C_Start(void);
void I2C_Stop(void);
uint8_t I2C_Write_Transaction (uint8_t Adress, uint8_t Register, uint16_t Data);
uint8_t I2C_Read_Transaction (uint8_t Adress, uint8_t Register, uint16_t * Data);

#endif
