/**
 ******************************************************************************
 * @file    nECU.h
 * @brief   This file contains all the data types
 *
 */

#ifndef _nECU_types_H_
#define _nECU_types_H_

#include "stdbool.h"
#include "main.h"

#define ARM_MATH_CM4
#include "arm_math.h"

#define FFT_THRESH_TABLE_LEN 5 // length of table

#define APB2_CLOCK 42000000 // APB2 clock speed

#define GENERAL_CHANNEL_COUNT 8                                                                                                                                                                                                // number of initialized channels of GENERAL_ADC
#define GENERAL_ADC_CLOCKDIVIDER 8                                                                                                                                                                                             // values of a clock divider for this peripheral
#define GENERAL_ADC_SAMPLINGCYCLES 480                                                                                                                                                                                         // number of cycles that it takes to conver single channel
#define GENERAL_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                                        // how many cycles per conversion is added due to precission
#define GENERAL_TARGET_UPDATE 25                                                                                                                                                                                               // time in ms how often should values be updated
#define GENERAL_DMA_LEN ((uint16_t)(((APB2_CLOCK * GENERAL_TARGET_UPDATE) / 1000) / ((GENERAL_ADC_RESOLUTIONCYCLES + GENERAL_ADC_SAMPLINGCYCLES) * GENERAL_ADC_CLOCKDIVIDER * GENERAL_CHANNEL_COUNT))) * GENERAL_CHANNEL_COUNT // length of DMA buffer for GENERAL_ADC

#define SPEED_CHANNEL_COUNT 4                                                                                                                                                                                    // number of initialized channels of SPEED_ADC
#define SPEED_ADC_CLOCKDIVIDER 8                                                                                                                                                                                 // values of a clock divider for this peripheral
#define SPEED_ADC_SAMPLINGCYCLES 480                                                                                                                                                                             // number of cycles that it takes to conver single channel
#define SPEED_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                            // how many cycles per conversion is added due to precission
#define SPEED_TARGET_UPDATE 25                                                                                                                                                                                   // time in ms how often should values be updated
#define SPEED_DMA_LEN ((uint16_t)(((APB2_CLOCK * SPEED_TARGET_UPDATE) / 1000) / ((SPEED_ADC_RESOLUTIONCYCLES + SPEED_ADC_SAMPLINGCYCLES) * SPEED_ADC_CLOCKDIVIDER * SPEED_CHANNEL_COUNT))) * SPEED_CHANNEL_COUNT // length of DMA buffer for GENERAL_ADC
#define SPEED_AVERAGE_BUFFER_SIZE 100                                                                                                                                                                            // number of conversions to average

#define KNOCK_CHANNEL_COUNT 1 // number of initialized channels of KNOCK_ADC
#define KNOCK_DMA_LEN 512     // length of DMA buffer for KNOCK_ADC
#define FFT_LENGTH 2048       // length of data passed to FFT code and result precision

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
    float Table[FFT_THRESH_TABLE_LEN + 2][3];
    uint32_t size;
} Knock_Interpol_Table;

/* ADCs */
typedef struct
{
    bool working;
    bool callback_half, callback_full, overflow;
} nECU_ADC_Status;
typedef struct nECU_ADC1
{
    uint16_t buffer[GENERAL_DMA_LEN];
    uint16_t out[GENERAL_CHANNEL_COUNT];
    nECU_ADC_Status status;
} nECU_ADC1;
typedef struct nECU_ADC2
{
    uint16_t buffer[SPEED_DMA_LEN];
    uint16_t out[SPEED_CHANNEL_COUNT];
    nECU_ADC_Status status;
} nECU_ADC2;
typedef struct nECU_ADC3
{
    uint16_t buffer[KNOCK_DMA_LEN];
    nECU_ADC_Status status;
    bool *UART_transmission;
    TIM_HandleTypeDef *samplingTimer;
} nECU_ADC3;
typedef struct
{
    uint8_t conv_count;
    uint16_t *ADC_data;
    uint16_t temperature;
    bool upToDate;
} nECU_InternalTemp;

