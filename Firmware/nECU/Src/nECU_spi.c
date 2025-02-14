/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for spi function handling.
 ******************************************************************************
 */
#include "nECU_spi.h"

static nECU_SPI SPI_Data_List[SPI_ID_MAX] = {0}; // List of all defined SPI
static SPI_HandleTypeDef *SPI_Handle_List[SPI_ID_MAX] = {
    [SPI_EGT_ID] = &hspi1, // peripheral to which egt ICs are connected
}; // Lists handles for each ID

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) // called when successfully recived data
{
  if (hspi == NULL) // break if pointer does not exist
    return;

  nECU_SPI_ID current_ID = nECU_SPI_Identify(hspi);
  if (current_ID >= SPI_ID_MAX) // Break if invalid ID
    return;

  SPI_Data_List[current_ID].CS_pin->State = GPIO_PIN_SET;
  HAL_GPIO_WritePin(SPI_Data_List[current_ID].CS_pin->GPIOx, SPI_Data_List[current_ID].CS_pin->GPIO_Pin, SPI_Data_List[current_ID].CS_pin->State);
  SPI_Data_List[current_ID].data_Pending = true;
  switch (current_ID)
  {
  case SPI_EGT_ID:
    nECU_EGT_Callback(); // update EGT sensors
    break;
  default:
    break;
  }
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) // called when error ocured
{
  if (hspi == NULL) // break if pointer does not exist
    return;

  nECU_SPI_ID current_ID = nECU_SPI_Identify(hspi);
  if (current_ID >= SPI_ID_MAX) // Break if invalid ID
    return;

  SPI_Data_List[current_ID].CS_pin->State = GPIO_PIN_SET;
  HAL_GPIO_WritePin(SPI_Data_List[current_ID].CS_pin->GPIOx, SPI_Data_List[current_ID].CS_pin->GPIO_Pin, SPI_Data_List[current_ID].CS_pin->State);
  SPI_Data_List[current_ID].data_Pending = false;
  switch (current_ID)
  {
  case SPI_EGT_ID:
    nECU_EGT_Error_Callback();
    break;
  default:
    break;
  }
}

void nECU_SPI_Rx_IT_Start(GPIO_struct *pCS_pin, nECU_SPI_ID ID, uint8_t *pData, uint16_t Size) // start communication with selected device
{
  if (ID >= SPI_ID_MAX || pCS_pin == NULL || pData == NULL || Size == 0) // Break if invalid data
    return;

  if (nECU_SPI_getBusy(ID) == false) // check if available
  {
    SPI_Data_List[ID].CS_pin = pCS_pin;
    SPI_Data_List[ID].CS_pin->State = GPIO_PIN_RESET;
    HAL_GPIO_WritePin(SPI_Data_List[ID].CS_pin->GPIOx, SPI_Data_List[ID].CS_pin->GPIO_Pin, SPI_Data_List[ID].CS_pin->State);
    HAL_SPI_Receive_IT(SPI_Handle_List[ID], (uint8_t *)pData, Size);
  }
}
void nECU_SPI_IT_Stop(nECU_SPI_ID ID) // end IT communication with selected device
{
  if (ID >= SPI_ID_MAX) // Break if invalid ID
    return;

  SPI_Data_List[ID].CS_pin->State = GPIO_PIN_SET;
  HAL_GPIO_WritePin(SPI_Data_List[ID].CS_pin->GPIOx, SPI_Data_List[ID].CS_pin->GPIO_Pin, SPI_Data_List[ID].CS_pin->State);
  HAL_SPI_Abort_IT(SPI_Handle_List[ID]);
}

static bool nECU_SPI_getBusy(nECU_SPI_ID ID) // get state (if busy) of the SPI communication
{
  if (ID >= SPI_ID_MAX) // Break if invalid ID
    return false;

  HAL_SPI_StateTypeDef EGT_CurrentState = HAL_SPI_GetState(SPI_Handle_List[ID]);
  bool output = false;
  if (EGT_CurrentState >= HAL_SPI_STATE_BUSY && EGT_CurrentState <= HAL_SPI_STATE_BUSY_TX_RX)
    output = true;

  return output;
}

static nECU_SPI_ID nECU_SPI_Identify(SPI_HandleTypeDef *hspi) // returns ID of given input
{
  if (hspi == NULL) // break if pointer does not exist
    return SPI_ID_MAX;

  for (nECU_SPI_ID current_ID = 0; current_ID < SPI_ID_MAX; current_ID++) // search for handle
  {
    if (hspi == SPI_Handle_List[current_ID]) // check if found
      return current_ID;
  }
  return SPI_ID_MAX; // return ID_MAX if not found
}
SPI_HandleTypeDef *nECU_SPI_getPointer(nECU_SPI_ID ID) // returns pointer to
{
  if (ID >= SPI_ID_MAX) // Break if invalid ID
    return NULL;

  return SPI_Handle_List[ID];
}