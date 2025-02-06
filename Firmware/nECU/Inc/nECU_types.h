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

#define GENERAL_CHANNEL_COUNT 8                                                                                                                                                                                                          // number of initialized channels of GENERAL_ADC
#define GENERAL_ADC_CLOCKDIVIDER 8                                                                                                                                                                                                       // values of a clock divider for this peripheral
#define GENERAL_ADC_SAMPLINGCYCLES 480                                                                                                                                                                                                   // number of cycles that it takes to conver single channel
#define GENERAL_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                                                  // how many cycles per conversion is added due to precission
#define GENERAL_TARGET_UPDATE 25                                                                                                                                                                                                         // time in ms how often should values be updated
#define GENERAL_DMA_LEN (((uint16_t)(((APB2_CLOCK * GENERAL_TARGET_UPDATE) / 1000) / ((GENERAL_ADC_RESOLUTIONCYCLES + GENERAL_ADC_SAMPLINGCYCLES) * GENERAL_ADC_CLOCKDIVIDER * GENERAL_CHANNEL_COUNT))) / 2) * 2 * GENERAL_CHANNEL_COUNT // length of DMA buffer for GENERAL_ADC, '2' for divisibility by two

#define SPEED_CHANNEL_COUNT 4                                                                                                                                                                                              // number of initialized channels of SPEED_ADC
#define SPEED_ADC_CLOCKDIVIDER 8                                                                                                                                                                                           // values of a clock divider for this peripheral
#define SPEED_ADC_SAMPLINGCYCLES 480                                                                                                                                                                                       // number of cycles that it takes to conver single channel
#define SPEED_ADC_RESOLUTIONCYCLES 15                                                                                                                                                                                      // how many cycles per conversion is added due to precission
#define SPEED_TARGET_UPDATE 25                                                                                                                                                                                             // time in ms how often should values be updated
#define SPEED_DMA_LEN (((uint16_t)(((APB2_CLOCK * SPEED_TARGET_UPDATE) / 1000) / ((SPEED_ADC_RESOLUTIONCYCLES + SPEED_ADC_SAMPLINGCYCLES) * SPEED_ADC_CLOCKDIVIDER * SPEED_CHANNEL_COUNT))) / 2) * 2 * SPEED_CHANNEL_COUNT // length of DMA buffer for SPEED_ADC, '2' for divisibility by two
#define SPEED_AVERAGE_BUFFER_SIZE 100                                                                                                                                                                                      // number of conversions to average
#define VSS_SMOOTH_BUFFER_LENGTH 75                                                                                                                                                                                        // length of smoothing buffer

#define KNOCK_CHANNEL_COUNT 1 // number of initialized channels of KNOCK_ADC
#define KNOCK_DMA_LEN 512     // length of DMA buffer for KNOCK_ADC
#define FFT_LENGTH 2048       // length of data passed to FFT code and result precision

#define PC_UART_BUF_LEN 128 // length of buffer for UART transmission to PC

#define DEBUG_QUE_LEN 50                // number of debug messages that will be stored in memory
#define ONBOARD_LED_ANIMATION_QUE_LEN 5 // number of animation access points

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

/* UART */
typedef struct
{
    UART_HandleTypeDef *huart;
    uint8_t *message;
    uint8_t length;
    bool pending;
} nECU_UART;

typedef struct
{
    GPIO_PinState State; // Current pin state
    GPIO_TypeDef *GPIOx; // GPIO pin port
    uint16_t GPIO_Pin;   // Pin of GPIO
} GPIO_struct;

typedef enum
{
    TIM_Channel_NONE = 0,
    TIM_Channel_PWM = 1,
    TIM_Channel_IC = 2
} nECU_TIM_Channel_Type;
typedef struct
{
    GPIO_struct buttonPin; // GPIO structure
    uint32_t CCR_High, CCR_Low, CCR_prev;
    float frequency; // of callbacks in Hz
    bool newData;    // flag that new data have arrived
} nECU_InputCapture;
typedef struct
{
    TIM_HandleTypeDef *htim;           // periperal pointer
    float refClock;                    // in Hz (pre calculated on initialization)
    float period;                      // in ms (pre calculated on initialization)
    nECU_TIM_Channel_Type Channels[4]; // List of configured channels
    nECU_InputCapture IC[4];           // list of possible Input Captures
} nECU_Timer;

