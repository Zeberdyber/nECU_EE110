/**
 ******************************************************************************
 * @file    nECU_Knock.c
 * @brief   This file provides code for Knock signal processing.
 ******************************************************************************
 */

#include "nECU_Knock.h"

nECU_Knock Knock;
IGF_Handle RPM;

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
    float SamplingFreq = TIM_CLOCK / ((ADC_SAMPLING_TIMER.Init.Prescaler + 1) * (ADC_SAMPLING_TIMER.Init.Period + 1));
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
    float rpm_float = RPM.RPM;
    uint8_t *rpm_can = nECU_CAN_getRPMPointer();
    rpm_float = *rpm_can * 20;
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

void nECU_RPM_Init(void) // initialize and start
{
    RPM.IGF_prevCCR = 0;
    RPM.tim.htim = &FREQ_INPUT_TIMER;
    RPM.IGF_Channel = TIM_CHANNEL_1;
    RPM.tim.refClock = TIM_CLOCK / (RPM.tim.htim->Init.Prescaler + 1);
    HAL_TIM_Base_Start_IT(RPM.tim.htim);
    HAL_TIM_IC_Start_IT(RPM.tim.htim, RPM.IGF_Channel);
}
void nECU_RPM_Calc(void) // calculate RPM based on IGF signal
{
    uint32_t CurrentCCR = HAL_TIM_ReadCapturedValue(RPM.tim.htim, RPM.IGF_Channel);
    /* Calculate difference */
    uint16_t Difference = 0; // in miliseconds
    if (RPM.IGF_prevCCR > CurrentCCR)
    {
        Difference = ((RPM.tim.htim->Init.Period + 1 - RPM.IGF_prevCCR) + CurrentCCR);
    }
    else
    {
        Difference = (CurrentCCR - RPM.IGF_prevCCR);
    }
    RPM.IGF_prevCCR = CurrentCCR;
    RPM.frequency = RPM.tim.refClock / Difference;
    RPM.RPM = RPM.frequency * 120;
}
void nECU_RPM_DeInit(void) // stop
{
    HAL_TIM_Base_Stop_IT(RPM.tim.htim);
    HAL_TIM_IC_Stop_IT(RPM.tim.htim, RPM.IGF_Channel);
}

void nECU_Table_Set(Interpol_Table *Table, const float *Axis, const float *Values1, const float *Values2, uint32_t length) // fill table with constants
{
    Table->size = length;
    for (uint8_t i = 1; i < length + 1; i++)
    {
        Table->Table[i][1] = Axis[i - 1];
        Table->Table[i][2] = Values1[i - 1];
        Table->Table[i][3] = Values2[i - 1];
    }
    // first and last value set to properly interpolate
    Table->Table[0][1] = 0;
    Table->Table[0][2] = Values1[0];
    Table->Table[0][3] = Values2[0];
    Table->Table[length + 1][1] = 999999999;
    Table->Table[length + 1][2] = Values1[length - 1];
    Table->Table[length + 1][3] = Values2[length - 1];
}
void nECU_Table_Get(float *input, Interpol_Table *Table, float *Out1, float *Out2) // find value corresponding to table
{
    /* find index */
    uint8_t index = 1;
    for (uint8_t i = 1; i < FFT_THRESH_TABLE_LEN; i++)
    {
        if (Table->Table[i][1] <= *input)
        {
            index++;
        }
        else
        {
            break;
        }
    }
    *Out1 = nECU_Table_Interpolate(&(Table->Table[index][1]), &(Table->Table[index][2]), &(Table->Table[index + 1][1]), &(Table->Table[index + 1][2]), input);
    *Out2 = nECU_Table_Interpolate(&(Table->Table[index][1]), &(Table->Table[index][3]), &(Table->Table[index + 1][1]), &(Table->Table[index + 1][3]), input);
}
float nECU_Table_Interpolate(float *Ax, float *Ay, float *Bx, float *By, float *X) // function to interpolate linearly between two points A&B where
{
    if (*X > *Bx)
    {
        return *By;
    }
    else if (*X < *Ax)
    {
        return *Ay;
    }
    else
    {
        float Y = 0;
        Y = *Ay * (*Bx - *X);
        Y += *By * (*X - *Ax);
        Y /= (*Bx - *Ax);
        return Y;
    }
}
bool nECU_Table_Interpolate_Test(void) // test interpolation method
{
    float Ax = 1000, Ay = 1, Bx = 2000, By = 2, X = 1200, Y = 1.2;
    float result = nECU_Table_Interpolate(&Ax, &Ay, &Bx, &By, &X);
    if (Y - result)
    {
        return true;
    }
    return false;
}
