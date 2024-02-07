/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

// local variables
nECU_ADC1 adc1_data;
nECU_ADC2 adc2_data;
nECU_ADC3 adc3_data;
nECU_InternalTemp MCU_temperature;

// output variables
uint16_t *ADC_MAP, *ADC_BackPressure, *ADC_OX, *ADC_AI1, *ADC_AI2, *ADC_AI3, *ADC_InternalTemp, *ADC_VREF;
uint16_t *ADC_V1, *ADC_V2, *ADC_V3, *ADC_V4;

/* Interrupt functions */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &GENERAL_ADC) // if ADC1 perform its routine
  {
    if (adc1_data.status.callback_full == true) // check if routine was not called
    {
      adc1_data.status.overflow = true;
    }
    adc1_data.status.callback_half = false; // clear flag to prevent memory access while DMA working
    adc1_data.status.callback_full = true;
  }
  else if (hadc == &SPEED_ADC) // if ADC2 perform its routine
  {
    if (adc2_data.status.callback_full == true) // check if routine was not called
    {
      adc2_data.status.overflow = true;
    }
    adc2_data.status.callback_half = false; // clear flag to prevent memory access while DMA working
    adc2_data.status.callback_full = true;
  }
  else if (hadc == &KNOCK_ADC) // if ADC3 perform its routine
  {
    if (adc3_data.status.callback_full == true) // check if routine was not called
    {
      adc3_data.status.overflow = true;
    }
    adc3_data.status.callback_half = false; // clear flag to prevent memory access while DMA working
    adc3_data.status.callback_full = true;
  }
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &GENERAL_ADC) // if ADC1 perform its routine
  {
    if (adc1_data.status.callback_half == true) // check if routine was not called
    {
      adc1_data.status.overflow = true;
    }
    adc1_data.status.callback_half = true;
    adc1_data.status.callback_full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &SPEED_ADC) // if ADC2 perform its routine
  {
    if (adc2_data.status.callback_half == true) // check if routine was not called
    {
      adc2_data.status.overflow = true;
    }
    adc2_data.status.callback_half = true;
    adc2_data.status.callback_full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &KNOCK_ADC) // if ADC3 perform its routine
  {
    if (adc3_data.status.callback_half == true) // check if routine was not called
    {
      adc3_data.status.overflow = true;
    }
    adc3_data.status.callback_half = true;
    adc3_data.status.callback_full = false; // clear flag to prevent memory access while DMA working
  }
}

