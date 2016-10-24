/*
INA219.cpp - Class file for the INA219 Zero-Drift, Bi-directional Current/Power Monitor Arduino Library.

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

#include "INA219.h"
#include "i2c.h"

static uint8_t inaAddress;
static uint16_t currentLSB, powerLSB;
static uint16_t vShuntMax;
static uint16_t vBusMax;
static uint16_t rShunt;

/**
  * Configure I2C, INA219
  */
void INA219_Config(INA219_InitTypeDef * INA219_InitStruct) {
  uint16_t config;

  /** INA219 configure */
  config  = INA219_InitStruct->INA219_RST;
  config |= INA219_InitStruct->INA219_BVR;
  config |= INA219_InitStruct->INA219_PG;
  config |= INA219_InitStruct->INA219_BADC;
  config |= INA219_InitStruct->INA219_SADC;
  config |= INA219_InitStruct->INA219_MODE;

  inaAddress = INA219_InitStruct->INA219_Addr;

  switch(INA219_InitStruct->INA219_BVR) {
  case INA219_RANGE_32V:
    vBusMax = 32000;
    break;
  case INA219_RANGE_16V:
    vBusMax = 16000;
    break;
  }

  switch(INA219_InitStruct->INA219_PG) {
  case INA219_GAIN_320MV:
    vShuntMax = 320;
    break;
  case INA219_GAIN_160MV:
    vShuntMax = 160;
    break;
  case INA219_GAIN_80MV:
    vShuntMax = 80;
    break;
  case INA219_GAIN_40MV:
    vShuntMax = 40;
    break;
  }

  I2C_Write_Transaction(inaAddress, INA219_REG_CONFIG, config);
}

/**
  * @brief  Fills each INA219_InitStruct member with its default value.
  * @param  INA219_InitStruct: pointer to a INA219_InitTypeDef structure which will
  *         be initialized.
  * @retval None
  */
void INA219_StructInit(INA219_InitTypeDef * INA219_InitStruct) {
  /* Reset INA219 init structure parameters values */
  INA219_InitStruct->INA219_Addr = INA219_ADDRESS;
  INA219_InitStruct->INA219_RST  = INA219_RESET_OFF;
  INA219_InitStruct->INA219_BVR  = INA219_RANGE_32V;
  INA219_InitStruct->INA219_PG   = INA219_GAIN_320MV;
  INA219_InitStruct->INA219_BADC = INA219_BUS_RES_12BIT;
  INA219_InitStruct->INA219_SADC = INA219_SHUNT_RES_12BIT;
  INA219_InitStruct->INA219_MODE = INA219_MODE_SHUNT_BUS_CONT;
}

/**
  * Calculate calibration values.
  * rShuntValue in miliohms, iMaxExpected in miliampers.
  */
void INA219_Calibrate(uint16_t rShuntValue, uint16_t iMaxExpected) {
  uint16_t calibrationValue;
  uint64_t tmp;

  rShunt = rShuntValue; /* in milli ohms */

  tmp = iMaxExpected * 1000UL;
  currentLSB = (uint16_t)((tmp + 16384) / 32768); /* uA */
  powerLSB = (uint16_t)(((tmp * 20) + 16384) / 32768); /* uW */

  tmp = ((tmp * rShuntValue * 1000UL) + 16384) / 32768UL;
  calibrationValue = (uint16_t)((40960000000UL + (tmp >> 1)) / tmp);

  I2C_Write_Transaction(inaAddress, INA219_REG_CALIBRATION, calibrationValue);
}

/** */
uint16_t getMaxPossibleCurrent(void) {
  return (((1000 * vShuntMax) + (rShunt>>1)) / rShunt);
}

/** */
uint16_t getMaxCurrent(void) {
  uint16_t maxCurrent = ((currentLSB * 32767) + 500) / 1000;
  uint16_t maxPossible = getMaxPossibleCurrent();

  if (maxCurrent > maxPossible) {
    return maxPossible;
  } else {
    return maxCurrent;
  }
}

/** */
uint16_t getMaxShuntVoltage(void) {
  uint16_t maxVoltage = ((getMaxCurrent() * rShunt) + 500) / 1000;

  if (maxVoltage >= vShuntMax) {
    return vShuntMax;
  } else {
    return maxVoltage;
  }
}

/** */
uint32_t getMaxPower(void) {
  return (((getMaxCurrent() * vBusMax) + 500000) / 1000000);
}

/** */
uint16_t readBusCurrent(void) {
  uint16_t current;
  int16_t tmp;

  I2C_Read_Transaction(inaAddress, INA219_REG_CURRENT, (uint16_t *)&tmp);
  if (tmp < 0) {
    tmp = - tmp;
  }
  current = (uint16_t)(((tmp * currentLSB) + 500) / 1000);

  return current;
}

/** */
uint32_t readBusPower(void) {
  uint16_t power;
  I2C_Read_Transaction(inaAddress, INA219_REG_POWER, &power);
  return (((power * powerLSB) + 500) / 1000);
}

/**
  * Currently return raw value of shunt voltage in 10 uV
  */
int16_t readShuntVoltage(void) {
  uint16_t shvolt;
  I2C_Read_Transaction(inaAddress, INA219_REG_SHUNTVOLTAGE, &shvolt);
  return shvolt;
}

/**
  * Return bus voltage in mV
  */
uint16_t readBusVoltage(void) {
  uint16_t volt;
  I2C_Read_Transaction(inaAddress, INA219_REG_BUSVOLTAGE, &volt);
  return ((volt >> 3) * 4);
}

ina219_bvr_t getRange(void) {
  uint16_t value;

  I2C_Read_Transaction(inaAddress, INA219_REG_CONFIG, &value);
  value &= 0x2000;
  value >>= 13;

  return (ina219_bvr_t)value;
}

ina219_pg_t getGain(void) {
  uint16_t value;

  I2C_Read_Transaction(inaAddress, INA219_REG_CONFIG, &value);
  value &= 0x1800;
  value >>= 11;

  return (ina219_pg_t)value;
}

ina219_badc_t getBusRes(void) {
  uint16_t value;

  I2C_Read_Transaction(inaAddress, INA219_REG_CONFIG, &value);
  value &= 0x0780;
  value >>= 7;

  return (ina219_badc_t)value;
}

ina219_sadc_t getShuntRes(void) {
  uint16_t value;

  I2C_Read_Transaction(inaAddress, INA219_REG_CONFIG, &value);
  value &= 0x0078;
  value >>= 3;

  return (ina219_sadc_t)value;
}

ina219_mode_t getMode(void) {
  uint16_t value;

  I2C_Read_Transaction(inaAddress, INA219_REG_CONFIG, &value);
  value &= 0x0007;

  return (ina219_mode_t)value;
}
