/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.c
 * @brief   This file provides code for frequency inputs.
 ******************************************************************************
 */

#include "nECU_Input_Frequency.h"

static uint16_t VSS_Buffer[10] = {0};

static nECU_InputFreq Sensor_List[FREQ_ID_MAX] = {0};
static SensorCalibration Sensor_calib_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = {
        0, 36,   // limits of freq readout
        0, 5.76, // limits of resulting output
        0.0, 1.0 // Place holders
    },
    [FREQ_IGF_ID] = {
        0, 1,    // limits of freq readout
        0, 120,  // limits of resulting output
        0.0, 1.0 // Place holders
    },
}; // List of default calibration values
static uint32_t Sensor_delay_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 0,
    [FREQ_IGF_ID] = 0,
}; // List of delay values between updates in ms
static Buffer_uint16 Sensor_Buffer_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = {VSS_Buffer, (sizeof(VSS_Buffer) / sizeof(VSS_Buffer[0]))},
    [FREQ_IGF_ID] = {NULL, 0},
}; // List of pointers to smoothing buffers and its lenghts
static float Sensor_Alpha_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 0.3,
    [FREQ_IGF_ID] = 1.0,
}; // List of alphas for smoothing
static nECU_TIM_ID Timer_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = TIM_IC_FREQ_ID,
    [FREQ_IGF_ID] = TIM_IC_FREQ_ID,
}; // List of timers for IC
static uint32_t Channel_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 1, // TIM_CHANNEL_2
    [FREQ_IGF_ID] = 0, // TIM_CHANNEL_1
}; // Timer channel list
static nECU_DigiInput_ID DigiInput_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = DigiInput_VSS_ID,
    [FREQ_IGF_ID] = DigiInput_IGF_ID,
}; // List of connected Digital Inputs

bool nECU_FreqInput_Start(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return true;

    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_VSS + ID))
    {
        // Calibration
        Sensor_List[ID].sensor.calibration = Sensor_calib_List[ID];
        nECU_calculateLinearCalibration(&(Sensor_List[ID].sensor.calibration));

        // Filtering
        Sensor_List[ID].sensor.filter.smoothingAlpha = Sensor_Alpha_List[ID];
        Sensor_List[ID].sensor.filter.buf = Sensor_Buffer_List[ID];
        status |= nECU_Delay_Set(&(Sensor_List[ID].sensor.filter.delay), Sensor_delay_List[ID]);

        // Default value
        Sensor_List[ID].sensor.output = 0.0;

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_VSS + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_VSS + ID) && status == false)
    {
        status |= nECU_TIM_IC_Start(Timer_List[ID], Channel_List[ID], DigiInput_List[ID]);
        status |= nECU_Delay_Start(&(Sensor_List[ID].sensor.filter.delay));

        // Pointers
        if (nECU_TIM_IC_getPointer(Timer_List[ID], Channel_List[ID]))
            Sensor_List[ID].ic = nECU_TIM_IC_getPointer(Timer_List[ID], Channel_List[ID]);
        else
            status |= true;

        // Connect frequency as input to sensor
        Sensor_List[ID].sensor.Input = &Sensor_List[ID].ic->frequency;

        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_VSS + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_VSS + ID);

    return status;
}
bool nECU_FreqInput_Stop(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FlowControl_Working_Check(D_VSS + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(Sensor_List[ID].sensor.filter.delay));
        status |= nECU_TIM_IC_Stop(Timer_List[ID], Channel_List[ID]);

        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_VSS + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_VSS + ID);

    return status;
}
void nECU_FreqInput_Routine(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return;

    if (!nECU_FlowControl_Working_Check(D_VSS + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_VSS + ID);
        return; // Break
    }
    if (Sensor_List[ID].ic->newData == false) // Check if there is new data to update
        return;                               // Break

    nECU_Sensor_Routine(&(Sensor_List[ID].sensor));
    Sensor_List[ID].ic->newData = false;
    nECU_Debug_ProgramBlockData_Update(D_VSS + ID);
}

float nECU_FreqInput_getValue(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return 0.0;

    if (!nECU_FlowControl_Working_Check(D_VSS + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_VSS + ID);
        return 0.0; // Break
    }

    return Sensor_List[ID].sensor.output;
}