typedef struct
{
    uint32_t previousTick; // tick registered on previous callback
    uint32_t difference;   // difference between updates of structure
    uint8_t convFactor;    // do: difference*convFactor = to get time in ms
} nECU_TickTrack;
typedef struct
{
    uint32_t timeSet;         // time to wait in ticks
    nECU_TickTrack timeTrack; // time track according to ticks
    uint32_t timePassed;      // number of ticks in total that have passed
    bool done;                // delay ended
    bool active;              // delay counting or ended
} nECU_Delay;

/* ADCs */
typedef struct
{
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
} nECU_ADC3;
typedef struct
{
    uint16_t *ADC_data;  // pointer to ADC data
    int16_t temperature; // output data (real_tem*100)
    nECU_Delay update;   // delay after MCU restart
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
    BUTTON_ID_RED,
    BUTTON_ID_ORANGE,
    BUTTON_ID_GREEN,
    BUTTON_ID_MAX
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
    nECU_InputCapture *channel;
    Button_ClickType Type; // current detected type of click
    bool newType;          // flag to indicate new type detected
} ButtonInput;
typedef struct
{
    uint8_t speed;           // animation speed 0-100%
    uint16_t count;          // number of animation cycles to do
    int8_t state, prevState; // current and previous state of animation
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
    nECU_TickTrack TimeTracker; // tracks time using systick
} ButtonLight;
typedef struct
{
    ButtonLight light;
    ButtonInput input;
} Button;

/* EGT */
typedef enum
{
    EGT1_ID,
    EGT2_ID,
    EGT3_ID,
    EGT4_ID,
    EGT_ID_MAX
} EGT_Sensor_ID;
typedef enum
{
    EGT_ERROR_DATA = 1, // data recived is not valid
    EGT_ERROR_OC = 2,   // thermocouple not connected
    EGT_ERROR_SC = 3,   // short circuit to GND or VCC
    EGT_ERROR_NONE
} EGT_Error_Code;
typedef struct
{
    GPIO_struct CS_pin;                              // GPIO for Chip Select
    uint8_t in_buffer[4];                            // recived data buffer
    bool OC_Fault, SCG_Fault, SCV_Fault, Data_Error; // Thermocouple state / data validity
    EGT_Error_Code ErrCode;                          // code of error acording to definition
    uint8_t comm_fail;                               // communication error count to IC
    float InternalTemp, TcTemp;                      // temperature of ADC chip, thermocouple temperature
    uint16_t EGT_Temperature;                        // output temperature
    int16_t IC_Temperature;                          // device temperature
} MAX31855;
typedef struct
{
    MAX31855 TC[EGT_ID_MAX];     // sensor structures
    EGT_Sensor_ID currentSensor; // number of current sensor (for sensor asking loop)
    nECU_Delay startup;          // used as a delay to prevent spi communication until ICs turn on properly
} nECU_EGT;

/* Menu */
typedef enum
{
    TuneSelector_LowBoost,
    TuneSelector_HighBoost,
    TuneSelector_ID_MAX
} TuneSelector_ID;
typedef enum
{
    LaunchControl_OFF,
    LaunchControl_Low,
    LaunchControl_Medium,
    LaunchControl_High,
    LaunchControl_Rolling,
    LaunchControl_ID_MAX
} LaunchControl_ID;
typedef enum
{
    TACHO_ID_TuneSelector,
    TACHO_ID_LaunchControl,
    TACHO_ID_MenuLvl,
    TACHO_ID_MAX
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
    uint16_t MenuLevel;    // Level of button menu
    nECU_Delay save_delay; // Delay until data is saved to FLASH
} ButtonMenu;

/* Speed */
typedef enum
{
    SPEED_SENSOR_ID_FL,
    SPEED_SENSOR_ID_FR,
    SPEED_SENSOR_ID_RL,
    SPEED_SENSOR_ID_RR,
    SPEED_SENSOR_ID_MAX
} nECU_ADC2_ID; // Here update if connected otherwise
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
    nECU_ADC2_ID id;        // id of this speed sensor
} Speed_Sensor;
typedef struct
{
    bool active;                            // routine is running
    bool averageReady[SPEED_SENSOR_ID_MAX]; // flags indicating end of data collection on each sensor
} CalibrateRoutine;

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
    nECU_Delay frame_delay;    // timing between frames

    bool LunchControl[LaunchControl_ID_MAX]; // flags from decoding

    // outside variables
    bool *Cranking, *Fan_ON, *Lights_ON, *IgnitionKey;
    bool *TachoShow[TACHO_ID_MAX];
    bool *Antilag, *TractionOFF, *ClearEngineCode;
    uint16_t *LunchControlLevel;
    uint16_t *SpeedSensor[SPEED_SENSOR_ID_MAX];
} Frame0_struct;
typedef struct
{
    nECU_CAN_TxFrame can_data; // peripheral data
    nECU_Delay frame_delay;    // timing between frames

    // outside variables
    uint16_t *EGT[EGT_ID_MAX];
    uint8_t *TachoVal[TACHO_ID_MAX];
    uint16_t *TuneSelector;
} Frame1_struct;
typedef struct
{
    nECU_CAN_TxFrame can_data; // peripheral data
    nECU_Delay frame_delay;    // timing between frames

    // outside variables
    uint8_t *Backpressure, *OX_Val;
    uint16_t *MAP_Stock_10bit;
    uint8_t *Knock;
    uint8_t *VSS;
    uint32_t *loop_time;
} Frame2_struct;

