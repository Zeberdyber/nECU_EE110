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
    float nECU_correctToVref(float input);

    /*ADC 1*/
    bool nECU_InputAnalog_ADC1_Start(nECU_ADC1_ID ID);
    bool nECU_InputAnalog_ADC1_Stop(nECU_ADC1_ID ID);
    void nECU_InputAnalog_ADC1_Routine(nECU_ADC1_ID ID);
    float nECU_InputAnalog_ADC1_getValue(nECU_ADC1_ID ID); // returns output value
    /*ADC 2*/
    bool nECU_InputAnalog_ADC2_Start(nECU_ADC2_ID ID);
    bool nECU_InputAnalog_ADC2_Stop(nECU_ADC2_ID ID);
    void nECU_InputAnalog_ADC2_Routine(nECU_ADC2_ID ID);
    float nECU_InputAnalog_ADC2_getValue(nECU_ADC2_ID ID); // returns output value

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_ANALOG_H_ */