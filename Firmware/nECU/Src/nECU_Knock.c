/**
 ******************************************************************************
 * @file    nECU_Knock.c
 * @brief   This file provides code for Knock signal processing.
 ******************************************************************************
 */

#include "nECU_Knock.h"

static nECU_Knock Knock = {0};

/* Knock detection */
bool nECU_Knock_Start(void) // initialize and start
{
    bool status = false;

    if (!nECU_FlowControl_Initialize_Check(D_Knock))
    {
        // UART
        status |= nECU_UART_Init(&Knock.uart, &PC_UART, Knock.UART_data_buffer);

        Knock.RetardPerc = 0; // initial value
        Knock.LevelWaiting = false;

        // regression timer
        nECU_TickTrack_Init(&(Knock.regres));

        // initialize threshold table
        const float inTable1[FFT_THRESH_TABLE_LEN] = {1000, 2000, 3000, 4000, 5000};          // RPM for mapping threshold values
        const float inTable2[FFT_THRESH_TABLE_LEN] = {28000, 125000, 300000, 450000, 400000}; // Min Knock threashold
        const float inTable3[FFT_THRESH_TABLE_LEN] = {50000, 150000, 400000, 550000, 500000}; // Max Knock threashold
        nECU_Table_Set(&(Knock.thresholdMap), inTable1, inTable2, inTable3, FFT_THRESH_TABLE_LEN);

        // initialize FFT module
        Knock.fft.Index = 0;
        Knock.fft.flag = false;
        TIM_HandleTypeDef tim = *nECU_TIM_getPointer(TIM_ADC_KNOCK_ID);
        float SamplingFreq = TIM_CLOCK / ((tim.Init.Prescaler + 1) * (tim.Init.Period + 1));
        Knock.fft.KnockIndex = (round((KNOCK_FREQUENCY * FFT_LENGTH) / (SamplingFreq)) * 2) - 1;
        status |= (arm_rfft_fast_init_f32(&(Knock.fft.Handler), FFT_LENGTH) != ARM_MATH_SUCCESS);

        if (!status)
            status |= !nECU_FlowControl_Initialize_Do(D_Knock);
    }
    if (!nECU_FlowControl_Working_Check(D_Knock) && status == false)
    {
        status |= nECU_ADC3_START();                 // ADC start
        status |= nECU_FreqInput_Start(FREQ_IGF_ID); // RPM reference
        if (!status)
            status |= !nECU_FlowControl_Working_Do(D_Knock);
    }
    if (status)
        nECU_FlowControl_Error_Do(D_Knock);

    return status;
}
void nECU_Knock_ADC_Callback(uint16_t *input_buffer) // periodic callback
{
    if (!nECU_FlowControl_Working_Check(D_Knock)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_Knock);
        return; // Break
    }

    if (Knock.UART_Transmission == true)
    {
        nECU_UART_SendKnock(input_buffer, &Knock.uart);
    }

    for (uint16_t i = 0; i < (KNOCK_DMA_LEN / 2); i++)
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
    if (Knock.fft.flag == true)
    {
        nECU_Knock_DetectMagn();
        Knock.fft.flag = false;
    }
}
void nECU_Knock_UpdatePeriodic(void) // function to calculate current retard value
{
    if (!nECU_FlowControl_Working_Check(D_Knock)) // Check if currently working
    {
        nECU_FlowControl_Error_Do(D_Knock);
        return; // Break
    }
    nECU_ADC3_Routine(); // Pull new data
    nECU_TickTrack_Update(&(Knock.regres));
    nECU_Delay_Update(&(Knock.delay));

    /* should be called every time timer time elapsed */
    if (Knock.LevelWaiting == true && Knock.delay.done == true)
    {
        Knock.LevelWaiting = false; // reset flag
        Knock.delay.done = false;   // reset flag

        Knock.RetardPerc += KNOCK_STEP * Knock.Level;

        if (Knock.RetardPerc > 100) // if boundry reached
        {
            Knock.RetardPerc = 100;
        }

        Knock.Level = 0; // reset level
    }
    else if (Knock.RetardPerc > 0)
    {
        Knock.RetardPerc -= (Knock.regres.difference * Knock.regres.convFactor * KNOCK_BETA) / 1000.0f;

        if (Knock.RetardPerc < 0) // round to 0
        {
            Knock.RetardPerc = 0;
        }

        Knock.RetardOut = (uint8_t)Knock.RetardPerc;
    }

    nECU_Debug_ProgramBlockData_Update(D_Knock);
}
static void nECU_Knock_DetectMagn(void) // function to detect knock based on ADC input
{
    float knockMagn = 0;
    knockMagn = sqrtf((Knock.fft.BufOut[Knock.fft.KnockIndex] * Knock.fft.BufOut[Knock.fft.KnockIndex]) + (Knock.fft.BufOut[Knock.fft.KnockIndex + 1] * Knock.fft.BufOut[Knock.fft.KnockIndex + 1]));
    nECU_Knock_Evaluate(&knockMagn);
}
static void nECU_Knock_Evaluate(float *magnitude) // check if magnitude is of knock range
{
    /* get thresholds */
    float rpm_float = nECU_FreqInput_getValue(FREQ_IGF_ID);
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
        uint32_t delay = (120000 / rpm_float); // 120000 = 120 (Hz to rpm) * 1000 (ms to s)
        nECU_Delay_Set(&(Knock.delay), delay);
        nECU_Delay_Start(&(Knock.delay));
    }
}
bool nECU_Knock_Stop(void) // stop
{
    bool status = false;
    if (nECU_FlowControl_Working_Check(D_Knock) && status == false)
    {
        status |= nECU_ADC3_STOP();
        status |= nECU_FreqInput_Stop(FREQ_IGF_ID);
        if (!status)
            status |= !nECU_FlowControl_Stop_Do(D_Knock);
    }
    return status;
}
uint8_t *nECU_Knock_GetPointer(void) // returns pointer to knock retard percentage
{
    return &Knock.RetardOut;
}