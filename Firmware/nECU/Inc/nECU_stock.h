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
    uint8_t *nECU_OX_GetPointer(void);          // returns pointer to resulting data
    bool nECU_OX_Init(void);                    // initialize narrowband lambda structure
    void nECU_OX_Update(void);                  // update narrowband lambda structure
    void nECU_OX_DeInit(void);                  // deinitialize narrowband lambda structure
    static void nECU_OX_PWM_Set(float *infill); // function to set PWM according to set infill
    /* GPIO inputs */
    bool nECU_stock_GPIO_Init(void);                      // initialize structure variables
    void nECU_stock_GPIO_update(void);                    // update structure variables
    bool *nECU_stock_GPIO_getPointer(stock_inputs_ID id); // return pointers of structure variables
    /* Immobilizer */
    bool *nECU_Immo_getPointer(void); // returns pointer to immobilizer valid
    /* GPIO */
    bool nECU_GPIO_Init(GPIO_struct *inst, uint16_t Pin, GPIO_TypeDef *Port); // Initialize structure - clear
    bool nECU_GPIO_Read(GPIO_struct *inst);                                   // Read value and save to structure
    bool nECU_GPIO_Write(GPIO_struct *inst, bool state);                      // Write value

#ifdef __cplusplus
}
#endif

#endif /* _NECU_STOCK_H_ */