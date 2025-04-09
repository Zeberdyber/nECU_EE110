/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

static nECU_ADC data_List[HADC_ID_MAX] = {
    [HADC1_ID] = {&hadc1, 150, 0.5},           // 150 samples, 0.5 smoothing
    [HADC2_ID] = {&hadc2, 80, 1.0},            // 80 samples, no smoothing
    [HADC3_ID] = {&hadc3, KNOCK_DMA_LEN, 1.0}, // 512 samples, no smoothing
};
void *ptr = NULL;

/* Interrupt functions */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == NULL) // Check if pointer exist
    return;         // Break

  nECU_HADC_ID hadc_id = nECU_ADC_Identify_hadc(hadc);
  if (hadc_id >= HADC_ID_MAX)
    return;
  bool *flags = (data_List[hadc_id].flags);
  if (flags != NULL) // do if adc identified
  {
    flags[ADC_STATUS_OVERFLOW] = flags[ADC_STATUS_FULL]; // indicate overflow if flag was not processed
    flags[ADC_STATUS_FULL] = true;
  }
}
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == NULL) // Check if pointer exist
    return;         // Break

  nECU_HADC_ID hadc_id = nECU_ADC_Identify_hadc(hadc);
  if (hadc_id >= HADC_ID_MAX)
    return;
  bool *flags = (data_List[hadc_id].flags);
  if (flags != NULL) // do if adc identified
  {
    flags[ADC_STATUS_OVERFLOW] = flags[ADC_STATUS_HALF]; // indicate overflow if flag was not processed
    flags[ADC_STATUS_HALF] = true;
  }
}

bool nECU_ADC_START(nECU_ADC_Sensor_ID ID)
{
  // identify sensor->adc connection
  nECU_HADC_ID hadc = nECU_ADC_Identify_SensorID(ID);
  if (hadc >= HADC_ID_MAX)
    return true;

  bool status = false;
  if (!nECU_FC_Initialize_Check(D_ADC1 + hadc))
  { /* Clear status flags */
    for (nECU_ADC_Status current = 0; current < ADC_STATUS_MAX; current++)
      data_List[hadc].flags[current] = false;
    // input buffer memory allocation
    data_List[hadc].in_buffer.len = data_List[hadc].handle->Init.NbrOfConversion * data_List[hadc].sample_count * sizeof(uint16_t);
    // status |= !nECU_Memory_Create(&data_List[hadc].in_buffer);
    data_List[hadc].in_buffer.Buffer.u16 = nECU_Memory_Create2(data_List[hadc].in_buffer.len);

    // output buffer memory allocation
    data_List[hadc].out_buffer.len = data_List[hadc].handle->Init.NbrOfConversion * sizeof(uint16_t);
    // status |= !nECU_Memory_Create(&data_List[hadc].out_buffer);
    data_List[hadc].out_buffer.Buffer.u16 = nECU_Memory_Create2(data_List[hadc].out_buffer.len);

    if (hadc == HADC3_ID)
      status |= nECU_TIM_Init(TIM_ADC_KNOCK_ID);

    if (!status)
      status |= !nECU_FC_Initialize_Do(D_ADC1 + hadc);
  }
  if (!nECU_FC_Working_Check(D_ADC1 + hadc) && status == false)
  {
    if (hadc == HADC3_ID)
      status |= nECU_TIM_Base_Start(TIM_ADC_KNOCK_ID);

    status |= (HAL_OK != HAL_ADC_Start_DMA((data_List[hadc].handle), data_List[hadc].in_buffer.Buffer.u32, data_List[hadc].in_buffer.len / sizeof(uint32_t)));
    if (!status)
      status |= !nECU_FlowControl_Working_Do(D_ADC1 + hadc);
  }
  if (status)
    nECU_FC_Error_Do(D_ADC1 + hadc);

  return status;
}
bool nECU_ADC_STOP(nECU_ADC_Sensor_ID ID)
{
  // identify sensor->adc connection
  nECU_HADC_ID hadc = nECU_ADC_Identify_SensorID(ID);
  if (hadc >= HADC_ID_MAX)
    return true;

  bool status = false;
  if (nECU_FC_Working_Check(D_ADC1 + hadc) && status == false)
  {
    if (hadc == HADC3_ID)
      status |= nECU_TIM_Base_Stop(TIM_ADC_KNOCK_ID);

    status |= (HAL_OK != HAL_ADC_Stop_DMA((data_List[hadc].handle)));
    nECU_ADC_Routine(ID); // finish routine if flags pending

    if (!status)
      status |= !nECU_FC_Stop_Do(D_ADC1 + hadc);

    if (nECU_FC_Stop_Check(D_ADC1 + hadc))
    {
      // Release memory; done only when STOP was done
      status |= !nECU_Memory_Destroy(&data_List[hadc].in_buffer);
      status |= !nECU_Memory_Destroy(&data_List[hadc].out_buffer);
    }
  }
  if (status)
    nECU_FC_Error_Do(D_ADC1 + hadc);

  return status;
}
bool nECU_ADC_Routine(nECU_ADC_Sensor_ID ID)
{
  // identify sensor->adc connection
  nECU_HADC_ID hadc = nECU_ADC_Identify_SensorID(ID);
  if (hadc >= HADC_ID_MAX)
    return false;
  // Check if currently working
  if (!nECU_FC_Working_Check(D_ADC1 + hadc))
  {
    nECU_FC_Error_Do(D_ADC1 + hadc);
    return false; // Break
  }
  /* Conversion Completed callbacks */
  uint16_t start_index = 0;
  if (data_List[hadc].flags[ADC_STATUS_OVERFLOW])
  {
    // Overflow handling - drop data
    data_List[hadc].flags[ADC_STATUS_FULL] = false;
    data_List[hadc].flags[ADC_STATUS_HALF] = false;
  }

  if (data_List[hadc].flags[ADC_STATUS_FULL])
  {
    data_List[hadc].flags[ADC_STATUS_FULL] = false; // clear flag
    start_index = 0;
  }
  else if (data_List[hadc].flags[ADC_STATUS_HALF])
  {
    data_List[hadc].flags[ADC_STATUS_HALF] = false; // clear flag
    start_index = (data_List[hadc].in_buffer.len / (sizeof(uint16_t) * 2));
  }
  else
    return false; // drop if no new data

  if (hadc != HADC3_ID)
    nECU_ADC_AverageDMA(data_List[hadc], start_index);
  else
    nECU_Knock_ADC_Callback(&data_List[hadc].in_buffer);

  nECU_FC_Timeout_Check(D_ADC1 + hadc);
  return true;
}

