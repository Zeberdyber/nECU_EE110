/**
 ******************************************************************************
 * @file    nECU_tests.c
 * @brief   This file provides code for self diagnosis of code.
 ******************************************************************************
 */

#include "nECU_tests.h"

bool nECU_systest_SpeedCalibration(void) // test both read and write to flash memory
{
    float T1 = 1.0, T2 = 9.99, T3 = -0.0015, T4 = 181516.2;
    nECU_saveSpeedCalibration(&T1, &T1, &T1, &T4);
    float R1, R2, R3, R4;
    nECU_readSpeedCalibration(&R1, &R2, &R3, &R4);
    if (R1 == T1 && R2 == T2 && R3 == T3 && R4 == T4)
    {
        return true;
    }
    return false;
}
bool nECU_systest_UserSettings(void) // test both read and write to flash memory
{
    bool d0, d1;
    d0 = true;
    d1 = false;
    nECU_saveUserSettings(&d0, &d1);
    nECU_readUserSettings(&d0, &d1);
    if (d0 != true || d1 != false)
    {
        return false;
    }

    d0 = false;
    d1 = true;
    nECU_saveUserSettings(&d0, &d1);
    nECU_readUserSettings(&d0, &d1);
    if (d0 != false || d1 != true)
    {
        return false;
    }

    return true;
}
bool nECU_codetest_compdecompBool(void) // test nECU_compressBool and nECU_decompressBool
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