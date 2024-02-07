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
#include "nECU_adc.h"

    extern OnBoardLED LED_L, LED_R;

/* Definitions */
#define SYSTEST_INDICATOR LED_L  // indicator for systests
#define SYSTEST_DO_FLASH false   // choose to do flas tests !they will delete any saved data!
#define CODETEST_INDICATOR LED_R // indicator for systests

#define ERROR_HALT true       // should program stop on error
#define ERROR_BLINK_TIMES 5   // number of blinks when error is detected
#define ERROR_BLINK_SPEED 250 // delay time in ms

    /* Function Prototypes */
    /* general */
    void nECU_tests_error_display(OnBoardLED *inst); // on board LEDs display

    /* system tests */
    bool nECU_systest_Flash_SpeedCalibration(void); // test both read and write to flash memory
    bool nECU_systest_Flash_UserSettings(void);     // test both read and write to flash memory
    void nECU_systest_run(void);                    // run tests of type systest
    void nECU_systest_error(void);                  // function to call when error detected

    /* code tests */
    bool nECU_codetest_Flash_compdecompBool(void); // test nECU_compressBool and nECU_decompressBool
    bool nECU_codetest_ADC_AvgSmooth(void);        // test script for general functions
    void nECU_codetest_run(void);                  // run tests of type codetest
    void nECU_codetest_error(void);                // function to call when error detected

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TESTS_H_ */