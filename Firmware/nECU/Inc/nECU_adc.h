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

#define INTERNAL_TEMP_SLOPE (float)2.5  // slope defined in datasheet [mV/C]
#define INTERNAL_TEMP_V25 (float)0.76   // Voltage at 25C from calibration (defined in datasheet)
#define INTERNAL_TEMP_UPDATE_DELAY 1000 // once per second (1000ms)
#define INTERNAL_TEMP_MULTIPLIER 100    // value by which internal temperature result will be multipled

#define VREFINT_CALIB ((uint16_t *)((uint32_t)0x1FFF7A2A)) // Internal voltage reference raw value at 30 degrees C, VDDA=3.3V (defined in datasheet)
#define VREF_CALIB 3.3                                     // VDDA voltage at which all other values were created (defined in datasheet)

#define ADC_MAX_VALUE_12BIT 4095 // Maximum value a 12bit ADC can produce

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

  uint16_t *getPointer_MAP_ADC(void);
  uint16_t *getPointer_Backpressure_ADC(void);
  uint16_t *getPointer_OX_ADC(void);
  uint16_t *getPointer_InternalTemp_ADC(void);
  uint16_t *getPointer_SpeedSens_ADC(Speed_Sensor_ID ID);

  /* Conversion functions */
  float ADCToVolts(uint16_t ADCValue);
  uint16_t VoltsToADC(float Voltage);

  /* Internal Temperatre (MCU) */
  void nECU_InternalTemp_Init(void);                                                                                            // initialize structure
  void nECU_InternalTemp_Callback(void);                                                                                        // run when conversion ended
  void nECU_InternalTemp_Update(void);                                                                                          // perform update of output variables
  uint16_t *nECU_InternalTemp_getTemperature(void);                                                                             // return current temperature pointer (multiplied 100x)
  void nECU_ADC_AverageDMA(ADC_HandleTypeDef *hadc, uint16_t *inData, uint16_t inLength, uint16_t *outData, float smoothAlpha); // average out dma buffer
  void nECU_ADC_expSmooth(uint16_t *inBuf, uint16_t *outBuf, uint16_t inLength, float alpha);                                   // exponential smoothing algorithm
  bool nECU_ADC_test(void);                                                                                                     // test script for general functions

#ifdef __cplusplus
}
#endif

#endif /* _NECU_ADC_H_ */