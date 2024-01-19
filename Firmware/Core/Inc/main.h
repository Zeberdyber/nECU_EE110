/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Lights_ON_Pin GPIO_PIN_4
#define Lights_ON_GPIO_Port GPIOE
#define Fan_ON_Pin GPIO_PIN_5
#define Fan_ON_GPIO_Port GPIOE
#define Cranking_Pin GPIO_PIN_6
#define Cranking_GPIO_Port GPIOE
#define V1_Pin GPIO_PIN_0
#define V1_GPIO_Port GPIOC
#define V2_Pin GPIO_PIN_1
#define V2_GPIO_Port GPIOC
#define V3_Pin GPIO_PIN_2
#define V3_GPIO_Port GPIOC
#define V4_Pin GPIO_PIN_3
#define V4_GPIO_Port GPIOC
#define KNOCK_Pin GPIO_PIN_1
#define KNOCK_GPIO_Port GPIOA
#define MAP_Pin GPIO_PIN_2
#define MAP_GPIO_Port GPIOA
#define Backpressure_Pin GPIO_PIN_3
#define Backpressure_GPIO_Port GPIOA
#define OX_Pin GPIO_PIN_4
#define OX_GPIO_Port GPIOA
#define AI1_Pin GPIO_PIN_5
#define AI1_GPIO_Port GPIOA
#define AI2_Pin GPIO_PIN_6
#define AI2_GPIO_Port GPIOA
#define AI3_Pin GPIO_PIN_7
#define AI3_GPIO_Port GPIOA
#define B1_H_Pin GPIO_PIN_9
#define B1_H_GPIO_Port GPIOE
#define B2_H_Pin GPIO_PIN_11
#define B2_H_GPIO_Port GPIOE
#define B3_H_Pin GPIO_PIN_13
#define B3_H_GPIO_Port GPIOE
#define IGF_Pin GPIO_PIN_12
#define IGF_GPIO_Port GPIOD
#define VSS_Pin GPIO_PIN_13
#define VSS_GPIO_Port GPIOD
#define B1_S_Pin GPIO_PIN_6
#define B1_S_GPIO_Port GPIOC
#define B2_S_Pin GPIO_PIN_7
#define B2_S_GPIO_Port GPIOC
#define B3_S_Pin GPIO_PIN_8
#define B3_S_GPIO_Port GPIOC
#define IMO_Pin GPIO_PIN_9
#define IMO_GPIO_Port GPIOA
#define IMI_Pin GPIO_PIN_10
#define IMI_GPIO_Port GPIOA
#define OX_PWM_Pin GPIO_PIN_15
#define OX_PWM_GPIO_Port GPIOA
#define T4_CS_Pin GPIO_PIN_3
#define T4_CS_GPIO_Port GPIOD
#define T3_CS_Pin GPIO_PIN_4
#define T3_CS_GPIO_Port GPIOD
#define T2_CS_Pin GPIO_PIN_5
#define T2_CS_GPIO_Port GPIOD
#define T1_CS_Pin GPIO_PIN_6
#define T1_CS_GPIO_Port GPIOD
#define LED1_Pin GPIO_PIN_8
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOB
#define S1_Pin GPIO_PIN_0
#define S1_GPIO_Port GPIOE
#define S2_Pin GPIO_PIN_1
#define S2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
