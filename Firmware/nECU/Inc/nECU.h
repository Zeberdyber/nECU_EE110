/**
 ******************************************************************************
 * @file    nECU.h
 * @brief   This file contains all the configuration and definitions for all
 *          nECU files.
 */

/* includes */
#include "stdio.h"
#include "stdbool.h"
#include "string.h"

#include "main.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "can.h"
#include "gpio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

#include "nECU_types.h"

#include "nECU_adc.h"
#include "nECU_button.h"
#include "nECU_can.h"
#include "nECU_data_processing.h"
#include "nECU_debug.h"
#include "nECU_EGT.h"
#include "nECU_flash.h"
#include "nECU_flowControl.h"
#include "nECU_frames.h"
#include "nECU_Input_Analog.h"
#include "nECU_Input_Frequency.h"
#include "nECU_Knock.h"
#include "nECU_main.h"
#include "nECU_menu.h"
#include "nECU_OnBoardLED.h"
#include "nECU_PC.h"
#include "nECU_spi.h"
#include "nECU_stock.h"
#include "nECU_table.h"
#include "nECU_tests.h"
#include "nECU_tim.h"
#include "nECU_UART.h"