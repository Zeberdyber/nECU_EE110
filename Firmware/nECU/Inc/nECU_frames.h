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
#include "stdbool.h"
#include "nECU_Speed.h"
#include "nECU_EGT.h"
#include "nECU_can.h"
#include "HumanInterface.h"
#include "nECU_stock.h"
    /* Definitions */

    /* typedef */
    union FloatToBytes
    {
        float floatValue;
        uint8_t byteArray[4];
    };
    union Int16ToBytes
    {
        uint16_t UintValue;
        int16_t IntValue;
        uint8_t byteArray[2];
    };

    typedef struct
    {
        bool Cranking, Fan_ON, Lights_ON, IgnitionKey;
        bool LunchControl1, LunchControl2, LunchControl3, RollingLunch;

        // outside variables
        bool *Antilag, *TractionOFF, *ClearEngineCode;
        bool *TachoShow1, *TachoShow2, *TachoShow3;
        uint16_t *LunchControlLevel;
        uint16_t *Speed1, *Speed2, *Speed3, *Speed4;
    } Frame0_struct;

    typedef struct
    {
        uint16_t *EGT1, *EGT2, *EGT3, *EGT4;
        uint8_t *TachoVal1, *TachoVal2, *TachoVal3;
        uint16_t *TuneSelector;
    } Frame1_struct;

    typedef struct
    {
        uint8_t *Backpressure, *OX_Val;
        uint16_t *MAP_Stock_10bit;
        uint8_t *Knock;
        uint8_t *VSS;
    } Frame2_struct;

    /* Function Prototypes */
    void Frame0_Init(bool *pTachoShow1, bool *pTachoShow2, bool *pTachoShow3, bool *pAntilag, bool *pTractionOFF, bool *pClearEngineCode, uint16_t *pLunchControlLevel); // initialization of data structure
    void Frame0_Update(void);                                                                                                                                            // update variables for frame 0
    void Frame0_PrepareBuffer(void);                                                                                                                                     // prepare Tx buffer for CAN transmission
    void Frame0_ComposeWord(uint8_t *buffer, bool *B1, bool *B2, bool *B3, bool *B4, uint16_t *Val12Bit);                                                                // function to create word for use in frame 0

    void Frame1_Init(uint8_t *pTachoVal1, uint8_t *pTachoVal2, uint8_t *pTachoVal3, uint16_t *pTuneSelector); // initialization of data structure
    void Frame1_Update(void);                                                                                 // update variables for frame 1
    void Frame1_PrepareBuffer(void);                                                                          // prepare Tx buffer for CAN transmission
    void Frame1_ComposeWord(uint8_t *buffer, uint8_t *Val6Bit, uint16_t *Val10Bit);                           // function to create word for use in frame 1

    void Frame2_Init(uint8_t *pBackpressure, uint8_t *pOX_Val, uint16_t *pMAP_Stock_10bit, uint8_t *pKnock, uint8_t *pVSS); // initialization of data structure
    void Frame2_Update(void);                                                                                               // update variables for frame 2
    void Frame2_PrepareBuffer(void);                                                                                        // prepare Tx buffer for CAN transmission

#ifdef __cplusplus
}
#endif

#endif /* _NECU_FRAMES_H_ */