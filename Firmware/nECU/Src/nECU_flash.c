/**
 ******************************************************************************
 * @file    nECU_flash.c
 * @brief   This file provides code for user defined flash access functions.
 ******************************************************************************
 */

#include "nECU_flash.h"

static nECU_FlashContent Flash = {0};
extern nECU_ProgramBlockData D_Flash;     // diagnostic and flow control data
extern nECU_ProgramBlockData D_Debug_Que; // data to  check if que was initialized

/* Speed calibration data functions (flash function interface) */
bool nECU_saveSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        status |= true;
        return status;
    }

    float tempSensor[] = {*Sensor1, *Sensor2, *Sensor3, *Sensor4};

    if (memcmp(&(Flash.speedData), &tempSensor, sizeof(tempSensor))) // break if they are the same
    {
        return status;
    }

    // copy config if needed to buffer
    Flash.speedData.SpeedSensor1 = *Sensor1;
    Flash.speedData.SpeedSensor2 = *Sensor2;
    Flash.speedData.SpeedSensor3 = *Sensor3;
    Flash.speedData.SpeedSensor4 = *Sensor4;

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_readSpeedCalibration(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        status |= true;
        return status;
    }

    // copy data to outputs
    *Sensor1 = Flash.speedData.SpeedSensor1;
    *Sensor2 = Flash.speedData.SpeedSensor2;
    *Sensor3 = Flash.speedData.SpeedSensor3;
    *Sensor4 = Flash.speedData.SpeedSensor4;

    nECU_Debug_ProgramBlockData_Update(&D_Flash);

    return status;
}

/* User settings data functions (flash function interface) */
bool nECU_saveUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        status |= true;
        return status;
    }

    // prepare buffers for comparison
    bool compressed[8] = {*pAntiLag, *pTractionOFF, 0, 0, 0, 0, 0, 0};
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
    if (memcmp(compressed, decompressed, sizeof(compressed))) // break if they are the same
    {
        return status;
    }

    // compress data to byte
    nECU_compressBool(compressed, &(Flash.userData.boolByte1));

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_readUserSettings(bool *pAntiLag, bool *pTractionOFF)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        status |= true;
        return status;
    }

    // decompress to output
    bool decompressed[8];
    nECU_decompressBool(&(Flash.userData.boolByte1), decompressed);
    *pAntiLag = decompressed[0];
    *pTractionOFF = decompressed[1];

    nECU_Debug_ProgramBlockData_Update(&D_Flash);

    return status;
}

/* Debug que (flash function interface) */
bool nECU_saveDebugQue(nECU_Debug_error_que *que)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        status |= true;
        return status;
    }
    // Set pointer to array -> debug que is so long it is pointless to store it in RAM
    Flash.DebugQueData = que;

    status |= (nECU_FLASH_saveFlashSector() != HAL_OK); // save and validate

    return status;
}
bool nECU_readDebugQue(nECU_Debug_error_que *que)
{
    bool status = false;

    // check if data was initialized
    if (!(D_Flash.Status & D_BLOCK_WORKING))
    {
        Flash.DebugQueData = que;
        status |= true;
        return status;
    }

    memcpy(Flash.DebugQueData, (const void *)FLASH_DATA_START_ADDR_DEBUGQUE, sizeof(nECU_Debug_error_que)); // user settings to RAM

    if (memcmp(Flash.DebugQueData, (const void *)FLASH_DATA_START_ADDR_DEBUGQUE, sizeof(nECU_Debug_error_que))) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, false);
        status |= true;
    }

    nECU_Debug_ProgramBlockData_Update(&D_Flash);

    return status;
}