/* Start functions */
void ADC_START_ALL(void)
{
  ADC1_START();
  ADC2_START();
  ADC3_START();
}
void ADC1_START(void)
{
  ADC_MAP = &adc1_data.out_buffer[0];
  ADC_BackPressure = &adc1_data.out_buffer[1];
  ADC_OX = &adc1_data.out_buffer[2];
  ADC_AI1 = &adc1_data.out_buffer[3];
  ADC_AI2 = &adc1_data.out_buffer[4];
  ADC_AI3 = &adc1_data.out_buffer[5];
  ADC_InternalTemp = &adc1_data.out_buffer[6];
  ADC_VREF = &adc1_data.out_buffer[7];

  HAL_ADC_Start_DMA(&GENERAL_ADC, (uint32_t *)adc1_data.in_buffer, sizeof(adc1_data.in_buffer) / sizeof(uint16_t));

  adc1_data.status.callback_half = false;
  adc1_data.status.callback_full = false;
  adc1_data.status.overflow = false;
  adc1_data.status.working = true;

  nECU_InternalTemp_Init();
}
void ADC2_START(void)
{
  ADC_V1 = &adc2_data.out_buffer[0];
  ADC_V2 = &adc2_data.out_buffer[1];
  ADC_V3 = &adc2_data.out_buffer[2];
  ADC_V4 = &adc2_data.out_buffer[3];

  HAL_ADC_Start_DMA(&SPEED_ADC, (uint32_t *)adc2_data.in_buffer, sizeof(adc2_data.in_buffer) / sizeof(uint16_t));

  adc2_data.status.callback_half = false;
  adc2_data.status.callback_full = false;
  adc2_data.status.overflow = false;
  adc2_data.status.working = true;
}
void ADC3_START(void)
{
  adc3_data.samplingTimer = &KNOCK_ADC_SAMPLING_TIMER;
  HAL_TIM_Base_Start(adc3_data.samplingTimer);
  HAL_ADC_Start_DMA(&KNOCK_ADC, (uint32_t *)adc3_data.in_buffer, sizeof(adc3_data.in_buffer) / sizeof(uint16_t));

  adc3_data.status.callback_half = false;
  adc3_data.status.callback_full = false;
  adc3_data.status.overflow = false;
  adc3_data.status.working = true;
}
/* Stop functions */
void ADC_STOP_ALL(void)
{
  if (adc1_data.status.working == true)
  {
    ADC1_STOP();
  }
  if (adc2_data.status.working == true)
  {
    ADC2_STOP();
  }
  if (adc3_data.status.working == true)
  {
    ADC3_STOP();
  }
}
void ADC1_STOP(void)
{
  adc3_data.UART_transmission = nECU_UART_KnockTx();
  HAL_ADC_Stop_DMA(&GENERAL_ADC);
  nECU_ADC1_Routine(); // finish routine if flags pending
  adc1_data.status.working = false;
}
void ADC2_STOP(void)
{
  HAL_ADC_Stop_DMA(&SPEED_ADC);
  nECU_ADC2_Routine(); // finish routine if flags pending
  adc2_data.status.working = false;
}
void ADC3_STOP(void)
{
  HAL_TIM_Base_Stop(adc3_data.samplingTimer);
  HAL_ADC_Stop_DMA(&KNOCK_ADC);
  adc2_data.status.working = false;
}
/* ADC Rutines */
void nECU_ADC_All_Routine(void)
{
  /* Remember that all ADC are working simoutainously, all callbacks will be at the same time */
  if (adc1_data.status.working == true)
  {
    nECU_ADC1_Routine();
  }
  if (adc2_data.status.working == true)
  {
    nECU_ADC2_Routine();
  }
  if (adc2_data.status.working == true)
  {
    nECU_ADC3_Routine();
  }
}
void nECU_ADC1_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc1_data.status.callback_half == true)
  {
    nECU_ADC_AverageDMA(&GENERAL_ADC, adc1_data.in_buffer, GENERAL_DMA_LEN / 2, adc1_data.out_buffer, GENERAL_SMOOTH_ALPHA);
    adc1_data.status.callback_half = false; // clear flag
  }
  else if (adc1_data.status.callback_full == true)
  {
    nECU_ADC_AverageDMA(&GENERAL_ADC, &adc1_data.in_buffer[(GENERAL_DMA_LEN / 2) - 1], GENERAL_DMA_LEN / 2, adc1_data.out_buffer, GENERAL_SMOOTH_ALPHA);
    nECU_InternalTemp_Callback();
    adc1_data.status.callback_full = false; // clear flag
  }
}
void nECU_ADC2_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc2_data.status.callback_half == true)
  {
    nECU_ADC_AverageDMA(&SPEED_ADC, adc2_data.in_buffer, SPEED_DMA_LEN / 2, adc2_data.out_buffer, SPEED_SMOOTH_ALPHA);
    adc2_data.status.callback_half = false; // clear flag
  }
  else if (adc2_data.status.callback_full == true)
  {
    nECU_ADC_AverageDMA(&GENERAL_ADC, &adc2_data.in_buffer[(SPEED_DMA_LEN / 2) - 1], SPEED_DMA_LEN / 2, adc2_data.out_buffer, SPEED_SMOOTH_ALPHA);
    adc2_data.status.callback_full = false; // clear flag
  }
}
void nECU_ADC3_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc3_data.status.callback_half == true)
  {
    adc3_data.status.callback_half = false; // clear flag
    if (*adc3_data.UART_transmission == true)
    {
#if TEST_UART == 1
      Send_Triangle_UART();
      return;
#endif
      nECU_UART_SendKnock(&adc3_data.in_buffer[0]);
    }
    nECU_Knock_ADC_Callback(&adc3_data.in_buffer[0]);
  }
  else if (adc3_data.status.callback_full == true)
  {
    adc3_data.status.callback_full = false; // clear flag
    if (*adc3_data.UART_transmission == true)
    {
#if TEST_UART == 1
      Send_Triangle_UART();
      return;
#endif
      nECU_UART_SendKnock(&adc3_data.in_buffer[(KNOCK_DMA_LEN / 2) - 1]);
    }
    nECU_Knock_ADC_Callback(&adc3_data.in_buffer[(KNOCK_DMA_LEN / 2) - 1]);
  }
}

