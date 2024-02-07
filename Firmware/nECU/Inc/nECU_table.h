/**
 ******************************************************************************
 * @file    nECU_table.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_table.c file
 */
#ifndef _NECU_TABLE_H_
#define _NECU_TABLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"

    /* Definitions */

    /* Interpolated reference table */
    void nECU_Table_Set(Knock_Interpol_Table *Table, const float *Axis, const float *Values1, const float *Values2, uint32_t length); // fill table with constants
    void nECU_Table_Get(float *input, Knock_Interpol_Table *Table, float *Out1, float *Out2);                                         // find value corresponding to table
    float nECU_Table_Interpolate(float *Ax, float *Ay, float *Bx, float *By, float *X);                                               // function to interpolate linearly between two points A&B where
    bool nECU_Table_Interpolate_Test(void);                                                                                           // test interpolation method

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TABLE_H_ */