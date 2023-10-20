#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"
#include "main.h"

extern APP_SYSTEM_DATA *SystemState;
extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim1;

#define CHECK(A) (SystemState->Current.Fans[A].Speed!=SystemState->Required.Fans[A].Speed)
#define REQUIREDSTATE(A) (SystemState->Required.Fans[A].Speed)
#define CURRENTSTATE(A) (SystemState->Current.Fans[A].Speed)

#define DACGAIN (4)
#define VPERBIT ((VREF*4)/4096)
#define VPERBIFF (0.002441406)

uint16_t FanCnt[4];
uint16_t LastPwm;

void setPWM(TIM_HandleTypeDef timer, uint32_t channel, uint16_t period, uint16_t pulse) {
	HAL_TIM_PWM_Stop(&timer, channel); // stop generation of pwm
	TIM_OC_InitTypeDef sConfigOC;
	timer.Init.Period = period; // set the period duration
	if (SystemState->UnitType == UNIT_ECOSAFE)
		timer.Init.Prescaler = 84 - 1;
	else
		timer.Init.Prescaler = 12 - 1;
	HAL_TIM_PWM_Init(&timer); // reinititialise with new period value
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = pulse; // set the pulse duration
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, channel);
	HAL_TIM_PWM_Start(&timer, channel); // start pwm generation
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == X0_Pin) {
		SystemState->LastRunTime.xLastWakeTime_EXTI[0] = xTaskGetTickCount();
		FanCnt[0]++;
	}

	if (GPIO_Pin == X1_Pin) {
		SystemState->LastRunTime.xLastWakeTime_EXTI[1] = xTaskGetTickCount();
		FanCnt[1]++;
	}

	if (GPIO_Pin == X2_Pin) {
		SystemState->LastRunTime.xLastWakeTime_EXTI[2] = xTaskGetTickCount();
		FanCnt[2]++;
	}

	if (GPIO_Pin == X3_Pin) {
		SystemState->LastRunTime.xLastWakeTime_EXTI[3] = xTaskGetTickCount();
		FanCnt[3]++;
	}

	if (GPIO_Pin == PWM_48_Pin) {
		//HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
		if (HAL_GPIO_ReadPin(PWM_48_GPIO_Port, PWM_48_Pin) == 1) {
			LastPwm = 1;
			__HAL_TIM_SetCounter(&htim1,0);
		} else {
			if (LastPwm == 1) {
				SystemState->ExternalCurrent2 = (((__HAL_TIM_GET_COUNTER(&htim1)) / (float) 2000.0) * 10000.0);
				SystemState->ExternalCurrent2 = CLAMP(SystemState->ExternalCurrent2, 10000, 0);
				HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
				LastPwm = 0;
			}
		}

	}

}

#define FANTASKFREQ 5000

