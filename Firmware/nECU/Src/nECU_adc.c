/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

// local variables
static nECU_ADC1 adc1_data;
static nECU_ADC2 adc2_data;
static nECU_ADC3 adc3_data;

extern nECU_ProgramBlockData D_ADC1, D_ADC2, D_ADC3; // diagnostic and flow control data

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
bool ADC_START_ALL(void)
{
  bool status = false;
  status |= ADC1_START();
  status |= ADC2_START();
  status |= ADC3_START();
  return status;
}
bool ADC1_START(void)
{
  bool status = false;

  if (D_ADC1.Status == D_BLOCK_STOP)
  {
    /* List of data in buffer */
    &adc1_data.out_buffer[0]; // Stock MAP sensor data
    &adc1_data.out_buffer[1]; // Backpressure sensor data [spare]
    &adc1_data.out_buffer[2]; // OX sensor data [spare]
    &adc1_data.out_buffer[3]; // ANALOG_IN_1 input data [spare]
    &adc1_data.out_buffer[4]; // ANALOG_IN_2 input data [spare]
    &adc1_data.out_buffer[5]; // ANALOG_IN_3 input data [spare]
    &adc1_data.out_buffer[6]; // Internal_temperature data
    &adc1_data.out_buffer[7]; // VREF data

    /* Clear status flags */
    adc1_data.status.callback_half = false;
    adc1_data.status.callback_full = false;
    adc1_data.status.overflow = false;

    D_ADC1.Status |= D_BLOCK_INITIALIZED;
  }
  if (D_ADC1.Status & D_BLOCK_INITIALIZED)
  {
    HAL_ADC_Start_DMA(&GENERAL_ADC, (uint32_t *)adc1_data.in_buffer, sizeof(adc1_data.in_buffer) / sizeof(uint16_t));
    D_ADC1.Status |= D_BLOCK_WORKING;
  }

  return status;
}
bool ADC2_START(void)
{
  bool status = false;

  if (D_ADC2.Status == D_BLOCK_STOP)
  {
    /* List of data in buffer */
    &adc2_data.out_buffer[0]; // Speed sensor 1
    &adc2_data.out_buffer[1]; // Speed sensor 2
    &adc2_data.out_buffer[2]; // Speed sensor 3
    &adc2_data.out_buffer[3]; // Speed sensor 4

    /* Clear status flags */
    adc2_data.status.callback_half = false;
    adc2_data.status.callback_full = false;
    adc2_data.status.overflow = false;

    D_ADC2.Status |= D_BLOCK_INITIALIZED;
  }
  if (D_ADC2.Status & D_BLOCK_INITIALIZED)
  {
    HAL_ADC_Start_DMA(&SPEED_ADC, (uint32_t *)adc2_data.in_buffer, sizeof(adc2_data.in_buffer) / sizeof(uint16_t));
    D_ADC2.Status |= D_BLOCK_WORKING;
  }

  return status;
}
bool ADC3_START(void)
{
  bool status = false;
  HAL_StatusTypeDef status_HAL = HAL_OK;

  if (D_ADC3.Status == D_BLOCK_STOP)
  {
    adc3_data.samplingTimer = &KNOCK_ADC_SAMPLING_TIMER;

    /* Clear status flags */
    adc3_data.status.callback_half = false;
    adc3_data.status.callback_full = false;
    adc3_data.status.overflow = false;

    D_ADC3.Status |= D_BLOCK_INITIALIZED;
  }
  if (D_ADC3.Status & D_BLOCK_INITIALIZED)
  {
    status_HAL |= HAL_TIM_Base_Start(adc3_data.samplingTimer);
    HAL_ADC_Start_DMA(&KNOCK_ADC, (uint32_t *)adc3_data.in_buffer, sizeof(adc3_data.in_buffer) / sizeof(uint16_t));
    D_ADC3.Status |= D_BLOCK_WORKING;
  }
  return status || (status_HAL != HAL_OK);
}
/* Stop functions */
void ADC_STOP_ALL(void)
{
  if (D_ADC1.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    ADC1_STOP();
  }
  if (D_ADC2.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    ADC2_STOP();
  }
  if (D_ADC3.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    ADC3_STOP();
  }
}
void ADC1_STOP(void)
{
  HAL_ADC_Stop_DMA(&GENERAL_ADC);
  nECU_ADC1_Routine(); // finish routine if flags pending
  D_ADC1.Status = D_BLOCK_STOP;
}
void ADC2_STOP(void)
{
  HAL_ADC_Stop_DMA(&SPEED_ADC);
  nECU_ADC2_Routine(); // finish routine if flags pending
  D_ADC2.Status = D_BLOCK_STOP;
}
void ADC3_STOP(void)
{
  HAL_TIM_Base_Stop(adc3_data.samplingTimer);
  HAL_ADC_Stop_DMA(&KNOCK_ADC);
  D_ADC1.Status = D_BLOCK_STOP;
}
/* ADC Rutines */
void nECU_ADC_All_Routine(void)
{
  /* Remember that all ADC are working simoutainously, all callbacks will be at the same time */
  if (D_ADC1.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    nECU_ADC1_Routine();
  }
  if (D_ADC2.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    nECU_ADC2_Routine();
  }
  if (D_ADC3.Status & D_BLOCK_INITIALIZED_WORKING)
  {
    nECU_ADC3_Routine();
  }
}
void nECU_ADC1_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc1_data.status.callback_half == true)
  {
    nECU_ADC_AverageDMA(&GENERAL_ADC, &(adc1_data.in_buffer[0]), GENERAL_DMA_LEN / 2, adc1_data.out_buffer, GENERAL_SMOOTH_ALPHA);
    adc1_data.status.callback_half = false; // clear flag
  }
  else if (adc1_data.status.callback_full == true)
  {
    nECU_ADC_AverageDMA(&GENERAL_ADC, &(adc1_data.in_buffer[GENERAL_DMA_LEN / 2]), GENERAL_DMA_LEN / 2, adc1_data.out_buffer, GENERAL_SMOOTH_ALPHA);
    adc1_data.status.callback_full = false; // clear flag
  }
  nECU_Debug_ProgramBlockData_Update(&D_ADC1);
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
    nECU_ADC_AverageDMA(&SPEED_ADC, &adc2_data.in_buffer[(SPEED_DMA_LEN / 2) - 1], SPEED_DMA_LEN / 2, adc2_data.out_buffer, SPEED_SMOOTH_ALPHA);
    adc2_data.status.callback_full = false; // clear flag
  }
  nECU_Debug_ProgramBlockData_Update(&D_ADC2);
}
void nECU_ADC3_Routine(void)
{
  /* Conversion Completed callbacks */
  if (adc3_data.status.callback_half == true)
  {
    adc3_data.status.callback_half = false; // clear flag
    nECU_Knock_ADC_Callback(&adc3_data.in_buffer[0]);
  }
  else if (adc3_data.status.callback_full == true)
  {
    adc3_data.status.callback_full = false; // clear flag
    nECU_Knock_ADC_Callback(&adc3_data.in_buffer[(KNOCK_DMA_LEN / 2) - 1]);
  }
  nECU_Debug_ProgramBlockData_Update(&D_ADC3);
#if TEST_KNOCK_UART == true
  Send_Triangle_UART();
  return;
#endif
}

