/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.c
 * @brief   This file provides code for frequency inputs.
 ******************************************************************************
 */

#include "nECU_Input_Frequency.h"

// internal variables
VSS_Handle VSS;
IGF_Handle IGF;

// initialized flags
static bool VSS_Initialized = false, VSS_Working = false,
            IGF_Initialized = false, IGF_Working = false;

/* VSS - Vehicle Speed Sensor */
uint8_t *nECU_VSS_GetPointer() // returns pointer to resulting data
{
    return &VSS.Speed;
}
void nECU_VSS_Init(void) // initialize VSS structure
{
    if (VSS_Initialized == false)
    {
        VSS.ic.previous_CCR = 0;
        VSS.tim.htim = &FREQ_INPUT_TIMER;
        nECU_tim_Init_struct(&VSS.tim);
        VSS.tim.Channel_Count = 1;
        VSS.tim.Channel_List[0] = TIM_CHANNEL_2;
        VSS_Initialized = true;
    }
    if (VSS_Working == false && VSS_Initialized == true)
    {
        nECU_tim_IC_start(&VSS.tim);
        VSS_Working = true;
    }
}
void nECU_VSS_Update(void) // update VSS structure
{
    if (VSS_Initialized == false || VSS_Working == false) // check if initialized
    {
        return;
    }

    float speed = (VSS.ic.frequency) * (3600.0f / VSS_PULSES_PER_KM); // 3600 for m/s to km/h
    if (speed > (float)UINT8_MAX)
    {
        speed = UINT8_MAX;
    }
    else if (speed < 0)
    {
        speed = 0;
    }

    // data smoothing
    uint16_t speed_old = VSS.Speed;
    uint16_t speed_new = speed;
    nECU_expSmooth(&speed_old, &speed_new, VSS_SMOOTH_ALPHA);

    VSS.Speed = (uint8_t)speed_new;
    nECU_VSS_Validate();
}
void nECU_VSS_Validate(void) // checks if recived signal is correct
{
    if (VSS_Initialized == false || VSS_Working == false) // check if initialized
    {
        return;
    }

    if (VSS.Speed > VSS_MAX_SPEED && VSS.overspeed_error == false) // set error
    {
        nECU_Debug_error_mesage temp;
        nECU_Debug_Message_Init(&temp);
        nECU_Debug_Message_Set(&temp, VSS.Speed, nECU_ERROR_VSS_MAX);
        VSS.overspeed_error = true; // to spit the error only once
    }
    else if (VSS.Speed < VSS_MAX_SPEED && VSS.overspeed_error == true)
    {
        VSS.overspeed_error = false; // to spit the error only once
    }
    // here add zero speed detectionAle
}
void nECU_VSS_DetectZero(TIM_HandleTypeDef *htim) // detect if zero km/h -- !!! to be fixed
{
    // float time = (TIM_CLOCK / (htim->Init.Prescaler + 1)) / (htim->Init.Period + 1);
    // if (VSS.Speed != 0)
    // {
    //     VSS.watchdogCount++;
    //     if ((VSS.watchdogCount / time) > (VSS.tim.htim->Init.Period / VSS.tim.refClock))
    //     {
    //         VSS.Speed = 0;
    //     }
    // }
}
void nECU_VSS_DeInit(void) // deinitialize VSS structure
{
    if (VSS_Initialized == true)
    {
        HAL_TIM_Base_Stop_IT(VSS.tim.htim);
        HAL_TIM_IC_Stop_IT(VSS.tim.htim, VSS.tim.Channel_List[0]);
    }
}
void nECU_VSS_Smooth_Update(void) // call this function to smooth the output data
{
}
/* IGF - Ignition feedback */
void nECU_IGF_Init(void) // initialize and start
{
    if (IGF_Initialized == false)
    {
        IGF.ic.previous_CCR = 0;
        IGF.tim.htim = &FREQ_INPUT_TIMER;
        nECU_tim_Init_struct(&IGF.tim);
        IGF.tim.Channel_Count = 1;
        IGF.tim.Channel_List[0] = TIM_CHANNEL_1;
        IGF_Initialized = true;
    }
    if (IGF_Working == false && IGF_Initialized == true)
    {
        nECU_tim_IC_start(&IGF.tim);
        IGF_Working = true;
    }
}
void nECU_IGF_Calc(void) // calculate RPM based on IGF signal
{
    if (IGF_Initialized == false || IGF_Working == false) // check if initialized
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
void nECU_IGF_DeInit(void) // stop
{
    if (IGF_Initialized == true)
    {
        HAL_TIM_Base_Stop_IT(IGF.tim.htim);
        HAL_TIM_IC_Stop_IT(IGF.tim.htim, IGF.tim.Channel_List[0]);
    }
}

/* General */
void nECU_Frequency_Start(void) // start of frequency input functions
{
    nECU_VSS_Init();
    nECU_IGF_Init();
}
void nECU_Frequency_Stop(void) // stop of frequency input functions
{
    nECU_VSS_DeInit();
    nECU_IGF_DeInit();
}
void nECU_Frequency_Update(void) // update of frequency input functions
{
    nECU_VSS_Update();
    nECU_IGF_Calc();
}