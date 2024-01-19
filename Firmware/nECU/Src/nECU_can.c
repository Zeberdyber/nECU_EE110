/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          timing.
 ******************************************************************************
 */

#include "nECU_can.h"

/* General variables */
bool CAN_Running = false;
uint8_t CAN_Code_Error = 0; // error in user code ex: no valid solution for given input

/* CAN Mailboxes */
uint32_t CAN_TxMailbox_Frame0 = CAN_TX_MAILBOX0;
uint32_t CAN_TxMailbox_Frame1 = CAN_TX_MAILBOX1;
uint32_t CAN_TxMailbox_Frame2 = CAN_TX_MAILBOX2;

/* CAN Headers */
CAN_TxHeaderTypeDef CanTxFrame0;
CAN_TxHeaderTypeDef CanTxFrame1;
CAN_TxHeaderTypeDef CanTxFrame2;
CAN_RxHeaderTypeDef CanRxFrame0;

/* CAN Frame buffers */
uint8_t TxData_Frame0[8];
uint8_t TxData_Frame1[8];
uint8_t TxData_Frame2[8];
uint8_t RxData_Frame0[8];

/* CAN filters */
CAN_FilterTypeDef WheelSetupFilter;
CAN_FilterTypeDef CoolantFilter;

/* CAN flags */
bool Rx_Data_Frame0_Pending = false;

extern uint16_t loopCounter;

// General functions
void nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  nECU_CAN_InitFrame(0);
  nECU_CAN_InitFrame(1);
  nECU_CAN_InitFrame(2);
  nECU_CAN_RX_InitFrame();

  HAL_TIM_Base_Start_IT(&CAN_LOW_PRIORITY_TIMER);  // timing counter
  HAL_TIM_Base_Start_IT(&CAN_HIGH_PRIORITY_TIMER); // timing counter
}
void nECU_CAN_WriteToBuffer(uint8_t frameNumber, uint8_t *TxData_Frame) // copy input data to corresponding frame buffer
{
  switch (frameNumber)
  {
  case 0:
    for (uint8_t i = 0; i < CanTxFrame0.DLC; i++)
    {
      TxData_Frame0[i] = TxData_Frame[i];
    }
    break;
  case 1:
    for (uint8_t i = 0; i < CanTxFrame1.DLC; i++)
    {
      TxData_Frame1[i] = TxData_Frame[i];
    }
    break;
  case 2:
    for (uint8_t i = 0; i < CanTxFrame2.DLC; i++)
    {
      TxData_Frame2[i] = TxData_Frame[i];
    }
    break;

  default:
    CAN_Code_Error = 1;
    break;
  }
}
void nECU_CAN_Stop(void) // stop all CAN code, with timing
{
  HAL_CAN_Stop(&hcan1);
  nECU_CAN_RX_Stop();
  HAL_TIM_Base_Stop_IT(&CAN_LOW_PRIORITY_TIMER);
  HAL_TIM_Base_Stop_IT(&CAN_HIGH_PRIORITY_TIMER);
}

