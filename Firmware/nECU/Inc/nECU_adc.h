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

/* HOW TO GET ADC FULL CONVERSION TIME
---------------------------------------------------------------
Time[s] = ((ResolutionTime {12bit => 15} + SamplingTime)* ClockPrescaler * BufferLen) / APB2_CLOCK {42MHz}
---------------------------------------------------------------
Time ADC1 {/8, 12bit, 8 channels, 480 cycles each}
[BufferLen = 8] ==>> 0,75 [ms]
[BufferLen = 320] ==>> 302 [ms]
*/
/* Definitions */
#define GENERAL_ADC hadc1               // ADC responsible for general analog inputs
#define GENERAL_SMOOTH_ALPHA (float)0.5 // strength for smoothing the data

#define SPEED_ADC hadc2               // ADC responsible for speed sensor data collection
#define SPEED_SMOOTH_ALPHA (float)0.8 // strength for smoothing the data

#define KNOCK_ADC hadc3 // ADC responsible for knock sensor data collection
  /* Interrupt functions */
  void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
  void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

  /* Start functions */
  bool nECU_ADC1_START(void);
  bool nECU_ADC2_START(void);
  bool nECU_ADC3_START(void);
  /* Stop functions */
  bool nECU_ADC1_STOP(void);
  bool nECU_ADC2_STOP(void);
  bool nECU_ADC3_STOP(void);
  /* ADC Rutines */
  void nECU_ADC1_Routine(void);
  void nECU_ADC2_Routine(void);
  void nECU_ADC3_Routine(void);

  uint16_t *nECU_ADC1_getPointer(nECU_ADC1_ID ID);
  uint16_t *nECU_ADC2_getPointer(nECU_ADC2_ID ID);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_ADC_H_ */