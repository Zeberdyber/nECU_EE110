/**
 ******************************************************************************
 * @file    nECU_tests.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_tests.c file
 */
#ifndef _NECU_TESTS_H_
#define _NECU_TESTS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "stdbool.h"
#include "nECU_flash.h"

    /* Definitions */

    /* typedef */

    /* Function Prototypes */
    /* nECU_flash */
    bool nECU_systest_SpeedCalibration(void); // test both read and write to flash memory
    bool nECU_systest_UserSettings(void);     // test both read and write to flash memory
    bool nECU_codetest_compdecompBool(void);  // test nECU_compressBool and nECU_decompressBool

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TESTS_H_ */