static nECU_HADC_ID nECU_ADC_Identify_SensorID(nECU_ADC_Sensor_ID ID) // returns correcr hadc id based on given sensor
{
  if (ID >= ADC_ID_MAX)
    return HADC_ID_MAX; // default

  for (nECU_HADC_ID currentID = HADC1_ID; currentID < HADC_ID_MAX; currentID++)
  {
    if (ID < (data_List[currentID].handle->Init.NbrOfConversion))
      return currentID; // found

    ID -= (data_List[currentID].handle->Init.NbrOfConversion); // subtract number of configured channels
  }

  return HADC_ID_MAX; // default (NOT FOUND)
}
static nECU_HADC_ID nECU_ADC_Identify_hadc(ADC_HandleTypeDef *hadc) // returns ID of given hadc structure pointer
{
  if (hadc == NULL)     // Check if pointer exist
    return HADC_ID_MAX; // Break

  for (nECU_HADC_ID currentID = HADC1_ID; currentID < HADC_ID_MAX; currentID++)
  {
    if (hadc == data_List[currentID].handle)
      return currentID;
  }
  return HADC_ID_MAX;
}
/* pointer get functions */
uint16_t *nECU_ADC_getPointer(nECU_ADC_Sensor_ID ID)
{
  if (ID >= ADC_ID_MAX)
    return NULL; // default

  for (nECU_HADC_ID currentID = HADC1_ID; currentID < HADC_ID_MAX; currentID++)
  {
    if (ID < (data_List[currentID].handle->Init.NbrOfConversion))
    {
      if (data_List[currentID].out_buffer.Buffer.v == NULL)
        return NULL; // Break if no buffer is assigned
      uint16_t *dat = &data_List[currentID].out_buffer.Buffer.u16[ID];
      return dat; // found
    }

    ID -= (data_List[currentID].handle->Init.NbrOfConversion); // subtract number of configured channels
  }
  return NULL; // in case not found
}