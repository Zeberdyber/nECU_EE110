/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for spi function handling.
 ******************************************************************************
 */
#include "nECU_spi.h"

bool egt_peripheral_working;

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
bool nECU_SPI_getBusy(SPI_HandleTypeDef *hspi) // get state (if busy) of the SPI communication
{
  HAL_SPI_StateTypeDef EGT_CurrentState = HAL_SPI_GetState(&hspi1);
  bool output = false;
  if (EGT_CurrentState >= HAL_SPI_STATE_BUSY && EGT_CurrentState <= HAL_SPI_STATE_BUSY_TX_RX)
  {
    output = true;
  }
  return output;
}
bool nECU_SPI_getError(SPI_HandleTypeDef *hspi) // get error type (if not ready and not busy)
{
  HAL_SPI_StateTypeDef EGT_CurrentState = HAL_SPI_GetState(hspi);
  bool output = false;
  if (EGT_CurrentState == HAL_SPI_STATE_RESET && EGT_CurrentState > HAL_SPI_STATE_BUSY_TX_RX)
  {
    output = true;
  }
  return output;
}

void nECU_SPI_Rx_DMA_Start(GPIO_TypeDef *GPIOx, uint16_t *GPIO_Pin, SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size) // start communication with selected device
{
  if (nECU_SPI_getBusy(hspi) == false) // check if available
  {
    HAL_GPIO_WritePin(GPIOx, *GPIO_Pin, RESET);
    HAL_SPI_Receive_DMA(hspi, pData, Size);
  }
}
void nECU_SPI_Rx_DMA_Stop(GPIO_TypeDef *GPIOx, uint16_t *GPIO_Pin, SPI_HandleTypeDef *hspi) // end communication with selected device
{
  bool ans = nECU_SPI_getBusy(hspi);
  ans = ans;
  if (nECU_SPI_getBusy(hspi) == false) // check if done
  {
    HAL_GPIO_WritePin(GPIOx, *GPIO_Pin, SET);
    HAL_SPI_DMAStop(hspi);
  }
}