// Communication functions
void nECU_CAN_TimerEvent(TIM_HandleTypeDef *htim) // funtion called after periodic interrupt from timing timers
{
  if (htim == &CAN_LOW_PRIORITY_TIMER) // Timing clock for normal priority CAN messages
  {
    nECU_CAN_TransmitFrame(0);
    nECU_CAN_TransmitFrame(1);
    loopCounter = 0;
  }
  if (htim == &CAN_HIGH_PRIORITY_TIMER) // Timing clock for high priority CAN messages
  {
    nECU_CAN_TransmitFrame(2);
  }
}
void nECU_CAN_InitFrame(uint8_t frameNumber) // initialize header for selected frame
{
  switch (frameNumber)
  {
  case 0:
    CanTxFrame0.IDE = CAN_ID_STD;
    CanTxFrame0.StdId = CAN_TX_FRAME0_ID;
    CanTxFrame0.RTR = CAN_RTR_DATA;
    CanTxFrame0.DLC = 8;
    break;
  case 1:
    CanTxFrame1.IDE = CAN_ID_STD;
    CanTxFrame1.StdId = CAN_TX_FRAME1_ID;
    CanTxFrame1.RTR = CAN_RTR_DATA;
    CanTxFrame1.DLC = 8;
    break;
  case 2:
    CanTxFrame2.IDE = CAN_ID_STD;
    CanTxFrame2.StdId = CAN_TX_FRAME2_ID;
    CanTxFrame2.RTR = CAN_RTR_DATA;
    CanTxFrame2.DLC = 8;
    break;

  default:
    CAN_Code_Error = 1;
    break;
  }
  if (!CAN_Running) // If not running start the peripheral
  {
    if (HAL_CAN_Start(&hcan1) == HAL_OK)
    {
      CAN_Running = true;
    }
  }
}
uint8_t nECU_CAN_TransmitFrame(uint8_t frameNumber) // send selected frame over CAN
{
  if (!CAN_Running && HAL_CAN_Start(&hcan1) == 0) // If not running start the peripheral
  {
    CAN_Running = true;
  }
  else
  {
    switch (frameNumber)
    {
    case 0:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame0) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &CanTxFrame0, TxData_Frame0, &CAN_TxMailbox_Frame0); // Transmit the data
      return 1;
      // }
      break;
    case 1:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame1) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &CanTxFrame1, TxData_Frame1, &CAN_TxMailbox_Frame1); // Transmit the data
      return 1;
      // }
      break;
    case 2:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame2) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &CanTxFrame2, TxData_Frame2, &CAN_TxMailbox_Frame2); // Transmit the data
      return 1;
      // }
      break;
    default:
      CAN_Code_Error = 1;
      break;
    }
  }
  return 0;
}
uint8_t nECU_CAN_IsBusy(void) // Check if any messages are pending
{
  if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame0) || HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame1) || HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame2))
  {
    return 1;
  }
  return 0;
}

// Diagnostic functions
uint8_t nECU_CAN_GetStatus(void) // get current status of nECU CAN
{
  return CAN_Code_Error;
}
bool nECU_CAN_GetState(void) // get data if can periperal buisy
{
  if (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_LISTENING) // RX transmission ongoing
  {
    return true;
  }
  if (HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame0) || HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame1) || HAL_CAN_IsTxMessagePending(&hcan1, CAN_TxMailbox_Frame2)) // message is waiting for TX
  {
    return true;
  }
  return false;
}
bool nECU_CAN_GetError(void) // get error state pf can periperal buisy
{
  HAL_CAN_StateTypeDef CurrentState = HAL_CAN_GetState(&hcan1);
  if (CurrentState >= HAL_CAN_STATE_SLEEP_PENDING || CurrentState == HAL_CAN_STATE_RESET)
  {
    return true;
  }
  return false;
}

// Rx functions
void nECU_CAN_RX_InitFrame(void) // initialize reciving frames with corresponding filters
{
  // Wheel setup and coolant
  WheelSetupFilter.FilterBank = 0;
  WheelSetupFilter.FilterMode = CAN_FILTERMODE_IDMASK;
  WheelSetupFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
  WheelSetupFilter.FilterIdHigh = CAN_RX_WHEELSPEED_ID << 5;
  WheelSetupFilter.FilterIdLow = 0;
  WheelSetupFilter.FilterMaskIdHigh = CAN_RX_WHEELSPEED_ID << 5;
  WheelSetupFilter.FilterMaskIdLow = 0;
  WheelSetupFilter.FilterScale = CAN_FILTERSCALE_32BIT;
  WheelSetupFilter.FilterActivation = ENABLE;
  WheelSetupFilter.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(&hcan1, &WheelSetupFilter);                   // Initialize CAN Filter
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // Initialize CAN Bus Rx Interrupt
}
void nECU_CAN_RX_Stop(void) // Disables Recive communication
{
  HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING); // Stop waiting for RX
  HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO1_MSG_PENDING); // Stop waiting for RX
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // interrupt callback when new Rx frame in FIFO0
{
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CanRxFrame0, RxData_Frame0); // Receive CAN bus message to canRX buffer
  Rx_Data_Frame0_Pending = true;
}
uint8_t *nECU_CAN_getWheelSetupPointer(void) // get pointer to the recived data of wheel setup variable
{
  return &RxData_Frame0[0];
}
uint8_t *nECU_CAN_getCoolantPointer(void) // get pointer to the recived data of coolant variable
{
  return &RxData_Frame0[1];
}
uint8_t *nECU_CAN_getRPMPointer(void) // get pointer to the recived data of RPM variable
{
  return &RxData_Frame0[2];
}
void nECU_CAN_RX_DecodeFrame0(void) // decode data from recived frame
{
  if (Rx_Data_Frame0_Pending == true)
  {
    Rx_Data_Frame0_Pending = false;
  }
}