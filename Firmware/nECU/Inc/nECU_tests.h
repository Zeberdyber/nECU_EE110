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

/* Definitions */
#define TEST_ENABLE true       // turn all tests on
#define SYSTEST_DO_FLASH false // choose to do flash tests !they will delete any saved data!

#define ERROR_HALT true       // should program stop on error
#define ERROR_BLINK_TIMES 5   // number of blinks when error is detected
#define ERROR_BLINK_SPEED 250 // delay time in ms

    /* Function Prototypes */
    /* Fail indication */
    void nECU_tests_fail_init(void);     // initializes indicator structure
    void nECU_tests_fail_deinit(void);   // deinitializes indicator structure
    void nECU_tests_error_display(void); // on board LEDs display

    /* system tests */
    bool nECU_systest_Flash_SpeedCalibration(void); // test both read and write to flash memory
    bool nECU_systest_Flash_UserSettings(void);     // test both read and write to flash memory
    void nECU_systest_run(void);                    // run tests of type systest
    void nECU_systest_error(void);                  // function to call when error detected

    /* code tests */
    bool nECU_codetest_Flash_compdecompBool(void); // test nECU_compressBool and nECU_decompressBool
    bool nECU_codetest_ADC_AvgSmooth(void);        // test script for general functions
    bool nECU_codetest_Speed_SensorUpdate(void);   // function to test Speed functions
    bool nECU_expSmooth_test(void);                // tests nECU_expSmooth() function
    bool nECU_averageSmooth_test(void);            // tests nECU_averageSmooth() function
    bool nECU_averageExpSmooth_test(void);         // tests nECU_averageExpSmooth() function
    void nECU_codetest_run(void);                  // run tests of type codetest
    void nECU_codetest_error(void);                // function to call when error detected

    /* General */
    void nECU_test(void); // perform tests

    /* temporary tests */
    void nECU_IGF_Test(void); // checks readout compared to CAN frame data

#ifdef __cplusplus
}
#endif

#endif /* _NECU_TESTS_H_ */