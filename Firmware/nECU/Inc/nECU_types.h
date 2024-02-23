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

typedef struct
{
    uint32_t value;  // current value
    uint32_t preset; // preset value
} Counter;

typedef struct
{
    TIM_HandleTypeDef *htim;  // periperal pointer
    float refClock;           // in Hz (pre calculated on initialization)
    float period;             // in ms (pre calculated on initialization)
    uint32_t Channel_List[4]; // list of configured channels
    uint8_t Channel_Count;    // number of actively used channels
} nECU_Timer;
typedef struct
{
    GPIO_PinState State; // Current pin state
    GPIO_TypeDef *GPIOx; // GPIO pin port
    uint16_t GPIO_Pin;   // Pin of GPIO
} GPIO_struct;
typedef struct
{
    uint32_t timeSet;   // time to wait in ticks
    uint32_t timeStart; // tick at which delay started
    bool done;          // delay ended
    bool active;        // delay counting or ended
} nECU_Delay;

/* ADCs */
typedef struct
{
    bool working;                                // flag to indicate ADC status
    bool callback_half, callback_full, overflow; // callback flags to indicate DMA buffer states
} nECU_ADC_Status;
typedef struct
{
    uint16_t in_buffer[GENERAL_DMA_LEN];        // input buffer (from DMA)
    uint16_t out_buffer[GENERAL_CHANNEL_COUNT]; // output buffer (after processing, like average)
    nECU_ADC_Status status;                     // statuses
} nECU_ADC1;
typedef struct
{
    uint16_t in_buffer[SPEED_DMA_LEN];        // input buffer (from DMA)
    uint16_t out_buffer[SPEED_CHANNEL_COUNT]; // output buffer (after processing, like average)
    nECU_ADC_Status status;                   // statuses
} nECU_ADC2;
typedef struct
{
    uint16_t in_buffer[KNOCK_DMA_LEN]; // input buffer (from DMA)
    nECU_ADC_Status status;            // statuses
    bool *UART_transmission;           // flag, UART transmission ongoing
    TIM_HandleTypeDef *samplingTimer;  // timer used for sampling
} nECU_ADC3;
typedef struct
{
    Counter conv_divider; // callback divider
    uint16_t *ADC_data;   // pointer to ADC data
    uint16_t temperature; // output data (real_tem*100)
    bool upToDate;        // flag that indicates that data is up to date
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
    BUTTON_MODE_OFF = 0,        // turned off
    BUTTON_MODE_RESTING = 1,    // brightness decreased
    BUTTON_MODE_GO_TO_REST = 2, // animation of fading down to rest state
    BUTTON_MODE_ANIMATED = 3,   // breathing or blinking
    BUTTON_MODE_ON = 5,         // turned on
    BUTTON_MODE_NONE
} ButtonLight_Mode;
typedef struct
{
    nECU_Timer Timer;                 // Timer used for Input Capture
    HAL_TIM_ActiveChannel Channel_IC; // Timers channel used for Input Capture
    GPIO_struct buttonPin;            // GPIO structure
    uint32_t RisingCCR;               // CCR captured at rising edge
    Button_ClickType Type;            // current detected type of click
    bool newType;                     // flag to indicate new type detected
} ButtonInput;
typedef struct
{
    uint8_t speed;             // animation speed 0-100%
    uint16_t count;            // number of animation cycles to do
    uint16_t state, prevState; // current and previous state of animation
} Button_Animation;
typedef struct
{
    nECU_Timer Timer;           // Timer used for light PWM
    uint16_t CCR;               // value of current bightness [timer value]
    float Brightness;           // value of set brightness [in %]
    Button_Animation Breathing; // variables for breathing animation
    Button_Animation Blinking;  // variables for blinking animation
    ButtonLight_Mode Mode;      // Modes according to typedef
    ButtonLight_Mode ModePrev;  // Previous state of Mode
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
    GPIO_struct LEDPin;       // pin of the LED structure
    nECU_Delay delay;         // delay structure for non-blocking blinking
    bool blinking, blinkPrev; // ON/OFF for blinking animation
} OnBoardLED;

