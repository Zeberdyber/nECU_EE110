/**
 ******************************************************************************
 * @file    nECU_frames.h
 * @brief   This file contains all the function prototypes for
 *          the nECU_frames.c file
 *          Turned out to be main file for data updates and module startup :)
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
#include "nECU_EGT.h"
#include "nECU_can.h"
#include "nECU_menu.h"
#include "nECU_stock.h"
#include "nECU_Input_Analog.h"
#include "nECU_Input_Frequency.h"

/* Definitions */
#define MAX_VAL_10BIT 1023    // maximal possible value for a 10bit uint
#define FRAME_MAP_OFFSET -100 // offset to the value

    /* Function Prototypes */
    bool Frame0_Start(void);                                                                                     // initialization of data structure
    void Frame0_Routine(void);                                                                                   // update variables for frame 0
    void Frame0_PrepareBuffer(void);                                                                             // prepare Tx buffer for CAN transmission
    static void Frame0_ComposeWord(uint8_t *buffer, bool *B1, bool *B2, bool *B3, bool *B4, uint16_t *Val12Bit); // function to create word for use in frame 0

    bool Frame1_Start(void);                                                               // initialization of data structure
    void Frame1_Routine(void);                                                             // update variables for frame 1
    void Frame1_PrepareBuffer(void);                                                       // prepare Tx buffer for CAN transmission
    static void Frame1_ComposeWord(uint8_t *buffer, uint8_t *Val6Bit, uint16_t *Val10Bit); // function to create word for use in frame 1

    bool Frame2_Start(void);         // initialization of data structure
    void Frame2_Routine(void);       // update variables for frame 2
    void Frame2_PrepareBuffer(void); // prepare Tx buffer for CAN transmission

    void nECU_Frame_TX_done(nECU_CAN_Frame_ID ID);        // callback after can TX is done
    uint8_t *nECU_Frame_getPointer(nECU_CAN_Frame_ID ID); // returns pointer to output buffer

#ifdef __cplusplus
}
#endif

#endif /* _NECU_FRAMES_H_ */