/* Input Analog */
typedef enum
{
    ADC1_MAP_ID,
    ADC1_BackPressure_ID,
    ADC1_OX_ID,
    ADC1_AI_1_ID,
    ADC1_AI_2_ID,
    ADC1_AI_3_ID,
    ADC1_MCUTemp_ID,
    ADC1_VREF_ID,
    ADC1_ID_MAX
} nECU_ADC1_ID;
typedef struct
{
    uint16_t ADC_MeasuredMax, ADC_MeasuredMin; // limits of ADC readout
    float OUT_MeasuredMax, OUT_MeasuredMin;    // limits of resulting output
    float offset, factor;                      // offset that is added to result, factor by which output is multiplied
} AnalogSensorCalibration;
typedef struct
{
    uint16_t *Buffer; // pointer to buffer
    uint8_t len;      // lenght of the buffer
} AnalogSensorBuffer;

typedef struct
{
    nECU_Delay delay;           // update delay structure
    float smoothingAlpha;       // value for smoothing
    uint16_t previous_ADC_Data; // value from previous run
    AnalogSensorBuffer buf;     // smoothing buffer
} AnalogSensorFiltering;
typedef struct
{
    AnalogSensorCalibration calibration; // calibration structure
    AnalogSensorFiltering filter;        // filtering structure
    uint16_t *ADC_input;                 // pointer to ADC input data
    float output;                        // resulting value in float
} AnalogSensor_Handle;

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

    uint8_t Level;
    uint8_t RetardOut;
    float RetardPerc;

    Knock_Interpol_Table thresholdMap;

    Knock_FFT fft;

    // regression
    nECU_TickTrack regres;

    // UART communication
    bool UART_Transmission;
    nECU_UART uart;
    uint8_t UART_data_buffer[KNOCK_DMA_LEN];

    // Delay
    nECU_Delay delay; // Minimum time between each knock retard action (time to check if knock is gone after retard)
} nECU_Knock;

/* SPI */
typedef struct
{
    GPIO_struct *CS_pin; // GPIO for Chip Select
    bool data_Pending;   // data was recieved and is pending
} nECU_SPI;
typedef enum
{
    SPI_EGT_ID,
    SPI_ID_MAX
} nECU_SPI_ID;

/* Stock */
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
    uint8_t Speed;        // resulting speed
    bool overspeed_error; // flag to indicate speed reached maximal allowed

    uint16_t VSS_Smooth_buffer[VSS_SMOOTH_BUFFER_LENGTH];
    nECU_Delay VSS_ZeroDetect;
} VSS_Handle;
typedef struct
{
    nECU_InputCapture *channel;
    uint16_t RPM; // resulting RPM
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
    TIM_PWM_BUTTON_ID,
    TIM_PWM_OX_ID,
    TIM_IC_BUTTON_ID,
    TIM_IC_FREQ_ID,
    TIM_ADC_KNOCK_ID,
    TIM_FRAME_ID,
    TIM_ID_MAX
} nECU_TIM_ID;
typedef struct
{
    nECU_Timer *tim;          // pointer to watched timer
    bool error, warning;      // flags
    uint32_t counter_ms;      // watchdog counter
    uint64_t counter_max;     // value which determines error state
    nECU_TickTrack timeTrack; // used to track time between clears
} nECU_tim_Watchdog;

