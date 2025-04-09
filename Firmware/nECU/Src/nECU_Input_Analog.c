/**
 ******************************************************************************
 * @file    nECU_Input_Analog.c
 * @brief   This file provides code for analog inputs.
 ******************************************************************************
 */

#include "nECU_Input_Analog.h"

static Sensor_Handle ADC_List[ADC_ID_MAX] = {
    [ADC_MAP_ID] = {
        {
            // Calibration
            1203, 3020, // limits of ADC readout
            270, 1020,  // limits of resulting output
            0.0, 1.0    // Place holders
        },
        {
            // Filter
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_BackPressure_ID] = {
        // Calibration
        {
            800, 3213, // limits of ADC readout
            -20, 0,    // limits of resulting output
            0.0, 1.0   // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_OX_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,        // limits of ADC readout
            0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
            0.0, 1.0                       // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_AI_1_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
            0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
            0.0, 1.0                         // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_AI_2_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
            0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
            0.0, 1.0                         // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_AI_3_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
            0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
            0.0, 1.0                         // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_MCUTemp_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,                     // limits of ADC readout
            TEMPSENSOR_CAL1_TEMP, TEMPSENSOR_CAL2_TEMP, // limits of resulting output
            0.0, 1.0                                    // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_VREF_ID] = {
        // Calibration
        {
            0, ADC_MAX_VALUE_12BIT,       // limits of ADC readout
            0.0, (float)VREFINT_CAL_VREF, // limits of resulting output
            0.0, 1.0                      // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0,                              // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_VSS_FL_ID] = {
        // Calibration
        {
            0, 575,  // limits of ADC readout
            0, 1000, // limits of resulting output 0-100km/h
            0.0, 1.0 // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0.04,                           // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_VSS_FR_ID] = {
        // Calibration
        {
            0, 575,  // limits of ADC readout
            0, 1000, // limits of resulting output 0-100km/h
            0.0, 1.0 // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0.04,                           // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_VSS_RL_ID] = {
        // Calibration
        {
            0, 575,  // limits of ADC readout
            0, 1000, // limits of resulting output 0-100km/h
            0.0, 1.0 // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0.04,                           // previous value
        },
        NULL, // input data
        0.0,  // output
    },
    [ADC_VSS_RR_ID] = {
        // Calibration
        {
            0, 575,  // limits of ADC readout
            0, 1000, // limits of resulting output 0-100km/h
            0.0, 1.0 // Place holders
        },
        // Filter
        {
            {0},                            // Delay
            1.0,                            // Smoothing Alpha
            {{NULL}, 0 * sizeof(uint16_t)}, // buffer (configure 0 to desired buffer len)
            0.04,                           // previous value
        },
        NULL, // input data
        0.0,  // output
    },
};
// Adjust below values!!
static uint32_t ADC_delay_List[ADC_ID_MAX] = {
    [ADC_MAP_ID] = 0,
    [ADC_BackPressure_ID] = 0,
    [ADC_OX_ID] = 0,
    [ADC_AI_1_ID] = 0,
    [ADC_AI_2_ID] = 0,
    [ADC_AI_3_ID] = 0,
    [ADC_MCUTemp_ID] = 0,
    [ADC_VREF_ID] = 0,
    [ADC_VSS_FL_ID] = 0,
    [ADC_VSS_FR_ID] = 0,
    [ADC_VSS_RL_ID] = 0,
    [ADC_VSS_RR_ID] = 0,
}; // List of delay values between updates in ms

float nECU_correctToVref(float input)
{
    if (!nECU_FC_Working_Check(D_ANALOG_VREF))
        return input;
    nECU_InputAnalog_Routine(ADC_VREF_ID);
    return (ADC_List[ADC_VREF_ID].output * input) / VREFINT_CAL_VREF;
}

bool nECU_InputAnalog_Start(nECU_ADC_Sensor_ID ID)
{
    if (ID >= ADC_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (!nECU_FC_Initialize_Check(D_ANALOG_MAP + ID) && status == false)
    {
        if (ID == ADC_MCUTemp_ID)
        {
            ADC_List[ID].calibration.ADC_MeasuredMin = *(TEMPSENSOR_CAL1_ADDR);
            ADC_List[ID].calibration.ADC_MeasuredMax = *(TEMPSENSOR_CAL2_ADDR);
        }
        if (ID == ADC_VREF_ID)
            ADC_List[ID].calibration.ADC_MeasuredMax = *(VREFINT_CAL_ADDR);

        // Pointers
        status |= nECU_ADC_START(ID);
        if (nECU_ADC_getPointer(ID))
            ADC_List[ID].Input = nECU_ADC_getPointer(ID);
        else
            status |= true;

        // Calibration
        nECU_calculateLinearCalibration(&(ADC_List[ID].calibration));

        // Delay
        status |= nECU_Delay_Set(&(ADC_List[ID].filter.delay), ADC_delay_List[ID]);

        // Buffer malloc
        if (ADC_List[ID].filter.buf.len > 0) // Check if buffer is specified
        {
            status |= !nECU_Memory_Create(&ADC_List[ID].filter.buf);
        }

        // Default value
        ADC_List[ID].output = 0.0;

        if (!status)
            status |= !nECU_FC_Initialize_Do(D_ANALOG_MAP + ID);
    }
    if (!nECU_FC_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Start(&(ADC_List[ID].filter.delay));
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_ANALOG_MAP + ID);
    }
    if (status)
        nECU_FC_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
bool nECU_InputAnalog_Stop(nECU_ADC_Sensor_ID ID)
{
    if (ID >= ADC_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FC_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(ADC_List[ID].filter.delay));
        status |= !nECU_Memory_Destroy(&ADC_List[ID].filter.buf);
        if (!status)
            status |= !nECU_FC_Stop_Do(D_ANALOG_MAP + ID);

        status |= nECU_ADC_STOP(ID);
    }
    if (status)
        nECU_FC_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
bool nECU_InputAnalog_Routine(nECU_ADC_Sensor_ID ID)
{
    if (ID >= ADC_ID_MAX) // check if ID valid
        return false;

    if (!nECU_FC_Working_Check(D_ANALOG_MAP + ID)) // Check if currently working
    {
        nECU_FC_Error_Do(D_ANALOG_MAP + ID);
        return false; // Break
    }

    if (!nECU_ADC_Routine(ID)) // Pull new data
        return false;          // No new data arived

    nECU_Sensor_Routine(&(ADC_List[ID]));

    nECU_FC_Timeout_Check(D_ANALOG_MAP + ID);
    return true;
}

float nECU_InputAnalog_getValue(nECU_ADC_Sensor_ID ID) // returns output value
{
    if (ID >= ADC_ID_MAX) // check if ID valid
        return 0.0;

    if (!nECU_FC_Working_Check(D_ANALOG_MAP + ID)) // Check if currently working
    {
        nECU_FC_Error_Do(D_ANALOG_MAP + ID);
        return 0.0; // Break
    }

    return ADC_List[ID].output;
}
