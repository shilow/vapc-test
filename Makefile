######################################
# STM32F0xx Makefile
######################################

######################################
# target
######################################
TARGET = svapcm

######################################
# programmator
######################################
PROGRAMMATOR = "C:/ARM/STMicroelectronics/STM32 ST-LINK Utility/ST-LINK Utility/ST-LINK_CLI.exe"

######################################
# building variables
######################################
# debug build?
DEBUG = 0
# build for debug in ram?
RAMBUILD = 0

#######################################
# paths
#######################################
# source path
VPATH := src MCU lib
# firmware library path
PERIPHLIBPATH = MCU
VPATH += $(PERIPHLIBPATH)/CMSIS/Device/ST/STM32F0xx/Source
VPATH += $(PERIPHLIBPATH)/STM32F0xx_StdPeriph_Driver/src
# Build path
BUILD_DIR = build

# #####################################
# source
# #####################################
SRCS = \
  main.c \
  delay.c \
  lcd1202_spi.c \
  usart.c \
  i2c.c \
  ina219.c \
  printf.c \
  stm32f0xx_it.c \
  system_stm32f0xx_hsi_pll.c
 
SRCSASM = \
  startup_stm32f030.s

# #####################################
# firmware library
# #####################################
PERIPHLIB_SOURCES = \
  stm32f0xx_gpio.c \
  stm32f0xx_i2c.c \
  stm32f0xx_misc.c \
  stm32f0xx_pwr.c \
  stm32f0xx_spi.c \
  stm32f0xx_syscfg.c \
  stm32f0xx_tim.c \
  stm32f0xx_usart.c \
  stm32f0xx_rcc.c

#######################################
# binaries
#######################################
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
CP = arm-none-eabi-objcopy
AR = arm-none-eabi-ar
SZ = arm-none-eabi-size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################
# CFLAGS
#######################################
# macros for gcc
DEFS = -DSTM32F030 -DUSE_STDPERIPH_DRIVER
ifeq ($(RAMBUILD), 1)
DEFS += -DVECT_TAB_SRAM
endif
ifeq ($(DEBUG), 1)
DEFS += -DDEBUG -D_DEBUG
endif
# includes for gcc
INCLUDES = -Iinc -Ilib
INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/Include
INCLUDES += -I$(PERIPHLIBPATH)/CMSIS/Device/ST/STM32F0xx/Include
INCLUDES += -I$(PERIPHLIBPATH)/STM32F0xx_StdPeriph_Driver/inc
# optimization
ifeq ($(DEBUG), 1)
OPT = -O0 -ggdb
else
OPT = -Os
endif
OPT += -ffunction-sections -fdata-sections
OPT += -fomit-frame-pointer -falign-functions=16
OPT += -fno-strict-aliasing -ffast-math -msoft-float -mfloat-abi=soft
# compile gcc flags
CFLAGS = -mthumb -mcpu=cortex-m0 -mtune=cortex-m0 $(DEFS) $(INCLUDES) $(OPT)
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif
# Generate dependency information
CFLAGS += -MD -MP -MF .dep/$(@F).d
CFLAGS += -std=gnu99 -Wall -pedantic

#######################################
# LDFLAGS
#######################################
# link script
ifeq ($(RAMBUILD), 1)
LDSCRIPT = MCU/STM32F030F4_ram.ld
else
LDSCRIPT = MCU/STM32F030F4_FLASH.ld
endif
# libraries
LIBS = -lc -lm -lnosys
LIBDIR =
LDFLAGS = -mthumb -mcpu=cortex-m0 -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections


# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex
load: $(BUILD_DIR)/$(TARGET).hex mcu_prog

#######################################
# build the application
#######################################
# list of firmware library objects
PERIPHLIB_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(PERIPHLIB_SOURCES:.c=.o)))
# list of C program objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.o)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(SRCSASM:.s=.o)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	@echo Compiling: $<
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo Compiling asm: $<
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(PERIPHLIB_OBJECTS) Makefile
	@echo Linking: $@
	@$(CC) $(OBJECTS) $(PERIPHLIB_OBJECTS) $(LDFLAGS) -o $@
	@echo '--------------------------------------------------------------------'
	@$(SZ) $@
	@rm -f $(BUILD_DIR)/*.o
	
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	@$(HEX) $< $@
	
$(BUILD_DIR):
	@mkdir -p $@


#######################################
# Load firmware
#######################################
mcu_prog:
	@$(PROGRAMMATOR) -c SWD -ME
	@$(PROGRAMMATOR) -c SWD -P "$(BUILD_DIR)/$(TARGET).hex" -V "$(BUILD_DIR)/$(TARGET).hex" -Q -Rst -Run
 
#######################################
# reset mcu
#######################################
mcu_reset:
	@$(PROGRAMMATOR) -c SWD -Rst -Run

#######################################
# delete all user application files
#######################################
clean:
	@-rm -fR .dep $(BUILD_DIR)
  
#
# Include the dependency files, should be the last of the makefile
#
-include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

# *** EOF ***
