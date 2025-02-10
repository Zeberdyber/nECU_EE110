/**
 ******************************************************************************
 * @file    nECU_OnBoardLED.c
 * @brief   This file provides code for animations and output control of on
 *          board LEDs.
 ******************************************************************************
 */

#include "nECU_OnBoardLED.h"

static OnBoardLED LED_L = {0}, LED_R = {0};
static OnBoardLED_Animate LED_blank_animation = {0};

/* GPIO */
static bool OnBoard_LED_GPIO_Init(OnBoardLED *inst, uint16_t GPIO_Pin, GPIO_TypeDef *GPIOx) // initializes single LED GPIO structure
{
    inst->LEDPin.GPIO_Pin = GPIO_Pin;
    inst->LEDPin.GPIOx = GPIOx;
    inst->LEDPin.State = GPIO_PIN_RESET;
    return false;
}
static void OnBoard_LED_GPIO_Update(GPIO_struct *inst, GPIO_PinState State) // update of GPIO output
{
    inst->State = State; // copy animation to the output
    HAL_GPIO_WritePin(inst->GPIOx, inst->GPIO_Pin, inst->State);
}

/* Animation */
bool OnBoard_LED_Animation_Init(OnBoardLED_Animate *inst, OnBoardLED_Animate_ID priority) // initializes animation structure
{
    nECU_Delay_Set(&(inst->blink_delay), 0);
    inst->state = GPIO_PIN_RESET;
    inst->blink_active = false;
    inst->priority = priority;

    return false;
}
static void OnBoard_LED_Animation_BlinkSetDelay(OnBoardLED_Animate *inst, uint32_t delay) // sets delay for blinking
{
    nECU_Delay_Set(&(inst->blink_delay), delay);
}
void OnBoard_LED_Animation_BlinkStart(OnBoardLED_Animate *inst, uint32_t delay, uint8_t count) // starts blink animation
{
    if (inst->blink_active == true) // break if animation is ongoing
    {
        return;
    }

    OnBoard_LED_Animation_BlinkSetDelay(inst, delay);
    inst->blink_active = true;
    nECU_Delay_Start(&(inst->blink_delay));
    OnBoard_LED_State_Set(inst, GPIO_PIN_SET);
    if (count < (UINT8_MAX) / 2)
    {
        inst->blink_count = count * 2; // times 2 to account for two state changes per blink
    }
    else
    {
        inst->blink_count = ONBOARDLED_BLINK_CONTINOUS;
    }
}
void OnBoard_LED_Animation_BlinkStop(OnBoardLED_Animate *inst) // stops blink animation
{
    nECU_Delay_Stop(&(inst->blink_delay));
    inst->blink_active = false;
    OnBoard_LED_State_Set(inst, GPIO_PIN_SET);
    inst->blink_count = ONBOARDLED_BLINK_CONTINOUS;
}
static void OnBoard_LED_Animation_BlinkUpdate(OnBoardLED_Animate *inst) // updates the blinking animation
{
    if (inst->blink_active == false)
    {
        return; // breaks if blinking is not needed
    }

    nECU_Delay_Update(&(inst->blink_delay));                  // update ticktrack for delay
    if (*(nECU_Delay_DoneFlag(&(inst->blink_delay))) == true) // check if delay has finished
    {
        nECU_Delay_Start(&(inst->blink_delay)); // restart delay
        OnBoard_LED_State_Flip(inst);
        if (inst->blink_count != ONBOARDLED_BLINK_CONTINOUS)
        {
            inst->blink_count--;
            if (inst->blink_count == 0)
            {
                OnBoard_LED_Animation_BlinkStop(inst);
            }
        }
    }
}
static void OnBoard_LED_Animation_Update(OnBoardLED_Animate *inst) // function to perform logic behind blinking times and update to GPIO
{
    OnBoard_LED_Animation_BlinkUpdate(inst); // update blinking
}
static void OnBoard_LED_State_Set(OnBoardLED_Animate *inst, GPIO_PinState State) // sets the current state
{
    inst->state = State;
}
static void OnBoard_LED_State_Flip(OnBoardLED_Animate *inst) // flips the current state
{
    inst->state = !(inst->state);
}

