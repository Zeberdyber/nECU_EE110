/**
 ******************************************************************************
 * @file    nECU_can.c
 * @brief   This file provides code for user defined can functions and data
 *          timing.
 ******************************************************************************
 */

#include "nECU_can.h"
#include "nECU_tests.h"

/* CAN RX */
static bool Rx_Data_Frame0_Pending = false;
static CAN_FilterTypeDef WheelSetupFilter = {0};
static uint8_t RxData_Frame0[8] = {0};
static CAN_RxHeaderTypeDef CanRxFrame0 = {0};

extern Frame0_struct F0_var;
extern Frame1_struct F1_var;
extern Frame2_struct F2_var;

// General functions
bool nECU_CAN_Start(void) // start periodic transmission of data accroding to the timers
{
  bool status_TX, status_RX = false;

  /* TX */
  if (!nECU_FlowControl_Initialize_Check(D_CAN_TX) && status_TX == false)
  {
    status_TX |= nECU_CAN_TX_InitFrame(nECU_Frame_Speed);
    status_TX |= nECU_CAN_TX_InitFrame(nECU_Frame_EGT);
    status_TX |= nECU_CAN_TX_InitFrame(nECU_Frame_Stock);

    nECU_Delay_Set(&(F0_var.frame_delay), CAN_TX_FRAME0_TIME);
    nECU_Delay_Start(&(F0_var.frame_delay));
    nECU_Delay_Set(&(F1_var.frame_delay), CAN_TX_FRAME1_TIME);
    nECU_Delay_Start(&(F1_var.frame_delay));
    nECU_Delay_Set(&(F2_var.frame_delay), CAN_TX_FRAME2_TIME);
    nECU_Delay_Start(&(F2_var.frame_delay));

    F0_var.can_data.Mailbox = CAN_TX_MAILBOX0;
    F1_var.can_data.Mailbox = CAN_TX_MAILBOX1;
    F2_var.can_data.Mailbox = CAN_TX_MAILBOX2;

    if (!status_TX)
    {
      !nECU_FlowControl_Initialize_Do(D_CAN_TX);
    }
  }
  if (!nECU_FlowControl_Working_Check(D_CAN_TX) && status_TX == false)
  {
    status_TX |= (HAL_CAN_Start(&hcan1) != HAL_OK);
    if (!status_TX)
    {
      status_TX |= !nECU_FlowControl_Working_Do(D_CAN_TX);
    }
  }
  if (status_TX)
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
  }

  /* RX */
  if (!nECU_FlowControl_Initialize_Check(D_CAN_RX) && status_RX == false)
  {
    status_RX |= nECU_CAN_RX_InitFrame();
    if (!status_RX)
    {
      status_RX |= !nECU_FlowControl_Initialize_Do(D_CAN_RX);
    }
  }
  if (!nECU_FlowControl_Working_Check(D_CAN_RX) && status_RX == false)
  {
    status_RX |= (HAL_CAN_Start(&hcan1) != HAL_OK);
    if (!status_RX)
    {
      status_RX |= !nECU_FlowControl_Working_Do(D_CAN_RX);
    }
  }
  if (status_RX)
  {
    nECU_FlowControl_Error_Do(D_CAN_RX);
  }

  return status_TX | status_RX;
}
void nECU_CAN_WriteToBuffer(nECU_CAN_Frame_ID frameID, uint8_t *TxData_Frame) // copy input data to corresponding frame buffer
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
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
    nECU_FlowControl_Error_Do(D_CAN_TX);
    pFrame = NULL;
    return;
  }
  for (uint8_t i = 0; i < pFrame->Header.DLC; i++)
  {
    pFrame->Send_Buffer[i] = TxData_Frame[i];
  }
}
bool nECU_CAN_Stop(void) // stop all CAN code, with timing
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_CAN_TX) && status == false)
  {
    status |= nECU_CAN_RX_Stop();
    status |= (HAL_OK != HAL_CAN_Stop(&hcan1));
    if (!status)
      status |= nECU_FlowControl_Stop_Do(D_CAN_TX);
  }
  return status;
}
void nECU_CAN_CheckTime(void) // checks if it is time to send packet
{
  if (!nECU_FlowControl_Working_Check(D_CAN_TX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    return;
  }

  // update times
  nECU_Delay_Update(&(F0_var.frame_delay));
  nECU_Delay_Update(&(F1_var.frame_delay));
  nECU_Delay_Update(&(F2_var.frame_delay));

  // check if time is above threshold
  if (*(nECU_Delay_DoneFlag(&(F0_var.frame_delay))) && nECU_FlowControl_Working_Check(D_F0))
  {
    nECU_CAN_TX_TransmitFrame(nECU_Frame_Speed);
    nECU_Delay_Start(&(F0_var.frame_delay));
    *F0_var.ClearEngineCode = false;
  }
  if (*(nECU_Delay_DoneFlag(&(F1_var.frame_delay))) && nECU_FlowControl_Working_Check(D_F1))
  {
    nECU_CAN_TX_TransmitFrame(nECU_Frame_EGT);
    nECU_Delay_Start(&(F1_var.frame_delay));
  }
  if (*(nECU_Delay_DoneFlag(&(F2_var.frame_delay))) && nECU_FlowControl_Working_Check(D_F2))
  {
    nECU_CAN_TX_TransmitFrame(nECU_Frame_Stock);
    nECU_Delay_Start(&(F2_var.frame_delay));
  }

  nECU_Debug_ProgramBlockData_Update(D_CAN_TX);
}

// Communication functions
static bool nECU_CAN_TX_InitFrame(nECU_CAN_Frame_ID frameID) // initialize header for selected frame
{
  bool status = false;

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
    nECU_FlowControl_Error_Do(D_CAN_TX);
    pFrame = NULL;
    status |= true;
    return status;
  }

  pFrame->Header.IDE = CAN_ID_STD;
  pFrame->Header.RTR = CAN_RTR_DATA;
  pFrame->Header.DLC = 8; // 8 bytes in length

  return status;
}
static bool nECU_CAN_TX_TransmitFrame(nECU_CAN_Frame_ID frameID) // send selected frame over CAN
{
  bool status = false;
  if (!nECU_FlowControl_Working_Check(D_CAN_TX))
  {
    nECU_FlowControl_Error_Do(D_CAN_TX);
    status |= true;
    return status;
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
    nECU_FlowControl_Error_Do(D_CAN_TX);
    status |= true;
    return status;
  }

  status |= (HAL_CAN_AddTxMessage(&hcan1, &(pFrame->Header), pFrame->Send_Buffer, &(pFrame->Mailbox)) != HAL_OK); // Transmit the data

  return status;
}

