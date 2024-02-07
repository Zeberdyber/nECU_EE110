/**
 ******************************************************************************
 * @file    nECU_tests.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_tests.c file
 */

/*
 * Test function types:
 *     -> systests => test components of IC
 *     -> selftest => startup test procedure
 *     -> codetest => test pure code functions
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
#include "nECU_debug.h"

    /* Definitions */

        /* Function Prototypes */
    /* nECU_flash */
    bool nECU_systest_Flash_SpeedCalibration(void); // test both read and write to flash memory
    bool nECU_systest_Flash_UserSettings(void);     // test both read and write to flash memory
    bool nECU_codetest_Flash_compdecompBool(void);  // test nECU_compressBool and nECU_decompressBool

    void nECU_systest_run(void);  // run tests of type systest
    void nECU_codetest_run(void); // run tests of type codetest

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TESTS_H_ */