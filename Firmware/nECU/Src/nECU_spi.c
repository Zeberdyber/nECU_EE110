/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for spi function handling.
 ******************************************************************************
 */
#include "nECU_spi.h"

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) // called when successfully recived data
{
  if (hspi == &SPI_PERIPHERAL_EGT)
  {
    bool *egt_communication_active = EGT_GetUpdateOngoing();
    if (*egt_communication_active == true)
    {
      EGT_GetSPIData(false); // update EGT sensors
    }
  }
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) // called when error ocured
{
  if (hspi == &SPI_PERIPHERAL_EGT)
  {
    bool *egt_communication_active = EGT_GetUpdateOngoing();
    if (*egt_communication_active == true)
    {
      EGT_GetSPIData(true); // update EGT sensors
    }
  }
}
bool nECU_SPI_getBusy(void) // get state (if busy) of the SPI communication
{
  HAL_SPI_StateTypeDef EGT_CurrentState = HAL_SPI_GetState(&SPI_PERIPHERAL_EGT);
  bool output = false;
  if (EGT_CurrentState >= HAL_SPI_STATE_BUSY && EGT_CurrentState <= HAL_SPI_STATE_BUSY_TX_RX)
  {
    output = true;
  }
  return output;
}
bool nECU_SPI_getError(void) // get state (if not ready and not busy)
{
  HAL_SPI_StateTypeDef EGT_CurrentState = HAL_SPI_GetState(&SPI_PERIPHERAL_EGT);
  bool output = false;
  if (EGT_CurrentState == HAL_SPI_STATE_RESET && EGT_CurrentState > HAL_SPI_STATE_BUSY_TX_RX)
  {
    output = true;
  }
  return output;
}
