#include <main.h>
#include <Macros.h>
#include <stdbool.h>
#include <stm32f4xx_hal.h>
#include <sys/_stdint.h>
#include "cmsis_os.h"
#include "appSystem.h"

extern APP_SYSTEM_DATA *SystemState;

#define CHECK(A) (SystemState->Current.Steppers[A].Position!=SystemState->Required.Steppers[A].Position)
#define REQUIREDSTATE(A) (SystemState->Required.Steppers[A].Position)
#define CURRENTSTATE(A) (SystemState->Current.Steppers[A].Position)

typedef struct {
	GPIO_TypeDef *gpio;
	uint16_t pin;
} my_pin_t;

const my_pin_t stepper_pins[3][4] = { { { M1A_GPIO_Port, M1A_Pin }, { M1B_GPIO_Port, M1B_Pin }, { M1C_GPIO_Port, M1C_Pin }, { M1D_GPIO_Port, M1D_Pin } },
									  { { M2A_GPIO_Port, M2A_Pin }, { M2B_GPIO_Port, M2B_Pin }, { M2C_GPIO_Port, M2C_Pin }, { M2D_GPIO_Port, M2D_Pin } },
									  { { M3A_GPIO_Port, M3A_Pin }, { M3B_GPIO_Port, M3B_Pin }, { M3C_GPIO_Port, M3C_Pin }, { M3D_GPIO_Port, M3D_Pin } } };

struct {
	uint16_t numberofsequences;
	uint16_t numberofrevsequences;
	uint8_t currentstep;
	uint8_t direction;
	DAMPER_STATE GoalState;
} stepper_internal[3];

void stepper_half_drive(uint8_t Motor, uint8_t step) {
	switch (step) {
	case 0:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_SET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_RESET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_RESET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_RESET);   // IN4
		break;

	case 1:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_SET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_SET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_RESET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_RESET);   // IN4
		break;

	case 2:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_RESET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_SET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_RESET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_RESET);   // IN4
		break;

	case 3:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_RESET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_SET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_SET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_RESET);   // IN4
		break;

	case 4:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_RESET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_RESET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_SET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_RESET);   // IN4
		break;

	case 5:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_RESET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_RESET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_SET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_SET);   // IN4
		break;

	case 6:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_RESET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_RESET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_RESET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_SET);   // IN4
		break;

	case 7:
		HAL_GPIO_WritePin(stepper_pins[Motor][0].gpio, stepper_pins[Motor][0].pin, GPIO_PIN_SET);   // IN1
		HAL_GPIO_WritePin(stepper_pins[Motor][1].gpio, stepper_pins[Motor][1].pin, GPIO_PIN_RESET);   // IN2
		HAL_GPIO_WritePin(stepper_pins[Motor][2].gpio, stepper_pins[Motor][2].pin, GPIO_PIN_RESET);   // IN3
		HAL_GPIO_WritePin(stepper_pins[Motor][3].gpio, stepper_pins[Motor][3].pin, GPIO_PIN_SET);   // IN4
		break;

	}
}

void ProcessStepper(uint8_t Motor) {
	if (stepper_internal[Motor].numberofrevsequences > 0) {

		//Going to first do as many reverse as needed
		if (stepper_internal[Motor].direction == 1) //Swap Direction
			stepper_internal[Motor].currentstep = stepper_internal[Motor].currentstep == 0 ? 7 : stepper_internal[Motor].currentstep - 1;
		else
			stepper_internal[Motor].currentstep = stepper_internal[Motor].currentstep == 7 ? 0 : stepper_internal[Motor].currentstep + 1;

		stepper_half_drive(Motor, stepper_internal[Motor].currentstep);

		if(stepper_internal[Motor].currentstep==0)
			stepper_internal[Motor].numberofrevsequences--;

		CURRENTSTATE(Motor) = DAMPER_BUSY;
	} else if (stepper_internal[Motor].numberofsequences > 0) {

		if (stepper_internal[Motor].direction == 0)
			stepper_internal[Motor].currentstep = stepper_internal[Motor].currentstep == 0 ? 7 : stepper_internal[Motor].currentstep - 1;
		else
			stepper_internal[Motor].currentstep = stepper_internal[Motor].currentstep == 7 ? 0 : stepper_internal[Motor].currentstep + 1;

		stepper_half_drive(Motor, stepper_internal[Motor].currentstep);

		if(stepper_internal[Motor].currentstep==0)
			stepper_internal[Motor].numberofsequences--;
		CURRENTSTATE(Motor) = DAMPER_BUSY;
	} else {
		CURRENTSTATE(Motor) = stepper_internal[Motor].GoalState;
	}
}

