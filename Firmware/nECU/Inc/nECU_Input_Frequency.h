/**
 ******************************************************************************
 * @file    nECU_Input_Frequency.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_Input_Frequency.c file
 */
#ifndef _NECU_INPUT_FREQUENCY_H_
#define _NECU_INPUT_FREQUENCY_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_tim.h"

/* Definitions */
#define VSS_PULSES_PER_KM 5000      // number of pulses that will be recived for a kilometer traveled
#define VSS_SMOOTH_BUFFER_LENGTH 75 // length of smoothing buffer

    /* typedef */

    /* Function Prototypes */
    bool nECU_FreqInput_Start(nECU_Freq_ID ID);
    bool nECU_FreqInput_Stop(nECU_Freq_ID ID);
    void nECU_FreqInput_Routine(nECU_Freq_ID ID);

    float nECU_FreqInput_getValue(nECU_Freq_ID ID);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_FREQUENCY_H_ */