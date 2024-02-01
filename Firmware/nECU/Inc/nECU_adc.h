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

/* HOW TO GET ADC FULL CONVERSION TIME
---------------------------------------------------------------
Time[s] = ((ResolutionTime {12bit => 15} + SamplingTime)* ClockPrescaler * BufferLen) / APB2_CLOCK {42MHz}
---------------------------------------------------------------
Time ADC1 {/8, 12bit, 8 channels, 480 cycles each}
[BufferLen = 8] ==>> 0,75 [ms]
[BufferLen = 320] ==>> 302 [ms]
*/
/* Definitions */
#define APB2_CLOCK 42000000 // APB2 clock speed

#define GENERAL_ADC hadc1                                                                                                                                                                                                      // ADC responsible for general analog inputs
#define GENERAL_CHANNEL_COUNT 8                                                                                                                                                                                                // number of initialized channels of GENERAL_ADC
#define GENERAL_ADC_CLOCKDIVIDER 8                                                                                                                                                                                             // values of a clock divider for this peripheral
#define GENERAL_ADC_SAMPLINGCYCLES 480                                                                                                                                                                                         // number of cycles that it takes to conver single channel
#define GENERAL_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                                        // how many cycles per conversion is added due to precission
#define GENERAL_TARGET_UPDATE 25                                                                                                                                                                                               // time in ms how often should values be updated
#define GENERAL_DMA_LEN ((uint16_t)(((APB2_CLOCK * GENERAL_TARGET_UPDATE) / 1000) / ((GENERAL_ADC_RESOLUTIONCYCLES + GENERAL_ADC_SAMPLINGCYCLES) * GENERAL_ADC_CLOCKDIVIDER * GENERAL_CHANNEL_COUNT))) * GENERAL_CHANNEL_COUNT // length of DMA buffer for GENERAL_ADC
#define GENERAL_SMOOTH_ALPHA (float)0.5                                                                                                                                                                                        // strength for smoothing the data

#define SPEED_ADC hadc2                                                                                                                                                                                          // ADC responsible for speed sensor data collection
#define SPEED_CHANNEL_COUNT 4                                                                                                                                                                                    // number of initialized channels of SPEED_ADC
#define SPEED_ADC_CLOCKDIVIDER 8                                                                                                                                                                                 // values of a clock divider for this peripheral
#define SPEED_ADC_SAMPLINGCYCLES 480                                                                                                                                                                             // number of cycles that it takes to conver single channel
#define SPEED_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                            // how many cycles per conversion is added due to precission
#define SPEED_TARGET_UPDATE 25                                                                                                                                                                                   // time in ms how often should values be updated
#define SPEED_DMA_LEN ((uint16_t)(((APB2_CLOCK * SPEED_TARGET_UPDATE) / 1000) / ((SPEED_ADC_RESOLUTIONCYCLES + SPEED_ADC_SAMPLINGCYCLES) * SPEED_ADC_CLOCKDIVIDER * SPEED_CHANNEL_COUNT))) * SPEED_CHANNEL_COUNT // length of DMA buffer for GENERAL_ADC
#define SPEED_SMOOTH_ALPHA (float)0.8                                                                                                                                                                            // strength for smoothing the data

#define KNOCK_ADC hadc3       // ADC responsible for knock sensor data collection
#define KNOCK_CHANNEL_COUNT 4 // number of initialized channels of KNOCK_ADC
#define KNOCK_DMA_LEN 512     // length of DMA buffer for KNOCK_ADC

#define INTERNAL_TEMP_SLOPE (float)2.5 // slope defined in datasheet [mV/C]
#define INTERNAL_TEMP_V25 (float)0.76  // Voltage at 25C from calibration (defined in datasheet)
#define INTERNAL_TEMP_CONV_DIV 100     // division of conversions taken into account (8bit)

#define VREFINT_CALIB ((uint16_t *)((uint32_t)0x1FFF7A2A)) // Internal voltage reference raw value at 30 degrees C, VDDA=3.3V (defined in datasheet)
#define VREF_CALIB 3.3                                     // VDDA voltage at which all other values were created (defined in datasheet)

#define ADC_MAX_VALUE_12BIT 4095 // Maximum value a 12bit ADC can produce

  /* typedef */
  typedef struct
  {
    uint16_t buffer[GENERAL_DMA_LEN];
    uint16_t out[GENERAL_CHANNEL_COUNT];
    bool working;
    bool callback_half, callback_full, overflow;
  } nECU_ADC1;
  typedef struct
  {
    uint16_t buffer[SPEED_DMA_LEN];
    uint16_t out[SPEED_CHANNEL_COUNT];
    bool working;
    bool callback_half, callback_full, overflow;
  } nECU_ADC2;
  typedef struct
  {
    uint16_t buffer[KNOCK_DMA_LEN];
    bool working;
    bool callback_half, callback_full, overflow;
    bool *UART_transmission;
    TIM_HandleTypeDef *samplingTimer;

  } nECU_ADC3;
  typedef struct
  {
    uint8_t conv_count;
    uint16_t *ADC_data;
    uint16_t temperature;
    bool upToDate;
  } nECU_InternalTemp;

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