/* Flash functions */
static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector(void) // clean flash sector
{
    HAL_StatusTypeDef status = HAL_OK;

    status |= HAL_FLASH_Unlock();
    FLASH_Erase_Sector(FLASH_SECTOR_11, FLASH_VOLTAGE_RANGE_3);
    FLASH_FlushCaches();
    status |= HAL_FLASH_Lock();
    status |= nECU_FLASH_cleanFlashSector_check();

    if (status != HAL_OK)
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_ERASE, true);
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector_check(void) // check if erase was successful
{
    HAL_StatusTypeDef status = HAL_OK;

    if (memcmp((const void *)0, (const void *)FLASH_DATA_START_ADDRESS, (FLASH_DATA_END_ADDRESS - FLASH_DATA_START_ADDRESS)))
    {
        status = HAL_ERROR;
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_getAllMemory(void) // get data from flash
{
    HAL_StatusTypeDef status = HAL_OK;

    /* copy data to the RAM */
    memcpy(&(Flash.speedData), (const void *)FLASH_DATA_START_ADDRESS, sizeof(nECU_SpeedCalibrationData)); // speed data to RAM
    memcpy(&(Flash.userData), (const void *)FLASH_DATA_START_ADDR_USER, sizeof(nECU_UserSettings));        // user settings to RAM

    /* Check if reading was successful */
    if (memcmp(&(Flash.speedData), (const void *)FLASH_DATA_START_ADDRESS, sizeof(nECU_SpeedCalibrationData)) != 0) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, false);
        status |= HAL_ERROR;
    }
    if (memcmp(&(Flash.userData), (const void *)FLASH_DATA_START_ADDR_USER, sizeof(nECU_UserSettings)) != 0) // check if reading was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, false);
        status |= HAL_ERROR;
    }

    return status;
}
static HAL_StatusTypeDef nECU_FLASH_saveFlashSector(void) // save everything, then read to RAM
{
    HAL_StatusTypeDef status = HAL_OK;

    status |= nECU_FLASH_cleanFlashSector();                                             // prepare memory for a save
    uint16_t byte_count = sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings); // define buffer length

    if (D_Debug_Que.Status & D_BLOCK_WORKING)
    {
        byte_count += sizeof(nECU_Debug_error_que); // add debug error que if it was initialized
    }

    uint8_t data[byte_count]; // create buffer (it has to be devidable by 4 so be carefull)

    /* copy data to the buffer */
    memcpy(&data[0], &(Flash.speedData), sizeof(nECU_SpeedCalibrationData));                        // copy speed data
    memcpy(&data[sizeof(nECU_SpeedCalibrationData)], &(Flash.userData), sizeof(nECU_UserSettings)); // copy user settings data
    if (D_Debug_Que.Status & D_BLOCK_WORKING)                                                       // copy debug que if it was initialized
    {
        memcpy(&data[sizeof(nECU_SpeedCalibrationData) + sizeof(nECU_UserSettings)], Flash.DebugQueData, sizeof(nECU_Debug_error_que));
    }

    /* Write the data to flash memory */
    status |= HAL_FLASH_Unlock();
    for (int i = 0; i < byte_count; i += 4)
    {
        status |= HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_DATA_START_ADDRESS + i, *(uint32_t *)((uint8_t *)data + i));
    }
    status |= HAL_FLASH_Lock();

    if (status != HAL_OK)
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, true);
    }

    status |= nECU_FLASH_getAllMemory(); // update RAM

    if (memcmp(&data[0], (const void *)FLASH_DATA_START_ADDRESS, byte_count) != 0) // check if save was successful
    {
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_SPEED, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_USER, true);
        nECU_Debug_FLASH_error(nECU_FLASH_ERROR_DBGQUE, true);
    }

    nECU_Debug_ProgramBlockData_Update(&D_Flash);

    return status;
}

/* Interface functions */
bool nECU_FLASH_Init(void) // initialize FLASH code
{
    bool status = false;

    status |= (nECU_FLASH_getAllMemory() != HAL_OK);

    if ((D_Flash.Status == D_BLOCK_STOP) && !status)
    {
        D_Flash.Status |= D_BLOCK_INITIALIZED_WORKING;
    }

    return status;
}
bool nECU_FLASH_Erase(void) // erases whole sector
{
    bool status = false;

    status |= (nECU_FLASH_cleanFlashSector() != HAL_OK);

    return status;
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