/* Buttons */
typedef enum
{
    CLICK_TYPE_SINGLE_CLICK = 1,
    CLICK_TYPE_DOUBLE_CLICK = 2,
    CLICK_TYPE_HOLD = 3,
    CLICK_TYPE_NONE
} Button_ClickType;
typedef enum
{
    RED_BUTTON_ID = 1,
    ORANGE_BUTTON_ID = 2,
    GREEN_BUTTON_ID = 3,
    NONE_BUTTON_ID
} Button_ID;
typedef enum
{
    BUTTON_MODE_OFF = 0,
    BUTTON_MODE_RESTING = 1,
    BUTTON_MODE_GO_TO_REST = 2,
    BUTTON_MODE_ANIMATED = 3, // breathing or blinking
    BUTTON_MODE_ON = 5,
    BUTTON_MODE_NONE
} ButtonLight_Mode;
typedef struct
{
    TIM_HandleTypeDef *Timer;         // Timer used for Input Capture
    HAL_TIM_ActiveChannel Channel_IC; // Timers channel used for Input Capture
    uint32_t Channel;                 // Timers channel used for Timer general functions
    GPIO_PinState State;              // Current pin state
    GPIO_TypeDef *GPIOx;              // GPIO pin port
    uint16_t GPIO_Pin;                // Pin on which button is
    uint32_t RisingCCR;               // CCR captured at rising edge
    Button_ClickType Type;            // current detected type of click
    float refClock;                   // clock reference for real time calculations
    bool newType;                     // flag to indicate new type detected
} ButtonInput;
typedef struct
{
    TIM_HandleTypeDef *Timer;   // Timer used for light PWM
    uint32_t Channel;           // Timers channel used
    uint16_t CCR;               // value of current bightness [timer value]
    float UpdateInterval;       // Period of how often lights will be updated in ms
    float Brightness;           // value of set brightness [in %]
    uint8_t BreathingSpeed;     // value determines speed of animation
    uint8_t BlinkingSpeed;      // value determines speed of animation
    uint16_t BreathingCount;    // how many Breaths to do
    uint16_t BlinkingCount;     // how many Blinks to do
    int16_t BreathingState;     // internal control value: 0 -OFF, 255 -ON
    int16_t BreathingStatePrev; // Previous state of BreathingState
    uint8_t BlinkingState;      // internal control value: 0 -OFF, 1 -ON
    uint8_t BlinkingStatePrev;  // Previous state of BlinkingState
    ButtonLight_Mode Mode;      // Modes according to typedef
    uint8_t ModePrev;           // Previous state of Mode
    uint8_t WaitingFlag;        // Indicates that light is waiting to go resting
    float Time;                 // internal time for resting animation in ms
} ButtonLight;
typedef struct
{
    ButtonLight light;
    ButtonInput input;
} Button;

/* Debug */
typedef struct
{
    GPIO_TypeDef *GPIOx;      // GPIO group of the LED
    uint16_t GPIO_Pin;        // pin of the LED
    GPIO_PinState State;      // actual state of the output
    GPIO_PinState BlinkState; // state of the output (for blinking)
    bool blinking;            // flag when blinking
    uint32_t LastTick;        // tick at which last blink update performed
} OnBoardLED;

/* EGT */
typedef struct
{
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *GPIOx;
    uint16_t GPIO_Pin;

    uint8_t buffer[4];
    uint8_t data_Pending;

    bool OC_Fault, SCG_Fault, SCV_Fault, Data_Error;
    float InternalTemp, TcTemp;
    uint16_t EGT_Temperature;
} MAX31855;
typedef struct
{
    MAX31855 TC1, TC2, TC3, TC4;
    MAX31855 *EGT_CurrentObj;
    bool EGT_FirstSensor, EGT_Initialized, EGT_CommunicationOngoing;
    uint8_t EGT_CurrentSensor;
} nECU_EGT;

/* Flash */
typedef struct
{
    float SpeedSensor1;
    float SpeedSensor2;
    float SpeedSensor3;
    float SpeedSensor4;

} nECU_SpeedCalibrationData;

typedef struct
{
    uint8_t boolByte1;
} nECU_UserSettings;

typedef struct
{
    nECU_SpeedCalibrationData speedData;
    nECU_UserSettings userData;
} nECU_FlashContent;

/* Frames */

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

