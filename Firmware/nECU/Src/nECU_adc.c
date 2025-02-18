/**
 ******************************************************************************
 * @file    nECU_adc.c
 * @brief   This file provides code for user defined ADC functions.
 ******************************************************************************
 */

#include "nECU_adc.h"

// #define APB2_CLOCK 42000000 // APB2 clock speed
#define ADC1_CH_COUNT 8
#define ADC2_CH_COUNT 4
#define ADC3_CH_COUNT 1

static uint16_t DMA_buffer_ADC1[150 * ADC1_CH_COUNT] = {0};           // 150 samples per channel
static uint16_t DMA_buffer_ADC2[75 * ADC2_CH_COUNT] = {0};            // 75 samples per channel
static uint16_t DMA_buffer_ADC3[KNOCK_DMA_LEN * ADC3_CH_COUNT] = {0}; // single channel
static uint16_t out_buffer[ADC1_CH_COUNT + ADC2_CH_COUNT] = {0};      // output buffer (ADC3 not connected)

static nECU_ADC data_List[HADC_ID_MAX] = {
    [HADC1_ID] = {
        {DMA_buffer_ADC1, (uint8_t)(sizeof(DMA_buffer_ADC1) / sizeof(DMA_buffer_ADC1[0]))}, // in buffer (DMA)
        {out_buffer, ADC1_CH_COUNT},                                                        // out buffer (average)
    },
    [HADC2_ID] = {
        {DMA_buffer_ADC2, (uint8_t)(sizeof(DMA_buffer_ADC2) / sizeof(DMA_buffer_ADC2[0]))}, // in buffer (DMA)
        {&out_buffer[ADC2_CH_COUNT - 1], ADC2_CH_COUNT},                                    // out buffer (average)
    },
    [HADC3_ID] = {
        {DMA_buffer_ADC3, (uint8_t)(sizeof(DMA_buffer_ADC3) / sizeof(DMA_buffer_ADC3[0]))}, // in buffer (DMA)
        {(void *)NULL, ADC3_CH_COUNT},                                                      // out buffer (average)
    },
};
static ADC_HandleTypeDef *hadc_List[HADC_ID_MAX] = {
    [HADC1_ID] = &hadc1,
    [HADC2_ID] = &hadc2,
    [HADC3_ID] = &hadc3,
};
static const float DMA_Smoothing[HADC_ID_MAX] = {
    [HADC1_ID] = 0.5,
    [HADC2_ID] = 0.8,
    [HADC3_ID] = 1.0,
};

/* Interrupt functions */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
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

    if (hadc == HADC3_ID)
      status |= nECU_TIM_Init(TIM_ADC_KNOCK_ID);

    if (!status)
      status |= !nECU_FC_Initialize_Do(D_ADC1 + hadc);
  }
  if (!nECU_FC_Working_Check(D_ADC1 + hadc) && status == false)
  {
    if (hadc == HADC3_ID)
      status |= nECU_TIM_Base_Start(TIM_ADC_KNOCK_ID);

    status |= (HAL_OK != HAL_ADC_Start_DMA((hadc_List[hadc]), (uint32_t *)data_List[hadc].in_buffer.Buffer, data_List[hadc].in_buffer.len));
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

    status |= (HAL_OK != HAL_ADC_Stop_DMA((hadc_List[hadc])));
    nECU_ADC_Routine(ID); // finish routine if flags pending
    if (!status)
      status |= !nECU_FC_Stop_Do(D_ADC1 + hadc);
  }
  if (status)
    nECU_FC_Error_Do(D_ADC1 + hadc);

  return status;
}
void nECU_ADC_Routine(nECU_ADC_Sensor_ID ID)
{
  // identify sensor->adc connection
  nECU_HADC_ID hadc = nECU_ADC_Identify_SensorID(ID);
  if (hadc >= HADC_ID_MAX)
    return;
  // Check if currently working
  if (!nECU_FC_Working_Check(D_ADC1 + hadc))
  {
    nECU_FC_Error_Do(D_ADC1 + hadc);
    return; // Break
  }
  /* Conversion Completed callbacks */
  uint16_t start_index = 0;
  if (data_List[hadc].flags[ADC_STATUS_FULL])
  {
    data_List[hadc].flags[ADC_STATUS_FULL] = false; // clear flag
    start_index = 0;
  }
  else if (data_List[hadc].flags[ADC_STATUS_HALF])
  {
    data_List[hadc].flags[ADC_STATUS_HALF] = false; // clear flag
    start_index = (data_List[hadc].in_buffer.len / 2) - 1;
  }
  else
    return; // drop if no new data

  if (hadc != HADC3_ID)
    nECU_ADC_AverageDMA(hadc_List[hadc], &data_List[hadc].in_buffer.Buffer[start_index], (data_List[hadc].in_buffer.len / 2), (data_List[hadc].out_buffer.Buffer), DMA_Smoothing[hadc]);
  else
    nECU_Knock_ADC_Callback(data_List[hadc].in_buffer.Buffer);

  nECU_FC_Timeout_Check(D_ADC1 + hadc);
}

static nECU_HADC_ID nECU_ADC_Identify_SensorID(nECU_ADC_Sensor_ID ID) // returns correcr hadc id based on given sensor
{
  if (ID >= ADC_ID_MAX)
    return HADC_ID_MAX; // default

  if (ID < ADC1_CH_COUNT)
    return HADC1_ID;
  else if (ID < (ADC1_CH_COUNT + ADC2_CH_COUNT))
    return HADC2_ID;
  else if (ID < (ADC1_CH_COUNT + ADC2_CH_COUNT + ADC3_CH_COUNT))
    return HADC3_ID;

  return HADC_ID_MAX; // default
}
static nECU_HADC_ID nECU_ADC_Identify_hadc(ADC_HandleTypeDef *hadc) // returns ID of given hadc structure pointer
{
  for (nECU_HADC_ID currentID = HADC1_ID; currentID < HADC_ID_MAX; currentID++)
  {
    if (hadc == hadc_List[currentID])
      return currentID;
  }
  return HADC_ID_MAX;
}
/* pointer get functions */
uint16_t *nECU_ADC_getPointer(nECU_ADC_Sensor_ID ID)
{
  // identify sensor->adc connection
  nECU_HADC_ID hadc = nECU_ADC_Identify_SensorID(ID);
  if (hadc >= HADC_ID_MAX)
    return NULL;

  return &out_buffer[ID];
}