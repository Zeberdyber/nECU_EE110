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
bool nECU_systest_Flash_SpeedCalibration(void) // test both read and write to flash memory
{
    float T1 = 1.0, T2 = 9.99, T3 = -0.0015, T4 = 181516.2;
    nECU_Flash_SpeedCalibration_save(&T1, &T2, &T3, &T4);
    float R1 = 0.0, R2 = 0.0, R3 = 0.0, R4 = 0.0;
    nECU_Flash_SpeedCalibration_read(&R1, &R2, &R3, &R4);
    if (R1 - T1 + R2 - T2 + R3 - T3 + R4 - T4 != 0)
    {
        return true;
    }
    T1 = 1.0;
    nECU_Flash_SpeedCalibration_save(&T1, &T1, &T1, &T1);
    nECU_Flash_SpeedCalibration_read(&R1, &R2, &R3, &R4);
    if (R1 + R2 + R3 + R4 != T1 * 4)
    {
        return true;
    }

    return false;
}
bool nECU_systest_Flash_UserSettings(void) // test both read and write to flash memory
{
    bool T0, T1, R0, R1;
    T0 = true;
    T1 = false;
    nECU_Flash_UserSettings_save(&T0, &T1);
    nECU_Flash_UserSettings_read(&R0, &R1);
    if (R0 != true || R1 != false)
    {
        return true;
    }

    T0 = false;
    T1 = true;
    nECU_Flash_UserSettings_save(&T0, &T1);
    nECU_Flash_UserSettings_read(&R0, &R1);
    if (R0 != false || R1 != true)
    {
        return true;
    }

    return false;
}
void nECU_systest_run(void) // run tests of type systest
{
    if (SYSTEST_DO_FLASH)
    {
        printf("Systest started: ");
        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        printf("XX");
        nECU_FLASH_Erase();
        printf("XX");
        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        printf("XX");
        if (nECU_systest_Flash_UserSettings())
        {
            printf("\nTest failed on UserSettings\n");
            nECU_systest_error();
        }
        printf("XX");

        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        if (nECU_systest_Flash_SpeedCalibration())
        {
            printf("\nTest failed on SpeedCalibration\n");
            nECU_systest_error();
        }
        printf("   DONE!");
    }
    else
    {
        printf("Systest turned off\n");
    }
}
void nECU_systest_error(void) // function to call when error detected
{
    nECU_tests_error_display();
}