/* Conversion functions */
float ADCToVolts(uint16_t ADCValue)
{
  return (VREF_CALIB * ADCValue) / ADC_MAX_VALUE_12BIT;
}
uint16_t VoltsToADC(float Voltage)
{
  Voltage *= ADC_MAX_VALUE_12BIT;
  Voltage /= VREF_CALIB;
  return (uint16_t)Voltage;
}

/* Internal Temperatre (MCU) */
void nECU_InternalTemp_Init(void) // initialize structure
{
  MCU_temperature.conv_divider.value = 0;
  MCU_temperature.conv_divider.preset = INTERNAL_TEMP_CONV_DIV;
  MCU_temperature.ADC_data = ADC_InternalTemp;
  MCU_temperature.temperature = 0;
}
void nECU_InternalTemp_Callback(void) // run when conversion ended
{
  MCU_temperature.conv_divider.value++;
  if (MCU_temperature.conv_divider.value > INTERNAL_TEMP_CONV_DIV)
  {
    MCU_temperature.conv_divider.value = 0;
    nECU_InternalTemp_Update();
    MCU_temperature.upToDate = true;
  }
}
void nECU_InternalTemp_Update(void) // perform update of output variables
{
  // convert to temperature
  float Temperature = ADCToVolts(*MCU_temperature.ADC_data);
  Temperature -= INTERNAL_TEMP_V25;
  Temperature /= (INTERNAL_TEMP_SLOPE / 1000); // 1000: mV -> V
  Temperature += 25;
  MCU_temperature.temperature = Temperature * 100;
}
uint16_t *nECU_InternalTemp_getTemperature(void) // return current temperature pointer (multiplied 100x)
{
  /* returned value is multipled 100 times, which means that it carries two digits after dot */
  return &MCU_temperature.temperature;
}

void nECU_ADC_AverageDMA(ADC_HandleTypeDef *hadc, uint16_t *inData, uint16_t inLength, uint16_t *outData, float smoothAlpha) // average out dma buffer
{
  uint32_t numChannels = hadc->Init.NbrOfConversion;
  uint32_t avgSum[numChannels];  // create buffer for sum values
  uint16_t avgData[numChannels]; // create temporary buffer for smoothing

  for (uint8_t avgLen = 0; avgLen < numChannels; avgLen++) // clear buffer
  {
    avgSum[avgLen] = 0;
  }

  for (uint16_t convCount = 0; convCount < inLength; convCount += numChannels) // increment per full conversions
  {
    for (uint8_t convChannel = 0; convChannel < numChannels; convChannel++) // go threw each measurement
    {
      avgSum[convChannel] += inData[convCount + convChannel]; // add up new measurement
    }
  }

  for (uint8_t Channel = 0; Channel < numChannels; Channel++)
  {
    avgData[Channel] = avgSum[Channel] / (inLength / numChannels); // average out
  }
  nECU_ADC_expSmooth(avgData, outData, numChannels, smoothAlpha);
}
void nECU_ADC_expSmooth(uint16_t *inBuf, uint16_t *outBuf, uint16_t inLength, float alpha) // exponential smoothing algorithm
{
  for (size_t i = 0; i < inLength; i++)
  {
    outBuf[i] = (inBuf[i] * alpha) + (outBuf[i] * (1 - alpha));
  }
}

bool nECU_ADC_test(void) // test script for general functions
{
  ADC_HandleTypeDef testADC;
  testADC.Init.NbrOfConversion = 2;

  uint16_t inputBuf[] = {10, 0, 20, 1, 30, 3, 40, 32768};
  uint16_t outputBuf[] = {0, 0, 0, 0}; // tool large buffer to test data spilage

  nECU_ADC_AverageDMA(&testADC, inputBuf, 8, &outputBuf[1], 1); // no smoothing

  // check limits for spilage
  if (outputBuf[0] != 0)
  {
    return false;
  }
  if (outputBuf[3] != 0)
  {
    return false;
  }

  // check for correct answers
  if (outputBuf[1] != 25)
  {
    return false;
  }
  if (outputBuf[2] != 8193)
  {
    return false;
  }

  uint16_t avgInputNew[] = {100, 4000};                    // new data for smoothing
  nECU_ADC_expSmooth(avgInputNew, &outputBuf[1], 2, 0.75); // perform smoothing with new data

  // check limits for spilage
  if (outputBuf[0] != 0)
  {
    return false;
  }
  if (outputBuf[3] != 0)
  {
    return false;
  }

  // check for correct answers
  if (outputBuf[1] != 81)
  {
    return false;
  }
  if (outputBuf[2] != 5048)
  {
    return false;
  }

  return true;
}