/**
 ******************************************************************************
 * @file    nECU_frames.c
 * @brief   This file provides code for data forming and update before can Tx.
 ******************************************************************************
 */

#include "nECU_frames.h"

Frame0_struct F0_var = {0};
Frame1_struct F1_var = {0};
Frame2_struct F2_var = {0};

/* Frame 0 */
bool Frame0_Start(void) // initialization of data structure
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_F0))
    {
        for (Speed_Sensor_ID current_ID = 0; current_ID < SPEED_SENSOR_ID_MAX; current_ID++)
        {
            if (Speed_GetSpeed(current_ID)) // Check if pointer exists
                F0_var.SpeedSensor[current_ID] = Speed_GetSpeed(current_ID);
            else
                status |= true;
        }

        status |= Button_Menu_Init();
        if (!status) // do only if no error
        {
            if (Button_Menu_getPointer_Antilag()) // Check if pointers exist
                F0_var.Antilag = Button_Menu_getPointer_Antilag();
            else
                status |= true;

            if (Button_Menu_getPointer_TractionOFF()) // Check if pointers exist
                F0_var.TractionOFF = Button_Menu_getPointer_TractionOFF();
            else
                status |= true;

            if (Button_Menu_getPointer_ClearEngineCode()) // Check if pointers exist
                F0_var.ClearEngineCode = Button_Menu_getPointer_ClearEngineCode();
            else
                status |= true;

            if (Button_Menu_getPointer_LunchControlLevel()) // Check if pointers exist
                F0_var.LunchControlLevel = Button_Menu_getPointer_LunchControlLevel();
            else
                status |= true;
        }

        status |= nECU_stock_GPIO_Init();
        if (!status) // do only if no error
        {
            if (nECU_stock_GPIO_getPointer(INPUT_CRANKING_ID)) // Check if pointers exist
                F0_var.Cranking = nECU_stock_GPIO_getPointer(INPUT_CRANKING_ID);
            else
                status |= true;

            if (nECU_stock_GPIO_getPointer(INPUT_FAN_ON_ID)) // Check if pointers exist
                F0_var.Fan_ON = nECU_stock_GPIO_getPointer(INPUT_FAN_ON_ID);
            else
                status |= true;

            if (nECU_stock_GPIO_getPointer(INPUT_LIGHTS_ON_ID)) // Check if pointers exist
                F0_var.Lights_ON = nECU_stock_GPIO_getPointer(INPUT_LIGHTS_ON_ID);
            else
                status |= true;
        }

        // add immo init (when implemented)
        F0_var.IgnitionKey = nECU_Immo_getPointer();

        status |= TachoValue_Init_All();
        if (!status) // do only if no error
        {
            for (Tacho_ID current_ID = 0; current_ID < TACHO_ID_MAX; current_ID++)
            {
                if (TachoValue_Get_ShowPointer(current_ID))
                    F0_var.TachoShow[current_ID] = TachoValue_Get_ShowPointer(current_ID);
                else
                    status |= true;
            }
        }

        if (!status)
            status |= !nECU_FlowControl_Initialize_Check(D_F0);
    }
    if (!nECU_FlowControl_Working_Check(D_F0))
    {
        status |= Speed_Start();
        if (!status)
            status |= !nECU_FlowControl_Working_Check(D_F0);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_F0);
    return status;
}
void Frame0_Routine(void) // update variables for frame 0
{
    if (!nECU_FlowControl_Working_Check(D_F0))
    {
        nECU_FlowControl_Error_Do(D_F0);
        return;
    }

    Speed_TimingEvent();
    nECU_stock_GPIO_update();
    TachoValue_Update_All();

    nECU_Debug_ProgramBlockData_Update(D_F0);

    for (LaunchControl_ID current_ID = 0; current_ID < LaunchControl_ID_MAX; current_ID++) // clear memory
    {
        F0_var.LunchControl[current_ID] = 0;
    }

    if (*F0_var.LunchControlLevel > LaunchControl_OFF) // Decode launch control level
    {
        F0_var.LunchControl[LaunchControl_Low] = (*F0_var.LunchControlLevel < LaunchControl_Rolling); // Low is also launch control enable for all levels, must be on except rolling
        F0_var.LunchControl[*F0_var.LunchControlLevel] = true;
    }
}
void Frame0_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_F0))
    {
        nECU_FlowControl_Error_Do(D_F0);
        return;
    }
    Frame0_Routine();
    uint8_t TxFrame[8];
    Frame0_ComposeWord(&TxFrame[0], F0_var.IgnitionKey, F0_var.Fan_ON, F0_var.Lights_ON, F0_var.Cranking, F0_var.SpeedSensor[SPEED_SENSOR_ID_FL]);
    Frame0_ComposeWord(&TxFrame[2], F0_var.ClearEngineCode, F0_var.TachoShow[TACHO_ID_MenuLvl], F0_var.TachoShow[TACHO_ID_LaunchControl], F0_var.TachoShow[TACHO_ID_TuneSelector], F0_var.SpeedSensor[SPEED_SENSOR_ID_FR]);
    Frame0_ComposeWord(&TxFrame[4], F0_var.Antilag, &F0_var.LunchControl[LaunchControl_High], &F0_var.LunchControl[LaunchControl_Medium], &F0_var.LunchControl[LaunchControl_Low], F0_var.SpeedSensor[SPEED_SENSOR_ID_RL]);
    Frame0_ComposeWord(&TxFrame[6], (bool *)false, (bool *)false, F0_var.TractionOFF, &F0_var.LunchControl[LaunchControl_Rolling], F0_var.SpeedSensor[SPEED_SENSOR_ID_RR]);
    nECU_CAN_WriteToBuffer(nECU_Frame_Speed, TxFrame);
}
static void Frame0_ComposeWord(uint8_t *buffer, bool *B1, bool *B2, bool *B3, bool *B4, uint16_t *Val12Bit) // function to create word for use in frame 0
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
bool Frame1_Start(void) // initialization of data structure
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_F1))
    {
        for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++)
        {
            if (nECU_EGT_Temperature_getPointer(current_ID))
                F1_var.EGT[current_ID] = nECU_EGT_Temperature_getPointer(current_ID);
            else
                status |= true;
        }

        status |= TachoValue_Init_All();
        if (!status) // do only if no error
        {
            for (Tacho_ID current_ID = 0; current_ID < TACHO_ID_MAX; current_ID++)
            {
                if (TachoValue_Get_OutputPointer(current_ID))
                    F1_var.TachoVal[current_ID] = TachoValue_Get_OutputPointer(current_ID);
                else
                    status |= true;
            }
        }

        status |= Button_Menu_Init();
        if (!status) // do only if no error
        {
            if (Button_Menu_getPointer_TuneSelector())
                F1_var.TuneSelector = Button_Menu_getPointer_TuneSelector();
            else
                status |= true;
        }

        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_F1);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_F1))
    {
        status |= EGT_Start();
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Check(D_F1);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_F1);
    }
    return status;
}
void Frame1_Routine(void) // update variables for frame 1
{
    if (!nECU_FlowControl_Working_Check(D_F1))
    {
        nECU_FlowControl_Error_Do(D_F1);
        return;
    }
    EGT_RequestUpdate();
    TachoValue_Update_All();

    nECU_Debug_ProgramBlockData_Update(D_F1);
}
void Frame1_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_F1))
    {
        nECU_FlowControl_Error_Do(D_F1);
        return;
    }
    Frame1_Routine();
    uint8_t TxFrame[8];
    Frame1_ComposeWord(&TxFrame[0], F1_var.TachoVal[TACHO_ID_TuneSelector], F1_var.EGT[EGT1_ID]);
    Frame1_ComposeWord(&TxFrame[2], F1_var.TachoVal[TACHO_ID_LaunchControl], F1_var.EGT[EGT2_ID]);
    Frame1_ComposeWord(&TxFrame[4], F1_var.TachoVal[TACHO_ID_MenuLvl], F1_var.EGT[EGT3_ID]);
    Frame1_ComposeWord(&TxFrame[6], (uint8_t *)F1_var.TuneSelector, F1_var.EGT[EGT4_ID]);
    nECU_CAN_WriteToBuffer(nECU_Frame_EGT, TxFrame);
    TachoValue_Clear_ShowPending(TACHO_ID_TuneSelector);
    TachoValue_Clear_ShowPending(TACHO_ID_LaunchControl);
    TachoValue_Clear_ShowPending(TACHO_ID_MenuLvl);
}
static void Frame1_ComposeWord(uint8_t *buffer, uint8_t *Val6Bit, uint16_t *Val10Bit) // function to create word for use in frame 1
{
    union Int16ToBytes Converter; // create memory union
    Converter.UintValue = *Val10Bit;

    buffer[1] = Converter.byteArray[0];
    buffer[0] = Converter.byteArray[1] & 0x3;
    buffer[0] |= (*Val6Bit << 2) & 0xFC;
}

