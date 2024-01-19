/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          frame forming.
 ******************************************************************************
 */
#include "nECU_spi.h"

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) // called when successfully recived data
{
  EGT_GetSPIData(false); // update EGT sensors
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) // called when error ocured
{
  EGT_GetSPIData(true); // update EGT sensors
}
/* Peripheral state */
bool nECU_SPI_getBusy(void) // get state (if busy) of the SPI communication
{
  HAL_SPI_StateTypeDef CurrentState = HAL_SPI_GetState(&hspi1);
  if (CurrentState >= HAL_SPI_STATE_BUSY && CurrentState <= HAL_SPI_STATE_BUSY_TX_RX)
  {
    return true;
  }
  return false;
}
bool nECU_SPI_getError(void) // get state (if not ready and not busy)
{
  HAL_SPI_StateTypeDef CurrentState = HAL_SPI_GetState(&hspi1);
  if (CurrentState == HAL_SPI_STATE_RESET && CurrentState > HAL_SPI_STATE_BUSY_TX_RX)
  {
    return true;
  }
  return false;
}
