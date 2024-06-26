/**
 ******************************************************************************
 * @file    nECU_frames.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_frames.c file
 */
#ifndef _NECU_FRAMES_H_
#define _NECU_FRAMES_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes */
#include "main.h"
#include "nECU_types.h"
#include "nECU_Speed.h"
#include "nECU_EGT.h"
#include "nECU_can.h"
#include "nECU_menu.h"
#include "nECU_stock.h"
    /* Definitions */

    /* Function Prototypes */
    void Frame0_Init(void);                                                                               // initialization of data structure
    void Frame0_Update(void);                                                                             // update variables for frame 0
    void Frame0_PrepareBuffer(void);                                                                      // prepare Tx buffer for CAN transmission
    void Frame0_ComposeWord(uint8_t *buffer, bool *B1, bool *B2, bool *B3, bool *B4, uint16_t *Val12Bit); // function to create word for use in frame 0
    bool Frame0_Working(void);                                                                            // return if frame is operational

    void Frame1_Init(void);                                                         // initialization of data structure
    void Frame1_Update(void);                                                       // update variables for frame 1
    void Frame1_PrepareBuffer(void);                                                // prepare Tx buffer for CAN transmission
    void Frame1_ComposeWord(uint8_t *buffer, uint8_t *Val6Bit, uint16_t *Val10Bit); // function to create word for use in frame 1
    bool Frame1_Working(void);                                                      // return if frame is operational

    void Frame2_Init(void);          // initialization of data structure
    void Frame2_Update(void);        // update variables for frame 2
    void Frame2_PrepareBuffer(void); // prepare Tx buffer for CAN transmission
    bool Frame2_Working(void);       // return if frame is operational

#ifdef __cplusplus
}
#endif

#endif /* _NECU_FRAMES_H_ */