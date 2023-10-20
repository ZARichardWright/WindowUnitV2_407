/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Y0_A_Pin GPIO_PIN_2
#define Y0_A_GPIO_Port GPIOE
#define Y1_A_Pin GPIO_PIN_3
#define Y1_A_GPIO_Port GPIOE
#define Y2_A_Pin GPIO_PIN_4
#define Y2_A_GPIO_Port GPIOE
#define M1A_Pin GPIO_PIN_5
#define M1A_GPIO_Port GPIOE
#define M1B_Pin GPIO_PIN_13
#define M1B_GPIO_Port GPIOC
#define M1C_Pin GPIO_PIN_14
#define M1C_GPIO_Port GPIOC
#define M1D_Pin GPIO_PIN_15
#define M1D_GPIO_Port GPIOC
#define DAC0_Pin GPIO_PIN_5
#define DAC0_GPIO_Port GPIOA
#define LAN_RESET_Pin GPIO_PIN_2
#define LAN_RESET_GPIO_Port GPIOB
#define OUT_48_Pin GPIO_PIN_7
#define OUT_48_GPIO_Port GPIOE
#define M3D_Pin GPIO_PIN_8
#define M3D_GPIO_Port GPIOE
#define M3C_Pin GPIO_PIN_9
#define M3C_GPIO_Port GPIOE
#define M3B_Pin GPIO_PIN_10
#define M3B_GPIO_Port GPIOE
#define M3A_Pin GPIO_PIN_11
#define M3A_GPIO_Port GPIOE
#define M2D_Pin GPIO_PIN_12
#define M2D_GPIO_Port GPIOE
#define M2C_Pin GPIO_PIN_13
#define M2C_GPIO_Port GPIOE
#define M2B_Pin GPIO_PIN_14
#define M2B_GPIO_Port GPIOE
#define M2A_Pin GPIO_PIN_15
#define M2A_GPIO_Port GPIOE
#define EN_485_1_Pin GPIO_PIN_15
#define EN_485_1_GPIO_Port GPIOB
#define TX_485_1_Pin GPIO_PIN_8
#define TX_485_1_GPIO_Port GPIOD
#define RX_485_1_Pin GPIO_PIN_9
#define RX_485_1_GPIO_Port GPIOD
#define FLASH_DO_Pin GPIO_PIN_10
#define FLASH_DO_GPIO_Port GPIOD
#define FLASH_DI_Pin GPIO_PIN_11
#define FLASH_DI_GPIO_Port GPIOD
#define FLASH_CLK_Pin GPIO_PIN_12
#define FLASH_CLK_GPIO_Port GPIOD
#define FLASH_CS_Pin GPIO_PIN_13
#define FLASH_CS_GPIO_Port GPIOD
#define BELL_Pin GPIO_PIN_6
#define BELL_GPIO_Port GPIOC
#define PWM2_Pin GPIO_PIN_8
#define PWM2_GPIO_Port GPIOA
#define TX_485_2_Pin GPIO_PIN_9
#define TX_485_2_GPIO_Port GPIOA
#define RX_485_2_Pin GPIO_PIN_10
#define RX_485_2_GPIO_Port GPIOA
#define EN_485_2_Pin GPIO_PIN_11
#define EN_485_2_GPIO_Port GPIOA
#define PWM_48_Pin GPIO_PIN_12
#define PWM_48_GPIO_Port GPIOA
#define PWM_48_EXTI_IRQn EXTI15_10_IRQn
#define GPS_TX_Pin GPIO_PIN_10
#define GPS_TX_GPIO_Port GPIOC
#define GPS_RX_Pin GPIO_PIN_11
#define GPS_RX_GPIO_Port GPIOC
#define GPS_PPS_Pin GPIO_PIN_12
#define GPS_PPS_GPIO_Port GPIOC
#define USER_SW_Pin GPIO_PIN_2
#define USER_SW_GPIO_Port GPIOD
#define X2_Pin GPIO_PIN_3
#define X2_GPIO_Port GPIOB
#define X2_EXTI_IRQn EXTI3_IRQn
#define X3_Pin GPIO_PIN_4
#define X3_GPIO_Port GPIOB
#define X3_EXTI_IRQn EXTI4_IRQn
#define X1_Pin GPIO_PIN_5
#define X1_GPIO_Port GPIOB
#define X1_EXTI_IRQn EXTI9_5_IRQn
#define PWM1_Pin GPIO_PIN_6
#define PWM1_GPIO_Port GPIOB
#define X0_Pin GPIO_PIN_7
#define X0_GPIO_Port GPIOB
#define X0_EXTI_IRQn EXTI9_5_IRQn
#define I2C_SCL_Pin GPIO_PIN_8
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_9
#define I2C_SDA_GPIO_Port GPIOB
#define LED_RUN_Pin GPIO_PIN_0
#define LED_RUN_GPIO_Port GPIOE
#define LED_ERR_Pin GPIO_PIN_1
#define LED_ERR_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
