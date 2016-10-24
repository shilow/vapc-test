/*
http://cxem.net/mc/mc362.php
Не блокирующая реализация I2C для микроконтроллеров STM32F030
Прерывания не используются
SPL используется только для выбора альтернативной функции
При желании можно настроить AF ручками и выкинуть SPL совсем
--------------------------
Автор Zlodey
masya-chel@mail.ru
masya-chel@yandex.ru
--------------------------
Март 2015г.
*/

#include "i2c.h"

/**
  * Выполняет первичную настройку шины.
  * Настраивает порты, альтернативные функции.
  * Настраивает тайминги для работы шины.
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
  //GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR9 | GPIO_OSPEEDR_OSPEEDR10; // Максимальная скорость

  /* Configure RCC for I2C1 */
  /* (1) Enable the peripheral clock I2C1 */
  /* (2) Use SysClk for I2C CLK */
  RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; /* (1) */
  RCC->CFGR3 |= RCC_CFGR3_I2C1SW; /* (2) */

  I2C_BUS->CR1 &= ~I2C_CR1_PE; // Отключаю I2C

  while (I2C_BUS->CR1 & I2C_CR1_PE) {}; // Жду пока отключится

  // Настраиваю тайминги
  I2C_BUS->TIMINGR = I2C_TIMINGS;

  I2C_BUS->CR1 |= I2C_CR1_PE; // Включаю I2C

  while ((I2C_BUS->CR1 & I2C_CR1_PE)==0) {}; // Жду пока включится
}

/**
  * Это служебная функция, использовать её не нужно.
  * Устанавливает направление данных - приём или передача.
  * Задаёт объём пересылаемых данных.
  * Задаёт адрес ведомого устройства.
  * Выдаёт старт на шину.
  * Параметры:
  * Direction - направление (0-отправка, 1-приём)
  * Adress - адрес ведомого устройства
  * Size - размер данных (от 1 до 255 байт)
  */
void I2C_Start_Direction_Adress_Size (I2C_Direction Direction, uint8_t Adress, uint8_t Size)
{
  //I2C_BUS->CR2 &= ~I2C_CR2_AUTOEND; // Выдавать стоп вручную
  //I2C_BUS->CR2 &= ~I2C_CR2_RELOAD; // Не использовать режим перезагрузки
  if (Direction) { // Режим приёма
    I2C_BUS->CR2 |= I2C_CR2_RD_WRN;
  } else { // Режим передачи
    I2C_BUS->CR2 &= ~I2C_CR2_RD_WRN;
  }

  I2C_BUS->CR2 &= ~I2C_CR2_NBYTES; // Очистить размер данных
  I2C_BUS->CR2 |= Size<<I2C_OFFSET_CR2_NBYTES; // Установить размер данных
  I2C_BUS->CR2 &= ~I2C_CR2_SADD; // Очистить адрес ведомого устройства
  I2C_BUS->CR2 |= Adress; // Установить адрес ведомого устройства
  I2C_BUS->CR2 |= I2C_CR2_START; // Выдать старт на шину

  while ((I2C_BUS->ISR & I2C_ISR_BUSY)==0) {}; // Ожидать выдачу старта
}

/**
  * Это служебная функция, использовать её не нужно.
  * Выдаёт стоп на шину.
  * Очищает флаги.
  * Проверяет наличие ошибок, очищает флаги ошибок.
  */
void I2C_Stop (void)
{
  I2C_BUS->CR2 |= I2C_CR2_STOP; // Выдать стоп на шину

  while (I2C_BUS->ISR & I2C_ISR_BUSY) {}; // Ожидать выдачу стопа

  // Очищаю флаги - необходимо для дальнейшей работы шины
  I2C_BUS->ICR |= I2C_ICR_STOPCF; // STOP флаг

  I2C_BUS->ICR |= I2C_ICR_NACKCF; // NACK флаг

  // Если есть ошибки на шине - очищаю флаги
  if (I2C_BUS->ISR & (I2C_ISR_ARLO | I2C_ISR_BERR)) {
    I2C_BUS->ICR |= I2C_ICR_ARLOCF;
    I2C_BUS->ICR |= I2C_ICR_BERRCF;
  }
}