/* Animation que */
static bool OnBoard_LED_Que_Init(OnBoardLED *inst) // initialize que data struct
{
    inst->Que_len = 0;
    for (uint8_t i = 0; i < ONBOARD_LED_ANIMATION_QUE_LEN; i++)
    {
        inst->Que[i] = &LED_blank_animation; // fill the que with blanks
    }
    inst->Animation = inst->Que[0]; // set current animation as the first in the que

    return false;
}
static void OnBoard_LED_Que_Add(OnBoardLED *inst, OnBoardLED_Animate *animation) // adds to the que
{
    if (!nECU_FlowControl_Working_Check(D_OnboardLED))
    {
        nECU_FlowControl_Error_Do(D_OnboardLED);
        return;
    }

    if (animation->priority == LED_ANIMATE_NONE_ID) // if priority not set
    {
        return;
    }
    inst->Que_len++;
    if (inst->Que_len > ONBOARD_LED_ANIMATION_QUE_LEN) // break if out of bound
    {
        return;
    }
    // check if animation was already added
    for (uint8_t i = 0; i < (inst->Que_len) - 1; i++)
    {
        if (inst->Que[i] == animation)
        {
            return;
        }
    }
    // add to the que
    inst->Que[(inst->Que_len - 1)] = animation;
    // if have higher priority set as current
    if (inst->Que[(inst->Que_len) - 1]->priority > inst->Animation->priority)
    {
        inst->Animation = inst->Que[(inst->Que_len) - 1];
    }
    return;
}
static void OnBoard_LED_Que_Remove(OnBoardLED *inst, OnBoardLED_Animate *animation) // removes from the que
{
    if (!nECU_FlowControl_Working_Check(D_OnboardLED))
    {
        nECU_FlowControl_Error_Do(D_OnboardLED);
        return;
    }

    if (animation->priority == LED_ANIMATE_NONE_ID) // if priority not set
    {
        return;
    }
    if (inst->Que_len == 0) // if nothing in que
    {
        return;
    }

    // find if in que
    uint8_t que_position = UINT8_MAX;
    for (uint8_t i = 0; i < (inst->Que_len) - 1; i++)
    {
        if (inst->Que[i] == animation)
        {
            que_position = i;
            break; // exit for loop
        }
    }
    if (que_position == UINT8_MAX) // animation not found
    {
        return;
    }

    // move que to remove animation
    for (uint8_t i = que_position; i < (inst->Que_len) - 1; i++)
    {
        inst->Que[i] = inst->Que[i + 1];
    }
    inst->Que_len--;

    // set first as current
    inst->Animation = inst->Que[0];

    // find one with highest priority and set as current
    que_position = UINT8_MAX;
    for (uint8_t i = 0; i < (inst->Que_len) - 1; i++)
    {
        if (inst->Que[i]->priority > inst->Animation->priority)
        {
            inst->Animation = inst->Que[i];
        }
    }
}
static void OnBoard_LED_Que_Check(OnBoardLED *inst) // check if current animation is done, move que
{
    if ((inst->Animation->blink_active == false || inst->Animation->priority == LED_ANIMATE_ERROR_ID) && inst->Que_len > 0) // check if animation is done and que has new animations waiting
    {
        inst->Animation = inst->Que[0]; // add new animation as a current one
        inst->Que_len--;
        for (uint8_t que_index = 1; que_index < (inst->Que_len); que_index++) // move que
        {
            inst->Que[que_index - 1] = inst->Que[que_index];
        }
    }
}

/* General */
bool OnBoard_LED_Start(void) // initialize structures for on board LEDs
{
    bool status = false;
    if (!nECU_FlowControl_Initialize_Check(D_OnboardLED))
    {
        /* Left LED */
        status |= OnBoard_LED_GPIO_Init(&(LED_L), LED1_Pin, LED1_GPIO_Port);
        LED_L.Animation = &LED_blank_animation;
        status |= OnBoard_LED_Animation_Init((LED_L.Animation), LED_ANIMATE_NONE_ID);
        status |= OnBoard_LED_Que_Init(&(LED_L));

        /* Right LED */
        status |= OnBoard_LED_GPIO_Init(&(LED_R), LED2_Pin, LED2_GPIO_Port);
        LED_R.Animation = &LED_blank_animation;
        status |= OnBoard_LED_Animation_Init((LED_R.Animation), LED_ANIMATE_NONE_ID);
        status |= OnBoard_LED_Que_Init(&(LED_R));

        if (!status)
            status |= nECU_FlowControl_Initialize_Do(D_OnboardLED);
    }
    if (!nECU_FlowControl_Working_Check(D_OnboardLED) && status == false)
    {
        if (!status)
        {
            status |= !nECU_FlowControl_Working_Do(D_OnboardLED);
        }
    }
    if (status)
    {
        nECU_FlowControl_Error_Do(D_OnboardLED);
    }
    return status;
}
void OnBoard_LED_Update(void) // update on board LEDs states
{
    if (!nECU_FlowControl_Working_Check(D_OnboardLED))
    {
        nECU_FlowControl_Error_Do(D_OnboardLED);
        return;
    }

    OnBoard_LED_Update_Single(&LED_L);
    OnBoard_LED_Update_Single(&LED_R);

    nECU_Debug_ProgramBlockData_Update(D_OnboardLED);
}
void OnBoard_LED_Update_Single(OnBoardLED *inst) // update of all internal variables
{
    OnBoard_LED_Que_Check(inst);
    OnBoard_LED_Animation_Update((inst->Animation)); // update only current animation
    OnBoard_LED_GPIO_Update(&(inst->LEDPin), inst->Animation->state);
}

/* LED interface functions */
void OnBoard_LED_L_Add_Animation(OnBoardLED_Animate *animation)
{
    OnBoard_LED_Que_Add(&LED_L, animation);
}
void OnBoard_LED_L_Remove_Animation(OnBoardLED_Animate *animation)
{
    OnBoard_LED_Que_Remove(&LED_L, animation);
}
void OnBoard_LED_R_Add_Animation(OnBoardLED_Animate *animation)
{
    OnBoard_LED_Que_Add(&LED_R, animation);
}
void OnBoard_LED_R_Remove_Animation(OnBoardLED_Animate *animation)
{
    OnBoard_LED_Que_Remove(&LED_R, animation);
}
