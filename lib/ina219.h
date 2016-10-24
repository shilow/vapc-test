/*
INA219.h - Header file for the Zero-Drift, Bi-directional Current/Power Monitor Arduino Library.

Version: 1.0.0
(c) 2014 Korneliusz Jarzebski
www.jarzebski.pl

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INA219_h
#define INA219_h

#include "stm32f0xx.h"

#define INA219_ADDRESS              (uint8_t)(0x40<<1)
#define INA219_ADDR_RD              (INA219_ADDRESS | 0x01)

/* INA219 Registers */
#define INA219_REG_CONFIG           0x00
#define INA219_REG_SHUNTVOLTAGE     0x01
#define INA219_REG_BUSVOLTAGE       0x02
#define INA219_REG_POWER            0x03
#define INA219_REG_CURRENT          0x04
#define INA219_REG_CALIBRATION      0x05

typedef enum
{
  INA219_RESET_OFF            = 0x0000,
  INA219_RESET_ON             = 0x8000
} ina219_reset_t;

typedef enum
{
  INA219_RANGE_16V            = 0x0000,
  INA219_RANGE_32V            = 0x2000
} ina219_bvr_t;

typedef enum
{
  INA219_GAIN_40MV            = 0x0000,
  INA219_GAIN_80MV            = 0x0800,
  INA219_GAIN_160MV           = 0x1000,
  INA219_GAIN_320MV           = 0x1800
} ina219_pg_t;

typedef enum
{
  INA219_BUS_RES_9BIT         = 0x0000,
  INA219_BUS_RES_10BIT        = 0x0080,
  INA219_BUS_RES_11BIT        = 0x0100,
  INA219_BUS_RES_12BIT        = 0x0180
} ina219_badc_t;

typedef enum
{
  INA219_SHUNT_RES_9BIT       = 0x0000,
  INA219_SHUNT_RES_10BIT      = 0x0008,
  INA219_SHUNT_RES_11BIT      = 0x0010,
  INA219_SHUNT_RES_12BIT      = 0x0018,
  INA219_SHUNT_RES_12BIT_1S   = 0x0040,
  INA219_SHUNT_RES_12BIT_2S   = 0x0048,
  INA219_SHUNT_RES_12BIT_4S   = 0x0050,
  INA219_SHUNT_RES_12BIT_8S   = 0x0058,
  INA219_SHUNT_RES_12BIT_16S  = 0x0060,
  INA219_SHUNT_RES_12BIT_32S  = 0x0068,
  INA219_SHUNT_RES_12BIT_64S  = 0x0070,
  INA219_SHUNT_RES_12BIT_128S = 0x0078
} ina219_sadc_t;

typedef enum
{
  INA219_MODE_POWER_DOWN      = 0x0000,
  INA219_MODE_SHUNT_TRIG      = 0x0001,
  INA219_MODE_BUS_TRIG        = 0x0002,
  INA219_MODE_SHUNT_BUS_TRIG  = 0x0003,
  INA219_MODE_ADC_OFF         = 0x0004,
  INA219_MODE_SHUNT_CONT      = 0x0005,
  INA219_MODE_BUS_CONT        = 0x0006,
  INA219_MODE_SHUNT_BUS_CONT  = 0x0007
} ina219_mode_t;

/**
  * @brief  INA219 Configuration register definition
  */
typedef struct {
  uint8_t         INA219_Addr; /* I2C address */
  ina219_reset_t  INA219_RST;  /* Reset Bit */
  ina219_bvr_t    INA219_BVR;  /* Bus Voltage Range */
  ina219_pg_t     INA219_PG;   /* PGA (Shunt Voltage Only) */
  ina219_badc_t   INA219_BADC; /* Bus ADC Resolution/Averaging */
  ina219_sadc_t   INA219_SADC; /* Shunt ADC Resolution/Averaging */
  ina219_mode_t   INA219_MODE; /* Operating Mode */
} INA219_InitTypeDef;

void INA219_Config(INA219_InitTypeDef * INA219_InitStruct);
void INA219_StructInit(INA219_InitTypeDef * INA219_InitStruct);

void INA219_Calibrate(uint16_t rShuntValue, uint16_t iMaxExcepted);

ina219_bvr_t getRange(void);
ina219_pg_t getGain(void);
ina219_badc_t getBusRes(void);
ina219_sadc_t getShuntRes(void);
ina219_mode_t getMode(void);

int16_t readShuntVoltage(void);
uint16_t readBusVoltage(void);
uint16_t readBusCurrent(void);
uint32_t readBusPower(void);

uint16_t getMaxPossibleCurrent(void);
uint16_t getMaxCurrent(void);
uint16_t getMaxShuntVoltage(void);
uint32_t getMaxPower(void);

#endif