/**
  * Выполняет транзакцию записи Size байт в регистр Register по адресу Adress.
  * Параметры:
  * Adress - адрес ведомого устройства
  * Register - регистр, в который хотим передать данные
  * Data - указывает откуда брать данные для передачи
  * Size - сколько байт хотим передать (от 1 до 254)
  * Возвращает:
  * 1 - если данные успешно переданы
  * 0 - если произошла ошибка
  */
uint8_t I2C_Write_Transaction (uint8_t Adress, uint8_t Register, uint16_t Data)
{
  uint8_t cnt=0; // Счётчик успешно переданных байт
  uint8_t dt[2];
  dt[0] = (uint8_t)(Data >> 8);
  dt[1] = (uint8_t)(Data & 0x00FF);

  // Старт
  I2C_Start_Direction_Adress_Size (I2C_Transmitter, Adress, 3);

  // Сейчас либо I2C запросит первый байт для отправки,
  // Либо взлетит NACK-флаг, говорящий о том, что микросхема не отвечает.
  // Если взлетит NACK-флаг, отправку прекращаем.
  while ((((I2C_BUS->ISR & I2C_ISR_TXIS)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {};

  if (I2C_BUS->ISR & I2C_ISR_TXIS) { // Отправляю адрес регистра
    I2C_BUS->TXDR = Register;
  }

  // Отправляем байты до тех пор, пока не взлетит TC-флаг.
  // Если взлетит NACK-флаг, отправку прекращаем.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_TXIS) { // Отправляю данные
      if (cnt > 1) {
        return 2;
      }
      I2C_BUS->TXDR = dt[cnt];
      cnt ++;
    }
  }

  I2C_Stop();

  if (cnt == 2) { // отправили два байта?
    return 0;
  } else {
    return 1;
  }
}

/**
  * Выполняет транзакцию чтения Size байт из регистра Register по адресу Adress.
  * Параметры:
  * Adress - адрес ведомого устройства
  * Register - регистр, из которого хотим принять данные
  * Data - указывает куда складывать принятые данные
  * Size - сколько байт хотим принять (от 1 до 255)
  * Возвращает:
  * 1 - если данные успешно приняты
  * 0 - если произошла ошибка
  */
uint8_t I2C_Read_Transaction (uint8_t Adress, uint8_t Register, uint16_t * Data)
{
  uint8_t cnt=0; // Счётчик успешно принятых байт
  uint8_t dt[2];

  // Старт
  I2C_Start_Direction_Adress_Size (I2C_Transmitter, Adress, 1);

  // Сейчас либо I2C запросит первый байт для отправки,
  // Либо взлетит NACK-флаг, говорящий о том, что микросхема не отвечает.
  // Если взлетит NACK-флаг, отправку прекращаем.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_TXIS) { // Отправляю адрес регистра
      I2C_BUS->TXDR = Register;
    }
  }

  // Повторный старт
  I2C_Start_Direction_Adress_Size (I2C_Receiver, Adress, 2);

  // Принимаем байты до тех пор, пока не взлетит TC-флаг.
  // Если взлетит NACK-флаг, приём прекращаем.
  while ((((I2C_BUS->ISR & I2C_ISR_TC)==0) && ((I2C_BUS->ISR & I2C_ISR_NACKF)==0)) && (I2C_BUS->ISR & I2C_ISR_BUSY)) {
    if (I2C_BUS->ISR & I2C_ISR_RXNE) { // Принимаю данные
      dt[cnt] = I2C_BUS->RXDR;
      cnt ++;
    }
  }

  I2C_Stop();

  if (cnt == 2) { // приняли 2 байта?
    *Data = (uint16_t)((dt[0] << 8) | dt[1]);
    return 0;
  } else { // не самый удачный вариант, но пока пусть будет
    return cnt;
  }
}
