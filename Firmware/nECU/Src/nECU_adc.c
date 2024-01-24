/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

nECU_ADC1 adc1_data;
nECU_ADC2 adc2_data;

uint16_t ADC3_DMA_BUF[KNOCK_BUFFOR_SIZE];

uint16_t *ADC_MAP, *ADC_BackPressure, *ADC_OX, *ADC_AI1, *ADC_AI2, *ADC_AI3, *ADC_InternalTemp, *ADC_VREF;
uint16_t *ADC_V1, *ADC_V2, *ADC_V3, *ADC_V4;

bool Callback_ADC3_Half = false, Callback_ADC3_Full = false, Callback_ADC3_Overflow = false;
extern bool Knock_UART_Transmission;

float InternalTemp = 0;

/* Interrupt functions */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &GENERAL_ADC) // if ADC1 perform its routine
  {
    if (adc1_data.callback_full == true) // check if routine was not called
    {
      adc1_data.overflow = true;
    }
    adc1_data.callback_half = false; // clear flag to prevent memory access while DMA working
    adc1_data.callback_full = true;
  }
  else if (hadc == &SPEED_ADC) // if ADC2 perform its routine
  {
    if (adc2_data.callback_full == true) // check if routine was not called
    {
      adc2_data.overflow = true;
    }
    adc2_data.callback_half = false; // clear flag to prevent memory access while DMA working
    adc2_data.callback_full = true;
  }
  else if (hadc == &KNOCK_ADC) // if ADC3 perform its routine
  {
    if (Callback_ADC3_Full == true) // check if routine was not called
    {
      Callback_ADC3_Overflow = true;
    }
    Callback_ADC3_Half = false; // clear flag to prevent memory access while DMA working
    Callback_ADC3_Full = true;
  }
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &GENERAL_ADC) // if ADC1 perform its routine
  {
    if (adc1_data.callback_half == true) // check if routine was not called
    {
      adc1_data.overflow = true;
    }
    adc1_data.callback_half = true;
    adc1_data.callback_full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &SPEED_ADC) // if ADC2 perform its routine
  {
    if (adc2_data.callback_half == true) // check if routine was not called
    {
      adc2_data.overflow = true;
    }
    adc2_data.callback_half = true;
    adc2_data.callback_full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &KNOCK_ADC) // if ADC3 perform its routine
  {
    if (Callback_ADC3_Half == true) // check if routine was not called
    {
      Callback_ADC3_Overflow = true;
    }
    Callback_ADC3_Half = true;
    Callback_ADC3_Full = false; // clear flag to prevent memory access while DMA working
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
  ADC_MAP = &adc1_data.buffer[0];
  ADC_BackPressure = &adc1_data.buffer[1];
  ADC_OX = &adc1_data.buffer[2];
  ADC_AI1 = &adc1_data.buffer[3];
  ADC_AI2 = &adc1_data.buffer[4];
  ADC_AI3 = &adc1_data.buffer[5];
  ADC_InternalTemp = &adc1_data.buffer[6];
  ADC_VREF = &adc1_data.buffer[7];

  HAL_ADC_Start_DMA(&GENERAL_ADC, (uint32_t *)adc1_data.buffer, 8);

  adc1_data.callback_half = false;
  adc1_data.callback_full = false;
  adc1_data.overflow = false;
  adc1_data.working = true;
}
void ADC2_START(void)
{
  ADC_V1 = &adc2_data.buffer[0];
  ADC_V2 = &adc2_data.buffer[1];
  ADC_V3 = &adc2_data.buffer[2];
  ADC_V4 = &adc2_data.buffer[3];

  HAL_ADC_Start_DMA(&SPEED_ADC, (uint32_t *)adc2_data.buffer, 4);

  adc2_data.callback_half = false;
  adc2_data.callback_full = false;
  adc2_data.overflow = false;
  adc2_data.working = true;
}
void ADC3_START(void)
{
  HAL_TIM_Base_Start(&KNOCK_ADC_SAMPLING_TIMER);
  HAL_ADC_Start_DMA(&KNOCK_ADC, (uint32_t *)ADC3_DMA_BUF, KNOCK_BUFFOR_SIZE);
}
/* Stop functions */
void ADC_STOP_ALL(void)
{
  if (adc1_data.working == true)
  {
    ADC1_STOP();
  }
  if (adc2_data.working == true)
  {
    ADC2_STOP();
  }

  ADC3_STOP();
}
void ADC1_STOP(void)
{
  HAL_ADC_Stop_DMA(&GENERAL_ADC);
  nECU_ADC1_Routine(); // finish routine if flags pending
  adc1_data.working = false;
}
void ADC2_STOP(void)
{
  HAL_ADC_Stop_DMA(&SPEED_ADC);
  nECU_ADC2_Routine(); // finish routine if flags pending
  adc2_data.working = false;
}
void ADC3_STOP(void)
{
  HAL_TIM_Base_Stop(&KNOCK_ADC_SAMPLING_TIMER);
  HAL_ADC_Stop_DMA(&KNOCK_ADC);
}
/* ADC Rutines */
void nECU_ADC_All_Routine(void)
{
  /* Remember that all ADC are working simoutainously, all callbacks will be at the same time */
  if (adc1_data.working == true)
  {
    nECU_ADC1_Routine();
  }
  if (adc2_data.working == true)
  {
    nECU_ADC2_Routine();
  }
  nECU_ADC3_Routine();
}
void nECU_ADC1_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc1_data.callback_half == true)
  {
    adc1_data.callback_half = false; // clear flag
  }
  else if (adc1_data.callback_full == true)
  {
    adc1_data.callback_full = false; // clear flag
  }
}
void nECU_ADC2_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc2_data.callback_half == true)
  {
    adc2_data.callback_half = false; // clear flag
  }
  else if (adc2_data.callback_full == true)
  {
    adc2_data.callback_full = false; // clear flag
  }
}
void nECU_ADC3_Routine(void)
{
  /* Conversion Completed callbacks */
  if (Callback_ADC3_Half == true)
  {
    Callback_ADC3_Half = false; // clear flag
    if (Knock_UART_Transmission == true)
    {
#if TEST_UART == 1
      Send_Triangle_UART();
      return;
#endif
      nECU_UART_SendKnock(&ADC3_DMA_BUF[0]);
    }
    nECU_Knock_ADC_Callback(&ADC3_DMA_BUF[0]);
  }
  else if (Callback_ADC3_Full == true)
  {
    Callback_ADC3_Full = false; // clear flag
    if (Knock_UART_Transmission == true)
    {
#if TEST_UART == 1
      Send_Triangle_UART();
      return;
#endif
      nECU_UART_SendKnock(&ADC3_DMA_BUF[(KNOCK_BUFFOR_SIZE / 2) - 1]);
    }
    nECU_Knock_ADC_Callback(&ADC3_DMA_BUF[(KNOCK_BUFFOR_SIZE / 2) - 1]);
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
/* Other ADC */
float get_InternalTemperature(void) // Calculate current temperature of the IC
{
  // From reference manual
  float Temperature = ADCToVolts(*ADC_InternalTemp);
  Temperature -= INTERNAL_TEMP_V25;
  Temperature /= (INTERNAL_TEMP_SLOPE / 1000); // 1000: mV -> V
  Temperature += 25;
  return Temperature;
}
void ADC_LP_Update(void) // Update low priority variables
{
  InternalTemp = get_InternalTemperature();
}
