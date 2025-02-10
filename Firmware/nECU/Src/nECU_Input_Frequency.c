/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.c
 * @brief   This file provides code for frequency inputs.
 ******************************************************************************
 */

#include "nECU_Input_Frequency.h"

static uint16_t VSS_Buffer[75] = {0};

static nECU_InputFreq Sensor_List[FREQ_ID_MAX] = {0};
static SensorCalibration Sensor_calib_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = {
        1203, 3020, // limits of freq readout
        270, 1020,  // limits of resulting output
        0.0, 1.0    // Place holders
    },
    [FREQ_IGF_ID] = {
        0, 1000, // limits of freq readout
        0, 500,  // limits of resulting output
        0.0, 1.0 // Place holders
    },
}; // List of default calibration values
static uint32_t Sensor_delay_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 0,
    [FREQ_IGF_ID] = 0,
}; // List of delay values between updates in ms
static Buffer Sensor_Buffer_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = {&VSS_Buffer, 75},
    [FREQ_IGF_ID] = {NULL, 0},
}; // List of pointers to smoothing buffers and its lenghts
static float Sensor_Alpha_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = 0.15,
    [FREQ_IGF_ID] = 1.0,
}; // List of alphas for smoothing
static nECU_TIM_ID Timer_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = TIM_IC_FREQ_ID,
    [FREQ_IGF_ID] = TIM_IC_FREQ_ID,
}; // List of timers for IC
static uint32_t Channel_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = TIM_CHANNEL_2,
    [FREQ_IGF_ID] = TIM_CHANNEL_1,
}; // Timer channel list
static uint16_t Pin_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = GPIO_PIN_13,
    [FREQ_IGF_ID] = GPIO_PIN_12,
}; // List of Pins
static TIM_HandleTypeDef *Port_List[FREQ_ID_MAX] = {
    [FREQ_VSS_ID] = GPIOD,
    [FREQ_IGF_ID] = GPIOD,
}; // List of Ports

bool nECU_VSS_Start(nECU_Freq_ID ID) // initialize VSS structure
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_VSS + ID))
    {
        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_VSS + ID);
    }
    if (!nECU_FlowControl_Working_Check(D_VSS + ID) && status == false)
    {
        status |= nECU_TIM_IC_Start(Timer_List[ID], Channel_List[ID], Pin_List[ID], Port_List[ID]);

        // Pointers
        if (nECU_ADC1_getPointer(ID))
            Sensor_List[ID].sensor.Input = (ID);
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
            status |= !nECU_FlowControl_Working_Do(D_VSS + ID);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_VSS + ID);

    return status;
}
void nECU_VSS_Update(void) // update VSS structure
{
    if (!nECU_FlowControl_Working_Check(D_VSS)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_VSS);
        return; // Break
    }

    float speed = (VSS.ic.frequency) * (3600.0f / VSS_PULSES_PER_KM); // 3600 for m/s to km/h
    if (speed > (float)UINT8_MAX)                                     // check limits
    {
        speed = UINT8_MAX;
    }
    else if (speed < 0)
    {
        speed = 0;
    }

    // data smoothing
    uint16_t speedUint = nECU_FloatToUint(speed, 16);
    speedUint = nECU_averageExpSmooth(VSS.VSS_Smooth_buffer, &speedUint, (uint16_t *)&VSS.Speed, VSS_SMOOTH_BUFFER_LENGTH, VSS_SMOOTH_ALPHA);

    VSS.Speed = (uint8_t)speedUint;
    nECU_VSS_Validate();
}
static void nECU_VSS_Validate(void) // checks if recived signal is correct
{
    if (!(D_VSS.Status & D_BLOCK_WORKING)) // check if working
    {
        return;
    }

    if (VSS.Speed > VSS_MAX_SPEED && VSS.overspeed_error == false) // set error
    {
        nECU_Debug_error_mesage temp;
        nECU_Debug_Message_Set(&temp, VSS.Speed, nECU_ERROR_VSS_MAX);
        VSS.overspeed_error = true; // to spit the error only once
        VSS.Speed = 0;
    }
    else if (VSS.Speed < VSS_MAX_SPEED && VSS.overspeed_error == true)
    {
        VSS.overspeed_error = false; // to spit the error only once
    }
    // zero speed detection
    if (VSS.ic.newData == false) // drop if no new data
    {
        nECU_Delay_Update(&(VSS.VSS_ZeroDetect));
        if (VSS.VSS_ZeroDetect.done == true)
        {
            VSS.ic.frequency = 0;
        }
    }
    else
    {
        nECU_Delay_Start(&(VSS.VSS_ZeroDetect));
        VSS.ic.newData = false;
    }
}
void nECU_VSS_Stop(void) // deinitialize VSS structure
{
    if (D_VSS.Status & D_BLOCK_INITIALIZED)
    {
        HAL_TIM_Base_Stop_IT(VSS.tim.htim);
        HAL_TIM_IC_Stop_IT(VSS.tim.htim, VSS.tim.Channel_List[0]);
    }
    printf("Stock VSS -> STOPPED!\n");
    D_VSS.Status -= D_BLOCK_INITIALIZED_WORKING;
}

