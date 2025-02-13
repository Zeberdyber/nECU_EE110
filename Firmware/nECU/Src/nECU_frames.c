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

    if (!nECU_FlowControl_Initialize_Check(D_Frame_Speed_ID))
    {
        // Start speed sensors
        for (nECU_ADC2_ID current_ID = 0; current_ID < ADC2_ID_MAX; current_ID++)
        {
            status |= nECU_InputAnalog_ADC2_Start(ADC2_VSS_FL_ID + current_ID);
            F0_var.SpeedSensor[current_ID] = 0;
        }

        status |= nECU_Menu_Start();
        if (!status) // do only if no error
        {
            if (nECU_Menu_Antilag_getPointer()) // Check if pointers exist
                F0_var.Antilag = nECU_Menu_Antilag_getPointer();
            else
                status |= true;

            if (nECU_Menu_TractionOFF_getPointer()) // Check if pointers exist
                F0_var.TractionOFF = nECU_Menu_TractionOFF_getPointer();
            else
                status |= true;

            if (nECU_Menu_ClearCode_getPointer()) // Check if pointers exist
                F0_var.ClearCode = nECU_Menu_ClearCode_getPointer();
            else
                status |= true;

            if (nECU_Menu_LunchLvl_getPointer()) // Check if pointers exist
                F0_var.LunchLvl = nECU_Menu_LunchLvl_getPointer();
            else
                status |= true;
        }

        status |= nECU_DigitalInput_Start(DigiInput_CRANKING_ID);
        status |= nECU_DigitalInput_Start(DigiInput_FAN_ON_ID);
        status |= nECU_DigitalInput_Start(DigiInput_LIGHTS_ON_ID);

        // add immo init (when implemented)
        F0_var.IgnitionKey = nECU_Immo_getPointer();

        status |= nECU_Tacho_Start();
        if (!status) // do only if no error
        {
            for (Tacho_ID current_ID = 0; current_ID < TACHO_ID_MAX; current_ID++)
            {
                if (nECU_Tacho_Show_getPointer(current_ID))
                    F0_var.TachoShow[current_ID] = nECU_Tacho_Show_getPointer(current_ID);
                else
                    status |= true;
            }
        }

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_Frame_Speed_ID);
    }
    if (!nECU_FlowControl_Working_Check(D_Frame_Speed_ID) && status == false)
    {
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_Frame_Speed_ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_Frame_Speed_ID);
    return status;
}
void Frame0_Routine(void) // update variables for frame 0
{
    if (!nECU_FlowControl_Working_Check(D_Frame_Speed_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_Speed_ID);
        return;
    }

    for (nECU_ADC2_ID current_ID = 0; current_ID < ADC2_ID_MAX; current_ID++) // Collect new data
    {
        nECU_InputAnalog_ADC2_Routine(D_ANALOG_SS1 + current_ID);
        F0_var.SpeedSensor[current_ID] = nECU_FloatToUint(nECU_InputAnalog_ADC2_getValue(D_ANALOG_SS1 + current_ID), 12);
    }

    nECU_DigitalInput_Routine(DigiInput_CRANKING_ID);
    nECU_DigitalInput_Routine(DigiInput_FAN_ON_ID);
    nECU_DigitalInput_Routine(DigiInput_LIGHTS_ON_ID);
    nECU_Tacho_Routine();

    memset(F0_var.LunchControl, 0, LaunchControl_ID_MAX); // clear memory
    if (*F0_var.LunchLvl > LaunchControl_OFF)             // Decode launch control level
    {
        F0_var.LunchControl[LaunchControl_Low] = (*F0_var.LunchLvl < LaunchControl_Rolling); // Low is also launch control enable for all levels, must be on except rolling
        F0_var.LunchControl[*F0_var.LunchLvl] = true;
    }

    nECU_Debug_ProgramBlockData_Update(D_Frame_Speed_ID);
}
void Frame0_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_Frame_Speed_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_Speed_ID);
        return;
    }
    Frame0_Routine();

    F0_var.Stock_GPIO[DigiInput_CRANKING_ID] = nECU_DigitalInput_getValue(DigiInput_CRANKING_ID);
    F0_var.Stock_GPIO[DigiInput_FAN_ON_ID] = nECU_DigitalInput_getValue(DigiInput_FAN_ON_ID);
    F0_var.Stock_GPIO[DigiInput_LIGHTS_ON_ID] = nECU_DigitalInput_getValue(DigiInput_LIGHTS_ON_ID);

    Frame0_ComposeWord(&F0_var.Buffer[0], F0_var.IgnitionKey, &F0_var.Stock_GPIO[DigiInput_FAN_ON_ID], &F0_var.Stock_GPIO[DigiInput_LIGHTS_ON_ID], &F0_var.Stock_GPIO[DigiInput_CRANKING_ID], &F0_var.SpeedSensor[ADC2_VSS_FL_ID]);
    Frame0_ComposeWord(&F0_var.Buffer[2], F0_var.ClearCode, F0_var.TachoShow[TACHO_ID_MenuLvl], F0_var.TachoShow[TACHO_ID_LunchLvl], F0_var.TachoShow[TACHO_ID_TuneSel], &F0_var.SpeedSensor[ADC2_VSS_FR_ID]);
    Frame0_ComposeWord(&F0_var.Buffer[4], F0_var.Antilag, &F0_var.LunchControl[LaunchControl_High], &F0_var.LunchControl[LaunchControl_Medium], &F0_var.LunchControl[LaunchControl_Low], &F0_var.SpeedSensor[ADC2_VSS_RL_ID]);
    Frame0_ComposeWord(&F0_var.Buffer[6], (bool *)false, (bool *)false, F0_var.TractionOFF, &F0_var.LunchControl[LaunchControl_Rolling], &F0_var.SpeedSensor[ADC2_VSS_RR_ID]);
    nECU_CAN_WriteToBuffer(nECU_Frame_Speed_ID, sizeof(F0_var.Buffer));
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

    if (!nECU_FlowControl_Initialize_Check(D_Frame_EGT_ID))
    {
        for (EGT_Sensor_ID current_ID = 0; current_ID < EGT_ID_MAX; current_ID++)
        {
            if (nECU_EGT_getPointer_Temperature(current_ID))
                F1_var.EGT[current_ID] = nECU_EGT_getPointer_Temperature(current_ID);
            else
                status |= true;
        }

        status |= nECU_Tacho_Start();
        if (!status) // do only if no error
        {
            for (Tacho_ID current_ID = 0; current_ID < TACHO_ID_MAX; current_ID++)
            {
                if (nECU_Tacho_getPointer(current_ID))
                    F1_var.TachoVal[current_ID] = nECU_Tacho_getPointer(current_ID);
                else
                    status |= true;
            }
        }

        status |= nECU_Menu_Start();
        if (!status) // do only if no error
        {
            if (nECU_Menu_TuneSel_getPointer())
                F1_var.TuneSel = nECU_Menu_TuneSel_getPointer();
            else
                status |= true;
        }

        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_Frame_EGT_ID);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_Frame_EGT_ID) && status == false)
    {
        status |= nECU_EGT_Start();
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_Frame_EGT_ID);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Frame_EGT_ID);
    }
    return status;
}
void Frame1_Routine(void) // update variables for frame 1
{
    if (!nECU_FlowControl_Working_Check(D_Frame_EGT_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_EGT_ID);
        return;
    }
    // nECU_EGT_RequestUpdate();
    nECU_Tacho_Routine();

    nECU_Debug_ProgramBlockData_Update(D_Frame_EGT_ID);
}
void Frame1_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_Frame_EGT_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_EGT_ID);
        return;
    }
    Frame1_Routine();
    Frame1_ComposeWord(&F1_var.Buffer[0], F1_var.TachoVal[TACHO_ID_TuneSel], F1_var.EGT[EGT1_ID]);
    Frame1_ComposeWord(&F1_var.Buffer[2], F1_var.TachoVal[TACHO_ID_LunchLvl], F1_var.EGT[EGT2_ID]);
    Frame1_ComposeWord(&F1_var.Buffer[4], F1_var.TachoVal[TACHO_ID_MenuLvl], F1_var.EGT[EGT3_ID]);
    Frame1_ComposeWord(&F1_var.Buffer[6], (uint8_t *)F1_var.TuneSel, F1_var.EGT[EGT4_ID]);
    nECU_CAN_WriteToBuffer(nECU_Frame_EGT_ID, sizeof(F1_var.Buffer));
    nECU_Tacho_Clear_getPointer(TACHO_ID_TuneSel);
    nECU_Tacho_Clear_getPointer(TACHO_ID_LunchLvl);
    nECU_Tacho_Clear_getPointer(TACHO_ID_MenuLvl);
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

    if (!nECU_FlowControl_Initialize_Check(D_Frame_Stock_ID))
    {
        status |= nECU_InputAnalog_ADC1_Start(ADC1_BackPressure_ID);
        status |= nECU_InputAnalog_ADC1_Start(ADC1_OX_ID);
        status |= nECU_InputAnalog_ADC1_Start(ADC1_MAP_ID);

        status |= nECU_Knock_Start();
        if (!status) // do only if no error
        {
            if (nECU_Knock_GetPointer())
                F2_var.Knock = nECU_Knock_GetPointer();
            else
                status |= true;
        }

        status |= nECU_FreqInput_Start(FREQ_VSS_ID);

        F2_var.loop_time = nECU_Debug_ProgramBlockData_getPointer_Diff(D_Main);

        if (!status)
        {
            status |= !nECU_FlowControl_Initialize_Do(D_Frame_Stock_ID);
        }
    }
    if (!nECU_FlowControl_Working_Check(D_Frame_Stock_ID) && status == false)
    {
        status |= nECU_OX_Start();
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_Frame_Stock_ID);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_Frame_Stock_ID);
    }

    return status;
}
void Frame2_Routine(void) // update variables for frame 2
{
    if (!nECU_FlowControl_Working_Check(D_Frame_Stock_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_Stock_ID);
        return;
    }

    nECU_OX_Routine(); // TODO
    nECU_FreqInput_Routine(FREQ_VSS_ID);

    nECU_InputAnalog_ADC1_Routine(ADC1_MAP_ID);
    nECU_InputAnalog_ADC1_Routine(ADC1_BackPressure_ID);
    nECU_InputAnalog_ADC1_Routine(ADC1_OX_ID);

    nECU_Debug_ProgramBlockData_Update(D_Frame_Stock_ID);
}
void Frame2_PrepareBuffer(void) // prepare Tx buffer for CAN transmission
{
    if (!nECU_FlowControl_Working_Check(D_Frame_Stock_ID))
    {
        nECU_FlowControl_Error_Do(D_Frame_Stock_ID);
        return;
    }

    Frame2_Routine();
    union Int16ToBytes Converter; // create memory union

    F2_var.MAP_Stock_10bit = nECU_FloatToUint(nECU_InputAnalog_ADC1_getValue(ADC1_MAP_ID), 10);
    F2_var.Backpressure = nECU_FloatToUint(nECU_InputAnalog_ADC1_getValue(ADC1_BackPressure_ID), 8);
    F2_var.OX_Val = nECU_FloatToUint(nECU_InputAnalog_ADC1_getValue(ADC1_OX_ID), 8);
    // F2_var.OX_Val = 0;
    F2_var.VSS = nECU_FloatToUint(nECU_FreqInput_getValue(FREQ_VSS_ID), 8);

    Converter.UintValue = F2_var.MAP_Stock_10bit + FRAME_MAP_OFFSET;
    if (Converter.UintValue > MAX_VAL_10BIT) // round if out of bound
    {
        Converter.UintValue = MAX_VAL_10BIT;
    }
    F2_var.Buffer[0] = Converter.byteArray[1] & 0x3; // 6bit value left;
    F2_var.Buffer[1] = Converter.byteArray[0];
    F2_var.Buffer[2] = F2_var.OX_Val;
    F2_var.Buffer[3] = F2_var.Backpressure;
    F2_var.Buffer[4] = *F2_var.Knock;
    F2_var.Buffer[5] = F2_var.VSS;
    Converter.UintValue = (uint16_t)*F2_var.loop_time;
    F2_var.Buffer[6] = Converter.byteArray[1]; // spare
    F2_var.Buffer[7] = Converter.byteArray[0]; // spare
    nECU_CAN_WriteToBuffer(nECU_Frame_Stock_ID, sizeof(F2_var.Buffer));
}

void nECU_Frame_TX_done(nECU_CAN_Frame_ID ID) // callback after can TX is done
{
    if (ID > nECU_Frame_ID_MAX)
        return;
    if (ID == D_Frame_Speed_ID)
        F0_var.ClearCode = false;
}
uint8_t *nECU_Frame_getPointer(nECU_CAN_Frame_ID ID) // returns pointer to output buffer
{
    if (ID > nECU_Frame_ID_MAX)
        return NULL;

    switch (ID)
    {
    case nECU_Frame_Speed_ID:
        return F0_var.Buffer;
        break;
    case nECU_Frame_EGT_ID:
        return F1_var.Buffer;
        break;
    case nECU_Frame_Stock_ID:
        return F2_var.Buffer;
        break;

    default:
        return NULL;
        break;
    }
    return NULL;
}