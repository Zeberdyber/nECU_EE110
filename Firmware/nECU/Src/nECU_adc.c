/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

// local variables
static nECU_ADC1 adc1_data = {0};
static nECU_ADC2 adc2_data = {0};
static nECU_ADC3 adc3_data = {0};

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
bool nECU_ADC1_START(void)
{
  bool status = false;

  if (!nECU_FlowControl_Initialize_Check(D_ADC1))
  {

    /*    List of data in buffer
    &adc1_data.out_buffer[0]; // Stock MAP sensor data
    &adc1_data.out_buffer[1]; // Backpressure sensor data [spare]
    &adc1_data.out_buffer[2]; // OX sensor data [spare]
    &adc1_data.out_buffer[3]; // ANALOG_IN_1 input data [spare]
    &adc1_data.out_buffer[4]; // ANALOG_IN_2 input data [spare]
    &adc1_data.out_buffer[5]; // ANALOG_IN_3 input data [spare]
    &adc1_data.out_buffer[6]; // Internal_temperature data
    &adc1_data.out_buffer[7]; // VREF data
    */

    /* Clear status flags */
    adc1_data.status.callback_half = false;
    adc1_data.status.callback_full = false;
    adc1_data.status.overflow = false;

    if (!status)
      status |= !nECU_FlowControl_Initialize_Do(D_ADC1);
  }
  if (!nECU_FlowControl_Working_Check(D_ADC1) && status == false)
  {
    status |= (HAL_OK != HAL_ADC_Start_DMA(&GENERAL_ADC, (uint32_t *)adc1_data.in_buffer, sizeof(adc1_data.in_buffer) / sizeof(uint16_t)));
    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_ADC1);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC1);

  return status;
}
bool nECU_ADC2_START(void)
{
  bool status = false;

  if (!nECU_FlowControl_Initialize_Check(D_ADC2))
  {
    /* List of data in buffer
    &adc2_data.out_buffer[0]; // Speed sensor 1
    &adc2_data.out_buffer[1]; // Speed sensor 2
    &adc2_data.out_buffer[2]; // Speed sensor 3
    &adc2_data.out_buffer[3]; // Speed sensor 4
    */

    /* Clear status flags */
    adc2_data.status.callback_half = false;
    adc2_data.status.callback_full = false;
    adc2_data.status.overflow = false;

    if (!status)
      status |= !nECU_FlowControl_Initialize_Do(D_ADC2);
  }
  if (!nECU_FlowControl_Working_Check(D_ADC2) && status == false)
  {
    status |= (HAL_OK != HAL_ADC_Start_DMA(&SPEED_ADC, (uint32_t *)adc2_data.in_buffer, sizeof(adc2_data.in_buffer) / sizeof(uint16_t)));
    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_ADC2);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC2);

  return status;
}
bool nECU_ADC3_START(void)
{
  bool status = false;

  if (!nECU_FlowControl_Initialize_Check(D_ADC3) && status == false)
  {
    /* Clear status flags */
    adc3_data.status.callback_half = false;
    adc3_data.status.callback_full = false;
    adc3_data.status.overflow = false;
    status |= nECU_TIM_Init(TIM_ADC_KNOCK_ID);
    if (!status)
      status |= !nECU_FlowControl_Initialize_Do(D_ADC3);
  }
  if (!nECU_FlowControl_Working_Check(D_ADC3))
  {
    status |= nECU_TIM_Base_Start(TIM_ADC_KNOCK_ID);
    status |= (HAL_OK != HAL_ADC_Start_DMA(&KNOCK_ADC, (uint32_t *)adc3_data.in_buffer, sizeof(adc3_data.in_buffer) / sizeof(uint16_t)));
    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_ADC3);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC3);

  return status;
}
/* Stop functions */
bool nECU_ADC1_STOP(void)
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_ADC1) && status == false)
  {
    status |= (HAL_OK != HAL_ADC_Stop_DMA(&GENERAL_ADC));
    nECU_ADC1_Routine(); // finish routine if flags pending
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_ADC1);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC1);

  return status;
}
bool nECU_ADC2_STOP(void)
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_ADC2) && status == false)
  {
    status |= (HAL_OK != HAL_ADC_Stop_DMA(&SPEED_ADC));
    nECU_ADC2_Routine(); // finish routine if flags pending
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_ADC2);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC1);

  return status;
}
bool nECU_ADC3_STOP(void)
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_ADC3) && status == false)
  {
    status |= nECU_TIM_Base_Stop(TIM_ADC_KNOCK_ID);
    status |= (HAL_OK != HAL_ADC_Stop_DMA(&KNOCK_ADC));
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_ADC3);
  }
  if (status)
    nECU_FlowControl_Error_Do(D_ADC1);

  return status;
}
/* ADC Rutines */
void nECU_ADC1_Routine(void)
{
  if (!nECU_FlowControl_Working_Check(D_ADC1)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_ADC1);
    return; // Break
  }

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
  nECU_Debug_ProgramBlockData_Update(D_ADC1);
}
void nECU_ADC2_Routine(void)
{
  if (!nECU_FlowControl_Working_Check(D_ADC2)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_ADC2);
    return; // Break
  }

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
  nECU_Debug_ProgramBlockData_Update(D_ADC2);
}
void nECU_ADC3_Routine(void)
{
  if (!nECU_FlowControl_Working_Check(D_ADC3)) // Check if currently working
  {
    nECU_FlowControl_Error_Do(D_ADC3);
    return; // Break
  }

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
  nECU_Debug_ProgramBlockData_Update(D_ADC3);
#if TEST_KNOCK_UART == true
  Send_Triangle_UART();
  return;
#endif
}

/* pointer get functions */
uint16_t *nECU_ADC1_getPointer(nECU_ADC1_ID ID)
{
  if (ID > ADC1_ID_MAX)
    return NULL;
  return &adc1_data.out_buffer[0 + ID];
}
uint16_t *nECU_ADC2_getPointer(nECU_ADC2_ID ID)
{
  if (ID > ADC2_ID_MAX)
    return NULL;

  return &adc2_data.out_buffer[0 + ID];
}
