/**
 ******************************************************************************
 * @file    nECU_tests.c
 * @brief   This file provides code for self diagnosis of code.
 ******************************************************************************
 */

#include "nECU_tests.h"

static OnBoardLED_Animate test_fail_animation = {0};
static bool Indicator_Initialized = false;

/* Fail indication */
void nECU_tests_fail_init(void) // initializes indicator structure
{
    if (Indicator_Initialized == false)
    {
        OnBoard_LED_Animation_Init(&test_fail_animation, LED_ANIMATE_TEST_ID);
        OnBoard_LED_L_Add_Animation(&test_fail_animation);
        Indicator_Initialized = true;
    }
}
void nECU_tests_fail_deinit(void) // deinitializes indicator structure
{
    if (Indicator_Initialized == true)
    {
        OnBoard_LED_L_Remove_Animation(&test_fail_animation);
        Indicator_Initialized = false;
    }
}
void nECU_tests_error_display(void) // on board LEDs display
{
    nECU_tests_fail_init();
    OnBoard_LED_Animation_BlinkStart(&test_fail_animation, ERROR_BLINK_SPEED, ERROR_BLINK_TIMES);
    while (test_fail_animation.blink_active == true)
    {
        HAL_Delay(1);
        OnBoard_LED_Update(); // keep on updating the LEDs
    }
    while (ERROR_HALT)
    {
        printf("Program stopped at test error. ERROR_HALT == true");
        HAL_Delay(UINT32_MAX); // stop execition
    }
    nECU_tests_fail_deinit();
}

/* system tests */
bool nECU_systest_run(void) // run tests of type systest
{
    bool status = false;
    printf("Systest started: ");
    if (SYSTEST_DO_FLASH)
    {
        if (!nECU_FLASH_test(true))
        {
            printf("\n\rTest failed on nECU_FLASH_test()\n\r");
            nECU_systest_error();
            status |= true;
            return status;
        }
    }
    else
        printf("\tnECU_FLASH_test turned off\t");
    printf("DONE!\n\r");
    return status;
}
void nECU_systest_error(void) // function to call when error detected
{
    nECU_tests_error_display();
}

/* code tests */
bool nECU_codetest_run(void) // run tests of type codetest
{
    bool status = false;
    printf("Code test started: \n\r");
    if (!nECU_DataProcessing_test(true))
    {
        printf("Test failed on nECU_DataProcessing_test()\n\r");
        nECU_codetest_error();
        status |= true;
    }
    printf(" DONE!\n\r");
    return status;
}
void nECU_codetest_error(void) // function to call when error detected
{
    nECU_tests_error_display();
}

/* General */
bool nECU_test(void) // perform tests
{
    bool status = false;
    if (TEST_ENABLE)
    {
        status |= nECU_codetest_run();
        status |= nECU_systest_run();
    }
    else
    {
        printf("Tests not configured\n");
    }
    return status;
}

/* temporary tests */
void nECU_IGF_Test(void) // checks readout compared to CAN frame data
{
#include "nECU_stock.h"
#define IGF_test_max_difference 100 // in RPM
    static bool IGF_test_Initialized = false;
    static uint8_t IGF_test_out_of_bounds = 0;
    // call this function only on CAN RX as latest data from CAN arrives
    // use only to check code, then delete this function
    if (IGF_test_Initialized == false)
    {
        nECU_CAN_Start();
        nECU_FreqInput_Start(FREQ_IGF_ID);
        IGF_test_Initialized = true;
    }
    float CAN_RPM = nECU_CAN_RX_getValue(CAN_RX_RPM_ID); // 20 is a divider value from MaxxECU config
    float RPM_difference = 0;                            // difference between RPM data

    nECU_FreqInput_Routine(FREQ_IGF_ID);                // update value
    if (CAN_RPM > nECU_FreqInput_getValue(FREQ_IGF_ID)) // calculate difference
    {
        RPM_difference = CAN_RPM - nECU_FreqInput_getValue(FREQ_IGF_ID);
    }
    else
    {
        RPM_difference = nECU_FreqInput_getValue(FREQ_IGF_ID) - CAN_RPM;
    }

    if (RPM_difference > IGF_test_max_difference)
    {
        IGF_test_out_of_bounds++;
    }

    if (IGF_test_out_of_bounds > 20)
    {
        IGF_test_out_of_bounds = 0;
        while (ERROR_HALT)
        {
            HAL_Delay(1); // get stuck in the loop
        }
    }
}
