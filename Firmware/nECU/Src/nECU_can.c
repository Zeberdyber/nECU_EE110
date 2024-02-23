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

/* RX variables */
/* CAN Headers */
CAN_RxHeaderTypeDef CanRxFrame0;
/* CAN Frame buffers */
uint8_t RxData_Frame0[8];
/* CAN filters */
CAN_FilterTypeDef WheelSetupFilter;
CAN_FilterTypeDef CoolantFilter;
/* CAN flags */
bool Rx_Data_Frame0_Pending = false;

extern uint16_t loopCounter;
extern Frame0_struct F0_var;
extern Frame1_struct F1_var;
extern Frame2_struct F2_var;

// General functions
void nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  nECU_CAN_InitFrame(nECU_Frame_Speed);
  nECU_CAN_InitFrame(nECU_Frame_EGT);
  nECU_CAN_InitFrame(nECU_Frame_Stock);
  nECU_CAN_RX_InitFrame();

  F0_var.send_timing.htim = &CAN_LOW_PRIORITY_TIMER;
  F1_var.send_timing.htim = &CAN_LOW_PRIORITY_TIMER;
  F2_var.send_timing.htim = &CAN_HIGH_PRIORITY_TIMER;

  nECU_tim_Init_struct(&(F0_var.send_timing));
  nECU_tim_Init_struct(&(F1_var.send_timing));
  nECU_tim_Init_struct(&(F2_var.send_timing));

  nECU_tim_base_start(&(F0_var.send_timing));
  nECU_tim_base_start(&(F1_var.send_timing));
  nECU_tim_base_start(&(F2_var.send_timing));

  F0_var.can_data.Mailbox = CAN_TX_MAILBOX0;
  F1_var.can_data.Mailbox = CAN_TX_MAILBOX1;
  F2_var.can_data.Mailbox = CAN_TX_MAILBOX2;
}
void nECU_CAN_WriteToBuffer(nECU_CAN_Frame_ID frameID, uint8_t *TxData_Frame) // copy input data to corresponding frame buffer
{
  switch (frameID)
  {
  case nECU_Frame_Speed:
    for (uint8_t i = 0; i < F0_var.can_data.Header.DLC; i++)
    {
      F0_var.can_data.Send_Buffer[i] = TxData_Frame[i];
    }
    break;
  case nECU_Frame_EGT:
    for (uint8_t i = 0; i < F1_var.can_data.Header.DLC; i++)
    {
      F1_var.can_data.Send_Buffer[i] = TxData_Frame[i];
    }
    break;
  case nECU_Frame_Stock:
    for (uint8_t i = 0; i < F2_var.can_data.Header.DLC; i++)
    {
      F2_var.can_data.Send_Buffer[i] = TxData_Frame[i];
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

  nECU_tim_base_stop(&(F0_var.send_timing));
  nECU_tim_base_stop(&(F1_var.send_timing));
  nECU_tim_base_stop(&(F2_var.send_timing));
}

// Communication functions
void nECU_CAN_TimerEvent(TIM_HandleTypeDef *htim) // funtion called after periodic interrupt from timing timers
{
  if (htim == &CAN_LOW_PRIORITY_TIMER) // Timing clock for normal priority CAN messages
  {
    nECU_CAN_TransmitFrame(nECU_Frame_Speed);
    nECU_CAN_TransmitFrame(nECU_Frame_EGT);
    loopCounter = 0;
  }
  if (htim == &CAN_HIGH_PRIORITY_TIMER) // Timing clock for high priority CAN messages
  {
    nECU_CAN_TransmitFrame(nECU_Frame_Stock);
  }
}
void nECU_CAN_InitFrame(nECU_CAN_Frame_ID frameID) // initialize header for selected frame
{
  switch (frameID)
  {
  case nECU_Frame_Speed:
    F0_var.can_data.Header.IDE = CAN_ID_STD;
    F0_var.can_data.Header.StdId = CAN_TX_FRAME0_ID;
    F0_var.can_data.Header.RTR = CAN_RTR_DATA;
    F0_var.can_data.Header.DLC = 8;
    break;
  case nECU_Frame_EGT:
    F1_var.can_data.Header.IDE = CAN_ID_STD;
    F1_var.can_data.Header.StdId = CAN_TX_FRAME1_ID;
    F1_var.can_data.Header.RTR = CAN_RTR_DATA;
    F1_var.can_data.Header.DLC = 8;
    break;
  case nECU_Frame_Stock:
    F2_var.can_data.Header.IDE = CAN_ID_STD;
    F2_var.can_data.Header.StdId = CAN_TX_FRAME2_ID;
    F2_var.can_data.Header.RTR = CAN_RTR_DATA;
    F2_var.can_data.Header.DLC = 8;
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
uint8_t nECU_CAN_TransmitFrame(nECU_CAN_Frame_ID frameID) // send selected frame over CAN
{
  if (!CAN_Running && HAL_CAN_Start(&hcan1) == 0) // If not running start the peripheral
  {
    CAN_Running = true;
  }
  else
  {
    switch (frameID)
    {
    case nECU_Frame_Speed:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, &(F0_var.can_data.Mailbox)) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &(F0_var.can_data.Header), F0_var.can_data.Send_Buffer, &(F0_var.can_data.Mailbox)); // Transmit the data
      return 1;
      // }
      break;
    case nECU_Frame_EGT:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, &(F1_var.can_data.Mailbox)) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &F1_var.can_data.Header, F1_var.can_data.Send_Buffer, &(F1_var.can_data.Mailbox)); // Transmit the data
      return 1;
      // }
      break;
    case nECU_Frame_Stock:
      // if (HAL_CAN_IsTxMessagePending(&hcan1, &(F2_var.can_data.Mailbox)) == 0) // Check if CAN mailbox empty, then send
      // {
      HAL_CAN_AddTxMessage(&hcan1, &F2_var.can_data.Header, F2_var.can_data.Send_Buffer, &(F2_var.can_data.Mailbox)); // Transmit the data
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
  if (HAL_CAN_IsTxMessagePending(&hcan1, F0_var.can_data.Mailbox) || HAL_CAN_IsTxMessagePending(&hcan1, F1_var.can_data.Mailbox) || HAL_CAN_IsTxMessagePending(&hcan1, F2_var.can_data.Mailbox))
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
  if (HAL_CAN_IsTxMessagePending(&hcan1, F0_var.can_data.Mailbox) || HAL_CAN_IsTxMessagePending(&hcan1, F1_var.can_data.Mailbox) || HAL_CAN_IsTxMessagePending(&hcan1, F2_var.can_data.Mailbox)) // message is waiting for TX
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