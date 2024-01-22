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
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "nECU_UART.h"
#include "nECU_Knock.h"

/* Definitions */
#define INTERNAL_TEMP_SLOPE 2.5                            // slope defined in datasheet [mV/C]
#define INTERNAL_TEMP_V25 0.76                             // Voltage at 25C from calibration (defined in datasheet)
#define VREFINT_CALIB ((uint16_t *)((uint32_t)0x1FFF7A2A)) // Internal voltage reference raw value at 30 degrees C, VDDA=3.3V (defined in datasheet)
#define VREF_CALIB 3.3                                     // VDDA voltage at which all other values were created (defined in datasheet)
#define ADC_MAX_VALUE_12BIT 4095                           // Maximum value a 12bit ADC can produce
#define KNOCK_SAMPLES_IN_BUFFOR 512                        // number of samples to gather in single DMA pull
#define KNOCK_BUFFOR_SIZE (KNOCK_SAMPLES_IN_BUFFOR)        // Size of DMA buffor for knock sensor input (smaller as it is of uint32_t type)

  /* typedef */

  /* Interrupt functions */
  void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
  void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

  /* Start functions */
  void ADC_START_ALL(void);
  void ADC1_START(void);
  void ADC2_START(void);
  void ADC3_START(void);
  /* Stop functions */
  void ADC_STOP_ALL(void);
  void ADC1_STOP(void);
  void ADC2_STOP(void);
  void ADC3_STOP(void);
  /* ADC Rutines */
  void nECU_ADC_All_Routine(void);
  void nECU_ADC1_Routine(void);
  void nECU_ADC2_Routine(void);
  void nECU_ADC3_Routine(void);

  /* Conversion functions */
  float ADCToVolts(uint16_t ADCValue);
  uint16_t VoltsToADC(float Voltage);
  /* Other ADC */
  float get_InternalTemperature(void); // Calculate current temperature of the IC
  void ADC_LP_Update(void);            // Update low priority variables

#ifdef __cplusplus
}
#endif

#endif /* _NECU_ADC_H_ */