void stepper_step_angle(uint8_t Motor, float angle, int direction) {
	//float anglepersequence = 0.703125;  // 360 = 512 sequences
	//float anglepersequence = 10;  // 360 = 36 sequences



	//The idea is that if you want just open or closed then no need to reverse drive just drive hard ie full range plus a bit in given direction
	//if you want an exact angle then reverse a full amount and then go to given angle
	if((angle == DAMPER_OPEN)||(angle == DAMPER_CLOSE))
		//stepper_internal[Motor].numberofsequences = (int) (180 / anglepersequence);
		if(Motor==STEPPER_EXPANTION)
			stepper_internal[Motor].numberofsequences = (int) 90;
		else
			stepper_internal[Motor].numberofsequences = (int) (720/2);
	else
	{
		int16_t Diff = angle- CURRENTSTATE(Motor);

		if(Diff>0){
			direction = DAMPER_COUNTER_CLOCKWISE;
			stepper_internal[Motor].numberofsequences = (int) (Diff);
		}
		else{
			direction = DAMPER_CLOCKWISE;
			stepper_internal[Motor].numberofsequences = (int) (Diff)*-1;
		}

	}


	/*
	if((angle == DAMPER_OPEN)||(angle == DAMPER_CLOSE))
		stepper_internal[Motor].numberofrevsequences = 0;
	else
		stepper_internal[Motor].numberofrevsequences = (int) 180 / anglepersequence;

*/

	stepper_internal[Motor].GoalState =angle;
	stepper_internal[Motor].direction = direction;
}

void StartDamperTask(void *argument) {
	/* Infinite loop */

	const portTickType xFrequency = 4;
	SystemState->LastRunTime.xLastWakeTime_Damper=xTaskGetTickCount();


	for (;;) {

		if(!CHECKBIT(SystemState->TASKRUNBITS,RUNBIT_DAMPER_TASK))
		{
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Damper, 10000);
			continue;
		}


		if (CHECK(STEPPER_RETRUN)) { //The required state and current state not the same
			if (CURRENTSTATE(STEPPER_RETRUN) != DAMPER_BUSY) {
				if(REQUIREDSTATE(STEPPER_RETRUN)==DAMPER_CLOSE)
					stepper_step_angle(STEPPER_RETRUN, REQUIREDSTATE(STEPPER_RETRUN), DAMPER_CLOCKWISE);
				else if (REQUIREDSTATE(STEPPER_RETRUN)==DAMPER_OPEN)
					stepper_step_angle(STEPPER_RETRUN, REQUIREDSTATE(STEPPER_RETRUN), DAMPER_COUNTER_CLOCKWISE);
				else
					stepper_step_angle(STEPPER_RETRUN, REQUIREDSTATE(STEPPER_RETRUN), DAMPER_COUNTER_CLOCKWISE);
			}
			ProcessStepper(STEPPER_RETRUN);
		}


		if (CHECK(STEPPER_FRESH)) { //The required state and current state not the same
			if (CURRENTSTATE(STEPPER_FRESH) != DAMPER_BUSY) {
				if(REQUIREDSTATE(STEPPER_FRESH)==DAMPER_CLOSE)
					stepper_step_angle(STEPPER_FRESH, REQUIREDSTATE(STEPPER_FRESH), DAMPER_CLOCKWISE);
				else if (REQUIREDSTATE(STEPPER_FRESH)==DAMPER_OPEN)
					stepper_step_angle(STEPPER_FRESH, REQUIREDSTATE(STEPPER_FRESH), DAMPER_COUNTER_CLOCKWISE);
				else
					stepper_step_angle(STEPPER_FRESH, REQUIREDSTATE(STEPPER_FRESH), DAMPER_COUNTER_CLOCKWISE);
			}
			ProcessStepper(STEPPER_FRESH);
		}


		if (CHECK(STEPPER_EXPANTION)) { //The required state and current state not the same
			if (CURRENTSTATE(STEPPER_EXPANTION) != DAMPER_BUSY) {
				if(REQUIREDSTATE(STEPPER_EXPANTION)==DAMPER_CLOSE)
					stepper_step_angle(STEPPER_EXPANTION, REQUIREDSTATE(STEPPER_EXPANTION), DAMPER_CLOCKWISE);
				else if (REQUIREDSTATE(STEPPER_EXPANTION)==DAMPER_OPEN)
					stepper_step_angle(STEPPER_EXPANTION, REQUIREDSTATE(STEPPER_EXPANTION), DAMPER_COUNTER_CLOCKWISE);
				else
					stepper_step_angle(STEPPER_EXPANTION, REQUIREDSTATE(STEPPER_EXPANTION), DAMPER_COUNTER_CLOCKWISE);
			}
			ProcessStepper(STEPPER_EXPANTION);
		}

		if( (CURRENTSTATE(STEPPER_RETRUN)!=DAMPER_BUSY)&&(CURRENTSTATE(STEPPER_FRESH)!=DAMPER_BUSY)&&(CURRENTSTATE(STEPPER_EXPANTION)!=DAMPER_BUSY))
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Damper, 1000);
		else
			vTaskDelayUntil(&SystemState->LastRunTime.xLastWakeTime_Damper, xFrequency);


	}
}