/* Frame 2 */
bool Frame2_Start(void) // initialization of data structure
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_F2))
    {
        status |= nECU_BackPressure_Start();
        if (!status) // do only if no error
        {
            if (nECU_BackPressure_GetPointer())
                F2_var.Backpressure = nECU_BackPressure_GetPointer();
            else
                status |= true;
        }

        status |= nECU_OX_Init();
        if (!status) // do only if no error
        {
            if (nECU_OX_GetPointer())
                F2_var.OX_Val = nECU_OX_GetPointer();
            else
                status |= true;
        }

        status |= nECU_MAP_Start();
        if (!status) // do only if no error
        {
            if (nECU_MAP_GetPointer())
                F2_var.MAP_Stock_10bit = nECU_MAP_GetPointer();
            else
                status |= true;
        }

        status |= nECU_Knock_Start();
        if (!status) // do only if no error
        {
            if (nECU_Knock_GetPointer())
                F2_var.Knock = nECU_Knock_GetPointer();
            else
                status |= true;
        }

        status |= nECU_VSS_Start();
        if (!status) // do only if no error
        {
            if (nECU_Knock_GetPointer())
                F2_var.VSS = nECU_VSS_GetPointer();
            else
                status |= true;
        }

        F2_var.loop_time = nECU_Debug_ProgramBlockData_getPointer_Diff(D_Main);

        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_F2);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_F2))
    {
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Check(D_F2);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_F2);
    }

    return status;
}
void Frame2_Routine(void) // update variables for frame 2
{
    if (!nECU_FlowControl_Working_Check(D_F2))
    {
        nECU_FlowControl_Error_Do(D_F2);
        return;
    }
    nECU_BackPressure_Routine();
    nECU_MAP_Routine();
    nECU_OX_Update();
    nECU_VSS_Update();

    nECU_Debug_ProgramBlockData_Update(D_F2);
}
void Frame2_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_F2))
    {
        nECU_FlowControl_Error_Do(D_F2);
        return;
    }

    Frame2_Routine();
    uint8_t TxFrame[8];
    union Int16ToBytes Converter; // create memory union

    Converter.UintValue = *F2_var.MAP_Stock_10bit + FRAME_MAP_OFFSET;
    if (Converter.UintValue > MAX_VAL_10BIT) // round if out of bound
    {
        Converter.UintValue = MAX_VAL_10BIT;
    }
    TxFrame[0] = Converter.byteArray[1] & 0x3; // 6bit value left;
    TxFrame[1] = Converter.byteArray[0];
    TxFrame[2] = *F2_var.OX_Val;
    TxFrame[3] = *F2_var.Backpressure;
    TxFrame[4] = *F2_var.Knock;
    TxFrame[5] = *F2_var.VSS;
    Converter.UintValue = (uint16_t)*F2_var.loop_time;
    TxFrame[6] = Converter.byteArray[1]; // spare
    TxFrame[7] = Converter.byteArray[0]; // spare
    nECU_CAN_WriteToBuffer(nECU_Frame_Stock, TxFrame);
}