/* Knock */
typedef struct
{
    arm_rfft_fast_instance_f32 Handler;
    float BufIn[FFT_LENGTH];
    float BufOut[FFT_LENGTH];
    bool flag;
    uint16_t Index;
    uint16_t KnockIndex;
} Knock_FFT;
typedef struct
{
    TIM_HandleTypeDef *htim;
    float refClock;
    float period; // in ms
} TimerKnock;
typedef struct
{
    bool LevelWaiting;
    bool *CycleDoneFlag;

    uint8_t Level;
    uint8_t RetardOut;
    float RetardPerc;
    uint8_t StepTime; // in miliseconds

    Knock_Interpol_Table thresholdMap;

    Knock_FFT fft;

    // regression
    TimerKnock regres;
} nECU_Knock;

/* Menu */
typedef struct
{
    bool showPending;
    uint16_t *input_value;
    uint16_t prev_input;
    uint8_t output_value;
    uint8_t output_multiplier;
} TachoValue;
typedef struct
{
    // output variables
    bool Antilag, TractionOFF, ClearEngineCode;
    uint16_t LunchControlLevel, TuneSelector;
    // internal variables
    uint16_t MenuLevel;
    bool initialized;
} ButtonMenu;

/* Speed */
typedef struct
{
    uint16_t Buffer[SPEED_AVERAGE_BUFFER_SIZE];
    uint8_t BufferIndex;
} SpeedAverage;
typedef struct
{
    uint16_t *InputData;
    uint8_t *WheelSetup;
    float SensorCorrection;
    uint16_t SpeedData;
    uint16_t SpeedDataPrev;
    uint16_t SpeedDataSlow;
    uint16_t WheelCirc;
    SpeedAverage Average;
} Speed_Sensor;
typedef struct
{
    bool active;
    bool initialized;
    bool averageReady[4];
} CalibrateRoutine;

/* Stock */
typedef struct
{
    uint16_t ADC_MeasuredMax, ADC_MeasuredMin;
    float OUT_MeasuredMax, OUT_MeasuredMin;
    float offset, factor;
} AnalogSensorCalibration;

typedef struct
{
    AnalogSensorCalibration calibrationData;
    uint16_t *ADC_input;
    uint16_t decimalPoint;
    float outputFloat;
    uint16_t output16bit;
    uint8_t output8bit;
} AnalogSensor_Handle;

typedef struct
{
    TIM_HandleTypeDef *Timer; // Timer used for PWM
    uint32_t Channel;         // Timers channel used
    float Infill;             // infill of PWM signal
} PWM_Out;

typedef struct
{
    PWM_Out Heater;
    AnalogSensor_Handle sensor;
    uint8_t *Coolant;
    float Infill_max, Infill_min;
    float Coolant_max, Coolant_min;
} Oxygen_Handle;

typedef struct
{
    TIM_HandleTypeDef *htim;
    float refClock;
    float period; // in ms
} TimerStock;

typedef struct
{
    TimerStock tim;
    uint32_t VSS_Channel;
    uint32_t VSS_prevCCR;
    float frequency;
    uint8_t Speed;
    uint16_t watchdogCount;
} VSS_Handle;

typedef struct
{
    TimerStock tim;
    uint32_t IGF_Channel;
    uint32_t IGF_prevCCR;
    float frequency;
    uint16_t RPM, prevRPM;
} IGF_Handle;

/* Timer */
typedef enum
{
    TIM_OK = 0,
    TIM_NONE = 1,
    TIM_ERROR = 2,
    TIM_NULL
} nECU_TIM_State;

typedef struct
{
    uint32_t timeSet;
    uint32_t timeStart;
    bool done;
    bool active;
} nECU_Delay;

typedef struct
{
    TIM_HandleTypeDef *htim;
    float refClock;           // in Hz (pre calculated on initialization)
    float period;             // in ms (pre calculated on initialization)
    uint32_t Channel_List[4]; // list of configured channels
    uint8_t Channel_Count;    // number of actively used channels
} nECU_Timer;

typedef struct
{
    nECU_Timer tim;
    bool error, warning;   // flags
    uint32_t counter_ms;   // watchdog counter
    uint64_t counter_max;  // value which determines error state
    uint32_t previousTick; // helper variable for counter_ms calculation
} nECU_tim_Watchdog;

#endif // _nECU_types_H_