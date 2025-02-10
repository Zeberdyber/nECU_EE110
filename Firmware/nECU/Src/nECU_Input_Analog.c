/**
 ******************************************************************************
 * @file    nECU_Input_Analog.c
 * @brief   This file provides code for analog inputs.
 ******************************************************************************
 */

#include "nECU_Input_Analog.h"

/*ADC 1*/
static Sensor_Handle ADC1_List[ADC1_ID_MAX] = {0}; // List of sensors ADC1
// Adjust below values!!
static SensorCalibration ADC1_calib_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = {
        1203, 3020, // limits of ADC readout
        270, 1020,  // limits of resulting output
        0.0, 1.0    // Place holders
    },
    [ADC1_BackPressure_ID] = {
        800, 3213, // limits of ADC readout
        -20, 0,    // limits of resulting output
        0.0, 1.0   // Place holders
    },
    [ADC1_OX_ID] = {
        0, ADC_MAX_VALUE_12BIT,        // limits of ADC readout
        0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
        0.0, 1.0                       // Place holders
    },
    [ADC1_AI_1_ID] = {
        // Place holder values
        0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
        0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
        0.0, 1.0                         // Place holders
    },
    [ADC1_AI_2_ID] = {
        // Place holder values
        0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
        0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
        0.0, 1.0                         // Place holders
    },
    [ADC1_AI_3_ID] = {
        // Place holder values
        0, ADC_MAX_VALUE_12BIT,          // limits of ADC readout
        0.0, (float)ADC_MAX_VALUE_12BIT, // limits of resulting output
        0.0, 1.0                         // Place holders
    },
    [ADC1_MCUTemp_ID] = {
        0, ADC_MAX_VALUE_12BIT,                     // limits of ADC readout
        TEMPSENSOR_CAL1_TEMP, TEMPSENSOR_CAL2_TEMP, // limits of resulting output
        0.0, 1.0                                    // Place holders
    },
    [ADC1_VREF_ID] = {
        // Place holder values
        0, ADC_MAX_VALUE_12BIT,         // limits of ADC readout
        0.0, (VREFINT_CAL_VREF / 1000), // limits of resulting output
        0.0, 1.0                        // Place holders
    },
}; // List of default calibration values
static uint32_t ADC1_delay_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = 0,
    [ADC1_BackPressure_ID] = 0,
    [ADC1_OX_ID] = 0,
    [ADC1_AI_1_ID] = 0,
    [ADC1_AI_2_ID] = 0,
    [ADC1_AI_3_ID] = 0,
    [ADC1_MCUTemp_ID] = 0,
    [ADC1_VREF_ID] = 0,
}; // List of delay values between updates in ms
static Buffer ADC1_Buffer_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = {NULL, 0},
    [ADC1_BackPressure_ID] = {NULL, 0},
    [ADC1_OX_ID] = {NULL, 0},
    [ADC1_AI_1_ID] = {NULL, 0},
    [ADC1_AI_2_ID] = {NULL, 0},
    [ADC1_AI_3_ID] = {NULL, 0},
    [ADC1_MCUTemp_ID] = {NULL, 0},
    [ADC1_VREF_ID] = {NULL, 0},
}; // List of pointers to smoothing buffers and its lenghts
static float ADC1_Alpha_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = 1.0,
    [ADC1_BackPressure_ID] = 1.0,
    [ADC1_OX_ID] = 1.0,
    [ADC1_AI_1_ID] = 1.0,
    [ADC1_AI_2_ID] = 1.0,
    [ADC1_AI_3_ID] = 1.0,
    [ADC1_MCUTemp_ID] = 1.0,
    [ADC1_VREF_ID] = 1.0,
}; // List of alphas for smoothing

/*ADC 2*/
static Sensor_Handle ADC2_List[ADC2_ID_MAX] = {0};                      // List of sensors ADC1
static uint16_t ADC2_Smoothing[ADC2_ID_MAX][SPEED_AVERAGE_BUFFER_SIZE]; // buffer to be filled with ADC data
// Adjust below values!!
static SensorCalibration ADC2_calib_List[ADC2_ID_MAX] = {
    [ADC2_VSS_FL_ID] = {
        0, 575,  // limits of ADC readout
        0, 1000, // limits of resulting output 0-100km/h
        0.0, 1.0 // Place holders
    },
    [ADC2_VSS_FR_ID] = {
        0, 575,  // limits of ADC readout
        0, 1000, // limits of resulting output 0-100km/h
        0.0, 1.0 // Place holders
    },
    [ADC2_VSS_RL_ID] = {
        0, 575,  // limits of ADC readout
        0, 1000, // limits of resulting output 0-100km/h
        0.0, 1.0 // Place holders
    },
    [ADC2_VSS_RR_ID] = {
        0, 575,  // limits of ADC readout
        0, 1000, // limits of resulting output 0-100km/h
        0.0, 1.0 // Place holders
    },
}; // List of default calibration values
static uint32_t ADC2_delay_List[ADC2_ID_MAX] = {
    [ADC2_VSS_FL_ID] = 0,
    [ADC2_VSS_FR_ID] = 0,
    [ADC2_VSS_RL_ID] = 0,
    [ADC2_VSS_RR_ID] = 0,
}; // List of delay values between updates in ms
static Buffer ADC2_Buffer_List[ADC2_ID_MAX] = {
    [ADC2_VSS_FL_ID] = {ADC2_Smoothing[ADC2_VSS_FL_ID], 0},
    [ADC2_VSS_FR_ID] = {ADC2_Smoothing[ADC2_VSS_FR_ID], 0},
    [ADC2_VSS_RL_ID] = {ADC2_Smoothing[ADC2_VSS_RL_ID], 0},
    [ADC2_VSS_RR_ID] = {ADC2_Smoothing[ADC2_VSS_RR_ID], 0},
}; // List of pointers to smoothing buffers and its lenghts
static float ADC2_Alpha_List[ADC2_ID_MAX] = {
    [ADC2_VSS_FL_ID] = 0.04,
    [ADC2_VSS_FR_ID] = 0.04,
    [ADC2_VSS_RL_ID] = 0.04,
    [ADC2_VSS_RR_ID] = 0.04,
}; // List of alphas for smoothing

