/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          timing.
 ******************************************************************************
 */

#include "nECU_can.h"
#include "nECU_tests.h"

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

extern Frame0_struct F0_var;
extern Frame1_struct F1_var;
extern Frame2_struct F2_var;

extern nECU_ProgramBlockData D_CAN_TX, D_CAN_RX; // diagnostic and flow control data
extern nECU_ProgramBlockData D_F0, D_F1, D_F2;   // diagnostic and flow control data

// General functions
void nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  if (D_CAN_TX.Status == D_BLOCK_STOP)
  {
    nECU_CAN_InitFrame(nECU_Frame_Speed);
    nECU_CAN_InitFrame(nECU_Frame_EGT);
    nECU_CAN_InitFrame(nECU_Frame_Stock);
    nECU_CAN_RX_InitFrame();

    nECU_TickTrack_Init(&(F0_var.timer));
    F0_var.timeElapsed = 0;
    nECU_TickTrack_Init(&(F1_var.timer));
    F1_var.timeElapsed = 0;
    nECU_TickTrack_Init(&(F2_var.timer));
    F2_var.timeElapsed = 0;

    F0_var.can_data.Mailbox = CAN_TX_MAILBOX0;
    F1_var.can_data.Mailbox = CAN_TX_MAILBOX1;
    F2_var.can_data.Mailbox = CAN_TX_MAILBOX2;

    D_CAN_TX.Status |= D_BLOCK_INITIALIZED;
  }
  if (D_CAN_TX.Status & D_BLOCK_INITIALIZED)
  {
    HAL_CAN_Start(&hcan1);
    D_CAN_TX.Status |= D_BLOCK_WORKING;
  }
}
void nECU_CAN_WriteToBuffer(nECU_CAN_Frame_ID frameID, uint8_t *TxData_Frame) // copy input data to corresponding frame buffer
{
  if (!(D_CAN_TX.Status & D_BLOCK_WORKING))
  {
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return;
  }
  nECU_CAN_TxFrame *pFrame;
  switch (frameID)
  {
  case nECU_Frame_Speed:
    pFrame = &F0_var.can_data;
    break;
  case nECU_Frame_EGT:
    pFrame = &F1_var.can_data;
    break;
  case nECU_Frame_Stock:
    pFrame = &F2_var.can_data;
    break;
  default:
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return;
  }
  for (uint8_t i = 0; i < pFrame->Header.DLC; i++)
  {
    pFrame->Send_Buffer[i] = TxData_Frame[i];
  }
}
void nECU_CAN_Stop(void) // stop all CAN code, with timing
{
  HAL_CAN_Stop(&hcan1);
  nECU_CAN_RX_Stop();
  D_CAN_TX.Status = D_BLOCK_STOP;
}

// Communication functions
void nECU_CAN_CheckTime(void) // checks if it is time to send packet
{
  if (!(D_CAN_TX.Status & D_BLOCK_WORKING))
  {
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return;
  }

  // update times
  nECU_TickTrack_Update(&(F0_var.timer));
  nECU_TickTrack_Update(&(F1_var.timer));
  nECU_TickTrack_Update(&(F2_var.timer));

  // add time to elapsed
  F0_var.timeElapsed += F0_var.timer.difference * F0_var.timer.convFactor;
  F1_var.timeElapsed += F1_var.timer.difference * F1_var.timer.convFactor;
  F2_var.timeElapsed += F2_var.timer.difference * F2_var.timer.convFactor;

  // check if time is above threshold
  if (F0_var.timeElapsed >= CAN_TX_FRAME0_TIME && (D_F0.Status & D_BLOCK_WORKING))
  {
    nECU_CAN_TransmitFrame(nECU_Frame_Speed);
    F0_var.timeElapsed = 0;
    *F0_var.ClearEngineCode = false;
  }
  if (F1_var.timeElapsed >= CAN_TX_FRAME1_TIME && (D_F1.Status & D_BLOCK_WORKING))
  {
    nECU_CAN_TransmitFrame(nECU_Frame_EGT);
    F1_var.timeElapsed = 0;
  }
  if (F2_var.timeElapsed >= CAN_TX_FRAME2_TIME && (D_F2.Status & D_BLOCK_WORKING))
  {
    nECU_CAN_TransmitFrame(nECU_Frame_Stock);
    F2_var.timeElapsed = 0;
    nECU_mainLoop_Reset();
    nECU_VSS_Update();
  }
}
void nECU_CAN_InitFrame(nECU_CAN_Frame_ID frameID) // initialize header for selected frame
{
  if (D_CAN_TX.Status & D_BLOCK_WORKING) // break if CAN is already working
  {
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return;
  }

  nECU_CAN_TxFrame *pFrame;

  switch (frameID)
  {
  case nECU_Frame_Speed:
    pFrame = &F0_var.can_data;
    pFrame->Header.StdId = CAN_TX_FRAME0_ID;
    break;
  case nECU_Frame_EGT:
    pFrame = &F1_var.can_data;
    pFrame->Header.StdId = CAN_TX_FRAME1_ID;
    break;
  case nECU_Frame_Stock:
    pFrame = &F2_var.can_data;
    pFrame->Header.StdId = CAN_TX_FRAME2_ID;
    break;

  default:
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return;
  }

  pFrame->Header.IDE = CAN_ID_STD;
  pFrame->Header.RTR = CAN_RTR_DATA;
  pFrame->Header.DLC = 8; // 8 bytes in length
}
uint8_t nECU_CAN_TransmitFrame(nECU_CAN_Frame_ID frameID) // send selected frame over CAN
{
  if (!(D_CAN_TX.Status & D_BLOCK_WORKING))
  {
    D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
    return 0;
  }
  if (D_CAN_TX.Status & D_BLOCK_WORKING)
  {
    nECU_CAN_TxFrame *pFrame;
    switch (frameID)
    {
    case nECU_Frame_Speed:
      pFrame = &F0_var.can_data;
      break;
    case nECU_Frame_EGT:
      pFrame = &F1_var.can_data;
      break;
    case nECU_Frame_Stock:
      pFrame = &F2_var.can_data;
      break;
    default:
      D_CAN_TX.Status |= D_BLOCK_CODE_ERROR;
      return 0;
    }
    HAL_CAN_AddTxMessage(&hcan1, &(pFrame->Header), pFrame->Send_Buffer, &(pFrame->Mailbox)); // Transmit the data
    return 1;
  }
  return 0;
}
uint8_t nECU_CAN_IsBusy(void) // Check if any messages are pending
{
  uint8_t result = 0;
  result += HAL_CAN_IsTxMessagePending(&hcan1, F0_var.can_data.Mailbox);
  result += HAL_CAN_IsTxMessagePending(&hcan1, F1_var.can_data.Mailbox);
  result += HAL_CAN_IsTxMessagePending(&hcan1, F2_var.can_data.Mailbox);
  return result;
}

// Diagnostic functions
bool nECU_CAN_GetState(void) // get data if can periperal buisy
{
  if (HAL_CAN_GetState(&hcan1) == HAL_CAN_STATE_LISTENING || nECU_CAN_IsBusy()) // check RX/TX transmission ongoing
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
    D_CAN_TX.Status |= D_BLOCK_ERROR;
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
  // nECU_IGF_Test();
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