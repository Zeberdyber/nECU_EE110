/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

uint16_t ADC1_DMA_BUF[KNOCK_BUFFOR_SIZE];
uint16_t ADC2_DMA_BUF[10];
uint16_t ADC3_DMA_BUF[KNOCK_BUFFOR_SIZE];

uint16_t *ADC_MAP, *ADC_BackPressure, *ADC_OX, *ADC_AI1, *ADC_AI2, *ADC_AI3, *ADC_InternalTemp, *ADC_VREF;
uint16_t *ADC_V1, *ADC_V2, *ADC_V3, *ADC_V4;

bool Callback_ADC1_Half = false, Callback_ADC1_Full = false, Callback_ADC1_Overflow = false;
bool Callback_ADC2_Half = false, Callback_ADC2_Full = false, Callback_ADC2_Overflow = false;
bool Callback_ADC3_Half = false, Callback_ADC3_Full = false, Callback_ADC3_Overflow = false;
extern bool Knock_UART_Transmission;

float InternalTemp = 0;

/* Interrupt functions */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &hadc1) // if ADC1 perform its routine
  {
    if (Callback_ADC1_Full == true) // check if routine was not called
    {
      Callback_ADC1_Overflow = true;
    }
    Callback_ADC1_Half = false; // clear flag to prevent memory access while DMA working
    Callback_ADC1_Full = true;
  }
  else if (hadc == &hadc2) // if ADC2 perform its routine
  {
    if (Callback_ADC2_Full == true) // check if routine was not called
    {
      Callback_ADC2_Overflow = true;
    }
    Callback_ADC2_Half = false; // clear flag to prevent memory access while DMA working
    Callback_ADC2_Full = true;
  }
  else if (hadc == &hadc3) // if ADC3 perform its routine
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
  if (hadc == &hadc1) // if ADC1 perform its routine
  {
    if (Callback_ADC1_Half == true) // check if routine was not called
    {
      Callback_ADC1_Overflow = true;
    }
    Callback_ADC1_Half = true;
    Callback_ADC1_Full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &hadc2) // if ADC2 perform its routine
  {
    if (Callback_ADC2_Half == true) // check if routine was not called
    {
      Callback_ADC2_Overflow = true;
    }
    Callback_ADC2_Half = true;
    Callback_ADC2_Full = false; // clear flag to prevent memory access while DMA working
  }
  else if (hadc == &hadc3) // if ADC3 perform its routine
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
  ADC_MAP = &ADC1_DMA_BUF[0];
  ADC_BackPressure = &ADC1_DMA_BUF[1];
  ADC_OX = &ADC1_DMA_BUF[2];
  ADC_AI1 = &ADC1_DMA_BUF[3];
  ADC_AI2 = &ADC1_DMA_BUF[4];
  ADC_AI3 = &ADC1_DMA_BUF[5];
  ADC_InternalTemp = &ADC1_DMA_BUF[6];
  ADC_VREF = &ADC1_DMA_BUF[7];

  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC1_DMA_BUF, 8);
}
void ADC2_START(void)
{
  ADC_V1 = &ADC2_DMA_BUF[0];
  ADC_V2 = &ADC2_DMA_BUF[1];
  ADC_V3 = &ADC2_DMA_BUF[2];
  ADC_V4 = &ADC2_DMA_BUF[3];

  HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADC2_DMA_BUF, 4);
}
void ADC3_START(void)
{
  HAL_TIM_Base_Start(&ADC_SAMPLING_TIMER);
  HAL_ADC_Start_DMA(&hadc3, (uint32_t *)ADC3_DMA_BUF, KNOCK_BUFFOR_SIZE);
}
/* Stop functions */
void ADC_STOP_ALL(void)
{
  ADC1_STOP();
  ADC2_STOP();
  ADC3_STOP();
}
void ADC1_STOP(void)
{
  HAL_ADC_Stop_DMA(&hadc1);
}
void ADC2_STOP(void)
{
  HAL_ADC_Stop_DMA(&hadc2);
}
void ADC3_STOP(void)
{
  HAL_TIM_Base_Stop(&ADC_SAMPLING_TIMER);
  HAL_ADC_Stop_DMA(&hadc3);
}
/* ADC Rutines */
void nECU_ADC_All_Routine(void)
{
  /* Remember that all ADC are working simoutainously, all callbacks will be at the same time */
  nECU_ADC1_Routine();
  nECU_ADC2_Routine();
  nECU_ADC3_Routine();
}
void nECU_ADC1_Routine(void)
{
  /* Conversion Completed callbacks */
  if (Callback_ADC1_Half == true)
  {
    Callback_ADC1_Half = false; // clear flag
  }
  else if (Callback_ADC1_Full == true)
  {
    Callback_ADC1_Full = false; // clear flag
  }
}
void nECU_ADC2_Routine(void)
{
  /* Conversion Completed callbacks */
  if (Callback_ADC2_Half == true)
  {
    Callback_ADC2_Half = false; // clear flag
  }
  else if (Callback_ADC2_Full == true)
  {
    Callback_ADC2_Full = false; // clear flag
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