/* IGF - Ignition feedback */
bool nECU_IGF_Start(void) // initialize and start
{
    bool status = false;

    if (D_IGF.Status == D_BLOCK_STOP)
    {
        IGF.ic.previous_CCR = 0;
        IGF.tim.htim = &FREQ_INPUT_TIMER;
        nECU_TIM_Init(&IGF.tim);
        IGF.tim.Channel_Count = 1;
        IGF.tim.Channel_List[0] = TIM_CHANNEL_1;
        D_IGF.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_IGF.Status & D_BLOCK_INITIALIZED)
    {
        status |= (nECU_TIM_IC_Start(&IGF.tim) != TIM_OK);
        printf("Stock IGF -> STARTED!\n");
        D_IGF.Status |= D_BLOCK_WORKING;
    }

    return status;
}
void nECU_IGF_Update(void) // calculate RPM based on IGF signal
{
    if (!(D_IGF.Status & D_BLOCK_WORKING)) // check if working
    {
        return;
    }

    uint16_t RPM = IGF.ic.frequency * 120;
    if (RPM > IGF_MAX_RPM)
    {
        return;
    }

    float rpm_rate = RPM - IGF.RPM;
    if (rpm_rate < 0)
    {
        rpm_rate = -rpm_rate;
    }
    rpm_rate *= IGF.ic.frequency;
    if (rpm_rate > IGF_MAX_RPM_RATE)
    {
        // nECU_Fault_Missfire();
    }
    IGF.RPM = RPM; // save current RPM
}
bool nECU_IGF_Stop(void) // stop
{
    bool status = false;
    if (nECU_FlowControl_Working_Check(D_IGF) && status == false)
    {
        status |= (HAL_OK != HAL_TIM_Base_Stop_IT(IGF.tim.htim));
        status |= (HAL_OK != HAL_TIM_IC_Stop_IT(IGF.tim.htim, IGF.tim.Channel_List[0]));
        status |= nECU_ADC3_STOP();
        status |= nECU_IGF_Stop();
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_IGF);
    }
    return status;
}

/* General */
bool nECU_Frequency_Start(void) // start of frequency input functions
{
    bool status = false;

    if (D_Input_Frequency.Status == D_BLOCK_STOP)
    {
        status |= nECU_VSS_Start();
        status |= nECU_IGF_Start();

        D_Input_Frequency.Status |= D_BLOCK_INITIALIZED_WORKING;
    }
    else
    {
        status |= true;
    }

    return status;
}
void nECU_Frequency_Stop(void) // stop of frequency input functions
{
    nECU_VSS_Stop();
    nECU_IGF_Stop();

    D_Input_Frequency.Status -= D_BLOCK_INITIALIZED_WORKING;
}
void nECU_Frequency_Update(void) // update of frequency input functions
{
    if (!(D_Input_Frequency.Status & D_BLOCK_WORKING))
    {
        return;
    }

    nECU_VSS_Update();
    nECU_IGF_Update();
}