void StartFanTask(void *argument) {
	/* Infinite loop */
	//uint16_t TDacValue =0x00;
	HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
	HAL_DAC_Start(&hdac, DAC_CHANNEL_2);
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
	HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
	uint16_t PrevSpeed[4] = { 0, 0, 0, 0 };
	uint16_t PwmValue = 10000;
	float ScaledP = 0;
	HAL_TIM_Base_Start(&htim1);
	HAL_TIM_Base_Start(&htim4);

	for (;;) {

		if (!CHECKBIT(SystemState->TASKRUNBITS, RUNBIT_FAN_TASK)) {
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Fan, 10000);
			continue;
		}

		if (CHECK(FAN_PORT_INDOOR) || (SystemState->LastRunTime.xLastWakeTime_Fan < 1000)) {
			//Convert 16_t Percent to Voltage then to Bits
			//7623 = 76.23 % = 7.623V = 3122
			//SystemState->LastRunTime.xLastWakeTime_EXTI[2] = xTaskGetTickCount();

			//DAC
			uint16_t DacValue = 0;
			if (REQUIREDSTATE(FAN_PORT_INDOOR) != 0) {
				ScaledP = ((REQUIREDSTATE(FAN_PORT_INDOOR) * 0.85) + 1500);
				DacValue = ((ScaledP / 100.0 / 10.0) / VPERBIFF) - 1;

			}

			//HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, DacValue);

			if (REQUIREDSTATE(FAN_PORT_INDOOR) != 0) {
				ScaledP = ((REQUIREDSTATE(FAN_PORT_INDOOR) / 100.0 * 0.0088) + 0.070);
				//DacValue = ((ScaledP / 100.0 / 10.0) / VPERBIFF) - 1;
				PwmValue = (ScaledP * 1000);
			} else
				PwmValue = 0;

			setPWM(htim4, TIM_CHANNEL_1, 1000, PwmValue);

			CURRENTSTATE(FAN_PORT_INDOOR) = REQUIREDSTATE(FAN_PORT_INDOOR);
			//PWM
			CLEARBIT(SystemState->Required.Realys, 0);

		}

		if (SystemState->UnitType == UNIT_SLIM) {
			if (REQUIREDSTATE(FAN_PORT_OUTDOOR) > 9500) {
				SETBIT(SystemState->Required.Realys, 2);
				CURRENTSTATE(FAN_PORT_OUTDOOR) = 10000;
			}
			if (REQUIREDSTATE(FAN_PORT_OUTDOOR) < 5200) {
				CLEARBIT(SystemState->Required.Realys, 2);
				CURRENTSTATE(FAN_PORT_OUTDOOR) = 0;
			}
		} else {
			if (CHECK(FAN_PORT_OUTDOOR) || (SystemState->LastRunTime.xLastWakeTime_Fan < 1000)) {

				CLEARBIT(SystemState->Required.Realys, 0);
				//float ScaledP = ((REQUIREDSTATE(FAN_PORT_OUTDOOR) * 1) + 0);
				//uint16_t PwmValue = (1) * ScaledP;

				/*
				 if (REQUIREDSTATE(FAN_PORT_OUTDOOR) != 0) {
				 ScaledP = ((REQUIREDSTATE(FAN_PORT_OUTDOOR) * 0.85) + 1500);
				 //DacValue = ((ScaledP / 100.0 / 10.0) / VPERBIFF) - 1;
				 PwmValue = (0.5) * ScaledP;
				 } else
				 PwmValue = 0;
				 */
				//PWM to analog
				//setPWM(htim1, TIM_CHANNEL_1, 5000, PwmValue);
				if (REQUIREDSTATE(FAN_PORT_OUTDOOR) != 0) {
					ScaledP = ((REQUIREDSTATE(FAN_PORT_OUTDOOR) / 100.0 * 0.0088) + 0.070);
					//DacValue = ((ScaledP / 100.0 / 10.0) / VPERBIFF) - 1;
					//PwmValue = (1000) - (ScaledP*1000);
					PwmValue = (ScaledP * 1000);
				} else
					PwmValue = 0;

				setPWM(htim1, TIM_CHANNEL_1, 1000, PwmValue);

				CURRENTSTATE(FAN_PORT_OUTDOOR) = REQUIREDSTATE(FAN_PORT_OUTDOOR);

			}
		}

		/*
		 if (CHECK(FAN_PORT_SPARE)) {
		 //Need to set PWM here
		 //Convert 16_t percent to PercentF then to Counts
		 float ScaledP = ((REQUIREDSTATE(FAN_PORT_SPARE) * 0.85) + 1500);
		 uint16_t PwmValue = (10000) * ScaledP;
		 __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, PwmValue);
		 CURRENTSTATE(FAN_PORT_SPARE) = REQUIREDSTATE(FAN_PORT_SPARE);
		 }
		 */

		if ((SystemState->Required.Fans[FAN_PORT_INDOOR].Speed > 0) && (PrevSpeed[FAN_PORT_INDOOR] != 0)) {
			HAL_NVIC_DisableIRQ(EXTI4_IRQn);
			if (xTaskGetTickCount() - SystemState->LastRunTime.xLastWakeTime_EXTI[2] > SECONDS(15)) {
				ADDFLAG(SystemState->E_Errors, E_ERR_INDOOR_FAN);
			} else {
				REMOVEFLAG(SystemState->E_Errors, E_ERR_INDOOR_FAN);
				SystemState->Measured.Fans[FAN_PORT_INDOOR].Speed = (FanCnt[2] * (60000 / FANTASKFREQ));
				FanCnt[2] = 0;
			}
			HAL_NVIC_EnableIRQ(EXTI4_IRQn);
		}

		if (SystemState->UnitType != UNIT_SLIM) {
			if ((SystemState->Required.Fans[FAN_PORT_OUTDOOR].Speed > 0) && (PrevSpeed[FAN_PORT_OUTDOOR] != 0)) {
				HAL_NVIC_DisableIRQ(EXTI3_IRQn);
				if (xTaskGetTickCount() - SystemState->LastRunTime.xLastWakeTime_EXTI[3] > SECONDS(15))
					ADDFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN);
				else {
					REMOVEFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN);
					SystemState->Measured.Fans[FAN_PORT_OUTDOOR].Speed = (FanCnt[3] * (60000 / FANTASKFREQ));
					FanCnt[3] = 0;
				}
				HAL_NVIC_EnableIRQ(EXTI3_IRQn);
			}
		} else
			REMOVEFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN);
		/*
		 if (REQUIREDSTATE(FAN_PORT_INDOOR) > 0) {
		 if (__HAL_GPIO_EXTI_GET_IT(X2_Pin) != RESET) {
		 __HAL_GPIO_EXTI_CLEAR_IT(X2_Pin);
		 ADDFLAG(SystemState->E_Errors, E_ERR_INDOOR_FAN);
		 }
		 } else
		 REMOVEFLAG(SystemState->E_Errors, E_ERR_INDOOR_FAN);

		 if (REQUIREDSTATE(FAN_PORT_OUTDOOR) > 0) {
		 if (__HAL_GPIO_EXTI_GET_IT(X3_Pin) != RESET) {
		 __HAL_GPIO_EXTI_CLEAR_IT(X3_Pin);
		 ADDFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN);
		 }
		 } else
		 REMOVEFLAG(SystemState->E_Errors, E_ERR_OUTDOOR_EC_FAN);
		 */

		if (PrevSpeed[FAN_PORT_OUTDOOR] == 0) {
			SystemState->LastRunTime.xLastWakeTime_EXTI[3] = xTaskGetTickCount();
		}

		if (PrevSpeed[FAN_PORT_INDOOR] == 0)
			SystemState->LastRunTime.xLastWakeTime_EXTI[2] = xTaskGetTickCount();

		PrevSpeed[FAN_PORT_OUTDOOR] = CURRENTSTATE(FAN_PORT_OUTDOOR);
		PrevSpeed[FAN_PORT_INDOOR] = CURRENTSTATE(FAN_PORT_INDOOR);

		SystemState->ExternalCurrent2 = 0;
		HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
		LastPwm = 0;
		vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Fan, FANTASKFREQ);
	}
}