/* EGT */
typedef enum
{
    EGT_CYL1 = 1,
    EGT_CYL2 = 2,
    EGT_CYL3 = 3,
    EGT_CYL4 = 4,
    EGT_CYL_NONE
} EGT_Sensor_ID;
typedef struct
{
    SPI_HandleTypeDef *hspi;                         // peripheral pointer
    GPIO_struct CS_pin;                              // GPIO for Chip Select
    uint8_t in_buffer[4];                            // recived data buffer
    bool data_Pending;                               // data was recieved and is pending to be decoded
    bool OC_Fault, SCG_Fault, SCV_Fault, Data_Error; // Thermocouple state / data validity
    float InternalTemp, TcTemp;                      // temperature of ADC chip, thermocouple temperature
    uint16_t EGT_Temperature;                        // output temperature
} MAX31855;
typedef struct
{
    MAX31855 TC1, TC2, TC3, TC4;                                     // sensor structures
    MAX31855 *EGT_CurrentObj;                                        // current object pointer (for sensor asking loop)
    bool EGT_FirstSensor, EGT_Initialized, EGT_CommunicationOngoing; // flags to indicate state
    uint8_t EGT_CurrentSensor;                                       // number of current sensor (for sensor asking loop)
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
    uint8_t boolByte1; // byte that holds states of user settings (combined boolean to bytes)
} nECU_UserSettings;
typedef struct
{
    nECU_SpeedCalibrationData speedData;
    nECU_UserSettings userData;
} nECU_FlashContent;

/* Frames */
typedef struct
{
    uint32_t Mailbox;           // mailbox responsible for message
    CAN_TxHeaderTypeDef Header; // header data of CAN frame
    uint8_t Send_Buffer[8];     // message content
} nECU_CAN_TxFrame;
typedef enum
{
    nECU_Frame_Speed = 0,
    nECU_Frame_EGT = 1,
    nECU_Frame_Stock = 2,
    nECU_Frame_NULL
} nECU_CAN_Frame_ID;
typedef struct
{
    nECU_CAN_TxFrame can_data; // peripheral data
    nECU_Timer send_timing;    // timer structure

    bool LunchControl1, LunchControl2, LunchControl3, RollingLunch; // flags from decoding

    // outside variables
    bool *Cranking, *Fan_ON, *Lights_ON, *IgnitionKey;
    bool *Antilag, *TractionOFF, *ClearEngineCode;
    bool *TachoShow1, *TachoShow2, *TachoShow3;
    uint16_t *LunchControlLevel;
    uint16_t *Speed_FL, *Speed_FR, *Speed_RL, *Speed_RR;
} Frame0_struct;
typedef struct
{
    nECU_CAN_TxFrame can_data; // peripheral data
    nECU_Timer send_timing;    // timer structure

    // outside variables
    uint16_t *EGT1, *EGT2, *EGT3, *EGT4;
    uint8_t *TachoVal1, *TachoVal2, *TachoVal3;
    uint16_t *TuneSelector;
} Frame1_struct;
typedef struct
{
    nECU_CAN_TxFrame can_data; // peripheral data
    nECU_Timer send_timing;    // timer structure

    // outside variables
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
    bool LevelWaiting;
    bool *CycleDoneFlag;

    uint8_t Level;
    uint8_t RetardOut;
    float RetardPerc;

    Knock_Interpol_Table thresholdMap;

    Knock_FFT fft;

    // regression
    nECU_Timer regres;
} nECU_Knock;

