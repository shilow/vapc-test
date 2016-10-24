/**
  * Nokia 1202 LCD Driver with hardware 9bit SPI
  * from http://randoman.ru/2013/09/21/nokia-1202-spi-stm32f0/
  */

#include <stm32f0xx.h>

#ifndef __LCD1202_SPI_H__
#define __LCD1202_SPI_H__

#define HW_SPI

//Configure SPI
#define LCD_SPI             SPI1
#define LCD_SPI_GPIO        GPIOA
#define LCD_SPI_GPIO_RCC    RCC_AHBPeriph_GPIOA
#define LCD_SPI_RCC         RCC_APB2Periph_SPI1
#define LCD_SCK_PIN         GPIO_Pin_5
#define LCD_SDA_PIN         GPIO_Pin_7
#define LCD_SCK_PIN_SOURCE  GPIO_PinSource5
#define LCD_SDA_PIN_SOURCE  GPIO_PinSource7

/* Pins and GPIO used for RESET and CS */
#define LCD_GPIO      GPIOA
#define LCD_GPIO_RCC  RCC_AHBPeriph_GPIOA

#define LCD_CS_PIN    GPIO_Pin_4
#define LCD_RES_PIN   GPIO_Pin_6

#define LCD_CS_LOW    LCD_GPIO->BRR = LCD_CS_PIN
#define LCD_CS_HIGH   LCD_GPIO->BSRR = LCD_CS_PIN

#define LCD_RES_LOW   LCD_GPIO->BRR = LCD_RES_PIN
#define LCD_RES_HIGH  LCD_GPIO->BSRR = LCD_RES_PIN

enum {
    LCD_COMMAND = 0,
    LCD_DATA = 0xFF
};
enum {
    LCD_STACK_DEPTH = 8
};
enum {
    LCD_SCREEN_WIDTH = 96
};
enum {
    LCD_SCREEN_HEIGHT = 8
};

void LCD_Config(void);
void LCD_Write(uint8_t cd, const uint8_t * data, uint16_t len, uint16_t repeat);
void LCD_Clear(void);
void LCD_GotoXY(uint8_t x, uint8_t y);
void LCD_Print_Char(uint8_t c);
void LCD_Print_String(const uint8_t *s);
void LCD_Print_Int(int32_t value);

extern uint8_t LCD_CurrentX;
extern uint8_t LCD_CurrentY;
extern const uint8_t LCD_ZERO;
extern const uint8_t LCD_FILL;

#endif //__LCD1202_SPI_H__