/* code tests */
bool nECU_codetest_Flash_compdecompBool(void) // test nECU_compressBool and nECU_decompressBool
{
    bool bufferIn[8] = {true, true, false, false, true, false, true, false};
    uint8_t byte = 0;
    nECU_compressBool(bufferIn, &byte);
    if (byte != 0b11001010) // check compression result
    {
        return false;
    }

    bool bufferOut[8];
    nECU_decompressBool(&byte, bufferOut);
    for (uint8_t i = 0; i < 8; i++) // check decompression result
    {
        if (bufferIn[i] != bufferOut[i])
        {
            return false;
        }
    }
    return true;
}
bool nECU_codetest_ADC_AvgSmooth(void) // test script for general functions
{
    ADC_HandleTypeDef testADC;
    testADC.Init.NbrOfConversion = 2;

    uint16_t inputBuf[] = {10, 0, 20, 1, 30, 3, 40, 32768, 10, 0, 20, 1, 30, 3, 40, 32768};
    uint16_t outputBuf[] = {0, 0, 0, 0}; // tool large buffer to test data spilage

    nECU_ADC_AverageDMA(&testADC, inputBuf, 16, &outputBuf[1], 1); // no smoothing

    // check limits for spilage
    if (outputBuf[0] != 0)
    {
        return true;
    }
    if (outputBuf[3] != 0)
    {
        return true;
    }

    // check for correct answers
    if (outputBuf[1] != 25)
    {
        return true;
    }
    if (outputBuf[2] != 8193)
    {
        return true;
    }

    uint16_t avgInputNew[] = {100, 4000};                    // new data for smoothing
    nECU_ADC_expSmooth(avgInputNew, &outputBuf[1], 2, 0.75); // perform smoothing with new data

    // check limits for spilage
    if (outputBuf[0] != 0)
    {
        return true;
    }
    if (outputBuf[3] != 0)
    {
        return true;
    }

    // check for correct answers
    if (outputBuf[1] != 81)
    {
        return true;
    }
    if (outputBuf[2] != 5048)
    {
        return true;
    }

    return false;
}
bool nECU_codetest_Speed_SensorUpdate(void) // function to test Speed functions
{
    // Success values
    uint16_t Corr_Circ = Wheel_Circ_Set_1; // according to definitions
    uint16_t Corr_Speed = 150;             // 150km/h

    uint16_t Input = VoltsToADC(((Corr_Speed)*Speed_ToothCount) / (3.6 * Wheel_Circ_Set_1 * Speed_HzTomVolts)); // ADC value to plug into test, calculated from known Voltage

    for (uint8_t i = 0; i < Speed_DecimalPoint; i++) // move decimal point
    {
        Corr_Speed = Corr_Speed * 10;
    }

    // Create test object
    Speed_Sensor Test_Obj;
    Test_Obj.InputData = &Input;
    uint8_t setup = 1;
    Test_Obj.WheelSetup = &setup;
    Test_Obj.SensorCorrection = 1.0;

    // Perfrom tests
    for (uint8_t i = 0; i < 255; i++) // do it 255 times to rule out any smoothing
    {
        Speed_SensorUpdate(&Test_Obj);
    }
    Speed_SensorUpdate(&Test_Obj);

    // Check results
    if (Test_Obj.WheelCirc != Corr_Circ)
    {
        return true;
    }
    int32_t result = Test_Obj.SpeedData - Corr_Speed;
    if (result > 50 || result < -50) // Allow for 0.5km/h error due to rounding
    {
        return true;
    }

    return false;
}
bool nECU_expSmooth_test(void) // tests nECU_expSmooth() function
{
    uint16_t A, B;
    float alpha;

    A = 100;
    B = 0;
    alpha = 0.5;
    nECU_expSmooth(&A, &B, &B, alpha);
    if (B != 50)
    {
        return false;
    }
    A = 0;
    B = 100;
    alpha = 0.5;
    nECU_expSmooth(&A, &B, &B, alpha);
    if (B != 50)
    {
        return false;
    }
    A = 100;
    B = 200;
    alpha = 0.2;
    nECU_expSmooth(&A, &B, &B, alpha);
    if (B != 180)
    {
        return false;
    }
    A = 100;
    B = 200;
    alpha = 0.8;
    nECU_expSmooth(&A, &B, &B, alpha);
    if (B != 120)
    {
        return false;
    }
    return true;
}
bool nECU_averageSmooth_test(void) // tests nECU_averageSmooth() function
{
    uint16_t buffer[10];
    uint8_t bufferLen = 10;
    for (uint8_t i = 0; i < 2 * 10; i += 2) // fills buffer with integers == {0,2,4,6,8,10,12,14,16,18}
    {
        buffer[i / 2] = i;
    }
    uint16_t A, B;
    A = 0;
    B = 0;
    nECU_averageSmooth(buffer, &A, &B, bufferLen);
    // buffer should have values == {2,4,6,8,10,12,14,16,18,0}
    if (buffer[bufferLen - 1] != A) // check if last value matches
    {
        return false;
    }
    if (B != 9) // check if result correct
    {
        return false;
    }
    A = 22;
    nECU_averageSmooth(buffer, &A, &B, bufferLen);
    // buffer should have values == {4,6,8,10,12,14,16,18,0,22}
    if (B != 11) // check if result correct
    {
        return false;
    }
    return true;
}
bool nECU_averageExpSmooth_test(void) // tests nECU_averageExpSmooth() function
{
    uint16_t buffer[10];
    uint8_t bufferLen = 10;
    for (uint8_t i = 0; i < 2 * 10; i += 2) // fills buffer with integers == {0,2,4,6,8,10,12,14,16,18}
    {
        buffer[i / 2] = i;
    }
    uint16_t A, B;
    float alpha = 0.5;
    A = 12;
    B = 24;
    nECU_averageExpSmooth(buffer, &A, &B, bufferLen, alpha);
    // buffer should have values == {2,4,6,8,10,12,14,16,18,18}
    if (B != 10) // check if result correct
    {
        return false;
    }
    A = 128;
    alpha = 0.8;
    nECU_averageExpSmooth(buffer, &A, &B, bufferLen, alpha);
    // buffer should have values == {4,6,8,10,12,14,16,18,18,104}
    if (B != 21) // check if result correct
    {
        return false;
    }
    return true;
}
void nECU_codetest_run(void) // run tests of type codetest
{
    printf("Code test started: ");
    if (nECU_codetest_Flash_compdecompBool())
    {
        printf("\nTest failed on Flash_compdecompBool\n");
        nECU_codetest_error();
    }
    printf("XX");

    if (nECU_codetest_ADC_AvgSmooth())
    {
        printf("\nTest failed on ADC_AvgSmooth\n");
        nECU_codetest_error();
    }
    printf("XX");

    if (nECU_codetest_Speed_SensorUpdate())
    {
        printf("\nTest failed on Speed_SensorUpdate\n");
        nECU_codetest_error();
    }
    printf("XX");

    if (!nECU_expSmooth_test())
    {
        printf("\nTest failed on expSmooth_test\n");
        nECU_codetest_error();
    }
    printf("XX");

    if (!nECU_averageSmooth_test())
    {
        printf("\nTest failed on averageSmooth_test\n");
        nECU_codetest_error();
    }
    printf("XX");

    if (!nECU_averageExpSmooth_test())
    {
        printf("\nTest failed on averageExpSmooth_test\n");
        nECU_codetest_error();
    }
    printf("   DONE!");
}
void nECU_codetest_error(void) // function to call when error detected
{
    nECU_tests_error_display();
}

/* General */
void nECU_test(void) // perform tests
{
    if (TEST_ENABLE)
    {
        nECU_codetest_run();
        nECU_systest_run();
    }
    else
    {
        printf("Tests not configured\n");
    }
}

/* temporary tests */
void nECU_IGF_Test(void) // checks readout compared to CAN frame data
{
#include "nECU_stock.h"
#define IGF_test_max_difference 100 // in RPM
    extern IGF_Handle IGF;
    static bool IGF_test_Initialized = false;
    static uint16_t *IGF_test_CAN_RPM;
    static uint8_t IGF_test_out_of_bounds = 0;
    // call this function only on CAN RX as latest data from CAN arrives
    // use only to check code, then delete this function
    if (IGF_test_Initialized == false)
    {
        IGF_test_CAN_RPM = nECU_CAN_getPointer_RPM();
        nECU_IGF_Start();
        IGF_test_Initialized = true;
    }
    float CAN_RPM = *IGF_test_CAN_RPM * 20; // 20 is a divider value from MaxxECU config
    float RPM_difference = 0;               // difference between RPM data

    nECU_IGF_Update();     // update value
    if (CAN_RPM > IGF.RPM) // calculate difference
    {
        RPM_difference = CAN_RPM - IGF.RPM;
    }
    else
    {
        RPM_difference = IGF.RPM - CAN_RPM;
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
