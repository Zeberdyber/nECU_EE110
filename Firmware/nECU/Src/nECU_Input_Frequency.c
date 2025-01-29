/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.c
 * @brief   This file provides code for frequency inputs.
 ******************************************************************************
 */

#include "nECU_Input_Frequency.h"

// internal variables
VSS_Handle VSS = {0};
IGF_Handle IGF = {0};

extern nECU_ProgramBlockData D_VSS, D_IGF, D_Input_Frequency; // diagnostic and flow control data

/* VSS - Vehicle Speed Sensor */
uint8_t *nECU_VSS_GetPointer() // returns pointer to resulting data
{
    return &VSS.Speed;
}
bool nECU_VSS_Start(void) // initialize VSS structure
{
    bool status = false;

    if (D_VSS.Status == D_BLOCK_STOP)
    {
        VSS.ic.previous_CCR = 0;
        VSS.tim.htim = &FREQ_INPUT_TIMER;
        nECU_tim_Init_struct(&VSS.tim);
        VSS.tim.Channel_Count = 1;
        VSS.tim.Channel_List[0] = TIM_CHANNEL_2;

        uint32_t zero_delay = ((VSS.tim.htim->Init.Period + 1) * (VSS.tim.period * 1000)) + 10; // time for the whole timer count up + 10ms (* 1000 for sec to ms conversion);
        nECU_Delay_Set(&VSS.VSS_ZeroDetect, &zero_delay);

        D_VSS.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_VSS.Status & D_BLOCK_INITIALIZED)
    {
        status |= nECU_tim_IC_start(&VSS.tim);
        D_VSS.Status |= D_BLOCK_WORKING;
    }

    return status;
}
void nECU_VSS_Update(void) // update VSS structure
{
    if (!(D_VSS.Status & D_BLOCK_WORKING)) // check if working
    {
        return;
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
    uint16_t speed_new = speed;
    nECU_averageExpSmooth(VSS.VSS_Smooth_buffer, (uint16_t *)&VSS.Speed, &speed_new, VSS_SMOOTH_BUFFER_LENGTH, VSS_SMOOTH_ALPHA);

    VSS.Speed = (uint8_t)speed_new;
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
        nECU_tim_Init_struct(&IGF.tim);
        IGF.tim.Channel_Count = 1;
        IGF.tim.Channel_List[0] = TIM_CHANNEL_1;
        D_IGF.Status |= D_BLOCK_INITIALIZED;
    }
    if (D_IGF.Status & D_BLOCK_INITIALIZED)
    {
        status |= (nECU_tim_IC_start(&IGF.tim) != TIM_OK);
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
void nECU_IGF_Stop(void) // stop
{
    if (D_IGF.Status & D_BLOCK_WORKING)
    {
        HAL_TIM_Base_Stop_IT(IGF.tim.htim);
        HAL_TIM_IC_Stop_IT(IGF.tim.htim, IGF.tim.Channel_List[0]);
    }

    D_IGF.Status -= D_BLOCK_INITIALIZED_WORKING;
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