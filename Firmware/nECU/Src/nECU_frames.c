/**
 ******************************************************************************
 * @file    nECU_frames.c
 * @brief   This file provides code for data forming and update before can Tx.
 ******************************************************************************
 */

#include "nECU_frames.h"

bool ZeroBool = 0; // Blank bit do not change value

Frame0_struct F0_var;
Frame1_struct F1_var;
Frame2_struct F2_var;

extern nECU_LoopCounter main_loop;

/* Frame 0 */
void Frame0_Init(bool *pTachoShow1, bool *pTachoShow2, bool *pTachoShow3, bool *pAntilag, bool *pTractionOFF, bool *pClearEngineCode, uint16_t *pLunchControlLevel) // initialization of data structure
{
    F0_var.Speed_FL = Speed_GetSpeed(SPEED_SENSOR_FRONT_LEFT);
    F0_var.Speed_FR = Speed_GetSpeed(SPEED_SENSOR_FRONT_RIGHT);
    F0_var.Speed_RL = Speed_GetSpeed(SPEED_SENSOR_REAR_LEFT);
    F0_var.Speed_RR = Speed_GetSpeed(SPEED_SENSOR_REAR_RIGHT);
    F0_var.TachoShow1 = pTachoShow1;
    F0_var.TachoShow2 = pTachoShow2;
    F0_var.TachoShow3 = pTachoShow3;
    F0_var.Antilag = pAntilag;
    F0_var.TractionOFF = pTractionOFF;
    F0_var.ClearEngineCode = pClearEngineCode;
    F0_var.LunchControlLevel = pLunchControlLevel;
    nECU_stock_GPIO_Init();
    F0_var.Cranking = nECU_stock_GPIO_getPointer(INPUT_CRANKING_ID);
    F0_var.Fan_ON = nECU_stock_GPIO_getPointer(INPUT_FAN_ON_ID);
    F0_var.Lights_ON = nECU_stock_GPIO_getPointer(INPUT_LIGHTS_ON_ID);
    F0_var.IgnitionKey = nECU_Immo_getPointer();
}
void Frame0_Update(void) // update variables for frame 0
{
    Speed_TimingEvent();
    nECU_stock_GPIO_update();
    F0_var.LunchControl1 = false;
    F0_var.LunchControl2 = false;
    F0_var.LunchControl3 = false;
    F0_var.RollingLunch = false;
    switch (*F0_var.LunchControlLevel) // Decode lunch controll level
    {
    case 1:
        F0_var.LunchControl1 = true;
        break;
    case 2:
        F0_var.LunchControl1 = true;
        F0_var.LunchControl2 = true;
        break;
    case 3:
        F0_var.LunchControl1 = true;
        F0_var.LunchControl3 = true;
        break;
    case 4:
        F0_var.RollingLunch = true;
        break;
    default:
        break;
    }
}
void Frame0_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    Frame0_Update();
    uint8_t TxFrame[8];
    Frame0_ComposeWord(&TxFrame[0], F0_var.IgnitionKey, F0_var.Fan_ON, F0_var.Lights_ON, F0_var.Cranking, F0_var.Speed_FL);
    Frame0_ComposeWord(&TxFrame[2], F0_var.ClearEngineCode, F0_var.TachoShow3, F0_var.TachoShow2, F0_var.TachoShow1, F0_var.Speed_FR);
    Frame0_ComposeWord(&TxFrame[4], F0_var.Antilag, &F0_var.LunchControl3, &F0_var.LunchControl2, &F0_var.LunchControl1, F0_var.Speed_RL);
    Frame0_ComposeWord(&TxFrame[6], &ZeroBool, &ZeroBool, F0_var.TractionOFF, &F0_var.RollingLunch, F0_var.Speed_RR);
    nECU_CAN_WriteToBuffer(0, TxFrame);
    *F0_var.ClearEngineCode = false;
}
void Frame0_ComposeWord(uint8_t *buffer, bool *B1, bool *B2, bool *B3, bool *B4, uint16_t *Val12Bit) // function to create word for use in frame 0
{
    union Int16ToBytes Converter; // create memory union
    Converter.UintValue = *Val12Bit;

    buffer[1] = Converter.byteArray[0];
    buffer[0] = Converter.byteArray[1] & 0xF;
    buffer[0] |= (*B1 & 1) << 7;
    buffer[0] |= (*B2 & 1) << 6;
    buffer[0] |= (*B3 & 1) << 5;
    buffer[0] |= (*B4 & 1) << 4;
}