/* Debug */
typedef enum
{
    // internal temperature out of spec
    nECU_ERROR_DEVICE_TEMP_MCU_ID = 1,
    nECU_ERROR_DEVICE_TEMP_EGT1_ID, // !Have to be in the same order as 'EGT_Sensor_ID'!
    nECU_ERROR_DEVICE_TEMP_EGT2_ID,
    nECU_ERROR_DEVICE_TEMP_EGT3_ID,
    nECU_ERROR_DEVICE_TEMP_EGT4_ID,

    // thermocouple over temperature (over defined threshold)
    nECU_ERROR_EGT_OVERTEMP_EGT1_ID, // !Have to be in the same order as 'EGT_Sensor_ID'!
    nECU_ERROR_EGT_OVERTEMP_EGT2_ID,
    nECU_ERROR_EGT_OVERTEMP_EGT3_ID,
    nECU_ERROR_EGT_OVERTEMP_EGT4_ID,

    // thermocouple spi communication
    nECU_ERROR_EGT_SPI_EGT1_ID, // !Have to be in the same order as 'EGT_Sensor_ID'!
    nECU_ERROR_EGT_SPI_EGT2_ID,
    nECU_ERROR_EGT_SPI_EGT3_ID,
    nECU_ERROR_EGT_SPI_EGT4_ID,

    // thermocouple connection (TC sensor not connected, shorted etc)
    nECU_ERROR_EGT_TC_EGT1_ID, // !Have to be in the same order as 'EGT_Sensor_ID'!
    nECU_ERROR_EGT_TC_EGT2_ID,
    nECU_ERROR_EGT_TC_EGT3_ID,
    nECU_ERROR_EGT_TC_EGT4_ID,

    // flash interaction
    nECU_ERROR_FLASH_SPEED_SAVE_ID,
    nECU_ERROR_FLASH_SPEED_READ_ID,
    nECU_ERROR_FLASH_USER_SAVE_ID,
    nECU_ERROR_FLASH_USER_READ_ID,
    nECU_ERROR_FLASH_DEBUG_QUE_SAVE_ID,
    nECU_ERROR_FLASH_DEBUG_QUE_READ_ID,
    nECU_ERROR_FLASH_ERASE_ID,

    // communication
    nECU_ERROR_CAN_ID,
    nECU_ERROR_SPI_ID,

    // VSS
    nECU_ERROR_VSS_MAX,

    // ProgramBlock
    nECU_ERROR_PROGRAMBLOCK,

    nECU_ERROR_NONE
} nECU_Error_ID;
typedef enum
{
    nECU_FLASH_ERROR_SPEED = 2,
    nECU_FLASH_ERROR_USER = 4,
    nECU_FLASH_ERROR_DBGQUE = 6,
    nECU_FLASH_ERROR_ERASE = 7,
    nECU_FLASH_ERROR_NONE
} nECU_Flash_Error_ID;
typedef struct
{
    bool error_flag;     // flag indicating error ocurring
    float value_at_flag; // value that coused flag
    nECU_Error_ID ID;    // ID of error
} nECU_Debug_error_mesage;
typedef struct
{
    int16_t *MCU;       // internal temperature of MCU
    int16_t *EGT_IC[4]; // internal temperature of EGT ICs
} nECU_Debug_IC_temp;   // error due to over/under temperature of ICs
typedef struct
{
    EGT_Error_Code *EGT_IC[4]; // error code from recived frame
} nECU_Debug_EGT_Comm;         // error got from communication with EGT IC
typedef struct
{
    uint16_t *EGT_IC[4]; // temperature of thermocouples
} nECU_Debug_EGT_Temp;   // error due to over temperature of thermocouple
typedef struct
{
    nECU_Debug_error_mesage messages[DEBUG_QUE_LEN]; // que
    Counter counter;                                 // counter to track position of newest message
    uint16_t message_count;                          // count of messages in the que
} nECU_Debug_error_que;
typedef struct
{
    nECU_Debug_EGT_Comm egt_communication; // error got from communication with EGT IC
    nECU_Debug_EGT_Temp egt_temperature;   // error due to over temperature of thermocouple
    nECU_Debug_IC_temp device_temperature; // error due to over/under temperature of ICs
    nECU_Debug_error_que error_que;        // que of active error messages
} nECU_Debug;
typedef enum
{
    LED_ANIMATE_TEST_ID = 3,
    LED_ANIMATE_ERROR_ID = 2,
    LED_ANIMATE_UART_ID = 1,
    LED_ANIMATE_NONE_ID
} OnBoardLED_Animate_ID;
typedef struct
{
    GPIO_PinState state;            // state to be displayed
    nECU_Delay blink_delay;         // delay structure for non-blocking blinking
    bool blink_active;              // ON/OFF for blinking animation
    uint8_t blink_count;            // number of blinks to do
    OnBoardLED_Animate_ID priority; // ID determines which animation will be used for the output
} OnBoardLED_Animate;
typedef struct
{
    GPIO_struct LEDPin;                                     // pin of the LED structure
    OnBoardLED_Animate *Animation;                          // variables used to determine what should be shown
    OnBoardLED_Animate *Que[ONBOARD_LED_ANIMATION_QUE_LEN]; // que for animations (if multiple want to access)
    uint8_t Que_len;                                        // number of animations in que
} OnBoardLED;
typedef struct
{
    nECU_TickTrack tracker;
    uint32_t time;    // time of whole loop [ms]
    uint32_t counter; // loop counter
} nECU_LoopCounter;
typedef enum
{
    D_BLOCK_NULL = 0,        // Block was not initialized
    D_BLOCK_STOP = 1,        // Block stopped
    D_BLOCK_INITIALIZED = 2, // Block structures initialized
    D_BLOCK_WORKING = 4,     // Block working
    D_BLOCK_SPARE_1 = 8,
    D_BLOCK_SPARE_2 = 16,
    D_BLOCK_SPARE_3 = 32,
    D_BLOCK_ERROR_OLD = 64, // error in memory
    D_BLOCK_ERROR = 128,    // error active
    D_BLOCK_NONE
} nECU_ProgramBlock_Status;
typedef enum
{
    // nECU_adc.c
    D_ADC1,
    D_ADC2,
    D_ADC3,
    // nECU_button.c  !Have to be in the same order as 'Button_ID'!
    D_Button_Red,
    D_Button_Orange,
    D_Button_Green,
    // nECU_can.c
    D_CAN_TX,
    D_CAN_RX,
    // nECU_data_processing.c
    D_Data_Processing,
    // nECU_debug.c
    D_Debug,
    D_Debug_Que,
    // nECU_EGT.c !Have to be in the same order as 'EGT_Sensor_ID'!
    D_EGT1,
    D_EGT2,
    D_EGT3,
    D_EGT4,
    // nECU_flash.c
    D_Flash,
    // nECU_frames.c
    D_F0,
    D_F1,
    D_F2,
    // nECU_Input_Analog.c !Have to be in the same order as 'nECU_ADC1_ID'!
    D_ANALOG_MAP,
    D_ANALOG_BackPressure,
    D_ANALOG_OX,
    D_ANALOG_AI_1,
    D_ANALOG_AI_2,
    D_ANALOG_AI_3,
    D_ANALOG_MCUTemp,
    D_ANALOG_VREF,
    // nECU_Input_Frequency.c
    D_VSS,
    D_IGF,
    D_Input_Frequency,
    // nECU_Knock.c
    D_Knock,
    // nECU_main.c
    D_Main,
    // nECU_menu.c
    D_Tacho,
    D_Menu,
    // nECU_OnBoardLED.c
    D_OnboardLED,
    // nECU_PC.c
    D_PC,
    // nECU_Speed.c
    D_SS1,
    D_SS2,
    D_SS3,
    D_SS4,
    // nECU_stock.c
    D_GPIO,
    D_OX,
    // nECU_tim.c
    D_TIM_PWM_BUTTON,
    D_TIM_PWM_OX,
    D_TIM_IC_BUTTON,
    D_TIM_IC_FREQ,
    D_TIM_ADC_KNOCK,
    D_TIM_FRAME,
    // Last value
    D_ID_MAX
} nECU_Module_ID;
typedef struct
{
    nECU_ProgramBlock_Status Status; // Status code
    nECU_TickTrack Update_ticks;     // Ticks it taken since last update call
    uint8_t timeout_value;           // time in seconds after which error should apear
} nECU_ProgramBlockData;

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
    uint8_t boolByte2;
    uint8_t boolByte3;
    uint8_t boolByte4;
} nECU_UserSettings;
typedef struct
{
    nECU_SpeedCalibrationData speedData;
    nECU_UserSettings userData;
    nECU_Debug_error_que *DebugQueData;
} nECU_FlashContent;

/* PC */
typedef struct
{
    nECU_UART output;
    nECU_UART input;
    uint8_t out_buf[PC_UART_BUF_LEN];
    uint8_t in_buf[PC_UART_BUF_LEN];
    OnBoardLED_Animate Tx_LED;
    OnBoardLED_Animate Rx_LED;
} nECU_PC;

#endif // _nECU_types_H_