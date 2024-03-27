/**
 ******************************************************************************
 * @file    nECU_tests.c
 * @brief   This file provides code for self diagnosis of code.
 ******************************************************************************
 */

#include "nECU_tests.h"

/* general */
void nECU_tests_error_display(OnBoardLED *inst) // on board LEDs display
{
    nECU_LED_SetState(inst, GPIO_PIN_RESET);
    do
    {
        for (uint8_t count = 0; count < ERROR_BLINK_TIMES * 2; count++)
        {
            HAL_Delay(ERROR_BLINK_SPEED);
            nECU_LED_FlipState(inst);
        }
    } while (ERROR_HALT);
    nECU_LED_SetState(inst, GPIO_PIN_RESET);
}

/* system tests */
bool nECU_systest_Flash_SpeedCalibration(void) // test both read and write to flash memory
{
    float T1 = 1.0, T2 = 9.99, T3 = -0.0015, T4 = 181516.2;
    nECU_saveSpeedCalibration(&T1, &T2, &T3, &T4);
    float R1 = 0.0, R2 = 0.0, R3 = 0.0, R4 = 0.0;
    nECU_readSpeedCalibration(&R1, &R2, &R3, &R4);
    if (R1 - T1 + R2 - T2 + R3 - T3 + R4 - T4 != 0)
    {
        return true;
    }
    T1 = 1.0;
    nECU_saveSpeedCalibration(&T1, &T1, &T1, &T1);
    nECU_readSpeedCalibration(&R1, &R2, &R3, &R4);
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
    nECU_saveUserSettings(&T0, &T1);
    nECU_readUserSettings(&R0, &R1);
    if (R0 != true || R1 != false)
    {
        return true;
    }

    T0 = false;
    T1 = true;
    nECU_saveUserSettings(&T0, &T1);
    nECU_readUserSettings(&R0, &R1);
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
        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        nECU_FLASH_cleanFlashSector();
        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        if (nECU_systest_Flash_UserSettings())
        {
            // nECU_systest_error();
        }

        HAL_Delay(FLASH_MINIMUM_RUN_TIME);
        if (nECU_systest_Flash_SpeedCalibration())
        {
            nECU_systest_error();
        }
    }
}
void nECU_systest_error(void) // function to call when error detected
{
    nECU_tests_error_display(&SYSTEST_INDICATOR);
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
void nECU_codetest_run(void) // run tests of type codetest
{
    if (nECU_codetest_Flash_compdecompBool())
    {
        nECU_codetest_error();
    }
    if (nECU_codetest_ADC_AvgSmooth())
    {
        nECU_codetest_error();
    }
    if (nECU_codetest_Speed_SensorUpdate())
    {
        nECU_codetest_error();
    }
}
void nECU_codetest_error(void) // function to call when error detected
{
    nECU_tests_error_display(&CODETEST_INDICATOR);
}
