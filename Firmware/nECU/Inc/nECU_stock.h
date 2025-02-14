/**
 ******************************************************************************
 * @file    nECU_stock.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_stock.c file
 */
#ifndef _NECU_STOCK_H_
#define _NECU_STOCK_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_Input_Analog.h"

    /* Definitions */
#define OXYGEN_HEATER_MAX 90   // maximum % infill of heater PWM
#define OXYGEN_HEATER_MIN 0    // maximum % infill of heater PWM
#define OXYGEN_COOLANT_MAX 80  // maximum degrees that will couse minimum infill
#define OXYGEN_COOLANT_MIN 40  // minimum degrees that will couse maximum infill
#define OXYGEN_DECIMAL_POINT 2 // how many numbers after decimal point

    /* Oxygen Sensor */
    bool nECU_OX_Start(void);   // initialize narrowband lambda structure
    void nECU_OX_Routine(void); // update narrowband lambda structure
    bool nECU_OX_Stop(void);    // deinitialize narrowband lambda structure

    float nECU_OX_GetValue(void); // returns pointer to resulting data

    /* GPIO inputs */
    bool nECU_DigitalInput_Start(nECU_DigiInput_ID ID);
    bool nECU_DigitalInput_Stop(nECU_DigiInput_ID ID);
    void nECU_DigitalInput_Routine(nECU_DigiInput_ID ID);

    bool nECU_DigitalInput_getValue(nECU_DigiInput_ID ID);

    /* Immobilizer */
    bool *nECU_Immo_getPointer(void); // returns pointer to immobilizer valid

#ifdef __cplusplus
}
#endif

#endif /* _NECU_STOCK_H_ */