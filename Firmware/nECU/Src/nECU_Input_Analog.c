/**
 ******************************************************************************
 * @file    nECU_Input_Analog.c
 * @brief   This file provides code for analog inputs.
 ******************************************************************************
 */

#include "nECU_Input_Analog.h"

static AnalogSensor_Handle AnalogSensor_List[ADC1_ID_MAX] = {0}; // List of sensors

// Adjust below values!!
static AnalogSensorCalibration calibPreset_List[ADC1_ID_MAX] = {
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
static uint32_t delay_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = 0,
    [ADC1_BackPressure_ID] = 0,
    [ADC1_OX_ID] = 0,
    [ADC1_AI_1_ID] = 0,
    [ADC1_AI_2_ID] = 0,
    [ADC1_AI_3_ID] = 0,
    [ADC1_MCUTemp_ID] = 0,
    [ADC1_VREF_ID] = 0,
}; // List of delay values between updates in ms
static AnalogSensorBuffer SmoothingBuffer_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = {NULL, 0},
    [ADC1_BackPressure_ID] = {NULL, 0},
    [ADC1_OX_ID] = {NULL, 0},
    [ADC1_AI_1_ID] = {NULL, 0},
    [ADC1_AI_2_ID] = {NULL, 0},
    [ADC1_AI_3_ID] = {NULL, 0},
    [ADC1_MCUTemp_ID] = {NULL, 0},
    [ADC1_VREF_ID] = {NULL, 0},
}; // List of pointers to smoothing buffers and its lenghts
static float SmoothingAlpha_List[ADC1_ID_MAX] = {
    [ADC1_MAP_ID] = 1.0,
    [ADC1_BackPressure_ID] = 1.0,
    [ADC1_OX_ID] = 1.0,
    [ADC1_AI_1_ID] = 1.0,
    [ADC1_AI_2_ID] = 1.0,
    [ADC1_AI_3_ID] = 1.0,
    [ADC1_MCUTemp_ID] = 1.0,
    [ADC1_VREF_ID] = 1.0,
}; // List of alphas for smoothing

static float nECU_correctToVref(float input)
{
    if (!nECU_FlowControl_Working_Check(D_ANALOG_VREF))
        return input;

    return (AnalogSensor_List[ADC1_VREF_ID].output * input) / (VREFINT_CAL_VREF / 1000);
}

/* General functions */
bool nECU_InputAnalog_Start(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_ANALOG_MAP + ID) && status == false)
    {
        if (ID == ADC1_MCUTemp_ID)
        {
            calibPreset_List[ID].ADC_MeasuredMin = *(TEMPSENSOR_CAL1_ADDR);
            calibPreset_List[ID].ADC_MeasuredMax = *(TEMPSENSOR_CAL2_ADDR);
        }
        if (ID == ADC1_VREF_ID)
        {
            calibPreset_List[ID].ADC_MeasuredMax = *(VREFINT_CAL_ADDR);
        }

        // Pointers
        if (nECU_ADC1_getPointer(ID))
            AnalogSensor_List[ID].ADC_input = nECU_ADC1_getPointer(ID);
        else
            status |= true;

        // Calibration
        AnalogSensor_List[ID].calibration = calibPreset_List[ID];
        nECU_calculateLinearCalibration(&(AnalogSensor_List[ID].calibration));

        // Filtering
        AnalogSensor_List[ID].filter.smoothingAlpha = SmoothingAlpha_List[ID];
        AnalogSensor_List[ID].filter.buf = SmoothingBuffer_List[ID];
        status |= nECU_Delay_Set(&(AnalogSensor_List[ID].filter.delay), delay_List[ID]);

        // Default value
        AnalogSensor_List[ID].output = 0.0;

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_ANALOG_MAP + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Start(&(AnalogSensor_List[ID].filter.delay));
        status |= nECU_ADC1_START();
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_ANALOG_MAP + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
bool nECU_InputAnalog_Stop(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return true;

    bool status = false;
    if (nECU_FlowControl_Working_Check(D_ANALOG_MAP + ID) && status == false)
    {
        status |= nECU_Delay_Stop(&(AnalogSensor_List[ID].filter.delay));
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_ANALOG_MAP + ID);

        status |= nECU_ADC1_STOP();
    }
    if (status)
        nECU_FlowControl_Error_Do(D_ANALOG_MAP + ID);

    return status;
}
void nECU_InputAnalog_Routine(nECU_ADC1_ID ID)
{
    if (ID >= ADC1_ID_MAX) // check if ID valid
        return;

    nECU_ADC1_Routine(); // Pull new data

    nECU_Delay_Update(&(AnalogSensor_List[ID].filter.delay));
    if (AnalogSensor_List[ID].filter.delay.done == false) // check if time have passed
        return;                                           // drop if not done

    nECU_Delay_Start(&(AnalogSensor_List[ID].filter.delay)); // restart delay

    uint16_t SmoothingRresult = *(AnalogSensor_List[ID].ADC_input);
    if (AnalogSensor_List[ID].filter.buf.Buffer != NULL) // check if buffer was configured
        SmoothingRresult = nECU_averageSmooth((AnalogSensor_List[ID].filter.buf.Buffer), &SmoothingRresult, AnalogSensor_List[ID].filter.buf.len);

    SmoothingRresult = nECU_expSmooth(&SmoothingRresult, &(AnalogSensor_List[ID].filter.previous_ADC_Data), AnalogSensor_List[ID].filter.smoothingAlpha);

    AnalogSensor_List[ID].filter.previous_ADC_Data = SmoothingRresult;                                           // save for smoothing
    AnalogSensor_List[ID].output = nECU_getLinearSensor(SmoothingRresult, &(AnalogSensor_List[ID].calibration)); // calculate, calibration
    AnalogSensor_List[ID].output = nECU_correctToVref(AnalogSensor_List[ID].output);                             // correct to vref

    nECU_Debug_ProgramBlockData_Update(D_ANALOG_MAP + ID);
}