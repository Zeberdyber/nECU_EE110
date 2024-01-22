/**
 ******************************************************************************
 * @file    nECU_Knock.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_Knock.c file
 */
#ifndef _NECU_KNOCK_H_
#define _NECU_KNOCK_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_adc.h"
#include "nECU_tim.h"
#include "nECU_table.h"

#define ARM_MATH_CM4
#include "arm_math.h"

/* Definitions */
#define KNOCK_BETA 10        // value in [%/s] of knock retard regression
#define KNOCK_LEVEL 5        // multiplier how much will steps will be taken for severe knock
#define KNOCK_STEP 5         // in % how much should be retarded in one step
#define FFT_LENGTH 2048      // length of data passed to FFT code and result precision
#define KNOCK_FREQUENCY 8000 // in Hz

    /* typedef */
    typedef struct
    {
        TIM_HandleTypeDef *htim;
        float refClock;
        float period; // in ms
    } TimerKnock;

    typedef struct
    {
        arm_rfft_fast_instance_f32 Handler;
        float BufIn[FFT_LENGTH];
        float BufOut[FFT_LENGTH];
        bool flag;
        uint16_t Index;
        uint16_t KnockIndex;
    } Knock_FFT;

    typedef struct
    {
        bool LevelWaiting;
        bool *CycleDoneFlag;

        uint8_t Level;
        uint8_t RetardOut;
        float RetardPerc;
        uint8_t StepTime; // in miliseconds

        Interpol_Table thresholdMap;

        Knock_FFT fft;

        // regression
        TimerKnock regres;
    } nECU_Knock;

    /* Knock detection */
    void nECU_Knock_Init(void);                           // initialize and start
    void nECU_Knock_ADC_Callback(uint16_t *input_buffer); // periodic callback
    void nECU_Knock_UpdatePeriodic(void);                 // function to perform time critical knock routines, call at regression timer interrupt
    void nECU_Knock_DetectMagn(void);                     // function to detect knock based on ADC input
    void nECU_Knock_Evaluate(float *magnitude);           // check if magnitude is of knock range
    void nECU_Knock_DeInit(void);                         // stop
    uint8_t *nECU_Knock_GetPointer(void);                 // returns pointer to knock retard percentage

#ifdef __cplusplus
}
#endif

#endif /* _NECU_KNOCK_H_ */