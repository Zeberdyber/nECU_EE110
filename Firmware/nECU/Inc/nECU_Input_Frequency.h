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
#define VSS_MAX_SPEED 220           // in km/h maximal speed that car can go
#define VSS_SMOOTH_ALPHA 0.15       // smoothing factor for the data, the higher alpha the higher smoothing
#define VSS_SMOOTH_BUFFER_LENGTH 75 // length of smoothing buffer

#define IGF_MAX_RPM_RATE 3000 // rpm/s rate; used for missfire detection
#define IGF_MAX_RPM 10000     // maximal rpm allowed

    /* typedef */

    /* Function Prototypes */
    /* VSS - Vehicle Speed Sensor */
    uint8_t *nECU_VSS_GetPointer(void);                // returns pointer to resulting data
    bool nECU_VSS_Init(void);                          // initialize VSS structure
    void nECU_VSS_Update(void);                        // update VSS structure
    void nECU_VSS_Validate(void);                      // checks if recived signal is correct
    void nECU_VSS_DeInit(void);                        // deinitialize VSS structure
    void nECU_VSS_DetectZero(TIM_HandleTypeDef *htim); // detect if zero km/h
    /* IGF - Ignition feedback */
    bool nECU_IGF_Init(void);   // initialize and start
    void nECU_IGF_Calc(void);   // calculate RPM based on IGF signal, detect missfire
    void nECU_IGF_DeInit(void); // stop
    /* General */
    void nECU_Frequency_Start(void);  // start of frequency input functions
    void nECU_Frequency_Stop(void);   // stop of frequency input functions
    void nECU_Frequency_Update(void); // update of frequency input functions

#ifdef __cplusplus
}
#endif

#endif /* _NECU_INPUT_FREQUENCY_H_ */