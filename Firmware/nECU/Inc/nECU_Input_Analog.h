/**
 ******************************************************************************
 * @file    nECU_Input_Analog.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_Input_Analog.c file
 */
#ifndef _NECU_INPUT_ANALOG_H_
#define _NECU_INPUT_ANALOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_tim.h"
#include "nECU_debug.h"

/* Definitions */
#define INTERNAL_TEMP_UPDATE_DELAY 1000 // once per second (1000ms)

    /* typedef */

    /* Function Prototypes */
    static float nECU_correctToVref(float input);

    bool nECU_InputAnalog_Start(nECU_ADC1_ID ID);
    bool nECU_InputAnalog_Stop(nECU_ADC1_ID ID);
    void nECU_InputAnalog_Routine(nECU_ADC1_ID ID);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_ANALOG_H_ */