float nECU_correctToVref(float input)
{
    if (!nECU_FlowControl_Working_Check(D_ANALOG_VREF))
        return input;

    return (ADC1_List[ADC1_VREF_ID].output * input) / (VREFINT_CAL_VREF / 1000);
}

/*ADC 1*/
bool nECU_InputAnalog_ADC1_Start(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_ANALOG_MAP + ID) && status == false)
    {
        if (ID == ADC1_MCUTemp_ID)
        {
            ADC1_calib_List[ID].ADC_MeasuredMin = *(TEMPSENSOR_CAL1_ADDR);
            ADC1_calib_List[ID].ADC_MeasuredMax = *(TEMPSENSOR_CAL2_ADDR);
        }
        if (ID == ADC1_VREF_ID)
        {
            ADC1_calib_List[ID].ADC_MeasuredMax = *(VREFINT_CAL_ADDR);
        }

        // Pointers
        if (nECU_ADC1_getPointer(ID))
            ADC1_List[ID].Input = nECU_ADC1_getPointer(ID);
        else
            status |= true;

        // Calibration
        ADC1_List[ID].calibration = ADC1_calib_List[ID];
        nECU_calculateLinearCalibration(&(ADC1_List[ID].calibration));

        // Filtering
        ADC1_List[ID].filter.smoothingAlpha = ADC1_Alpha_List[ID];
        ADC1_List[ID].filter.buf = ADC1_Buffer_List[ID];
        status |= nECU_Delay_Set(&(ADC1_List[ID].filter.delay), ADC1_delay_List[ID]);

        // Default value
        ADC1_List[ID].output = 0.0;

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_ANALOG_MAP + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Start(&(ADC1_List[ID].filter.delay));
        status |= nECU_ADC1_START();
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_ANALOG_MAP + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
bool nECU_InputAnalog_ADC1_Stop(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(ADC1_List[ID].filter.delay));
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_ANALOG_MAP + ID);

        status |= nECU_ADC1_STOP();
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
void nECU_InputAnalog_ADC1_Routine(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return;

    if (!nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);
        return; // Break
    }

    nECU_ADC1_Routine(); // Pull new data

    nECU_Sensor_Routine(&(ADC1_List[ID]));

    nECU_Debug_ProgramBlockData_Update(D_ANALOG_MAP + ID);
}

float nECU_InputAnalog_ADC1_getValue(nECU_ADC1_ID ID) // returns output value
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return 0.0;

    if (!nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);
        return 0.0; // Break
    }

    return ADC1_List[ID].output;
}

/*ADC 2*/
bool nECU_InputAnalog_ADC2_Start(nECU_ADC2_ID ID)
{
    if (ID >= ADC2_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_ANALOG_SS1 + ID) && status == false)
    {
        // Pointers
        if (nECU_ADC2_getPointer(ID))
            ADC2_List[ID].Input = nECU_ADC2_getPointer(ID);
        else
            status |= true;

        // Calibration
        ADC2_List[ID].calibration = ADC2_calib_List[ID];
        nECU_calculateLinearCalibration(&(ADC2_List[ID].calibration));

        // Filtering
        ADC2_List[ID].filter.smoothingAlpha = ADC2_Alpha_List[ID];
        ADC2_List[ID].filter.buf = ADC2_Buffer_List[ID];
        status |= nECU_Delay_Set(&(ADC2_List[ID].filter.delay), ADC2_delay_List[ID]);

        // Default value
        ADC2_List[ID].output = 0.0;

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_ANALOG_SS1 + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_ANALOG_SS1 + ID) && status == false)
    {
        status |= nECU_Delay_Start(&(ADC2_List[ID].filter.delay));
        status |= nECU_ADC2_START();
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_ANALOG_SS1 + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_SS1 + ID);

    return status;
}
bool nECU_InputAnalog_ADC2_Stop(nECU_ADC2_ID ID)
{
    if (ID >= ADC2_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FlowControl_Working_Check(D_ANALOG_SS1 + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(ADC2_List[ID].filter.delay));
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_ANALOG_SS1 + ID);

        status |= nECU_ADC2_STOP();
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_SS1 + ID);

    return status;
}
void nECU_InputAnalog_ADC2_Routine(nECU_ADC2_ID ID)
{
    if (ID >= ADC2_ID_MAX) // check if ID valid
        return;

    if (!nECU_FlowControl_Working_Check(D_ANALOG_SS1 + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_ANALOG_SS1 + ID);
        return; // Break
    }

    nECU_ADC2_Routine(); // Pull new data

    nECU_Sensor_Routine(&(ADC2_List[ID]));

    nECU_Debug_ProgramBlockData_Update(D_ANALOG_SS1 + ID);
}

float nECU_InputAnalog_ADC2_getValue(nECU_ADC2_ID ID) // returns output value
{
    if (ID >= ADC2_ID_MAX) // check if ID valid
        return 0.0;

    if (!nECU_FlowControl_Working_Check(D_ANALOG_SS1 + ID)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_ANALOG_SS1 + ID);
        return 0.0; // Break
    }

    return ADC2_List[ID].output;
}
