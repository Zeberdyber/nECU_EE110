/**
 ******************************************************************************
 * @file    nECU_flash.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_flash.c file
 */

#ifndef _NECU_FLASH_H_
#define _NECU_FLASH_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "stm32f4xx.h"
#include "string.h"
#include "nECU_debug.h"

/* Definitions */
#define FLASH_DATA_START_ADDRESS 0x080E0000                                                          // address of sector 11 of flash memory
#define FLASH_DATA_END_ADDRESS 0x080FFFFF                                                            // end address of sector 11 of flash memory
#define FLASH_MINIMUM_RUN_TIME 1000                                                                  // to allow debugger to work
#define FLASH_DATA_START_ADDR_SPEED (FLASH_DATA_START_ADDRESS)                                       // start address for Speed Calibration data
#define FLASH_DATA_START_ADDR_USER (FLASH_DATA_START_ADDR_SPEED + sizeof(nECU_SpeedCalibrationData)) // start address for User Settings data
#define FLASH_DATA_START_ADDR_DEBUGQUE (FLASH_DATA_START_ADDR_USER + sizeof(nECU_UserSettings))      // start address of Debug Que data

    /* Function Prototypes */
    /* Speed calibration data functions (flash function interface) */
    bool nECU_Flash_SpeedCalibration_save(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);
    bool nECU_Flash_SpeedCalibration_read(float *Sensor1, float *Sensor2, float *Sensor3, float *Sensor4);

    /* User settings data functions (flash function interface) */
    bool nECU_Flash_UserSettings_save(bool *pAntiLag, bool *pTractionOFF);
    bool nECU_Flash_UserSettings_read(bool *pAntiLag, bool *pTractionOFF);

    /* Debug que (flash function interface) */
    bool nECU_Flash_DebugQue_save(nECU_Debug_error_que *que);
    bool nECU_Flash_DebugQue_read(nECU_Debug_error_que *que);

    /* Flash functions */
    static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector(void);       // clean flash sector
    static HAL_StatusTypeDef nECU_FLASH_cleanFlashSector_check(void); // check if erase was successful
    static HAL_StatusTypeDef nECU_FLASH_getAllMemory(void);           // get data from flash
    static HAL_StatusTypeDef nECU_FLASH_saveFlashSector(void);        // save everything, then read to RAM

    /* Interface functions */
    bool nECU_FLASH_Start(void);
    bool nECU_FLASH_Stop(void);
    bool nECU_FLASH_Erase(void);

#ifdef __cplusplus
}
#endif

#endif /* _NECU_CAN_H_ */