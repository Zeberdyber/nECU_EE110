/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.c
 * @brief   This file provides code for frequency inputs.
 ******************************************************************************
 */

#include "nECU_Input_Frequency.h"

static nECU_InputFreq Sensor_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = {
        // Sensor
        {
            // Calibration
            {
                0, 36,   // limits of freq readout
                0, 5.76, // limits of resulting output
                0.0, 1.0 // Place holders
            },
            // Filter
            {
                {0},        // Delay
                1.0,        // Smoothing Alpha
                {NULL, 10}, // buffer (configure 0 to desired buffer len)
                0.3,        // previous value
            },
            NULL, // input data
            0.0,  // output
        },
        TIM_IC_FREQ_ID,   // correlated timer
        1,                // timer channel (TIM_CHANNEL_2)
        NULL,             // IC pointer
        DigiInput_VSS_ID, // correlated GPIO pin
    },
    [FREQ_IGF_ID] = {
        // Sensor
        {
            // Calibration
            {
                0, 1,    // limits of freq readout
                0, 120,  // limits of resulting output
                0.0, 1.0 // Place holders
            },
            // Filter
            {
                {0},        // Delay
                1.0,        // Smoothing Alpha
                {NULL, 10}, // buffer (configure 0 to desired buffer len)
                1.0,        // previous value
            },
            NULL, // input data
            0.0,  // output
        },
        TIM_IC_FREQ_ID,   // correlated timer
        0,                // timer channel (TIM_CHANNEL_1)
        NULL,             // IC pointer
        DigiInput_IGF_ID, // correlated GPIO pin
    },
};
// Adjust below values!!
static uint32_t Sensor_delay_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 0,
    [FREQ_IGF_ID] = 0,
}; // List of delay values between updates in ms

bool nECU_FreqInput_Start(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return true;

    bool status = false;

    if (!nECU_FC_Initialize_Check(D_VSS + ID))
    {
        // Calibration
        nECU_calculateLinearCalibration(&(Sensor_List[ID].sensor.calibration));

        // Delay
        status |= nECU_Delay_Set(&(Sensor_List[ID].sensor.filter.delay), Sensor_delay_List[ID]);

        // Buffer malloc
        if (Sensor_List[ID].sensor.filter.buf.len > 0) // Check if buffer is specified
        {
            Sensor_List[ID].sensor.filter.buf.Buffer = malloc(Sensor_List[ID].sensor.filter.buf.len * sizeof(uint16_t));
            if (Sensor_List[ID].sensor.filter.buf.Buffer == NULL)
                status |= true;
            else
                memset(Sensor_List[ID].sensor.filter.buf.Buffer, 0, Sensor_List[ID].sensor.filter.buf.len * sizeof(uint16_t));
        }
        // Default value
        Sensor_List[ID].sensor.output = 0.0;

        if (!status)
            status |= !nECU_FC_Initialize_Do(D_VSS + ID);
    }
    if (!nECU_FC_Working_Check(D_VSS + ID) && status == false)
    {
        status |= nECU_TIM_IC_Start(Sensor_List[ID].timer_ID, Sensor_List[ID].ic_channel, Sensor_List[ID].gpio_ID);
        status |= nECU_Delay_Start(&(Sensor_List[ID].sensor.filter.delay));

        // Pointers
        if (nECU_TIM_IC_getPointer(Sensor_List[ID].timer_ID, Sensor_List[ID].ic_channel))
            Sensor_List[ID].ic = nECU_TIM_IC_getPointer(Sensor_List[ID].timer_ID, Sensor_List[ID].ic_channel);
        else
            status |= true;

        // Connect frequency as input to sensor
        Sensor_List[ID].sensor.Input = &Sensor_List[ID].ic->frequency;

        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_VSS + ID);
    }
    if (status)
        nECU_FC_Error_Do(D_VSS + ID);

    return status;
}
bool nECU_FreqInput_Stop(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FC_Working_Check(D_VSS + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(Sensor_List[ID].sensor.filter.delay));
        status |= nECU_TIM_IC_Stop(Sensor_List[ID].timer_ID, Sensor_List[ID].ic_channel);
        free(Sensor_List[ID].sensor.filter.buf.Buffer);

        if (!status)
            status |= !nECU_FC_Stop_Do(D_VSS + ID);
    }
    if (status)
        nECU_FC_Error_Do(D_VSS + ID);

    return status;
}
void nECU_FreqInput_Routine(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return;

    if (!nECU_FC_Working_Check(D_VSS + ID)) // Check if currently working
    {
        nECU_FC_Error_Do(D_VSS + ID);
        return; // Break
    }
    if (Sensor_List[ID].ic->newData == false) // Check if there is new data to update
        return;                               // Break

    nECU_Sensor_Routine(&(Sensor_List[ID].sensor));
    Sensor_List[ID].ic->newData = false;
    nECU_FC_Timeout_Check(D_VSS + ID);
}

float nECU_FreqInput_getValue(nECU_Freq_ID ID)
{
    if (ID >= FREQ_ID_MAX) // check if ID valid
        return 0.0;

    if (!nECU_FC_Working_Check(D_VSS + ID)) // Check if currently working
    {
        nECU_FC_Error_Do(D_VSS + ID);
        return 0.0; // Break
    }

    return Sensor_List[ID].sensor.output;
}