/* Frame 1 */
void Frame1_Init(uint8_t *pTachoVal1, uint8_t *pTachoVal2, uint8_t *pTachoVal3, uint16_t *pTuneSelector) // initialization of data structure
{
    F1_var.EGT1 = EGT_GetTemperaturePointer(EGT_CYL1);
    F1_var.EGT2 = EGT_GetTemperaturePointer(EGT_CYL2);
    F1_var.EGT3 = EGT_GetTemperaturePointer(EGT_CYL3);
    F1_var.EGT4 = EGT_GetTemperaturePointer(EGT_CYL4);
    F1_var.TachoVal1 = pTachoVal1;
    F1_var.TachoVal2 = pTachoVal2;
    F1_var.TachoVal3 = pTachoVal3;
    F1_var.TuneSelector = pTuneSelector;
}
void Frame1_Update(void) // update variables for frame 1
{
    EGT_PeriodicEventHP();
}
void Frame1_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    Frame1_Update();
    uint8_t TxFrame[8];
    Frame1_ComposeWord(&TxFrame[0], F1_var.TachoVal1, F1_var.EGT1);
    Frame1_ComposeWord(&TxFrame[2], F1_var.TachoVal2, F1_var.EGT2);
    Frame1_ComposeWord(&TxFrame[4], F1_var.TachoVal3, F1_var.EGT3);
    Frame1_ComposeWord(&TxFrame[6], (uint8_t *)F1_var.TuneSelector, F1_var.EGT4);
    nECU_CAN_WriteToBuffer(1, TxFrame);
    TachoValue_Clear_ShowPending(TACHO_SHOW_1);
    TachoValue_Clear_ShowPending(TACHO_SHOW_2);
    TachoValue_Clear_ShowPending(TACHO_SHOW_3);
}
void Frame1_ComposeWord(uint8_t *buffer, uint8_t *Val6Bit, uint16_t *Val10Bit) // function to create word for use in frame 1
{
    union Int16ToBytes Converter; // create memory union
    Converter.UintValue = *Val10Bit;

    buffer[1] = Converter.byteArray[0];
    buffer[0] = Converter.byteArray[1] & 0x3;
    buffer[0] |= (*Val6Bit << 2) & 0xFC;
}

/* Frame 2 */
void Frame2_Init(uint8_t *pBackpressure, uint8_t *pOX_Val, uint16_t *pMAP_Stock_10bit, uint8_t *pKnock, uint8_t *pVSS) // initialization of data structure
{
    F2_var.Backpressure = pBackpressure;
    F2_var.OX_Val = pOX_Val;
    F2_var.MAP_Stock_10bit = pMAP_Stock_10bit;
    F2_var.Knock = pKnock;
    F2_var.VSS = pVSS;
}
void Frame2_Update(void) // update variables for frame 2
{
    UNUSED(0);
}
void Frame2_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    Frame2_Update();
    uint8_t TxFrame[8];
    union Int16ToBytes Converter; // create memory union

    Converter.UintValue = *F2_var.MAP_Stock_10bit;
    TxFrame[0] = Converter.byteArray[1] & 0x3; // 6bit value left;
    TxFrame[1] = Converter.byteArray[0];
    TxFrame[2] = *F2_var.OX_Val;
    TxFrame[3] = *F2_var.Backpressure;
    TxFrame[4] = *F2_var.Knock;
    TxFrame[5] = *F2_var.VSS;
    Converter.UintValue = (uint16_t)main_loop.counter;
    TxFrame[6] = Converter.byteArray[1]; // spare
    TxFrame[7] = Converter.byteArray[0]; // spare
    nECU_CAN_WriteToBuffer(2, TxFrame);
}
