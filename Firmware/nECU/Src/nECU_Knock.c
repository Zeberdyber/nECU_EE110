/**
 ******************************************************************************
 * @file    nECU_Knock.c
 * @brief   This file provides code for Knock signal processing.
 ******************************************************************************
 */

#include "nECU_Knock.h"

nECU_Knock Knock;

/* Knock detection */
void nECU_Knock_Init(void) // initialize and start
{
    Knock.RetardPerc = 0; // initial value
    Knock.LevelWaiting = false;
    Knock.CycleDoneFlag = nECU_Knock_Delay_DoneFlag();

    // RPM reference
    nECU_RPM_Init();

    // start regression timer
    Knock.regres.htim = &KNOCK_REGRES_TIMER;
    Knock.regres.refClock = TIM_CLOCK / (Knock.regres.htim->Init.Prescaler + 1);
    Knock.StepTime = ((Knock.regres.htim->Init.Period + 1) * 1000.0f) / Knock.regres.refClock;
    HAL_TIM_Base_Start_IT(Knock.regres.htim);

    // initialize threshold table
    const float inTable1[FFT_THRESH_TABLE_LEN] = {1000, 2000, 3000, 4000, 5000};          // RPM for mapping threshold values
    const float inTable2[FFT_THRESH_TABLE_LEN] = {28000, 125000, 300000, 450000, 400000}; // Min Knock threashold
    const float inTable3[FFT_THRESH_TABLE_LEN] = {50000, 150000, 400000, 550000, 500000}; // Max Knock threashold
    nECU_Table_Set(&(Knock.thresholdMap), inTable1, inTable2, inTable3, FFT_THRESH_TABLE_LEN);

    // initialize FFT module
    Knock.fft.Index = 0;
    Knock.fft.flag = false;
    float SamplingFreq = TIM_CLOCK / ((KNOCK_ADC_SAMPLING_TIMER.Init.Prescaler + 1) * (KNOCK_ADC_SAMPLING_TIMER.Init.Period + 1));
    Knock.fft.KnockIndex = (round((KNOCK_FREQUENCY * FFT_LENGTH) / (SamplingFreq)) * 2) - 1;
    arm_rfft_fast_init_f32(&(Knock.fft.Handler), FFT_LENGTH);
}
void nECU_Knock_ADC_Callback(uint16_t *input_buffer) // periodic callback
{
    for (uint16_t i = 0; i < (KNOCK_SAMPLES_IN_BUFFOR / 2); i++)
    {
        Knock.fft.BufIn[Knock.fft.Index] = (float)input_buffer[i];
        Knock.fft.Index++;
        if (Knock.fft.Index == FFT_LENGTH) // if buffer full perform FFT
        {
            arm_rfft_fast_f32(&(Knock.fft.Handler), Knock.fft.BufIn, Knock.fft.BufOut, 0);
            Knock.fft.flag = true;
            Knock.fft.Index = 0;
        }
    }
    if (Knock.fft.flag = true)
    {
        nECU_Knock_DetectMagn();
        Knock.fft.flag = false;
    }
}
void nECU_Knock_UpdatePeriodic(void) // function to perform time critical knock routines, call at regression timer interrupt
{
    /* should be called every time timer time elapsed */
    if (Knock.LevelWaiting == true && *(Knock.CycleDoneFlag) == true)
    {
        Knock.LevelWaiting = false;     // reset flag
        *(Knock.CycleDoneFlag) = false; // reset flag

        Knock.RetardPerc += KNOCK_STEP * Knock.Level;

        if (Knock.RetardPerc > 100) // if boundry reached
        {
            Knock.RetardPerc = 100;
        }

        Knock.Level = 0; // reset level
    }
    else if (Knock.RetardPerc > 0)
    {
        Knock.RetardPerc -= (Knock.StepTime * KNOCK_BETA) / 1000.0f;

        if (Knock.RetardPerc < 0)
        {
            Knock.RetardPerc = 0;
        }

        Knock.RetardOut = (uint8_t)Knock.RetardPerc;
    }
}
void nECU_Knock_DetectMagn(void) // function to detect knock based on ADC input
{
    float knockMagn = 0;
    knockMagn = sqrtf((Knock.fft.BufOut[Knock.fft.KnockIndex] * Knock.fft.BufOut[Knock.fft.KnockIndex]) + (Knock.fft.BufOut[Knock.fft.KnockIndex + 1] * Knock.fft.BufOut[Knock.fft.KnockIndex + 1]));
    nECU_Knock_Evaluate(&knockMagn);
}
void nECU_Knock_Evaluate(float *magnitude) // check if magnitude is of knock range
{
    /* get thresholds */
    // float rpm_float = RPM.RPM;
    uint8_t *rpm_can = nECU_CAN_getRPMPointer();
    float rpm_float = *rpm_can * 20;
    if (rpm_float < 750) // while idle
    {
        return;
    }

    rpm_float -= 500;
    float threshold_min, threshold_max;
    nECU_Table_Get(&rpm_float, &(Knock.thresholdMap), &threshold_min, &threshold_max);

    /* if knock detected */
    if (*magnitude > threshold_min && Knock.LevelWaiting == false)
    {
        float minOut = 1, maxOut = KNOCK_LEVEL;
        Knock.Level = nECU_Table_Interpolate(&threshold_min, &minOut, &threshold_max, &maxOut, magnitude);
        Knock.LevelWaiting = true;
        nECU_Knock_Delay_Start(&rpm_float);
    }
}
void nECU_Knock_DeInit(void) // stop
{
    nECU_RPM_DeInit();
    HAL_TIM_Base_Stop_IT(Knock.regres.htim);
}
uint8_t *nECU_Knock_GetPointer(void) // returns pointer to knock retard percentage
{
    return &Knock.RetardOut;
}