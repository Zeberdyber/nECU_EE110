/**
 ******************************************************************************
 * @file    nECU_Knock.c
 * @brief   This file provides code for Knock signal processing.
 ******************************************************************************
 */

#include "nECU_Knock.h"

nECU_Knock Knock = {0};
extern IGF_Handle IGF;

static bool Knock_Initialized = false, Knock_Working = false;
static bool Knock_UART_Transmission = false;
static nECU_UART knock_uart;
static uint8_t UART_data_buffer[512];

/* Knock detection */
void nECU_Knock_Init(void) // initialize and start
{
    if (Knock_Initialized == false)
    {
        // UART
        nECU_UART_Init(&knock_uart, &PC_UART, UART_data_buffer);

        Knock.RetardPerc = 0; // initial value
        Knock.LevelWaiting = false;
        Knock.CycleDoneFlag = nECU_Knock_Delay_DoneFlag();

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
        float SamplingFreq = TIM_CLOCK / ((KNOCK_ADC_SAMPLING_TIMER.Init.Prescaler + 1) * (KNOCK_ADC_SAMPLING_TIMER.Init.Period + 1));
        Knock.fft.KnockIndex = (round((KNOCK_FREQUENCY * FFT_LENGTH) / (SamplingFreq)) * 2) - 1;
        arm_rfft_fast_init_f32(&(Knock.fft.Handler), FFT_LENGTH);

        // set initialized flag
        Knock_Initialized = true;
    }
    if (Knock_Working == false && Knock_Initialized == true)
    {
        // ADC start
        ADC3_START();
        // RPM reference
        nECU_IGF_Init();
        // set working flag
        Knock_Working = true;
    }
}
void nECU_Knock_ADC_Callback(uint16_t *input_buffer) // periodic callback
{
    if (Knock_UART_Transmission == true)
    {
        nECU_Knock_Send_UART(input_buffer);
    }

    if (Knock_Working == true)
    {
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
        if (Knock.fft.flag = true)
        {
            nECU_Knock_DetectMagn();
            Knock.fft.flag = false;
        }
    }
}
void nECU_Knock_UpdatePeriodic(void) // function to perform time critical knock routines, call at regression timer interrupt
{
    if (Knock_Working == true)
    {
        nECU_TickTrack_Update(&(Knock.regres));
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
            Knock.RetardPerc -= (Knock.regres.difference * Knock.regres.convFactor * KNOCK_BETA) / 1000.0f;

            if (Knock.RetardPerc < 0)
            {
                Knock.RetardPerc = 0;
            }

            Knock.RetardOut = (uint8_t)Knock.RetardPerc;
        }
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
    float rpm_float = IGF.RPM;
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
    Knock_Initialized = false;
}
uint8_t *nECU_Knock_GetPointer(void) // returns pointer to knock retard percentage
{
    return &Knock.RetardOut;
}
nECU_UART *nECU_Knock_UART_Pointer(void) // returns pointer to Tx UART object
{
    return &knock_uart;
}
void nECU_Knock_Send_UART(uint16_t *ADC_data) // sends RAW ADC data over UART
{
    nECU_UART_SendKnock(ADC_data, &knock_uart);
}