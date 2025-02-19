/**
 ******************************************************************************
 * @file    nECU_adc.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_adc.c file
 */
#ifndef _NECU_ADC_H_
#define _NECU_ADC_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "nECU_UART.h"
#include "nECU_Knock.h"
#include "stdlib.h"

/* HOW TO GET ADC FULL CONVERSION TIME
---------------------------------------------------------------
Time[s] = ((ResolutionTime {12bit => 15} + SamplingTime)* ClockPrescaler * BufferLen) / APB2_CLOCK {42MHz}
---------------------------------------------------------------
Time ADC1 {/8, 12bit, 8 channels, 480 cycles each}
[BufferLen = 8] ==>> 0,75 [ms]
[BufferLen = 320] ==>> 302 [ms]
*/
/* Definitions */
#define GENERAL_SMOOTH_ALPHA (float)0.5 // strength for smoothing the data
#define SPEED_SMOOTH_ALPHA (float)0.8   // strength for smoothing the data

  /* Interrupt functions */
  void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
  void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

  bool nECU_ADC_START(nECU_ADC_Sensor_ID ID);
  bool nECU_ADC_STOP(nECU_ADC_Sensor_ID ID);
  bool nECU_ADC_Routine(nECU_ADC_Sensor_ID ID);

  static nECU_HADC_ID nECU_ADC_Identify_SensorID(nECU_ADC_Sensor_ID ID); // returns correcr hadc id based on given sensor
  static nECU_HADC_ID nECU_ADC_Identify_hadc(ADC_HandleTypeDef *hadc);   // returns ID of given hadc structure pointer

  uint16_t *nECU_ADC_getPointer(nECU_ADC_Sensor_ID ID);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_ADC_H_ */