/**
 ******************************************************************************
 * @file    nECU_flash.c
 * @brief   This file provides code for user defined flash access functions.
 ******************************************************************************
 */

#include "nECU_flash.h"

nECU_FlashContent Flash;
bool FlashdataInitialized = false;

/* Speed calibration data functions (flash function interface) */
void nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy config if needed to buffer
    Flash.speedData.SpeedSensor1 = *Sensor1;
    Flash.speedData.SpeedSensor2 = *Sensor2;
    Flash.speedData.SpeedSensor3 = *Sensor3;
    Flash.speedData.SpeedSensor4 = *Sensor4;

    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // copy data to outputs
    *Sensor1 = Flash.speedData.SpeedSensor1;
    *Sensor2 = Flash.speedData.SpeedSensor2;
    *Sensor3 = Flash.speedData.SpeedSensor3;
    *Sensor4 = Flash.speedData.SpeedSensor4;
}

/* User settings data functions (flash function interface) */
void nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // compress data to byte
    bool compressed[8] = {*pAntiLag, *pTractionOFF, 0, 0, 0, 0, 0, 0};
    nECU_compressBool(compressed, &(Flash.userData.boolByte1));
    nECU_FLASH_saveFlashSector(); // save and validate
}
void nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    // check if data was initialized
    if (FlashdataInitialized == false)
    {
        nECU_FLASH_getAllMemory();
    }
    // decompress to output
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
    *pAntiLag = decompressed[0];
    *pTractionOFF = decompressed[1];
}

/* Flash functions */
void nECU_FLASH_cleanFlashSector(void) // clean flash sector
{
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_11, FLASH_VOLTAGE_RANGE_3);
    FLASH_FlushCaches();
    HAL_FLASH_Lock();
}
void nECU_FLASH_getAllMemory(void) // get data from flash
{
    /* copy data to the RAM */
    memcpy(&(Flash.speedData), (const void *)FLASH_DATA_START_ADDRESS, sizeof(nECU_SpeedCalibrationData));                            // speed data to RAM
    memcpy(&(Flash.userData), (const void *)FLASH_DATA_START_ADDRESS + sizeof(nECU_SpeedCalibrationData), sizeof(nECU_UserSettings)); // user settings to RAM
    if (FlashdataInitialized == false)
    {
        FlashdataInitialized = true;
    }
}
void nECU_FLASH_saveFlashSector(void) // save everything, then read to RAM
{
    nECU_FLASH_cleanFlashSector();                                                       // prepare memory for a save
    uint16_t byte_count = sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings); // define buffer length
    uint8_t data[byte_count];                                                            // create buffer (it has to be devidable by 4 so be carefull)

    /* copy data to the buffer */
    memcpy(&data[0], &(Flash.speedData), sizeof(nECU_SpeedCalibrationData));                        // copy speed data
    memcpy(&data[sizeof(nECU_SpeedCalibrationData)], &(Flash.userData), sizeof(nECU_UserSettings)); // copy user settings data

    /* Write the data to flash memory */
    HAL_FLASH_Unlock();
    for (int i = 0; i < byte_count; i += 4)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + i, *(uint32_t *)((uint8_t *)data + i));
    }
    HAL_FLASH_Lock();

    nECU_FLASH_getAllMemory(); // update RAM
}

/* Helper functions */
void nECU_compressBool(bool *bufferIn, uint8_t *out) // compress bool array to one byte
{
    // zero-out output
    *out = 0;

    // fill output with data
    for (uint8_t i = 0; i < 8; i++)
    {
        *out |= (bufferIn[i] & 1) << i;
    }
}
void nECU_decompressBool(uint8_t *in, bool *bufferOut) // decompress byte to bool array
{
    for (uint8_t i = 0; i < 8; i++)
    {
        bufferOut[i] = ((*in) >> i) & 1; // copy to outputs
    }
}