/* pointer get functions */
uint16_t *getPointer_MAP_ADC(void)
{
  return &adc1_data.out_buffer[0];
}
uint16_t *getPointer_Backpressure_ADC(void)
{
  return &adc1_data.out_buffer[1];
}
uint16_t *getPointer_OX_ADC(void)
{
  return &adc1_data.out_buffer[2];
}
uint16_t *getPointer_InternalTemp_ADC(void)
{
  return &adc1_data.out_buffer[6];
}
uint16_t *getPointer_SpeedSens_ADC(Speed_Sensor_ID ID)
{
  if (ID = SPEED_SENSOR_NONE_ID)
  {
    return &adc2_data.out_buffer[0];
  }
  return &adc2_data.out_buffer[0 + ID];
}
uint16_t *getPointer_AnalogInput(nECU_AnalogNumber ID)
{
  if (ID = ANALOG_IN_NONE)
  {
    return &adc1_data.out_buffer[3];
  }
  return &adc1_data.out_buffer[3 + ID];
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

/* ADC buffer operations */
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
void nECU_ADC_expSmooth(uint16_t *inBuf, uint16_t *outBuf, uint16_t inLength, float alpha) // exponential smoothing algorithm for whole buffer
{
  for (size_t i = 0; i < inLength; i++)
  {
    outBuf[i] = (inBuf[i] * alpha) + (outBuf[i] * (1 - alpha));
  }
}