/* Menu */
typedef enum
{
    TACHO_SHOW_1 = 1,
    TACHO_SHOW_2 = 2,
    TACHO_SHOW_3 = 3,
    TACHO_SHOW_NONE
} Tacho_ID;
typedef struct
{
    bool showPending;          // flag indicates that data was not displayed
    uint16_t *input_value;     // pointer to source value
    uint16_t prev_input;       // stored previous value
    uint8_t output_value;      // output value that will be sent over CAN
    uint8_t output_multiplier; // factor to multiply the output value by
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
typedef enum
{
    SPEED_SENSOR_FRONT_LEFT = 1,
    SPEED_SENSOR_FRONT_RIGHT = 2,
    SPEED_SENSOR_REAR_LEFT = 3,
    SPEED_SENSOR_REAR_RIGHT = 4,
    SPEED_SENSOR_NONE_ID
} Speed_Sensor_ID; // Here update if connected otherwise
typedef struct
{
    uint16_t Buffer[SPEED_AVERAGE_BUFFER_SIZE]; // buffer to be filled with ADC data
    uint8_t BufferIndex;                        // current index at which data should be plugged
} SpeedAverage;
typedef struct
{
    uint16_t *InputData;    // pointer to ADC input data
    uint8_t *WheelSetup;    // pointer to current wheel setup selected
    float SensorCorrection; // factor by which result will be multiplied to correct by calibration
    uint16_t SpeedData;     // output speed
    uint16_t SpeedDataSlow; // output speed, averaged from multiple measuerements
    uint16_t WheelCirc;     // circumference of wheel (according to wheel setup)
    SpeedAverage Average;   // averaging structure
} Speed_Sensor;
typedef struct
{
    bool active;          // routine is running
    bool initialized;     // properly initialized ready to collect data
    bool averageReady[4]; // flags indicating end of data collection on each sensor
} CalibrateRoutine;

/* Stock */
typedef struct
{
    uint16_t ADC_MeasuredMax, ADC_MeasuredMin; // limits of ADC readout
    float OUT_MeasuredMax, OUT_MeasuredMin;    // limits of resulting output
    float offset, factor;                      // offset that is added to result, factor by which output is multiplied
} AnalogSensorCalibration;
typedef struct
{
    AnalogSensorCalibration calibrationData; // calibration structure
    uint16_t *ADC_input;                     // pointer to ADC input data
    uint16_t decimalPoint;                   // decimal point indicator to store floating point as uint16_t
    float outputFloat;                       // resulting value in float
    uint16_t output16bit;                    // resulting value in 16bit
    uint8_t output8bit;                      // resulting value in 8bit
} AnalogSensor_Handle;
typedef struct
{
    nECU_Timer Heater;              // timer structure
    float Heater_Infill;            // infill of PWM signal
    AnalogSensor_Handle sensor;     // Analog sensor structure
    uint8_t *Coolant;               // pointer to coolant temperature
    float Infill_max, Infill_min;   // ranges of heater infill
    float Coolant_max, Coolant_min; // ranges of coolant temperature
} Oxygen_Handle;
typedef struct
{
    nECU_Timer tim;       // timer structure
    uint32_t VSS_prevCCR; // previous CCR readout
    float frequency;      // calculated frequency of interrupts
    uint8_t Speed;        // resulting speed
} VSS_Handle;
typedef struct
{
    nECU_Timer tim;       // timer structure
    uint32_t IGF_prevCCR; // previous CCR readout
    float frequency;      // calculated frequency of interrupts
    uint16_t RPM;         // resulting RPM
} IGF_Handle;
typedef enum
{
    INPUT_CRANKING_ID = 1,
    INPUT_FAN_ON_ID = 2,
    INPUT_LIGHTS_ON_ID = 3,
    INPUT_NONE_ID
} stock_inputs_ID;
typedef struct
{
    GPIO_struct Cranking, Fan_ON, Lights_ON; // structues for stock GPIO inputs
    bool Cranking_b, Fan_ON_b, Lights_ON_b;  // boolean states of GPIO inputs
} stock_GPIO;

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
    nECU_Timer tim;
    bool error, warning;   // flags
    uint32_t counter_ms;   // watchdog counter
    uint64_t counter_max;  // value which determines error state
    uint32_t previousTick; // helper variable for counter_ms calculation
} nECU_tim_Watchdog;

#endif // _nECU_types_H_