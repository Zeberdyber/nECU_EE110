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

static bool F0_Initialized = false, F0_Working = false;
static bool F1_Initialized = false, F1_Working = false;
static bool F2_Initialized = false, F2_Working = false;

/* Frame 0 */
void Frame0_Init(void) // initialization of data structure
{
    if (F0_Initialized == false)
    {
        Speed_Start();
        F0_var.Speed_FL = Speed_GetSpeed(SPEED_SENSOR_FRONT_LEFT);
        F0_var.Speed_FR = Speed_GetSpeed(SPEED_SENSOR_FRONT_RIGHT);
        F0_var.Speed_RL = Speed_GetSpeed(SPEED_SENSOR_REAR_LEFT);
        F0_var.Speed_RR = Speed_GetSpeed(SPEED_SENSOR_REAR_RIGHT);

        TachoValue_Init_All();
        F0_var.TachoShow1 = TachoValue_Get_ShowPointer(TACHO_SHOW_1);
        F0_var.TachoShow2 = TachoValue_Get_ShowPointer(TACHO_SHOW_2);
        F0_var.TachoShow3 = TachoValue_Get_ShowPointer(TACHO_SHOW_3);

        Button_Menu_Init();
        F0_var.Antilag = Button_Menu_getPointer_Antilag();
        F0_var.TractionOFF = Button_Menu_getPointer_TractionOFF();
        F0_var.ClearEngineCode = Button_Menu_getPointer_ClearEngineCode();
        F0_var.LunchControlLevel = Button_Menu_getPointer_LunchControlLevel();

        nECU_stock_GPIO_Init();
        F0_var.Cranking = nECU_stock_GPIO_getPointer(INPUT_CRANKING_ID);
        F0_var.Fan_ON = nECU_stock_GPIO_getPointer(INPUT_FAN_ON_ID);
        F0_var.Lights_ON = nECU_stock_GPIO_getPointer(INPUT_LIGHTS_ON_ID);

        // add immo init (when implemented)
        F0_var.IgnitionKey = nECU_Immo_getPointer();

        F0_Initialized = true;
    }
    if (F0_Working == false && F0_Initialized == true)
    {
        Speed_Start();
        F0_Working = true;
    }
}
void Frame0_Update(void) // update variables for frame 0
{
    Speed_TimingEvent();
    nECU_stock_GPIO_update();
    TachoValue_Update_All();

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
    if (F0_Working == false)
    {
        return;
    }

    Frame0_Update();
    uint8_t TxFrame[8];
    Frame0_ComposeWord(&TxFrame[0], F0_var.IgnitionKey, F0_var.Fan_ON, F0_var.Lights_ON, F0_var.Cranking, F0_var.Speed_FL);
    Frame0_ComposeWord(&TxFrame[2], F0_var.ClearEngineCode, F0_var.TachoShow3, F0_var.TachoShow2, F0_var.TachoShow1, F0_var.Speed_FR);
    Frame0_ComposeWord(&TxFrame[4], F0_var.Antilag, &F0_var.LunchControl3, &F0_var.LunchControl2, &F0_var.LunchControl1, F0_var.Speed_RL);
    Frame0_ComposeWord(&TxFrame[6], &ZeroBool, &ZeroBool, F0_var.TractionOFF, &F0_var.RollingLunch, F0_var.Speed_RR);
    nECU_CAN_WriteToBuffer(nECU_Frame_Speed, TxFrame);
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
bool Frame0_Working(void) // return if frame is operational
{
    return F0_Working;
}

/* Frame 1 */
void Frame1_Init(void) // initialization of data structure
{
    if (F1_Initialized == false)
    {
        EGT_Init();
        F1_var.EGT1 = EGT_GetTemperaturePointer(EGT_CYL1);
        F1_var.EGT2 = EGT_GetTemperaturePointer(EGT_CYL2);
        F1_var.EGT3 = EGT_GetTemperaturePointer(EGT_CYL3);
        F1_var.EGT4 = EGT_GetTemperaturePointer(EGT_CYL4);

        TachoValue_Init_All();
        F1_var.TachoVal1 = TachoValue_Get_OutputPointer(TACHO_SHOW_1);
        F1_var.TachoVal2 = TachoValue_Get_OutputPointer(TACHO_SHOW_2);
        F1_var.TachoVal3 = TachoValue_Get_OutputPointer(TACHO_SHOW_3);

        Button_Menu_Init();
        F1_var.TuneSelector = Button_Menu_getPointer_TuneSelector();

        F1_Initialized = true;
    }
    if (F1_Working == false && F1_Initialized == true)
    {
        EGT_Start();
        F1_Working = true;
    }
}
void Frame1_Update(void) // update variables for frame 1
{
    EGT_RequestUpdate();
    TachoValue_Update_All();
}
void Frame1_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (F1_Working == false)
    {
        return;
    }

    Frame1_Update();
    uint8_t TxFrame[8];
    Frame1_ComposeWord(&TxFrame[0], F1_var.TachoVal1, F1_var.EGT1);
    Frame1_ComposeWord(&TxFrame[2], F1_var.TachoVal2, F1_var.EGT2);
    Frame1_ComposeWord(&TxFrame[4], F1_var.TachoVal3, F1_var.EGT3);
    Frame1_ComposeWord(&TxFrame[6], (uint8_t *)F1_var.TuneSelector, F1_var.EGT4);
    nECU_CAN_WriteToBuffer(nECU_Frame_EGT, TxFrame);
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
bool Frame1_Working(void) // return if frame is operational
{
    return F1_Working;
}

/* Frame 2 */
void Frame2_Init(void) // initialization of data structure
{
    if (F2_Initialized == false)
    {
        nECU_BackPressure_Init();
        F2_var.Backpressure = nECU_BackPressure_GetPointer();
        nECU_OX_Init();
        F2_var.OX_Val = nECU_OX_GetPointer();
        nECU_MAP_Init();
        F2_var.MAP_Stock_10bit = nECU_MAP_GetPointer();
        nECU_Knock_Init();
        F2_var.Knock = nECU_Knock_GetPointer();
        nECU_VSS_Init();
        F2_var.VSS = nECU_VSS_GetPointer();
        nECU_mainLoop_Init();
        F2_var.loop_count = nECU_mainLoop_getValue();
        F2_Initialized = true;
    }
    if (F2_Working == false && F2_Initialized == true)
    {
        ADC1_START();
        nECU_Stock_Start();
        F2_Working = true;
    }
}
void Frame2_Update(void) // update variables for frame 2
{
    nECU_BackPressure_Update();
    nECU_MAP_Update();
    nECU_OX_Update();
    nECU_VSS_Update();
    nECU_mainLoop_Update();
}
void Frame2_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (F2_Working == false)
    {
        return;
    }

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
    Converter.UintValue = (uint16_t)*F2_var.loop_count;
    TxFrame[6] = Converter.byteArray[1]; // spare
    TxFrame[7] = Converter.byteArray[0]; // spare
    nECU_CAN_WriteToBuffer(nECU_Frame_Stock, TxFrame);
}
bool Frame2_Working(void) // return if frame is operational
{
    return F2_Working;
}