// Diagnostic functions
static uint8_t nECU_CAN_IsBusy(void) // Check if any messages are pending
{
  uint8_t result = 0;
  result += HAL_CAN_IsTxMessagePending(&hcan1, F0_var.can_data.Mailbox);
  result += HAL_CAN_IsTxMessagePending(&hcan1, F1_var.can_data.Mailbox);
  result += HAL_CAN_IsTxMessagePending(&hcan1, F2_var.can_data.Mailbox);
  return result;
}
static bool nECU_CAN_GetState(void) // get data if can periperal buisy
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
    nECU_FlowControl_Error_Do(D_CAN_TX);
    nECU_FlowControl_Error_Do(D_CAN_RX);
    return true;
  }
  return false;
}

// Rx functions
static bool nECU_CAN_RX_InitFrame(void) // initialize reciving frames with corresponding filters
{
  bool status = false;

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

  status |= (HAL_CAN_ConfigFilter(&hcan1, &WheelSetupFilter) != HAL_OK);                   // Initialize CAN Filter
  status |= (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK); // Initialize CAN Bus Rx Interrupt

  return status;
}
bool nECU_CAN_RX_Stop(void) // Disables Recive communication
{
  bool status = false;
  if (nECU_FlowControl_Working_Check(D_CAN_RX) && status == false)
  {
    status |= (HAL_OK != HAL_CAN_DeactivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING)); // Stop waiting for RX
    if (!status)
      status |= !nECU_FlowControl_Stop_Do(D_CAN_RX);
  }
  return status;
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) // interrupt callback when new Rx frame in FIFO0
{
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &CanRxFrame0, RxData_Frame0); // Receive CAN bus message to canRX buffer
  Rx_Data_Frame0_Pending = true;
  nECU_Debug_ProgramBlockData_Update(D_CAN_RX);
}

// Pointers
uint8_t *nECU_CAN_getPointer_WheelSetup(void) // get pointer to the recived data of wheel setup variable
{
  return &RxData_Frame0[0];
}
uint8_t *nECU_CAN_getPointer_Coolant(void) // get pointer to the recived data of coolant variable
{
  return &RxData_Frame0[1];
}
uint16_t *nECU_CAN_getPointer_RPM(void) // get pointer to the recived data of RPM variable
{
  return (uint16_t *)&RxData_Frame0[2];
}